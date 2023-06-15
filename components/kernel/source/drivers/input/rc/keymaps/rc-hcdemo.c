// SPDX-License-Identifier: GPL-2.0+
// Keytable for hcdemo Remote Controller
//
// Copyright © 2021 HiChip Semiconductor Co., Ltd.
//              http://www.hichiptech.com

#include <kernel/module.h>
#include <kernel/io.h>

#include <hcuapi/input-event-codes.h>
#include <hcuapi/rc-proto.h>
#include "../rc-map.h"
static struct rc_map_table hcdemo[] =
{
 {0x45, KEY_POWER},
 {0x46, KEY_MENU},
 {0x47, KEY_MUTE},
 {0x44,KEY_MODE},
 {0x40,KEY_UP},
 
 {0x43,KEY_ESC},
 {0x7,KEY_LEFT},
 {0x15, KEY_OK},
 {0x9,KEY_RIGHT},
 {0x16,KEY_0},
 
 {0x19,KEY_DOWN},
 {0xd,KEY_OK},
 {0xc,KEY_1},
 {0x18, KEY_2},
 {0x5e, KEY_3},
 
 {0x8, KEY_4},
 {0x1c, KEY_5},
 {0x5a, KEY_6},
 {0x42, KEY_7},
 {0x52, KEY_8},
 
 {0x4a, KEY_9}, 
};



	static struct rc_map_table hcdemo1[] = {
	
		/*	0x1c   0x3	 0x42  0x8	*
		 * POWER  AUDIO  SAT   MUTE *
		 *							*/
		{ 0x45, KEY_POWER },
		{ 0x46, KEY_TV	 },
		//{ 0x47, KEY_USB_SD	 },
		
		{ 0x44, KEY_1 },
		{ 0x40, KEY_2	 },
		{ 0x43, KEY_3	 },
		{ 0x07, KEY_4 },
		{ 0x15, KEY_5	 },
		{ 0x09, KEY_6	 },
		{ 0x16, KEY_7 },
		{ 0x19, KEY_8	 },
		{ 0x0D, KEY_9	 },
	//	{ 0x0C, KEY_10 },
		{ 0x18, KEY_UP	 },
		{ 0x5e, KEY_BACK	 },
		{ 0x08, KEY_VOLUMEDOWN },
		{ 0x1C, KEY_ENTER	 },
		{ 0x5A, KEY_VOLUMEUP	 },
	//	{ 0x42, KEY_REPEAT },
		{ 0x52, KEY_DOWN	 },
	//		{0x4A,KEY_NP},
	
		/*	{0X7E1000,KEY_1},
			{0X7EF098,KEY_2},

			{0X7E20F4,KEY_3},
			{0X7E2082,KEY_4},
			{0X7E300A,KEY_5},
			{0X7E007A,KEY_6},
			{0X7E60F6,KEY_7},
			{0X7ED0BE,KEY_8},
			{0X7E70B1,KEY_9},
			{0X7E5079,KEY_10},

		
			{0X771000,KEY_DOWN},
			{0X772058,KEY_UP},
			{0X77D0F4,KEY_OK},
			{0X773082,KEY_BACK},
			{0X7E1000,KEY_PLAY},
			{0X7EF098,KEY_PAUSE},


			{0X7E1000,KEY_STOP},
			{0X7EF098,KEY_RESTART},

		
			{0X7E1000,KEY_VOLUMEDOWN},
			{0X7EF098,KEY_VOLUMEUP},*/
	};

static struct rc_map_list hcdemo_map = {
	.map = {
		.scan     = hcdemo,
		.size     = ARRAY_SIZE(hcdemo),
		.rc_proto = RC_PROTO_NEC,
		.name     = "rc-hcdemo",
	}
};

static int init_rc_map_hcdemo(void)
{
	return rc_map_register(&hcdemo_map);
}

module_driver(rc_map_hcdemo, init_rc_map_hcdemo, NULL, 0)
