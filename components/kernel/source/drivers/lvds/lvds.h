#ifndef _LVDS_H_

#define _LVDS_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <getopt.h>
#include <sys/ioctl.h>
#include <kernel/io.h>
#include <kernel/types.h>
#include <kernel/vfs.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <freertos/event_groups.h>
#include <kernel/lib/console.h>
#include <kernel/elog.h>
#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif

//#define LVDS_REG_PHY_SYS                                                                (0xB8800440)
#define LVDS_REG_PHY_SYS                                                                (0x440)

//DIGTAL

#define LVDS_REG_DIGTAL_CNTR                                                    0x00

#define LVDS_REG_DIGTAL_TIGGER                                                  0x04

//PHY

#define LVDS_REG_PHY_0x0                                                                (0x0)
#define LVDS_REG_PHY_0x1                                                                (0x1)
#define LVDS_REG_PHY_0x2                                                                (0x2)
#define LVDS_REG_PHY_0x4                                                                (0x4)
#define LVDS_REG_PHY_0x5                                                                (0x5)
#define LVDS_REG_PHY_0x08                                                               (0x8)
#define LVDS_REG_PHY_0xc                                                                (0xc)

#define LVDS_REG_PHY_DIG_01                                                             (0x21)

typedef enum _E_VIDEO_SRC_SEL_ {
	E_SRC_SEL_FXDE = 0,
	E_SRC_SEL_4KDE,
	E_SRC_SEL_HDMI_RX,
} E_VIDEO_SRC_SEL;

typedef enum _E_VIDEO_2_SRC_SEL_ {
	E_SRC_2_SEL_FXDE = 0,
	E_SRC_2_SEL_4KDE,
	E_SRC_2_SEL_HDMI_RX,
	E_SRC_2_SEL_FXDE_,
	E_SRC_2_SEL_FXDE_LOW_TO_HIGH,
	E_SRC_2_SEL_DE4K_LOW_TO_HIGH,
	E_SRC_2_SEL_HDMI_RX_LOW_TO_HIGH,
	E_SRC_2_SEL_FXDE__,
} E_VIDEO_2_SRC_SEL;

typedef struct _T_LVDS_CFG_ {
	uint32_t channel_mode;
	uint32_t map_mode;
	uint32_t ch0_src_sel;
	uint32_t ch1_src_sel;
	uint32_t ch0_invert_clk_sel;
	uint32_t ch1_invert_clk_sel;
	uint32_t ch0_clk_gate;
	uint32_t ch1_clk_gate;
	uint32_t hsync_polarity;
	uint32_t vsync_polarity;
	uint32_t even_odd_adjust_mode;
	uint32_t even_odd_init_value;
	E_VIDEO_SRC_SEL src_sel;
} T_LVDS_CFG;

typedef enum _E_LVDS_CHANNEL_MODE_ {
	E_CHANNEL_MODE_SINGLE_IN_SINGLE_OUT,
	E_CHANNEL_MODE_SINGLE_IN_DUAL_OUT,
} E_LVDS_CHANNEL_MODE;

typedef enum _E_LVDS_MAP_MODE_ {
	E_MAP_MODE_VESA_24BIT,
	E_MAP_MODE_VESA_18BIT_OR_JEDIA,
} E_LVDS_MAP_MODE;

typedef enum _E_LVDS_EVEN_ODD_ADJUST_MODE_ {
	E_ADJUST_MODE_FRAME_START,
	E_ADJUST_MODE_HSYNC_POS,
	E_ADJUST_MODE_VSYNC_POS,
} E_LVDS_EVEN_ODD_ADJUST_MODE;

#ifdef __cplusplus
}
#endif

#endif
