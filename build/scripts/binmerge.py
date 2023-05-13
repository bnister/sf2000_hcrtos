#!/usr/bin/env python3

import os, sys
import struct
import argparse
import fdt

def main():
	parser = argparse.ArgumentParser(description='binary merge')
	parser.add_argument("--input", action="store", dest="input",
		type=str, help="input file")
	parser.add_argument("--output", action="store", dest="output",
		type=str, help="output file")
	parser.add_argument("--dtb", action="store", dest="dtb",
		type=str, help="input dtb file")
	parser.add_argument("--partname", action="store", dest="partname",
		type=str, help="partition name")
	parser.add_argument("--offset", action="store", dest="offset",
		type=str, help="offset in output file")
	parser.add_argument("--size", action="store", dest="size",
		type=str, help="merge size")
	parser.add_argument("--fill", action="store", dest="fill",
		type=str, help="fill value if input file size is less than parameter passed by --size")
	parser.add_argument("--expand", action="store", dest="expand",
		type=str, help="expand with fill value to offset + size")

	args = parser.parse_args()
	finput = ""
	foutput = ""
	size = 0
	offset = 0
	pos = 0
	endpos = 0
	fill = 0xff
	expand = "true"

	if args.fill:
		if args.fill[0] == '0' and args.fill[1].lower() == 'x':
			fill = int(args.fill, 16)
		else:
			fill = int(args.fill, 10)

	if args.expand:
		expand = args.expand

	if args.input:
		finput = args.input

	if args.output:
		foutput = args.output

	if args.offset:
		if len(args.offset) > 1 and args.offset[0] == '0' and args.offset[1].lower() == 'x':
			offset = int(args.offset, 16)
		else:
			offset = int(args.offset, 10)

	if args.size:
		if len(args.size) > 1 and args.size[0] == '0' and args.size[1].lower() == 'x':
			size = int(args.size, 16)
		else:
			size = int(args.size, 10)

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

	if size == 0 and finput != "":
		size = os.path.getsize(finput)

	if foutput == "" or size == 0:
		parser.print_help()
		sys.exit(1)

	obin = open(foutput,'ab+')
	obinsize = os.path.getsize(foutput)
	if obinsize > 0:
		obin.seek(0)
		odata = obin.read()
	obin.close()
	obin = open(foutput, 'wb')
	if obinsize > 0:
		obin.write(odata)

	if finput != "":
		try:
			ibin = open(finput, 'rb')
		except OSError as reason:
			print (str(reason))
			print ("merge full section fill with 0x%02x" % fill)
			ibin = ""
	else:
		print ("merge (NULL), full section fill with 0x%02x" % fill)
		ibin = ""

	if ibin != "":
		ibinsize = os.path.getsize(finput)
		if ibinsize > size:
			print ("input file size(%d) is larger than size(%d)" % (ibinsize, size))
			sys.exit(1)
		print ("merge %s (file size: 0x%08x)" % (finput, ibinsize))
	else:
		print ("merge %s (file size: 0)" % finput)

	print ("offset 0x%08x, size 0x%08x, fill value 0x%02x, expand %s" % (offset, size, fill, expand))

	if expand == "false":
		size = ibinsize

	fillpack = struct.pack('B', fill)

	if obinsize >= offset:
		pos = offset
	else:
		pos = obinsize

	endpos = offset + size

	obin.seek(pos)
	while pos < endpos:
		if pos == offset and ibin != "":
			data = ibin.read()
			obin.write(data)
			pos = obin.tell()
		else:
			obin.write(fillpack)
			pos = obin.tell()

	obin.flush()
	obin.close()
	if ibin != "":
		ibin.close()

	sys.exit(0)

if __name__ == '__main__':
	main()
