// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2008 Semihalf
 *
 * (C) Copyright 2000-2006
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 */

#include <generated/br2_autoconf.h>

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <image.h>
#include <bootstage.h>
#include <display_options.h>
#include <kernel/lib/crc32.h>
#include <kernel/lib/lzma.h>
#include <kernel/lib/gzip.h>

#ifdef BR2_PACKAGE_LIBLZO
#include <lzo1x.h>
#endif

unsigned long image_load_addr = 0;	/* Default Load Address */

static const table_entry_t uimage_arch[] = {
	{	IH_ARCH_INVALID,	"invalid",	"Invalid ARCH",	},
	{	IH_ARCH_MIPS,		"mips",		"RISC",		},
	{	IH_ARCH_MIPS64,		"mips64",	"MIPS 64 Bit",	},
	{	IH_ARCH_SANDBOX,	"sandbox",	"Sandbox",	},
	{	-1,			"",		"",		},
};

static const table_entry_t uimage_os[] = {
	{	IH_OS_INVALID,	"invalid",	"Invalid OS",		},
	{	IH_OS_LINUX,	"linux",	"Linux",		},
	{	IH_OS_U_BOOT,	"u-boot",	"HC-BOOT",		},
	{	-1,		"",		"",			},
};

static const table_entry_t uimage_type[] = {
	{	IH_TYPE_FIRMWARE,   "firmware",	  "Firmware",		},
	{	IH_TYPE_KERNEL,	    "kernel",	  "Kernel Image",	},
	{	IH_TYPE_KERNEL_NOLOAD, "kernel_noload",  "Kernel Image (no loading done)", },
	{	IH_TYPE_RAMDISK,    "ramdisk",	  "RAMDisk Image",	},
	{	IH_TYPE_STANDALONE, "standalone", "Standalone Program", },
	{	-1,		    "",		  "",			},
};

static const table_entry_t uimage_comp[] = {
	{	IH_COMP_NONE,	"none",		"uncompressed",		},
	{	IH_COMP_BZIP2,	"bzip2",	"bzip2 compressed",	},
	{	IH_COMP_GZIP,	"gzip",		"gzip compressed",	},
	{	IH_COMP_LZMA,	"lzma",		"lzma compressed",	},
	{	IH_COMP_LZO,	"lzo",		"lzo compressed",	},
	{	IH_COMP_LZ4,	"lz4",		"lz4 compressed",	},
	{	IH_COMP_ZSTD,	"zstd",		"zstd compressed",	},
	{	-1,		"",		"",			},
};

struct table_info {
	const char *desc;
	int count;
	const table_entry_t *table;
};

static const struct comp_magic_map image_comp[] = {
	{	IH_COMP_BZIP2,	"bzip2",	{0x42, 0x5a},},
	{	IH_COMP_GZIP,	"gzip",		{0x1f, 0x8b},},
	{	IH_COMP_LZMA,	"lzma",		{0x5d, 0x00},},
	{	IH_COMP_LZO,	"lzo",		{0x89, 0x4c},},
	{	IH_COMP_NONE,	"none",		{},	},
};

static const struct table_info table_info[IH_COUNT] = {
	{ "architecture", IH_ARCH_COUNT, uimage_arch },
	{ "compression", IH_COMP_COUNT, uimage_comp },
	{ "operating system", IH_OS_COUNT, uimage_os },
	{ "image type", IH_TYPE_COUNT, uimage_type },
};

/*****************************************************************************/
/* Legacy format routines */
/*****************************************************************************/
int image_check_hcrc(const image_header_t *hdr)
{
	unsigned long hcrc;
	unsigned long len = image_get_header_size();
	image_header_t header;

	/* Copy header so we can blank CRC field for re-calculation */
	memmove(&header, (char *)hdr, image_get_header_size());
	image_set_hcrc(&header, 0);

	hcrc = crc32(0, (unsigned char *)&header, len);

	return (hcrc == image_get_hcrc(hdr));
}

int image_check_dcrc(const image_header_t *hdr)
{
	unsigned long data = image_get_data(hdr);
	unsigned long len = image_get_data_size(hdr);
	unsigned long dcrc = crc32_wd(0, (unsigned char *)data, len, CHUNKSZ_CRC32);

	return (dcrc == image_get_dcrc(hdr));
}

static void image_print_type(const image_header_t *hdr)
{
	const char __attribute__((__unused__)) *os, *arch, *type, *comp;

	os = genimg_get_os_name(image_get_os(hdr));
	arch = genimg_get_arch_name(image_get_arch(hdr));
	type = genimg_get_type_name(image_get_type(hdr));
	comp = genimg_get_comp_name(image_get_comp(hdr));

	printf("%s %s %s (%s)\n", arch, os, type, comp);
}

/**
 * image_print_contents - prints out the contents of the legacy format image
 * @ptr: pointer to the legacy format image header
 * @p: pointer to prefix string
 *
 * image_print_contents() formats a multi line legacy image contents description.
 * The routine prints out all header fields followed by the size/offset data
 * for MULTI/SCRIPT images.
 *
 * returns:
 *     no returned results
 */
void image_print_contents(const void *ptr)
{
	const image_header_t *hdr = (const image_header_t *)ptr;
	const char __attribute__((__unused__)) *p;

	p = IMAGE_INDENT_STRING;
	printf("%sImage Name:   %.*s\n", p, IH_NMLEN, image_get_name(hdr));
	printf("%sImage Type:   ", p);
	image_print_type(hdr);
	printf("%sData Size:    ", p);
	genimg_print_size(image_get_data_size(hdr));
	printf("%sLoad Address: %08lx\n", p, image_get_load(hdr));
	printf("%sEntry Point:  %08lx\n", p, image_get_ep(hdr));
}

/**
 * print_decomp_msg() - Print a suitable decompression/loading message
 *
 * @type:	OS type (IH_OS_...)
 * @comp_type:	Compression type being used (IH_COMP_...)
 * @is_xip:	true if the load address matches the image start
 */
static void print_decomp_msg(int comp_type, int type, bool is_xip)
{
	const char *name = genimg_get_type_name(type);

	if (comp_type == IH_COMP_NONE)
		printf("   %s %s\n", is_xip ? "XIP" : "Loading", name);
	else
		printf("   Uncompressing %s\n", name);
}

int image_decomp_type(const unsigned char *buf, unsigned long len)
{
	const struct comp_magic_map *cmagic = image_comp;

	if (len < 2)
		return -EINVAL;

	for (; cmagic->comp_id > 0; cmagic++) {
		if (!memcmp(buf, cmagic->magic, 2))
			break;
	}

	return cmagic->comp_id;
}

int image_decomp(int comp, unsigned long load, unsigned long image_start, int type,
		 void *load_buf, void *image_buf, unsigned long image_len,
		 unsigned long unc_len, unsigned long *load_end)
{
	int ret = 0;

	*load_end = load;
	print_decomp_msg(comp, type, load == image_start);

	/*
	 * Load the image to the right place, decompressing if needed. After
	 * this, image_len will be set to the number of uncompressed bytes
	 * loaded, ret will be non-zero on error.
	 */
	switch (comp) {
	case IH_COMP_NONE:
		if (load == image_start)
			break;
		if (image_len <= unc_len)
			memmove_wd(load_buf, image_buf, image_len, CHUNKSZ);
		else
			ret = -ENOSPC;
		break;
#ifdef BR2_PACKAGE_LIBLZO
	case IH_COMP_LZO: {
		unsigned long decomp_zimage = 0;

		ret = lzo1x_decompress((const unsigned char *)image_buf,image_len,load_buf, &decomp_zimage,NULL);

		image_len = decomp_zimage;
		break;
	}
#endif
#ifdef CONFIG_LIB_GZIP
	case IH_COMP_GZIP: {
		ret = gunzip(load_buf, unc_len, image_buf, &image_len);
		break;
	}
#endif /* CONFIG_GZIP */
#ifdef CONFIG_LIB_BZIP2
	case IH_COMP_BZIP2: {
		unsigned long size = unc_len;

		/*
		 * If we've got less than 4 MB of malloc() space,
		 * use slower decompression algorithm which requires
		 * at most 2300 KB of memory.
		 */
		ret = BZ2_bzBuffToBuffDecompress(load_buf, &size,
			image_buf, image_len,
			CONFIG_SYS_MALLOC_LEN < (4096 * 1024), 0);
		image_len = size;
		break;
	}
#endif /* CONFIG_BZIP2 */
#ifdef CONFIG_LIB_LZMA
	case IH_COMP_LZMA: {
		unsigned long lzma_len = unc_len;

		ret = lzmaBuffToBuffDecompress(load_buf, &lzma_len,
					       image_buf, image_len);
		image_len = lzma_len;
		break;
	}
#endif /* CONFIG_LZMA */
#ifdef CONFIG_LIB_LZO
	case IH_COMP_LZO: {
		size_t size = unc_len;

		ret = lzop_decompress(image_buf, image_len, load_buf, &size);
		image_len = size;
		break;
	}
#endif /* CONFIG_LZO */
#if CONFIG_LIB_LZ4
	case IH_COMP_LZ4: {
		size_t size = unc_len;

		ret = ulz4fn(image_buf, image_len, load_buf, &size);
		image_len = size;
		break;
	}
#endif /* CONFIG_LZ4 */
#if CONFIG_LIB_ZSTD
	case IH_COMP_ZSTD: {
		size_t size = unc_len;
		ZSTD_DStream *dstream;
		ZSTD_inBuffer in_buf;
		ZSTD_outBuffer out_buf;
		void *workspace;
		size_t wsize;

		wsize = ZSTD_DStreamWorkspaceBound(image_len);
		workspace = malloc(wsize);
		if (!workspace) {
			printf("%s: cannot allocate workspace of size %zu\n", __func__,
			      wsize);
			return -1;
		}

		dstream = ZSTD_initDStream(image_len, workspace, wsize);
		if (!dstream) {
			printf("%s: ZSTD_initDStream failed\n", __func__);
			return ZSTD_getErrorCode(ret);
		}

		in_buf.src = image_buf;
		in_buf.pos = 0;
		in_buf.size = image_len;

		out_buf.dst = load_buf;
		out_buf.pos = 0;
		out_buf.size = size;

		while (1) {
			size_t ret;

			ret = ZSTD_decompressStream(dstream, &out_buf, &in_buf);
			if (ZSTD_isError(ret)) {
				printf("%s: ZSTD_decompressStream error %d\n", __func__,
				       ZSTD_getErrorCode(ret));
				return ZSTD_getErrorCode(ret);
			}

			if (in_buf.pos >= image_len || !ret)
				break;
		}

		image_len = out_buf.pos;

		break;
	}
#endif /* CONFIG_ZSTD */
	default:
		printf("Unimplemented compression type %d\n", comp);
		return -ENOSYS;
	}

	*load_end = load + image_len;

	return ret;
}

/**
 * image_get_ramdisk - get and verify ramdisk image
 * @rd_addr: ramdisk image start address
 * @arch: expected ramdisk architecture
 * @verify: checksum verification flag
 *
 * image_get_ramdisk() returns a pointer to the verified ramdisk image
 * header. Routine receives image start address and expected architecture
 * flag. Verification done covers data and header integrity and os/type/arch
 * fields checking.
 *
 * returns:
 *     pointer to a ramdisk image header, if image was found and valid
 *     otherwise, return NULL
 */
static const image_header_t *image_get_ramdisk(unsigned long rd_addr, uint8_t arch,
						int verify)
{
	const image_header_t *rd_hdr = (const image_header_t *)rd_addr;

	if (!image_check_magic(rd_hdr)) {
		puts("Bad Magic Number\n");
		bootstage_error(BOOTSTAGE_ID_RD_MAGIC);
		return NULL;
	}

	if (!image_check_hcrc(rd_hdr)) {
		puts("Bad Header Checksum\n");
		bootstage_error(BOOTSTAGE_ID_RD_HDR_CHECKSUM);
		return NULL;
	}

	bootstage_mark(BOOTSTAGE_ID_RD_MAGIC);
	image_print_contents(rd_hdr);

	if (verify) {
		puts("   Verifying Checksum ... ");
		if (!image_check_dcrc(rd_hdr)) {
			puts("Bad Data CRC\n");
			bootstage_error(BOOTSTAGE_ID_RD_CHECKSUM);
			return NULL;
		}
		puts("OK\n");
	}

	bootstage_mark(BOOTSTAGE_ID_RD_HDR_CHECKSUM);

	if (!image_check_os(rd_hdr, IH_OS_LINUX) ||
	    !image_check_arch(rd_hdr, arch) ||
	    !image_check_type(rd_hdr, IH_TYPE_RAMDISK)) {
		printf("No Linux %s Ramdisk Image\n",
				genimg_get_arch_name(arch));
		bootstage_error(BOOTSTAGE_ID_RAMDISK);
		return NULL;
	}

	return rd_hdr;
}

/*****************************************************************************/
/* Shared dual-format routines */
/*****************************************************************************/
void memmove_wd(void *to, void *from, size_t len, unsigned long chunksz)
{
	memmove(to, from, len);
}

void genimg_print_size(uint32_t size)
{
	printf("%ld Bytes = ", size);
	print_size(size, "\n");
}

const table_entry_t *get_table_entry(const table_entry_t *table, int id)
{
	for (; table->id >= 0; ++table) {
		if (table->id == id)
			return table;
	}
	return NULL;
}

static const char *unknown_msg(enum ih_category category)
{
	static const char unknown_str[] = "Unknown ";
	static char msg[30];

	strcpy(msg, unknown_str);
	strncat(msg, table_info[category].desc,
		sizeof(msg) - sizeof(unknown_str));

	return msg;
}

/**
 * genimg_get_cat_name - translate entry id to long name
 * @category: category to look up (enum ih_category)
 * @id: entry id to be translated
 *
 * This will scan the translation table trying to find the entry that matches
 * the given id.
 *
 * @return long entry name if translation succeeds; error string on failure
 */
const char *genimg_get_cat_name(enum ih_category category, uint id)
{
	const table_entry_t *entry;

	entry = get_table_entry(table_info[category].table, id);
	if (!entry)
		return unknown_msg(category);
	return entry->lname;
}

/**
 * genimg_get_cat_short_name - translate entry id to short name
 * @category: category to look up (enum ih_category)
 * @id: entry id to be translated
 *
 * This will scan the translation table trying to find the entry that matches
 * the given id.
 *
 * @return short entry name if translation succeeds; error string on failure
 */
const char *genimg_get_cat_short_name(enum ih_category category, uint id)
{
	const table_entry_t *entry;

	entry = get_table_entry(table_info[category].table, id);
	if (!entry)
		return unknown_msg(category);
	return entry->sname;
}

int genimg_get_cat_count(enum ih_category category)
{
	return table_info[category].count;
}

const char *genimg_get_cat_desc(enum ih_category category)
{
	return table_info[category].desc;
}

/**
 * genimg_cat_has_id - check whether category has entry id
 * @category: category to look up (enum ih_category)
 * @id: entry id to be checked
 *
 * This will scan the translation table trying to find the entry that matches
 * the given id.
 *
 * @return true if category has entry id; false if not
 */
bool genimg_cat_has_id(enum ih_category category, uint id)
{
	if (get_table_entry(table_info[category].table, id))
		return true;

	return false;
}

/**
 * get_table_entry_name - translate entry id to long name
 * @table: pointer to a translation table for entries of a specific type
 * @msg: message to be returned when translation fails
 * @id: entry id to be translated
 *
 * get_table_entry_name() will go over translation table trying to find
 * entry that matches given id. If matching entry is found, its long
 * name is returned to the caller.
 *
 * returns:
 *     long entry name if translation succeeds
 *     msg otherwise
 */
char *get_table_entry_name(const table_entry_t *table, char *msg, int id)
{
	table = get_table_entry(table, id);
	if (!table)
		return msg;
	return table->lname;
}

const char *genimg_get_os_name(uint8_t os)
{
	return (get_table_entry_name(uimage_os, "Unknown OS", os));
}

const char *genimg_get_arch_name(uint8_t arch)
{
	return (get_table_entry_name(uimage_arch, "Unknown Architecture",
					arch));
}

const char *genimg_get_type_name(uint8_t type)
{
	return (get_table_entry_name(uimage_type, "Unknown Image", type));
}

const char *genimg_get_comp_name(uint8_t comp)
{
	return (get_table_entry_name(uimage_comp, "Unknown Compression",
					comp));
}

static const char *genimg_get_short_name(const table_entry_t *table, int val)
{
	table = get_table_entry(table, val);
	if (!table)
		return "unknown";
	return table->sname;
}

const char *genimg_get_type_short_name(uint8_t type)
{
	return genimg_get_short_name(uimage_type, type);
}

const char *genimg_get_comp_short_name(uint8_t comp)
{
	return genimg_get_short_name(uimage_comp, comp);
}

const char *genimg_get_os_short_name(uint8_t os)
{
	return genimg_get_short_name(uimage_os, os);
}

const char *genimg_get_arch_short_name(uint8_t arch)
{
	return genimg_get_short_name(uimage_arch, arch);
}

int genimg_get_os_id(const char *name)
{
	return (get_table_entry_id(uimage_os, "OS", name));
}

int genimg_get_arch_id(const char *name)
{
	return (get_table_entry_id(uimage_arch, "CPU", name));
}

int genimg_get_type_id(const char *name)
{
	return (get_table_entry_id(uimage_type, "Image", name));
}

int genimg_get_comp_id(const char *name)
{
	return (get_table_entry_id(uimage_comp, "Compression", name));
}

/**
 * genimg_get_kernel_addr_fit - get the real kernel address and return 2
 *                              FIT strings
 * @img_addr: a string might contain real image address
 * @fit_uname_config: double pointer to a char, will hold pointer to a
 *                    configuration unit name
 * @fit_uname_kernel: double pointer to a char, will hold pointer to a subimage
 *                    name
 *
 * genimg_get_kernel_addr_fit get the real kernel start address from a string
 * which is normally the first argv of bootm/bootz
 *
 * returns:
 *     kernel start address
 */
unsigned long genimg_get_kernel_addr_fit(char * const img_addr,
			     const char **fit_uname_config,
			     const char **fit_uname_kernel)
{
	unsigned long kernel_addr;

	/* find out kernel image address */
	if (!img_addr) {
		kernel_addr = image_load_addr;
		printf("*  kernel: default image load address = 0x%08lx\n",
		      image_load_addr);
	} else {
		kernel_addr = strtoul(img_addr, NULL, 16);
		printf("*  kernel: cmdline image address = 0x%08lx\n",
		       kernel_addr);
	}

	return kernel_addr;
}

/**
 * genimg_get_kernel_addr() is the simple version of
 * genimg_get_kernel_addr_fit(). It ignores those return FIT strings
 */
unsigned long genimg_get_kernel_addr(char * const img_addr)
{
	const char *fit_uname_config = NULL;
	const char *fit_uname_kernel = NULL;

	return genimg_get_kernel_addr_fit(img_addr, &fit_uname_config,
					  &fit_uname_kernel);
}

/**
 * genimg_get_format - get image format type
 * @img_addr: image start address
 *
 * genimg_get_format() checks whether provided address points to a valid
 * legacy or FIT image.
 *
 * New uImage format and FDT blob are based on a libfdt. FDT blob
 * may be passed directly or embedded in a FIT image. In both situations
 * genimg_get_format() must be able to dectect libfdt header.
 *
 * returns:
 *     image format type or IMAGE_FORMAT_INVALID if no image is present
 */
extern int fdt_check_header(const void *fdt);
int genimg_get_format(const void *img_addr)
{
	const image_header_t *hdr;

	hdr = (const image_header_t *)img_addr;
	if (image_check_magic(hdr))
		return IMAGE_FORMAT_LEGACY;

	if (fdt_check_header(img_addr) == 0)
		return IMAGE_FORMAT_FIT;

	return IMAGE_FORMAT_INVALID;
}

/**
 * fit_has_config - check if there is a valid FIT configuration
 * @images: pointer to the bootm command headers structure
 *
 * fit_has_config() checks if there is a FIT configuration in use
 * (if FTI support is present).
 *
 * returns:
 *     0, no FIT support or no configuration found
 *     1, configuration found
 */
int genimg_has_config(bootm_headers_t *images)
{
	return 0;
}

/**
 * boot_get_ramdisk - main ramdisk handling routine
 * @argc: command argument count
 * @argv: command argument list
 * @images: pointer to the bootm images structure
 * @arch: expected ramdisk architecture
 * @rd_start: pointer to a unsigned long variable, will hold ramdisk start address
 * @rd_end: pointer to a unsigned long variable, will hold ramdisk end
 *
 * boot_get_ramdisk() is responsible for finding a valid ramdisk image.
 * Curently supported are the following ramdisk sources:
 *      - multicomponent kernel/ramdisk image,
 *      - commandline provided address of decicated ramdisk image.
 *
 * returns:
 *     0, if ramdisk image was found and valid, or skiped
 *     rd_start and rd_end are set to ramdisk start/end addresses if
 *     ramdisk image is found and valid
 *
 *     1, if ramdisk image is found but corrupted, or invalid
 *     rd_start and rd_end are set to 0 if no ramdisk exists
 */
int boot_get_ramdisk(int argc, char *const argv[], bootm_headers_t *images,
		     uint8_t arch, unsigned long *rd_start, unsigned long *rd_end)
{
	unsigned long rd_addr, rd_load;
	unsigned long rd_data, rd_len;
	const image_header_t *rd_hdr;
	void *buf;
	char *end;
	const char *select = NULL;

	*rd_start = 0;
	*rd_end = 0;

	if (argc >= 2)
		select = argv[1];

	/*
	 * Look for a '-' which indicates to ignore the
	 * ramdisk argument
	 */
	if (select && strcmp(select, "-") ==  0) {
		printf("## Skipping init Ramdisk\n");
		rd_len = rd_data = 0;
	} else if (select || genimg_has_config(images)) {
			{
				rd_addr = strtoul(select, NULL, 16);
				printf("*  ramdisk: cmdline image address = "
						"0x%08lx\n",
						rd_addr);
			}

		/*
		 * Check if there is an initrd image at the
		 * address provided in the second bootm argument
		 * check image type, for FIT images get FIT node.
		 */
		buf = (void *)rd_addr;
		switch (genimg_get_format(buf)) {
		case IMAGE_FORMAT_LEGACY:
			printf("## Loading init Ramdisk from Legacy "
					"Image at %08lx ...\n", rd_addr);

			bootstage_mark(BOOTSTAGE_ID_CHECK_RAMDISK);
			rd_hdr = image_get_ramdisk(rd_addr, arch,
							images->verify);

			if (rd_hdr == NULL)
				return 1;

			rd_data = image_get_data(rd_hdr);
			rd_len = image_get_data_size(rd_hdr);
			rd_load = image_get_load(rd_hdr);
			break;
		default:
			end = NULL;
			if (select)
				end = strchr(select, ':');
			if (end) {
				rd_len = strtoul(++end, NULL, 16);
				rd_data = rd_addr;
			} else
			{
				puts("Wrong Ramdisk Image Format\n");
				rd_data = rd_len = rd_load = 0;
				return 1;
			}
		}
	} else {
		/*
		 * no initrd image
		 */
		bootstage_mark(BOOTSTAGE_ID_NO_RAMDISK);
		rd_len = rd_data = 0;
	}

	if (!rd_data) {
		printf("## No init Ramdisk\n");
	} else {
		*rd_start = rd_data;
		*rd_end = rd_data + rd_len;
	}
	printf("   ramdisk start = 0x%08lx, ramdisk end = 0x%08lx\n",
			*rd_start, *rd_end);

	return 0;
}
