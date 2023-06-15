/*
tv_sys.h
 */
#ifndef __TV_SYS_H__
#define __TV_SYS_H__

#ifdef __cplusplus
extern "C" {
#endif

int tv_sys_app_sys_to_de_sys(int app_tv_sys);
int tv_sys_app_set(int app_tv_sys);
int tv_sys_app_auto_set(int app_tv_sys, uint32_t timeout);
int tv_sys_best_tv_type_get(void);
int tv_sys_app_start_set(int check);

#ifdef __cplusplus
} /*extern "C"*/
#endif //

#endif
