#!/usr/bin/env python3

import os, sys
import struct
import argparse
import fdt

def main():
	parser = argparse.ArgumentParser(description='hcprog ini generator')
	parser.add_argument("--dtb", action="store", dest="dtb",
		type=str, help="input dtb file")
	parser.add_argument("--output", action="store", dest="ofile",
		type=str, help="output file")
	parser.add_argument("--chip", action="store", dest="chip",
		type=str, help="chip type")
	parser.add_argument("--draminit", action="store", dest="draminit",
		type=str, help="dram init file")
	parser.add_argument("--version", action="store", dest="version",
		type=str, help="version")
	parser.add_argument("--product", action="store", dest="product",
		type=str, help="product")

	args = parser.parse_args()
	size = 0
	offset = 0
	ofile = ""
	chip = ""
	draminit = ""
	version = ""
	product = ""

	if args.ofile:
		ofile = args.ofile
	if args.chip:
		chip = args.chip
	if args.draminit:
		draminit = args.draminit
	if args.version:
		version = args.version
	if args.product:
		product = args.product

	if ofile == "" or chip == "" or draminit == "" or version == "" or product == "":
		parser.print_help()
		sys.exit(1)

	myfile = open(ofile,'w')

	if args.dtb != "":
		with open(args.dtb, "rb") as f:
			section = '''
[SYSINI]
HICHIP = %s

[STARTUP-FILE]
DRAM = %s
UPDATER = updater.bin
UPDATER_LOAD_ADDR = 0xA0000200
UPDATER_RUN_ADDR  = 0xA0000200

[NORFLASH-DEFAULT-CONFIG]
NORF_TOTAL_SIZE = 0x2000000
NORF_ERASE_SIZE = 0x10000
NORF_SECTOR_SIZE= 0x10000
NORF_PAGE_SIZE  = 0x100

[SYSINFO]
VERSION = %s
PRODUCT = %s
''' % (chip, draminit, version, product)
			myfile.write(section)

			dtb_data = f.read()
			dt = fdt.parse_dtb(dtb_data)
			node = dt.get_node("/hcrtos/sfspi/spi_nor_flash/partitions")
			part_num = node.get_property("part-num")

			for i in range(1, part_num.data[0] + 1):
				label = node.get_property("part" + str(i) + "-label")
				filename = node.get_property("part" + str(i) + "-filename")
				reg = node.get_property("part" + str(i) + "-reg")
				offset = reg.data[0]
				size = reg.data[1]
				section = '''

[PARTITION%d]
DEVICE = NOR
NAME   = %s
OFFSET = 0x%08x
SIZE   = 0x%08x
FILE   = %s
''' % (i-1, label.data[0], offset, size, filename.data[0])
				myfile.write(section)

	myfile.close()
	sys.exit(0)

if __name__ == '__main__':
	main()
