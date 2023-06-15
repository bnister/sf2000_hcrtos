#!/usr/bin/env python3

import os, sys
import struct
import argparse
import fdt

def main():
	parser = argparse.ArgumentParser(description='dtb to dts convert')
	parser.add_argument("-b", "--dtb", action="store", dest="dtb",
		type=str, help="input dtb file")
	parser.add_argument("-s", "--dts", action="store", dest="dts",
		type=str, help="output dts file")

	args = parser.parse_args()
	dtb = ""
	dts = ""

	if args.dtb:
		dtb = args.dtb
	if args.dts:
		dts = args.dts

	if dtb == "" or dts == "":
		parser.print_help()
		sys.exit(1)

	with open(dtb, "rb") as f:
		dtb_data = f.read()

	dt1 = fdt.parse_dtb(dtb_data)

	with open(dts, "w") as f:
		f.write(dt1.to_dts())

	sys.exit(0)

if __name__ == '__main__':
	main()
