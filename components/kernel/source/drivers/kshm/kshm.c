#define LOG_TAG "MODULE"
#define ELOG_OUTPUT_LVL ELOG_LVL_ERROR

#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <unistd.h>
#include <kernel/io.h>
#include <kernel/elog.h>
#include <kernel/delay.h>
#include <kernel/types.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <kernel/hwspinlock.h>

#include <hcuapi/common.h>
#include <hcuapi/kshm.h>
#include <kernel/module.h>
#include <kernel/wait.h>

#ifndef max
#define max(a, b)	(((a) > (b)) ? (a) : (b))
#endif

#define ROUNDUP(a, b)	(((a) + ((b) - 1)) & ~((b) - 1))

#define KSHM_REQUEST_READ_BLOCKING_MODE 0

struct kshm_private {
	wait_queue_head_t wait;
};

uint32_t __attribute__((weak)) dma_copy(uint8_t channel , void *src , void *dst , uint32_t size , uint8_t flags)
{
	memcpy(dst, src, size);
	return 0;
}

uint8_t __attribute__((weak)) dma_open(uint8_t channel, uint32_t flags, void *sc_attr)
{
	return -1;
}

uint8_t __attribute__((weak)) dma_open2(uint8_t channel, uint32_t flags, void *sc_attr, void (*done)(uint32_t), uint32_t param)
{
	return -1;
}

void __attribute__((weak)) dma_close(uint8_t channel)
{
}

void __attribute__((weak)) dma_close2(uint8_t channel, void (*done)(uint32_t), uint32_t param)
{
}

void __attribute__((weak)) dma_wait(uint32_t id , uint8_t mode)
{
}

static int kshm_allocate_buffer(struct kshm_cfg *cfg, size_t size)
{
	cfg->size = size;

	cfg->base = memalign(32, cfg->size);
	if (!cfg->base){
		return KSHM_FAIL;
	}

	return KSHM_OK;
}

static int kshm_free_buffer(struct kshm_cfg *cfg)
{
	if (cfg->base) {
		vPortCacheInvalidate(cfg->base, cfg->size);
		free(cfg->base);
		cfg->base = NULL;
	}

	return 0;
}

static void kshm_dma_copy_done(uint32_t param)
{
	struct kshm_info *info = (struct kshm_info *)param;

	if (!info) {
		return;
	}

	if (info->desc->dma_xfer_id != -1) {
		kshm_update_write(info, info->desc->update_write_size);
		info->desc->dma_xfer_id = -1;
	}
}

kshm_handle_t kshm_create(size_t size)
{
	struct kshm_info *info = NULL;
	struct kshm_private *priv;

	info = malloc(sizeof(struct kshm_info) + sizeof(struct kshm_private));
	if (!info) {
		return NULL;
	}
	memset(info, 0, sizeof(struct kshm_info) + sizeof(struct kshm_private));

	/* addr & size must all be 32 bytes align */
	info->desc = memalign(32, ROUNDUP(sizeof(struct kshm_desc), 32));
	if (!info->desc) {
		free(info);
		return NULL;
	}
	memset(info->desc, 0, sizeof(struct kshm_desc));

#ifdef CONFIG_DRV_KSHM_FOR_DUALCORE
	vPortCacheFlush(info->desc, sizeof(struct kshm_desc));
	info->desc = (struct kshm_desc *)MIPS_UNCACHED_ADDR(info->desc);
#endif

	if (kshm_allocate_buffer(&info->cfg, size) != KSHM_OK) {
#ifdef CONFIG_DRV_KSHM_FOR_DUALCORE
		info->desc = (struct kshm_desc *)MIPS_CACHED_ADDR(info->desc);
		vPortCacheInvalidate(info->desc, sizeof(struct kshm_desc));
#endif
		free(info->desc);
		free(info);
		return NULL;
	}

	priv = (struct kshm_private *)&info->opaque;

	info->cfg.dma_ch = dma_open(8, 0, NULL);
	info->desc->dma_xfer_id = -1;
	init_waitqueue_head(&priv->wait);

	return info;
}

int kshm_set_data_direction(kshm_handle_t hdl, enum KSHM_DATA_DIRECTION data_direction)
{
	struct kshm_info *info = (struct kshm_info *)hdl;

	if (!info) {
		return KSHM_FAIL;
	}

	info->cfg.data_direction = data_direction;
	return KSHM_OK;
}

kshm_handle_t kshm_create_ext(size_t size, enum KSHM_DATA_DIRECTION data_direction)
{
	kshm_handle_t hdl = kshm_create(size);
	kshm_set_data_direction(hdl, data_direction);
	return hdl;
}

int kshm_destroy(kshm_handle_t hdl)
{
	struct kshm_info *info = (struct kshm_info *)hdl;

	if (!info) {
		return KSHM_FAIL;
	}

	kshm_free_buffer(&info->cfg);
	if (info->desc) {
#ifdef CONFIG_DRV_KSHM_FOR_DUALCORE
		info->desc = (struct kshm_desc *)MIPS_CACHED_ADDR(info->desc);
		vPortCacheInvalidate(info->desc, sizeof(struct kshm_desc));
#endif
		free(info->desc);
	}

	if (info->cfg.dma_ch != -1)
		dma_close(info->cfg.dma_ch);

	free(info);

	return KSHM_OK;
}

int kshm_reset(kshm_handle_t hdl)
{
	struct kshm_info *info = (struct kshm_info *)hdl;

	if (!info) {
		return KSHM_OK;
	}

	memset(info->desc, 0, sizeof (struct kshm_desc));

	return 0;
}

int kshm_special_reset(kshm_handle_t hdl)
{
	struct kshm_info *info = (struct kshm_info *)hdl;
	size_t index;

	if (!info) {
		return KSHM_OK;
	}

	index = 1 - info->desc->index;
	info->desc->rd[index] = 0;
	info->desc->wt[index] = 0;
	info->desc->reserved[index] = 0;
	info->desc->index = index;
	info->desc->rd_times = info->desc->wt_times = 0;

	return KSHM_OK;
}

void *kshm_request_read(kshm_handle_t hdl, size_t size)
{
	struct kshm_info *info = (struct kshm_info *)hdl;
	size_t wt, rd, avail, index;
#if KSHM_REQUEST_READ_BLOCKING_MODE
	struct kshm_private *priv;
	int err_retry = 1;
#endif

	if (!info || !size) {
		return NULL;
	}

	size = ROUNDUP(size, 32);

retry:
	index = info->desc->index;
	wt = info->desc->wt[index];
	rd = info->desc->rd[index];

	if (wt >= rd) {
		avail = wt - rd;
	} else {
		avail = max(info->cfg.size - rd, wt);
	}

	if (avail < size ) {
#ifndef CONFIG_DRV_KSHM_FOR_DUALCORE
#if KSHM_REQUEST_READ_BLOCKING_MODE
		priv = (struct kshm_private *)&info->opaque;
		if (wait_event_timeout(priv->wait, wt != info->desc->wt[info->desc->index], 40) <= 0) {
			/* timeout */
			return NULL;
		} else if (err_retry--) {
			goto retry;
		}
#endif
		return NULL;
#else
		return NULL;
#endif
	}

	if (rd + size > info->cfg.size) {
		/* loop back */
#ifndef CONFIG_DRV_KSHM_FOR_DUALCORE
		if ((info->cfg.data_direction == KSHM_FROM_DEVICE ||
		     info->cfg.data_direction == KSHM_BIDIRECTIONAL) &&
		    (info->desc->rd_times % 2)) {
			vPortCacheInvalidate(info->cfg.base, size);
		}
#else
		vPortCacheInvalidate(info->cfg.base, size);
#endif
		return info->cfg.base;
	}

#ifndef CONFIG_DRV_KSHM_FOR_DUALCORE
	if ((info->cfg.data_direction == KSHM_FROM_DEVICE ||
	     info->cfg.data_direction == KSHM_BIDIRECTIONAL) &&
	    (info->desc->rd_times % 2)) {
		vPortCacheInvalidate(info->cfg.base + rd, size);
	}
#else
	vPortCacheInvalidate(info->cfg.base + rd, size);
#endif

#ifdef CONFIG_DRV_KSHM_FOR_DUALCORE
	write_sync();
#endif

	return info->cfg.base + rd;
}

int kshm_update_read(kshm_handle_t hdl, size_t size)
{
	struct kshm_info *info = (struct kshm_info *)hdl;
	size_t index;
	size_t rd;

	if (!info) {
		return KSHM_FAIL;
	}

	size = ROUNDUP(size, 32);
	index = info->desc->index;
	rd = info->desc->rd[index];
	if (rd + size > info->cfg.size) {
		rd = size;
	} else {
		rd += size;
		if (rd == info->cfg.size) {
			rd = 0;
		}
	}

	info->desc->rd[index] = rd;
	info->desc->rd_times++;

	return KSHM_OK;

}

int kshm_update_reserved(kshm_handle_t hdl, size_t size)
{
	struct kshm_info *info = (struct kshm_info *)hdl;

	if (!info) {
		return KSHM_FAIL;
	}

	size = ROUNDUP(size, 32);

	info->desc->reserved[info->desc->index] = size;

#ifdef CONFIG_DRV_KSHM_FOR_DUALCORE
	write_sync();
#endif

	return KSHM_OK;
}

int kshm_read(kshm_handle_t hdl, void *buf, size_t size)
{
	struct kshm_info *info = (struct kshm_info *)hdl;
	void *request = NULL;

	if (!info || !size) {
		return KSHM_FAIL;
	}

	request = kshm_request_read(hdl, size);
	if (!request) {
		return KSHM_FAIL;
	}

	memcpy(buf, request, size);

	kshm_update_read(hdl, size);

	return size;
}

void *kshm_request_write(kshm_handle_t hdl, size_t size)
{
	struct kshm_info *info = (struct kshm_info *)hdl;
	size_t rd, wt, tail, head, reserved, index;

	if (!info || !size) {
		return NULL;
	}

	size = ROUNDUP(size, 32);

	index = info->desc->index;
	rd = info->desc->rd[index];
	wt = info->desc->wt[index];
	reserved = info->desc->reserved[index];

	if (wt >= rd) {
		tail = info->cfg.size - wt;
		head = rd;
	} else {
		tail = rd - wt;
		head = 0;
	}

	if (head < reserved + size && tail < reserved + size) {
		if (rd == wt) {
			if (size > info->cfg.size) {
				log_e("err, write size > kshm total size\n");
			} else {
				log_e("warning, write size > kshm remain size\n");
				kshm_special_reset(hdl);
				msleep(100);
				return info->cfg.base;
			}
		}
		return NULL;
	}

	if (tail > size) {
		return info->cfg.base + wt;
	} else if (tail == size && head > 0) {
		return info->cfg.base + wt;
	} else if (head > size) {
		return info->cfg.base;
	}

	return NULL;
}

int kshm_update_write(kshm_handle_t hdl, size_t size)
{
	struct kshm_info *info = (struct kshm_info *)hdl;
	size_t wt, index;
#if KSHM_REQUEST_READ_BLOCKING_MODE
	struct kshm_private *priv;
#endif

	if (!info || !size) {
		return KSHM_FAIL;
	}

	size = ROUNDUP(size, 32);

	index = info->desc->index;
	wt = info->desc->wt[index];
	
	if (wt + size > info->cfg.size) {
		wt = size;
	} else {
		wt += size;
		if (wt == info->cfg.size) {
			wt = 0;
		}
	}

#ifdef CONFIG_DRV_KSHM_FOR_DUALCORE
	write_sync();
#endif

	info->desc->wt[index] = wt;
	info->desc->wt_times++;

#if KSHM_REQUEST_READ_BLOCKING_MODE
	priv = (struct kshm_private *)&info->opaque;
	wake_up(&priv->wait);
#endif
	return KSHM_OK;
}

int kshm_write(kshm_handle_t hdl, void *buf, size_t size)
{
	struct kshm_info *info = (struct kshm_info *)hdl;
	void *request = NULL;

	if (!info || !size) {
		return KSHM_FAIL;
	}

	request = kshm_request_write(hdl, size);
	if (!request) {
		return KSHM_FAIL;
	}

	if (size < 512 || info->cfg.dma_ch == -1) {
		memcpy(request, buf, size);
#ifndef CONFIG_DRV_KSHM_FOR_DUALCORE
		if ((info->cfg.data_direction == KSHM_TO_DEVICE ||
		     info->cfg.data_direction == KSHM_BIDIRECTIONAL) &&
		    (info->desc->wt_times % 2)) {
			vPortCacheFlush(request, size);
		}
#else
		vPortCacheFlush(request, size);
#endif
		kshm_update_write(hdl, size);
	} else {
		vPortCacheFlush(buf, size);
		vPortCacheInvalidate(request, size);
		info->desc->dma_xfer_id = dma_copy(info->cfg.dma_ch, buf, request, size, 0);
		info->desc->update_write_size = size;
		if (info->desc->dma_xfer_id != -1) {
			dma_wait(info->desc->dma_xfer_id, 0);
			kshm_update_write(hdl, info->desc->update_write_size);
			info->desc->dma_xfer_id = -1;
		}
	}

	return size;
}

size_t kshm_get_valid_size(kshm_handle_t hdl)
{
	struct kshm_info *info = (struct kshm_info *)hdl;
	size_t wt, rd, avail, index;

	if (!info) {
		return 0;
	}

	index = info->desc->index;
	wt = info->desc->wt[index];
	rd = info->desc->rd[index];

	if (wt >= rd) {
		avail = wt - rd;
	} else {
		avail = max(info->cfg.size - rd, wt);
	}

	return avail;
}

size_t kshm_get_free_size(kshm_handle_t hdl)
{
	struct kshm_info *info = (struct kshm_info *)hdl;
	size_t tail, head, rd, wt, index;
	
	if (!info) {
		return KSHM_FAIL;
	}

	index = info->desc->index;
	wt = info->desc->wt[index];
	rd = info->desc->rd[index];

	if (wt >= rd) {
		tail = info->cfg.size - wt;
		head = rd;
	} else {
		tail = rd - wt;
		head = 0;
	}

	return max(head, tail) - 1;
}

size_t kshm_get_total_size(kshm_handle_t hdl)
{
	struct kshm_info *info = (struct kshm_info *)hdl;

	if (!info) {
		return KSHM_FAIL;
	}

	return info->cfg.size - 1;
}
