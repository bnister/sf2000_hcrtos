#ifndef _HCUAPI_JPEG_ENC_H_
#define _HCUAPI_JPEG_ENC_H_

typedef struct jpeg_enc_quant {
	uint8_t dec_quant_y[64];
	uint8_t dec_quant_c[64];
	uint16_t enc_quant_y[64];
	uint16_t enc_quant_c[64];
} jpeg_enc_quant_t;

typedef enum JPEG_ENC_QUALITY_TYPE {
	JPEG_ENC_QUALITY_TYPE_NORMAL,
	JPEG_ENC_QUALITY_TYPE_LOW_BITRATE,
	JPEG_ENC_QUALITY_TYPE_HIGH_QUALITY,
	JPEG_ENC_QUALITY_TYPE_ULTRA_HIGH_QUALITY,
	JPEG_ENC_QUALITY_TYPE_USER_DEFINE,
	JPEG_ENC_QUALITY_TYPE_NUM,
} jpeg_enc_quality_type_e;

#endif
