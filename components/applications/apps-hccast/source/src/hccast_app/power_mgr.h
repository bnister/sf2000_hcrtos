/*
power_mgr.h
 */

#ifndef __POWER_MGR_H__
#define __POWER_MGR_H__

#ifdef __cplusplus
extern "C" {
#endif

int power_enter_standby(void);
int power_reboot(void);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
