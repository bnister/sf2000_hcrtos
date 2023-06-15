#ifndef _MI_COMMON_H_
#define _MI_COMMON_H_

#include <stdint.h>
#include <stdbool.h>

#define MI_GFX_INITIAL_ERROR_CODE 1
#define E_MI_GFX_ERR_SUCCESS 0


#ifndef MI_S32 
typedef int32_t MI_S32;
#endif

#ifndef MI_U8
typedef uint8_t MI_U8;
#endif

#ifndef MI_U16
typedef uint16_t MI_U16;
#endif

#ifndef MI_U32 
typedef uint32_t MI_U32;
#endif

#ifndef MI_BOOL
typedef bool MI_BOOL;
#endif

#ifndef MI_PHY_
typedef void* MI_PHY;
#endif


#endif
