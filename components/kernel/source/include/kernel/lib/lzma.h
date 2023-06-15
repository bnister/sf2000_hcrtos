#ifndef _LZMA_
#define _LZMA_

#ifdef __cplusplus
extern "C" {
#endif

int lzmaBuffToBuffDecompress(unsigned char *outData, unsigned long *deompressedSize,
	 unsigned char *inData, unsigned long inLength);

#ifdef __cplusplus
}
#endif

#endif /* _LZMA_ */
