#ifndef _RN6752V1_INITABLE_1080P25H_
#define _RN6752V1_INITABLE_1080P25H_

#include "../dvp.h"

struct regval_list RN6752V1_init_cfg_1080P25[] = {
    // 1080P@25 BT656
    // Slave address is 0x58
    // Register, data

    // if clock source(Xin) of RN6752 is 26MHz, please add these procedures marked first
    //0xD2, 0x85, // disable auto clock detect
    //0xD6, 0x37, // 27MHz default
    //0xD8, 0x18, // switch to 26MHz clock
    //delay(100), // delay 100ms

    {0x81, 0x01}, // turn on video decoder
    {0xA3, 0x04}, //
    {0xDF, 0xFE}, // enable HD format
    {0xF0, 0xC0},

    {0x88, 0x40}, // disable SCLK0B out
    {0xF6, 0x40}, // disable SCLK3A out

    // ch0
    {0xFF, 0x00}, // switch to ch0 (default; optional)
    {0x00, 0x20}, // internal use*
    {0x06, 0x08}, // internal use*
    {0x07, 0x63}, // HD format
    {0x2A, 0x01}, // filter control
    {0x3A, 0x00}, // No Insert Channel ID in SAV/EAV code
    {0x3F, 0x10}, // channel ID
    {0x4C, 0x37}, // equalizer
    {0x4F, 0x03}, // sync control
    {0x50, 0x03}, // 1080p resolution
    {0x56, 0x02}, // 144M and BT656 mode
    {0x5F, 0x44}, // blank level
    {0x63, 0xF8}, // filter control
    {0x59, 0x00}, // extended register access
    {0x5A, 0x48}, // data for extended register
    {0x58, 0x01}, // enable extended register write
    {0x59, 0x33}, // extended register access
    {0x5A, 0x23}, // data for extended register
    {0x58, 0x01}, // enable extended register write
    {0x51, 0xF4}, // scale factor1
    {0x52, 0x29}, // scale factor2
    {0x53, 0x15}, // scale factor3
    {0x5B, 0x01}, // H-scaling control
    {0x5E, 0x08}, // enable H-scaling control
    {0x6A, 0x87}, // H-scaling control
    {0x28, 0x92}, // cropping
    {0x03, 0x80}, // saturation
    {0x04, 0x80}, // hue
    {0x05, 0x04}, // sharpness
    {0x57, 0x23}, // black/white stretch
    {0x68, 0x00}, // coring
    {0x37, 0x33},
    {0x61, 0x6C},

    #ifdef RN6752V1_HDR_Configuration
    {0x1D, 0xFF},
    {0x28, 0x90},
    {0x2D, 0xFA},
    {0x4E, 0x01},
    {0x67, 0x66},
    {0x0B, 0x80},
    {0x09, 0x00},
    {0x0D, 0x18},
    {0x49, 0x84},
    {0x57, 0x63},
    {0x59, 0x20},
    {0x5A, 0x64},
    {0x58, 0x01},
    #endif

    {0x8E, 0x00}, // single channel output for VP
    {0x8F, 0x80}, // 1080p mode for VP
    {0x8D, 0x31}, // enable VP out
    {0x89, 0x0A}, // select 144MHz for SCLK
    {0x88, 0x41}, // enable SCLK out

    {0xFF, 0xFF}
};

#endif
