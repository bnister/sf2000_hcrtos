#!/usr/bin/env python3

import os, sys
import struct
import argparse
import fdt

def main():
	parser = argparse.ArgumentParser(description='get fdt property')
	parser.add_argument("--dtb", action="store", dest="dtb",
		type=str, help="input dtb file")
	parser.add_argument("--node", action="store", dest="node",
		type=str, help="node")
	parser.add_argument("--prop", action="store", dest="prop",
		type=str, help="property")

	args = parser.parse_args()
	dtb = ""
	node = ""
	prop = ""

	if args.dtb:
		dtb = args.dtb
	if args.node:
		node = args.node
	if args.prop:
		prop = args.prop

	if dtb == "" or node == "" or prop == "":
		parser.print_help()
		sys.exit(1)

	with open(dtb, "rb") as f:
		dtb_data = f.read()
		dt = fdt.parse_dtb(dtb_data)
		fdtnode = dt.get_node(node)
		fdtprop = fdtnode.get_property(prop)
		print(fdtprop.data[0])

	sys.exit(0)

if __name__ == '__main__':
	main()
