#ifndef __UAPI_HCGE_H_H
#define __UAPI_HCGE_H_H

#include <hcuapi/iocbase.h>

#define HCGE_REQUEST_IRQ		_IO (HCGE_IOCBASE, 0x01)
#define HCGE_FREE_IRQ			_IO (HCGE_IOCBASE, 0x02)
#define HCGE_RESET			_IO (HCGE_IOCBASE, 0x03)
#define HCGE_SYNC_TIMEOUT		_IO (HCGE_IOCBASE, 0x04)
#define HCGE_GET_CMDQ_BUFINFO		_IO (HCGE_IOCBASE, 0x05)
#define HCGE_SET_CLOCK			_IO (HCGE_IOCBASE, 0x06)
#define HCGE_GET_GE_REGISTER		_IO (HCGE_IOCBASE, 0x07)

typedef enum hcge_clock {
	HCGE_CLOCK_198MHZ = 0,
	HCGE_CLOCK_148MHZ,
	HCGE_CLOCK_225MHZ,
	HCGE_CLOCK_238MHZ,
} hcge_clock_e;

/*
 * command queue buffer
 * */
typedef struct cmdq_buf_info {
	uint32_t addr;
	uint32_t size;
} cmdq_buf_info_t;

/*
 * command queue buffer map to userspace, mapped buffer header is blow structure,
 * located on the buffer header.
 */
typedef struct cmdq_node_ctx {
	//depict command queue node buffer boundary
	uint32_t phy_addr_min;
	uint32_t phy_addr_max;
	uint32_t is_finish;
	uint32_t serial_num;
	uint32_t is_reset;
	uint32_t point_wrap_addr;
	
	//next node data copy to this address
	uint32_t phy_addr_start;
	//aligned to 8bytes
	uint32_t clut_tbl[256]  __attribute__ ((aligned (8)));
} cmdq_node_ctx_t;

#endif
