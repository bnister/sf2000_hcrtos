/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * (C) Copyright 2008 Semihalf
 *
 * (C) Copyright 2000-2005
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 ********************************************************************
 * NOTE: This header file defines an interface to U-Boot. Including
 * this (unmodified) header file in another file is considered normal
 * use of U-Boot, and does *not* fall under the heading of "derived
 * work".
 ********************************************************************
 */

#ifndef __HDMI_SWITCH_H__
#define __HDMI_SWITCH_H__

//#define HDMI_SWITCH_HDMI_RX_TO_HDMI_TX
#define HDMI_SWITCH_HDMI_RX_TO_LCD
//#define HDMI_SWITCH_DE_EFFECT_ADJUST
//#define HDMI_SWITCH_HDMI_RX_DIS_PILLBOX
//#define HDMI_SWITCH_HDMI_RX_SET_ZOOM_480x640

#if defined(HDMI_SWITCH_HDMI_RX_TO_HDMI_TX)
#define HDMI_SWITCH_BACK_BOOTLOGO
#define HDMI_SWITCH_HDMI_TX
#define HDMI_SWITCH_HDMI_RX
#elif defined(HDMI_SWITCH_HDMI_RX_TO_LCD)
//#define HDMI_SWITCH_BACK_BOOTLOGO
#define HDMI_SWITCH_HDMI_RX
#endif


int hdmi_switch (void);

#endif	/* __HDMI_SWITCH_H__ */

