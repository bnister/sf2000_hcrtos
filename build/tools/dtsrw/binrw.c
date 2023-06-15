// binrw.cpp : Defines the entry point for the console application.
//

//#include "stdafx.h"


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void show_help()
{
	printf("Usage: ");
	printf("        binrw <input file> <offset> <length> <output file> <offset> <length>\n");
}

int main(int argc, char* argv[])
{
	int ret = 0;
	char *input_file = NULL;
	char *output_file = NULL;
	int input_offset = 0;
	int output_offset = 0;
	int input_len = 0;
	int output_len = 0;
	FILE *in_fp = NULL;
	FILE *out_fp = NULL;
	char *file_data = NULL;
	int in_file_length = 0;
	int out_file_length = 0;
	int read_length = 0;
	int write_length = 0;
	
	if (argc != 7)
	{
		show_help();
		return -1;
	}
	input_file = argv[1];
	input_offset = atoi(argv[2]);
	input_len = atoi(argv[3]);
	output_file = argv[4];
	output_offset = atoi(argv[5]);
	output_len = atoi(argv[6]);

	
	in_fp = fopen(input_file, "rb");
	if (!in_fp)
	{
		printf("Open input file %s fail!\n", input_file);
		return -1;
	}
	fseek(in_fp, 0, SEEK_END);
	in_file_length = ftell(in_fp);
	if (input_offset >= in_file_length)
	{
		printf("input_offset can not larger than input file length!\n");
		ret = -1;
		goto exit;
	}
	if (0 == input_len)
		input_len = in_file_length;

	if (input_offset+input_len <=  in_file_length)
		read_length = input_len;
	else
		read_length = in_file_length - input_offset;

	write_length = output_len;
	if (read_length > write_length)
		write_length = read_length;

	out_fp = fopen(output_file, "rb");
	if (out_fp)
	{
		fclose(out_fp);
		out_fp = fopen(output_file, "rb+");
	}
	else
	{
		out_fp = fopen(output_file, "wb+");
	}
	if (!out_fp)
	{
		printf("Open output file %s fail!\n", output_file);
		ret = -1;
		goto exit;
	}
	fseek(out_fp, 0, SEEK_END);
	out_file_length = ftell(out_fp);

	if (output_offset < out_file_length)
		fseek(out_fp, output_offset, SEEK_SET);
	else
		fseek(out_fp, 0, SEEK_END);
	
	fseek(in_fp, input_offset, SEEK_SET);
	
	file_data = (char*)malloc(write_length);
	if (!file_data)
	{
		printf("malloc %d bytes error!\n", write_length);
		ret = -1;
		goto exit;
	}
	memset(file_data, 0xff, write_length);

	int rw_len;
	rw_len = fread(file_data, 1, read_length, in_fp);
	printf("read data return: %d\n", rw_len);
	rw_len = fwrite(file_data, 1, write_length, out_fp);
	printf("write data return: %d\n", rw_len);

exit:
	if (file_data)
		free(file_data);
	if (in_fp)
		fclose(in_fp);
	if (out_fp)
		fclose(out_fp);

	if (ret)
		printf("Write %d data to %s %d fail!\n", write_length, output_file, output_offset);
	else
		printf("Write %d data to %s %d successfully!\n", write_length, output_file, output_offset);

	return 0;
}

