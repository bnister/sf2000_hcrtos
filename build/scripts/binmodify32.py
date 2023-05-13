#!/usr/bin/env python3

import os
import struct
import argparse

def main():
	parser = argparse.ArgumentParser(description='binary merge')
	parser.add_argument("--input", action="store", dest="input",
		type=str, help="input file")
	parser.add_argument("--output", action="store", dest="output",
		type=str, help="output file")
	parser.add_argument("--offset", action="store", dest="offset",
		type=str, help="offset in output file")
	parser.add_argument("--fill", action="store", dest="fill",
		type=str, help="fill value at offset")

	args = parser.parse_args()
	finput = ""
	foutput = ""
	offset = 0
	fill = 0xffffffff

	if args.fill:
		if args.fill[0] == '0' and args.fill[1].lower() == 'x':
			fill = int(args.fill, 16)
		else:
			fill = int(args.fill, 10)

	if args.input:
		finput = args.input

	if args.output:
		foutput = args.output

	if args.offset:
		if len(args.offset) > 1 and args.offset[0] == '0' and args.offset[1].lower() == 'x':
			offset = int(args.offset, 16)
		else:
			offset = int(args.offset, 10)
	
	if finput == "":
		parser.print_help()
		sys.exit(1)
	
	if foutput == "":
		foutput = finput

	ibin = open(finput, 'rb')
	data = ibin.read()
	ibin.close()

	obin = open(foutput, 'wb')
	obin.write(data)
	obin.seek(offset)
	fillpack = struct.pack('l', fill)
	obin.write(fillpack)
	obin.flush()
	obin.close()

if __name__ == '__main__':
	main()
