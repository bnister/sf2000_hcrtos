/*
 *  This util is base on LzmaDec.c file from LZMA SDK 16.04
 *  
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "LzmaDec.h"
#include "Types.h"

#define LZMA_PROPERTIES_OFFSET 0
#define LZMA_SIZE_OFFSET LZMA_PROPS_SIZE
#define LZMA_DATA_OFFSET (LZMA_SIZE_OFFSET + sizeof(UInt64))

//#define LZMA_DEBUG

#ifdef LZMA_DEBUG
#define LZMA_INFO(fmt, args...)                                                \
	do {                                                                   \
		printf("\nfunc:%s    line:%d   ", __FUNCTION__, __LINE__);     \
		printf(fmt, ##args);                                           \
	} while (0)
#else
#define LZMA_INFO(fmt, args...)                                                \
	do {                                                                   \
	} while (0)
#endif

#define LZMA_MEMSET memset

static void *lzmaAlloc(void *p, size_t size)
{
	return malloc(size);
}
static void lzmaFree(void *p, void *address)
{
	free(address);
}

int lzmaBuffToBuffDecompress(unsigned char *outData, unsigned long *deompressedSize,
	 unsigned char *inData, unsigned long inLength)
{
	int ret = SZ_ERROR_DATA;
	int i = 0;
	ISzAlloc g_lzmaAlloc = { NULL, NULL };

	SizeT outSizeFull = 0xFFFFFFFF; /* 4GBytes limit */
	SizeT compressedSize = (SizeT)(inLength - LZMA_PROPS_SIZE);
	SizeT outProcessed = 0;
	SizeT outSize = 0;
	SizeT outSizeHigh = 0;

	ELzmaStatus status;
	LZMA_MEMSET(&status, 0, sizeof(status));

	g_lzmaAlloc.Alloc = lzmaAlloc;
	g_lzmaAlloc.Free = lzmaFree;

	LZMA_INFO("LZMA: Image address............... 0x%p\n", inData);
	LZMA_INFO("LZMA: Destination address......... 0x%p\n", outData);

	/* get the uncompressed size */
	for (i = 0; i < 8; i++) {
		unsigned char b = inData[LZMA_SIZE_OFFSET + i];
		if (i < 4)
			outSize += (UInt32)(b) << (i * 8);
		else
			outSizeHigh += (UInt32)(b) << ((i - 4) * 8);
	}

	outSizeFull = (SizeT)outSize;
	/*
     * SizeT is a 64 bit uint => We can manage files larger than 4GB!
     */
	if (sizeof(SizeT) >= 8) {
		outSizeFull |= (((SizeT)outSizeHigh << 16) << 16);
	} else if (outSizeHigh != 0 || (UInt32)(SizeT)outSize != outSize) {
		/*
         * SizeT is a 32 bit uint => We cannot manage files larger than
         * 4GB!  Assume however that all 0xf values is "unknown size" and
         * not actually a file of 2^64 bits.
         *
         */
		if (outSizeHigh != (SizeT)-1 || outSize != (SizeT)-1) {
			LZMA_INFO("LZMA: 64bit support not enabled.\n");
			return SZ_ERROR_DATA;
		}
	}
	/* if the uncompressed data size bigger than deompressedSize */
	if ((outSizeFull != (SizeT)-1) && (*deompressedSize < outSizeFull))
		return SZ_ERROR_OUTPUT_EOF;

	/* Decompress length */
	outProcessed = ((outSizeFull > *deompressedSize) ? *deompressedSize :
							   outSizeFull);

	ret = LzmaDecompress(outData, &outProcessed, inData + LZMA_DATA_OFFSET,
			     &compressedSize, inData, LZMA_PROPS_SIZE,
			     LZMA_FINISH_END, &status, &g_lzmaAlloc);

	/* The size of the actual decompression */
	*deompressedSize = outProcessed;
	LZMA_INFO("LZMA: Uncompressed ............... 0x%lx,ret:%x\n",
		  *deompressedSize, ret);

	return ret;
}
