#ifndef _HCUAPI_SCI_H_
#define _HCUAPI_SCI_H_

#include <hcuapi/iocbase.h>

#define SCIIOC_SET_HIGH_SPEED          _IO (SCI_IOCBASE, 0)
#define SCIIOC_SET_NORMAL_SPEED        _IO (SCI_IOCBASE, 1)

#define SCIIOC_SET_BAUD_RATE_115200    _IO (SCI_IOCBASE, 2)
#define SCIIOC_SET_BAUD_RATE_57600     _IO (SCI_IOCBASE, 3)
#define SCIIOC_SET_BAUD_RATE_19200     _IO (SCI_IOCBASE, 4)
#define SCIIOC_SET_BAUD_RATE_9600      _IO (SCI_IOCBASE, 5)

#define SCIIOC_SET_BAUD_RATE_1125000   _IO (SCI_IOCBASE, 7)
#define SCIIOC_SET_BAUD_RATE_921600    _IO (SCI_IOCBASE, 8)
#define SCIIOC_SET_BAUD_RATE_675000    _IO (SCI_IOCBASE, 9)

#define SCIIOC_SET_SETTING             _IOW (SCI_IOCBASE, 6, struct sci_setting)

enum sci_bits_mode {
	bits_mode_default,	// 1 stop bit and 8 data bits
	bits_mode1,	// 1.5 stop bits and 5 data bits
	bits_mode2,	// 2 stop bits and 6 data bits
	bits_mode3,	// 2 stop bits and 7 data bits
	bits_mode4,	// 2 stop bits and 8 data bits
};

enum sci_parity_mode{
	PARITY_EVEN,
	PARITY_ODD,
	PARITY_NONE,
};

struct sci_setting{
	enum sci_bits_mode bits_mode;
	enum sci_parity_mode parity_mode;
};

#endif	/* _HCUAPI_SCI_H_ */
