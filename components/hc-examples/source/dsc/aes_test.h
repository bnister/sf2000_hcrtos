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
#ifndef __AES_HEADER_H__
#define __AES_HEADER_H__

static uint8_t aes_test_iv[16]=
{
	0xf0,0x01,0x02,0x03,0xe4,0x05,0x06,0xf7,
    0xf8,0xe9,0x0a,0x0b,0x0c,0x0d,0xfe,0x0f
};

/* 128 bits key*/
static uint8_t aes_test_128key[16]=
{
	0x81,0x47,0xd5,0x94,0x3d,0xe5,0xa4,0x5d,
    0x48,0x84,0x72,0x24,0x44,0xc7,0x0e,0x21
};

/* 192 bits key */
static uint8_t aes_test_192key[24]=
{
	0x81,0x47,0xd5,0x94,0x3d,0xe5,0xa4,0x5d,
    0x48,0x84,0x72,0x24,0x44,0xc7,0x0e,0x21,
    0x12,0x21,0x13,0x31,0x14,0x41,0x15,0x51
};

/* 256 bits key */
static uint8_t aes_test_256key[32]=
{
	0x81,0x47,0xd5,0x94,0x3d,0xe5,0xa4,0x5d,
    0x48,0x84,0x72,0x24,0x44,0xc7,0x0e,0x21,
    0x18,0x74,0x5d,0x49,0xd3,0x5e,0x4a,0xd5,
    0x84,0x48,0x27,0x42,0x44,0x7c,0xe0,0x12
};

/* input data */
static uint8_t aes_test_pure_data_in[34]=
{
	0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
    0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,
	0x11,0x22,0x33,0x44,0x04,0x05,0x06,0x07,
	0x08,0x09,0x0a,0xbb,0xcc,0xdd,0xee,0xff,
	0x12,0xcd
};

/* ecb mode */
static uint8_t aes_ecb_128key_en_result[34] = 
{
	0x73,0xc7,0xdd,0xf6,0x8d,0xee,0xb0,0x23,
    0x99,0x90,0x3f,0x69,0xa3,0xf8,0xf9,0x47,
	0x03,0xb0,0x95,0x66,0x26,0xad,0x0e,0xc3,
	0x6c,0x2b,0xea,0x9d,0x32,0xac,0x42,0xbc,
	0x12,0xcd
};
static uint8_t aes_ecb_128key_de_result[34] = 
{
	0xc9,0x0c,0x88,0xf2,0x04,0xe2,0x19,0xbb,
    0x27,0x6e,0x30,0xf7,0x42,0xa8,0xa7,0x67,
	0x97,0x24,0xd1,0x41,0x1c,0xf3,0xbf,0x8b,
	0xab,0xd6,0x35,0x40,0x70,0x3d,0x31,0xf0,
	0x12,0xcd
};
static uint8_t aes_ecb_192key_en_result[34] = 
{
    0xba,0xc4,0x7d,0x39,0x5c,0xb3,0x4c,0x93,
    0x15,0x9f,0x27,0xc7,0x47,0x0c,0xb4,0xf7,
    0xc0,0xb9,0x83,0x67,0x1f,0xe0,0x1e,0x8e,
    0x03,0xe3,0x81,0xc4,0x51,0x2d,0x6e,0x11,
    0x12,0xcd
};
static uint8_t aes_ecb_192key_de_result[34] = 
{
    0xbb,0x2a,0x57,0xee,0x90,0x7d,0xc3,0x9d,
    0xd8,0xa1,0x67,0x6b,0xe1,0x62,0xdf,0x94,
    0x34,0x96,0x9a,0x12,0x55,0x98,0x52,0x1d,
    0x66,0x76,0x74,0x34,0xb6,0x06,0x60,0xd8,
    0x12,0xcd
};

static uint8_t aes_ecb_256key_en_result[34] = 
{
    0x27,0x85,0x58,0x11,0xb6,0x05,0x8e,0x61,
    0xee,0x79,0x23,0x78,0xd8,0xe8,0xff,0x08,
    0x7c,0xe2,0xbf,0x03,0xcd,0x40,0x1b,0xee,
    0xb6,0x9b,0xea,0xa4,0x94,0x99,0xce,0x44,
    0x12,0xcd
};
static uint8_t aes_ecb_256key_de_result[34] = 
{
    0xa7,0x7d,0x01,0x89,0x5b,0xe0,0x32,0xd4,
    0xea,0xd8,0x18,0xd2,0x60,0x36,0xb1,0x8d,
    0x1b,0x4e,0x07,0x10,0x9f,0x0d,0x66,0x96,
    0xa9,0x2c,0xaa,0x36,0xd0,0x08,0x1a,0x73,
    0x12,0xcd
};

/* ctr mode*/
static uint8_t aes_ctr_128key_en_de_result[34] = 
{
	0xc0,0xbe,0x3f,0xdf,0xbd,0x16,0x97,0x4d,
    0x75,0x34,0x80,0xc3,0x29,0x1e,0xf4,0x83,
    0x78,0x62,0x70,0xd4,0xa5,0x7f,0x45,0x8a,
    0x66,0x35,0x09,0x53,0x99,0x54,0x22,0x36,
    0x50,0xf8
};

static uint8_t aes_ctr_192key_en_de_result[34] = 
{
    0x2c,0x59,0xf7,0x7a,0x7a,0x9c,0xa2,0x38,
    0xb7,0xe4,0xc6,0x0e,0x53,0xad,0xf1,0x94,
    0xc1,0xbc,0x74,0x11,0x56,0x3e,0xf9,0x65,
    0x95,0xcb,0xd3,0xe8,0xd5,0x19,0xfa,0xe3,
    0x22,0xb8
};

static uint8_t aes_ctr_256key_en_de_result[34] = 
{
    0x6b,0xe4,0xbf,0x77,0xa3,0x6d,0x11,0x81,
    0xb0,0x5d,0x02,0x38,0x82,0xf0,0xd4,0x80,
    0xc3,0xb8,0x23,0x27,0x88,0x60,0x87,0x5b,
    0x1b,0xc4,0x27,0xfc,0x90,0x76,0x7c,0x96,
    0xa2,0xc8
};

/* cbc mode*/
static uint8_t aes_cbc_128key_clear_en_result[34] = 
{
	0x26,0xce,0xf5,0x15,0x93,0xd3,0xac,0xff,
    0x80,0xb8,0xd7,0xb7,0xd9,0x1e,0xd6,0xfc,
    0x03,0x66,0x94,0xf7,0xcd,0xaf,0xb7,0xe5,
    0x9c,0x26,0xf8,0x37,0xce,0xbc,0xfa,0xf9,
    0x12,0xcd
};
static uint8_t aes_cbc_128key_clear_de_result[34] = 
{
	0x39,0x0d,0x8a,0xf1,0xe0,0xe7,0x1f,0x4c,
    0xdf,0x87,0x3a,0xfc,0x4e,0xa5,0x59,0x68,
    0x97,0x25,0xd3,0x42,0x18,0xf6,0xb9,0x8c,
    0xa3,0xdf,0x3f,0x4b,0x7c,0x30,0x3f,0xff,
    0x12,0xcd
};

static uint8_t aes_cbc_192key_clear_en_result[34] = 
{
    0x93,0xbd,0xbf,0xab,0x0d,0x87,0x8a,0x10,
    0x34,0x45,0xc5,0x32,0x29,0x1e,0xb5,0x4f,
    0xd5,0xda,0x00,0x4d,0xc9,0x20,0x80,0xe0,
    0x2c,0x1c,0x0b,0x99,0xdc,0x34,0xbb,0x82,
    0x12,0xcd
};

static uint8_t aes_cbc_192key_clear_de_result[34] = 
{
    0x4b,0x2b,0x55,0xed,0x74,0x78,0xc5,0x6a,
    0x20,0x48,0x6d,0x60,0xed,0x6f,0x21,0x9b,
    0x34,0x97,0x98,0x11,0x51,0x9d,0x54,0x1a,
    0x6e,0x7f,0x7e,0x3f,0xba,0x0b,0x6e,0xd7,
    0x12,0xcd
};

static uint8_t aes_cbc_256key_clear_en_result[34] = 
{
	0x96,0x86,0x83,0xec,0xb6,0x6b,0x31,0x7c,
    0x04,0x92,0x0e,0x29,0x2f,0x3a,0x0c,0x6f,
    0x1f,0xd1,0xe2,0xea,0x14,0x65,0x71,0x89,
    0x95,0x8e,0x17,0x1a,0xe6,0x43,0x5e,0x38,
    0x12,0xcd
};

static uint8_t aes_cbc_256key_clear_de_result[34] = 
{
	0x57,0x7c,0x03,0x8a,0xbf,0xe5,0x34,0x23,
    0x12,0x31,0x12,0xd9,0x6c,0x3b,0x4f,0x82,
    0x1b,0x4f,0x05,0x13,0x9b,0x08,0x60,0x91,
    0xa1,0x25,0xa0,0x3d,0xdc,0x05,0x14,0x7c,
    0x12,0xcd
};
/* aes cbc resuide is ansi scte52 */
static uint8_t aes_cbc_128key_atsc_en_result[34] = 
{
	0x26,0xce,0xf5,0x15,0x93,0xd3,0xac,0xff,
    0x80,0xb8,0xd7,0xb7,0xd9,0x1e,0xd6,0xfc,
    0x03,0x66,0x94,0xf7,0xcd,0xaf,0xb7,0xe5,
    0x9c,0x26,0xf8,0x37,0xce,0xbc,0xfa,0xf9,
    0x8f,0xc6
};
static uint8_t aes_cbc_128key_atsc_de_result[34] = 
{
	0x39,0x0d,0x8a,0xf1,0xe0,0xe7,0x1f,0x4c,
    0xdf,0x87,0x3a,0xfc,0x4e,0xa5,0x59,0x68,
    0x97,0x25,0xd3,0x42,0x18,0xf6,0xb9,0x8c,
    0xa3,0xdf,0x3f,0x4b,0x7c,0x30,0x3f,0xff,
    0x11,0x7d
};

static uint8_t aes_cbc_192key_atsc_en_result[34] = 
{
    0x93,0xbd,0xbf,0xab,0x0d,0x87,0x8a,0x10,
    0x34,0x45,0xc5,0x32,0x29,0x1e,0xb5,0x4f,
    0xd5,0xda,0x00,0x4d,0xc9,0x20,0x80,0xe0,
    0x2c,0x1c,0x0b,0x99,0xdc,0x34,0xbb,0x82,
    0x5c,0x57
};

static uint8_t aes_cbc_192key_atsc_de_result[34] = 
{
    0x4b,0x2b,0x55,0xed,0x74,0x78,0xc5,0x6a,
    0x20,0x48,0x6d,0x60,0xed,0x6f,0x21,0x9b,
    0x34,0x97,0x98,0x11,0x51,0x9d,0x54,0x1a,
    0x6e,0x7f,0x7e,0x3f,0xba,0x0b,0x6e,0xd7,
    0xd2,0x74
};

static uint8_t aes_cbc_256key_atsc_en_result[34] = 
{
	0x96,0x86,0x83,0xec,0xb6,0x6b,0x31,0x7c,
    0x04,0x92,0x0e,0x29,0x2f,0x3a,0x0c,0x6f,
    0x1f,0xd1,0xe2,0xea,0x14,0x65,0x71,0x89,
    0x95,0x8e,0x17,0x1a,0xe6,0x43,0x5e,0x38,
    0x6f,0xef
};

static uint8_t aes_cbc_256key_atsc_de_result[34] = 
{
	0x57,0x7c,0x03,0x8a,0xbf,0xe5,0x34,0x23,
    0x12,0x31,0x12,0xd9,0x6c,0x3b,0x4f,0x82,
    0x1b,0x4f,0x05,0x13,0x9b,0x08,0x60,0x91,
    0xa1,0x25,0xa0,0x3d,0xdc,0x05,0x14,0x7c,
    0x6e,0x2f
};

/* aes cbc resuide is cipher stealing */
static uint8_t aes_cbc_128key_cts_en_result[34] = 
{
	0x26,0xce,0xf5,0x15,0x93,0xd3,0xac,0xff,
    0x80,0xb8,0xd7,0xb7,0xd9,0x1e,0xd6,0xfc,
    0x99,0xd3,0x0e,0xa8,0xc8,0xaa,0x9e,0x00,
    0x76,0x29,0x0b,0x5b,0xde,0x15,0x2c,0xd6,
    0x03,0x66
};
static uint8_t aes_cbc_128key_cts_de_result[34] = 
{
	0x39,0x0d,0x8a,0xf1,0xe0,0xe7,0x1f,0x4c,
    0xdf,0x87,0x3a,0xfc,0x4e,0xa5,0x59,0x68,
    0x24,0xab,0xcc,0x03,0x01,0xeb,0x37,0x59,
    0xee,0x9b,0xb6,0x2d,0x6e,0xee,0x70,0x04,
    0x85,0xe9
};

static uint8_t aes_cbc_192key_cts_en_result[34] = 
{
    0x93,0xbd,0xbf,0xab,0x0d,0x87,0x8a,0x10,
    0x34,0x45,0xc5,0x32,0x29,0x1e,0xb5,0x4f,
    0xe2,0xb4,0x29,0x11,0xe0,0x79,0x57,0xf5,
    0x17,0xdf,0x16,0x16,0x90,0x28,0x70,0xcd,
    0xd5,0xda
};

static uint8_t aes_cbc_192key_cts_de_result[34] = 
{
    0x4b,0x2b,0x55,0xed,0x74,0x78,0xc5,0x6a,
    0x20,0x48,0x6d,0x60,0xed,0x6f,0x21,0x9b,
    0x80,0x8f,0x78,0xb0,0x57,0x2d,0x97,0x67,
    0xc9,0xc8,0x9b,0xc3,0xca,0x82,0x46,0x71,
    0x26,0x5b
};

static uint8_t aes_cbc_256key_cts_en_result[34] = 
{
	0x96,0x86,0x83,0xec,0xb6,0x6b,0x31,0x7c,
    0x04,0x92,0x0e,0x29,0x2f,0x3a,0x0c,0x6f,
    0x60,0x0a,0xcc,0x93,0x09,0xe0,0xb2,0x1a,
    0x36,0xcd,0x00,0x0a,0x13,0xb3,0xa3,0xef,
    0x1f,0xd1
};

static uint8_t aes_cbc_256key_cts_de_result[34] = 
{
	0x57,0x7c,0x03,0x8a,0xbf,0xe5,0x34,0x23,
    0x12,0x31,0x12,0xd9,0x6c,0x3b,0x4f,0x82,
    0xb3,0x65,0xbf,0xab,0xf4,0xd6,0xf2,0x48,
    0x45,0x6f,0xe3,0xc1,0x7e,0xfe,0x57,0x72,
    0x09,0x83
};

/* aes cfb resuide is clear */
static uint8_t aes_cfb_128key_clear_en_result[34] = 
{
	0xc0,0xbe,0x3f,0xdf,0xbd,0x16,0x97,0x4d,
    0x75,0x34,0x80,0xc3,0x29,0x1e,0xf4,0x83,
    0x23,0x0f,0x5d,0x0d,0x2d,0x3d,0x2a,0x59,
    0x0e,0x24,0x6c,0xcb,0x2f,0xa2,0xf4,0x3d,
    0x12,0xcd
};
static uint8_t aes_cfb_128key_clear_de_result[34] = 
{
	0xc0,0xbe,0x3f,0xdf,0xbd,0x16,0x97,0x4d,
    0x75,0x34,0x80,0xc3,0x29,0x1e,0xf4,0x83,
    0x62,0xe5,0xee,0xb2,0x89,0xeb,0xb6,0x24,
    0x91,0x99,0x35,0xd2,0x6f,0x25,0x17,0xb8,
    0x12,0xcd
};

static uint8_t aes_cfb_192key_clear_en_result[34] = 
{
    0x2c,0x59,0xf7,0x7a,0x7a,0x9c,0xa2,0x38,
    0xb7,0xe4,0xc6,0x0e,0x53,0xad,0xf1,0x94,
    0xd2,0x82,0xb0,0x77,0xc1,0x72,0x60,0x8d,
    0xf5,0xc2,0xd7,0x41,0x43,0x8a,0x41,0xa3,
    0x12,0xcd
};

static uint8_t aes_cfb_192key_clear_de_result[34] = 
{
    0x2c,0x59,0xf7,0x7a,0x7a,0x9c,0xa2,0x38,
    0xb7,0xe4,0xc6,0x0e,0x53,0xad,0xf1,0x94,
    0xab,0xe6,0x4e,0x7d,0x58,0xb6,0x4a,0x94,
    0x1d,0x96,0x2d,0x7c,0x8b,0xd1,0x5a,0x08,
    0x12,0xcd
};

static uint8_t aes_cfb_256key_clear_en_result[34] = 
{
	0x6b,0xe4,0xbf,0x77,0xa3,0x6d,0x11,0x81,
    0xb0,0x5d,0x02,0x38,0x82,0xf0,0xd4,0x80,
    0xf0,0xea,0xf1,0x54,0x0d,0x59,0x38,0xc3,
    0x18,0x1a,0x3b,0xf2,0xf1,0x80,0xea,0xa8,
    0x12,0xcd
};

static uint8_t aes_cfb_256key_clear_de_result[34] = 
{
	0x6b,0xe4,0xbf,0x77,0xa3,0x6d,0x11,0x81,
    0xb0,0x5d,0x02,0x38,0x82,0xf0,0xd4,0x80,
    0x36,0xa7,0x6b,0x55,0xb2,0x00,0x88,0x66,
    0xe6,0x70,0x29,0xc3,0x14,0x35,0x11,0xf7,
    0x12,0xcd
};


/* aes ofb resuide is clear */
static uint8_t aes_ofb_128key_clear_en_result[34] = 
{
	0xc0,0xbe,0x3f,0xdf,0xbd,0x16,0x97,0x4d,
    0x75,0x34,0x80,0xc3,0x29,0x1e,0xf4,0x83,
    0x48,0xe9,0x2f,0xef,0x68,0x40,0x38,0x89,
    0xbc,0x80,0xfc,0x74,0x30,0x62,0x5f,0x23,
    0x12,0xcd
};
static uint8_t aes_ofb_128key_clear_de_result[34] = 
{
	0xc0,0xbe,0x3f,0xdf,0xbd,0x16,0x97,0x4d,
    0x75,0x34,0x80,0xc3,0x29,0x1e,0xf4,0x83,
    0x48,0xe9,0x2f,0xef,0x68,0x40,0x38,0x89,
    0xbc,0x80,0xfc,0x74,0x30,0x62,0x5f,0x23,
    0x12,0xcd
};

static uint8_t aes_ofb_192key_clear_en_result[34] = 
{
    0x2c,0x59,0xf7,0x7a,0x7a,0x9c,0xa2,0x38,
    0xb7,0xe4,0xc6,0x0e,0x53,0xad,0xf1,0x94,
    0xd7,0x39,0xcc,0xe2,0x68,0xfc,0xdd,0x5f,
    0xe3,0x38,0x5c,0x25,0xa6,0x3a,0xa5,0x2c,
    0x12,0xcd
};

static uint8_t aes_ofb_192key_clear_de_result[34] = 
{
    0x2c,0x59,0xf7,0x7a,0x7a,0x9c,0xa2,0x38,
    0xb7,0xe4,0xc6,0x0e,0x53,0xad,0xf1,0x94,
    0xd7,0x39,0xcc,0xe2,0x68,0xfc,0xdd,0x5f,
    0xe3,0x38,0x5c,0x25,0xa6,0x3a,0xa5,0x2c,
    0x12,0xcd
};

static uint8_t aes_ofb_256key_clear_en_result[34] = 
{
	0x6b,0xe4,0xbf,0x77,0xa3,0x6d,0x11,0x81,
    0xb0,0x5d,0x02,0x38,0x82,0xf0,0xd4,0x80,
    0x8b,0x74,0x67,0x51,0x23,0xce,0x36,0x6f,
    0x4e,0x31,0xe6,0xce,0xbb,0xb2,0x75,0xe7,
    0x12,0xcd
};

static uint8_t aes_ofb_256key_clear_de_result[34] = 
{
	0x6b,0xe4,0xbf,0x77,0xa3,0x6d,0x11,0x81,
    0xb0,0x5d,0x02,0x38,0x82,0xf0,0xd4,0x80,
    0x8b,0x74,0x67,0x51,0x23,0xce,0x36,0x6f,
    0x4e,0x31,0xe6,0xce,0xbb,0xb2,0x75,0xe7,
    0x12,0xcd
};

struct _dsc_aes_test_case{
	const char* testcase;
	struct dsc_algo_params params;
	struct dsc_buffer result;
	uint8_t *buffer;
	int size;
};

struct _dsc_aes_test_case dsc_aes_test_case[]={
	//AES OFB
	{
		.testcase = "DSC_AES_OFB_128KEY_CLEAR_EN",
		.params.algo_type = DSC_ALGO_AES,
		.params.crypto_mode = DSC_ENCRYPT,
		.params.chaining_mode = DSC_MODE_OFB,
		.params.residue_mode = DSC_RESIDUE_CLEAR,
		.params.key.buffer = (void*)aes_test_128key,
		.params.key.size = sizeof(aes_test_128key),
		.params.iv.buffer = (void*)aes_test_iv,
		.params.iv.size = sizeof(aes_test_iv),		
		.buffer = (void*)aes_test_pure_data_in,
		.size = sizeof(aes_test_pure_data_in),
		.result.buffer = (void*)aes_ofb_128key_clear_en_result,
		.result.size = sizeof(aes_ofb_128key_clear_en_result),
	},
	{
		.testcase = "DSC_AES_OFB_128KEY_CLEAR_DE",
		.params.algo_type = DSC_ALGO_AES,
		.params.crypto_mode = DSC_DECRYPT,
		.params.chaining_mode = DSC_MODE_OFB,
		.params.residue_mode = DSC_RESIDUE_CLEAR,
		.params.key.buffer = (void*)aes_test_128key,
		.params.key.size = sizeof(aes_test_128key),
		.params.iv.buffer = (void*)aes_test_iv,
		.params.iv.size = sizeof(aes_test_iv),		
		.buffer = (void*)aes_test_pure_data_in,
		.size = sizeof(aes_test_pure_data_in),
		.result.buffer = (void*)aes_ofb_128key_clear_de_result,
		.result.size = sizeof(aes_ofb_128key_clear_de_result),
	},
	{
		.testcase = "DSC_AES_OFB_192KEY_CLEAR_EN",
		.params.algo_type = DSC_ALGO_AES,
		.params.crypto_mode = DSC_ENCRYPT,
		.params.chaining_mode = DSC_MODE_OFB,
		.params.residue_mode = DSC_RESIDUE_CLEAR,
		.params.key.buffer = (void*)aes_test_192key,
		.params.key.size = sizeof(aes_test_192key),
		.params.iv.buffer = (void*)aes_test_iv,
		.params.iv.size = sizeof(aes_test_iv),		
		.buffer = (void*)aes_test_pure_data_in,
		.size = sizeof(aes_test_pure_data_in),
		.result.buffer = (void*)aes_ofb_192key_clear_en_result,
		.result.size = sizeof(aes_ofb_192key_clear_en_result),
	},
	{
		.testcase = "DSC_AES_OFB_192KEY_CLEAR_DE",
		.params.algo_type = DSC_ALGO_AES,
		.params.crypto_mode = DSC_DECRYPT,
		.params.chaining_mode = DSC_MODE_OFB,
		.params.residue_mode = DSC_RESIDUE_CLEAR,
		.params.key.buffer = (void*)aes_test_192key,
		.params.key.size = sizeof(aes_test_192key),
		.params.iv.buffer = (void*)aes_test_iv,
		.params.iv.size = sizeof(aes_test_iv),		
		.buffer = (void*)aes_test_pure_data_in,
		.size = sizeof(aes_test_pure_data_in),
		.result.buffer = (void*)aes_ofb_192key_clear_de_result,
		.result.size = sizeof(aes_ofb_192key_clear_de_result),
	},
	
	{
		.testcase = "DSC_AES_OFB_256KEY_CLEAR_EN",
		.params.algo_type = DSC_ALGO_AES,
		.params.crypto_mode = DSC_ENCRYPT,
		.params.chaining_mode = DSC_MODE_OFB,
		.params.residue_mode = DSC_RESIDUE_CLEAR,
		.params.key.buffer = (void*)aes_test_256key,
		.params.key.size = sizeof(aes_test_256key),
		.params.iv.buffer = (void*)aes_test_iv,
		.params.iv.size = sizeof(aes_test_iv),		
		.buffer = (void*)aes_test_pure_data_in,
		.size = sizeof(aes_test_pure_data_in),
		.result.buffer = (void*)aes_ofb_256key_clear_en_result,
		.result.size = sizeof(aes_ofb_256key_clear_en_result),
	},
	{
		.testcase = "DSC_AES_OFB_256KEY_CLEAR_DE",
		.params.algo_type = DSC_ALGO_AES,
		.params.crypto_mode = DSC_DECRYPT,
		.params.chaining_mode = DSC_MODE_OFB,
		.params.residue_mode = DSC_RESIDUE_CLEAR,
		.params.key.buffer = (void*)aes_test_256key,
		.params.key.size = sizeof(aes_test_256key),
		.params.iv.buffer = (void*)aes_test_iv,
		.params.iv.size = sizeof(aes_test_iv),		
		.buffer = (void*)aes_test_pure_data_in,
		.size = sizeof(aes_test_pure_data_in),
		.result.buffer = (void*)aes_ofb_256key_clear_de_result,
		.result.size = sizeof(aes_ofb_256key_clear_de_result),
	},
	//AES CFB
	{
		.testcase = "DSC_AES_CFB_128KEY_CLEAR_EN",
		.params.algo_type = DSC_ALGO_AES,
		.params.crypto_mode = DSC_ENCRYPT,
		.params.chaining_mode = DSC_MODE_CFB,
		.params.residue_mode = DSC_RESIDUE_CLEAR,
		.params.key.buffer = (void*)aes_test_128key,
		.params.key.size = sizeof(aes_test_128key),
		.params.iv.buffer = (void*)aes_test_iv,
		.params.iv.size = sizeof(aes_test_iv),		
		.buffer = (void*)aes_test_pure_data_in,
		.size = sizeof(aes_test_pure_data_in),
		.result.buffer = (void*)aes_cfb_128key_clear_en_result,
		.result.size = sizeof(aes_cfb_128key_clear_en_result),
	},
	{
		.testcase = "DSC_AES_CFB_128KEY_CLEAR_DE",
		.params.algo_type = DSC_ALGO_AES,
		.params.crypto_mode = DSC_DECRYPT,
		.params.chaining_mode = DSC_MODE_CFB,
		.params.residue_mode = DSC_RESIDUE_CLEAR,
		.params.key.buffer = (void*)aes_test_128key,
		.params.key.size = sizeof(aes_test_128key),
		.params.iv.buffer = (void*)aes_test_iv,
		.params.iv.size = sizeof(aes_test_iv),		
		.buffer = (void*)aes_test_pure_data_in,
		.size = sizeof(aes_test_pure_data_in),
		.result.buffer = (void*)aes_cfb_128key_clear_de_result,
		.result.size = sizeof(aes_cfb_128key_clear_de_result),
	},
	{
		.testcase = "DSC_AES_CFB_192KEY_CLEAR_EN",
		.params.algo_type = DSC_ALGO_AES,
		.params.crypto_mode = DSC_ENCRYPT,
		.params.chaining_mode = DSC_MODE_CFB,
		.params.residue_mode = DSC_RESIDUE_CLEAR,
		.params.key.buffer = (void*)aes_test_192key,
		.params.key.size = sizeof(aes_test_192key),
		.params.iv.buffer = (void*)aes_test_iv,
		.params.iv.size = sizeof(aes_test_iv),		
		.buffer = (void*)aes_test_pure_data_in,
		.size = sizeof(aes_test_pure_data_in),
		.result.buffer = (void*)aes_cfb_192key_clear_en_result,
		.result.size = sizeof(aes_cfb_192key_clear_en_result),
	},
	{
		.testcase = "DSC_AES_CFB_192KEY_CLEAR_DE",
		.params.algo_type = DSC_ALGO_AES,
		.params.crypto_mode = DSC_DECRYPT,
		.params.chaining_mode = DSC_MODE_CFB,
		.params.residue_mode = DSC_RESIDUE_CLEAR,
		.params.key.buffer = (void*)aes_test_192key,
		.params.key.size = sizeof(aes_test_192key),
		.params.iv.buffer = (void*)aes_test_iv,
		.params.iv.size = sizeof(aes_test_iv),		
		.buffer = (void*)aes_test_pure_data_in,
		.size = sizeof(aes_test_pure_data_in),
		.result.buffer = (void*)aes_cfb_192key_clear_de_result,
		.result.size = sizeof(aes_cfb_192key_clear_de_result),
	},
	
	{
		.testcase = "DSC_AES_CFB_256KEY_CLEAR_EN",
		.params.algo_type = DSC_ALGO_AES,
		.params.crypto_mode = DSC_ENCRYPT,
		.params.chaining_mode = DSC_MODE_CFB,
		.params.residue_mode = DSC_RESIDUE_CLEAR,
		.params.key.buffer = (void*)aes_test_256key,
		.params.key.size = sizeof(aes_test_256key),
		.params.iv.buffer = (void*)aes_test_iv,
		.params.iv.size = sizeof(aes_test_iv),		
		.buffer = (void*)aes_test_pure_data_in,
		.size = sizeof(aes_test_pure_data_in),
		.result.buffer = (void*)aes_cfb_256key_clear_en_result,
		.result.size = sizeof(aes_cfb_256key_clear_en_result),
	},
	{
		.testcase = "DSC_AES_CFB_256KEY_CLEAR_DE",
		.params.algo_type = DSC_ALGO_AES,
		.params.crypto_mode = DSC_DECRYPT,
		.params.chaining_mode = DSC_MODE_CFB,
		.params.residue_mode = DSC_RESIDUE_CLEAR,
		.params.key.buffer = (void*)aes_test_256key,
		.params.key.size = sizeof(aes_test_256key),
		.params.iv.buffer = (void*)aes_test_iv,
		.params.iv.size = sizeof(aes_test_iv),		
		.buffer = (void*)aes_test_pure_data_in,
		.size = sizeof(aes_test_pure_data_in),
		.result.buffer = (void*)aes_cfb_256key_clear_de_result,
		.result.size = sizeof(aes_cfb_256key_clear_de_result),
	},	
	//AES CBC CLEAR
	{
		.testcase = "DSC_AES_CBC_128KEY_CLEAR_EN",
		.params.algo_type = DSC_ALGO_AES,
		.params.crypto_mode = DSC_ENCRYPT,
		.params.chaining_mode = DSC_MODE_CBC,
		.params.residue_mode = DSC_RESIDUE_CLEAR,
		.params.key.buffer = (void*)aes_test_128key,
		.params.key.size = sizeof(aes_test_128key),
		.params.iv.buffer = (void*)aes_test_iv,
		.params.iv.size = sizeof(aes_test_iv),		
		.buffer = (void*)aes_test_pure_data_in,
		.size = sizeof(aes_test_pure_data_in),
		.result.buffer = (void*)aes_cbc_128key_clear_en_result,
		.result.size = sizeof(aes_cbc_128key_clear_en_result),
	},
	{
		.testcase = "DSC_AES_CBC_128KEY_CLEAR_DE",
		.params.algo_type = DSC_ALGO_AES,
		.params.crypto_mode = DSC_DECRYPT,
		.params.chaining_mode = DSC_MODE_CBC,
		.params.residue_mode = DSC_RESIDUE_CLEAR,
		.params.key.buffer = (void*)aes_test_128key,
		.params.key.size = sizeof(aes_test_128key),
		.params.iv.buffer = (void*)aes_test_iv,
		.params.iv.size = sizeof(aes_test_iv),		
		.buffer = (void*)aes_test_pure_data_in,
		.size = sizeof(aes_test_pure_data_in),
		.result.buffer = (void*)aes_cbc_128key_clear_de_result,
		.result.size = sizeof(aes_cbc_128key_clear_de_result),
	},
	{
		.testcase = "DSC_AES_CBC_192KEY_CLEAR_EN",
		.params.algo_type = DSC_ALGO_AES,
		.params.crypto_mode = DSC_ENCRYPT,
		.params.chaining_mode = DSC_MODE_CBC,
		.params.residue_mode = DSC_RESIDUE_CLEAR,
		.params.key.buffer = (void*)aes_test_192key,
		.params.key.size = sizeof(aes_test_192key),
		.params.iv.buffer = (void*)aes_test_iv,
		.params.iv.size = sizeof(aes_test_iv),		
		.buffer = (void*)aes_test_pure_data_in,
		.size = sizeof(aes_test_pure_data_in),
		.result.buffer = (void*)aes_cbc_192key_clear_en_result,
		.result.size = sizeof(aes_cbc_192key_clear_en_result),
	},
	{
		.testcase = "DSC_AES_CBC_192KEY_CLEAR_DE",
		.params.algo_type = DSC_ALGO_AES,
		.params.crypto_mode = DSC_DECRYPT,
		.params.chaining_mode = DSC_MODE_CBC,
		.params.residue_mode = DSC_RESIDUE_CLEAR,
		.params.key.buffer = (void*)aes_test_192key,
		.params.key.size = sizeof(aes_test_192key),
		.params.iv.buffer = (void*)aes_test_iv,
		.params.iv.size = sizeof(aes_test_iv),		
		.buffer = (void*)aes_test_pure_data_in,
		.size = sizeof(aes_test_pure_data_in),
		.result.buffer = (void*)aes_cbc_192key_clear_de_result,
		.result.size = sizeof(aes_cbc_192key_clear_de_result),
	},
	
	{
		.testcase = "DSC_AES_CBC_256KEY_CLEAR_EN",
		.params.algo_type = DSC_ALGO_AES,
		.params.crypto_mode = DSC_ENCRYPT,
		.params.chaining_mode = DSC_MODE_CBC,
		.params.residue_mode = DSC_RESIDUE_CLEAR,
		.params.key.buffer = (void*)aes_test_256key,
		.params.key.size = sizeof(aes_test_256key),
		.params.iv.buffer = (void*)aes_test_iv,
		.params.iv.size = sizeof(aes_test_iv),		
		.buffer = (void*)aes_test_pure_data_in,
		.size = sizeof(aes_test_pure_data_in),
		.result.buffer = (void*)aes_cbc_256key_clear_en_result,
		.result.size = sizeof(aes_cbc_256key_clear_en_result),
	},
	{
		.testcase = "DSC_AES_CBC_256KEY_CLEAR_DE",
		.params.algo_type = DSC_ALGO_AES,
		.params.crypto_mode = DSC_DECRYPT,
		.params.chaining_mode = DSC_MODE_CBC,
		.params.residue_mode = DSC_RESIDUE_CLEAR,
		.params.key.buffer = (void*)aes_test_256key,
		.params.key.size = sizeof(aes_test_256key),
		.params.iv.buffer = (void*)aes_test_iv,
		.params.iv.size = sizeof(aes_test_iv),		
		.buffer = (void*)aes_test_pure_data_in,
		.size = sizeof(aes_test_pure_data_in),
		.result.buffer = (void*)aes_cbc_256key_clear_de_result,
		.result.size = sizeof(aes_cbc_256key_clear_de_result),
	},
	//AES CBC ATSC
	{
		.testcase = "DSC_AES_CBC_128KEY_ATSC_EN",
		.params.algo_type = DSC_ALGO_AES,
		.params.crypto_mode = DSC_ENCRYPT,
		.params.chaining_mode = DSC_MODE_CBC,
		.params.residue_mode = DSC_RESIDUE_AS_ATSC,
		.params.key.buffer = (void*)aes_test_128key,
		.params.key.size = sizeof(aes_test_128key),
		.params.iv.buffer = (void*)aes_test_iv,
		.params.iv.size = sizeof(aes_test_iv),		
		.buffer = (void*)aes_test_pure_data_in,
		.size = sizeof(aes_test_pure_data_in),
		.result.buffer = (void*)aes_cbc_128key_atsc_en_result,
		.result.size = sizeof(aes_cbc_128key_atsc_en_result),
	},
	{
		.testcase = "DSC_AES_CBC_128KEY_ATSC_DE",
		.params.algo_type = DSC_ALGO_AES,
		.params.crypto_mode = DSC_DECRYPT,
		.params.chaining_mode = DSC_MODE_CBC,
		.params.residue_mode = DSC_RESIDUE_AS_ATSC,
		.params.key.buffer = (void*)aes_test_128key,
		.params.key.size = sizeof(aes_test_128key),
		.params.iv.buffer = (void*)aes_test_iv,
		.params.iv.size = sizeof(aes_test_iv),		
		.buffer = (void*)aes_test_pure_data_in,
		.size = sizeof(aes_test_pure_data_in),
		.result.buffer = (void*)aes_cbc_128key_atsc_de_result,
		.result.size = sizeof(aes_cbc_128key_atsc_de_result),
	},
	{
		.testcase = "DSC_AES_CBC_192KEY_ATSC_EN",
		.params.algo_type = DSC_ALGO_AES,
		.params.crypto_mode = DSC_ENCRYPT,
		.params.chaining_mode = DSC_MODE_CBC,
		.params.residue_mode = DSC_RESIDUE_AS_ATSC,
		.params.key.buffer = (void*)aes_test_192key,
		.params.key.size = sizeof(aes_test_192key),
		.params.iv.buffer = (void*)aes_test_iv,
		.params.iv.size = sizeof(aes_test_iv),		
		.buffer = (void*)aes_test_pure_data_in,
		.size = sizeof(aes_test_pure_data_in),
		.result.buffer = (void*)aes_cbc_192key_atsc_en_result,
		.result.size = sizeof(aes_cbc_192key_atsc_en_result),
	},
	{
		.testcase = "DSC_AES_CBC_192KEY_ATSC_DE",
		.params.algo_type = DSC_ALGO_AES,
		.params.crypto_mode = DSC_DECRYPT,
		.params.chaining_mode = DSC_MODE_CBC,
		.params.residue_mode = DSC_RESIDUE_AS_ATSC,
		.params.key.buffer = (void*)aes_test_192key,
		.params.key.size = sizeof(aes_test_192key),
		.params.iv.buffer = (void*)aes_test_iv,
		.params.iv.size = sizeof(aes_test_iv),		
		.buffer = (void*)aes_test_pure_data_in,
		.size = sizeof(aes_test_pure_data_in),
		.result.buffer = (void*)aes_cbc_192key_atsc_de_result,
		.result.size = sizeof(aes_cbc_192key_atsc_de_result),
	},
	
	{
		.testcase = "DSC_AES_CBC_256KEY_ATSC_EN",
		.params.algo_type = DSC_ALGO_AES,
		.params.crypto_mode = DSC_ENCRYPT,
		.params.chaining_mode = DSC_MODE_CBC,
		.params.residue_mode = DSC_RESIDUE_AS_ATSC,
		.params.key.buffer = (void*)aes_test_256key,
		.params.key.size = sizeof(aes_test_256key),
		.params.iv.buffer = (void*)aes_test_iv,
		.params.iv.size = sizeof(aes_test_iv),		
		.buffer = (void*)aes_test_pure_data_in,
		.size = sizeof(aes_test_pure_data_in),
		.result.buffer = (void*)aes_cbc_256key_atsc_en_result,
		.result.size = sizeof(aes_cbc_256key_atsc_en_result),
	},
	{
		.testcase = "DSC_AES_CBC_256KEY_ATSC_DE",
		.params.algo_type = DSC_ALGO_AES,
		.params.crypto_mode = DSC_DECRYPT,
		.params.chaining_mode = DSC_MODE_CBC,
		.params.residue_mode = DSC_RESIDUE_AS_ATSC,
		.params.key.buffer = (void*)aes_test_256key,
		.params.key.size = sizeof(aes_test_256key),
		.params.iv.buffer = (void*)aes_test_iv,
		.params.iv.size = sizeof(aes_test_iv),		
		.buffer = (void*)aes_test_pure_data_in,
		.size = sizeof(aes_test_pure_data_in),
		.result.buffer = (void*)aes_cbc_256key_atsc_de_result,
		.result.size = sizeof(aes_cbc_256key_atsc_de_result),
	},
	//AES CBC CTS
	{
		.testcase = "DSC_AES_CBC_128KEY_CTS_EN",
		.params.algo_type = DSC_ALGO_AES,
		.params.crypto_mode = DSC_ENCRYPT,
		.params.chaining_mode = DSC_MODE_CBC,
		.params.residue_mode = DSC_RESIDUE_HW_CTS_CS2,
		.params.key.buffer = (void*)aes_test_128key,
		.params.key.size = sizeof(aes_test_128key),
		.params.iv.buffer = (void*)aes_test_iv,
		.params.iv.size = sizeof(aes_test_iv),		
		.buffer = (void*)aes_test_pure_data_in,
		.size = sizeof(aes_test_pure_data_in),
		.result.buffer = (void*)aes_cbc_128key_cts_en_result,
		.result.size = sizeof(aes_cbc_128key_cts_en_result),
	},
	{
		.testcase = "DSC_AES_CBC_128KEY_CTS_DE",
		.params.algo_type = DSC_ALGO_AES,
		.params.crypto_mode = DSC_DECRYPT,
		.params.chaining_mode = DSC_MODE_CBC,
		.params.residue_mode = DSC_RESIDUE_HW_CTS_CS2,
		.params.key.buffer = (void*)aes_test_128key,
		.params.key.size = sizeof(aes_test_128key),
		.params.iv.buffer = (void*)aes_test_iv,
		.params.iv.size = sizeof(aes_test_iv),		
		.buffer = (void*)aes_test_pure_data_in,
		.size = sizeof(aes_test_pure_data_in),
		.result.buffer = (void*)aes_cbc_128key_cts_de_result,
		.result.size = sizeof(aes_cbc_128key_cts_de_result),
	},
	{
		.testcase = "DSC_AES_CBC_192KEY_CTS_EN",
		.params.algo_type = DSC_ALGO_AES,
		.params.crypto_mode = DSC_ENCRYPT,
		.params.chaining_mode = DSC_MODE_CBC,
		.params.residue_mode = DSC_RESIDUE_HW_CTS_CS2,
		.params.key.buffer = (void*)aes_test_192key,
		.params.key.size = sizeof(aes_test_192key),
		.params.iv.buffer = (void*)aes_test_iv,
		.params.iv.size = sizeof(aes_test_iv),		
		.buffer = (void*)aes_test_pure_data_in,
		.size = sizeof(aes_test_pure_data_in),
		.result.buffer = (void*)aes_cbc_192key_cts_en_result,
		.result.size = sizeof(aes_cbc_192key_cts_en_result),
	},
	{
		.testcase = "DSC_AES_CBC_192KEY_CTS_DE",
		.params.algo_type = DSC_ALGO_AES,
		.params.crypto_mode = DSC_DECRYPT,
		.params.chaining_mode = DSC_MODE_CBC,
		.params.residue_mode = DSC_RESIDUE_HW_CTS_CS2,
		.params.key.buffer = (void*)aes_test_192key,
		.params.key.size = sizeof(aes_test_192key),
		.params.iv.buffer = (void*)aes_test_iv,
		.params.iv.size = sizeof(aes_test_iv),		
		.buffer = (void*)aes_test_pure_data_in,
		.size = sizeof(aes_test_pure_data_in),
		.result.buffer = (void*)aes_cbc_192key_cts_de_result,
		.result.size = sizeof(aes_cbc_192key_cts_de_result),
	},
	
	{
		.testcase = "DSC_AES_CBC_256KEY_CTS_EN",
		.params.algo_type = DSC_ALGO_AES,
		.params.crypto_mode = DSC_ENCRYPT,
		.params.chaining_mode = DSC_MODE_CBC,
		.params.residue_mode = DSC_RESIDUE_HW_CTS_CS2,
		.params.key.buffer = (void*)aes_test_256key,
		.params.key.size = sizeof(aes_test_256key),
		.params.iv.buffer = (void*)aes_test_iv,
		.params.iv.size = sizeof(aes_test_iv),		
		.buffer = (void*)aes_test_pure_data_in,
		.size = sizeof(aes_test_pure_data_in),
		.result.buffer = (void*)aes_cbc_256key_cts_en_result,
		.result.size = sizeof(aes_cbc_256key_cts_en_result),
	},
	{
		.testcase = "DSC_AES_CBC_256KEY_CTS_DE",
		.params.algo_type = DSC_ALGO_AES,
		.params.crypto_mode = DSC_DECRYPT,
		.params.chaining_mode = DSC_MODE_CBC,
		.params.residue_mode = DSC_RESIDUE_HW_CTS_CS2,
		.params.key.buffer = (void*)aes_test_256key,
		.params.key.size = sizeof(aes_test_256key),
		.params.iv.buffer = (void*)aes_test_iv,
		.params.iv.size = sizeof(aes_test_iv),		
		.buffer = (void*)aes_test_pure_data_in,
		.size = sizeof(aes_test_pure_data_in),
		.result.buffer = (void*)aes_cbc_256key_cts_de_result,
		.result.size = sizeof(aes_cbc_256key_cts_de_result),
	},
	//AES CTR
	{
		.testcase = "DSC_AES_CTR_128KEY_EN",
		.params.algo_type = DSC_ALGO_AES,
		.params.crypto_mode = DSC_ENCRYPT,
		.params.chaining_mode = DSC_MODE_CTR,
		.params.residue_mode = DSC_RESIDUE_AS_ATSC,
		.params.key.buffer = (void*)aes_test_128key,
		.params.key.size = sizeof(aes_test_128key),
		.params.iv.buffer = (void*)aes_test_iv,
		.params.iv.size = sizeof(aes_test_iv),		
		.buffer = (void*)aes_test_pure_data_in,
		.size = sizeof(aes_test_pure_data_in),
		.result.buffer = (void*)aes_ctr_128key_en_de_result,
		.result.size = sizeof(aes_ctr_128key_en_de_result),
	},
	{
		.testcase = "DSC_AES_CTR_128KEY_DE",
		.params.algo_type = DSC_ALGO_AES,
		.params.crypto_mode = DSC_DECRYPT,
		.params.chaining_mode = DSC_MODE_CTR,
		.params.residue_mode = DSC_RESIDUE_AS_ATSC,
		.params.key.buffer = (void*)aes_test_128key,
		.params.key.size = sizeof(aes_test_128key),
		.params.iv.buffer = (void*)aes_test_iv,
		.params.iv.size = sizeof(aes_test_iv),		
		.buffer = (void*)aes_test_pure_data_in,
		.size = sizeof(aes_test_pure_data_in),
		.result.buffer = (void*)aes_ctr_128key_en_de_result,
		.result.size = sizeof(aes_ctr_128key_en_de_result),
	},
	{
		.testcase = "DSC_AES_CTR_192KEY_EN",
		.params.algo_type = DSC_ALGO_AES,
		.params.crypto_mode = DSC_ENCRYPT,
		.params.chaining_mode = DSC_MODE_CTR,
		.params.residue_mode = DSC_RESIDUE_AS_ATSC,
		.params.key.buffer = (void*)aes_test_192key,
		.params.key.size = sizeof(aes_test_192key),
		.params.iv.buffer = (void*)aes_test_iv,
		.params.iv.size = sizeof(aes_test_iv),		
		.buffer = (void*)aes_test_pure_data_in,
		.size = sizeof(aes_test_pure_data_in),
		.result.buffer = (void*)aes_ctr_192key_en_de_result,
		.result.size = sizeof(aes_ctr_192key_en_de_result),
	},
	{
		.testcase = "DSC_AES_CTR_192KEY_DE",
		.params.algo_type = DSC_ALGO_AES,
		.params.crypto_mode = DSC_DECRYPT,
		.params.chaining_mode = DSC_MODE_CTR,
		.params.residue_mode = DSC_RESIDUE_AS_ATSC,
		.params.key.buffer = (void*)aes_test_192key,
		.params.key.size = sizeof(aes_test_192key),
		.params.iv.buffer = (void*)aes_test_iv,
		.params.iv.size = sizeof(aes_test_iv),		
		.buffer = (void*)aes_test_pure_data_in,
		.size = sizeof(aes_test_pure_data_in),
		.result.buffer = (void*)aes_ctr_192key_en_de_result,
		.result.size = sizeof(aes_ctr_192key_en_de_result),
	},
	
	{
		.testcase = "DSC_AES_CTR_256KEY_EN",
		.params.algo_type = DSC_ALGO_AES,
		.params.crypto_mode = DSC_ENCRYPT,
		.params.chaining_mode = DSC_MODE_CTR,
		.params.residue_mode = DSC_RESIDUE_AS_ATSC,
		.params.key.buffer = (void*)aes_test_256key,
		.params.key.size = sizeof(aes_test_256key),
		.params.iv.buffer = (void*)aes_test_iv,
		.params.iv.size = sizeof(aes_test_iv),		
		.buffer = (void*)aes_test_pure_data_in,
		.size = sizeof(aes_test_pure_data_in),
		.result.buffer = (void*)aes_ctr_256key_en_de_result,
		.result.size = sizeof(aes_ctr_256key_en_de_result),
	},
	{
		.testcase = "DSC_AES_CTR_256KEY_DE",
		.params.algo_type = DSC_ALGO_AES,
		.params.crypto_mode = DSC_DECRYPT,
		.params.chaining_mode = DSC_MODE_CTR,
		.params.residue_mode = DSC_RESIDUE_AS_ATSC,
		.params.key.buffer = (void*)aes_test_256key,
		.params.key.size = sizeof(aes_test_256key),
		.params.iv.buffer = (void*)aes_test_iv,
		.params.iv.size = sizeof(aes_test_iv),
		.buffer = (void*)aes_test_pure_data_in,
		.size = sizeof(aes_test_pure_data_in),
		.result.buffer = (void*)aes_ctr_256key_en_de_result,
		.result.size = sizeof(aes_ctr_256key_en_de_result),
	},

	//AES ECB
	{
		.testcase = "DSC_AES_ECB_128KEY_CLEAR_EN",
		.params.algo_type = DSC_ALGO_AES,
		.params.crypto_mode = DSC_ENCRYPT,
		.params.chaining_mode = DSC_MODE_ECB,
		.params.residue_mode = DSC_RESIDUE_CLEAR,
		.params.key.buffer = (void*)aes_test_128key,
		.params.key.size = sizeof(aes_test_128key),
		.buffer = (void*)aes_test_pure_data_in,
		.size = sizeof(aes_test_pure_data_in),
		.result.buffer = (void*)aes_ecb_128key_en_result,
		.result.size = sizeof(aes_ecb_128key_en_result),
	},
	{
		.testcase = "DSC_AES_ECB_128KEY_CLEAR_DE",
		.params.algo_type = DSC_ALGO_AES,
		.params.crypto_mode = DSC_DECRYPT,
		.params.chaining_mode = DSC_MODE_ECB,
		.params.residue_mode = DSC_RESIDUE_CLEAR,
		.params.key.buffer = (void*)aes_test_128key,
		.params.key.size = sizeof(aes_test_128key),
		.buffer = (void*)aes_test_pure_data_in,
		.size = sizeof(aes_test_pure_data_in),
		.result.buffer = (void*)aes_ecb_128key_de_result,
		.result.size = sizeof(aes_ecb_128key_de_result),
	},
	{
		.testcase = "DSC_AES_ECB_192KEY_CLEAR_EN",
		.params.algo_type = DSC_ALGO_AES,
		.params.crypto_mode = DSC_ENCRYPT,
		.params.chaining_mode = DSC_MODE_ECB,
		.params.residue_mode = DSC_RESIDUE_CLEAR,
		.params.key.buffer = (void*)aes_test_192key,
		.params.key.size = sizeof(aes_test_192key),
		.buffer = (void*)aes_test_pure_data_in,
		.size = sizeof(aes_test_pure_data_in),
		.result.buffer = (void*)aes_ecb_192key_en_result,
		.result.size = sizeof(aes_ecb_192key_en_result),
	},
	{
		.testcase = "DSC_AES_ECB_192KEY_CLEAR_DE",
		.params.algo_type = DSC_ALGO_AES,
		.params.crypto_mode = DSC_DECRYPT,
		.params.chaining_mode = DSC_MODE_ECB,
		.params.residue_mode = DSC_RESIDUE_CLEAR,
		.params.key.buffer = (void*)aes_test_192key,
		.params.key.size = sizeof(aes_test_192key),
		.buffer = (void*)aes_test_pure_data_in,
		.size = sizeof(aes_test_pure_data_in),
		.result.buffer = (void*)aes_ecb_192key_de_result,
		.result.size = sizeof(aes_ecb_192key_de_result),
	},
	
	{
		.testcase = "DSC_AES_ECB_256KEY_CLEAR_EN",
		.params.algo_type = DSC_ALGO_AES,
		.params.crypto_mode = DSC_ENCRYPT,
		.params.chaining_mode = DSC_MODE_ECB,
		.params.residue_mode = DSC_RESIDUE_CLEAR,
		.params.key.buffer = (void*)aes_test_256key,
		.params.key.size = sizeof(aes_test_256key),
		.buffer = (void*)aes_test_pure_data_in,
		.size = sizeof(aes_test_pure_data_in),
		.result.buffer = (void*)aes_ecb_256key_en_result,
		.result.size = sizeof(aes_ecb_256key_en_result),
	},
	{
		.testcase = "DSC_AES_ECB_256KEY_CLEAR_DE",
		.params.algo_type = DSC_ALGO_AES,
		.params.crypto_mode = DSC_DECRYPT,
		.params.chaining_mode = DSC_MODE_ECB,
		.params.residue_mode = DSC_RESIDUE_CLEAR,
		.params.key.buffer = (void*)aes_test_256key,
		.params.key.size = sizeof(aes_test_256key),
		.buffer = (void*)aes_test_pure_data_in,
		.size = sizeof(aes_test_pure_data_in),
		.result.buffer = (void*)aes_ecb_256key_de_result,
		.result.size = sizeof(aes_ecb_256key_de_result),
	},

}; 

#define DSC_AES_TESTCASE_NUM (sizeof(dsc_aes_test_case)/sizeof(dsc_aes_test_case[0]))

#endif /* __WAVE_HEADER_H__ */

