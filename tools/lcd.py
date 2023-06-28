#!/usr/bin/env python3

'''A tool to snip LCD initialization sequence from HiChip's ST7789V_80I driver

  Copyright (C) 2023 Nikita Burnashev

Permission to use, copy, modify, and/or distribute this software
for any purpose with or without fee is hereby granted.

THIS SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND!'''

import sys

with open(sys.argv[1], 'rb') as f:
	bin = f.read()

# The driver sets a bit of a register (fast GPIO) then loads an array reference
MIPS_machine_code = (
	b'\x10\x00\xb0\xaf' # sw    s0, 0x10(sp)
	b'\x80\xb8\x03\x3c' # li    v1, 0xb8800094
	b'\x94\x00\x63\x34'
	b'\x00\x00\x62\x8c' # lw    v0, 0(v1)
	b'\x01\x00\x04\x3c' # li    a0, 1 << 16
	b'\x25\x10\x44\x00' # or    v0, a0
	b'\x00\x00\x62\xac' # sw    v0, 0(v1)
	b'\x21\x80\x00\x00' # clear s0
	#'\xxx\xxx\x02\x3c' # lui   v0, %hi(g_LCD_init)
	#'\xxx\xxx\x51\x24' # addiu s1, v0, %lo(g_LCD_init)
)
pc = bin.find(MIPS_machine_code)
if pc < 0:
	sys.exit("ST7789V_80I initialization routine wasn't found")
pc += len(MIPS_machine_code)

pd = bin[pc + 1] << 24 | bin[pc] << 16 | bin[pc + 5] << 8 | bin[pc + 4]
pd -= pd << 1 & 0x10000 # addiu takes a sign-extended operand
pd -= 0x80000000 # image base

ms = 0 # RGB_CLK_NORMAL
if (
	b'\x78\x00\x03\x36' # ori   v1, s0, 0x78 # 0xb8800094, SYS_CLK_CTR
	b'\x00\x00\x62\x8c' # lw    v0, 0(v1)
	b'\x00\x80\x42\x34' # ori   v0, 0x8000 # RGB clock skew/invert?
	b'\x00\x00\x62\xac' # sw    v0, 0(v1)
) in bin:
	ms = 1 # RGB_CLK_SKEW

commands = { # some common ones
	0x11: 'SLPOUT',	# Sleep Out
	0x13: 'NORON',	# Normal Display Mode On
	    
	0x21: 'INVON',	# Display Inversion On
	0x29: 'DISPON',	# Display On
	0x2a: 'CASET',	# Column Address Set
	0x2b: 'RASET',	# Row Address Set
	    
	0x35: 'TEON',	# Tearing Effect Line On
	0x36: 'MADCTL',	# Memory Data Access Control
	0x3a: 'COLMOD'	# Interface Pixel Format
};

while bin[pd] != 255 or bin[pd + 1] != 255:
	if bin[pd + 1] == 0x10: # command
		cmd = bin[pd]
		if cmd in commands:
			bytes = [ commands[cmd] ]
		else:
			bytes = [ '0x%02x' % (cmd, ) ]
		pd += 2
		while bin[pd + 1] == 0: # data
			bytes.append('0x%02x' % (bin[pd], ))
			pd += 2
		print('%d, %d, %s,' % (ms, len(bytes), ', '.join(bytes)))
		ms = 0
	elif bin[pd + 1] == 0x20: # delay in ms
		while bin[pd + 1] == 0x20:
			ms += bin[pd]
			pd += 2
	else:
		sys.exit('Unexpected 0x%02x at 0x%x' % (bin[pd + 1], pd + 1))
