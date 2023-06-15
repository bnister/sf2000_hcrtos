#ifndef _HCUAPI_MIPI_H_
#define _HCUAPI_MIPI_H_

#include <hcuapi/iocbase.h>

#define MIPI_DSI_SET_ON					_IO (MIPI_IOCBASE, 0)
#define MIPI_DSI_SET_OFF				_IO (MIPI_IOCBASE, 1)
#define MIPI_DSI_SET_FORMAT				_IO (MIPI_IOCBASE, 2)		//<! enum MIPI_LCD_COLOR_MODE
#define MIPI_DSI_SET_CFG				_IO (MIPI_IOCBASE, 3)		//<! u8 config: [0, 3f]
#define MIPI_DSI_INIT					_IO (MIPI_IOCBASE, 4)

#define MIPI_DSI_DCS_READ				_IOWR (MIPI_IOCBASE, 5, struct hc_dcs_read)
#define MIPI_DSI_DCS_WRITE				_IOW (MIPI_IOCBASE, 6, struct hc_dcs_write)
#define MIPI_DSI_SEND_DCS_CMDS				_IOW (MIPI_IOCBASE, 7, struct hc_dcs_cmds)
#define MIPI_DSI_SEND_DCS_INIT_SEQUENCE			_IO (MIPI_IOCBASE, 8)		//<! the init sequence is set in DTS
#define MIPI_DSI_TIMING_INIT				_IO (MIPI_IOCBASE, 9)
#define MIPI_DSI_SET_TIMING				_IOW (MIPI_IOCBASE, 10, struct mipi_display_timing)
#define MIPI_DSI_GET_TIMING				_IOR (MIPI_IOCBASE, 11, struct mipi_display_timing)
#define MIPI_DSI_GPIO_ENABLE				_IO (MIPI_IOCBASE, 12)		//!< param: value: 0:disabled 1:enable

/*
 * The AMP RPC can only hold maximum 4096 bytes of args
 * Except for the RPC arg header, the maximum buffer for
 * extra data is limit to 4032 bytes
 */
#define MIPI_MAX_PACKET_SIZE 4032
#define MIPI_MAX_WRITE_SIZE 256

struct hc_dcs_write {
	unsigned char type;
	unsigned char len;
	unsigned char delay_ms;
	unsigned char payload[MIPI_MAX_WRITE_SIZE];
};

struct hc_dcs_read {
	unsigned char type;
	unsigned char command[2];
	unsigned char delay_ms;		//!< delay time after read operation
	unsigned char received_size;
	unsigned char received_data[3];
};

struct hc_dcs_cmds {
	unsigned int count;
	unsigned char packet[MIPI_MAX_PACKET_SIZE];
};

typedef enum MIPI_LCD_COLOR_MODE
{
	MIPI_COLOR_RGB565_0,
	MIPI_COLOR_RGB565_1,
	MIPI_COLOR_RGB565_2,
	MIPI_COLOR_RGB666_0,
	MIPI_COLOR_RGB666_1,
	MIPI_COLOR_RGB888,
} mipi_lcd_color_mode_e;

struct mipi_display_timing
{
	unsigned int clk_frequency;
	unsigned int h_total_len;
	unsigned int v_total_len;
	unsigned int h_active_len;
	unsigned int v_active_len;
	unsigned int h_front_len;
	unsigned int h_sync_len;
	unsigned int h_back_len;
	unsigned int v_front_len;
	unsigned int v_sync_len;
	unsigned int v_back_len;
	unsigned int packet_cfg;
	unsigned char pclk;			//!< set mipi output-clock
	unsigned char lane_num;
	mipi_lcd_color_mode_e color_mode;
	unsigned char h_sync_level;
	unsigned char v_sync_level;
};

#endif
