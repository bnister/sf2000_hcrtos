// SPDX-License-Identifier: GPL-2.0+
// Keytable for projector_c1_wjv6_ Remote Controller
//
// Copyright © 2021 HiChip Semiconductor Co., Ltd.
//              http://www.hichiptech.com

#include <kernel/module.h>
#include <kernel/io.h>

#include <hcuapi/input-event-codes.h>
#include <hcuapi/rc-proto.h>
#include "../rc-map.h"

static struct rc_map_table projector_c1_wjv6[] = {
	#if 1 //RC305
	{ 0xa8, KEY_POWER },
	{ 0x88, KEY_MUTE },
	// { 0x98, KEY_LEFTSHIFT },
	// { 0x93, KEY_PLAY },
	// { 0x82, KEY_RIGHTSHIFT },
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
	#if 1 // WJV6
	{ 0x14, KEY_POWER },
	{ 0x01, KEY_MUTE },
	{ 0x98, KEY_MENU },
	{ 0x93, KEY_BACK },
	{ 0x82, KEY_FORWARD },
	{ 0x03, KEY_UP    },
	{ 0x0E, KEY_LEFT  },
	{ 0x1A, KEY_RIGHT },
	{ 0X02, KEY_DOWN  },
	{ 0X07, KEY_OK  },
	{ 0X5C, KEY_EXIT  },
	{ 0X13, KEY_EPG  },
	{ 0X48, KEY_HOME  },
	{ 0X58, KEY_VOLUMEDOWN  },
	{ 0X0B, KEY_VOLUMEUP  },
	#endif
};

static struct rc_map_list projector_c1_wjv6_map = {
	.map = {
		.scan     = projector_c1_wjv6,
		.size     = ARRAY_SIZE(projector_c1_wjv6),
		.rc_proto = RC_PROTO_NEC,
		.name     = "rc-projector-c1-wjv6",
	}
};

static int init_rc_map_projector_c1_wjv6(void)
{
	return rc_map_register(&projector_c1_wjv6_map);
}

module_driver(rc_map_projector_c1_wjv6, init_rc_map_projector_c1_wjv6, NULL, 0)
