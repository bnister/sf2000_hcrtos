#define LOG_TAG "MTD"
#define ELOG_OUTPUT_LVL ELOG_LVL_ERROR

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/mtd/mtd.h>

#include <nuttx/mtd/mtd.h>
#include <nuttx/drivers/drivers.h>
#include <nuttx/fs/fs.h>
#include <kernel/elog.h>
#include <kernel/lib/fdt_api.h>
#include <kernel/list.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define MTDNOR_TEST 0

#ifndef max
#define max(a, b) (((a) > (b)) ? (a) : (b))
#endif

LIST_HEAD(__mtd_idr);
static int __mtd_idx = 0;

struct hc_flash_dev {
	struct mtd_dev_s nuttx_mtd;
	struct mtd_info *mtd;
};

/*
 * NOTE: startblock here is in units of erasesize
 * Return:
 *    0:      success
 *    others: fail
 */
static int hc_flash_erase(struct mtd_dev_s *dev, off_t startblock,
			  size_t nblocks)
{
	struct hc_flash_dev *priv = (struct hc_flash_dev *)dev;
	struct mtd_info *mtd = priv->mtd;
	struct erase_info instr = { 0 };

	instr.addr = mtd->erasesize * startblock;
	instr.len = mtd->erasesize * nblocks;

	return mtd_erase(mtd, &instr);
}

/*
 * NOTE: startblock here is in units of blocksize
 * Return:
 *    Number of read blocks
 */
static ssize_t hc_flash_bread(struct mtd_dev_s *dev, off_t startblock,
			      size_t nblocks, uint8_t *buf)
{
	struct hc_flash_dev *priv = (struct hc_flash_dev *)dev;
	struct mtd_info *mtd = priv->mtd;
	uint32_t blocksize = dev->writesize;
	loff_t from;
	size_t len;
	size_t retlen = 0;
	int ret;

	from = startblock * blocksize;
	len = nblocks * blocksize;

	ret = mtd_read(mtd, from, len, &retlen, buf);

	if (ret)
		return ret;

	if (retlen != len)
		return -EIO;

	return retlen / blocksize;
}

/*
 * Return:
 *    Number of read bytes
 */
static ssize_t hc_flash_read(struct mtd_dev_s *dev, off_t offset, size_t nbytes,
			     uint8_t *buf)
{
	struct hc_flash_dev *priv = (struct hc_flash_dev *)dev;
	struct mtd_info *mtd = priv->mtd;
	size_t retlen = 0;
	int ret;

	ret = mtd_read(mtd, offset, nbytes, &retlen, buf);

	if (ret)
		return ret;

	if (retlen != nbytes)
		return -EIO;

	/* unit: number of bytes */
	return retlen;
}

/*
 * NOTE: startblock here is in units of blocksize
 * Return:
 *    Number of written blocks
 */
static ssize_t hc_flash_bwrite(struct mtd_dev_s *dev, off_t startblock,
			       size_t nblocks, const uint8_t *buf)
{
	struct hc_flash_dev *priv = (struct hc_flash_dev *)dev;
	struct mtd_info *mtd = priv->mtd;
	uint32_t blocksize = dev->writesize;
	loff_t to;
	size_t len;
	ssize_t retlen = 0;
	int ret;

	to = startblock * blocksize;
	len = nblocks * blocksize;

	ret = mtd_write(mtd, to, len, &retlen, buf);
	if (ret)
		return ret;

	return retlen / blocksize;
}

static ssize_t hc_flash_write(struct mtd_dev_s *dev, off_t offset,
			      size_t nbytes, const uint8_t *buf)
{
	struct hc_flash_dev *priv = (struct hc_flash_dev *)dev;
	struct mtd_info *mtd = priv->mtd;
	ssize_t retlen = 0;
	int ret;

	ret = mtd_write(mtd, offset, nbytes, &retlen, buf);

	if (ret)
		return ret;

	return retlen;
}

static int hc_flash_read_oob(FAR struct mtd_dev_s *dev, off_t offset,
			     uint8_t *data, int data_len,
			     uint8_t *oob, int oob_len)
{
	struct hc_flash_dev *priv = (struct hc_flash_dev *)dev;
	struct mtd_info *mtd = priv->mtd;
	loff_t from = (loff_t)offset;
	struct mtd_oob_ops ops;

	memset(&ops, 0, sizeof(ops));
	ops.mode = MTD_OPS_AUTO_OOB;
	ops.len = (data) ? data_len : 0;
	ops.ooblen = oob_len;
	ops.datbuf = data;
	ops.oobbuf = oob;

	return mtd_read_oob(mtd, from, &ops);
}

static int hc_flash_write_oob(FAR struct mtd_dev_s *dev, off_t offset,
			      const uint8_t *data, int data_len,
			      const uint8_t *oob, int oob_len)
{
	struct hc_flash_dev *priv = (struct hc_flash_dev *)dev;
	struct mtd_info *mtd = priv->mtd;
	loff_t to = (loff_t)offset;
	struct mtd_oob_ops ops;

	if (!data || !data_len) {
		data = NULL;
		data_len = 0;
	}

	if (!oob || !oob_len) {
		oob = NULL;
		oob_len = 0;
	}

	memset(&ops, 0, sizeof(ops));
	ops.mode = MTD_OPS_AUTO_OOB;
	ops.len = (data) ? data_len : 0;
	ops.ooblen = oob_len;
	ops.datbuf = (u8 *)data;
	ops.oobbuf = (u8 *)oob;

	return mtd_write_oob(mtd, to, &ops);
}

/* NOTE: startblock here is in units of erasesize */
static int hc_flash_block_isbad(FAR struct mtd_dev_s *dev, off_t startblock)
{
	struct hc_flash_dev *priv = (struct hc_flash_dev *)dev;
	struct mtd_info *mtd = priv->mtd;

	return mtd_block_isbad(mtd, startblock * mtd->erasesize);
}

/* NOTE: startblock here is in units of erasesize */
static int hc_flash_block_markbad(FAR struct mtd_dev_s *dev, off_t startblock)
{
	struct hc_flash_dev *priv = (struct hc_flash_dev *)dev;
	struct mtd_info *mtd = priv->mtd;

	return mtd_block_markbad(mtd, startblock * mtd->erasesize);
}

static int hc_flash_ioctl(struct mtd_dev_s *dev,
                             int cmd, unsigned long arg)
{
	struct hc_flash_dev *priv = (struct hc_flash_dev *)dev;
	struct mtd_info *mtd = priv->mtd;
	int ret = OK;

	switch (cmd) {
	case MTDIOC_GEOMETRY: {
		struct mtd_geometry_s *geo =
			(struct mtd_geometry_s *)((uintptr_t)arg);
		if (geo) {
			geo->blocksize = dev->writesize;
			geo->erasesize = dev->erasesize;
			geo->neraseblocks = dev->size / dev->erasesize;
		}
		break;
	}

	case MTDIOC_BULKERASE: {
		struct erase_info instr = { 0 };
		instr.addr = 0;
		instr.len = mtd->size;
		mtd_erase(mtd, &instr);
		break;
	}

	case MTDIOC_MEMERASE: {
		struct mtd_eraseinfo_s *eraseinfo = (struct mtd_eraseinfo_s *)arg;
		struct erase_info instr = { 0 };
		instr.addr = eraseinfo->start;
		instr.len = eraseinfo->length;
		ret = mtd_erase(mtd, &instr);
		break;
	}

	default:
		ret = -ENOTTY;
		break;
	}

	return ret;
}

int mtd_device_register(struct mtd_info *mtd, const char *node)
{
	struct hc_flash_dev *priv;
	struct mtd_dev_s *master;
	struct mtd_dev_s *part;
	off_t nblocks;
	off_t firstblock;
	u32 start, size, npart;
	char strbuf[128];
	const char *partname;
	int np;
	int ret = OK;
	int i;

	priv = kzalloc(sizeof(struct hc_flash_dev), GFP_KERNEL);
	if (priv == NULL) {
		log_e("Not enough memory!\n");
		return -ENOMEM;
	}

	priv->mtd = mtd;

	priv->nuttx_mtd.erase = hc_flash_erase;
	priv->nuttx_mtd.bread = hc_flash_bread;
	priv->nuttx_mtd.bwrite = hc_flash_bwrite;
	priv->nuttx_mtd.read = hc_flash_read;
	priv->nuttx_mtd.block_isbad = (mtd->_block_isbad == NULL) ? NULL : hc_flash_block_isbad;
	priv->nuttx_mtd.block_markbad = (mtd->_block_markbad == NULL) ? NULL : hc_flash_block_markbad;
	priv->nuttx_mtd.read_oob = (mtd->_read_oob == NULL) ? NULL : hc_flash_read_oob;
	priv->nuttx_mtd.write_oob = (mtd->_write_oob == NULL) ? NULL : hc_flash_write_oob;
#ifdef CONFIG_MTD_BYTE_WRITE
	priv->nuttx_mtd.write = hc_flash_write;
#endif
	priv->nuttx_mtd.ioctl = hc_flash_ioctl;
	priv->nuttx_mtd.name = "mtd-master";

	master = (struct mtd_dev_s *)priv;

	master->type = mtd->type;
	master->size = mtd->size;
	master->erasesize = mtd->erasesize;
	master->writesize = (mtd->type == MTD_NORFLASH) ? mtd->erasesize : mtd->writesize;
	master->oobsize = mtd->oobsize;
	master->oobavail = mtd->oobavail;
	master->ecc_strength = mtd->ecc_strength;

	printf("Flash Geometry:\n");
	printf("  blocksize:      %lu\n", (unsigned long)master->writesize);
	printf("  erasesize:      %lu\n", (unsigned long)master->erasesize);
	printf("  neraseblocks:   %lu\n", (unsigned long)(master->size / master->erasesize));
	printf("  chipsize:       0x%llx\n", master->size);

	memset(strbuf, 0, sizeof(strbuf));
	snprintf(strbuf, sizeof(strbuf), "%s/partitions", node);
	np = fdt_node_probe_by_path(strbuf);
	if (np < 0)
		return 0;

	npart = 0;
	fdt_get_property_u_32_index(np, "part-num", 0, &npart);

	for (i = 0; i <= (int)npart; i++) {
		start = 0;
		size = 0;
		partname = NULL;
		memset(strbuf, 0, sizeof(strbuf));
		snprintf(strbuf, sizeof(strbuf), "part%d-reg", i);
		fdt_get_property_u_32_index(np, strbuf, 0, &start);
		fdt_get_property_u_32_index(np, strbuf, 1, &size);
		snprintf(strbuf, sizeof(strbuf), "part%d-label", i);
		fdt_get_property_string_index(np, strbuf, 0, &partname);

		if (i == 0 && size == 0) {
			/* part0 is for entire flash by default */
			start = 0;
			size = master->size;
			partname = "mtd_master";
		}

		firstblock = start / master->writesize;
		nblocks = size / master->writesize;
		part = mtd_partition(master, firstblock, nblocks);
		if (!part) {
			log_e("ERROR: mtd_partition failed. firstblock=%lu nblocks=%lu\n",
					(unsigned long)firstblock, (unsigned long)nblocks);
			ret = -1;
			break;
		}

		part->name = partname;
		part->index = i + __mtd_idx;

		memset(strbuf, 0, sizeof(strbuf));
		snprintf(strbuf, sizeof(strbuf), "/dev/mtdblock%d", part->index);
		ret = register_mtddriver(strbuf, part, 0, part);
		if (ret < 0) {
			log_e("ERROR: register_mtddriver %s failed: %d\n", strbuf, ret);
			break;
		}

		list_add_tail(&part->node, &__mtd_idr);
	}

	__mtd_idx += i;

	return ret;
}

struct mtd_dev_s *get_mtd_device_nm(const char *name)
{
	struct mtd_dev_s *curr, *next;

	list_for_each_entry_safe (curr, next, &__mtd_idr, node) {
		if (curr->name && !strncmp(curr->name, name, max(strlen(curr->name), strlen(name)))) {
			return curr;
		}
	}

	return NULL;
}
