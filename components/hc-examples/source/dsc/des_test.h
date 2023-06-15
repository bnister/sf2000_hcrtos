/*
 * Copyright (C) 2019 X-Sail Technology Co,. LTD.
 *
 * Authors:  X-Sail
 *
 * This source is confidential and is  X-Sail's proprietary information.
 * This source is subject to  X-Sail License Agreement, and shall not be
 * disclosed to unauthorized individual.
 *
 * THIS CODE AND INFORMATION ARE PROVIDED "AS IS" WITHOUT WARRANTY
 * OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED
 * TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
 * PARTICULAR PURPOSE.
 */
#ifndef __DES_HEADER_H__
#define __DES_HEADER_H__
static uint8_t des_test_iv[8]=
{
	0xf0,0x01,0x02,0x03,0xe4,0x05,0x06,0xf7
};

/* 64 bits key */
static uint8_t des_test_64key[8]=
{
    0x18,0x74,0x5d,0x49,0xd3,0x5e,0x4a,0xd5
};

/* 128 bits key*/
static uint8_t tdes_test_128key[16]=
{
	0x81,0x47,0xd5,0x94,0x3d,0xe5,0xa4,0x5d,
    0x48,0x84,0x72,0x24,0x44,0xc7,0x0e,0x21
};

/* 192 bits key */
static uint8_t tdes_test_192key[24]=
{
	0x81,0x47,0xd5,0x94,0x3d,0xe5,0xa4,0x5d,
    0x48,0x84,0x72,0x24,0x44,0xc7,0x0e,0x21,
    0x12,0x21,0x13,0x31,0x14,0x41,0x15,0x51
};

/* input data */
static uint8_t des_test_pure_data_in[34]=
{
	0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
    0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,
	0x11,0x22,0x33,0x44,0x04,0x05,0x06,0x07,
	0x08,0x09,0x0a,0xbb,0xcc,0xdd,0xee,0xff,
	0x12,0xcd
};

/* des ecb mode , resuide is clear*/
static uint8_t des_ecb_64key_clear_en_result[34] = 
{
	0x33,0x94,0xc3,0x8c,0x5c,0x11,0x43,0xda,
    0x2c,0xde,0x36,0x54,0x61,0x07,0x84,0x51,
    0x01,0x30,0x4e,0x60,0xb6,0xe4,0xc0,0x8c,
    0xb4,0x45,0x89,0x96,0x88,0xbe,0x47,0xb0,
    0x12,0xcd
};

static uint8_t des_ecb_64key_clear_de_result[34] = 
{
	0xee,0x9d,0x6f,0xc5,0x1c,0x52,0x4a,0xc1,
    0xa4,0x8c,0xce,0x21,0xef,0x4c,0x84,0x8e,
    0x55,0xdd,0xfb,0x4f,0x20,0xbc,0x3e,0xa5,
    0xac,0xb8,0xc6,0xc5,0x72,0x51,0x89,0x38,
    0x12,0xcd
};

static uint8_t tdes_ecb_128key_clear_en_result[34] = 
{
	0xa3,0x3f,0xf6,0xcb,0x48,0xd8,0x12,0xe2,
    0x75,0xc3,0x6c,0x8c,0x2f,0x0b,0xe6,0x69,
    0xf4,0xdb,0x96,0xe4,0xf3,0x57,0x05,0x66,
    0x34,0xca,0xcb,0x7d,0xd2,0xcf,0xe6,0x2d,
    0x12,0xcd
};

static uint8_t tdes_ecb_128key_clear_de_result[34] = 
{
	0x94,0x13,0x05,0xc7,0xca,0x17,0x1a,0xdc,
    0xc5,0x20,0x7b,0x89,0x10,0x24,0x20,0xe6,
    0xae,0x1c,0xe7,0xfb,0x2c,0xfe,0x52,0xa0,
    0x4c,0xc1,0xb5,0xa5,0xee,0x6a,0x53,0xc3,
    0x12,0xcd
};

static uint8_t tdes_ecb_192key_clear_en_result[34] = 
{
	0x69,0x11,0x59,0xf4,0x78,0xd9,0x90,0xe6,
    0x73,0x1b,0xba,0x19,0xcf,0xe2,0x55,0xdf,
    0xe9,0x92,0xe3,0x2d,0xd7,0xf9,0x8f,0x92,
    0x64,0xd0,0xad,0x77,0x0b,0x81,0xc4,0x38,
    0x12,0xcd
};

static uint8_t tdes_ecb_192key_clear_de_result[34] = 
{
	0xfa,0xbe,0x3a,0x59,0x4f,0xb8,0x77,0xf6,
    0xf9,0xb0,0xe3,0x6c,0x5d,0x00,0x0a,0x0a,
    0x8f,0xdc,0x0d,0x68,0xa3,0x3c,0x29,0x3a,
    0xba,0x7a,0xbf,0x95,0xbd,0x2e,0x08,0x62,
    0x12,0xcd
};


/* des cbc mode test case, resuide is clear  */
static uint8_t des_cbc_64key_clear_en_result[34] = 
{
	0x2a,0x67,0x3a,0xf5,0xd1,0x92,0x05,0x2f,
    0xf6,0x9b,0x30,0x0c,0xd7,0xb9,0x22,0xb6,
    0x8b,0x60,0x6a,0x06,0x78,0xf3,0xec,0x7d,
    0xbb,0x27,0xf2,0xec,0xfa,0xac,0x62,0x7c,
    0x12,0xcd
};

static uint8_t des_cbc_64key_clear_de_result[34] = 
{
	0x1e,0x9c,0x6d,0xc6,0xf8,0x57,0x4c,0x36,
    0xa4,0x8d,0xcc,0x22,0xeb,0x49,0x82,0x89,
    0x5d,0xd4,0xf1,0x44,0x2c,0xb1,0x30,0xaa,
    0xbd,0x9a,0xf5,0x81,0x76,0x54,0x8f,0x3f,
    0x12,0xcd
};

static uint8_t tdes_cbc_128key_clear_en_result[34] = 
{
	0xde,0x7f,0x6e,0xe6,0x30,0xba,0xea,0x95,
    0xcf,0xce,0x88,0x98,0xf0,0xe6,0xd6,0x00,
    0x7b,0xe5,0xb7,0xe2,0xd5,0x05,0x8e,0xdd,
    0xc1,0xb7,0x77,0x4c,0xa8,0x7d,0x95,0xa2,
    0x12,0xcd
};

static uint8_t tdes_cbc_128key_clear_de_result[34] = 
{
	0x64,0x12,0x07,0xc4,0x2e,0x12,0x1c,0x2b,
    0xc5,0x21,0x79,0x8a,0x14,0x21,0x26,0xe1,
    0xa6,0x15,0xed,0xf0,0x20,0xf3,0x5c,0xaf,
    0x5d,0xe3,0x86,0xe1,0xea,0x6f,0x55,0xc4,
    0x12,0xcd
};

static uint8_t tdes_cbc_192key_clear_en_result[34] = 
{
	0xa1,0x85,0x59,0xd2,0x65,0x10,0x42,0x9f,
    0xbf,0x61,0x09,0x6e,0xc5,0x6f,0x7e,0x1e,
    0x9b,0x62,0x84,0xc1,0x60,0xed,0xbe,0xa5,
    0xe2,0x82,0x07,0xb4,0x5d,0x5b,0xe0,0xb6,
    0x12,0xcd
};

static uint8_t tdes_cbc_192key_clear_de_result[34] = 
{
	0x0a,0xbf,0x38,0x5a,0xab,0xbd,0x71,0x01,
    0xf9,0xb1,0xe1,0x6f,0x59,0x05,0x0c,0x0d,
    0x87,0xd5,0x07,0x63,0xaf,0x31,0x27,0x35,
    0xab,0x58,0x8c,0xd1,0xb9,0x2b,0x0e,0x65,
    0x12,0xcd
};

/* des cbc mode test case, resuide is resuide is ANSI SCT 52  */
static uint8_t des_cbc_64key_atsc_en_result[34] = 
{
	0x2a,0x67,0x3a,0xf5,0xd1,0x92,0x05,0x2f,
    0xf6,0x9b,0x30,0x0c,0xd7,0xb9,0x22,0xb6,
    0x8b,0x60,0x6a,0x06,0x78,0xf3,0xec,0x7d,
    0xbb,0x27,0xf2,0xec,0xfa,0xac,0x62,0x7c,
    0x5b,0x92
};

static uint8_t des_cbc_64key_atsc_de_result[34] = 
{
	0x1e,0x9c,0x6d,0xc6,0xf8,0x57,0x4c,0x36,
    0xa4,0x8d,0xcc,0x22,0xeb,0x49,0x82,0x89,
    0x5d,0xd4,0xf1,0x44,0x2c,0xb1,0x30,0xaa,
    0xbd,0x9a,0xf5,0x81,0x76,0x54,0x8f,0x3f,
    0xa6,0x88
};

static uint8_t tdes_cbc_128key_atsc_en_result[34] = 
{
	0xde,0x7f,0x6e,0xe6,0x30,0xba,0xea,0x95,
    0xcf,0xce,0x88,0x98,0xf0,0xe6,0xd6,0x00,
    0x7b,0xe5,0xb7,0xe2,0xd5,0x05,0x8e,0xdd,
    0xc1,0xb7,0x77,0x4c,0xa8,0x7d,0x95,0xa2,
    0x8d,0xcd
};

static uint8_t tdes_cbc_128key_atsc_de_result[34] = 
{
	0x64,0x12,0x07,0xc4,0x2e,0x12,0x1c,0x2b,
    0xc5,0x21,0x79,0x8a,0x14,0x21,0x26,0xe1,
    0xa6,0x15,0xed,0xf0,0x20,0xf3,0x5c,0xaf,
    0x5d,0xe3,0x86,0xe1,0xea,0x6f,0x55,0xc4,
    0x26,0x07
};

static uint8_t tdes_cbc_192key_atsc_en_result[34] = 
{
	0xa1,0x85,0x59,0xd2,0x65,0x10,0x42,0x9f,
    0xbf,0x61,0x09,0x6e,0xc5,0x6f,0x7e,0x1e,
    0x9b,0x62,0x84,0xc1,0x60,0xed,0xbe,0xa5,
    0xe2,0x82,0x07,0xb4,0x5d,0x5b,0xe0,0xb6,
    0x03,0x96
};

static uint8_t tdes_cbc_192key_atsc_de_result[34] = 
{
	0x0a,0xbf,0x38,0x5a,0xab,0xbd,0x71,0x01,
    0xf9,0xb1,0xe1,0x6f,0x59,0x05,0x0c,0x0d,
    0x87,0xd5,0x07,0x63,0xaf,0x31,0x27,0x35,
    0xab,0x58,0x8c,0xd1,0xb9,0x2b,0x0e,0x65,
    0x76,0x1d
};

/* des cbc mode test case, resuide is cipher stealing  */
static uint8_t des_cbc_64key_cts_en_result[34] = 
{
	0x2a,0x67,0x3a,0xf5,0xd1,0x92,0x05,0x2f,
    0xf6,0x9b,0x30,0x0c,0xd7,0xb9,0x22,0xb6,
    0x8b,0x60,0x6a,0x06,0x78,0xf3,0xec,0x7d,
    0x15,0xfb,0xd3,0x05,0xf5,0xdb,0x76,0x73,
    0xbb,0x27
};

static uint8_t des_cbc_64key_cts_de_result[34] = 
{
	0x1e,0x9c,0x6d,0xc6,0xf8,0x57,0x4c,0x36,
    0xa4,0x8d,0xcc,0x22,0xeb,0x49,0x82,0x89,
    0x5d,0xd4,0xf1,0x44,0x2c,0xb1,0x30,0xaa,
    0xf9,0x96,0x2b,0x7e,0x9e,0xa0,0x68,0x2a,
    0xbe,0x75
};

static uint8_t tdes_cbc_128key_cts_en_result[34] = 
{
	0xde,0x7f,0x6e,0xe6,0x30,0xba,0xea,0x95,
    0xcf,0xce,0x88,0x98,0xf0,0xe6,0xd6,0x00,
    0x7b,0xe5,0xb7,0xe2,0xd5,0x05,0x8e,0xdd,
    0x21,0xc0,0x3a,0xe5,0x94,0x3b,0x89,0x3b,
    0xc1,0xb7
};

static uint8_t tdes_cbc_128key_cts_de_result[34] = 
{
	0x64,0x12,0x07,0xc4,0x2e,0x12,0x1c,0x2b,
    0xc5,0x21,0x79,0x8a,0x14,0x21,0x26,0xe1,
    0xa6,0x15,0xed,0xf0,0x20,0xf3,0x5c,0xaf,
    0x36,0xf3,0x60,0x4d,0xc8,0x09,0x34,0xf4,
    0x5e,0x0c
};

static uint8_t tdes_cbc_192key_cts_en_result[34] = 
{
	0xa1,0x85,0x59,0xd2,0x65,0x10,0x42,0x9f,
    0xbf,0x61,0x09,0x6e,0xc5,0x6f,0x7e,0x1e,
    0x9b,0x62,0x84,0xc1,0x60,0xed,0xbe,0xa5,
    0x9b,0x0e,0x75,0x9d,0x8f,0x68,0xcc,0xeb,
    0xe2,0x82
};

static uint8_t tdes_cbc_192key_cts_de_result[34] = 
{
	0x0a,0xbf,0x38,0x5a,0xab,0xbd,0x71,0x01,
    0xf9,0xb1,0xe1,0x6f,0x59,0x05,0x0c,0x0d,
    0x87,0xd5,0x07,0x63,0xaf,0x31,0x27,0x35,
    0xa8,0x81,0x1a,0xe7,0x0b,0xc2,0x45,0x19,
    0xa8,0xb7
};

/* des cfb mode test case, resuide is clear */
static uint8_t des_cfb_64key_clear_en_result[34] = 
{
	0x30,0xcc,0xe2,0xfc,0xaa,0x2c,0x69,0x01,
    0x66,0x17,0x6e,0xfb,0xaf,0xfc,0x50,0x19,
    0xfb,0x7c,0xb6,0xa1,0xc6,0xfa,0x3e,0x6d,
    0xf1,0xa1,0x3a,0xd6,0x3e,0x3c,0xde,0x65,
    0x12,0xcd
};

static uint8_t des_cfb_64key_clear_de_result[34] = 
{
	0x30,0xcc,0xe2,0xfc,0xaa,0x2c,0x69,0x01,
    0x3b,0x9d,0xc9,0x87,0x50,0x1c,0x4d,0xd5,
    0x3d,0xfc,0x05,0x10,0x65,0x02,0x82,0x56,
    0x09,0x39,0x44,0xdb,0x7a,0x39,0x2e,0x73,
    0x12,0xcd
};

static uint8_t tdes_cfb_128key_clear_en_result[34] = 
{
	0x8a,0x6c,0xd2,0x20,0x37,0x7a,0x45,0x52,
    0x16,0xb0,0xd9,0xf5,0xa2,0x98,0x98,0x89,
    0xd0,0xdd,0x9f,0x13,0x34,0xba,0x29,0xc2,
    0x2b,0xce,0x97,0x20,0xcf,0xfa,0x73,0x92,
    0x12,0xcd
};

static uint8_t tdes_cfb_128key_clear_de_result[34] = 
{
	0x8a,0x6c,0xd2,0x20,0x37,0x7a,0x45,0x52,
    0xab,0x36,0xfc,0xc0,0x44,0xd5,0x1c,0xed,
    0x64,0xe1,0x5f,0xc8,0x2b,0x0e,0xe0,0x6e,
    0xfc,0xd2,0x9c,0x5f,0x3f,0x8a,0xeb,0x99,
    0x12,0xcd
};

static uint8_t tdes_cfb_192key_clear_en_result[34] = 
{
	0x03,0x11,0x52,0xb6,0x1b,0x10,0xb6,0xf6,
    0x53,0x6a,0x1f,0xdd,0x20,0x70,0x5e,0x60,
    0xf5,0x0f,0xa5,0xb9,0xf7,0xb5,0xb6,0xea,
    0x5c,0x9e,0x1c,0xe2,0x7e,0x72,0x05,0xb9,
    0x12,0xcd
};

static uint8_t tdes_cfb_192key_clear_de_result[34] = 
{
	0x03,0x11,0x52,0xb6,0x1b,0x10,0xb6,0xf6,
    0x61,0x18,0x53,0xff,0x74,0xd4,0x9e,0xe9,
    0x62,0x39,0x89,0x5d,0xcb,0xe7,0x53,0xd8,
    0xe1,0x9b,0xe9,0x96,0x1b,0x24,0x61,0x6d,
    0x12,0xcd
};

/* des ofb mode test case, resuide is clear */
static uint8_t des_ofb_64key_clear_en_result[34] = 
{
	0x30,0xcc,0xe2,0xfc,0xaa,0x2c,0x69,0x01,
    0xa9,0x2d,0x63,0x4f,0xad,0x2d,0x5e,0xa1,
    0xcc,0x76,0xa4,0x11,0x1a,0x3b,0x49,0x06,
    0x79,0xa2,0x9c,0xa6,0x18,0xd3,0x45,0x0a,
    0x12,0xcd
};

static uint8_t des_ofb_64key_clear_de_result[34] = 
{
	0x30,0xcc,0xe2,0xfc,0xaa,0x2c,0x69,0x01,
    0xa9,0x2d,0x63,0x4f,0xad,0x2d,0x5e,0xa1,
    0xcc,0x76,0xa4,0x11,0x1a,0x3b,0x49,0x06,
    0x79,0xa2,0x9c,0xa6,0x18,0xd3,0x45,0x0a,
    0x12,0xcd
};

static uint8_t tdes_ofb_128key_clear_en_result[34] = 
{
	0x8a,0x6c,0xd2,0x20,0x37,0x7a,0x45,0x52,
    0x6d,0xdd,0x06,0x9c,0xc4,0x4c,0x5d,0x1f,
    0x05,0x72,0xeb,0x50,0x98,0x83,0x06,0x00,
    0x24,0x71,0x8b,0x30,0x61,0x82,0xfc,0x30,
    0x12,0xcd
};

static uint8_t tdes_ofb_128key_clear_de_result[34] = 
{
	0x8a,0x6c,0xd2,0x20,0x37,0x7a,0x45,0x52,
    0x6d,0xdd,0x06,0x9c,0xc4,0x4c,0x5d,0x1f,
    0x05,0x72,0xeb,0x50,0x98,0x83,0x06,0x00,
    0x24,0x71,0x8b,0x30,0x61,0x82,0xfc,0x30,
    0x12,0xcd
};

static uint8_t tdes_ofb_192key_clear_en_result[34] = 
{
	0x03,0x11,0x52,0xb6,0x1b,0x10,0xb6,0xf6,
    0x6e,0xe3,0x4f,0xb3,0x3e,0xf3,0x6d,0xf0,
    0x5d,0xa8,0x5e,0x21,0xc3,0x19,0xf3,0x99,
    0x67,0x64,0xb1,0x76,0x4e,0x84,0x77,0x25,
    0x12,0xcd
};

static uint8_t tdes_ofb_192key_clear_de_result[34] = 
{
	0x03,0x11,0x52,0xb6,0x1b,0x10,0xb6,0xf6,
    0x6e,0xe3,0x4f,0xb3,0x3e,0xf3,0x6d,0xf0,
    0x5d,0xa8,0x5e,0x21,0xc3,0x19,0xf3,0x99,
    0x67,0x64,0xb1,0x76,0x4e,0x84,0x77,0x25,
    0x12,0xcd
};


struct _dsc_des_test_case{
	const char* testcase;
	struct dsc_algo_params params;
	struct dsc_buffer result;
	uint8_t *buffer;
	int size;
};

struct _dsc_des_test_case dsc_des_test_case[]={
	//DES OFB
	{
		.testcase = "DSC_DES_OFB_64KEY_CLEAR_EN",
		.params.algo_type = DSC_ALGO_DES,
		.params.crypto_mode = DSC_ENCRYPT,
		.params.chaining_mode = DSC_MODE_OFB,
		.params.residue_mode = DSC_RESIDUE_CLEAR,
		.params.key.buffer = (void*)des_test_64key,
		.params.key.size = sizeof(des_test_64key),
		.params.iv.buffer = (void*)des_test_iv,
		.params.iv.size = sizeof(des_test_iv),		
		.buffer = (void*)des_test_pure_data_in,
		.size = sizeof(des_test_pure_data_in),
		.result.buffer = (void*)des_ofb_64key_clear_en_result,
		.result.size = sizeof(des_ofb_64key_clear_en_result),
	},
	{
		.testcase = "DSC_DES_OFB_64KEY_CLEAR_DE",
		.params.algo_type = DSC_ALGO_DES,
		.params.crypto_mode = DSC_DECRYPT,
		.params.chaining_mode = DSC_MODE_OFB,
		.params.residue_mode = DSC_RESIDUE_CLEAR,
		.params.key.buffer = (void*)des_test_64key,
		.params.key.size = sizeof(des_test_64key),
		.params.iv.buffer = (void*)des_test_iv,
		.params.iv.size = sizeof(des_test_iv),		
		.buffer = (void*)des_test_pure_data_in,
		.size = sizeof(des_test_pure_data_in),
		.result.buffer = (void*)des_ofb_64key_clear_de_result,
		.result.size = sizeof(des_ofb_64key_clear_de_result),
	},
	
	{
		.testcase = "DSC_TDES_OFB_128KEY_CLEAR_EN",
		.params.algo_type = DSC_ALGO_TDES,
		.params.crypto_mode = DSC_ENCRYPT,
		.params.chaining_mode = DSC_MODE_OFB,
		.params.residue_mode = DSC_RESIDUE_CLEAR,
		.params.key.buffer = (void*)tdes_test_128key,
		.params.key.size = sizeof(tdes_test_128key),
		.params.iv.buffer = (void*)des_test_iv,
		.params.iv.size = sizeof(des_test_iv),		
		.buffer = (void*)des_test_pure_data_in,
		.size = sizeof(des_test_pure_data_in),
		.result.buffer = (void*)tdes_ofb_128key_clear_en_result,
		.result.size = sizeof(tdes_ofb_128key_clear_en_result),
	},
	{
		.testcase = "DSC_TDES_OFB_128KEY_CLEAR_DE",
		.params.algo_type = DSC_ALGO_TDES,
		.params.crypto_mode = DSC_DECRYPT,
		.params.chaining_mode = DSC_MODE_OFB,
		.params.residue_mode = DSC_RESIDUE_CLEAR,
		.params.key.buffer = (void*)tdes_test_128key,
		.params.key.size = sizeof(tdes_test_128key),
		.params.iv.buffer = (void*)des_test_iv,
		.params.iv.size = sizeof(des_test_iv),		
		.buffer = (void*)des_test_pure_data_in,
		.size = sizeof(des_test_pure_data_in),
		.result.buffer = (void*)tdes_ofb_128key_clear_de_result,
		.result.size = sizeof(tdes_ofb_128key_clear_de_result),
	},
	
	{
		.testcase = "DSC_TDES_OFB_192KEY_CLEAR_EN",
		.params.algo_type = DSC_ALGO_TDES,
		.params.crypto_mode = DSC_ENCRYPT,
		.params.chaining_mode = DSC_MODE_OFB,
		.params.residue_mode = DSC_RESIDUE_CLEAR,
		.params.key.buffer = (void*)tdes_test_192key,
		.params.key.size = sizeof(tdes_test_192key),
		.params.iv.buffer = (void*)des_test_iv,
		.params.iv.size = sizeof(des_test_iv),		
		.buffer = (void*)des_test_pure_data_in,
		.size = sizeof(des_test_pure_data_in),
		.result.buffer = (void*)tdes_ofb_192key_clear_en_result,
		.result.size = sizeof(tdes_ofb_192key_clear_en_result),
	},
	{
		.testcase = "DSC_TDES_OFB_192KEY_CLEAR_DE",
		.params.algo_type = DSC_ALGO_TDES,
		.params.crypto_mode = DSC_DECRYPT,
		.params.chaining_mode = DSC_MODE_OFB,
		.params.residue_mode = DSC_RESIDUE_CLEAR,
		.params.key.buffer = (void*)tdes_test_192key,
		.params.key.size = sizeof(tdes_test_192key),
		.params.iv.buffer = (void*)des_test_iv,
		.params.iv.size = sizeof(des_test_iv),		
		.buffer = (void*)des_test_pure_data_in,
		.size = sizeof(des_test_pure_data_in),
		.result.buffer = (void*)tdes_ofb_192key_clear_de_result,
		.result.size = sizeof(tdes_ofb_192key_clear_de_result),
	},
	//DES CFB
	{
		.testcase = "DSC_DES_CFB_64KEY_CLEAR_EN",
		.params.algo_type = DSC_ALGO_DES,
		.params.crypto_mode = DSC_ENCRYPT,
		.params.chaining_mode = DSC_MODE_CFB,
		.params.residue_mode = DSC_RESIDUE_CLEAR,
		.params.key.buffer = (void*)des_test_64key,
		.params.key.size = sizeof(des_test_64key),
		.params.iv.buffer = (void*)des_test_iv,
		.params.iv.size = sizeof(des_test_iv),		
		.buffer = (void*)des_test_pure_data_in,
		.size = sizeof(des_test_pure_data_in),
		.result.buffer = (void*)des_cfb_64key_clear_en_result,
		.result.size = sizeof(des_cfb_64key_clear_en_result),
	},
	{
		.testcase = "DSC_DES_CFB_64KEY_CLEAR_DE",
		.params.algo_type = DSC_ALGO_DES,
		.params.crypto_mode = DSC_DECRYPT,
		.params.chaining_mode = DSC_MODE_CFB,
		.params.residue_mode = DSC_RESIDUE_CLEAR,
		.params.key.buffer = (void*)des_test_64key,
		.params.key.size = sizeof(des_test_64key),
		.params.iv.buffer = (void*)des_test_iv,
		.params.iv.size = sizeof(des_test_iv),		
		.buffer = (void*)des_test_pure_data_in,
		.size = sizeof(des_test_pure_data_in),
		.result.buffer = (void*)des_cfb_64key_clear_de_result,
		.result.size = sizeof(des_cfb_64key_clear_de_result),
	},
	
	{
		.testcase = "DSC_TDES_CFB_128KEY_CLEAR_EN",
		.params.algo_type = DSC_ALGO_TDES,
		.params.crypto_mode = DSC_ENCRYPT,
		.params.chaining_mode = DSC_MODE_CFB,
		.params.residue_mode = DSC_RESIDUE_CLEAR,
		.params.key.buffer = (void*)tdes_test_128key,
		.params.key.size = sizeof(tdes_test_128key),
		.params.iv.buffer = (void*)des_test_iv,
		.params.iv.size = sizeof(des_test_iv),		
		.buffer = (void*)des_test_pure_data_in,
		.size = sizeof(des_test_pure_data_in),
		.result.buffer = (void*)tdes_cfb_128key_clear_en_result,
		.result.size = sizeof(tdes_cfb_128key_clear_en_result),
	},
	{
		.testcase = "DSC_TDES_CFB_128KEY_CLEAR_DE",
		.params.algo_type = DSC_ALGO_TDES,
		.params.crypto_mode = DSC_DECRYPT,
		.params.chaining_mode = DSC_MODE_CFB,
		.params.residue_mode = DSC_RESIDUE_CLEAR,
		.params.key.buffer = (void*)tdes_test_128key,
		.params.key.size = sizeof(tdes_test_128key),
		.params.iv.buffer = (void*)des_test_iv,
		.params.iv.size = sizeof(des_test_iv),		
		.buffer = (void*)des_test_pure_data_in,
		.size = sizeof(des_test_pure_data_in),
		.result.buffer = (void*)tdes_cfb_128key_clear_de_result,
		.result.size = sizeof(tdes_cfb_128key_clear_de_result),
	},
	
	{
		.testcase = "DSC_TDES_CFB_192KEY_CLEAR_EN",
		.params.algo_type = DSC_ALGO_TDES,
		.params.crypto_mode = DSC_ENCRYPT,
		.params.chaining_mode = DSC_MODE_CFB,
		.params.residue_mode = DSC_RESIDUE_CLEAR,
		.params.key.buffer = (void*)tdes_test_192key,
		.params.key.size = sizeof(tdes_test_192key),
		.params.iv.buffer = (void*)des_test_iv,
		.params.iv.size = sizeof(des_test_iv),		
		.buffer = (void*)des_test_pure_data_in,
		.size = sizeof(des_test_pure_data_in),
		.result.buffer = (void*)tdes_cfb_192key_clear_en_result,
		.result.size = sizeof(tdes_cfb_192key_clear_en_result),
	},
	{
		.testcase = "DSC_TDES_CFB_192KEY_CLEAR_DE",
		.params.algo_type = DSC_ALGO_TDES,
		.params.crypto_mode = DSC_DECRYPT,
		.params.chaining_mode = DSC_MODE_CFB,
		.params.residue_mode = DSC_RESIDUE_CLEAR,
		.params.key.buffer = (void*)tdes_test_192key,
		.params.key.size = sizeof(tdes_test_192key),
		.params.iv.buffer = (void*)des_test_iv,
		.params.iv.size = sizeof(des_test_iv),		
		.buffer = (void*)des_test_pure_data_in,
		.size = sizeof(des_test_pure_data_in),
		.result.buffer = (void*)tdes_cfb_192key_clear_de_result,
		.result.size = sizeof(tdes_cfb_192key_clear_de_result),
	},	
	//DES CBC CTS
	{
		.testcase = "DSC_DES_CBC_64KEY_CTS_EN",
		.params.algo_type = DSC_ALGO_DES,
		.params.crypto_mode = DSC_ENCRYPT,
		.params.chaining_mode = DSC_MODE_CBC,
		.params.residue_mode = DSC_RESIDUE_HW_CTS_CS2,
		.params.key.buffer = (void*)des_test_64key,
		.params.key.size = sizeof(des_test_64key),
		.params.iv.buffer = (void*)des_test_iv,
		.params.iv.size = sizeof(des_test_iv),		
		.buffer = (void*)des_test_pure_data_in,
		.size = sizeof(des_test_pure_data_in),
		.result.buffer = (void*)des_cbc_64key_cts_en_result,
		.result.size = sizeof(des_cbc_64key_cts_en_result),
	},
	{
		.testcase = "DSC_DES_CBC_64KEY_CTS_DE",
		.params.algo_type = DSC_ALGO_DES,
		.params.crypto_mode = DSC_DECRYPT,
		.params.chaining_mode = DSC_MODE_CBC,
		.params.residue_mode = DSC_RESIDUE_HW_CTS_CS2,
		.params.key.buffer = (void*)des_test_64key,
		.params.key.size = sizeof(des_test_64key),
		.params.iv.buffer = (void*)des_test_iv,
		.params.iv.size = sizeof(des_test_iv),		
		.buffer = (void*)des_test_pure_data_in,
		.size = sizeof(des_test_pure_data_in),
		.result.buffer = (void*)des_cbc_64key_cts_de_result,
		.result.size = sizeof(des_cbc_64key_cts_de_result),
	},
	
	{
		.testcase = "DSC_TDES_CBC_128KEY_CLEAR_EN",
		.params.algo_type = DSC_ALGO_TDES,
		.params.crypto_mode = DSC_ENCRYPT,
		.params.chaining_mode = DSC_MODE_CBC,
		.params.residue_mode = DSC_RESIDUE_HW_CTS_CS2,
		.params.key.buffer = (void*)tdes_test_128key,
		.params.key.size = sizeof(tdes_test_128key),
		.params.iv.buffer = (void*)des_test_iv,
		.params.iv.size = sizeof(des_test_iv),		
		.buffer = (void*)des_test_pure_data_in,
		.size = sizeof(des_test_pure_data_in),
		.result.buffer = (void*)tdes_cbc_128key_cts_en_result,
		.result.size = sizeof(tdes_cbc_128key_cts_en_result),
	},
	{
		.testcase = "DSC_TDES_CBC_128KEY_CLEAR_DE",
		.params.algo_type = DSC_ALGO_TDES,
		.params.crypto_mode = DSC_DECRYPT,
		.params.chaining_mode = DSC_MODE_CBC,
		.params.residue_mode = DSC_RESIDUE_HW_CTS_CS2,
		.params.key.buffer = (void*)tdes_test_128key,
		.params.key.size = sizeof(tdes_test_128key),
		.params.iv.buffer = (void*)des_test_iv,
		.params.iv.size = sizeof(des_test_iv),		
		.buffer = (void*)des_test_pure_data_in,
		.size = sizeof(des_test_pure_data_in),
		.result.buffer = (void*)tdes_cbc_128key_cts_de_result,
		.result.size = sizeof(tdes_cbc_128key_cts_de_result),
	},
	
	{
		.testcase = "DSC_TDES_CBC_192KEY_CTS_EN",
		.params.algo_type = DSC_ALGO_TDES,
		.params.crypto_mode = DSC_ENCRYPT,
		.params.chaining_mode = DSC_MODE_CBC,
		.params.residue_mode = DSC_RESIDUE_HW_CTS_CS2,
		.params.key.buffer = (void*)tdes_test_192key,
		.params.key.size = sizeof(tdes_test_192key),
		.params.iv.buffer = (void*)des_test_iv,
		.params.iv.size = sizeof(des_test_iv),		
		.buffer = (void*)des_test_pure_data_in,
		.size = sizeof(des_test_pure_data_in),
		.result.buffer = (void*)tdes_cbc_192key_cts_en_result,
		.result.size = sizeof(tdes_cbc_192key_cts_en_result),
	},
	{
		.testcase = "DSC_TDES_CBC_192KEY_CTS_DE",
		.params.algo_type = DSC_ALGO_TDES,
		.params.crypto_mode = DSC_DECRYPT,
		.params.chaining_mode = DSC_MODE_CBC,
		.params.residue_mode = DSC_RESIDUE_HW_CTS_CS2,
		.params.key.buffer = (void*)tdes_test_192key,
		.params.key.size = sizeof(tdes_test_192key),
		.params.iv.buffer = (void*)des_test_iv,
		.params.iv.size = sizeof(des_test_iv),		
		.buffer = (void*)des_test_pure_data_in,
		.size = sizeof(des_test_pure_data_in),
		.result.buffer = (void*)tdes_cbc_192key_cts_de_result,
		.result.size = sizeof(tdes_cbc_192key_cts_de_result),
	},
	//DES CBC ATSC
	{
		.testcase = "DSC_DES_CBC_64KEY_ATSC_EN",
		.params.algo_type = DSC_ALGO_DES,
		.params.crypto_mode = DSC_ENCRYPT,
		.params.chaining_mode = DSC_MODE_CBC,
		.params.residue_mode = DSC_RESIDUE_AS_ATSC,
		.params.key.buffer = (void*)des_test_64key,
		.params.key.size = sizeof(des_test_64key),
		.params.iv.buffer = (void*)des_test_iv,
		.params.iv.size = sizeof(des_test_iv),		
		.buffer = (void*)des_test_pure_data_in,
		.size = sizeof(des_test_pure_data_in),
		.result.buffer = (void*)des_cbc_64key_atsc_en_result,
		.result.size = sizeof(des_cbc_64key_atsc_en_result),
	},
	{
		.testcase = "DSC_DES_CBC_64KEY_ATSC_DE",
		.params.algo_type = DSC_ALGO_DES,
		.params.crypto_mode = DSC_DECRYPT,
		.params.chaining_mode = DSC_MODE_CBC,
		.params.residue_mode = DSC_RESIDUE_AS_ATSC,
		.params.key.buffer = (void*)des_test_64key,
		.params.key.size = sizeof(des_test_64key),
		.params.iv.buffer = (void*)des_test_iv,
		.params.iv.size = sizeof(des_test_iv),		
		.buffer = (void*)des_test_pure_data_in,
		.size = sizeof(des_test_pure_data_in),
		.result.buffer = (void*)des_cbc_64key_atsc_de_result,
		.result.size = sizeof(des_cbc_64key_atsc_de_result),
	},
	
	{
		.testcase = "DSC_TDES_CBC_128KEY_ATSC_EN",
		.params.algo_type = DSC_ALGO_TDES,
		.params.crypto_mode = DSC_ENCRYPT,
		.params.chaining_mode = DSC_MODE_CBC,
		.params.residue_mode = DSC_RESIDUE_AS_ATSC,
		.params.key.buffer = (void*)tdes_test_128key,
		.params.key.size = sizeof(tdes_test_128key),
		.params.iv.buffer = (void*)des_test_iv,
		.params.iv.size = sizeof(des_test_iv),		
		.buffer = (void*)des_test_pure_data_in,
		.size = sizeof(des_test_pure_data_in),
		.result.buffer = (void*)tdes_cbc_128key_atsc_en_result,
		.result.size = sizeof(tdes_cbc_128key_atsc_en_result),
	},
	{
		.testcase = "DSC_TDES_CBC_128KEY_ATSC_DE",
		.params.algo_type = DSC_ALGO_TDES,
		.params.crypto_mode = DSC_DECRYPT,
		.params.chaining_mode = DSC_MODE_CBC,
		.params.residue_mode = DSC_RESIDUE_AS_ATSC,
		.params.key.buffer = (void*)tdes_test_128key,
		.params.key.size = sizeof(tdes_test_128key),
		.params.iv.buffer = (void*)des_test_iv,
		.params.iv.size = sizeof(des_test_iv),		
		.buffer = (void*)des_test_pure_data_in,
		.size = sizeof(des_test_pure_data_in),
		.result.buffer = (void*)tdes_cbc_128key_atsc_de_result,
		.result.size = sizeof(tdes_cbc_128key_atsc_de_result),
	},
	
	{
		.testcase = "DSC_TDES_CBC_192KEY_ATSC_EN",
		.params.algo_type = DSC_ALGO_TDES,
		.params.crypto_mode = DSC_ENCRYPT,
		.params.chaining_mode = DSC_MODE_CBC,
		.params.residue_mode = DSC_RESIDUE_AS_ATSC,
		.params.key.buffer = (void*)tdes_test_192key,
		.params.key.size = sizeof(tdes_test_192key),
		.params.iv.buffer = (void*)des_test_iv,
		.params.iv.size = sizeof(des_test_iv),		
		.buffer = (void*)des_test_pure_data_in,
		.size = sizeof(des_test_pure_data_in),
		.result.buffer = (void*)tdes_cbc_192key_atsc_en_result,
		.result.size = sizeof(tdes_cbc_192key_atsc_en_result),
	},
	{
		.testcase = "DSC_TDES_CBC_192KEY_ATSC_DE",
		.params.algo_type = DSC_ALGO_TDES,
		.params.crypto_mode = DSC_DECRYPT,
		.params.chaining_mode = DSC_MODE_CBC,
		.params.residue_mode = DSC_RESIDUE_AS_ATSC,
		.params.key.buffer = (void*)tdes_test_192key,
		.params.key.size = sizeof(tdes_test_192key),
		.params.iv.buffer = (void*)des_test_iv,
		.params.iv.size = sizeof(des_test_iv),		
		.buffer = (void*)des_test_pure_data_in,
		.size = sizeof(des_test_pure_data_in),
		.result.buffer = (void*)tdes_cbc_192key_atsc_de_result,
		.result.size = sizeof(tdes_cbc_192key_atsc_de_result),
	},
	//DES CBC CLEAR
	{
		.testcase = "DSC_DES_CBC_64KEY_CLEAR_EN",
		.params.algo_type = DSC_ALGO_DES,
		.params.crypto_mode = DSC_ENCRYPT,
		.params.chaining_mode = DSC_MODE_CBC,
		.params.residue_mode = DSC_RESIDUE_CLEAR,
		.params.key.buffer = (void*)des_test_64key,
		.params.key.size = sizeof(des_test_64key),
		.params.iv.buffer = (void*)des_test_iv,
		.params.iv.size = sizeof(des_test_iv),		
		.buffer = (void*)des_test_pure_data_in,
		.size = sizeof(des_test_pure_data_in),
		.result.buffer = (void*)des_cbc_64key_clear_en_result,
		.result.size = sizeof(des_cbc_64key_clear_en_result),
	},
	{
		.testcase = "DSC_DES_CBC_64KEY_CLEAR_DE",
		.params.algo_type = DSC_ALGO_DES,
		.params.crypto_mode = DSC_DECRYPT,
		.params.chaining_mode = DSC_MODE_CBC,
		.params.residue_mode = DSC_RESIDUE_CLEAR,
		.params.key.buffer = (void*)des_test_64key,
		.params.key.size = sizeof(des_test_64key),
		.params.iv.buffer = (void*)des_test_iv,
		.params.iv.size = sizeof(des_test_iv),		
		.buffer = (void*)des_test_pure_data_in,
		.size = sizeof(des_test_pure_data_in),
		.result.buffer = (void*)des_cbc_64key_clear_de_result,
		.result.size = sizeof(des_cbc_64key_clear_de_result),
	},
	
	{
		.testcase = "DSC_TDES_CBC_128KEY_CLEAR_EN",
		.params.algo_type = DSC_ALGO_TDES,
		.params.crypto_mode = DSC_ENCRYPT,
		.params.chaining_mode = DSC_MODE_CBC,
		.params.residue_mode = DSC_RESIDUE_CLEAR,
		.params.key.buffer = (void*)tdes_test_128key,
		.params.key.size = sizeof(tdes_test_128key),
		.params.iv.buffer = (void*)aes_test_iv,
		.params.iv.size = sizeof(aes_test_iv),		
		.buffer = (void*)des_test_pure_data_in,
		.size = sizeof(des_test_pure_data_in),
		.result.buffer = (void*)tdes_cbc_128key_clear_en_result,
		.result.size = sizeof(tdes_cbc_128key_clear_en_result),
	},
	{
		.testcase = "DSC_TDES_CBC_128KEY_CLEAR_DE",
		.params.algo_type = DSC_ALGO_TDES,
		.params.crypto_mode = DSC_DECRYPT,
		.params.chaining_mode = DSC_MODE_CBC,
		.params.residue_mode = DSC_RESIDUE_CLEAR,
		.params.key.buffer = (void*)tdes_test_128key,
		.params.key.size = sizeof(tdes_test_128key),
		.params.iv.buffer = (void*)aes_test_iv,
		.params.iv.size = sizeof(aes_test_iv),		
		.buffer = (void*)des_test_pure_data_in,
		.size = sizeof(des_test_pure_data_in),
		.result.buffer = (void*)tdes_cbc_128key_clear_de_result,
		.result.size = sizeof(tdes_cbc_128key_clear_de_result),
	},
	
	{
		.testcase = "DSC_TDES_CBC_192KEY_CLEAR_EN",
		.params.algo_type = DSC_ALGO_TDES,
		.params.crypto_mode = DSC_ENCRYPT,
		.params.chaining_mode = DSC_MODE_CBC,
		.params.residue_mode = DSC_RESIDUE_CLEAR,
		.params.key.buffer = (void*)tdes_test_192key,
		.params.key.size = sizeof(tdes_test_192key),
		.params.iv.buffer = (void*)des_test_iv,
		.params.iv.size = sizeof(des_test_iv),		
		.buffer = (void*)des_test_pure_data_in,
		.size = sizeof(des_test_pure_data_in),
		.result.buffer = (void*)tdes_cbc_192key_clear_en_result,
		.result.size = sizeof(tdes_cbc_192key_clear_en_result),
	},
	{
		.testcase = "DSC_TDES_CBC_192KEY_CLEAR_DE",
		.params.algo_type = DSC_ALGO_TDES,
		.params.crypto_mode = DSC_DECRYPT,
		.params.chaining_mode = DSC_MODE_CBC,
		.params.residue_mode = DSC_RESIDUE_CLEAR,
		.params.key.buffer = (void*)tdes_test_192key,
		.params.key.size = sizeof(tdes_test_192key),
		.params.iv.buffer = (void*)des_test_iv,
		.params.iv.size = sizeof(des_test_iv),		
		.buffer = (void*)des_test_pure_data_in,
		.size = sizeof(des_test_pure_data_in),
		.result.buffer = (void*)tdes_cbc_192key_clear_de_result,
		.result.size = sizeof(tdes_cbc_192key_clear_de_result),
	},
	//DES ECB
	{
		.testcase = "DSC_DES_ECB_64KEY_CLEAR_EN",
		.params.algo_type = DSC_ALGO_DES,
		.params.crypto_mode = DSC_ENCRYPT,
		.params.chaining_mode = DSC_MODE_ECB,
		.params.residue_mode = DSC_RESIDUE_CLEAR,
		.params.key.buffer = (void*)des_test_64key,
		.params.key.size = sizeof(des_test_64key),
		.params.iv.buffer = (void*)des_test_iv,
		.params.iv.size = sizeof(des_test_iv),		
		.buffer = (void*)des_test_pure_data_in,
		.size = sizeof(des_test_pure_data_in),
		.result.buffer = (void*)des_ecb_64key_clear_en_result,
		.result.size = sizeof(des_ecb_64key_clear_en_result),
	},
	{
		.testcase = "DSC_DES_ECB_64KEY_CLEAR_DE",
		.params.algo_type = DSC_ALGO_DES,
		.params.crypto_mode = DSC_DECRYPT,
		.params.chaining_mode = DSC_MODE_ECB,
		.params.residue_mode = DSC_RESIDUE_CLEAR,
		.params.key.buffer = (void*)des_test_64key,
		.params.key.size = sizeof(des_test_64key),
		.params.iv.buffer = (void*)des_test_iv,
		.params.iv.size = sizeof(des_test_iv),		
		.buffer = (void*)des_test_pure_data_in,
		.size = sizeof(des_test_pure_data_in),
		.result.buffer = (void*)des_ecb_64key_clear_de_result,
		.result.size = sizeof(des_ecb_64key_clear_de_result),
	},
	
	{
		.testcase = "DSC_TDES_ECB_128KEY_CLEAR_DE",
		.params.algo_type = DSC_ALGO_TDES,
		.params.crypto_mode = DSC_ENCRYPT,
		.params.chaining_mode = DSC_MODE_ECB,
		.params.residue_mode = DSC_RESIDUE_CLEAR,
		.params.key.buffer = (void*)tdes_test_128key,
		.params.key.size = sizeof(tdes_test_128key),
		.params.iv.buffer = (void*)des_test_iv,
		.params.iv.size = sizeof(des_test_iv),		
		.buffer = (void*)des_test_pure_data_in,
		.size = sizeof(des_test_pure_data_in),
		.result.buffer = (void*)tdes_ecb_128key_clear_en_result,
		.result.size = sizeof(tdes_ecb_128key_clear_en_result),
	},
	{
		.testcase = "DSC_TDES_ECB_128KEY_CLEAR_DE",
		.params.algo_type = DSC_ALGO_TDES,
		.params.crypto_mode = DSC_DECRYPT,
		.params.chaining_mode = DSC_MODE_ECB,
		.params.residue_mode = DSC_RESIDUE_CLEAR,
		.params.key.buffer = (void*)tdes_test_128key,
		.params.key.size = sizeof(tdes_test_128key),
		.params.iv.buffer = (void*)des_test_iv,
		.params.iv.size = sizeof(des_test_iv),		
		.buffer = (void*)des_test_pure_data_in,
		.size = sizeof(des_test_pure_data_in),
		.result.buffer = (void*)tdes_ecb_128key_clear_de_result,
		.result.size = sizeof(tdes_ecb_128key_clear_de_result),
	},
	
	{
		.testcase = "DSC_TDES_ECB_192KEY_CLEAR_DE",
		.params.algo_type = DSC_ALGO_TDES,
		.params.crypto_mode = DSC_ENCRYPT,
		.params.chaining_mode = DSC_MODE_ECB,
		.params.residue_mode = DSC_RESIDUE_CLEAR,
		.params.key.buffer = (void*)tdes_test_192key,
		.params.key.size = sizeof(tdes_test_192key),
		.params.iv.buffer = (void*)des_test_iv,
		.params.iv.size = sizeof(des_test_iv),		
		.buffer = (void*)des_test_pure_data_in,
		.size = sizeof(des_test_pure_data_in),
		.result.buffer = (void*)tdes_ecb_192key_clear_en_result,
		.result.size = sizeof(tdes_ecb_192key_clear_en_result),
	},
	{
		.testcase = "DSC_TDES_ECB_192KEY_CLEAR_DE",
		.params.algo_type = DSC_ALGO_TDES,
		.params.crypto_mode = DSC_DECRYPT,
		.params.chaining_mode = DSC_MODE_ECB,
		.params.residue_mode = DSC_RESIDUE_CLEAR,
		.params.key.buffer = (void*)tdes_test_192key,
		.params.key.size = sizeof(tdes_test_192key),
		.params.iv.buffer = (void*)des_test_iv,
		.params.iv.size = sizeof(des_test_iv),		
		.buffer = (void*)des_test_pure_data_in,
		.size = sizeof(des_test_pure_data_in),
		.result.buffer = (void*)tdes_ecb_192key_clear_de_result,
		.result.size = sizeof(tdes_ecb_192key_clear_de_result),
	},

}; 

#define DSC_DES_TESTCASE_NUM (sizeof(dsc_des_test_case)/sizeof(dsc_des_test_case[0]))
#endif /* __WAVE_HEADER_H__ */

