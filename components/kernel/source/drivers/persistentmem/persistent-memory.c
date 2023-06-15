#define LOG_TAG "persistmem"
#define ELOG_OUTPUT_LVL ELOG_LVL_ERROR

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <nuttx/fs/fs.h>
#include <kernel/module.h>
#include <kernel/lib/fdt_api.h>
#include <kernel/lib/crc32.h>
#include <kernel/elog.h>
#include <nuttx/mtd/mtd.h>
#include <linux/mutex.h>
#include <hcuapi/persistentmem.h>
#include <linux/slab.h>

#define DRIVER_NAME "persistentmem"

#ifndef UINT32_MAX
#define UINT32_MAX             (4294967295U)
#endif

#ifndef max
#define max(a, b) (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif

struct norflash_block {
	uint32_t ofs;
	uint32_t ofs_next;
	struct list_head nodes;
	uint8_t *buffer;
	uint32_t buf_size;

	uint32_t norflash_address;
	uint32_t norflash_next_address;
	uint32_t nor_size;
};

struct nodeinfo {
	/*
	 * logical block length, for data recovery usage,
	 * we put the two bytes of len at the head of the block.
	 */
	uint16_t len;

	/*
	 * logical block start offset
	 */
	uint16_t start;
};

/* The size of "nodeinfo + crc" */
#define EXTRA_SIZE  (sizeof(struct nodeinfo) + sizeof(uint32_t))

struct bufnode {
	struct list_head node;
	struct nodeinfo info;
};

enum node_status { NODE_OK, NODE_CORRUPTED, NODE_END };

struct persistentmem_device {
	size_t size;
	const char *mtdname;
	struct mtd_dev_s *mtd;
	size_t mtd_part_size;
	size_t mtd_erasesize;
	struct norflash_block blk;
	struct mutex files_lock;
};

struct vnode {
	uint16_t id;
	uint16_t size;
};

static struct persistentmem_device persistentmem_dev = { 0 };
static struct persistentmem_device *gdev = &persistentmem_dev;

static int flash_read_wrap(uint32_t start_addr, uint32_t read_size,
			   uint32_t *actual_size, uint8_t *buf)
{
	uint32_t retsize;

	retsize = MTD_READ(gdev->mtd, start_addr, read_size, buf);
	if (retsize < read_size)
		return -1;

	*actual_size = retsize;

	return 0;
}

static int flash_write_wrap(uint32_t start_addr, uint32_t write_size,
			    uint32_t *actual_size, uint8_t *buf)
{
	uint32_t retsize;
	
	retsize = MTD_WRITE(gdev->mtd, start_addr, write_size, buf);
	if (retsize < write_size)
		return -1;

	*actual_size = retsize;

	return 0;
}

static int flash_erase_wrap(unsigned long start_addr, unsigned long erase_size)
{
	off_t startblock;
	size_t nblocks;

	startblock = start_addr / gdev->mtd_erasesize;
	nblocks = erase_size / gdev->mtd_erasesize;

	return MTD_ERASE(gdev->mtd, startblock, nblocks);
}

static int erase_block(struct norflash_block *blk)
{
	flash_erase_wrap(blk->norflash_address, blk->nor_size);
	blk->ofs = 0;
	return 0;
}

static int erase_next_block(struct norflash_block *blk)
{
	flash_erase_wrap(blk->norflash_next_address, blk->nor_size);
	blk->ofs_next = 0;
	return 0;
}

static int update_block(struct norflash_block *blk,
			int start, int len, uint8_t *buffer)
{
	uint32_t crc;
	uint32_t actlen;
	uint8_t buf[4];
	uint32_t i;

	if (blk->ofs + 4 + (uint32_t)len > blk->nor_size) {
		log_e("[%s:%d]BUG: memory overflow!\n", __FILE__, __LINE__);
		return -1;
	}

	i = 0;
	buf[i++] = ((len >> 8) & 0xff);
	buf[i++] = (len & 0xff);
	buf[i++] = ((start >> 8) & 0xff);
	buf[i++] = (start & 0xff);

	flash_write_wrap(blk->norflash_address + blk->ofs, i, &actlen, buf);

	blk->ofs += i;

	flash_write_wrap(blk->norflash_address + blk->ofs, len, &actlen,
			 buffer);

	blk->ofs += len;

	crc = crc32(UINT32_MAX, buffer, len);
	flash_write_wrap(blk->norflash_address + blk->ofs, sizeof(crc), &actlen,
			 (uint8_t *)&crc);

	blk->ofs += sizeof(crc);

	return 0;
}

static int update_next_block(struct norflash_block *blk,
			int start, int len, uint8_t *buffer)
{
	uint32_t crc;
	uint32_t actlen;
	uint8_t buf[4];
	uint32_t i;

	if (blk->ofs_next + 4 + len > blk->nor_size) {
		log_e("[%s:%d]BUG: memory overflow!\n", __FILE__, __LINE__);
		return -1;
	}

	i = 0;
	buf[i++] = ((len >> 8) & 0xff);
	buf[i++] = (len & 0xff);
	buf[i++] = ((start >> 8) & 0xff);
	buf[i++] = (start & 0xff);

	flash_write_wrap(blk->norflash_next_address + blk->ofs_next, i, &actlen,
			 buf);

	blk->ofs_next += i;

	flash_write_wrap(blk->norflash_next_address + blk->ofs_next, len,
			 &actlen, buffer);

	blk->ofs_next += len;

	crc = crc32(UINT32_MAX, buffer, len);
	flash_write_wrap(blk->norflash_next_address + blk->ofs_next,
			 sizeof(crc), &actlen, (uint8_t *)&crc);

	blk->ofs_next += sizeof(crc);

	return 0;
}

static int switch_block(struct norflash_block *blk)
{
	uint32_t t_ofs;
	uint32_t t_norflash_address;

	t_norflash_address = blk->norflash_address;
	blk->norflash_address = blk->norflash_next_address;
	blk->norflash_next_address = t_norflash_address;

	t_ofs = blk->ofs;
	blk->ofs = blk->ofs_next;
	blk->ofs_next = t_ofs;

	return 0;
}

static enum node_status check_node(uint8_t *pbuf)
{
	uint16_t len, start;
	uint32_t crc, crc_data;

	len = pbuf[0] << 8 | pbuf[1];

	if (len == 0xffff) {
		start = pbuf[2] << 8 | pbuf[3];
		if (start == 0xffff)
			return NODE_END; /* End of nodes. Unused free space from here. */
		else
			return NODE_CORRUPTED;
	}

	memcpy(&crc_data, pbuf + 4 + len, sizeof(crc_data));
	crc = crc32(UINT32_MAX, pbuf + 4, len);
	if (crc != crc_data) {
		/*
		 * A bad node. Need to skip this node region.
		 */
		return NODE_CORRUPTED;
	} else {
		/*
		 * A good node.
		 */
		return NODE_OK;
	}
}

static int merge_node(struct norflash_block *blk, struct nodeinfo *pinfo)
{
	struct bufnode *pnode;
	struct list_head *pcur, *ptmp;

	pnode = malloc(sizeof(*pnode));
	assert(pnode != NULL);

	memset(pnode, 0, sizeof(*pnode));

	pnode->info = *pinfo;

	if (list_empty(&blk->nodes)) {
		list_add_tail(&pnode->node, &blk->nodes);
	} else {
		list_for_each_safe (pcur, ptmp, &blk->nodes) {
			struct bufnode *p = (struct bufnode *)pcur;
			uint16_t start1 = p->info.start;
			uint16_t end1 = p->info.start + p->info.len;
			uint16_t start2 = pnode->info.start;
			uint16_t end2 = pnode->info.start + pnode->info.len;

			if (!((start1 > end2) || (end1 < start2))) {
				start2 = min(start1, start2);
				end2 = max(end1, end2);
				pnode->info.start = start2;
				pnode->info.len = end2 - start2;
				list_del(pcur);
				free(pcur);
			}
		}

		list_add_tail(&pnode->node, &blk->nodes);
	}

	return 0;
}

static int update_buffer(struct norflash_block *blk,
			int start, int len, uint8_t *buffer)
{
	struct nodeinfo info;

	if (start + len > (int)blk->buf_size) {
		log_e("[%s:%d]BUG: memory overflow! start 0x%08x, len %d, size %ld\n",
		       __FILE__, __LINE__, start, len, blk->buf_size);
		return 0;
	}

	memcpy(blk->buffer + start, buffer, len);

	info.start = start;
	info.len = len;

	merge_node(blk, &info);

	return len;
}

static int sync_to_next_block(struct norflash_block *blk)
{
	struct list_head *pnode, *ptmp;

	if (!list_empty(&blk->nodes)) {
		list_for_each_safe (pnode, ptmp, &blk->nodes) {
			struct bufnode *p = (struct bufnode *)pnode;

			update_next_block(blk, p->info.start, p->info.len,
					  blk->buffer + p->info.start);
		}
	}

	return 0;
}

static enum node_status scan_block(struct norflash_block *blk)
{
	uint8_t *pnode_head;
	uint16_t len, start;
	enum node_status ret = NODE_END;
	uint8_t *buf;
	uint32_t actlen;

	buf = malloc(blk->nor_size);
	assert(buf != NULL);

	flash_read_wrap(blk->norflash_address, blk->nor_size, &actlen, buf);
	pnode_head = buf;
	blk->ofs = 0;

	for (;;) {
		switch (check_node(pnode_head)) {
		case NODE_END:
			/*
			 * norflash scan over.
			 */
			ret = NODE_END;
			goto scan_over;

		case NODE_OK:
			len = pnode_head[0] << 8 | pnode_head[1];
			start = pnode_head[2] << 8 | pnode_head[3];

			update_buffer(blk, start, len, pnode_head + 4);

			break;

		case NODE_CORRUPTED:
			ret = NODE_CORRUPTED;
			goto scan_over;

		default:
			break;
		}

		if (blk->ofs + len + EXTRA_SIZE >= blk->nor_size) {
			/*
			 * norflash scan over.
			 */
			ret = NODE_END;
			goto scan_over;
		} else {
			blk->ofs += len + EXTRA_SIZE;
			pnode_head += len + EXTRA_SIZE;
		}
	}

scan_over:
	free(buf);

	return ret;
}

static void reset_block_and_nodes(void)
{
	struct list_head *pnode, *ptmp;

	if (gdev->blk.buffer) {
		memset(gdev->blk.buffer, 0, gdev->blk.buf_size);
	}

	if (!list_empty(&gdev->blk.nodes)) {
		list_for_each_safe(pnode, ptmp, &gdev->blk.nodes) {
			list_del(pnode);
			free(pnode);
		}
	}
}

static int prepare_block(struct norflash_block *blk)
{
	uint8_t *buf;
	uint32_t crc, crc_data;
	uint32_t ncrc, ncrc_data;
	bool blk_is_new, nblk_is_new;
	uint32_t i;
	uint32_t actlen;

	buf = malloc(blk->nor_size*2);
	assert(buf != NULL);

	flash_read_wrap(blk->norflash_address, blk->nor_size*2, &actlen, buf);

	memcpy(&crc_data, buf + blk->nor_size - 4, sizeof(crc_data));
	crc = crc32(UINT32_MAX, buf, blk->nor_size - 4);

	memcpy(&ncrc_data, buf + blk->nor_size * 2 - 4, sizeof(crc_data));
	ncrc = crc32(UINT32_MAX, buf + blk->nor_size, blk->nor_size - 4);

	assert(!(crc == crc_data && ncrc == ncrc_data));

	blk_is_new = true;
	nblk_is_new = true;

	for (i = 0; i < blk->nor_size; i++) {
		if ((*(buf + i)) != 0xff) {
			blk_is_new = false;
			break;
		}
	}

	for (i = 0; i < blk->nor_size; i++) {
		if ((*(buf + blk->nor_size + i)) != 0xff) {
			nblk_is_new = false;
			break;
		}
	}

	if (blk_is_new && nblk_is_new)
		return -1;

	if (blk_is_new) {
		/*
		 * norflash is new erased block.
		 * So, use norflash_next.
		 */
		log_d("Now, use norflash_next!\n");

		switch_block(blk);

		if (scan_block(blk) == NODE_CORRUPTED) {
			erase_next_block(blk);
			sync_to_next_block(blk);
			erase_block(blk);
			switch_block(blk);
			//reset_block_and_nodes();
			//erase_block(blk);
			//assert(scan_block(blk) == NODE_END);
		}
		free(buf);

		return 0;
	}

	if (nblk_is_new) {
		/*
		 * norflash_next is new erased block.
		 * So, use norflash.
		 */
		log_d("Now, use norflash!\n");

		if (scan_block(blk) == NODE_CORRUPTED) {
			erase_next_block(blk);
			sync_to_next_block(blk);
			erase_block(blk);
			switch_block(blk);
			//reset_block_and_nodes();
			//erase_block(blk);
			//assert(scan_block(blk) == NODE_END);
		}
		free(buf);

		return 0;
	}

	if (crc == crc_data) {
		/*
		 * norflash write complete, but switch to
		 * norflash_next not finished.
		 * Now, finished the switch again.
		 */
		log_d("norflash write complete, but switch to\n"
			 "norflash_next not finished.\n"
			 "Now, finished the switch again.\n");

		scan_block(blk);
		erase_next_block(blk);
		sync_to_next_block(blk);
		erase_block(blk);
		switch_block(blk);
		free(buf);

		return 0;
	}

	if (ncrc == ncrc_data) {
		/*
		 * norflash_next write complete, but switch to
		 * norflash not finished.
		 * Now, finished the switch again.
		 */
		log_d("norflash_next write complete, but switch to\n"
			 "norflash not finished.\n"
			 "Now, finished the switch again.\n");

		switch_block(blk);
		scan_block(blk);
		erase_next_block(blk);
		sync_to_next_block(blk);
		erase_block(blk);
		switch_block(blk);
		free(buf);

		return 0;
	}

	/*
	 * BUG: Because the erase operation may not be completely finished.
	 * both norflash and norflash_next may be broken!!!
	 * Need to check which one need to complete the erase operation.
	 */
	log_e("BUG: Because the erase operation may not be completely finished.\n");
	log_e("both norflash and norflash_next may be broken!!!\n");
	log_e("Need to check which one need to complete the erase operation.\n");

	blk_is_new = true;
	nblk_is_new = true;

	for (i = 0; i < 4; i++) {
		if ((*(buf + i)) != 0xff) {
			blk_is_new = false;
			break;
		}
	}

	for (i = 0; i < 4; i++) {
		if ((*(buf + blk->nor_size + i)) != 0xff) {
			nblk_is_new = false;
			break;
		}
	}

	if (!blk_is_new && nblk_is_new) {
		assert(scan_block(blk) == NODE_END);
		erase_next_block(blk);
	} else if (blk_is_new && !nblk_is_new) {
		switch_block(blk);
		assert(scan_block(blk) == NODE_END);
		erase_next_block(blk);
	} else {
		asm volatile("nop; .word 0x1000ffff; nop;");
		erase_block(blk);
		erase_next_block(blk);
	}

	free(buf);

	return 0;
}

static int free_size(struct norflash_block *blk)
{
	if ((blk->ofs + EXTRA_SIZE + sizeof(uint32_t)) < blk->nor_size)
		return (blk->nor_size - EXTRA_SIZE - blk->ofs - sizeof(uint32_t));
	else
		return 0;
}

static int wear_level_memory_write(struct norflash_block *blk, int start,
				   int len, uint8_t *buffer)
{
	uint8_t *buf;
	uint32_t crc;
	uint32_t actlen;

	if (len == 0)
		return 0;

	if (len > free_size(blk)) {
		/*
		 * save crc of whole block.
		 */
		buf = malloc(blk->nor_size);
		assert(buf != NULL);
		flash_read_wrap(blk->norflash_address, blk->nor_size, &actlen,
				buf);

		log_d("buf:%p, blk->nor_size:0x%lx\n", buf, blk->nor_size);
		crc = crc32(UINT32_MAX, buf, blk->nor_size - 4);
		free(buf);

		flash_write_wrap(blk->norflash_address + blk->nor_size - 4,
				 sizeof(crc), &actlen, (uint8_t *)&crc);

		/*
		 * update buffer to next block.
		 */
		update_buffer(blk, start, len, buffer);
		sync_to_next_block(blk);

		/*
		 * erase current block.
		 */
		erase_block(blk);

		/*
		 * switch to next block
		 */
		switch_block(blk);

		return len;
	}

	/* Update write operation to NorFlash */
	update_block(blk, start, len, buffer);

	/* Update write operation to temperory norflash buffer */
	update_buffer(blk, start, len, buffer);

	return len;
}

static int wear_level_memory_read(struct norflash_block *blk,
		int start, int len, uint8_t *buffer)
{
	if (len == 0)
		return 0;

	if (start + len > (int)blk->buf_size) {
		log_e("[%s:%d]BUG: memory overflow!\n", __FILE__, __LINE__);
		return -1;
	}

	memcpy(buffer, blk->buffer + start, len);

	return len;
}

static void flash_wear_leveling_exit(void)
{
	struct list_head *pnode, *ptmp;

	if (gdev->blk.buffer) {
		free(gdev->blk.buffer); 
		gdev->blk.buffer = NULL;
	}

	if (!list_empty(&gdev->blk.nodes)) {
		list_for_each_safe(pnode, ptmp, &gdev->blk.nodes) {
			list_del(pnode);
			free(pnode);
		}
	}
}

static int flash_wear_leveling_init(void)
{
	uint8_t *buffer = NULL;
	int ret = 0;

	buffer = (uint8_t *)malloc(gdev->size);
	if (!buffer) {
		log_e("malloc failed!\n");
		ret = -ENOMEM;
		goto error;
	}
	memset(buffer, 0x0, gdev->size);

	gdev->blk.buffer = buffer;
	gdev->blk.buf_size = gdev->size;
	gdev->blk.norflash_address = 0;
	gdev->blk.norflash_next_address = gdev->mtd_part_size >> 1;
	gdev->blk.nor_size = gdev->mtd_part_size >> 1;
	INIT_LIST_HEAD(&gdev->blk.nodes);

	ret = prepare_block(&gdev->blk);
	if (ret) {
		log_e("No data to read\n");
	}

	log_d("Flash init success!\n");
	return 0;

error:
	if (buffer) {
		free(buffer);
		gdev->blk.buffer = NULL;
		buffer = NULL;	
	}

	return ret;
}

static bool __persistentmem_read(const uint32_t addr, uint8_t *const data,
				 const uint32_t data_len)
{
	int len;

	mutex_lock(&gdev->files_lock);

	len = wear_level_memory_read(&gdev->blk, addr, data_len, data);

	if (len != (int)data_len) {
		mutex_unlock(&gdev->files_lock);
		return false;
	}

	mutex_unlock(&gdev->files_lock);
	return true;
}

static bool __persistentmem_write(const uint32_t addr,
				  const uint8_t *const data,
				  const uint32_t data_len)
{
	int len;

	len = wear_level_memory_write(&gdev->blk, addr, data_len, (uint8_t *)data);

	if (len != (int)data_len) {
		mutex_unlock(&gdev->files_lock);
		log_e("Write error, len:%d, data_len:%ld\n", len, data_len);
		return false;
	}

	mutex_unlock(&gdev->files_lock);
	return true;
}

static ssize_t persistentmem_read(struct file *file, char *buf, size_t nbytes)
{
	uint32_t addr = file->f_pos;

	if (!__persistentmem_read(addr, buf, nbytes)) {
		return -EFAULT;
	}

	file->f_pos += nbytes;

	return nbytes;
}

static ssize_t persistentmem_write(struct file *file, const char *buf,
				   size_t nbytes)
{
	uint32_t addr = file->f_pos;

	if (!__persistentmem_write(addr, buf, nbytes)) {
		return -EFAULT;
	}

	file->f_pos += nbytes;

	return nbytes;
}

static off_t persistentmem_seek(struct file *file, off_t offset, int whence)
{
	off_t newpos;

	switch(whence) {
	case SEEK_SET:
		newpos = offset;
		break;
	case SEEK_CUR:
		newpos = file->f_pos + offset;
		break;
	case SEEK_END:
		newpos = gdev->size + offset;
		break;
	default:
		return -EINVAL;
	}

	if (newpos < 0)
		return -EINVAL;

	file->f_pos = newpos;

	return newpos;
}

static void *node_find(void *buffer, uint32_t size, unsigned long id)
{
	struct vnode *pvnode;
	void *p = buffer;

	while (p < buffer + size) {
		pvnode = (struct vnode *)p;
		if (pvnode->id == id)
			return p;
		if (pvnode->id == PERSISTENTMEM_NODE_ID_UNUSED)
			return NULL;

		p += (sizeof(*pvnode) + pvnode->size);
	}

	return NULL;
}

static int node_create(void *buffer, uint32_t size, struct persistentmem_node_create *node)
{
	struct vnode vnode;
	void *p = buffer;
	int len;

	mutex_lock(&gdev->files_lock);

	/* check if id duplicate */
	if (node_find(buffer, size, node->id)) {
		mutex_unlock(&gdev->files_lock);
		return PERSISTENTMEM_ERR_ID_DUPLICATED;
	}

	p = node_find(buffer, size, PERSISTENTMEM_NODE_ID_UNUSED);
	if ((p + sizeof(struct vnode) + node->size) >= (buffer + size)) {
		mutex_unlock(&gdev->files_lock);
		return PERSISTENTMEM_ERR_NO_SPACE;
	}

	vnode.id = node->id;
	vnode.size = node->size;
	len = wear_level_memory_write(&gdev->blk, (int)(p - buffer),
				      sizeof(struct vnode), (uint8_t *)&vnode);
	if (len != sizeof(struct vnode)) {
		mutex_unlock(&gdev->files_lock);
		log_e("Write error, len:%d, data_len:%ld", len, sizeof(struct vnode));
		return PERSISTENTMEM_ERR_FAULT;
	}

	mutex_unlock(&gdev->files_lock);

	return 0;
}

static int node_delete(void *buffer, uint32_t size, unsigned long id)
{
	struct vnode *pvnode;
	void *pcur = NULL;
	void *pnext = NULL;
	void *pend = NULL;
	int len;
	void *tmp;

	mutex_lock(&gdev->files_lock);

	pcur = node_find(buffer, size, id);
	if (!pcur) {
		mutex_unlock(&gdev->files_lock);
		return PERSISTENTMEM_ERR_ID_NOTFOUND;
	}

	pend = node_find(buffer, size, PERSISTENTMEM_NODE_ID_UNUSED);
	pvnode = (struct vnode *)pcur;
	pnext = pcur + sizeof(struct vnode) + pvnode->size;
	tmp = kmalloc(pend - pcur, GFP_KERNEL);
	if (!tmp) {
		mutex_unlock(&gdev->files_lock);
		return -ENOMEM;
	}

	if (pnext == pend) {
		memset(tmp, 0, pend - pcur);
	} else {
		memcpy(tmp, pnext, pend - pnext);
		memset(tmp + (pend - pnext), 0, (pnext - pcur));
	}

	len = wear_level_memory_write(&gdev->blk, (int)(pcur - buffer),
				      (int)(pend - pcur), (uint8_t *)tmp);
	if (len != (pend - pcur)) {
		mutex_unlock(&gdev->files_lock);
		log_e("Write error, len:%d, data_len:%ld", len, pend - pcur);
		return PERSISTENTMEM_ERR_FAULT;
	}

	mutex_unlock(&gdev->files_lock);

	return 0;
}

static int node_get(void *buffer, uint32_t size, struct persistentmem_node *node)
{
	struct vnode *pvnode;

	mutex_lock(&gdev->files_lock);

	pvnode = node_find(buffer, size, node->id);
	if (!pvnode) {
		mutex_unlock(&gdev->files_lock);
		return PERSISTENTMEM_ERR_ID_NOTFOUND;
	}

	if (node->offset + node->size > pvnode->size) {
		mutex_unlock(&gdev->files_lock);
		return PERSISTENTMEM_ERR_OVERFLOW;
	}

	memcpy(node->buf, (char *)pvnode + sizeof(struct vnode) + node->offset, node->size);

	mutex_unlock(&gdev->files_lock);

	return 0;
}

static int node_put(void *buffer, uint32_t size, struct persistentmem_node *node)
{
	struct vnode *pvnode;
	int start;
	uint32_t len;

	mutex_lock(&gdev->files_lock);

	pvnode = node_find(buffer, size, node->id);
	if (!pvnode) {
		mutex_unlock(&gdev->files_lock);
		return PERSISTENTMEM_ERR_ID_NOTFOUND;
	}

	if (node->offset + node->size > pvnode->size) {
		mutex_unlock(&gdev->files_lock);
		return PERSISTENTMEM_ERR_OVERFLOW;
	}

	start = (int)((void *)pvnode + sizeof(struct vnode) + node->offset - buffer);
	len = wear_level_memory_write(&gdev->blk, start, node->size, (uint8_t *)node->buf);
	if (len != node->size) {
		mutex_unlock(&gdev->files_lock);
		log_e("Write error, len:%ld, data_len:%d", len, node->size);
		return PERSISTENTMEM_ERR_FAULT;
	}

	mutex_unlock(&gdev->files_lock);

	return 0;
}

static int persistentmem_ioctl(struct file *filep, int cmd, unsigned long arg)
{
	int rc = 0;

	switch (cmd) {
	case PERSISTENTMEM_IOCTL_NODE_CREATE: {
		struct persistentmem_node_create node;
		memcpy((void *)&node, (void *)arg, sizeof(node));
		node.size = ((node.size + 3) >> 2) << 2;
		rc = node_create(gdev->blk.buffer, gdev->blk.buf_size, &node);
		break;
	}
	case PERSISTENTMEM_IOCTL_NODE_DELETE: {
		rc = node_delete(gdev->blk.buffer, gdev->blk.buf_size, arg);
		break;
	}
	case PERSISTENTMEM_IOCTL_NODE_GET: {
		rc = node_get(gdev->blk.buffer, gdev->blk.buf_size, (struct persistentmem_node *)arg);
		break;
	}
	case PERSISTENTMEM_IOCTL_NODE_PUT: {
		rc = node_put(gdev->blk.buffer, gdev->blk.buf_size, (struct persistentmem_node *)arg);
		break;
	}
	default:
		break;
	}

	return rc;
}

static const struct file_operations persistentmem_fops = {
	.open = dummy_open, /* open */
	.close = dummy_close, /* close */
	.read = persistentmem_read, /* read */
	.write = persistentmem_write, /* write */
	.seek = persistentmem_seek, /* seek */
	.ioctl = persistentmem_ioctl, /* ioctl */
	.poll = NULL /* poll */
#ifndef CONFIG_DISABLE_PSEUDOFS_OPERATIONS
	,
	.unlink = NULL /* unlink */
#endif
};

static int persistentmem_probe(const char *node)
{
	int ret = 0;
	u32 size = 0;
	const char *mtdname = NULL;
	int np;

	np = fdt_node_probe_by_path(node);
	if (np < 0)
		return 0;

	fdt_get_property_u_32_index(np, "size", 0, &size);
	if (size == 0) {
		printf("No size specified for persistent memory\n");
		return 0;
	}

	fdt_get_property_string_index(np, "mtdname", 0, &mtdname);
	if (mtdname == NULL) {
		printf("No mtdblock name specified for persistent memory\n");
		return 0;
	}

	gdev->mtdname = mtdname;
	gdev->mtd = get_mtd_device_nm(mtdname);
	gdev->size = size;
	gdev->mtd_part_size = gdev->mtd->size;
	gdev->mtd_erasesize = gdev->mtd->erasesize;

	ret = flash_wear_leveling_init();
	if (ret)
		goto err_probe;

	mutex_init(&gdev->files_lock);

	ret = register_driver("/dev/persistentmem", &persistentmem_fops, 0666, NULL);
	if (ret) {
		log_e("unable to register persistentmem device\n");
		goto err_probe;
	}

	return ret;

err_probe:
	flash_wear_leveling_exit();
	return ret;
}

static int persistentmem_init(void)
{
	int rc;

	rc = persistentmem_probe("/hcrtos/persistentmem");

	return rc;
}

static int persistentmem_exit(void)
{
	flash_wear_leveling_exit();
	unregister_driver("/dev/persistentmem");
	return 0;
}

module_system(persistentmem, persistentmem_init, persistentmem_exit, 2)
