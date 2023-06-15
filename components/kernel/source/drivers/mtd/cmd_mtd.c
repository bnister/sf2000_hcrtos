#define LOG_TAG "MTDCMD"
#define ELOG_OUTPUT_LVL ELOG_LVL_ERROR

#include <stdio.h>
#include <linux/types.h>
#include <linux/err.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/math.h>
#include <asm-generic/div64.h>
#include <linux/mtd/mtd.h>
#include <nuttx/mtd/mtd.h>
#include <kernel/elog.h>

#include <kernel/lib/console.h>

static struct mtd_dev_s *get_mtd_by_name(const char *name)
{
	struct mtd_dev_s *mtd;

	mtd = get_mtd_device_nm(name);
	if (IS_ERR_OR_NULL(mtd))
		printf("MTD device %s not found, ret %ld\n", name,
		       PTR_ERR(mtd));

	return mtd;
}

static unsigned int mtd_len_to_pages(struct mtd_dev_s *mtd, u64 len)
{
	do_div(len, mtd->writesize);
	return (unsigned int)len;
}

static bool mtd_is_aligned_with_min_io_size(struct mtd_dev_s *mtd, u64 size)
{
	return !do_div(size, mtd->writesize);
}

static bool mtd_is_aligned_with_block_size(struct mtd_dev_s *mtd, u64 size)
{
	return !do_div(size, mtd->erasesize);
}

static void mtd_dump_buf(const u8 *buf, uint len, uint offset)
{
	int i, j;

	for (i = 0; i < len; ) {
		printf("0x%08x:\t", offset + i);
		for (j = 0; j < 8; j++)
			printf("%02x ", buf[i + j]);
		printf(" ");
		i += 8;
		for (j = 0; j < 8; j++)
			printf("%02x ", buf[i + j]);
		printf("\n");
		i += 8;
	}
}

static void mtd_dump_device_buf(struct mtd_dev_s *mtd, u64 start_off,
				const u8 *buf, u64 len, bool woob)
{
	bool has_pages = mtd->type == MTD_NANDFLASH ||
		mtd->type == MTD_MLCNANDFLASH;
	int npages = mtd_len_to_pages(mtd, len);
	uint page;

	if (has_pages) {
		for (page = 0; page < npages; page++) {
			u64 data_off = page * mtd->writesize;

			printf("\nDump %ld data bytes from 0x%08llx:\n",
			       mtd->writesize, start_off + data_off);
			mtd_dump_buf(&buf[data_off],
				     mtd->writesize, start_off + data_off);

			if (woob) {
				u64 oob_off = page * mtd->oobsize;

				printf("Dump %ld OOB bytes from page at 0x%08llx:\n",
				       mtd->oobsize, start_off + data_off);
				mtd_dump_buf(&buf[len + oob_off],
					     mtd->oobsize, 0);
			}
		}
	} else {
		printf("\nDump %lld data bytes from 0x%llx:\n",
		       len, start_off);
		mtd_dump_buf(buf, len, start_off);
	}
}

static void mtd_show_device(struct mtd_dev_s *mtd)
{
	/* Device */
	if (mtd->name)
		printf("* %s\n", mtd->name);

	/* MTD device information */
	printf("  - type: ");
	switch (mtd->type) {
	case MTD_RAM:
		printf("RAM\n");
		break;
	case MTD_ROM:
		printf("ROM\n");
		break;
	case MTD_NORFLASH:
		printf("NOR flash\n");
		break;
	case MTD_NANDFLASH:
		printf("NAND flash\n");
		break;
	case MTD_DATAFLASH:
		printf("Data flash\n");
		break;
	case MTD_UBIVOLUME:
		printf("UBI volume\n");
		break;
	case MTD_MLCNANDFLASH:
		printf("MLC NAND flash\n");
		break;
	case MTD_ABSENT:
	default:
		printf("Unknown\n");
		break;
	}

	printf("  - block size: 0x%lx bytes\n", mtd->erasesize);
	printf("  - min I/O: 0x%lx bytes\n", mtd->writesize);

	if (mtd->oobsize) {
		printf("  - OOB size: %lu bytes\n", mtd->oobsize);
		printf("  - OOB available: %lu bytes\n", mtd->oobavail);
	}
}

/* Logic taken from fs/ubifs/recovery.c:is_empty() */
static bool mtd_oob_write_is_empty(struct mtd_oob_ops *op)
{
	int i;

	for (i = 0; i < op->len; i++)
		if (op->datbuf[i] != 0xff)
			return false;

	for (i = 0; i < op->ooblen; i++)
		if (op->oobbuf[i] != 0xff)
			return false;

	return true;
}

static int do_mtd_list(int argc, char *argv[])
{
	struct mtd_dev_s *curr, *next;
	int dev_nb = 0;
	extern struct list_head __mtd_idr;

	printf("List of MTD devices:\n");

	list_for_each_entry_safe (curr, next, &__mtd_idr, node) {
		mtd_show_device(curr);
		dev_nb++;
	}

	if (!dev_nb) {
		printf("No MTD device found\n");
		return -1;
	}

	return 0;
}

static int mtd_special_write_oob(struct mtd_dev_s *mtd, u64 off,
				 struct mtd_oob_ops *io_op,
				 bool write_empty_pages, bool woob)
{
	int ret = 0;

	/*
	 * By default, do not write an empty page.
	 * Skip it by simulating a successful write.
	 */
	if (!write_empty_pages && mtd_oob_write_is_empty(io_op)) {
		io_op->retlen = mtd->writesize;
		io_op->oobretlen = woob ? mtd->oobsize : 0;
	} else {
		ret = MTD_WRITE_OOB(mtd, off, io_op->datbuf, io_op->len, io_op->oobbuf, io_op->ooblen);
	}

	return ret;
}

int do_mtd_io(int argc, char *argv[])
{
	bool dump, read, raw, woob, write_empty_pages, has_pages = false;
	u64 start_off, off, len, remaining, default_len;
	struct mtd_oob_ops io_op = {};
	uint user_addr = 0, npages;
	const char *cmd = argv[0];
	struct mtd_dev_s *mtd;
	u32 oob_len;
	u8 *buf;
	int ret;

	if (argc < 2)
		return -1;

	mtd = get_mtd_by_name(argv[1]);
	if (IS_ERR_OR_NULL(mtd))
		return -1;

	if (mtd->type == MTD_NANDFLASH || mtd->type == MTD_MLCNANDFLASH)
		has_pages = true;

	dump = !strncmp(cmd, "dump", 4);
	read = dump || !strncmp(cmd, "read", 4);
	raw = strstr(cmd, "_raw");
	woob = strstr(cmd, "_oob");
	write_empty_pages = !has_pages || strstr(cmd, "_dontskipff");

	argc -= 2;
	argv += 2;

	if (!dump) {
		if (!argc) {
			ret = -1;
			goto out_put_mtd;
		}

		user_addr = strtoul(argv[0], NULL, 16);
		argc--;
		argv++;
	}

	start_off = argc > 0 ? strtoul(argv[0], NULL, 16) : 0;
	if (!mtd_is_aligned_with_min_io_size(mtd, start_off)) {
		printf("Offset not aligned with a page (0x%lx)\n",
		       mtd->writesize);
		ret = -1;
		goto out_put_mtd;
	}

	default_len = dump ? mtd->writesize : mtd->size;
	len = argc > 1 ? strtoul(argv[1], NULL, 16) : default_len;
	if (!mtd_is_aligned_with_min_io_size(mtd, len)) {
		len = round_up(len, mtd->writesize);
		printf("Size not on a page boundary (0x%lx), rounding to 0x%llx\n",
		       mtd->writesize, len);
	}

	remaining = len;
	npages = mtd_len_to_pages(mtd, len);
	oob_len = woob ? npages * mtd->oobsize : 0;

	if (dump)
		buf = kmalloc(len + oob_len, GFP_KERNEL);
	else
		buf = (u8 *)user_addr;

	if (!buf) {
		printf("Could not map/allocate the user buffer\n");
		ret = -1;
		goto out_put_mtd;
	}

	if (has_pages)
		printf("%s %lld byte(s) (%d page(s)) at offset 0x%08llx%s%s%s\n",
		       read ? "Reading" : "Writing", len, npages, start_off,
		       raw ? " [raw]" : "", woob ? " [oob]" : "",
		       !read && write_empty_pages ? " [dontskipff]" : "");
	else
		printf("%s %lld byte(s) at offset 0x%08llx\n",
		       read ? "Reading" : "Writing", len, start_off);

	io_op.mode = raw ? MTD_OPS_RAW : MTD_OPS_AUTO_OOB;
	io_op.len = has_pages ? mtd->writesize : len;
	io_op.ooblen = woob ? mtd->oobsize : 0;
	io_op.datbuf = buf;
	io_op.oobbuf = woob ? &buf[len] : NULL;

	/* Search for the first good block after the given offset */
	off = start_off;
	while (MTD_BLOCK_ISBAD(mtd, off / mtd->erasesize))
		off += mtd->erasesize;

	/* Loop over the pages to do the actual read/write */
	while (remaining) {
		/* Skip the block if it is bad */
		if (mtd_is_aligned_with_block_size(mtd, off) &&
		    MTD_BLOCK_ISBAD(mtd, off / mtd->erasesize)) {
			off += mtd->erasesize;
			continue;
		}

		if (read)
			ret = MTD_READ_OOB(mtd, off, io_op.datbuf, io_op.len, io_op.oobbuf, io_op.ooblen);
		else
			ret = mtd_special_write_oob(mtd, off, &io_op,
						    write_empty_pages, woob);

		if (ret) {
			printf("Failure while %s at offset 0x%llx\n",
			       read ? "reading" : "writing", off);
			break;
		}

		off += io_op.len;
		remaining -= io_op.len;
		io_op.datbuf += io_op.len;
		io_op.oobbuf += io_op.ooblen;
	}

	if (!ret && dump)
		mtd_dump_device_buf(mtd, start_off, buf, len, woob);

	if (dump)
		kfree(buf);

	if (ret) {
		printf("%s on %s failed with error %d\n",
		       read ? "Read" : "Write", mtd->name, ret);
		ret = -1;
	} else {
		ret = 0;
	}

out_put_mtd:

	return ret;
}

static inline uint32_t mtd_div_by_eb(uint64_t sz, struct mtd_dev_s *mtd)
{
	do_div(sz, mtd->erasesize);
	return sz;
}       

int do_mtd_erase(int argc, char *argv[])
{
	struct mtd_dev_s *mtd;
	u64 off, len;
	int ret = 0;

	if (argc < 2)
		return -1;

	mtd = get_mtd_by_name(argv[1]);
	if (IS_ERR_OR_NULL(mtd))
		return -1;

	argc -= 2;
	argv += 2;

	off = argc > 0 ? strtoul(argv[0], NULL, 16) : 0;
	len = argc > 1 ? strtoul(argv[1], NULL, 16) : mtd->size;

	if (!mtd_is_aligned_with_block_size(mtd, off)) {
		printf("Offset not aligned with a block (0x%lx)\n",
		       mtd->erasesize);
		ret = -1;
		goto out_put_mtd;
	}

	if (!mtd_is_aligned_with_block_size(mtd, len)) {
		printf("Size not a multiple of a block (0x%lx)\n",
		       mtd->erasesize);
		ret = -1;
		goto out_put_mtd;
	}

	printf("Erasing 0x%08llx ... 0x%08llx (%ld eraseblock(s))\n",
	       off, off + len - 1, mtd_div_by_eb(len, mtd));

	while (len) {
		ret = MTD_ERASE(mtd, off / mtd->erasesize, 1);

		if (ret) {
			/* Abort if its not a bad block error */
			if (ret != -EIO)
				break;
			printf("Skipping bad block at 0x%08llx\n", off);
		}

		len -= mtd->erasesize;
		off += mtd->erasesize;
	}

	if (ret && ret != -EIO)
		ret = -1;
	else
		ret = 0;

out_put_mtd:

	return ret;
}

int do_mtd_bad(int argc, char *argv[])
{
	struct mtd_dev_s *mtd;
	loff_t off;

	if (argc < 2)
		return -1;

	mtd = get_mtd_by_name(argv[1]);
	if (IS_ERR_OR_NULL(mtd))
		return -1;

	if (!MTD_CAN_HAVE_BB(mtd)) {
		printf("Only NAND-based devices can have bad blocks\n");
		goto out_put_mtd;
	}

	printf("MTD device %s bad blocks list:\n", mtd->name);
	for (off = 0; off < mtd->size; off += mtd->erasesize) {
		if (MTD_BLOCK_ISBAD(mtd, off / mtd->erasesize))
			printf("\t0x%08llx\n", off);
	}

out_put_mtd:

	return 0;
}

CONSOLE_CMD(mtd, NULL, NULL, CONSOLE_CMD_MODE_SELF, "mtd cmds")
CONSOLE_CMD(list, "mtd", do_mtd_list, CONSOLE_CMD_MODE_SELF, "list mtd")
CONSOLE_CMD(read, "mtd", do_mtd_io, CONSOLE_CMD_MODE_SELF, "mtd read[_raw][_oob]                  <name> <addr> [<off> [<size>]]")
CONSOLE_CMD(read_raw, "mtd", do_mtd_io, CONSOLE_CMD_MODE_SELF, "mtd read[_raw][_oob]                  <name> <addr> [<off> [<size>]]")
CONSOLE_CMD(read_oob, "mtd", do_mtd_io, CONSOLE_CMD_MODE_SELF, "mtd read[_raw][_oob]                  <name> <addr> [<off> [<size>]]")
CONSOLE_CMD(dump, "mtd", do_mtd_io, CONSOLE_CMD_MODE_SELF, "mtd dump[_raw][_oob]                  <name>        [<off> [<size>]]")
CONSOLE_CMD(dump_raw, "mtd", do_mtd_io, CONSOLE_CMD_MODE_SELF, "mtd dump[_raw][_oob]                  <name>        [<off> [<size>]]")
CONSOLE_CMD(dump_oob, "mtd", do_mtd_io, CONSOLE_CMD_MODE_SELF, "mtd dump[_raw][_oob]                  <name>        [<off> [<size>]]")
CONSOLE_CMD(write, "mtd", do_mtd_io, CONSOLE_CMD_MODE_SELF, "mtd write[_raw][_oob][_dontskipff]    <name> <addr> [<off> [<size>]]")
CONSOLE_CMD(write_raw, "mtd", do_mtd_io, CONSOLE_CMD_MODE_SELF, "mtd write[_raw][_oob][_dontskipff]    <name> <addr> [<off> [<size>]]")
CONSOLE_CMD(write_oob, "mtd", do_mtd_io, CONSOLE_CMD_MODE_SELF, "mtd write[_raw][_oob][_dontskipff]    <name> <addr> [<off> [<size>]]")
CONSOLE_CMD(write_oob_dontskipff, "mtd", do_mtd_io, CONSOLE_CMD_MODE_SELF, "mtd write[_raw][_oob][_dontskipff]    <name> <addr> [<off> [<size>]]")
CONSOLE_CMD(write_raw_dontskipff, "mtd", do_mtd_io, CONSOLE_CMD_MODE_SELF, "mtd write[_raw][_oob][_dontskipff]    <name> <addr> [<off> [<size>]]")
CONSOLE_CMD(write_dontskipff, "mtd", do_mtd_io, CONSOLE_CMD_MODE_SELF, "mtd write[_raw][_oob][_dontskipff]    <name> <addr> [<off> [<size>]]")
CONSOLE_CMD(erase, "mtd", do_mtd_erase, CONSOLE_CMD_MODE_SELF, "mtd erase[_dontskipbad]               <name>        [<off> [<size>]]")
CONSOLE_CMD(bad, "mtd", do_mtd_bad, CONSOLE_CMD_MODE_SELF, "mtd bad                               <name>")
