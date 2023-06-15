/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
/*
 * lirc.h - linux infrared remote control header file
 * last modified 2010/07/13 by Jarod Wilson
 */

#ifndef _LINUX_LIRC_H
#define _LINUX_LIRC_H

/**
 * enum rc_proto - the Remote Controller protocol
 *
 * @RC_PROTO_UNKNOWN: Protocol not known
 * @RC_PROTO_OTHER: Protocol known but proprietary
 * @RC_PROTO_RC5: Philips RC5 protocol
 * @RC_PROTO_RC5X_20: Philips RC5x 20 bit protocol
 * @RC_PROTO_RC5_SZ: StreamZap variant of RC5
 * @RC_PROTO_JVC: JVC protocol
 * @RC_PROTO_SONY12: Sony 12 bit protocol
 * @RC_PROTO_SONY15: Sony 15 bit protocol
 * @RC_PROTO_SONY20: Sony 20 bit protocol
 * @RC_PROTO_NEC: NEC protocol
 * @RC_PROTO_NECX: Extended NEC protocol
 * @RC_PROTO_NEC32: NEC 32 bit protocol
 * @RC_PROTO_SANYO: Sanyo protocol
 * @RC_PROTO_MCIR2_KBD: RC6-ish MCE keyboard
 * @RC_PROTO_MCIR2_MSE: RC6-ish MCE mouse
 * @RC_PROTO_RC6_0: Philips RC6-0-16 protocol
 * @RC_PROTO_RC6_6A_20: Philips RC6-6A-20 protocol
 * @RC_PROTO_RC6_6A_24: Philips RC6-6A-24 protocol
 * @RC_PROTO_RC6_6A_32: Philips RC6-6A-32 protocol
 * @RC_PROTO_RC6_MCE: MCE (Philips RC6-6A-32 subtype) protocol
 * @RC_PROTO_SHARP: Sharp protocol
 * @RC_PROTO_XMP: XMP protocol
 * @RC_PROTO_CEC: CEC protocol
 * @RC_PROTO_IMON: iMon Pad protocol
 * @RC_PROTO_RCMM12: RC-MM protocol 12 bits
 * @RC_PROTO_RCMM24: RC-MM protocol 24 bits
 * @RC_PROTO_RCMM32: RC-MM protocol 32 bits
 * @RC_PROTO_XBOX_DVD: Xbox DVD Movie Playback Kit protocol
 * @RC_PROTO_MAX: Maximum value of enum rc_proto
 */
enum rc_proto {
	RC_PROTO_UNKNOWN	= 0,
	RC_PROTO_OTHER		= 1,
	RC_PROTO_RC5		= 2,
	RC_PROTO_RC5X_20	= 3,
	RC_PROTO_RC5_SZ		= 4,
	RC_PROTO_JVC		= 5,
	RC_PROTO_SONY12		= 6,
	RC_PROTO_SONY15		= 7,
	RC_PROTO_SONY20		= 8,
	RC_PROTO_NEC		= 9,
	RC_PROTO_NECX		= 10,
	RC_PROTO_NEC32		= 11,
	RC_PROTO_SANYO		= 12,
	RC_PROTO_MCIR2_KBD	= 13,
	RC_PROTO_MCIR2_MSE	= 14,
	RC_PROTO_RC6_0		= 15,
	RC_PROTO_RC6_6A_20	= 16,
	RC_PROTO_RC6_6A_24	= 17,
	RC_PROTO_RC6_6A_32	= 18,
	RC_PROTO_RC6_MCE	= 19,
	RC_PROTO_SHARP		= 20,
	RC_PROTO_XMP		= 21,
	RC_PROTO_CEC		= 22,
	RC_PROTO_IMON		= 23,
	RC_PROTO_RCMM12		= 24,
	RC_PROTO_RCMM24		= 25,
	RC_PROTO_RCMM32		= 26,
	RC_PROTO_XBOX_DVD	= 27,
	RC_PROTO_MAX		= RC_PROTO_XBOX_DVD,
};

#endif
