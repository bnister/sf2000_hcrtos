#ifndef _HCUAPI_KSHM_H_
#define _HCUAPI_KSHM_H_

#include <hcuapi/iocbase.h>

#define KSHM_OK				(0)
#define KSHM_FAIL			(-1)

#define KSHM_HDL_ACCESS			_IOR (KSHM_IOCBASE, 0, struct kshm_info)
#define KSHM_WRITE_HDL_ACCESS		KSHM_HDL_ACCESS
#define KSHM_READ_HDL_ACCESS		_IOR (KSHM_IOCBASE, 2, struct kshm_info)

#define KSHM_HDL_SET			_IOR (KSHM_IOCBASE, 1, struct kshm_info)
#define KSHM_WRITE_HDL_SET		KSHM_HDL_SET
#define KSHM_READ_HDL_SET		_IOR (KSHM_IOCBASE, 3, struct kshm_info)

typedef void * kshm_handle_t;

enum KSHM_TYPE {
	KSHM_TYPE_GENERIC,
	KSHM_TYPE_AUDIO,
	KSHM_TYPE_VIDEO,
};

enum KSHM_DATA_DIRECTION {
	KSHM_BIDIRECTIONAL = 0,
	KSHM_TO_DEVICE = 1,
	KSHM_FROM_DEVICE = 2,
	KSHM_NONE = 3,
};

struct kshm_desc {
	size_t rd[2];
	size_t wt[2];
	size_t reserved[2];
	size_t index;
	int32_t dma_xfer_id;
	size_t update_write_size;
	int wt_times;
	int rd_times;
};

struct kshm_cfg {
	void *base;
	size_t size;
	int8_t dma_ch;
	enum KSHM_DATA_DIRECTION data_direction;
};

struct kshm_info {
	struct kshm_desc *desc;
	struct kshm_cfg cfg;
	char opaque[0];
};

kshm_handle_t kshm_create(size_t size);

kshm_handle_t kshm_create_ext(size_t size, enum KSHM_DATA_DIRECTION data_direction);

int kshm_set_data_direction(kshm_handle_t hdl, enum KSHM_DATA_DIRECTION data_direction);

int kshm_destroy(kshm_handle_t hdl);

int kshm_reset(kshm_handle_t hdl);

void *kshm_request_read(kshm_handle_t hdl, size_t size);

int kshm_update_read(kshm_handle_t hdl, size_t size);

int kshm_update_reserved(kshm_handle_t hdl, size_t size);

int kshm_read(kshm_handle_t hdl, void *buf, size_t size);

void *kshm_request_write(kshm_handle_t hdl, size_t size);

int kshm_update_write(kshm_handle_t hdl, size_t size);

int kshm_write(kshm_handle_t hdl, void *buf, size_t size);

size_t kshm_get_valid_size(kshm_handle_t hdl);

size_t kshm_get_free_size(kshm_handle_t hdl);

size_t kshm_get_total_size(kshm_handle_t hdl);

#endif /* _HCUAPI_KSHM_H_ */
