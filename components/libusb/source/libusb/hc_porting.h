#ifndef __HC_PORINT_H__
#define __HC_PORINT_H__

#undef container_of
#undef list_next_entry
#undef list_for_each_entry
#undef list_for_each_entry_safe

#include <stdio.h>

#undef LOG_TAG
#define LOG_TAG "libusb"
#undef ELOG_OUTPUT_LVL
#define ELOG_OUTPUT_LVL ELOG_LVL_DEBUG
#include <kernel/elog.h>

#include <time.h>
#include <linux/usb.h>

#endif // !__HC_PORINT_H__
