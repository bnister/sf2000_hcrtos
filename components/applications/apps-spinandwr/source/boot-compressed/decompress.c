#include <generated/br2_autoconf.h>
#include "lzo1x/lzo1x.h"

unsigned long free_mem_ptr;
unsigned long free_mem_end_ptr;

/* The linker tells us where the image is. */
extern unsigned char __image_begin, __image_end;
void *memcpy(void *dest, const void *src, unsigned int n);
int lzmaBuffToBuffDecompress(unsigned char *outData, unsigned long *deompressedSize,
	 unsigned char *inData, unsigned long inLength);
void cache_flush(void *src, unsigned long len, unsigned long cacheline);

void free(void *ptr)
{
	return;
}

void *malloc(unsigned int size)
{
	void *tmp = (void *)free_mem_ptr;

	free_mem_ptr += size;

	return tmp;
}

#if defined(CONFIG_SPINANDWR_SELFCOMPRESSED_LZMA)

void decompress_kernel(unsigned long boot_heap_start)
{
	unsigned long zimage_start, zimage_size;
	unsigned long lzma_len = 0x200000;
	unsigned long cacheline = 16;

	free_mem_ptr = boot_heap_start;
	free_mem_end_ptr = boot_heap_start + BOOT_HEAP_SIZE;

	zimage_start = (unsigned long)(&__image_begin);
	zimage_size = (unsigned long)(&__image_end) -
	    (unsigned long)(&__image_begin);

	/* Decompress the kernel with according algorithm */
	lzmaBuffToBuffDecompress((unsigned char *)VMLINUX_LOAD_ADDRESS_ULL, &lzma_len,
					       (unsigned char *)zimage_start, zimage_size);

	if (((*(volatile unsigned long *)(0xb8800000)) & 0xffff0000) == 0x1512)
		cacheline = 16;
	else if (((*(volatile unsigned long *)(0xb8800000)) & 0xffff0000) == 0x1600)
		cacheline = 32;

	cache_flush((unsigned char *)VMLINUX_LOAD_ADDRESS_ULL, lzma_len, cacheline);
}

#elif defined(CONFIG_SPINANDWR_SELFCOMPRESSED_LZO1X)

void decompress_kernel(unsigned long boot_heap_start)
{
	int ret = -1;
	unsigned long zimage_start, zimage_size;
	unsigned long decom_zimage_len = 0x200000;
	unsigned long cacheline = 16;

	free_mem_ptr = boot_heap_start;
	free_mem_end_ptr = boot_heap_start + BOOT_HEAP_SIZE;

	zimage_start = (unsigned long)(&__image_begin);
	zimage_size = (unsigned long)(&__image_end) -
	    (unsigned long)(&__image_begin);

	/* lzo */
	ret = lzo1x_decompress((const unsigned char *)zimage_start,zimage_size,(unsigned char *)VMLINUX_LOAD_ADDRESS_ULL,&decom_zimage_len,NULL);
	if (ret < 0)
		/* error */

	if (((*(volatile unsigned long *)(0xb8800000)) & 0xffff0000) == 0x1512)
		cacheline = 16;
	else if (((*(volatile unsigned long *)(0xb8800000)) & 0xffff0000) == 0x1600)
		cacheline = 32;

	cache_flush((unsigned char *)VMLINUX_LOAD_ADDRESS_ULL, decom_zimage_len, cacheline);
}

#endif
