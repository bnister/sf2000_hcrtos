#!/usr/bin/env python3

import os, sys
import struct
import argparse
import fdt

def main():
	parser = argparse.ArgumentParser(description='partition info')
	parser.add_argument("--dtb", action="store", dest="dtb",
		type=str, help="input dtb file")
	parser.add_argument("--partname", action="store", dest="partname",
		type=str, help="partition name")
	parser.add_argument("--get-offset", action="store", dest="get_offset",
		type=str, help="get offset from dtb")
	parser.add_argument("--get-size", action="store", dest="get_size",
		type=str, help="get size from dtb")

	args = parser.parse_args()
	size = 0
	offset = 0
	get_size = "false"
	get_offset = "false"

	if args.get_offset:
		get_offset = args.get_offset

	if args.get_size:
		get_size = args.get_size

	if args.dtb != "" and args.partname != "":
		with open(args.dtb, "rb") as f:
			dtb_data = f.read()
			dt = fdt.parse_dtb(dtb_data)
			node = dt.get_node("/hcrtos/sfspi/spi_nor_flash/partitions")
			part_num = node.get_property("part-num")
			for i in range(1, part_num.data[0] + 1):
				label = node.get_property("part" + str(i) + "-label")
				if label.data[0] != args.partname:
					continue
				reg = node.get_property("part" + str(i) + "-reg")
				offset = reg.data[0]
				size = reg.data[1]
				break

	if get_size == "true":
		print ("0x%08x" % size)

	if get_offset == "true":
		print ("0x%08x" % offset)

	sys.exit(0)

if __name__ == '__main__':
	main()
