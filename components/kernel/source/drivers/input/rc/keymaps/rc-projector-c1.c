// SPDX-License-Identifier: GPL-2.0+
// Keytable for projector_c1_ Remote Controller
//
// Copyright © 2021 HiChip Semiconductor Co., Ltd.
//              http://www.hichiptech.com

#include <kernel/module.h>
#include <kernel/io.h>

#include <hcuapi/input-event-codes.h>
#include <hcuapi/rc-proto.h>
#include "../rc-map.h"

static struct rc_map_table projector_c1[] = {
	{ 0xa8, KEY_POWER },
	{ 0x88, KEY_MUTE },
	{ 0x98, KEY_LEFTSHIFT },
	{ 0x93, KEY_PLAY },
	{ 0x82, KEY_RIGHTSHIFT },
	{ 0x95, KEY_UP    },
	{ 0x9b, KEY_LEFT  },
	{ 0x99, KEY_RIGHT },
	{ 0x9A, KEY_DOWN  },
	{ 0x9E, KEY_OK },
	{ 0xA4, KEY_EXIT },
	{ 0x91, KEY_EPG },
	{ 0x97, KEY_MENU },
	{ 0x9C, KEY_VOLUMEDOWN },
	{ 0x8C, KEY_VOLUMEUP },

	#if 1
	{ 0x1c, KEY_POWER },
	{ 0x04, KEY_ROTATE_DISPLAY },
	{ 0x42, KEY_PREVIOUS },
	{ 0x01, KEY_NEXT },
	{ 0x03, KEY_PLAY },
	{ 0x11, KEY_STOP },
	/*  0x09       0x05        0x4b   0x4f *
	 *  LEFTSHIFT RIGHTSHIFT PREVIOUS NEXT *
	 *                                     */
	{ 0x40, KEY_RIGHTSHIFT },
	{ 0x02, KEY_LEFTSHIFT },
	
	/*          0x1a          *
	 *           Up           *
	 *                        *
	 *  0x47    0x06    0x07  *
	 *  Left     Ok     Right *
	 *                        *
	 *         0x48           *
	 *         Down           *
	 *                        */
	{ 0x4e, KEY_UP    },
	{ 0x0c, KEY_LEFT  },
	{ 0x05, KEY_RIGHT },
	{ 0x4d, KEY_DOWN  },
	{ 0x06, KEY_OK },
	/*  0x14              0x13      *
	 *  KEY_VOLUMEUP KEY_VOLUMEDOWN *
	 *                              */
	{ 0x47, KEY_VOLUMEDOWN },
	{ 0x43, KEY_VOLUMEUP },

	{ 0x0f, KEY_EPG },
	{ 0x4c, KEY_EXIT },

	{ 0x46, KEY_MENU },
	{ 0x59, KEY_MUTE },
	/*  0x54    0x16    0x15  *
	 *   1       2       3    *
	 *                        *
	 *  0x50    0x12    0x11  *
	 *   4       5       6    *
	 *                        *
	 *  0x4c    0xe    0xd    *
	 *   7       8       9    *
	 *                        */
	{ 0x1e, KEY_NUMERIC_0 },
	{ 0x45, KEY_NUMERIC_1 },
	{ 0x17, KEY_NUMERIC_2 },
	{ 0x12, KEY_NUMERIC_3 },
	{ 0x14, KEY_NUMERIC_4 },
	{ 0x07, KEY_NUMERIC_5 },
	{ 0x0d, KEY_NUMERIC_6 },
	{ 0x1b, KEY_NUMERIC_7 },
	{ 0x49, KEY_NUMERIC_8 },
	{ 0x44, KEY_NUMERIC_9 },
	#endif
	#if 0 // AK02
	{ 0x118, KEY_POWER },
	{ 0x117, KEY_EPG },
	{ 0x11A, KEY_LIGHTS_TOGGLE},
	{ 0x146, KEY_UP    },
	{ 0x147, KEY_LEFT  },
	{ 0x115, KEY_RIGHT },
	{ 0x116, KEY_DOWN  },
	{ 0x155, KEY_OK },
	{ 0x114, KEY_VOLUMEDOWN },
	{ 0x110, KEY_VOLUMEUP },
	{ 0x106, KEY_MENU },
	{ 0x140, KEY_EXIT },
	{ 0x166, KEY_VOICECOMMAND },
	#endif
	#if 1 // CJ01
	{ 0x100, KEY_POWER },
	{ 0x101, KEY_EPG },
	{ 0x109, KEY_MENU},
	{ 0x105, KEY_UP    },
	{ 0x102, KEY_LEFT  },
	{ 0x10A, KEY_RIGHT },
	{ 0X104, KEY_DOWN  },
	{ 0X106, KEY_OK },
	{ 0X103, KEY_PLAY },
	{ 0X10B, KEY_EXIT },
	{ 0X140, KEY_VOLUMEDOWN },
	{ 0X148, KEY_VOLUMEUP },
	{ 0X144, KEY_MUTE },
	#endif
};

static struct rc_map_list projector_c1_map = {
	.map = {
		.scan     = projector_c1,
		.size     = ARRAY_SIZE(projector_c1),
		.rc_proto = RC_PROTO_NEC,
		.name     = "rc-projector-c1",
	}
};

static int init_rc_map_projector_c1(void)
{
	return rc_map_register(&projector_c1_map);
}

module_driver(rc_map_projector_c1, init_rc_map_projector_c1, NULL, 0)
