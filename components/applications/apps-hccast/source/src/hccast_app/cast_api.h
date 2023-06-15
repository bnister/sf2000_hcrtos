/**
 * @file cast_api.h
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2022-01-20
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#ifndef __CAST_API_H__
#define __CAST_API_H__

#ifdef __cplusplus
extern "C" {
#endif



typedef enum{
    CAST_TYPE_AIRCAST = 0,
    CAST_TYPE_DLNA,
    CAST_TYPE_MIRACAST,

    CAST_TYPE_NONE,
}cast_type_t;

void cast_airpaly_open(void);
void cast_airpaly_close(void);

void cast_dlna_open(void);
void cast_dlna_close(void);

void cast_miracast_open(void);
void cast_miracast_close(void);

int cast_get_service_name(cast_type_t cast_type, char *service_name, int length);


#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif



