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
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdint.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <hcuapi/dsc.h>

#include "sha_test.h"
#include "aes_test.h"
#include "des_test.h"



#define LOG_TAG "DSCUTI"
#define log_e printf
#define log_i printf

int dsc_fd = -1;


static int dsc_entry(void);
static void dsc_close(void);
static void dump_bin(char *prefix, uint8_t *data, int size);
#if 0
static int dsc_tdes_test(int argc, char **argv)
{
	int ret = 0;
	uint8_t i = 0;
	uint8_t des_test_out[34]={0};

	dsc_entry();
	for(i=0;i<DSC_DES_TESTCASE_NUM;i++){
		if(!dsc_des_test_case[i].testcase)
			continue;
		printf("TESTCASE: %s\n",dsc_des_test_case[i].testcase);
		dump_bin("key:", dsc_des_test_case[i].params.key.buffer, dsc_des_test_case[i].params.key.size);
		dump_bin("iv:", dsc_des_test_case[i].params.iv.buffer, dsc_des_test_case[i].params.iv.size);
		dump_bin("input:", dsc_des_test_case[i].buffer, dsc_des_test_case[i].size);
		dsc_des_test_case[i].params.output.buffer = (void*)des_test_out;
		dsc_des_test_case[i].params.output.size = sizeof(des_test_out); 
		ioctl(dsc_fd, DSC_DO_AES_DES, (unsigned long)&dsc_des_test_case[i].params);
		dump_bin("output:", dsc_des_test_case[i].params.output.buffer, dsc_des_test_case[i].params.output.size);

		ret = memcmp(dsc_des_test_case[i].params.output.buffer,
				(void*)dsc_des_test_case[i].result.buffer,dsc_des_test_case[i].result.size);
				
		if(ret)
			log_e("%s test failed!\n",dsc_des_test_case[i].testcase);
		else	
			log_e("%s test succeed!\n",dsc_des_test_case[i].testcase);
		/*break;*/
	}
	dsc_close();
	
	log_i("dsc des test done!\n");
	return 0;
}
#endif

static void dump_bin(char *prefix, uint8_t *data, int size)
{
	if(!data)
		return;
	printf("%s", prefix);
	for (int i = 0; i < size; i++)
		printf("%02x ", data[i]);
	printf("\n");
}
#if 0
static int dsc_aes_test(int argc, char **argv)
{
	int ret = 0;
	uint8_t i = 0;
	uint8_t aes_test_out[34]={0};
	

	// i = 2, OFB
	for(i=0;i<DSC_AES_TESTCASE_NUM;i++){
		if(!dsc_aes_test_case[i].testcase || !strstr(dsc_aes_test_case[i].testcase,"CTR"))
			continue;
		dsc_entry();
		printf("TESTCASE: %s\n",dsc_aes_test_case[i].testcase);
		dump_bin("key:", dsc_aes_test_case[i].params.key.buffer, dsc_aes_test_case[i].params.key.size);
		dump_bin("iv:", dsc_aes_test_case[i].params.iv.buffer, dsc_aes_test_case[i].params.iv.size);
		dump_bin("input:", dsc_aes_test_case[i].buffer, dsc_aes_test_case[i].size);
		dsc_aes_test_case[i].params.output.buffer = (void*)aes_test_out;
		dsc_aes_test_case[i].params.output.size = sizeof(aes_test_out);	
		ioctl(dsc_fd, DSC_DO_AES_DES, (unsigned long)&dsc_aes_test_case[i].params);
		dump_bin("output:", dsc_aes_test_case[i].params.output.buffer, dsc_aes_test_case[i].params.output.size);

		ret = memcmp(dsc_aes_test_case[i].params.output.buffer,
				(void*)dsc_aes_test_case[i].result.buffer,dsc_aes_test_case[i].result.size);
				
		if(ret){
			log_e("%s test failed!\n",dsc_aes_test_case[i].testcase);
		}else{	
			log_e("%s test succeed!\n",dsc_aes_test_case[i].testcase);
		}	
		dsc_close();
		/*break;*/
	}
	
	log_i("dsc aes test done!\n");
	return 0;
}
#endif

static int dsc_aes_seperate_test(int argc, char **argv)
{
	int ret = 0;
	uint8_t i = 0;
	uint8_t aes_test_out[34]={0};
	struct dsc_crypt out;

	memset(&out, 0, sizeof(out));
	printf("seperate\n");
	
	// i = 2, OFB
	for(i=0;i<DSC_AES_TESTCASE_NUM;i++){
		if(!dsc_aes_test_case[i].testcase || !strstr(dsc_aes_test_case[i].testcase,"CTR"))
			continue;
		dsc_entry();
		printf("TESTCASE: %s\n",dsc_aes_test_case[i].testcase);
		dump_bin("key:", dsc_aes_test_case[i].params.key.buffer, dsc_aes_test_case[i].params.key.size);
		dump_bin("iv:", dsc_aes_test_case[i].params.iv.buffer, dsc_aes_test_case[i].params.iv.size);
		dump_bin("input:", dsc_aes_test_case[i].buffer, dsc_aes_test_case[i].size);
		ioctl(dsc_fd, DSC_CONFIG, (unsigned long)&dsc_aes_test_case[i].params);
		
		printf("********************** 1 ***************\n");
		out.output = (void*)aes_test_out;
		out.size = 16;
		out.input = dsc_aes_test_case[i].buffer;
		ioctl(dsc_fd, DSC_DO_ENCRYPT, (unsigned long)&out);
		dump_bin("output:", aes_test_out, 34);

		printf("********************** 2 ***************\n");
		out.output = (void*)aes_test_out + 16;
		out.size = sizeof(aes_test_out) -16;	
		out.input = dsc_aes_test_case[i].buffer + 16;
		ioctl(dsc_fd, DSC_DO_ENCRYPT, (unsigned long)&out);
		dump_bin("output:", aes_test_out, 34);

		ret = memcmp(aes_test_out,
				(void*)dsc_aes_test_case[i].result.buffer,dsc_aes_test_case[i].result.size);
				
		if(ret){
			log_e("%s test failed!\n",dsc_aes_test_case[i].testcase);
		}else{	
			log_e("%s test succeed!\n",dsc_aes_test_case[i].testcase);
		}	
		dsc_close();
		/*break;*/
	}
	
	log_i("dsc aes test done!\n");
	return 0;
}

int dsc_sha_test(int argc, char **argv)
{
	int ret = 0;
	uint8_t i = 0;
	uint8_t sha1_out[64]={0};

	if(dsc_fd<0)
		return -ENODEV;
		
	for(i=0;i<DSC_SHA_TESTCASE_NUM;i++){
		if(!dsc_sha_test_case[i].testcase)
			continue;
		dsc_sha_test_case[i].params.output.buffer = (void*)sha1_out;
		dsc_sha_test_case[i].params.output.size = sizeof(sha1_out);
		ioctl(dsc_fd, DSC_DO_SHA, (unsigned long)&dsc_sha_test_case[i].params);

		ret = memcmp(dsc_sha_test_case[i].params.output.buffer,
				dsc_sha_test_case[i].result.buffer,dsc_sha_test_case[i].result.size);
		if(ret)
			log_e("%s test failed!\n",dsc_sha_test_case[i].testcase);
		else	
			log_e("%s test succeed!\n",dsc_sha_test_case[i].testcase);
	}	
	
	log_i("dsc sha test done!\n");
	return 0;
}

static int dsc_entry(void)
{
	dsc_fd = open("/dev/dsc", O_RDWR);
	if (dsc_fd < 0) {
		log_e("open dsc dev failed!\n");
		return -ENODEV;
	}
	return 0;
}

static void dsc_close(void)
{
	if (dsc_fd > 0) {
		close(dsc_fd);
		dsc_fd = -1;
	}
}

int main(int argc, char *argv[])
{
	/*dsc_aes_test(0, NULL);*/
	dsc_aes_seperate_test(0, NULL);
	/*dsc_tdes_test(0, NULL);*/
	return 0;
}

