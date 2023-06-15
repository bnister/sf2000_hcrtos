#!/usr/bin/env python3

import os, sys
import struct
import argparse
import fdt

def main():
	parser = argparse.ArgumentParser(description='modify dtb property')
	parser.add_argument("--dtb", action="store", dest="dtb",
		type=str, help="input dtb file")
	parser.add_argument("--path", action="store", dest="path",
		type=str, help="path of node")
	parser.add_argument("--property", action="store", dest="property",
		type=str, help="property in the node")
	parser.add_argument("--string", action="store", dest="value",
		type=str, help="value of property in the node")

	args = parser.parse_args()
	dtb = ""
	node = ""
	prop = ""
	val = ""

	if args.dtb:
		dtb = args.dtb
	if args.path:
		node = args.path
	if args.property:
		prop = args.property
	if args.value:
		val = args.value

	if dtb == "" or node == "" or property == "" or val == "":
		parser.print_help()
		sys.exit(1)

	with open(dtb, "rb") as f:
		dtb_data = f.read()

	dt1 = fdt.parse_dtb(dtb_data)

	dt1.set_property(prop, [val], path=node)

	with open(dtb, "wb") as f:
		f.write(dt1.to_dtb(version=17))

	sys.exit(0)

if __name__ == '__main__':
	main()
