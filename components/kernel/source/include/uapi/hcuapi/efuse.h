#ifndef _HCUAPI_EFUSE_H_
#define _HCUAPI_EFUSE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <hcuapi/iocbase.h>
#include <hcuapi/chipid.h>

/* ******************************* */
// hichip efuse  bit map
/*
| Zone | Partition | bits |      Usage      |            Owner            |
| :--: | :-------: | :--: | :-------------: | :-------------------------: |
|  0   |  [31:0]   |  32  |    Unique ID    |         Chip Vendor         |
|  1   |  [63:32]  |  32  |    Unique ID    |         Chip Vendor         |
|  2   |  [71:64]  |  8   | hichip reserve0 |         Chip Vendor         |
|      |  [79:72]  |  8   | hichip reserve1 |         Chip Vendor         |
|      |  [95:80]  |  16  | hichip reserve2 |         Chip Vendor         |
|  3   | [127:96]  |  32  | Application use |          Customer           |
|  4   | [159:128] |  32  | Application use |          Customer           |
|  5   | [191:160] |  32  | Application use |          Customer           |
|  6   | [223:192] |  32  | Application use |          Customer           |
|  7   | [247:224] |  24  | Application use |          Customer           |
|  NA  | [250:248] |  3   |  Write Portect  |    HQ: zone[2] ~ zone[0]    |
|  NA  | [255:251] |  5   |  Write Portect  | Customer: zone[7] ~ zone[3] |
*/

/*
!note: 

There are two ways to burn or read efuse bits, the write/read 
and ioctrl(CMD: EFUSE_DUMP and EFUSE_PROGRAM) ways respectively.

Examples of specific use can be found in file A.txt

*/

/* ******************************* */

struct __chip_vendor {
	uint32_t unique_id0 : 32;
	uint32_t unique_id1 : 32;
	uint8_t hichip_reserve0 : 8;
	uint8_t hichip_reserve1 : 8;
	uint16_t hichip_reserve2 : 16;
} __attribute__((packed));

struct __customer {
	uint32_t content0 : 32;
	uint32_t content1 : 32;
	uint32_t content2 : 32;
	uint32_t content3 : 32;
	uint32_t content4 : 24;
} __attribute__((packed));

struct __write_protect {
	/* write protect for Chip Vendor */
	uint8_t wp_zone0 : 1; // for bit[31:0]
	uint8_t wp_zone1 : 1; // for bit[63:32]

	/* write protect for Customer */
	uint8_t wp_zone2 : 1; // for bit[95:64]
	uint8_t wp_zone3 : 1; // for bit[127:96]
	uint8_t wp_zone4 : 1; // for bit[159:128]
	uint8_t wp_zone5 : 1; // for bit[191:160]
	uint8_t wp_zone6 : 1; // for bit[223:192]
	uint8_t wp_zone7 : 1; // for bit[247:224]
} __attribute__((packed));

/* 
 * hichip efuse bit map
 */
struct hc_efuse_bit_map {
	struct __chip_vendor chip_vendor;
	struct __customer customer;
	struct __write_protect wp;
} __attribute__((packed));

struct hc_efuse_bits_program {
	uint32_t bits_offset;    // bit offset
	uint32_t bits_length;    // program bits numbers
	uint32_t bits_mask;      // program bits mask
	uint32_t bits_value;     // program bits value
};

#define EFUSE_DUMP		_IOR(EFUSE_IOCBASE, 0, struct hc_efuse_bit_map)
#define EFUSE_PROGRAM		_IOW(EFUSE_IOCBASE, 1, struct hc_efuse_bit_map)
#define EFUSE_BITS_PROGRAM	_IOW(EFUSE_IOCBASE, 2, struct hc_efuse_bits_program)
#define EFUSE_GET_CHIPID	_IOR(EFUSE_IOCBASE, 3, enum HC_CHIPID)

#ifdef __cplusplus
}
#endif

#endif // ! _HCUAPI_EFUSE_H_
