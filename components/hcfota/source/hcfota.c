#include <generated/br2_autoconf.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stddef.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <hcuapi/sysdata.h>
#include <hcuapi/persistentmem.h>
#include "hcfota.h"
#include <kernel/lib/gzip.h>
#include <kernel/lib/lzma.h>
#include <kernel/lib/crc32.h>
#include <kernel/io.h>
#include <nuttx/mtd/mtd.h>
#include <nuttx/fs/fs.h>
#include <nuttx/fs/partition.h>
#include <cpu_func.h>

enum {
	IH_COMP_NONE = 0, /*  No   Compression Used       */
	IH_COMP_GZIP, /* gzip  Compression Used       */
	IH_COMP_LZMA, /* lzma  Compression Used       */
};

enum {
	IH_DEVT_SPINOR = 0, /*  spi norflash       */
	IH_DEVT_SPINAND, /* spi nandflash       */
	IH_DEVT_NAND, /* nandflash       */
	IH_DEVT_EMMC, /* nandflash       */
};

enum {
	IH_ENTTRY_NORMAL = 0, /*  normal entry       */
	IH_ENTTRY_REMAP = 1, /*  remap entry       */
};


typedef struct table_entry {
	uint8_t comp;
	char *sname; /* short (input) name to find table entry */
} table_entry_t;

struct hcfota_progress {
	long long current;
	long long total;
	hcfota_report_t report_cb;
	unsigned long usrdata;
};

struct partition_state_s
{
  FAR struct mtd_dev_s *mtd;
  FAR struct inode *blk;
  blkcnt_t nblocks;
  blksize_t blocksize;
  size_t erasesize;
};

static const table_entry_t hcfota_comp[] = {
	{ IH_COMP_NONE, "none" },
	{ IH_COMP_GZIP, "gzip" },
	{ IH_COMP_LZMA, "lzma" },
	{ -1, "" },
};

static const table_entry_t hcfota_devtype[] = {
	{ IH_DEVT_SPINOR, "spinor" },
	{ IH_DEVT_SPINAND, "spinand" },
	{ IH_DEVT_NAND, "nand" },
	{ IH_DEVT_EMMC, "emmc" },
	{ -1, "" },
};

static bool url_is_network(const char *url)
{
	if (!strncmp(url, "http://", 7) || !strncmp(url, "https://", 8) || !strncmp(url, "ftp://", 6))
		return true;
	return false;
}

static int hcfota_load_file(const char *path, void **loadbuf, unsigned long *file_size)
{
	struct stat sb;
	FILE *fota_fp;
	void *buf;
	unsigned long fota_size;
	struct hcfota_header h;

	*file_size = 0;
	*loadbuf = NULL;

	if (stat(path, &sb) == -1) {
		perror("stat");
		return -ENOENT;
	}

	fota_fp = fopen(path, "rb");
	if (!fota_fp) {
		printf("open %s failed\n", path);
		return -ENOENT;
	}

	fseek(fota_fp, 0, SEEK_SET);
	if (fread((void *)&h, 1, sizeof(h), fota_fp) != sizeof(h)) {
		printf("read %s failed\n", path);
		fclose(fota_fp);
		return -EIO;
	}

	fota_size = (unsigned long)sb.st_size;
	if (fota_size != (h.payload_size + sizeof(h))) {
		printf("size not match\n");
		fclose(fota_fp);
		return -EIO;
	}

	buf = malloc(fota_size);
	if (!buf) {
		printf("Not enough memory!\n");
		return -ENOMEM;
	}

	fseek(fota_fp, 0, SEEK_SET);
	if (fread(buf, 1, fota_size, fota_fp) != fota_size) {
		printf("read %s failed\n", path);
		free(buf);
		fclose(fota_fp);
		return -EIO;
	}

	fclose(fota_fp);

	*file_size = fota_size;
	*loadbuf = buf;

	return 0;
}

static int hcfota_set_sysdata_version(uint32_t version)
{
	int fd;
	struct persistentmem_node node;
	struct sysdata sysdata = { 0 };

	fd = open("/dev/persistentmem", O_SYNC | O_RDWR);
	if (fd < 0) {
		printf("open /dev/persistentmem failed\n");
		return -1;
	}

	sysdata.firmware_version = version;
	node.id = PERSISTENTMEM_NODE_ID_SYSDATA;
	node.offset = offsetof(struct sysdata, firmware_version);
	node.size = sizeof(sysdata.firmware_version);
	node.buf = &sysdata.firmware_version;
	if (ioctl(fd, PERSISTENTMEM_IOCTL_NODE_PUT, &node) < 0) {
		printf("put sysdata failed\n");
		close(fd);
		return -1;
	}

	close(fd);

	return 0;
}

static void hcfota_set_crc(struct hcfota_header *header, uint32_t crc)
{
	header->crc = crc;
}

static uint32_t hcfota_get_crc(struct hcfota_header *header)
{
	return header->crc;
}

static void hcfota_set_payload_crc(struct hcfota_payload_header *header, uint32_t crc)
{
	header->crc = crc;
}

static uint32_t hcfota_get_payload_crc(struct hcfota_payload_header *header)
{
	return header->crc;
}

int hcfota_info_url(const char *url)
{
	void *buf;
	unsigned long fota_size;
	int rc;

	if (url_is_network(url)) {
		rc = hcfota_download(url, "/tmp/hcfota.bin", NULL, 0);
		if (rc)
			return rc;
		return hcfota_info_url("/tmp/hcfota.bin");
	}

	printf("Show info from %s\n", url);

	rc = hcfota_load_file(url, &buf, &fota_size);
	if (rc) {
		return rc;
	}

	rc = hcfota_info_memory(buf, fota_size);

	free(buf);

	return rc;
}

static char *hcfota_get_comp_name(uint8_t comp)
{
	unsigned int i;
	for (i = 0; i < ARRAY_SIZE(hcfota_comp); i++) {
		if (hcfota_comp[i].comp == comp)
			break;
	}

	return hcfota_comp[i].sname;
}

static char *hcfota_get_devtype_name(uint8_t comp)
{
	unsigned int i;
	for (i = 0; i < ARRAY_SIZE(hcfota_devtype); i++) {
		if (hcfota_devtype[i].comp == comp)
			break;
	}

	return hcfota_devtype[i].sname;
}

static int hcfota_check_header_crc(const char *buf, unsigned long size)
{
	struct hcfota_header *header;
	uint32_t crc, crc2;

	header = (struct hcfota_header *)buf;

	crc = hcfota_get_crc(header);
	hcfota_set_crc(header, 0);

	crc2 = crc32(0, (const uint8_t *)buf, size);
	if (crc != crc2) {
		printf("crc check failed!\n");
		printf("crc calc 0x%08lx\n", crc2);
		printf("crc in fota 0x%08lx\n", crc);
		hcfota_set_crc(header, crc);
		return HCFOTA_ERR_HEADER_CRC;
	}

	hcfota_set_crc(header, crc);
	return 0;
}

static int hcfota_cmp_header_crc(const char *path)
{
	struct hcfota_header *header;
	uint32_t crc;
	long int file_size = 0;
	void *cal_buf;
	FILE *file = NULL;
	struct stat sb;
	int i = 0;
	ssize_t segment = 0x10000;
	uint32_t total_crc = 0, tmp_crc = 0;

	i = segment;

	if (stat(path, &sb) == -1) {
		perror("stat");
		printf("stat failed\n");
		return -ENOENT;
	}

	file_size = (long int)sb.st_size;

	cal_buf = malloc(segment);
	if (!cal_buf)
		return -ENOMEM;

	int hcfota_fd = open(path, O_RDWR);
	if (hcfota_fd  < 0)
		return -ENOENT;

	read(hcfota_fd, cal_buf, i);
	header = (struct hcfota_header *)cal_buf;

	tmp_crc = header->crc;
	header->crc = 0;

	total_crc = crc32(total_crc, (const uint8_t *)cal_buf, i);
	file_size -= i;

	while(file_size){
		if (file_size < segment)
			i = file_size;

		read(hcfota_fd, cal_buf, i);

		total_crc = crc32(total_crc, (const uint8_t *)cal_buf, i);

		file_size -= i;
	}

	if (tmp_crc != total_crc) {
		printf("crc check failed!\n");
		printf("crc calc 	0x%08lx\n", total_crc);
		printf("crc in fota 	0x%08lx\n", tmp_crc);
		hcfota_set_crc(header, tmp_crc);

		return HCFOTA_ERR_HEADER_CRC;
	}

	printf("crc check ok\n");
	printf("crc calc	 0x%08lx\n", total_crc);
	printf("crc in fota	 0x%08lx\n", tmp_crc);
	hcfota_set_crc(header, tmp_crc);

	return 0;
}

static int hcfota_check_payload_crc(const char *buf, unsigned long size)
{
	struct hcfota_payload_header *payload_header;
	uint32_t crc, crc2;

	payload_header = (struct hcfota_payload_header *)buf;

	crc = hcfota_get_payload_crc(payload_header);
	hcfota_set_payload_crc(payload_header, 0);
	crc2 = crc32(0, (const uint8_t *)payload_header, size);
	if (crc != crc2) {
		printf("payload crc check failed!\n");
		printf("crc calc 0x%08lx\n", crc2);
		printf("crc in fota 0x%08lx\n", crc);
		hcfota_set_payload_crc(payload_header, crc);
		return HCFOTA_ERR_PAYLOAD_CRC;
	}

	return 0;
}

static int hcfota_mmc_do_backup(union hcfota_entry *entry, struct hcfota_progress *progress)
{
	return 0;
}

static int hcfota_do_backup(union hcfota_entry *entry, struct hcfota_progress *progress)
{
	if (!entry->backup.upgrade_enable)
		return 0;

	if (entry->backup.old_dev_type == IH_DEVT_SPINOR) {
		struct mtd_geometry_s geo = { 0 };
		struct mtd_eraseinfo_s erase;
		int fd, err, i;
		void *buf;
		void *tmp;
		ssize_t res;
		size_t size, written, segment;

		fd = open("/dev/mtdblock0", O_SYNC | O_RDWR);
		if (fd < 0) {
			return -errno;
		}
		err = ioctl(fd, MTDIOC_GEOMETRY, &geo);
		if (err < 0) {
			close(fd);
			return -errno;
		}

		segment = 0x10000;

		err = lseek(fd, (off_t)entry->backup.old_offset_in_dev, SEEK_SET);
		if (err < 0) {
			close(fd);
			return -errno;
		}

		buf = malloc(entry->backup.old_length + segment);
		if (!buf) {
			close(fd);
			return -ENOMEM;
		}
		tmp = buf + entry->backup.old_length;

		res = read(fd, buf, (size_t)entry->backup.old_length);
		if (res != (ssize_t)entry->backup.old_length) {
			close(fd);
			free(buf);
			return -errno;
		}

		size = entry->backup.old_length;
		written = 0;
		i = segment;
		while (size) {
			if (size < segment)
				i = size;

			erase.start = entry->backup.new_offset_in_dev + written;
			erase.length = (i + geo.erasesize - 1) / geo.erasesize;
			erase.length *= geo.erasesize;

			if (lseek(fd, (off_t)erase.start, SEEK_SET) < 0) {
				close(fd);
				free(buf);
				return -errno;
			}

			if ((read(fd, tmp, i) != i) || memcmp(buf + written, tmp, i)) {
				lseek(fd, (off_t)erase.start, SEEK_SET);
				res = write(fd, buf + written, i);
				if (i != res) {
					close(fd);
					free(buf);
					return -errno;
				}
			}

			written += i;
			size -= i;

			progress->current += i;
			if (progress->report_cb) {
				progress->report_cb(
					HCFOTA_REPORT_EVENT_UPGRADE_PROGRESS,
					progress->current * 100 / progress->total,
					progress->usrdata);
			}
		}
		close(fd);
		free(buf);
	} else {
		printf("Not supported device!\n");
		return -ENOTSUP;
	}

	return 0;
}

void hcfota_updata_progress(struct hcfota_progress *progress, enum HCFOTA_REPORT_EVENT event);

static int hcfota_mmc_upgrade(const char *path,
			      hcfota_external_flash_type_e flash_type,
			      struct hcfota_progress *progress,
			      union hcfota_entry *entry)
{
	struct partition_state_s state = { 0 };
	long file_nblock = 0;
	char dev_path[32] = { 0 };
	struct geometry geo = { 0 };
	int once_len = 0, write_size = 0, nblocks = 0;
	blkcnt_t startblock = 0;
	void *buf = NULL;
	int ret = 0;
	long int file_size = 0;
	long int segment = 0x10000;

	FILE *file_fp = fopen(path, "rb");
	if (!file_fp) {
		printf("open %s failed\n", path);
		return -ENOENT;
	}

	if (file_fp)
		fseek(file_fp, entry->upgrade.offset_in_payload + (sizeof(struct hcfota_header) +
			       sizeof(struct hcfota_payload_header)), SEEK_SET);

	/* block device */
	switch (flash_type) {
	case HCFOTA_EXTERNAL_FLASH_EMMC:
		strcpy(dev_path, "/dev/mmcblk0");
		break;

	case HCFOTA_EXTERNAL_FLASH_NAND:
	case HCFOTA_EXTERNAL_FLASH_SDCARD:
	case HCFOTA_EXTERNAL_FLASH_UDISK:
		printf("%s:%d: not support\n", __func__, __LINE__);
		break;

	default:
		printf("%s:%d: unkown flash type(%d)\n", __func__, __LINE__,
		       flash_type);
		ret = -ENODEV;
		goto err;
	}

	ret = open_blockdriver(dev_path, MS_RDONLY, &state.blk);
	if (ret < 0) {
		printf("%s:%d:err: open_blockdriver failed\n", __func__,
		       __LINE__);
		ret = -ENODEV;
		goto err;
	}

	state.mtd = NULL;
	ret = state.blk->u.i_bops->geometry(state.blk, &geo);
	if (ret < 0) {
		printf("%s:%d:err: geometry failed\n", __func__, __LINE__);
		ret = -ENODEV;
		goto err;
	}
	state.blocksize = geo.geo_sectorsize;
	state.erasesize = geo.geo_sectorsize;
	state.nblocks = geo.geo_nsectors;

	file_size = entry->upgrade.length;
	file_nblock = file_size / state.blocksize;
	if (file_size % state.blocksize != 0)
		file_nblock += 1;

	/* file size can not bigger then flash size */
	if ((state.nblocks) < file_nblock) {
		printf("%s:%d: file size can not bigger then flash size\n",
		       __func__, __LINE__);
		printf("%s:%d: blocksize=%ld, nblocks=%lld, file_size=%ld\n",
		       __func__, __LINE__, state.blocksize, state.nblocks,
		       file_size);
		ret = -EINVAL;
		goto err;
	}

	buf = malloc(segment);
	if (buf == NULL) {
		printf("%s:%d:err: malloc failed\n", __func__, __LINE__);
		ret = -ENOMEM;
		goto err;
	}

	/* write data to flash */
	once_len = fread(buf, 1, segment, file_fp);
	while (once_len != 0) {
		/* if last time is not a multiple of 512, what to do? */
		nblocks = once_len / 512;
		if (once_len % 512) {
			nblocks++;
		}
		write_size = state.blk->u.i_bops->write(state.blk, buf, startblock, nblocks);

		if (write_size != nblocks) {
			printf("%s:%d: block write failed, startblock=%lld, nblocks=%d, write_size=%d\n",
			       __func__, __LINE__, startblock, nblocks, write_size);
			ret = -EBADF;
			goto err;
		}
		startblock += write_size;

		progress->current += once_len;
		hcfota_updata_progress(progress, HCFOTA_REPORT_EVENT_UPGRADE_PROGRESS);

		once_len = fread(buf, 1, segment, file_fp);
	}

err:
	if (file_fp) {
		fclose(file_fp);
		file_fp = NULL;
	}

	if (state.blk) {
		close_blockdriver(state.blk);
		state.blk = NULL;
	}

	if (buf) {
		free(buf);
		buf = NULL;
	}

	return ret;
}

static int hcfota_do_upgrade(union hcfota_entry *entry, void *payload, struct hcfota_progress *progress)
{
	if (!entry->upgrade.upgrade_enable)
		return 0;

	if (entry->upgrade.dev_type == IH_DEVT_SPINOR) {
		struct mtd_geometry_s geo = { 0 };
		struct mtd_eraseinfo_s erase;
		int fd, err, i;
		void *buf;
		void *tmp;
		ssize_t res;
		size_t size, written, eraseoff, segment;

		fd = open("/dev/mtdblock0", O_SYNC | O_RDWR);
		if (fd < 0) {
			return -errno;
		}
		err = ioctl(fd, MTDIOC_GEOMETRY, &geo);
		if (err < 0) {
			close(fd);
			return -errno;
		}

		segment = 0x10000;

		tmp = malloc(segment);
		if (!tmp) {
			close(fd);
			return -ENOMEM;
		}

		buf = payload + entry->upgrade.offset_in_payload;
		size = entry->upgrade.length;
		written = 0;
		eraseoff = 0;
		i = segment;
		while (size) {
			if (size < segment)
				i = size;

			erase.start = entry->upgrade.offset_in_dev + written;
			erase.length = (i + geo.erasesize - 1) / geo.erasesize;
			erase.length *= geo.erasesize;

			if (lseek(fd, (off_t)erase.start, SEEK_SET) < 0) {
				close(fd);
				free(tmp);
				return -errno;
			}

			if (i != (int)segment || (read(fd, tmp, i) != i) ||
			    memcmp(buf + written, tmp, i)) {
				if (i != (int)segment) {
					ioctl(fd, MTDIOC_MEMERASE, &erase);
				}

				lseek(fd, (off_t)erase.start, SEEK_SET);
				res = write(fd, buf + written, i);
				if (i != res) {
					free(tmp);
					close(fd);
					return -errno;
				}
			}

			written += i;
			size -= i;
			eraseoff += erase.length;

			progress->current += i;
			if (progress->report_cb) {
				progress->report_cb(
					HCFOTA_REPORT_EVENT_UPGRADE_PROGRESS,
					progress->current * 100 / progress->total,
					progress->usrdata);
			}
		}

		if (entry->upgrade.erase_length > eraseoff) {
			erase.start = entry->upgrade.offset_in_dev + eraseoff;
			erase.length = entry->upgrade.erase_length - eraseoff;;
			if (ioctl(fd, MTDIOC_MEMERASE, &erase) < 0) {
				close(fd);
				free(tmp);
				return -errno;
			}
		}
		close(fd);
		free(tmp);
	} else {
		printf("Not supported device!\n");
		return -ENOTSUP;
	}

	return 0;
}

static int hcfota_do_upgrade_from_path(union hcfota_entry *entry, const char *path, struct hcfota_progress *progress)
{
	if (!entry->upgrade.upgrade_enable)
		return 0;

	int hcfota_fd = open(path, O_RDWR);
	if (hcfota_fd  < 0)
		return -ENOENT;

	if (hcfota_fd)
		lseek(hcfota_fd, entry->upgrade.offset_in_payload + (sizeof(struct hcfota_header) +
			       sizeof(struct hcfota_payload_header)), SEEK_SET);

	int fd;
	int ret;
	void *buf;
	void *tmp;
	size_t segment;

	segment = 0x10000;
	buf = malloc(segment);
	tmp = malloc(segment);
	if (!tmp || !buf) {
		return -ENOMEM;
	}

	if (entry->upgrade.dev_type == IH_DEVT_SPINOR) {
		struct mtd_geometry_s geo = { 0 };
		struct mtd_eraseinfo_s erase;
		int i;
		ssize_t res;
		size_t size, written, eraseoff;

		fd = open("/dev/mtdblock0", O_SYNC | O_RDWR);
		if (fd < 0) {
			free(buf);
			free(tmp);
			close(hcfota_fd);
			return -errno;
		}
		ret = ioctl(fd, MTDIOC_GEOMETRY, &geo);
		if (ret < 0) {
			goto err;
		}

		size = entry->upgrade.length;
		written = 0;
		eraseoff = 0;
		i = segment;
		erase.start = entry->upgrade.offset_in_dev + written;
		while (size) {
			if (size < segment)
				i = size;

			erase.start = entry->upgrade.offset_in_dev + written;
			erase.length = (i + geo.erasesize - 1) / geo.erasesize;
			erase.length *= geo.erasesize;

			if (lseek(fd, (off_t)erase.start, SEEK_SET) < 0) {
				goto err;
			}

			read(hcfota_fd, buf, i);

			if (i != (int)segment || (read(fd, tmp, i) != i) ||
			    memcmp(buf, tmp, i)) {
				if (i != (int)segment) {
					ioctl(fd, MTDIOC_MEMERASE, &erase);
				}

				lseek(fd, (off_t)erase.start, SEEK_SET);
				res = write(fd, buf, i);
				if (i != res) {
					goto err;
				}
			}

			written += i;
			size -= i;
			eraseoff += erase.length;
			progress->current += i;
			if (progress->report_cb) {
				progress->report_cb(
					HCFOTA_REPORT_EVENT_UPGRADE_PROGRESS,
					progress->current * 100 /
						progress->total,
					progress->usrdata);
			}
		}

		if (entry->upgrade.erase_length > eraseoff) {
			erase.start = entry->upgrade.offset_in_dev + eraseoff;
			erase.length = entry->upgrade.erase_length - eraseoff;
			if (ioctl(fd, MTDIOC_MEMERASE, &erase) < 0) {
				printf("erase fail\n");
				goto err;
			}
		}
		close(fd);

	} else if (entry->upgrade.dev_type == IH_DEVT_EMMC) {
		ret = hcfota_mmc_upgrade(path, HCFOTA_EXTERNAL_FLASH_EMMC,
					 progress, entry);
	} else {
		printf("Not supported device!\n");
		return -ENOTSUP;
	}

	/* crc check*/

	free(buf);
	free(tmp);
	close(hcfota_fd);

	return ret;

err:
	close(fd);
	free(buf);
	free(tmp);
	close(hcfota_fd);
	return -errno;
}

static void hcfota_show_header(struct hcfota_header *header)
{
	printf("HCFOTA HEADER INFO:\n");
	printf("    crc:                       0x%08lx\n", header->crc);
	printf("    compress type:             %s\n", hcfota_get_comp_name(header->compress_type));
	printf("    ignore version check:      %s\n", header->ignore_version_check ? "true" : "false");
	printf("    version:                   %ld\n", header->version);
	printf("    uncompressed length:       %ld\n", header->uncompressed_length);
	printf("    length:                    %ld\n", header->uncompressed_length);
	printf("    board:                     %s\n", header->board);
}

static void hcfota_show_payload_header(struct hcfota_payload_header *header)
{
	printf("HCFOTA PAYLOAD INFO:\n");
	printf("    crc:                       0x%08lx\n", header->crc);
	printf("    entry number:              %ld\n", header->entry_number);
}

static void hcfota_show_normal_entry(union hcfota_entry *entry)
{
	printf("  new normal entry:\n");
	printf("    index:                     %d\n", entry->upgrade.index);
	printf("    entry type:                %d\n", entry->upgrade.entry_type);
	printf("    upgrade enable:            %s\n", entry->upgrade.upgrade_enable ? "true" : "false");
	printf("    device index:              %d\n", entry->upgrade.dev_index);
	printf("    device type:               %s\n", hcfota_get_devtype_name(entry->upgrade.dev_type));
	printf("    offset in payload:         0x%08lx\n", entry->upgrade.offset_in_payload);
	printf("    length:                    0x%08lx\n", entry->upgrade.length);
	printf("    erase length:              0x%08lx\n", entry->upgrade.erase_length);
	printf("    offset in device:          0x%08lx\n", entry->upgrade.offset_in_dev);
	printf("    offset in mtdblock device: 0x%08lx\n", entry->upgrade.offset_in_blkdev);
}

static void hcfota_show_backup_entry(union hcfota_entry *entry)
{
	printf("  new backup entry:\n");
	printf("    index:                     %d\n", entry->backup.index);
	printf("    entry type:                %d\n", entry->backup.entry_type);
	printf("    upgrade enable:            %s\n", entry->backup.upgrade_enable ? "true" : "false");
	printf("    old device index:          %d\n", entry->backup.old_dev_index);
	printf("    old device type:           %s\n", hcfota_get_devtype_name(entry->backup.old_dev_type));
	printf("    old offset in device:      0x%08lx\n", entry->backup.old_offset_in_dev);
	printf("    old length:                0x%08lx\n", entry->backup.old_length);
	printf("    new device index:          %d\n", entry->backup.new_dev_index);
	printf("    new device type:           %s\n", hcfota_get_devtype_name(entry->backup.new_dev_type));
	printf("    new offset in device:      0x%08lx\n", entry->backup.new_offset_in_dev);
	printf("    new length:                0x%08lx\n", entry->backup.new_length);
}

int hcfota_info_memory(const char *buf, unsigned long size)
{
	struct hcfota_header *header;
	struct hcfota_payload_header *payload_header = NULL;
	void *decomp_buf = NULL;
	int rc = 0;
	unsigned int i;

	rc = hcfota_check_header_crc(buf, size);
	if (rc) {
		return rc;
	}

	header = (struct hcfota_header *)buf;
	hcfota_show_header(header);

	switch(header->compress_type) {
	case IH_COMP_NONE:
		payload_header = (struct hcfota_payload_header *)(buf + sizeof(struct hcfota_header));
		break;

#ifdef CONFIG_LIB_GZIP
	case IH_COMP_GZIP: {
		unsigned long image_len = size - sizeof(struct hcfota_header);
		decomp_buf = malloc(header->uncompressed_length);
		rc = gunzip(decomp_buf, header->uncompressed_length,
			     (unsigned char *)buf + sizeof(struct hcfota_header),
			     &image_len);
		if (!rc)
			payload_header = decomp_buf;
		break;
	}
#endif

#ifdef CONFIG_LIB_LZMA
	case IH_COMP_LZMA: {
		unsigned long lzma_len = header->uncompressed_length;
		decomp_buf = malloc(header->uncompressed_length);
		rc = lzmaBuffToBuffDecompress(
			decomp_buf, &lzma_len,
			(unsigned char *)buf + sizeof(struct hcfota_header),
			size - sizeof(struct hcfota_header));
		if (!rc)
			payload_header = decomp_buf;

		break;
	}
#endif
	default:
		printf("Compress type error: %d\n", header->compress_type);
		return -1;
	}

	if (rc) {
		if (decomp_buf)
			free(decomp_buf);
		printf("decompress error: %d!\n", rc);
		return rc;
	}

	if (header->compress_type != IH_COMP_NONE) {
		rc = hcfota_check_payload_crc((const char *)payload_header, header->uncompressed_length);
		if (rc) {
			if (decomp_buf)
				free(decomp_buf);
			return rc;
		}
	}

	hcfota_show_payload_header(payload_header);
	for (i = 0; i < payload_header->entry_number; i++) {
		if (payload_header->entry[i].upgrade.entry_type == IH_ENTTRY_NORMAL) {
			hcfota_show_normal_entry(&payload_header->entry[i]);
		} else if (payload_header->entry[i].upgrade.entry_type == IH_ENTTRY_REMAP) {
			hcfota_show_backup_entry(&payload_header->entry[i]);
		}
	}

	if (decomp_buf)
		free(decomp_buf);

	return 0;
}

int hcfota_download(const char *url, const char *path, hcfota_report_t report_cb, unsigned long usrdata)
{
	printf("Downloading from %s to %s\n", url, path);
	return -1;
}

int hcfota_reboot(unsigned long modes)
{
	int fd;
	struct persistentmem_node_create new_node;
	struct persistentmem_node node;
	struct sysdata sysdata = { 0 };

	printf("Reboot mode %ld\n", modes);
	fd = open("/dev/persistentmem", O_SYNC | O_RDWR);
	if (fd < 0) {
		printf("open /dev/persistentmem failed\n");
		return -1;
	}

	node.id = PERSISTENTMEM_NODE_ID_SYSDATA;
	node.offset = 0;
	node.size = sizeof(struct sysdata);
	node.buf = &sysdata;
	if (ioctl(fd, PERSISTENTMEM_IOCTL_NODE_GET, &node) < 0) {
		new_node.id = PERSISTENTMEM_NODE_ID_SYSDATA;
		new_node.size = sizeof(struct sysdata);
		if (ioctl(fd, PERSISTENTMEM_IOCTL_NODE_CREATE, &new_node) < 0) {
			printf("get sysdata failed\n");
			close(fd);
			return -1;
		}
	}

	if (sysdata.ota_detect_modes != modes) {
		sysdata.ota_detect_modes = modes;
		node.id = PERSISTENTMEM_NODE_ID_SYSDATA;
		node.offset = offsetof(struct sysdata, ota_detect_modes);
		node.size = sizeof(sysdata.ota_detect_modes);
		node.buf = &sysdata.ota_detect_modes;
		if (ioctl(fd, PERSISTENTMEM_IOCTL_NODE_PUT, &node) < 0) {
			printf("put sysdata failed\n");
			close(fd);
			return -1;
		}
	}

	close(fd);
	reset();

	return 0;
}

int hcfota_url(const char *url, hcfota_report_t report_cb, unsigned long usrdata)
{
	int rc;
	void *buf;
	unsigned long fota_size;

	printf("Upgrade from %s\n", url);

	if (url_is_network(url)) {
		rc = hcfota_download(url, "/tmp/hcfota.bin", report_cb, usrdata);
		if (rc)
			return HCFOTA_ERR_DOWNLOAD;
		return hcfota_url("/tmp/hcfota.bin", report_cb, usrdata);
	}

	rc = hcfota_load_file(url, &buf, &fota_size);
	if (rc) {
		printf("load file %s failed!\n", url);
		return HCFOTA_ERR_LOADFOTA;
	}

	rc = hcfota_memory(buf, fota_size, report_cb, usrdata);
	if (rc) {
		printf("Upgrade failed %d\n", rc);
	}

	free(buf);

	return rc;
}

int hcfota_memory(const char *buf, unsigned long size, hcfota_report_t report_cb, unsigned long usrdata)
{
	struct hcfota_header *header;
	struct hcfota_payload_header *payload_header = NULL;
	void *payload;
	void *decomp_buf = NULL;
	struct hcfota_progress progress = { 0 };
	struct sysdata sysdata = { 0 };
	int rc;
	unsigned int i;

	rc = hcfota_check_header_crc(buf, size);
	if (rc) {
		return rc;
	}

	header = (struct hcfota_header *)buf;

	if (!sys_get_sysdata(&sysdata)) {
		if (header->version <= sysdata.firmware_version &&
		    !header->ignore_version_check)
			return HCFOTA_ERR_VERSION;
	}

	switch(header->compress_type) {
	case IH_COMP_NONE:
		payload_header = (struct hcfota_payload_header *)(buf + sizeof(struct hcfota_header));
		break;
#ifdef CONFIG_LIB_GZIP
	case IH_COMP_GZIP: {
		unsigned long image_len = size - sizeof(struct hcfota_header);
		decomp_buf = malloc(header->uncompressed_length);
		rc = gunzip(decomp_buf, header->uncompressed_length,
			     (unsigned char *)buf + sizeof(struct hcfota_header),
			     &image_len);
		if (!rc)
			payload_header = decomp_buf;
		break;
	}
#endif

#ifdef CONFIG_LIB_LZMA
	case IH_COMP_LZMA: {
		unsigned long lzma_len = header->uncompressed_length;
		decomp_buf = malloc(header->uncompressed_length);
		rc = lzmaBuffToBuffDecompress(
			decomp_buf, &lzma_len,
			(unsigned char *)buf + sizeof(struct hcfota_header),
			size - sizeof(struct hcfota_header));
		if (!rc)
			payload_header = decomp_buf;

		break;
	}
#endif
	default:
		printf("Compress type error: %d\n", header->compress_type);
		return -1;
	}

	if (rc) {
		if (decomp_buf)
			free(decomp_buf);
		printf("decompress error: %d!\n", rc);
		return HCFOTA_ERR_DECOMPRESSS;
	}

	if (header->compress_type != IH_COMP_NONE) {
		rc = hcfota_check_payload_crc((const char *)payload_header, header->uncompressed_length);
		if (rc) {
			if (decomp_buf)
				free(decomp_buf);
			return rc;
		}
	}

	progress.report_cb = report_cb;
	progress.usrdata = usrdata;
	for (i = 0; i < payload_header->entry_number; i++) {
		if (payload_header->entry[i].upgrade.entry_type == IH_ENTTRY_NORMAL) {
			if (payload_header->entry[i].upgrade.upgrade_enable)
				progress.total += payload_header->entry[i].upgrade.length;
		} else if (payload_header->entry[i].upgrade.entry_type == IH_ENTTRY_REMAP) {
			if (payload_header->entry[i].backup.upgrade_enable)
				progress.total += payload_header->entry[i].backup.old_length;
		}
	}

	if (progress.report_cb) {
		progress.report_cb(HCFOTA_REPORT_EVENT_UPGRADE_PROGRESS,
				   progress.current * 100 / progress.total,
				   progress.usrdata);
	}

	/* Do backup first */
	rc = 0;
	for (i = 0; i < payload_header->entry_number; i++) {
		if (payload_header->entry[i].upgrade.entry_type == IH_ENTTRY_REMAP) {
			rc |= hcfota_do_backup(&payload_header->entry[i], &progress);
			if (rc) {
				printf("Backup partition failed\n");
				hcfota_show_backup_entry(&payload_header->entry[i]);
			}
		}
	}

	/* Do upgrade */
	payload = (void *)((char *)payload_header + sizeof(struct hcfota_payload_header));
	for (i = 0; i < payload_header->entry_number; i++) {
		if (payload_header->entry[i].upgrade.entry_type == IH_ENTTRY_NORMAL) {
			rc |= hcfota_do_upgrade(&payload_header->entry[i], payload, &progress);
			if (rc) {
				printf("Upgrade partition failed\n");
				hcfota_show_normal_entry(&payload_header->entry[i]);
			}
		}
	}

	if (decomp_buf)
		free(decomp_buf);

	if (rc) {
#ifdef CONFIG_BOOT_UPGRADE_SHOW_WITH_LED
		if (progress.report_cb) {
			progress.report_cb(HCFOTA_REPORT_EVENT_UPGRADE_PROGRESS, 0xff, 0xff);
		}
#endif
		printf("\nUpgrade fail\n");
	} else {
		if (!header->ignore_version_update)
			hcfota_set_sysdata_version(header->version);
		printf("\nUpgrade success\n");
	}

	return rc ? HCFOTA_ERR_UPGRADE : 0;
}

static int hcfota_do_upgrade_b2b_mmc(union hcfota_entry *entry, void *payload, struct hcfota_progress *progress)
{
	struct partition_state_s state = { 0 };
	long file_nblock = 0;
	char dev_path[32] = { 0 };
	struct geometry geo = { 0 };
	int  write_size = 0, nblocks = 0;
	blkcnt_t startblock = 0;
	void *buf = NULL;
	int ret = 0;
	long int file_size = 0;
	long int segment = 0x10000;

	/* block device */
	strcpy(dev_path, "/dev/mmcblk0");
	ret = open_blockdriver(dev_path, MS_RDONLY, &state.blk);
	if (ret < 0) {
		printf("%s:%d:err: open_blockdriver failed\n", __func__,
		       __LINE__);
		ret = -ENODEV;
		goto err;
	}

	state.mtd = NULL;
	ret = state.blk->u.i_bops->geometry(state.blk, &geo);
	if (ret < 0) {
		printf("%s:%d:err: geometry failed\n", __func__, __LINE__);
		ret = -ENODEV;
		goto err;
	}
	state.blocksize = geo.geo_sectorsize;
	state.erasesize = geo.geo_sectorsize;
	state.nblocks = geo.geo_nsectors;

	file_size = entry->upgrade.length;
	file_nblock = file_size / state.blocksize;
	if (file_size % state.blocksize != 0)
		file_nblock += 1;

	/* file size can not bigger then flash size */
	if ((state.nblocks) < file_nblock) {
		printf("%s:%d: file size can not bigger then flash size\n", __func__, __LINE__);
		printf("%s:%d: blocksize=%ld, nblocks=%lld, file_size=%ld\n",
		       __func__, __LINE__, state.blocksize, state.nblocks,
		       file_size);
		ret = -EINVAL;
		goto err;
	}

	buf = payload + entry->upgrade.offset_in_payload;
	if (buf == NULL) {
		ret = -ENOMEM;
		goto err;
	}

	/* write data to flash */
	nblocks = 128;
	while (file_nblock != 0) {
		/* if last time is not a multiple of 512, what to do? */
		if (file_nblock < nblocks)
			nblocks = file_nblock;

		write_size = state.blk->u.i_bops->write(state.blk, buf, startblock, nblocks);

		if (write_size != nblocks) {
			printf("%s:%d: block write failed, startblock=%lld, nblocks=%d, write_size=%d\n",
			       __func__, __LINE__, startblock, nblocks, write_size);
			ret = -EBADF;
			goto err;
		}
		startblock += write_size;
		buf = buf + write_size * 512;

		progress->current += write_size * 512;
		hcfota_updata_progress(progress, HCFOTA_REPORT_EVENT_UPGRADE_PROGRESS);

		file_nblock -= nblocks;
	}

	if (state.blk) {
		close_blockdriver(state.blk);
		state.blk = NULL;
	}

	return 0;

err:

	if (state.blk) {
		close_blockdriver(state.blk);
		state.blk = NULL;
	}

	return ret;
}

static int hcfota_do_upgrade_b2b(union hcfota_entry *entry, void *payload, struct hcfota_progress *progress)
{
	int ret = 0;
	if (!entry->upgrade.upgrade_enable)
		return 0;

	if (entry->upgrade.dev_type == IH_DEVT_SPINOR) {
		struct mtd_geometry_s geo = { 0 };
		struct mtd_eraseinfo_s erase;
		int fd, err, i;
		void *buf;
		void *tmp;
		ssize_t res;
		size_t size, written, eraseoff, segment;

		fd = open("/dev/mtdblock0", O_SYNC | O_RDWR);
		if (fd < 0) {
			return -errno;
		}
		err = ioctl(fd, MTDIOC_GEOMETRY, &geo);
		if (err < 0) {
			close(fd);
			return -errno;
		}

		segment = 0x10000;

		tmp = malloc(segment);
		if (!tmp) {
			close(fd);
			return -ENOMEM;
		}

		buf = payload + entry->upgrade.offset_in_payload;
		size = entry->upgrade.length;
		written = 0;
		eraseoff = 0;
		i = segment;
		while (size) {
			if (size < segment)
				i = size;

			erase.start = entry->upgrade.offset_in_dev + written;
			erase.length = (i + geo.erasesize - 1) / geo.erasesize;
			erase.length *= geo.erasesize;

			if (lseek(fd, (off_t)erase.start, SEEK_SET) < 0) {
				close(fd);
				free(tmp);
				return -errno;
			}

			if (i != (int)segment || (read(fd, tmp, i) != i) ||
			    memcmp(buf + written, tmp, i)) {
				if (i != (int)segment) {
					ioctl(fd, MTDIOC_MEMERASE, &erase);
				}

				lseek(fd, (off_t)erase.start, SEEK_SET);
				res = write(fd, buf + written, i);
				if (i != res) {
					free(tmp);
					close(fd);
					return -errno;
				}
			}

			written += i;
			size -= i;
			eraseoff += erase.length;

			progress->current += i;
			if (progress->report_cb) {
				progress->report_cb(
					HCFOTA_REPORT_EVENT_UPGRADE_PROGRESS,
					progress->current * 100 / progress->total,
					progress->usrdata);
			}
		}

		if (entry->upgrade.erase_length > eraseoff) {
			erase.start = entry->upgrade.offset_in_dev + eraseoff;
			erase.length = entry->upgrade.erase_length - eraseoff;;
			if (ioctl(fd, MTDIOC_MEMERASE, &erase) < 0) {
				close(fd);
				free(tmp);
				return -errno;
			}
		}
		close(fd);
		free(tmp);
	} else if (entry->upgrade.dev_type == IH_DEVT_EMMC) {
		ret = hcfota_do_upgrade_b2b_mmc(entry, payload, progress);
	} else {
		printf("Not supported device!\n");
		return -ENOTSUP;
	}

	return 0;
}
int hcfota_memory_b2b(const char *buf, unsigned long size, hcfota_report_t report_cb, unsigned long usrdata)
{
	struct hcfota_header *header;
	struct hcfota_payload_header *payload_header = NULL;
	void *payload;
	void *decomp_buf = NULL;
	struct hcfota_progress progress = { 0 };
	struct sysdata sysdata = { 0 };
	int rc;
	unsigned int i;

	get_data_from_burner_t get_data_from_burner_cb = (get_data_from_burner_t)usrdata;

	rc = hcfota_check_header_crc(buf, size);
	if (rc) {
		get_data_from_burner_cb(BURNER_FAIL, 0xcc, 0, NULL, NULL);
		return rc;
	}

	header = (struct hcfota_header *)buf;

	if (!sys_get_sysdata(&sysdata)) {
		if (header->version <= sysdata.firmware_version &&
		    !header->ignore_version_check) {
			get_data_from_burner_cb(BURNER_FAIL, 0xdd, 0, NULL,
						NULL);
			return HCFOTA_ERR_VERSION;
		}
	}

	switch(header->compress_type) {
	case IH_COMP_NONE:
		payload_header = (struct hcfota_payload_header *)(buf + sizeof(struct hcfota_header));
		break;
#ifdef CONFIG_LIB_GZIP
	case IH_COMP_GZIP: {
		break;
	}
#endif

#ifdef CONFIG_LIB_LZMA
	case IH_COMP_LZMA: {
		break;
	}
#endif
	default:
		printf("Compress type error: %d\n", header->compress_type);
		return -1;
	}

	if (rc) {
		if (decomp_buf)
			free(decomp_buf);
		printf("decompress error: %d!\n", rc);
		return HCFOTA_ERR_DECOMPRESSS;
	}

	if (header->compress_type != IH_COMP_NONE) {
		rc = hcfota_check_payload_crc((const char *)payload_header, header->uncompressed_length);
		if (rc) {
			if (decomp_buf)
				free(decomp_buf);
			return rc;
		}
	}

	progress.report_cb = report_cb;
	progress.usrdata = usrdata;
	for (i = 0; i < payload_header->entry_number; i++) {
		if (payload_header->entry[i].upgrade.entry_type == IH_ENTTRY_NORMAL) {
			if (payload_header->entry[i].upgrade.upgrade_enable)
				progress.total += payload_header->entry[i].upgrade.length;
		} else if (payload_header->entry[i].upgrade.entry_type == IH_ENTTRY_REMAP) {
			if (payload_header->entry[i].backup.upgrade_enable)
				progress.total += payload_header->entry[i].backup.old_length;
		}
	}

	if (progress.report_cb) {
		progress.report_cb(HCFOTA_REPORT_EVENT_UPGRADE_PROGRESS,
				   progress.current * 100 / progress.total,
				   progress.usrdata);
	}

	/* Do backup first */
	rc = 0;
	for (i = 0; i < payload_header->entry_number; i++) {
		if (payload_header->entry[i].upgrade.entry_type == IH_ENTTRY_REMAP) {
			rc |= hcfota_do_backup(&payload_header->entry[i], &progress);
			if (rc) {
				printf("Backup partition failed\n");
				hcfota_show_backup_entry(&payload_header->entry[i]);
			}
		}
	}

	/* Do upgrade */
	payload = (void *)((char *)payload_header + sizeof(struct hcfota_payload_header));
	for (i = 0; i < payload_header->entry_number; i++) {
		if (payload_header->entry[i].upgrade.entry_type == IH_ENTTRY_NORMAL) {
			rc |= hcfota_do_upgrade_b2b(&payload_header->entry[i], payload, &progress);
			if (rc) {
				printf("Upgrade partition failed\n");
				hcfota_show_normal_entry(&payload_header->entry[i]);
			}
			if(get_data_from_burner_cb != NULL&& rc ==0) {
				get_data_from_burner_cb(BURNER_SEND_STATUS, (i+1)*100/payload_header->entry_number, 0, NULL, NULL);
			} else if(get_data_from_burner_cb != NULL&& rc != 0) {
				get_data_from_burner_cb(BURNER_FAIL, 0xff, 0, NULL, NULL);
			}

		}
	}

	vTaskDelay(500);

	if (decomp_buf)
		free(decomp_buf);

	if (rc)
		printf("\nUpgrade fail\n");
	else {
		if (!header->ignore_version_update)
			hcfota_set_sysdata_version(header->version);
		printf("\nUpgrade success\n");
	}

	return rc ? HCFOTA_ERR_UPGRADE : 0;
}


static int hcfota_from_mem_upgrade_to_spinor(union hcfota_entry *entry,
					     void *hcfota_data_buf, uint32_t buf_read_size,
					     uint32_t *finsh_len)
{
	int fd;
	int ret;
	void *tmp;
	void *buf = hcfota_data_buf;
	size_t segment;

	segment = 0x10000;
	tmp = malloc(segment);
	if (!tmp) {
		return -ENOMEM;
	}

	if (!entry->upgrade.upgrade_enable)
		return 0;

	struct mtd_geometry_s geo = { 0 };
	struct mtd_eraseinfo_s erase;
	int i;
	ssize_t res;
	size_t size, written, eraseoff;

	fd = open("/dev/mtdblock0", O_SYNC | O_RDWR);
	if (fd < 0) {
		return -errno;
	}

	ret = ioctl(fd, MTDIOC_GEOMETRY, &geo);
	if (ret < 0) {
		goto err;
	}

	size = buf_read_size;
	//size = entry->upgrade.length;
	written = 0;
	eraseoff = 0;
	i = segment;
	erase.start = entry->upgrade.offset_in_dev + *finsh_len;
	while (size) {
		if (size < segment)
			i = size;

		//erase.start = entry->upgrade.offset_in_dev + written;
		erase.start = entry->upgrade.offset_in_dev + written + *finsh_len;
		printf("start %lx\n",erase.start);
		erase.length = (i + geo.erasesize - 1) / geo.erasesize;
		erase.length *= geo.erasesize;

		if (lseek(fd, (off_t)erase.start, SEEK_SET) < 0) {
			goto err;
		}

		buf = hcfota_data_buf + written;

		if (i != (int)segment || (read(fd, tmp, i) != i) ||
		    memcmp(buf, tmp, i)) {
			if (i != (int)segment) {
				ioctl(fd, MTDIOC_MEMERASE, &erase);
			}

			lseek(fd, (off_t)erase.start, SEEK_SET);
			res = write(fd, buf, i);
			if (i != res) {
				goto err;
			}
		}

		written += i;
		size -= i;
		eraseoff += erase.length;
	}

	if (entry->upgrade.erase_length > eraseoff) {
		erase.start = entry->upgrade.offset_in_dev + eraseoff + *finsh_len;
		erase.length = entry->upgrade.erase_length - eraseoff;
		if (ioctl(fd, MTDIOC_MEMERASE, &erase) < 0) {
			printf("erase fail\n");
			goto err;
		}
	}


	close(fd);
	free(tmp);
	return 0;

err:
	close(fd);
	free(tmp);
	return -errno;
}

static int hcfota_from_mem_upgrade_to_sdmmc(union hcfota_entry *entry,
					     void *hcfota_data_buf, uint32_t buf_read_size,
					     uint32_t *finsh_len)
{
	int ret = 0;
	long file_nblock = 0;
	long int file_size = 0;
	char dev_path[32] = { 0 };
	int once_len = 0, write_nblock = 0, nblocks = 0;
	blkcnt_t startblock = 0;

	struct partition_state_s state = { 0 };
	struct geometry geo = { 0 };

	strcpy(dev_path, "/dev/mmcblk0");
	ret = open_blockdriver(dev_path, MS_RDONLY, &state.blk);
	if (ret < 0) {
		printf("%s:%d:err: open_blockdriver failed\n", __func__, __LINE__);
		ret = -ENODEV;
		goto err;
	}

	state.mtd = NULL;
	ret = state.blk->u.i_bops->geometry(state.blk, &geo);
	if (ret < 0) {
		printf("%s:%d:err: geometry failed\n", __func__, __LINE__);
		ret = -ENODEV;
		goto err;
	}
	state.blocksize = geo.geo_sectorsize;
	state.erasesize = geo.geo_sectorsize;
	state.nblocks = geo.geo_nsectors;

	file_size = buf_read_size;
	file_nblock = file_size / state.blocksize;
	if (file_size % state.blocksize != 0)
		file_nblock += 1;

	/* file size can not bigger then dev size */
	if ((state.nblocks) < file_nblock) {
		printf("%s:%d: file size can not bigger then flash size\n", __func__, __LINE__);
		printf("%s:%d: blocksize=%ld, nblocks=%lld, file_size=%ld\n",
		       __func__, __LINE__, state.blocksize, state.nblocks, file_size);
		ret = -EINVAL;
		goto err;
	}

	startblock = *finsh_len / 512;
	if (((*finsh_len) % 512) && (*finsh_len != 0))
		startblock++;

	nblocks = buf_read_size / 512;
	if ((nblocks) % 512)
		nblocks++;

	write_nblock = state.blk->u.i_bops->write(state.blk, hcfota_data_buf,
						  startblock, nblocks);
	if (write_nblock != nblocks) {
		printf("mmc write error\n");
		return -EBADF;
	}

	*finsh_len += buf_read_size;

	return 0;

err:
	return ret;
}

int hcfota_from_mem(union hcfota_entry *entry, void *buf,uint32_t buf_read_size,uint32_t *finsh_len)
{
	int ret = 0;
	if (entry->upgrade.dev_type == IH_DEVT_SPINOR) {
		ret = hcfota_from_mem_upgrade_to_spinor(entry, buf, buf_read_size, finsh_len);
	} else if (entry->upgrade.dev_type == IH_DEVT_EMMC) {
		//ret = hcfota_from_mem_upgrade_to_sdmmc(entry, buf, buf_read_size, finsh_len);
	} else {
		printf("Not supported device!\n");
		return -ENOTSUP;
	}

	return ret;
}

int hcfota_from_path(const char *path, hcfota_report_t report_cb, unsigned long usrdata)
{
	struct hcfota_header *header;
	struct hcfota_payload_header *payload_header = NULL;
	void *payload;
	struct hcfota_progress progress = { 0 };
	struct sysdata sysdata = { 0 };
	int rc = 0;
	unsigned int i;
	struct stat sb;
	void *buf;
	int hcfota_fd;

	if (stat(path, &sb) == -1) {
		printf("%s:%d: path=%s is not exist\n", __func__, __LINE__, path);
		return -ENOENT;
	}

	/*check crc out*/
	rc = hcfota_cmp_header_crc(path);
	if (rc < 0)
		return HCFOTA_ERR_HEADER_CRC;

	hcfota_fd = open(path, O_RDWR);
	if (hcfota_fd  < 0)
		return -ENOENT;

	buf = malloc(sizeof(struct hcfota_header) + sizeof(struct hcfota_payload_header ));
	if (!buf)
		return -ENOMEM;

	read(hcfota_fd , buf, (sizeof(struct hcfota_header) + sizeof(struct hcfota_payload_header)));
	header = (struct hcfota_header *)buf;

	close(hcfota_fd);

	if (!sys_get_sysdata(&sysdata)) {
		if (header->version <= sysdata.firmware_version &&
		    !header->ignore_version_check)
			return HCFOTA_ERR_VERSION;
	}

	payload_header = (struct hcfota_payload_header *)(buf + sizeof(struct hcfota_header));

	switch(header->compress_type) {
	case IH_COMP_NONE:
		payload_header = (struct hcfota_payload_header *)(buf + sizeof(struct hcfota_header));
		break;
#ifdef CONFIG_LIB_GZIP
	case IH_COMP_GZIP: {
		printf("Compress type error: %d\n", header->compress_type);
		return -1;
	}
#endif

#ifdef CONFIG_LIB_LZMA
	case IH_COMP_LZMA: {
		printf("Compress type error: %d\n", header->compress_type);
		return -1;
	}
#endif
	default:
		printf("Compress type error: %d\n", header->compress_type);
		return -1;
	}

	progress.report_cb = report_cb;
	progress.usrdata = usrdata;
	for (i = 0; i < payload_header->entry_number; i++) {
		if (payload_header->entry[i].upgrade.entry_type == IH_ENTTRY_NORMAL) {
			if (payload_header->entry[i].upgrade.upgrade_enable)
				progress.total += payload_header->entry[i].upgrade.length;
		} else if (payload_header->entry[i].upgrade.entry_type == IH_ENTTRY_REMAP) {
			if (payload_header->entry[i].backup.upgrade_enable)
				progress.total += payload_header->entry[i].backup.old_length;
		}
	}

	if (progress.report_cb) {
		progress.report_cb(HCFOTA_REPORT_EVENT_UPGRADE_PROGRESS,
				   progress.current * 100 / progress.total,
				   progress.usrdata);
	}


	rc = 0;
	/* Do backup first */
	for (i = 0; i < payload_header->entry_number; i++) {
		if (payload_header->entry[i].upgrade.entry_type == IH_ENTTRY_REMAP) {
			rc |= hcfota_do_backup(&payload_header->entry[i], &progress);
			if (rc) {
				printf("Backup partition failed\n");
				hcfota_show_backup_entry(&payload_header->entry[i]);
			}
		}
	}

	/* Do upgrade */
	payload = (void *)((char *)payload_header + sizeof(struct hcfota_payload_header));
	for (i = 0; i < payload_header->entry_number; i++) {
		if (payload_header->entry[i].upgrade.entry_type == IH_ENTTRY_NORMAL) {
			rc |= hcfota_do_upgrade_from_path(&payload_header->entry[i], path, &progress);
			if (rc) {
				printf("Upgrade partition failed\n");
				hcfota_show_normal_entry(&payload_header->entry[i]);
			}
		}
	}

	if (rc < 0) {
#ifdef CONFIG_BOOT_UPGRADE_SHOW_WITH_LED
		if (progress.report_cb) {
			progress.report_cb(HCFOTA_REPORT_EVENT_UPGRADE_PROGRESS, 0xff, 0xff);
		}
#endif
		printf("\nUpgrade fail\n");
	} else {
		if (!header->ignore_version_update)
			hcfota_set_sysdata_version(header->version);
		printf("\nUpgrade success\n");
	}

	free(buf);
	return rc ? HCFOTA_ERR_UPGRADE : 0;
}

void hcfota_updata_progress(struct hcfota_progress *progress, enum HCFOTA_REPORT_EVENT event)
{
    if( progress && progress->total){
        progress->report_cb(
        event,
        progress->current * 100 / progress->total,
        progress->usrdata);
    }else{
        printf("err: hcfota_updata_progress, progress or progress->total is invaild");
    }

    return;
}

int hcfota_do_upgrade_extern_flash_data(const char *path, hcfota_external_flash_type_e flash_type,
    hcfota_report_t report_cb, unsigned long usrdata)
{
    struct stat sb;
    FILE *file_fp = NULL;
    long int file_size = 0;
    struct partition_state_s state = {0};
    long file_nblock = 0;
    char dev_path[32] = {0};
    struct geometry geo = {0};
    int once_len = 0, write_size = 0, nblocks = 0;
    blkcnt_t startblock = 0;
    void *buf = NULL;
    int ret = 0;
    struct hcfota_progress progress = {0};

    /* check file integrity */
    if (stat(path, &sb) == -1) {
		perror("stat");
		printf("stat failed\n");
		return -ENOENT;
	}

	file_fp = fopen(path, "rb");
	if (!file_fp) {
		printf("open %s failed\n", path);
		return -ENOENT;
	}

	file_size = (long int)sb.st_size;
	printf("%s:%d: file_size = %ld\n", __func__, __LINE__, file_size);

    /* file_size can be zero */
    //fseek(file_fp, 0, SEEK_END);
    //file_size = ftell(file_fp);
    if(file_size == 0){
        printf("%s:%d: file_size can be zero\n", __func__, __LINE__);
        ret = -ENOENT;
        goto err;
    }
    printf("%s:%d: file_size = %ld\n", __func__, __LINE__, file_size);

    /* init progress */
    progress.total = file_size;
    progress.report_cb = report_cb;
    progress.usrdata = usrdata;

    /* block device */
	switch(flash_type){
        case HCFOTA_EXTERNAL_FLASH_EMMC:
            strcpy(dev_path, "/dev/mmcblk0");
            break;

        case HCFOTA_EXTERNAL_FLASH_NAND:
        case HCFOTA_EXTERNAL_FLASH_SDCARD:
        case HCFOTA_EXTERNAL_FLASH_UDISK:
            printf("%s:%d: not support\n", __func__, __LINE__);
            break;

        default:
            printf("%s:%d: unkown flash type(%d)\n", __func__, __LINE__, flash_type);
            ret = -ENODEV;
            goto err;
	}

    ret = open_blockdriver(dev_path, MS_RDONLY, &state.blk);
    if (ret < 0) {
        printf("%s:%d:err: open_blockdriver failed\n", __func__, __LINE__);
        ret = -ENODEV;
        goto err;
    }

    state.mtd = NULL;
    ret = state.blk->u.i_bops->geometry(state.blk, &geo);
    if (ret < 0) {
        printf("%s:%d:err: geometry failed\n", __func__, __LINE__);
        ret = -ENODEV;
        goto err;
    }
    state.blocksize = geo.geo_sectorsize;
    state.erasesize = geo.geo_sectorsize;
    state.nblocks   = geo.geo_nsectors;

    file_nblock = file_size / state.blocksize;
    if(file_size % state.blocksize != 0)
	    file_nblock += 1;

    /* file size can not bigger then flash size */
    if((state.nblocks) < file_nblock){
        printf("%s:%d: file size can not bigger then flash size\n", __func__, __LINE__);
        printf("%s:%d: blocksize=%ld, nblocks=%lld, file_size=%ld\n", __func__, __LINE__,
            state.blocksize, state.nblocks, file_size);
        ret = -EINVAL;
        goto err;
    }

    buf = malloc(65536);
    if (buf == NULL) {
        printf("%s:%d:err: malloc failed\n", __func__, __LINE__);
        ret = -ENOMEM;
        goto err;
    }

    /* write data to flash */
    startblock = 0;
    fseek(file_fp, 0, SEEK_SET);
    once_len = fread(buf, 1, 65536, file_fp);
    while(once_len != 0){
        /* if last time is not a multiple of 512, what to do? */
        nblocks = once_len/512;
        if(once_len % 512){
            nblocks++;
        }
/*
        printf("%s:%d: startblock=%d, nblocks=%d, once_len=%d\n",
                __func__, __LINE__, startblock, nblocks, once_len);
*/
        write_size = state.blk->u.i_bops->write(state.blk, buf, startblock, nblocks);
        if(write_size != nblocks){
            printf("%s:%d: block write failed, startblock=%lld, nblocks=%d, write_size=%d\n",
                    __func__, __LINE__, startblock, nblocks, write_size);
            ret = -EBADF;
            goto err;
        }
        startblock += write_size;

        progress.current += once_len;
        hcfota_updata_progress(&progress, HCFOTA_REPORT_EVENT_UPGRADE_PROGRESS);

        once_len = fread(buf, 1, 65536, file_fp);
    }

    ret = 0;

err:
    if(file_fp){
        fclose(file_fp);
        file_fp = NULL;
    }

    if(state.blk){
        close_blockdriver(state.blk);
        state.blk = NULL;
    }

    if(buf){
        free(buf);
        buf = NULL;
    }

    return ret;
}

/*
 * partition distribution:
 * nor : hcboot/kernel/rootfs
 * emmc/nand: system_data/usr_data
 *
 */
int hcfota_url_extern_flash_data(const char *url, hcfota_external_flash_type_e flash_type,
    hcfota_report_t report_cb, unsigned long usrdata)
{
	int rc = 0;
	void *buf = NULL;
	unsigned long fota_size = 0;


	printf("hcfota_url_extern_flash_data: Upgrade from %s\n", url);

    /* if data image from network, download image to memory or udisk? */
	if (url_is_network(url)) {
		rc = hcfota_download(url, "/tmp/hcfota_data.bin", report_cb, usrdata);
		if (rc)
			return HCFOTA_ERR_DOWNLOAD;
		return hcfota_url_extern_flash_data("/tmp/hcfota_data.bin", flash_type, report_cb, usrdata);
	}

    return hcfota_do_upgrade_extern_flash_data(url, flash_type, report_cb, usrdata);
}

/*
 * partition distribution:
 * nor : hcboot
 * emmc/nand: kernel/rootfs/system_data/usr_data/...
 *
 */
int hcfota_url_extern_flash(const char *url, hcfota_external_flash_type_e flash_type,
    hcfota_report_t report_cb, unsigned long usrdata)
{
	printf("hcfota_url_extern_flash: Upgrade from %s\n", url);

    return 0;
}

