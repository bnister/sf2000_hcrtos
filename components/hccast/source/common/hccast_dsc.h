#ifndef _DSC_COMMON_H
#define _DSC_COMMON_H
#include <stdint.h>
#include <hcuapi/dsc.h>


typedef enum
{
    AES_CTR,
    AES_CBC,
} aes_mode_t;


typedef struct
{
    int fd;
    aes_mode_t mode;
    unsigned char key[16];
    unsigned char iv[16];
    struct dsc_algo_params algo_params;
	int mmap_size;
	unsigned char* mmap_addr;
} dsc_ctx_t;


dsc_ctx_t* hccast_dsc_open(aes_mode_t aes_mode,int mmap_size);
void hccast_dsc_close(dsc_ctx_t* dsc_ctx);
int hccast_dsc_aes_decrypt(dsc_ctx_t* dsc_ctx, unsigned char *key, unsigned char* iv, unsigned char *data, unsigned char *output, int size);

void* hccast_dsc_aes_ctr_open(int mmap_size);
void* hccast_dsc_aes_cbc_open(int mmap_size);
void hccast_dsc_ctx_destroy(void* ctx);
int hccast_dsc_decrypt(void* ctx, unsigned char *key, unsigned char *iv, unsigned char *input, unsigned char *output, int len);
int hccast_dsc_encrypt(void* ctx, unsigned char *key, unsigned char *iv, unsigned char *input, unsigned char *output, int len);


#endif
