#ifndef _BLUETOOTH_H
#define _BLUETOOTH_H

#define BLUETOOTH_MAC_LEN 6
#define BLUETOOTH_NAME_LEN 128

struct bluetooth_slave_dev {
	unsigned char mac[BLUETOOTH_MAC_LEN];
	char name[BLUETOOTH_NAME_LEN];
};

#define BT_RET_SUCCESS 0
#define BT_RET_ERROR   -1

#define BLUETOOTH_EVENT_SLAVE_DEV_SCANNED		        0	/* param = (struct bluetooth_slave_dev *) param */
#define BLUETOOTH_EVENT_SLAVE_DEV_SCAN_FINISHED	    	1	/* param = 0 */
#define BLUETOOTH_EVENT_SLAVE_DEV_DISCONNECTED		    2	/* param = 0 */
#define BLUETOOTH_EVENT_SLAVE_DEV_CONNECTED		    	3	/* param = 0 */
#define BLUETOOTH_EVENT_SLAVE_DEV_GET_CONNECTED_INFO 	4	/* param = (struct bluetooth_slave_dev *) param */
struct input_event_bt {
	uint16_t type;
	uint16_t code;
	int32_t value;
};

typedef int (*bluetooth_callback_t)(unsigned long event, unsigned long param);
typedef int (*bluetooth_ir_control_t)(struct input_event_bt event);

int bluetooth_init(const char *uart_path, bluetooth_callback_t callback);
int bluetooth_deinit(void);
int bluetooth_poweron(void);
int bluetooth_poweroff(void);
int bluetooth_scan(void);
int bluetooth_stop_scan(void);
int bluetooth_connect(unsigned char *mac);
int bluetooth_is_connected(void);
int bluetooth_disconnect(void);
int bluetooth_set_music_vol(unsigned char val);
int bluetooth_memory_connection(unsigned char value);//0 memory connection 1 no memory connection
int bluetooth_set_gpio_backlight(unsigned char value);//0 disable 1 enable
int bluetooth_set_gpio_mutu(unsigned char value);//0 disable 1 enable
int bluetooth_set_cvbs_aux_mode(void);
int bluetooth_set_cvbs_fiber_mode(void);
int bluetooth_del_all_device(void);
int bluetooth_del_list_device(void);
int bluetooth_set_connection_cvbs_aux_mode(void);
int bluetooth_set_connection_cvbs_fiber_mode(void);
int bluetooth_ir_key_init(bluetooth_ir_control_t control);
#endif
