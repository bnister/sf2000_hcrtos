#! /usr/bin/python3
#_*_ coding:UTF-8 _*_

import sys, getopt, fileinput, re, os
import argparse
from tempfile import NamedTemporaryFile

def create_tmpfile_for_parse(inputfile, outputfile):
	write_dst = False
	fin = open(inputfile)
	fout = open(outputfile, 'w')
	for line in fin:
		if (re.match("^\.text", line) != None):
			write_dst = True
		if (re.match("\.reginfo", line) != None):
			break;
		if (write_dst):
			fout.write(line)

	fin.close()
	fout.close()

def get_lib_name(str):
	obj = re.search("lib[\w|\-|\+]+\.a", str)
	if (obj != None):
		return obj.group()
	else:
		return None

def get_lib_name2(str):
	obj = re.search("lib[\w|\-|\+]+\.a", str)
	if (obj != None):
		return obj.group()
	else:
		obj = re.search("built-in.o", str)
		if (obj != None):
			return obj.group()
		else:
			return None

def get_libs(path):
	list = []
	for line in fileinput.input(path):
		name = get_lib_name(line)
		if ((name != None) and (name not in list)):
			list.append(name)
	list.append('fill')
	list.append('built-in.o')
	return list

def is_sub_section(line):
	if ((line[0:2] == " .") or (line[0:3] == " CO")):
		return True
	else:
		return False

def is_section(line):
	if (line[0:1] == "."):
		return True
	else:
		return False

def get_sections(path):
	list = []
	for line in fileinput.input(path):
		if (is_section(line)):
			name = line.split()[0]
		if (name not in list):
			list.append(name)
	return list

def parse_link_size(path, lib_list, sec_list, printType = 0):
	sec_dic = {}
	for sec in sec_list:
		lib_dic = dict.fromkeys(lib_list, 0)
		sec_dic.update({sec:lib_dic})

	fp = open(path)
	sec = None
	pre_arr = ['0x0','0x0','0x0','0x0']
	for line in fp:
		if (is_section(line)):
			sec = line.split()[0]
		if (is_sub_section(line)):
			arr = line.split()
			if(len(arr) == 1):
				line =fp.readline()
				arr.extend(line.split())
			lib = get_lib_name(arr[3])
			if (lib != None):
				sec_dic[sec][lib]    += int(arr[2],16)
			elif (arr[3] in lib_list):
				sec_dic[sec][arr[3]] += int(arr[2],16)
			else:
				print("Unknow module: %s"%arr[3])
				sys.exit()
			if (printType == 2):
				# 打印递增异常项
				if (int(arr[1],16) != (int(pre_arr[1],16) + int(pre_arr[2],16))):
					print(pre_arr)
					print(line)
				pre_arr = arr
			if (printType == 3 and len(arr) != 4):
				# 打印不规则项
				print(arr)
		if (line[1:7] == "*fill*"):
			arr = line.split()
			sec_dic[sec]['fill']     += int(arr[2], 16)
			if (printType == 2):
				# 打印递增异常项
				if (int(arr[1],16) != (int(pre_arr[1],16) + int(pre_arr[2],16))):
					print(pre_arr)
					print(line)
				pre_arr = arr
	fp.close()

	return sec_dic

def filt_library(inputfile, libfilt):
	write_dst = False
	lib_write_dst = False
	fp = open(inputfile)
	for line in fp:
		nline = ''
		if (re.match("^\.text", line) != None):
			write_dst = True
			print(line,end="")
		if (re.match("\.reginfo", line) != None):
			break;

		if (is_sub_section(line)):
			arr = line.split()
			if(len(arr) == 1):
				nline =fp.readline()
				arr.extend(nline.split())
			lib = get_lib_name2(arr[3])
			if (lib == libfilt or libfilt == 'all'):
				lib_write_dst = True
			elif (lib != None):
				lib_write_dst = False

		if (write_dst and lib_write_dst):
			print(line,end="")
			if (nline != ''):
				print(nline,end="")
	fp.close()

def get_map_range(path):
	begin = 0
	end = 0
	for line in fileinput.input(path):
		if (re.match("\.text", line) != None):
			arr = line.split()
			begin = int(arr[1], 16)
		if (re.search(" __bss_end = \.", line) != None):
			arr = line.split()
			end = int(arr[0], 16)
	return begin, end

def main():
	parser = argparse.ArgumentParser(description='mapfile parser')
	parser.add_argument("-m", "--map", action="store", dest="map",
		type=str, help="input mapfile")
	parser.add_argument("-l", "--lib", action="store", dest="libfilt",
		type=str, help="filter linked size of specific lib")

	args = parser.parse_args()
	inputfile = ''
	libfilt = ''
	outputfile = ''
	printType = 0

	if args.map:
		inputfile = args.map
	if args.libfilt:
		libfilt = args.libfilt

	if inputfile == '':
		parser.print_help()
		sys.exit(1)

	if libfilt != '':
		filt_library(inputfile, libfilt)
		sys.exit(0)

	with NamedTemporaryFile('w+t', delete=False) as f:
		outputfile = f.name
	f.close()

	print('输入文件：', inputfile)
	print('临时文件：', outputfile)

	create_tmpfile_for_parse(inputfile, outputfile)

	sec_list = get_sections(outputfile)

	lib_list = get_libs(outputfile)

	sec_dic = parse_link_size(outputfile, lib_list, sec_list, printType)

	map_begin, map_end = get_map_range(outputfile)

	os.remove(outputfile)

	lib_size_dic = dict.fromkeys(lib_list, 0)

	# 计算每个.a大小及内存总大小
	mem_total = 0
	for lib in lib_list:
		lib_sum = 0
		for sec in sec_list:
			lib_sum += sec_dic[sec][lib]
		lib_size_dic[lib] = lib_sum
		mem_total += lib_sum

	# 按大小排列 .a
	lib_sorted_list = sorted(lib_size_dic.items(), key = lambda kv:(kv[1], kv[0]), reverse=False)

	if (printType == 0):
		# 打印 .a 信息
		for item in lib_sorted_list:
			lib = item[0]
			print("Module: %s"%lib)
			print("%-20s %10s %10s" %('section', 'byte', 'KB'))
			print('-' * 42)
			for sec in sec_list:
				sec_size = sec_dic[sec][lib]
				print("%-20s %10d %10d" %(sec, sec_size, sec_size/1024))
			print('-' * 42)
			print("%-20s %10d %10d %20s" %('Sum', item[1], item[1]/1024, lib))
			print('')

	if (printType == 1):
		# 打印 section 信息
		print("%-20s %10s %10s" %('section Sum', 'byte', 'KB'))
		print('-' * 42)
		for sec in sec_list:
			sec_sum = 0
			for lib in lib_list:
				sec_sum += sec_dic[sec][lib]
			print("%-20s %10d %10d"%(sec, sec_sum, sec_sum/1024))

	if (printType == 0 or printType == 1):
		# 打印识别到的总大小
		print('=' * 50)
		print("%-20s %10d %10d\n" %("Total", mem_total, (mem_total)/1024))

		# 打印 mapfile 映射范围
		print('=' * 50)
		map_size =  map_end - map_begin
		print("Parse range in [%s]:"%inputfile)
		print('-' * 42)
		print(".text = 0x%016x"%map_begin)
		print(".end  = 0x%016x"%map_end)
		print("size  = 0x%x | %d byte | %d KB\n"%(map_size, map_size, map_size/1024))

if __name__ == "__main__":
	main()
