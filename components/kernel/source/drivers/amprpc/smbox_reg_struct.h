#ifndef __SMBOX_REG_STRUCT_H__
#define __SMBOX_REG_STRUCT_H__

#ifndef __KERNEL__
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#endif

typedef volatile struct smbox_reg {

	union {
		struct {
			/**
			bitpos: [[0]]
			Trig another cpu interrupt
			 */
			uint32_t int_trig:			1;
			uint32_t reserved1:			31;
		};
		uint32_t val;
	} mbox_int_trig;	/* REG_SMBOX_BASE + 0x0 */

	union {
		struct {
			/**
			bitpos: [[0]]
			Clear another cpu interrupt
			 */
			uint32_t int_clr:			1;
			uint32_t reserved1:			31;
		};
		uint32_t val;
	} mbox_int_clr;	/* REG_SMBOX_BASE + 0x4 */

	union {
		struct {
			/**
			bitpos: [[0]]
			Enable receive interrupt
			 */
			uint32_t int_rec_en:			1;
			/**
			bitpos: [[1]]
			Enable callback interrupt
			 */
			uint32_t int_cbk_en:			1;
			uint32_t reserved2:			6;
			/**
			bitpos: [[8]]
			Receive interrupt status
			 */
			uint32_t int_rec_sta:			1;
			uint32_t reserved9:			7;
			/**
			bitpos: [[16]]
			Callback interrupt status
			 */
			uint32_t int_cbk_sta:			1;
			uint32_t reserved17:			15;
		};
		uint32_t val;
	} mbox_int_enst;	/* REG_SMBOX_BASE + 0x8 */

	union {
		struct {
			/**
			bitpos: [[31:0]]
			data descriptor register0
			 */
			uint32_t data:			32;
		};
		uint32_t val;
	} mbox_data0;	/* REG_SMBOX_BASE + 0xc */

	union {
		struct {
			/**
			bitpos: [[31:0]]
			data descriptor register1
			 */
			uint32_t data:			32;
		};
		uint32_t val;
	} mbox_data1;	/* REG_SMBOX_BASE + 0x10 */

	union {
		struct {
			/**
			bitpos: [[31:0]]
			data descriptor register2
			 */
			uint32_t data:			32;
		};
		uint32_t val;
	} mbox_data2;	/* REG_SMBOX_BASE + 0x14 */

	union {
		struct {
			/**
			bitpos: [[31:0]]
			data descriptor register3
			 */
			uint32_t data:			32;
		};
		uint32_t val;
	} mbox_data3;	/* REG_SMBOX_BASE + 0x18 */

	union {
		struct {
			/**
			bitpos: [[31:0]]
			data descriptor register4
			 */
			uint32_t data:			32;
		};
		uint32_t val;
	} mbox_data4;	/* REG_SMBOX_BASE + 0x1c */

} smbox_reg_t;

#ifndef __KERNEL__
#ifdef __cplusplus
}
#endif
#endif

#endif  /* __SMBOX_REG_STRUCT_H__ */
