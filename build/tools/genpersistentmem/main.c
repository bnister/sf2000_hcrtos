#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <malloc.h>
#include <stddef.h>
#include <math.h>
#include <fcntl.h>
#include <getopt.h>
#include <errno.h>
#include <hcuapi/sysdata.h>
#include <hcuapi/persistentmem.h>

#ifndef UINT32_MAX
#define UINT32_MAX             (4294967295U)
#endif

//#define __DEBUG__

void *gbuf;
int gsize;

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

struct vnode {
	uint16_t id;
	uint16_t size;
};

#ifndef __LITTLE_ENDIAN
#define __LITTLE_ENDIAN 1234
#endif
#define __BYTE_ORDER    __LITTLE_ENDIAN

#if __BYTE_ORDER == __LITTLE_ENDIAN
#define cpu_to_le32(x) (x)
#define le32_to_cpu(x) (x)
#endif

#define tole(x) cpu_to_le32(x)
/* ========================================================================
 * Table of CRC-32's of all single-byte values (made by make_crc_table)
 */

static const uint32_t crc_table[256] = {
	tole(0x00000000L), tole(0x77073096L), tole(0xee0e612cL),
	tole(0x990951baL), tole(0x076dc419L), tole(0x706af48fL),
	tole(0xe963a535L), tole(0x9e6495a3L), tole(0x0edb8832L),
	tole(0x79dcb8a4L), tole(0xe0d5e91eL), tole(0x97d2d988L),
	tole(0x09b64c2bL), tole(0x7eb17cbdL), tole(0xe7b82d07L),
	tole(0x90bf1d91L), tole(0x1db71064L), tole(0x6ab020f2L),
	tole(0xf3b97148L), tole(0x84be41deL), tole(0x1adad47dL),
	tole(0x6ddde4ebL), tole(0xf4d4b551L), tole(0x83d385c7L),
	tole(0x136c9856L), tole(0x646ba8c0L), tole(0xfd62f97aL),
	tole(0x8a65c9ecL), tole(0x14015c4fL), tole(0x63066cd9L),
	tole(0xfa0f3d63L), tole(0x8d080df5L), tole(0x3b6e20c8L),
	tole(0x4c69105eL), tole(0xd56041e4L), tole(0xa2677172L),
	tole(0x3c03e4d1L), tole(0x4b04d447L), tole(0xd20d85fdL),
	tole(0xa50ab56bL), tole(0x35b5a8faL), tole(0x42b2986cL),
	tole(0xdbbbc9d6L), tole(0xacbcf940L), tole(0x32d86ce3L),
	tole(0x45df5c75L), tole(0xdcd60dcfL), tole(0xabd13d59L),
	tole(0x26d930acL), tole(0x51de003aL), tole(0xc8d75180L),
	tole(0xbfd06116L), tole(0x21b4f4b5L), tole(0x56b3c423L),
	tole(0xcfba9599L), tole(0xb8bda50fL), tole(0x2802b89eL),
	tole(0x5f058808L), tole(0xc60cd9b2L), tole(0xb10be924L),
	tole(0x2f6f7c87L), tole(0x58684c11L), tole(0xc1611dabL),
	tole(0xb6662d3dL), tole(0x76dc4190L), tole(0x01db7106L),
	tole(0x98d220bcL), tole(0xefd5102aL), tole(0x71b18589L),
	tole(0x06b6b51fL), tole(0x9fbfe4a5L), tole(0xe8b8d433L),
	tole(0x7807c9a2L), tole(0x0f00f934L), tole(0x9609a88eL),
	tole(0xe10e9818L), tole(0x7f6a0dbbL), tole(0x086d3d2dL),
	tole(0x91646c97L), tole(0xe6635c01L), tole(0x6b6b51f4L),
	tole(0x1c6c6162L), tole(0x856530d8L), tole(0xf262004eL),
	tole(0x6c0695edL), tole(0x1b01a57bL), tole(0x8208f4c1L),
	tole(0xf50fc457L), tole(0x65b0d9c6L), tole(0x12b7e950L),
	tole(0x8bbeb8eaL), tole(0xfcb9887cL), tole(0x62dd1ddfL),
	tole(0x15da2d49L), tole(0x8cd37cf3L), tole(0xfbd44c65L),
	tole(0x4db26158L), tole(0x3ab551ceL), tole(0xa3bc0074L),
	tole(0xd4bb30e2L), tole(0x4adfa541L), tole(0x3dd895d7L),
	tole(0xa4d1c46dL), tole(0xd3d6f4fbL), tole(0x4369e96aL),
	tole(0x346ed9fcL), tole(0xad678846L), tole(0xda60b8d0L),
	tole(0x44042d73L), tole(0x33031de5L), tole(0xaa0a4c5fL),
	tole(0xdd0d7cc9L), tole(0x5005713cL), tole(0x270241aaL),
	tole(0xbe0b1010L), tole(0xc90c2086L), tole(0x5768b525L),
	tole(0x206f85b3L), tole(0xb966d409L), tole(0xce61e49fL),
	tole(0x5edef90eL), tole(0x29d9c998L), tole(0xb0d09822L),
	tole(0xc7d7a8b4L), tole(0x59b33d17L), tole(0x2eb40d81L),
	tole(0xb7bd5c3bL), tole(0xc0ba6cadL), tole(0xedb88320L),
	tole(0x9abfb3b6L), tole(0x03b6e20cL), tole(0x74b1d29aL),
	tole(0xead54739L), tole(0x9dd277afL), tole(0x04db2615L),
	tole(0x73dc1683L), tole(0xe3630b12L), tole(0x94643b84L),
	tole(0x0d6d6a3eL), tole(0x7a6a5aa8L), tole(0xe40ecf0bL),
	tole(0x9309ff9dL), tole(0x0a00ae27L), tole(0x7d079eb1L),
	tole(0xf00f9344L), tole(0x8708a3d2L), tole(0x1e01f268L),
	tole(0x6906c2feL), tole(0xf762575dL), tole(0x806567cbL),
	tole(0x196c3671L), tole(0x6e6b06e7L), tole(0xfed41b76L),
	tole(0x89d32be0L), tole(0x10da7a5aL), tole(0x67dd4accL),
	tole(0xf9b9df6fL), tole(0x8ebeeff9L), tole(0x17b7be43L),
	tole(0x60b08ed5L), tole(0xd6d6a3e8L), tole(0xa1d1937eL),
	tole(0x38d8c2c4L), tole(0x4fdff252L), tole(0xd1bb67f1L),
	tole(0xa6bc5767L), tole(0x3fb506ddL), tole(0x48b2364bL),
	tole(0xd80d2bdaL), tole(0xaf0a1b4cL), tole(0x36034af6L),
	tole(0x41047a60L), tole(0xdf60efc3L), tole(0xa867df55L),
	tole(0x316e8eefL), tole(0x4669be79L), tole(0xcb61b38cL),
	tole(0xbc66831aL), tole(0x256fd2a0L), tole(0x5268e236L),
	tole(0xcc0c7795L), tole(0xbb0b4703L), tole(0x220216b9L),
	tole(0x5505262fL), tole(0xc5ba3bbeL), tole(0xb2bd0b28L),
	tole(0x2bb45a92L), tole(0x5cb36a04L), tole(0xc2d7ffa7L),
	tole(0xb5d0cf31L), tole(0x2cd99e8bL), tole(0x5bdeae1dL),
	tole(0x9b64c2b0L), tole(0xec63f226L), tole(0x756aa39cL),
	tole(0x026d930aL), tole(0x9c0906a9L), tole(0xeb0e363fL),
	tole(0x72076785L), tole(0x05005713L), tole(0x95bf4a82L),
	tole(0xe2b87a14L), tole(0x7bb12baeL), tole(0x0cb61b38L),
	tole(0x92d28e9bL), tole(0xe5d5be0dL), tole(0x7cdcefb7L),
	tole(0x0bdbdf21L), tole(0x86d3d2d4L), tole(0xf1d4e242L),
	tole(0x68ddb3f8L), tole(0x1fda836eL), tole(0x81be16cdL),
	tole(0xf6b9265bL), tole(0x6fb077e1L), tole(0x18b74777L),
	tole(0x88085ae6L), tole(0xff0f6a70L), tole(0x66063bcaL),
	tole(0x11010b5cL), tole(0x8f659effL), tole(0xf862ae69L),
	tole(0x616bffd3L), tole(0x166ccf45L), tole(0xa00ae278L),
	tole(0xd70dd2eeL), tole(0x4e048354L), tole(0x3903b3c2L),
	tole(0xa7672661L), tole(0xd06016f7L), tole(0x4969474dL),
	tole(0x3e6e77dbL), tole(0xaed16a4aL), tole(0xd9d65adcL),
	tole(0x40df0b66L), tole(0x37d83bf0L), tole(0xa9bcae53L),
	tole(0xdebb9ec5L), tole(0x47b2cf7fL), tole(0x30b5ffe9L),
	tole(0xbdbdf21cL), tole(0xcabac28aL), tole(0x53b39330L),
	tole(0x24b4a3a6L), tole(0xbad03605L), tole(0xcdd70693L),
	tole(0x54de5729L), tole(0x23d967bfL), tole(0xb3667a2eL),
	tole(0xc4614ab8L), tole(0x5d681b02L), tole(0x2a6f2b94L),
	tole(0xb40bbe37L), tole(0xc30c8ea1L), tole(0x5a05df1bL),
	tole(0x2d02ef8dL)
};

/* ========================================================================= */
#if __BYTE_ORDER == __LITTLE_ENDIAN
#define DO_CRC(x) crc = tab[(crc ^ (x)) & 255] ^ (crc >> 8)
#else
#define DO_CRC(x) crc = tab[((crc >> 24) ^ (x)) & 255] ^ (crc << 8)
#endif

/* ========================================================================= */

/* No ones complement version. JFFS2 (and other things ?)
 * don't use ones compliment in their CRC calculations.
 */
static uint32_t __crc32_no_comp(uint32_t crc, const uint8_t *buf, uint32_t len)
{
	const uint32_t *tab = crc_table;
	const uint32_t *b = (const uint32_t *)buf;
	uint32_t rem_len;

	crc = cpu_to_le32(crc);
	/* Align it */
	if (((long)b) & 3 && len) {
		uint8_t *p = (uint8_t *)b;
		do {
			DO_CRC(*p++);
		} while ((--len) && ((long)p) & 3);
		b = (uint32_t *)p;
	}

	rem_len = len & 3;
	len = len >> 2;
	for (--b; len; --len) {
		/* load data 32 bits wide, xor data 32 bits wide. */
		crc ^= *++b; /* use pre increment for speed */
		DO_CRC(0);
		DO_CRC(0);
		DO_CRC(0);
		DO_CRC(0);
	}
	len = rem_len;
	/* And the last few bytes */
	if (len) {
		uint8_t *p = (uint8_t *)(b + 1) - 1;
		do {
			DO_CRC(*++p); /* use pre increment for speed */
		} while (--len);
	}

	return le32_to_cpu(crc);
}

static uint32_t __crc32(uint32_t crc, const uint8_t *p, uint32_t len)
{
	return __crc32_no_comp(crc ^ 0xffffffffL, p, len) ^ 0xffffffffL;
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

static int node_create(struct persistentmem_node_create *node)
{
	struct vnode vnode;
	void *p;

	/* check if id duplicate */
	if (node_find(gbuf, gsize, node->id)) {
		return PERSISTENTMEM_ERR_ID_DUPLICATED;
	}

	p = node_find(gbuf, gsize, PERSISTENTMEM_NODE_ID_UNUSED);
	if ((p + sizeof(struct vnode) + node->size) >= (gbuf + gsize)) {
		gsize = (p + sizeof(struct vnode) + node->size) - gbuf;
		gbuf = realloc(gbuf, gsize);
	}

	p = node_find(gbuf, gsize, PERSISTENTMEM_NODE_ID_UNUSED);

	vnode.id = node->id;
	vnode.size = node->size;
	memcpy(p, (void *)&vnode, sizeof(struct vnode));

	return 0;
}

static int node_put(struct persistentmem_node *node)
{
	struct vnode *pvnode;
	int start;

	pvnode = node_find(gbuf, gsize, node->id);
	if (!pvnode) {
		return PERSISTENTMEM_ERR_ID_NOTFOUND;
	}

	if (node->offset + node->size > pvnode->size) {
		return PERSISTENTMEM_ERR_OVERFLOW;
	}

	start = (int)((void *)pvnode + sizeof(struct vnode) + node->offset - gbuf);
	memcpy(gbuf + start, node->buf, node->size);
	return 0;
}

static void print_usage(const char *prog)
{
	printf("Usage: %s [-vpo]\n", prog);
	puts("  -v --version  firmware version\n"
	     "  -p --product  product\n"
	     "  -V --volume   volume\n"
	     "  -t --tvtype   tvtype\n"
	     "  -o --output   output file\n");
}

int main(int argc, char *argv[])
{
	struct persistentmem_node_create new_node;
	struct persistentmem_node node;
	struct sysdata sysdata = { 0 };
	uint32_t crc;
	uint32_t i = 0;
	char *buf;
	int size;
	const char *fout = "persistentmem.bin";
	FILE *fpout;

	gsize = sizeof(struct vnode);
	gbuf = malloc(gsize);
	memset(gbuf, 0, gsize);

	strncpy(sysdata.product_id, "none", sizeof(sysdata.product_id));
	sysdata.firmware_version = 0;
	sysdata.ota_detect_modes = HCFOTA_REBOOT_OTA_DETECT_NONE;
	sysdata.tvtype = -1;
	sysdata.volume = -1;

	opterr = 0;
	optind = 0;
	while (1) {
		static const struct option lopts[] = {
			{ "version", 1, 0, 'v' },
			{ "product", 1, 0, 'p' },
			{ "volume", 1, 0, 'V' },
			{ "tvtype", 1, 0, 't' },
			{ "output", 1, 0, 'o' },
			{ NULL, 0, 0, 0 },
		};
		int c;

		c = getopt_long(argc, argv, "v:p:V:t:o:", lopts, NULL);

		if (c == -1)
			break;

		switch (c) {
		case 'v':
			sysdata.firmware_version = atoi(optarg);
			break;
		case 'V':
			sysdata.volume = atoi(optarg);
			break;
		case 't':
			sysdata.tvtype = atoi(optarg);
			break;
		case 'p':
			memset(sysdata.product_id, 0, sizeof(sysdata.product_id));
			strncpy(sysdata.product_id, optarg, sizeof(sysdata.product_id));
			break;
		case 'o':
			fout = optarg;
			break;
		default:
			print_usage(argv[0]);
			return -1;
		}
	}

#ifdef __DEBUG__
	printf("\ngbuf:\n");
	for (i = 0; i < gsize; i++)
		printf("%02x ", *(unsigned char *)(gbuf + i));
	printf("\n");
#endif

	new_node.id = PERSISTENTMEM_NODE_ID_SYSDATA;
	new_node.size = ((sizeof(struct sysdata) + 3) >> 2) << 2;
	node_create(&new_node);

#ifdef __DEBUG__
	printf("node size %d\n", new_node.size);
	printf("sizeof sysdata %ld\n", sizeof(struct sysdata));
	printf("offset of sysdata.product_id %ld\n", offsetof(struct sysdata, product_id));
	printf("offset of sysdata.firmware_version %ld\n", offsetof(struct sysdata, firmware_version));
	printf("offset of sysdata.ota_detect_modes %ld\n", offsetof(struct sysdata, ota_detect_modes));
	printf("offset of sysdata.tvtype %ld\n", offsetof(struct sysdata, tvtype));
	printf("offset of sysdata.volume %ld\n", offsetof(struct sysdata, volume));

	printf("\ngbuf:\n");
	for (i = 0; i < gsize; i++)
		printf("%02x ", *(unsigned char *)(gbuf + i));
	printf("\n");
#endif

	node.id = PERSISTENTMEM_NODE_ID_SYSDATA;
	node.offset = 0;
	node.size = sizeof(struct sysdata);
	node.buf = &sysdata;
	node_put(&node);

#ifdef __DEBUG__
	printf("\ngbuf:\n");
	for (i = 0; i < gsize; i++)
		printf("%02x ", *(unsigned char *)(gbuf + i));
	printf("\n");
#endif

	size = gsize + EXTRA_SIZE;
	buf = malloc(size);
	memset(buf, 0, size);

	i = 0;
	buf[i++] = ((gsize >> 8) & 0xff);
	buf[i++] = (gsize & 0xff);
	buf[i++] = ((0 >> 8) & 0xff);
	buf[i++] = (0 & 0xff);
	memcpy(buf + i, gbuf, gsize);
	i += gsize;
	crc = __crc32(UINT32_MAX, (uint8_t *)gbuf, gsize);
	memcpy(buf + i, (void *)&crc, sizeof(crc));

#ifdef __DEBUG__
	printf("\nbuf:\n");
	for (i = 0; i < size; i++)
		printf("%02x ", *(unsigned char *)(buf + i));
	printf("\n");
#endif

	fpout = fopen(fout, "wb+");
	if (fpout == NULL) {
		printf("open %s failed\n", fout);
		free(gbuf);
		free(buf);
		return -EIO;
	}

	fwrite(buf, 1, size, fpout);
	fflush(fpout);
	fclose(fpout);
	return 0;
}
