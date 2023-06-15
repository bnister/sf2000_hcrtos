#ifndef __POK_H
#define __POK_H
// #include "stdint-uintn.h"

#include <hcuapi/iocbase.h>

#define POK_OUT33_LEVEL	0x20000
#define POK_FALL33_INT_FLAG 0X800
#define POK_RISE33_INT_FLAG 0x400

//幻数
//定义命令
#define POK_READ _IOR(POK_IOCBASE, 2, int)

#endif // !__POK_H 