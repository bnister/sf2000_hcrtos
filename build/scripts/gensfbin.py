#!/usr/bin/env python3

import os, sys
import struct
import argparse
import fdt

def main():
	parser = argparse.ArgumentParser(description='binary merge')
	parser.add_argument("--dtb", action="store", dest="dtb",
		type=str, help="input dtb file")
	parser.add_argument("--output", action="store", dest="ofile",
		type=str, help="output file")
	parser.add_argument("--wkdir", action="store", dest="wkdir",
		type=str, help="work directory")
	parser.add_argument("--fill", action="store", dest="fill",
		type=str, help="fill value if input file size is less than parameter passed by --size")

	args = parser.parse_args()
	foutput = ""
	size = 0
	offset = 0
	pos = 0
	endpos = 0
	fill = 0xff
	dtb = ""
	wkdir = "."

	if args.fill:
		if args.fill[0] == '0' and args.fill[1].lower() == 'x':
			fill = int(args.fill, 16)
		else:
			fill = int(args.fill, 10)

	if args.ofile:
		foutput = args.ofile
	if args.dtb:
		dtb = args.dtb
	if args.wkdir:
		wkdir = args.wkdir

	if foutput == "" or dtb == "":
		parser.print_help()
		sys.exit(1)

	obin = open(foutput,'wb')
	fillpack = struct.pack('B', fill)

	with open(args.dtb, "rb") as f:
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

			if filename.data[0] == "null":
				endpos = offset + size
				while pos < endpos:
					obin.write(fillpack)
					pos = obin.tell()
			else:
				try:
					ibin = open(wkdir + "/" + filename.data[0], 'rb')
				except OSError as reason:
					print (str(reason))
					sys.exit(1)

				ibinsize = os.path.getsize(wkdir + "/" + filename.data[0])
				if ibinsize > size:
					print ("input file size(%d) is larger than size(%d) for partition %s" % (ibinsize, size, label.data[0]))
					sys.exit(1)

				endpos = offset + size
				data = ibin.read()
				obin.write(data)
				pos = obin.tell()
				while pos < endpos:
					obin.write(fillpack)
					pos = obin.tell()

				ibin.close()

	obin.flush()
	obin.close()

	sys.exit(0)

if __name__ == '__main__':
	main()
