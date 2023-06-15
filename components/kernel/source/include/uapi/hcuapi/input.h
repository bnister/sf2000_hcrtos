/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 1999-2002 Vojtech Pavlik
 */
#ifndef _INPUT_H
#define _INPUT_H

#ifdef __HCRTOS__

#include <hcuapi/iocbase.h>
#include <hcuapi/input-event-codes.h>
#ifndef __KERNEL__
#include <stdint.h>
#endif

struct input_event {
	uint16_t type;
	uint16_t code;
	int32_t value;
};

/**
 * struct input_keymap_entry - used by EVIOCGKEYCODE/EVIOCSKEYCODE ioctls
 * @scancode: scancode represented in machine-endian form.
 * @len: length of the scancode that resides in @scancode buffer.
 * @index: index in the keymap, may be used instead of scancode
 * @flags: allows to specify how kernel should handle the request. For
 *	example, setting INPUT_KEYMAP_BY_INDEX flag indicates that kernel
 *	should perform lookup in keymap by @index instead of @scancode
 * @keycode: key code assigned to this scancode
 *
 * The structure is used to retrieve and modify keymap data. Users have
 * option of performing lookup either by @scancode itself or by @index
 * in keymap entry. EVIOCGKEYCODE will also return scancode or index
 * (depending on which element was used to perform lookup).
 */
struct input_keymap_entry {
#define INPUT_KEYMAP_BY_INDEX	(1 << 0)
	uint8_t  flags;
	uint8_t  len;
	uint16_t index;
	uint32_t keycode;
	uint8_t  scancode[32];
};

#define EVIOCGKEYCODE	_IOR(INPUT_IOCBASE, 0x04, struct input_keymap_entry)
#define EVIOCSKEYCODE	_IOW(INPUT_IOCBASE, 0x04, struct input_keymap_entry)

/*
 * IDs.
 */

#define ID_BUS			0
#define ID_VENDOR		1
#define ID_PRODUCT		2
#define ID_VERSION		3

#define BUS_PCI			0x01
#define BUS_ISAPNP		0x02
#define BUS_USB			0x03
#define BUS_HIL			0x04
#define BUS_BLUETOOTH		0x05
#define BUS_VIRTUAL		0x06

#define BUS_ISA			0x10
#define BUS_I8042		0x11
#define BUS_XTKBD		0x12
#define BUS_RS232		0x13
#define BUS_GAMEPORT		0x14
#define BUS_PARPORT		0x15
#define BUS_AMIGA		0x16
#define BUS_ADB			0x17
#define BUS_I2C			0x18
#define BUS_HOST		0x19
#define BUS_GSC			0x1A
#define BUS_ATARI		0x1B
#define BUS_SPI			0x1C

/*
 * MT_TOOL types
 */
#define MT_TOOL_FINGER		0
#define MT_TOOL_PEN		1
#define MT_TOOL_PALM		2
#define MT_TOOL_MAX		2

/*
 * Values describing the status of a force-feedback effect
 */
#define FF_STATUS_STOPPED	0x00
#define FF_STATUS_PLAYING	0x01
#define FF_STATUS_MAX		0x01


#endif

#endif
