#ifndef __HCUAPI_DSC_H__
#define __HCUAPI_DSC_H__

#include <hcuapi/iocbase.h>

#define DSC_DO_SHA		_IOWR(DSC_IOCBASE, 1, struct dsc_sha_params)
#define DSC_CONFIG		_IOWR(DSC_IOCBASE, 2, struct dsc_algo_params)
#define DSC_DO_ENCRYPT		_IOWR(DSC_IOCBASE, 3, struct dsc_crypt)
#define DSC_DO_DECRYPT		_IOWR(DSC_IOCBASE, 4, struct dsc_crypt)

typedef enum DSC_SHA_TYPE {
	DSC_SHA_TYPE_1		= 0,	//!< Indicates the hash type is SHA1, HASH length is 20 bytes.
	DSC_SHA_TYPE_224	= 1,	//!< Indicates the hash type is SHA224, HASH length is 28 bytes.
	DSC_SHA_TYPE_256	= 2,	//!< Indicates the hash type is SHA256, HASH length is 32 bytes.
	DSC_SHA_TYPE_384	= 3,	//!< Indicates the hash type is SHA384, HASH length is 48 bytes.
	DSC_SHA_TYPE_512	= 4	//!< Indicates the hash type is SHA512, HASH length is 64 bytes.
} dsc_sha_type_e;

typedef enum DSC_ALGO_TYPE {
	DSC_ALGO_AES	= 0,	//!< Indicates the algorithm is AES.
	DSC_ALGO_DES	= 1,	//!< Indicates the algorithm is DES.
	DSC_ALGO_TDES	= 2,	//!< Indicates the algorithm is TDES.
	DSC_ALGO_CSA1	= 3,	//!< Indicates the algorithm is CSA1 64 bits supports only TS payload data mode.
	DSC_ALGO_CSA2	= 4,	//!< Indicates the algorithm is CSA2.64 bits supports only TS payload data mode.
	DSC_ALGO_CSA3	= 5	//!< Indicates the algorithm is CSA3.128 bits support both raw and TS payload data mode.
} dsc_algo_type_e;

typedef enum DSC_CRYPTO_MODE {
	DSC_ENCRYPT	= (0<<8),	//!< Indicates the mode is encryption.
	DSC_DECRYPT	= (1<<8)	//!< Indicates the mode is decryption.
} dsc_crypto_mode_e;

typedef enum DSC_CHAINING_MODE {
	DSC_MODE_CBC	= (0<<4),	//!< Cipher-Block chaining.
	DSC_MODE_ECB	= (1<<4),	//!< Electronic codeboock, no IV.
	DSC_MODE_OFB	= (2<<4),	//!< Output Feedback mode.
	DSC_MODE_CFB	= (3<<4),	//!< Cipher Feedback mode.
	DSC_MODE_CTR	= (4<<4),	//!< Counter mode, 16 bytes IV contains initial counter.
	DSC_MODE_CTR8	= (5<<4)	//!< Counter mode, 8 bytes IV contains initial counter.
} dsc_chaining_mode_e;

typedef enum DSC_RESIDUE_MODE {
	DSC_RESIDUE_CLEAR	= (0<<12),
	DSC_RESIDUE_AS_ATSC	= (1<<12),
	DSC_RESIDUE_HW_CTS_CS2	= (2<<12),
	DSC_RESIDUE_CTR_HDL	= (3<<12),
	DSC_RESIDUE_CABLE_CARD	= (4<<12),
	DSC_RESIDUE_CTS_CS3	= (5<<12),
} dsc_residue_mode_e;

typedef struct dsc_buffer {
	void* buffer;
	uint32_t size;
} dsc_buffer_t;

typedef struct dsc_sha_params {
	dsc_sha_type_e type;
	struct dsc_buffer input;
	struct dsc_buffer output;
} dsc_sha_params_t;

typedef struct dsc_algo_params {
	dsc_algo_type_e algo_type;
	dsc_crypto_mode_e crypto_mode;
	dsc_chaining_mode_e chaining_mode;
	dsc_residue_mode_e residue_mode;
	struct dsc_buffer key;
	struct dsc_buffer iv;
} dsc_algo_params_t;

typedef struct dsc_crypt {
	void *input;
	void *output;
	uint32_t size;
} dsc_crypt_t;

#endif
