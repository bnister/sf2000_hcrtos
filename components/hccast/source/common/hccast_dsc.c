#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>
#include <pthread.h>
#ifndef HC_RTOS
#include <sys/mman.h>
#endif
#include <hccast_dsc.h>
#include <hccast_log.h>

int hccast_dsc_mmp_init(dsc_ctx_t* dsc_ctx)
{
#ifdef HC_RTOS
    dsc_ctx->mmap_addr = memalign(32,dsc_ctx->mmap_size);
#else
    dsc_ctx->mmap_addr = (unsigned char*)mmap(NULL, dsc_ctx->mmap_size, PROT_READ | PROT_WRITE, MAP_SHARED, dsc_ctx->fd, 0);
#endif
    if (dsc_ctx->mmap_addr == NULL)
    {
        hccast_log(LL_ERROR,"[%s] fd: %d mmap size: %d is fail\n", __func__,dsc_ctx->fd, dsc_ctx->mmap_size);
    }
    else
    {
        hccast_log(LL_INFO,"[%s] fd: %d mmap size: %d is ok\n", __func__,dsc_ctx->fd, dsc_ctx->mmap_size);	
    }
}

int hccast_dsc_mmp_deinit(dsc_ctx_t* dsc_ctx)
{
    if(dsc_ctx->mmap_addr)
    {
#ifdef HC_RTOS
        free(dsc_ctx->mmap_addr);
#else
        munmap(dsc_ctx->mmap_addr, dsc_ctx->mmap_size);
#endif
        dsc_ctx->mmap_addr = NULL;
    }
}

dsc_ctx_t* hccast_dsc_open(aes_mode_t aes_mode, int size)
{
    dsc_ctx_t* dsc_ctx = (dsc_ctx_t*)malloc(sizeof(dsc_ctx_t)) ;
    if (dsc_ctx == NULL)
    {
        hccast_log(LL_ERROR,"dsc_ctx malloc fail\n");
        return NULL;
    }

    memset(dsc_ctx, 0, sizeof(dsc_ctx_t));

    dsc_ctx->fd = open("/dev/dsc", O_RDWR);
    if (dsc_ctx->fd < 0)
    {
        hccast_log(LL_ERROR,"Open /dev/dsc error, %s\n", strerror(errno));
        free(dsc_ctx);
        return NULL;
    }

    dsc_ctx->mmap_size = size;
    hccast_dsc_mmp_init(dsc_ctx);
    dsc_ctx->mode = aes_mode;

    return dsc_ctx;
}


void hccast_dsc_close(dsc_ctx_t* dsc_ctx)
{
    if (dsc_ctx == NULL)
        return ;
    hccast_dsc_mmp_deinit(dsc_ctx);
    close(dsc_ctx->fd);
    free(dsc_ctx);
}


int hccast_dsc_ctr_setkey(dsc_ctx_t* dsc_ctx, unsigned char *key, unsigned char* iv)
{
    dsc_ctx->algo_params.algo_type = DSC_ALGO_AES;
    dsc_ctx->algo_params.crypto_mode = DSC_DECRYPT;
    dsc_ctx->algo_params.chaining_mode = DSC_MODE_CTR;
    dsc_ctx->algo_params.residue_mode = DSC_RESIDUE_CLEAR;

    dsc_ctx->algo_params.key.buffer = dsc_ctx->key;
    dsc_ctx->algo_params.key.size = 16;

    dsc_ctx->algo_params.iv.buffer = dsc_ctx->iv;
    dsc_ctx->algo_params.iv.size = 16;

    if (key && iv)
    {
        memcpy(dsc_ctx->key, key, 16);
        memcpy(dsc_ctx->iv, iv, 16);
    }
    else if (iv == NULL)
    {
        return 0;
    }
    else
    {
        hccast_log(LL_DEBUG,"key and iv is null\n");
        return -1;
    }

    if (ioctl(dsc_ctx->fd, DSC_CONFIG, &dsc_ctx->algo_params) < 0)
    {
        hccast_log(LL_ERROR,"ioctl error, %s\n", strerror(errno));
        return -1;
    }

    return 0;
}

int hccast_dsc_cbc_setkey(dsc_ctx_t* dsc_ctx, unsigned char *key, unsigned char* iv)
{

    if (!key || !iv)
    {
        hccast_log(LL_WARNING,"%s :key and iv is null\n", __func__);
        return -1;
    }

    dsc_ctx->algo_params.algo_type = DSC_ALGO_AES;
    dsc_ctx->algo_params.crypto_mode = DSC_DECRYPT;
    dsc_ctx->algo_params.chaining_mode = DSC_MODE_CBC;
    dsc_ctx->algo_params.residue_mode = DSC_RESIDUE_CLEAR;

    dsc_ctx->algo_params.key.buffer = dsc_ctx->key;
    dsc_ctx->algo_params.key.size = 16;

    dsc_ctx->algo_params.iv.buffer = dsc_ctx->iv;
    dsc_ctx->algo_params.iv.size = 16;

    memcpy(dsc_ctx->key, key, 16);
    memcpy(dsc_ctx->iv, iv, 16);

    if (ioctl(dsc_ctx->fd, DSC_CONFIG, &dsc_ctx->algo_params) < 0)
    {
        hccast_log(LL_ERROR,"ioctl error, %s\n", strerror(errno));
        return -1;
    }

    return 0;
}

void hccast_dsc_common_decrypt(dsc_ctx_t* dsc_ctx, unsigned char *data, unsigned char *output, int size)
{
    struct dsc_crypt out = {0};
    
    if (!data || !output)
    {
        hccast_log(LL_WARNING,"decrypt data and output is null\n");
        return ;
    }

    if (dsc_ctx->mmap_addr == NULL)
    {
        hccast_log(LL_WARNING,"[%s]: dsc_ctx->mmap_addr is not init\n", __func__);
        return ;
    }

    if(size > dsc_ctx->mmap_size)
    {
        hccast_log(LL_WARNING,"[%s]: decrypt size-%d is bigger than %d \n", __func__,size,dsc_ctx->mmap_size);
        return ;
    }
#ifdef HC_RTOS
    if(((unsigned int)data & 0x1F) || ((unsigned int)output & 0x1F))
    {
        out.input = dsc_ctx->mmap_addr;
        out.size = size;
        out.output = dsc_ctx->mmap_addr;
        memcpy(dsc_ctx->mmap_addr, data, size);    
        ioctl(dsc_ctx->fd, DSC_DO_DECRYPT, &out);
        memcpy(output, dsc_ctx->mmap_addr, size);
    }
    else
    {
        out.input = data;
        out.size = size;
        out.output = output;
        ioctl(dsc_ctx->fd, DSC_DO_DECRYPT, &out);
    }
#else
    out.input = dsc_ctx->mmap_addr;
    out.size = size;
    out.output = dsc_ctx->mmap_addr;
    memcpy(dsc_ctx->mmap_addr, data, size);    
    ioctl(dsc_ctx->fd, DSC_DO_DECRYPT, &out);
    memcpy(output, dsc_ctx->mmap_addr, size);
#endif
}

int hccast_dsc_aes_decrypt(dsc_ctx_t* dsc_ctx, unsigned char *key, unsigned char* iv, unsigned char *data, unsigned char *output, int size)
{
    if (!dsc_ctx)
    {
        hccast_log(LL_WARNING,"dsc_ctx is null\n");
        return -1;
    }

    if (dsc_ctx->mode == AES_CTR)
    {
        if (hccast_dsc_ctr_setkey(dsc_ctx, key, iv) < 0)
        {
            hccast_log(LL_ERROR,"hc_dsc_ctr_setkey fail\n");
            return -1;

        }

    }
    else if (dsc_ctx->mode == AES_CBC)
    {
        if (hccast_dsc_cbc_setkey(dsc_ctx, key, iv) < 0)
        {
            hccast_log(LL_ERROR,"hc_dsc_cbc_setkey fail\n");
            return -1;
        }
    }


    hccast_dsc_common_decrypt(dsc_ctx, data, output, size);

    return 0;
}

int hccast_dsc_aes_encrypt(dsc_ctx_t* dsc_ctx, unsigned char *key, unsigned char *iv, unsigned char *input, unsigned char *output, int len)
{
    struct dsc_crypt out = {0};
    
    if (!dsc_ctx)
    {
        hccast_log(LL_WARNING,"dsc_ctx is null\n");
        return -1;
    }

    if (dsc_ctx->mode == AES_CBC)
    {
        unsigned char cbc_key_hw[16] = { 0 };
        unsigned char cbc_iv_hw[16] = { 0 };

        memcpy(cbc_key_hw, key, sizeof(cbc_key_hw));
        memcpy(cbc_iv_hw, iv, sizeof(cbc_iv_hw));

        struct dsc_algo_params algo_params_cbc =
        {
            .algo_type = DSC_ALGO_AES,
            .crypto_mode = DSC_ENCRYPT,
            /*.crypto_mode = DSC_DECRYPT,*/
            .chaining_mode =
            DSC_MODE_CBC, //output: 0a 94 3f d0 a0 99 33 fc 75 c7 6c d5 cb 14 76 0b
            .residue_mode = DSC_RESIDUE_CLEAR,
            .key = { cbc_key_hw, 16 },
            .iv = { cbc_iv_hw, 16 },
        };

        algo_params_cbc.crypto_mode = DSC_ENCRYPT;

        if (ioctl(dsc_ctx->fd, DSC_CONFIG, &algo_params_cbc) < 0)
        {
            hccast_log(LL_ERROR,"ioctl error, %s\n", strerror(errno));
            return -1;
        }

        if (dsc_ctx->mmap_addr == NULL)
        {
            hccast_log(LL_ERROR,"[%s]: dsc_ctx->mmap_addr is not init\n", __func__);
            return -1;
        }

        if(len > dsc_ctx->mmap_size)
        {
            hccast_log(LL_WARNING,"[%s]: decrypt size-%d is bigger than %d \n", __func__,len,dsc_ctx->mmap_size);
            return -1;
        }

        memcpy(dsc_ctx->mmap_addr, input, len);
        struct dsc_crypt out =
        {
            .input = dsc_ctx->mmap_addr,
            .size = len,
        };
        out.output = dsc_ctx->mmap_addr;
        ioctl(dsc_ctx->fd, DSC_DO_ENCRYPT, &out);
        memcpy(output, dsc_ctx->mmap_addr, len);
    }
    else if (dsc_ctx->mode == AES_CTR)
    {
        unsigned char ctr_key_hw[16] = { 0 };
        unsigned char ctr_iv_hw[16] = { 0 };

        memcpy(ctr_key_hw, key, sizeof(ctr_key_hw));
        memcpy(ctr_iv_hw, iv, sizeof(ctr_iv_hw));

        struct dsc_algo_params algo_params_ctr =
        {
            .algo_type = DSC_ALGO_AES,
            .crypto_mode = DSC_ENCRYPT,
            /*.crypto_mode = DSC_DECRYPT,*/
            .chaining_mode =
            DSC_MODE_CTR, //output: 0a 94 3f d0 a0 99 33 fc 75 c7 6c d5 cb 14 76 0b
            .residue_mode = DSC_RESIDUE_AS_ATSC,
            .key = { ctr_key_hw, 16 },
            .iv = { ctr_iv_hw, 16 },
        };

        algo_params_ctr.crypto_mode = DSC_ENCRYPT;

        if (ioctl(dsc_ctx->fd, DSC_CONFIG, &algo_params_ctr) < 0)
        {
            hccast_log(LL_ERROR,"ioctl error, %s\n", strerror(errno));
            return -1;
        }

        if (dsc_ctx->mmap_addr == NULL)
        {
            hccast_log(LL_WARNING,"[%s]: dsc_ctx->mmap_addr is not init\n", __func__);
            return -1;
        }

        if(len > dsc_ctx->mmap_size)
        {
            hccast_log(LL_WARNING,"[%s]: decrypt size-%d is bigger than %d \n", __func__,len,dsc_ctx->mmap_size);
            return -1;
        }

#ifdef HC_RTOS
        if(((unsigned int)input & 0x1F) || ((unsigned int)output & 0x1F))
        {
            out.input = dsc_ctx->mmap_addr;
            out.size = len;
            out.output = dsc_ctx->mmap_addr;
            memcpy(dsc_ctx->mmap_addr, input, len);    
            ioctl(dsc_ctx->fd, DSC_DO_ENCRYPT, &out);
            memcpy(output, dsc_ctx->mmap_addr, len);
        }
        else
        {
            out.input = input;
            out.size = len;
            out.output = output;
            ioctl(dsc_ctx->fd, DSC_DO_ENCRYPT, &out);
        }
#else
        out.input = dsc_ctx->mmap_addr;
        out.output = dsc_ctx->mmap_addr;
        out.size = len;
        memcpy(dsc_ctx->mmap_addr, input, len);
        ioctl(dsc_ctx->fd, DSC_DO_ENCRYPT, &out);
        memcpy(output, dsc_ctx->mmap_addr, len);    
#endif
    }
    else
    {
        hccast_log(LL_ERROR,"dsc_ctx->mode err! (%d) \n", dsc_ctx->mode);
    }

    return 0;
}

void* hccast_dsc_aes_ctr_open(int mmp_size)
{
    return (void*)hccast_dsc_open(AES_CTR,mmp_size);
}

void* hccast_dsc_aes_cbc_open(int mmp_size)
{
    return (void*)hccast_dsc_open(AES_CBC,mmp_size);
}

void hccast_dsc_ctx_destroy(void* ctx)
{
    hccast_dsc_close((dsc_ctx_t*)ctx);
}

int hccast_dsc_decrypt(void* ctx, unsigned char *key, unsigned char *iv, unsigned char *input, unsigned char *output, int len)
{
    return hccast_dsc_aes_decrypt((dsc_ctx_t*)ctx,key,iv,input,output,len);
}

int hccast_dsc_encrypt(void* ctx, unsigned char *key, unsigned char *iv, unsigned char *input, unsigned char *output, int len)
{
    return hccast_dsc_aes_encrypt((dsc_ctx_t*)ctx,key,iv,input,output,len);
}

