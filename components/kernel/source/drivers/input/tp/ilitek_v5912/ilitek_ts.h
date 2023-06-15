#ifndef __ILITEK_TS_H__
#define __ILITEK_TS_H__

#define ILITEK_BL_ADDR				0x41

#define ILITEK_PLAT_QCOM			1
#define ILITEK_PLAT_MTK				2
#define ILITEK_PLAT_ROCKCHIP			3
#define ILITEK_PLAT_ALLWIN			4
#define ILITEK_PLAT_AMLOGIC			5
#define ILITEK_PLAT ILITEK_PLAT_QCOM
//#define CONFIG_QCOM_DRM
#if ILITEK_PLAT == ILITEK_PLAT_QCOM
#ifdef CONFIG_QCOM_DRM
#include <linux/msm_drm_notify.h>
#define ILITEK_BLANK_POWERDOWN			MSM_DRM_BLANK_POWERDOWN
#define ILITEK_BLANK_UNBLANK			MSM_DRM_BLANK_UNBLANK
#define ILITEK_EVENT_BLANK			MSM_DRM_EVENT_BLANK
#define ILITEK_BLANK_NORMAL			MSM_DRM_BLANK_UNBLANK
#endif
#endif

#define ILITEK_GESTURE_TYPES			\
	X(Disable, 	0, "disable")		\
	X(Single_Click, 1, "single-click")	\
	X(Double_Click, 2, "double-click")
#define ILITEK_GESTURE_DEFAULT			Gesture_Disable

#define X(_type, _id, _str)	Gesture_##_type = _id,
enum Gesture_Type {
	ILITEK_GESTURE_TYPES
};
#undef X

#define ILITEK_LOW_POWER_TYPES		\
	X(Sleep,	0, "sleep")	\
	X(Idle,		1, "idle")	\
	X(PowerOff,	2, "poweroff")
#define ILITEK_LOW_POWER_DEFAULT		Low_Power_Sleep

#define X(_type, _id, _str)	Low_Power_##_type = _id,
enum Low_Power_Type {
	ILITEK_LOW_POWER_TYPES
};
#undef X

#define ILITEK_TOUCH_PROTOCOL_B
//#define ILITEK_REPORT_PRESSURE
#define ILITEK_USE_LCM_RESOLUTION
//#define ILITEK_ISR_PROTECT

#define ILITEK_TUNING_MESSAGE
#define ILITEK_REGISTER_SUSPEND_RESUME
#define ILITEK_ESD_CHECK_ENABLE			0

#define ILITEK_TOOL

#define ILITEK_ROTATE_FLAG			0
#define ILITEK_REVERT_X				0
#define ILITEK_REVERT_Y				0
#define TOUCH_SCREEN_X_MAX			1080	//LCD_WIDTH
#define TOUCH_SCREEN_Y_MAX			1920	//LCD_HEIGHT
#define ILITEK_RESOLUTION_MAX			16384
//#define ILITEK_ENABLE_REGULATOR_POWER_ON
#define ILITEK_GET_GPIO_NUM

#define ILITEK_GET_TIME_FUNC_WITH_TIME		0
#define ILITEK_GET_TIME_FUNC_WITH_JIFFIES	1
#define ILITEK_GET_TIME_FUNC			ILITEK_GET_TIME_FUNC_WITH_JIFFIES

#define DOUBLE_CLICK_DISTANCE			1000
#define DOUBLE_CLICK_ONE_CLICK_USED_TIME	800
#define DOUBLE_CLICK_NO_TOUCH_TIME		1000
#define DOUBLE_CLICK_TOTAL_USED_TIME		(DOUBLE_CLICK_NO_TOUCH_TIME + (DOUBLE_CLICK_ONE_CLICK_USED_TIME * 2))

#define ILITEK_TS_NAME				"ilitek_ts"

#define ABSSUB(a, b) ((a > b) ? (a - b) : (b - a))

#endif
