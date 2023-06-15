#include <kernel/module.h>
#include <sys/unistd.h>
#include <errno.h>
#include <kernel/lib/fdt_api.h>
#include <kernel/ld.h>
#include <linux/jiffies.h>
#include <stdio.h>
#include <nuttx/fs/fs.h>

#include <errno.h>
#include <nuttx/fs/fs.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/printk.h>
#include <kernel/lib/fdt_api.h>
#include <linux/workqueue.h>
#include <hcuapi/gpio.h>

#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <stddef.h>
#include <fcntl.h>
#include <stdint.h>
#include <hcuapi/i2c-master.h>

#include "ilitek_common.h"

int ilitek_log_level_value = ILITEK_DEFAULT_LOG_LEVEL;
struct ilitek_ts_data *ts;

int event_spacing;
static uint8_t finger_state;
static int start_x;
static int start_y;
static int current_x;
static int current_y;

#if ILITEK_GET_TIME_FUNC == ILITEK_GET_TIME_FUNC_WITH_TIME
static struct timeval start_event_time;
#else
unsigned long start_event_time_jiffies;
#endif

static int ilitek_i2c_write_and_read(uint8_t *cmd, int w_len, int delay_ms,
				     uint8_t *buf, int r_len)
{
	int error;
	/*
	 * Default ILITEK_BL_ADDR. is firstly used.
	 * if communication failed, change between BL addr. and
	 * other addr. defined by DTS, then retry.
	 */
	static unsigned short addr = 0x41;
	struct i2c_transfer_s xfer_read;
	struct i2c_msg_s msgs[2] = {
		{ .addr = addr, .flags = 0, .length = w_len, .buffer = cmd },
		{ .addr = addr, .flags = 0x1, .length = r_len, .buffer = buf }
	};

	xfer_read.msgv = msgs;
	xfer_read.msgc = 2;
	/*
	 * IMPORTANT: If I2C repeat start is required, please check with ILITEK.
	 */
	if (w_len > 0 && r_len > 0 && !delay_ms) {
		//if (ilitek_i2c_transfer(msgs, 2) < 0) {
		if (ioctl(ts->fd, I2CIOC_TRANSFER, &xfer_read) < 0) {
			/* try another i2c addr. (default: 0x41) */
			msgs[0].addr = msgs[1].addr = addr;

			//return ilitek_i2c_transfer(msgs, 2);
			return ioctl(ts->fd, I2CIOC_TRANSFER, &xfer_read);
		}

		return 0;
	}

	xfer_read.msgc = 1;
	//if (w_len > 0 && ilitek_i2c_transfer(msgs, 1) < 0) {
	if (w_len > 0 && ioctl(ts->fd, I2CIOC_TRANSFER, &xfer_read) < 0) {
		msgs[0].addr = msgs[1].addr = addr;

		if ((error = ioctl(ts->fd, I2CIOC_TRANSFER, &xfer_read)) < 0)
			return error;
	}

	if (delay_ms > 0)
		usleep(delay_ms);

	xfer_read.msgv = msgs + 1;
	xfer_read.msgc = 1;
	//if (r_len > 0 && ilitek_i2c_transfer(msgs + 1, 1) < 0) {
	if (r_len > 0 && ioctl(ts->fd, I2CIOC_TRANSFER, &xfer_read) < 0) {
		msgs[0].addr = msgs[1].addr = addr;

		return ioctl(ts->fd, I2CIOC_TRANSFER, &xfer_read);
	}

	return 0;
}

static int __maybe_unused ilitek_i2c_write(uint8_t *cmd, int len)
{
	return ilitek_i2c_write_and_read(cmd, len, 0, NULL, 0);
}

static int __maybe_unused ilitek_i2c_read(uint8_t *buf, int len)
{
	return ilitek_i2c_write_and_read(NULL, 0, 0, buf, len);
}

int ilitek_read(uint8_t *buf, int len)
{
	int error;

	error = ilitek_i2c_read(buf, len);

	return (error < 0) ? error : 0;
}

int ilitek_write_and_read(uint8_t *cmd, int w_len, int delay_ms, uint8_t *buf,
			  int r_len)
{
	int error;

	error = ilitek_i2c_write_and_read(cmd, w_len, delay_ms, buf, r_len);

	return (error < 0) ? error : 0;
}

static int _ilitek_write_then_read(unsigned char *wbuf, int wlen,
				   unsigned char *rbuf, int rlen, void *data)
{
	return ilitek_write_and_read(wbuf, wlen, 1, rbuf, rlen);
}

static void _ilitek_delay(unsigned int delay_ms)
{
	usleep(delay_ms * 1000);
}

static int _ilitek_wait_ack(uint8_t cmd, unsigned int tout_ms, void *data)
{
#if 0
	unsigned int t_ms = 0;
	int tmp, error = -ETIME;

	UNUSED(cmd);

	do {
		if ((tmp = atomic_read(&ts->get_INT))) {
			error = 0;
			break;
		}

		udelay(1000);
		t_ms++;
	} while (t_ms < tout_ms);

	ts->unhandle_irq = false;
	ilitek_irq_disable();

	return error;
#else 
	usleep(10000);

	return 0;
#endif
}

struct ilitek_ts_callback dev_cb = {
	.write_then_read = _ilitek_write_then_read,
	.read_interrupt_in = NULL,
	.init_ack = NULL,
	.wait_ack = _ilitek_wait_ack,
	.hw_reset = NULL,
	.re_enum = NULL,
	.delay_ms = _ilitek_delay,
	.msg = NULL,
};

static int ilitek_check_key_down(int x, int y)
{
	int j;

	for (j = 0; j < ts->dev->tp_info.key_num; j++) {
		if ((x >= ts->dev->key.info.keys[j].x &&
		     x <= ts->dev->key.info.keys[j].x +
			ts->dev->key.info.x_len) &&
		    (y >= ts->dev->key.info.keys[j].y &&
		     y <= ts->dev->key.info.keys[j].y +
			ts->dev->key.info.y_len)) {
#if ILITEK_PLAT != ILITEK_PLAT_MTK
			input_report_key(ts->input_dev, ts->dev->key.info.keys[j].id, 1);
#else
#ifndef MTK_UNDTS
			if (tpd_dts_data.use_tpd_button) {
				x = tpd_dts_data.tpd_key_dim_local[j].key_x;
				y = tpd_dts_data.tpd_key_dim_local[j].key_y;
				tp_dbg("key index=%x, tpd_dts_data.tpd_key_local[%d]=%d key down\n",
					j, j, tpd_dts_data.tpd_key_local[j]);
				ilitek_touch_down(0, x, y, 10, 128, 1);
			}
#else
			x = touch_key_point_maping_array[j].point_x;
			y = touch_key_point_maping_array[j].point_y;
			ilitek_touch_down(0, x, y, 10, 128, 1);
#endif
#endif
			ts->dev->key.clicked[j] = true;
			ts->touch_key_hold_press = true;
			ts->is_touched = true;
			tp_dbg("Key, Keydown ID=%d, X=%d, Y=%d, key_status=%d\n",
				ts->dev->key.info.keys[j].id, x, y,
				ts->dev->key.clicked[j]);
			break;
		}
	}
	return 0;
}

static int ilitek_check_key_release(int x, int y, int check_point)
{
	int j = 0;

	for (j = 0; j < ts->dev->tp_info.key_num; j++) {
		if (!ts->dev->key.clicked[j])
			continue;

		if (check_point) {
			if (x < ts->dev->key.info.keys[j].x ||
			    x > ts->dev->key.info.keys[j].x + ts->dev->key.info.x_len ||
			    y < ts->dev->key.info.keys[j].y ||
			    y > ts->dev->key.info.keys[j].y + ts->dev->key.info.y_len) {
#if ILITEK_PLAT != ILITEK_PLAT_MTK
				input_report_key(ts->input_dev,
						 ts->dev->key.info.keys[j].id, 0);
#else
#if 0
#ifndef MTK_UNDTS
				if (tpd_dts_data.use_tpd_button) {
					tp_dbg("key index=%x, tpd_dts_data.tpd_key_local[%d]=%d key up\n", j, j, tpd_dts_data.tpd_key_local[j]);
					ilitek_touch_release(0);
				}
#else
				ilitek_touch_release(0);
#endif
#endif
				ilitek_touch_release(0);
#endif
				ts->dev->key.clicked[j] = false;
				ts->touch_key_hold_press = false;
				tp_dbg("Key, Keyout ID=%d, X=%d, Y=%d, key_status=%d\n",
					ts->dev->key.info.keys[j].id, x, y,
					ts->dev->key.clicked[j]);
				break;
			}
		} else {
#if ILITEK_PLAT != ILITEK_PLAT_MTK
			input_report_key(ts->input_dev, ts->dev->key.info.keys[j].id, 0);
#else
#if 0
#ifndef MTK_UNDTS
			if (tpd_dts_data.use_tpd_button) {
				tp_dbg("key index=%x, tpd_dts_data.tpd_key_local[%d]=%d key up\n", j, j, tpd_dts_data.tpd_key_local[j]);
				ilitek_touch_release(0);
			}
#else
			ilitek_touch_release(0);
#endif
#endif
			ilitek_touch_release(0);
#endif
			ts->dev->key.clicked[j] = false;
			ts->touch_key_hold_press = false;
			tp_dbg("Key, Keyout ID=%d, X=%d, Y=%d, key_status=%d\n",
				ts->dev->key.info.keys[j].id, x, y,
				ts->dev->key.clicked[j]);
			break;
		}
	}
	return 0;
}

static int ilitek_touch_down(int id, int x, int y, int p, int h, int w)
{
	struct input_dev *input = ts->input_dev;

#ifdef ILITEK_USE_LCM_RESOLUTION
	x = (x - ts->dev->screen_info.x_min) * TOUCH_SCREEN_X_MAX /
		(ts->dev->screen_info.x_max - ts->dev->screen_info.x_min);
	y = (y - ts->dev->screen_info.y_min) * TOUCH_SCREEN_Y_MAX /
		(ts->dev->screen_info.y_max - ts->dev->screen_info.y_min);
	tp_dbg("Resolution Touch id=%02X, x: %04d, y: %04d\n", id, x, y);
#endif

	input_report_key(input, BTN_TOUCH, 1);
#ifdef ILITEK_TOUCH_PROTOCOL_B
	input_mt_slot(input, id);
	input_mt_report_slot_state(input, MT_TOOL_FINGER, true);
#endif
#if !ILITEK_ROTATE_FLAG
	input_event(input, EV_ABS, ABS_MT_POSITION_X, x);
	input_event(input, EV_ABS, ABS_MT_POSITION_Y, y);
#else
	input_event(input, EV_ABS, ABS_MT_POSITION_X, y);
	input_event(input, EV_ABS, ABS_MT_POSITION_Y, x);
#endif
	input_event(input, EV_ABS, ABS_MT_TOUCH_MAJOR, h);
	input_event(input, EV_ABS, ABS_MT_WIDTH_MAJOR, w);
#ifdef ILITEK_REPORT_PRESSURE
	input_event(input, EV_ABS, ABS_MT_PRESSURE, p);
#endif
#ifndef ILITEK_TOUCH_PROTOCOL_B
	input_event(input, EV_ABS, ABS_MT_TRACKING_ID, id);
	input_mt_sync(input);
#endif

#if ILITEK_PLAT == ILITEK_PLAT_MTK
#ifdef CONFIG_MTK_BOOT
#ifndef MTK_UNDTS
	if (tpd_dts_data.use_tpd_button) {
		if (FACTORY_BOOT == get_boot_mode() || RECOVERY_BOOT == get_boot_mode()) {
			tpd_button(x, y, 1);
			tp_dbg("tpd_button(x, y, 1) = tpd_button(%d, %d, 1)\n", x, y);
		}
	}
#endif
#endif
#endif
	return 0;
}

static int ilitek_touch_release(int id)
{
	struct input_dev *input = ts->input_dev;

#ifdef ILITEK_TOUCH_PROTOCOL_B
	if (ts->touch_flag[id] == 1) {
		tp_dbg("release point id = %d\n", id);
		input_mt_slot(input, id);
		input_mt_report_slot_state(input, MT_TOOL_FINGER, false);
	}
#else
	input_report_key(input, BTN_TOUCH, 0);
	input_mt_sync(input);
#endif
	set_arr(ts->touch_flag, id, 0);

#if ILITEK_PLAT == ILITEK_PLAT_MTK
#ifdef CONFIG_MTK_BOOT
#ifndef MTK_UNDTS
	if (tpd_dts_data.use_tpd_button) {
		if (FACTORY_BOOT == get_boot_mode() ||
		    RECOVERY_BOOT == get_boot_mode()) {
			tpd_button(0, 0, 0);
			tp_dbg("tpd_button(x, y, 0) = tpd_button(0, 0, 0)\n");
		}
	}
#endif
#endif
#endif

	return 0;
}

static int ilitek_touch_release_all_point(void)
{
	struct input_dev *input = ts->input_dev;
	int i = 0;

#ifdef ILITEK_TOUCH_PROTOCOL_B
	input_report_key(input, BTN_TOUCH, 0);
	for (i = 0; i < ts->dev->tp_info.max_fingers; i++)
		ilitek_touch_release(i);
#else
	for (i = 0; i < ts->dev->tp_info.max_fingers; i++)
		set_arr(ts->touch_flag, i, 0);
	ilitek_touch_release(0);
#endif
	ts->is_touched = false;
	input_sync(input);
	return 0;
}

static int ilitek_get_time_diff(void)
{
	int diff_milliseconds = 0;
#if ILITEK_GET_TIME_FUNC == ILITEK_GET_TIME_FUNC_WITH_TIME
	struct timeval time_now;

	do_gettimeofday(&time_now);
	diff_milliseconds += (time_now.tv_sec - start_event_time.tv_sec) * 1000;

	if (time_now.tv_usec < start_event_time.tv_usec) {
		diff_milliseconds -= 1000;
		diff_milliseconds += (1000 * 1000 + time_now.tv_usec - start_event_time.tv_usec) / 1000;
	} else
		diff_milliseconds += (time_now.tv_usec - start_event_time.tv_usec) / 1000;

	if (diff_milliseconds < (-10000))
		diff_milliseconds = 10000;
	tp_msg("time_now.tv_sec = %d start_event_time.tv_sec = %d time_now.tv_usec = %d start_event_time.tv_usec = %d diff_milliseconds = %d\n",
			(int)time_now.tv_sec, (int)start_event_time.tv_sec, (int)time_now.tv_usec, (int)start_event_time.tv_usec, diff_milliseconds);
#else
	diff_milliseconds = jiffies_to_msecs(jiffies) - jiffies_to_msecs(start_event_time_jiffies);
	tp_msg("jiffies_to_msecs(jiffies) = %lu jiffies_to_msecs(start_event_time_jiffies) = %lu diff_milliseconds = %d\n", jiffies_to_msecs(jiffies),
			jiffies_to_msecs(start_event_time_jiffies), diff_milliseconds);
#endif
	return diff_milliseconds;
}

static uint8_t ilitek_double_click_touch(int finger_id, int x, int y,
					 uint8_t finger_state)
{
	tp_msg("start finger_state = %d\n", finger_state);
	if (finger_id > 0) {
		finger_state = 0;
		goto out;
	}
	if (finger_state == 0 || finger_state == 5) {

		finger_state = 1;
		start_x = x;
		start_y = y;
		current_x = 0;
		current_y = 0;
		event_spacing = 0;
#if ILITEK_GET_TIME_FUNC == ILITEK_GET_TIME_FUNC_WITH_TIME
		do_gettimeofday(&start_event_time);
#else
		start_event_time_jiffies = jiffies;
#endif
	} else if (finger_state == 1) {
		event_spacing = ilitek_get_time_diff();
		if (event_spacing > DOUBLE_CLICK_ONE_CLICK_USED_TIME)
			finger_state = 4;
	} else if (finger_state == 2) {
		finger_state = 3;
		current_x = x;
		current_y = y;
		event_spacing = ilitek_get_time_diff();
		if (event_spacing > (DOUBLE_CLICK_ONE_CLICK_USED_TIME + DOUBLE_CLICK_NO_TOUCH_TIME))
			finger_state = 0;
	} else if (finger_state == 3) {
		current_x = x;
		current_y = y;
		event_spacing = ilitek_get_time_diff();
		if (event_spacing > DOUBLE_CLICK_TOTAL_USED_TIME) {
			start_x = current_x;
			start_y = current_y;
			finger_state = 4;
		}
	}
out:
	tp_msg("finger_state = %d event_spacing = %d\n", finger_state, event_spacing);
	return finger_state;
}

static uint8_t ilitek_double_click_release(uint8_t finger_state)
{
	tp_msg("start finger_state = %d\n", finger_state);
	if (finger_state == 1) {
		finger_state = 2;
		event_spacing = ilitek_get_time_diff();
		if (event_spacing > DOUBLE_CLICK_ONE_CLICK_USED_TIME)
			finger_state = 0;
	}
	if (finger_state == 3) {
		event_spacing = ilitek_get_time_diff();
		if ((event_spacing < DOUBLE_CLICK_TOTAL_USED_TIME && event_spacing > 50) && (ABSSUB(current_x, start_x) < DOUBLE_CLICK_DISTANCE)
				&& ((ABSSUB(current_y, start_y) < DOUBLE_CLICK_DISTANCE))) {
			finger_state = 5;
			goto out;
		} else
			finger_state = 0;
	} else if (finger_state == 4)
		finger_state = 0;
out:
	tp_msg("finger_state = %d event_spacing = %d\n", finger_state, event_spacing);
	return finger_state;
}

void __maybe_unused ilitek_gesture_handle(bool touch, int idx, int x, int y)
{
	struct input_dev *input = ts->input_dev;

	if (ts->gesture_status == Gesture_Double_Click) {
		if (touch) {
			finger_state = ilitek_double_click_touch(idx, x, y,
								 finger_state);
			return;
		}
		finger_state = ilitek_double_click_release(finger_state);

		if (finger_state != 5)
			return;
	}

#ifdef ILITEK_WAKELOCK_SUPPORT
	wake_lock_timeout(&ilitek_wake_lock, 5 * HZ);
#endif

	input_report_key(input, KEY_POWER, 1);
	input_sync(input);
	input_report_key(input, KEY_POWER, 0);
	input_sync(input);
}

static int ilitek_pen_handler(unsigned char *buf, int size)
{
	struct pen_fmt *parser;
	struct input_dev *pen_input = ts->pen_input_dev;
	static int curr_tool = BTN_TOOL_PEN;
	int tool;

	if (!pen_input)
		return -EINVAL;

	parser = (struct pen_fmt *)(buf + 1);

	tool = (parser->in_range && parser->invert) ? BTN_TOOL_RUBBER :
						      BTN_TOOL_PEN;

	tp_dbg("x: %u, y: %u, x_tilt: %u, y_tilt: %u, curr_tool: %d, tool: %d\n",
	       parser->x, parser->y, parser->x_tilt, parser->y_tilt, curr_tool,
	       tool);

	if (curr_tool != tool) {
		input_report_key(pen_input, curr_tool, 0);
		input_sync(pen_input);
		curr_tool = tool;
	}

	input_report_key(pen_input, BTN_TOUCH,
			 parser->tip_sw || parser->eraser);
	input_report_key(pen_input, curr_tool, parser->in_range);
	input_report_key(pen_input, BTN_STYLUS, parser->barrel_sw);
	input_event(pen_input, EV_ABS, ABS_X, parser->x);
	input_event(pen_input, EV_ABS, ABS_Y, parser->y);
	input_event(pen_input, EV_ABS, ABS_PRESSURE, parser->pressure);
	input_event(pen_input, EV_ABS, ABS_TILT_X, parser->x_tilt);
	input_event(pen_input, EV_ABS, ABS_TILT_Y, parser->y_tilt);

	input_sync(pen_input);

	return 0;
}

static bool is_checksum_matched(uint8_t _checksum, int start, int end,
				uint8_t *buf, int buf_size)
{
	uint8_t checksum;

	checksum = ~(get_checksum(start, end, buf, buf_size)) + 1;
	if (checksum != _checksum) {
		tp_err("[buf]: %*phD, checksum : %hhx/%hhx not matched\n",
		       end - start, buf + start, checksum, _checksum);
		return false;
	}

	return true;
}

static int ilitek_read_data_and_report_6XX(void)
{
	int ret = 0;
	int count = 0;
	int report_max_point = 6;
	int release_point = 0;
	int i = 0;
	struct input_dev *input = ts->input_dev;
	uint8_t tmp;
	int len = 0;
	int max_cnt = 0;
	struct touch_fmt *parser;
	uint8_t checksum;

	/*
	 * ISR may be activated after registering irq and
	 * before creating input_dev
	 */
	if (!input) {
		tp_err("input_dev is not registerred\n");
		return -EINVAL;
	}

	len = ts->dev->finger.size;
	max_cnt = ts->dev->finger.max_cnt;

	tp_dbg("format:%d, packet cnts: %u, packet len: %u\n",
		ts->dev->tp_info.format, ts->dev->finger.max_cnt,
		ts->dev->finger.size);

	for(i = 0; i < ts->dev->tp_info.max_fingers; i++)
		ts->tp[i].status = false;

	memset(ts->buf, 0, sizeof(ts->buf));

	switch (ts->irq_handle_type) {
	case irq_type_c_model:
		for (i = 0, count = 1; i < count; i++) {
			ilitek_read(ts->buf, ts->irq_read_len);
			//ilitek_udp_reply(ts->buf, ts->irq_read_len);
			count = ts->buf[ts->irq_read_len - 1];
		}

		return 0;
	case irq_type_debug:
		ret = ilitek_read(ts->buf, 64);

		if (ts->buf[0] == 0xDB) {
			//ilitek_udp_reply(ts->buf, 64);
			return 0;
		}
		break;
	case irq_type_normal:
	default:
		ret = ilitek_read(ts->buf, 64);
		break;
	}

	if (ret < 0) {
		if (ts->is_touched) {
			ilitek_touch_release_all_point();
			ilitek_check_key_release(0, 0, 0);
		}
		return ret;
	}

	/*
	 * Check checksum for I2C comms. debug.
	 */
	checksum = ~(get_checksum(0, 63, ts->buf, 64)) + 1;
	if (!(is_checksum_matched(ts->buf[63], 0, 63,
	    			  ts->buf, sizeof(ts->buf))))
		return 0;

	//ilitek_udp_reply(ts->buf, 64);
	//tp_dbg("[rbuf]: %*phD, len: %d\n", 64, ts->buf, 64);

	/* Pen Report ID */
	if (ts->buf[0] == 0x0C || ts->buf[0] == 0x0D)
		return ilitek_pen_handler(ts->buf, 64);

	report_max_point = ts->buf[61];
	if (report_max_point > ts->dev->tp_info.max_fingers) {
		tp_err("FW report max point:%d > panel information max point:%d\n",
			report_max_point, ts->dev->tp_info.max_fingers);
		return -EINVAL;
	}
	count = CEIL(report_max_point, max_cnt);
	for (i = 1; i < count; i++) {
		tmp = ts->buf[i * len * max_cnt];
		ret = ilitek_read(ts->buf + i * len * max_cnt, 64);
		if (ret < 0) {
			if (ts->is_touched) {
				ilitek_touch_release_all_point();
				ilitek_check_key_release(0, 0, 0);
			}
			return ret;
		}

		if (!(is_checksum_matched(ts->buf[i * len * max_cnt + 63],
					  0, 63, ts->buf + i * len * max_cnt,
					  64)))
			return 0;

		//ilitek_udp_reply(ts->buf + i * len * max_cnt, 64);
		//		tp_dbg("[rbuf]: %*phD, len: %d\n", 64,
		//			ts->buf + i * len * max_cnt, 64);
		ts->buf[i * len * max_cnt] = tmp;
	}

	for (i = 0; i < report_max_point; i++) {
		parser = (struct touch_fmt *)(ts->buf + i * len + 1);

		ts->tp[i].status = parser->status;
		ts->tp[i].id = parser->id;

		if (ts->tp[i].id >= ts->dev->tp_info.max_fingers) {
			tp_err("id: %d, limit: %hhu OOB\n", ts->tp[i].id,
				ts->dev->tp_info.max_fingers);
			continue;
		}

		if (!ts->tp[i].status) {
			release_point++;
#ifdef ILITEK_TOUCH_PROTOCOL_B
			ilitek_touch_release(ts->tp[i].id);
#endif
			continue;
		}

		set_arr(ts->touch_flag, ts->tp[i].id, 1);

		ts->tp[i].x = parser->x;
		ts->tp[i].y = parser->y;

		ts->tp[i].p = 10;
		ts->tp[i].w = 10;
		ts->tp[i].h = 10;

		switch (ts->dev->tp_info.format) {
		case touch_fmt_1:
			ts->tp[i].p = parser->p;
			break;
		case touch_fmt_2:
			ts->tp[i].w = parser->w;
			ts->tp[i].h = parser->h;
			break;
		case touch_fmt_3:
			ts->tp[i].p = parser->p;
			ts->tp[i].w = parser->w;
			ts->tp[i].h = parser->h;
			break;
		}

		tp_dbg("id: %d, x: %d, y: %d, p: %d, w: %d, h: %d\n",
			ts->tp[i].id, ts->tp[i].x, ts->tp[i].y,
			ts->tp[i].p, ts->tp[i].w, ts->tp[i].h);
		if (ts->system_suspend) {
			tp_msg("system is suspend not report point\n");
			ilitek_gesture_handle(true, i, ts->tp[i].x, ts->tp[i].y);
			continue;
		}

		if (!(ts->is_touched))
			ilitek_check_key_down(ts->tp[i].x, ts->tp[i].y);

		if (!(ts->touch_key_hold_press)) {
			if (ts->tp[i].x > ts->dev->screen_info.x_max ||
			    ts->tp[i].y > ts->dev->screen_info.y_max ||
			    ts->tp[i].x < ts->dev->screen_info.x_min ||
			    ts->tp[i].y < ts->dev->screen_info.y_min) {
				tp_err("Point[%d]: (%d, %d), Limit: (%d:%d, %d:%d) OOB\n",
					ts->tp[i].id, ts->tp[i].x, ts->tp[i].y,
					ts->dev->screen_info.x_min, ts->dev->screen_info.x_max,
					ts->dev->screen_info.y_min, ts->dev->screen_info.y_max);
			} else {
				ts->is_touched = true;
				if (ILITEK_REVERT_X)
					ts->tp[i].x = ts->dev->screen_info.x_max - ts->tp[i].x + ts->dev->screen_info.x_min;

				if (ILITEK_REVERT_Y)
					ts->tp[i].y = ts->dev->screen_info.y_max - ts->tp[i].y + ts->dev->screen_info.y_min;

				tp_dbg("Point[%02X]: X:%04d, Y:%04d, P:%d, H:%d, W:%d\n",
						ts->tp[i].id, ts->tp[i].x,ts->tp[i].y,
						ts->tp[i].p, ts->tp[i].h, ts->tp[i].w);
				ilitek_touch_down(ts->tp[i].id, ts->tp[i].x,
						  ts->tp[i].y, ts->tp[i].p,
						  ts->tp[i].h, ts->tp[i].w);
			}
		}
		if ((ts->touch_key_hold_press))
			ilitek_check_key_release(ts->tp[i].x, ts->tp[i].y, 1);
	}

	tp_dbg("release point counter: %d , report max point: %d\n",
		release_point, report_max_point);
	if (release_point == report_max_point) {
		if (ts->is_touched)
			ilitek_touch_release_all_point();

		ilitek_check_key_release(0, 0, 0);
		ts->is_touched = false;

		if (ts->system_suspend)
			ilitek_gesture_handle(false, 0, 0, 0);
	}
	input_sync(input);
	return 0;
}

static int ilitek_read_data_and_report_3XX(void)
{
	int ret = 0;
	int packet = 0;
	int report_max_point = 6;
	int release_point = 0;
	int tp_status = 0;
	int i = 0, x = 0, y = 0;
	struct input_dev *input = ts->input_dev;
	uint8_t mode_status;

	/*
	 * ISR may be activated after registering irq and
	 * before creating input_dev
	 */
	if (!input) {
		tp_err("input_dev is not registerred\n");
		return -EINVAL;
	}

	memset(ts->buf, 0, 64);
	ts->buf[0] = 0x10;
	ret = ilitek_write_and_read(ts->buf, 1, 0, ts->buf, 32);
	if (ret < 0) {
		tp_err("get touch information err\n");
		if (ts->is_touched) {
			ilitek_touch_release_all_point();
			ilitek_check_key_release(x, y, 0);
		}
		return ret;
	}

	mode_status = ts->buf[31];
	ts->buf[31] = 0;

	packet = ts->buf[0];
	if (packet == 2) {
		if ((ret = ilitek_read(ts->buf + 31, 20)) < 0) {
			tp_err("get touch information packet 2 err\n");
			if (ts->is_touched) {
				ilitek_touch_release_all_point();
				ilitek_check_key_release(x, y, 0);
			}
			return ret;
		}
		report_max_point = 10;
	}
	ts->buf[62] = mode_status;

	//ilitek_udp_reply(ts->buf, 64);
	//tp_dbg("[rbuf]: %*phD, len: %d\n", 64, ts->buf, 64);

	if (ts->buf[1] == 0x5F || ts->buf[0] == 0xDB) {
		tp_dbg("debug message return\n");
		return 0;
	}

	for (i = 0; i < report_max_point; i++) {
		tp_status = ts->buf[i * 5 + 1] >> 7;
		if (!tp_status) {
			release_point++;
#ifdef ILITEK_TOUCH_PROTOCOL_B
			ilitek_touch_release(i);
#endif
			continue;
		}

		set_arr(ts->touch_flag, i, 1);

		x = ((ts->buf[i * 5 + 1] & 0x3F) << 8) + ts->buf[i * 5 + 2];
		y = (ts->buf[i * 5 + 3] << 8) + ts->buf[i * 5 + 4];

		if (ts->system_suspend) {
			tp_msg("system is suspend not report point\n");
			ilitek_gesture_handle(true, i, x, y);
			continue;
		}
		if (!(ts->is_touched))
			ilitek_check_key_down(x, y);
		if (!(ts->touch_key_hold_press)) {
			if (x > ts->dev->screen_info.x_max || y > ts->dev->screen_info.y_max ||
			    x < ts->dev->screen_info.x_min || y < ts->dev->screen_info.y_min) {
				tp_err("Point[%d]: (%d, %d), Limit: (%d:%d, %d:%d) OOB\n",
					i, x, y, ts->dev->screen_info.x_min,
					ts->dev->screen_info.x_max,
					ts->dev->screen_info.y_min,
					ts->dev->screen_info.y_max);
				tp_err("Raw data: %*phD\n", 5,
					ts->buf + i * 5 + 1);
				continue;
			}
			ts->is_touched = true;
			if (ILITEK_REVERT_X)
				x = ts->dev->screen_info.x_max - x + ts->dev->screen_info.x_min;
			if (ILITEK_REVERT_Y)
				y = ts->dev->screen_info.y_max - y + ts->dev->screen_info.y_min;
#ifndef ILITEK_USE_LCM_RESOLUTION
			tp_dbg("Touch id=%02X, x: %04d, y: %04d\n", i, x, y);
#endif
			ilitek_touch_down(i, x, y, 10, 128, 1);
		}
	}
	tp_dbg("release point:  %d, packet: %d\n", release_point, packet);
	if (packet == 0 || release_point == report_max_point) {
		if (ts->is_touched)
			ilitek_touch_release_all_point();

		ilitek_check_key_release(x, y, 0);
		ts->is_touched = false;

		if (ts->system_suspend)
			ilitek_gesture_handle(false, 0, 0, 0);
	}
	input_sync(input);
	return 0;
}

static void ilitek_work(void *param)
{
	ts->process_and_report();
}

static void ilitek_irq_callback(uint32_t param)
{
	if (work_available(&ts->work)) {
		work_queue(HPWORK, &ts->work, ilitek_work, (void *)ts, 0);
	}
}

static int ilitek_request_pen_input_dev(void)
{
	int error;
	struct input_dev *input;

	if (!(input = input_allocate_device()))
		return -ENOMEM;

	tp_dbg("registering pen input device\n");

	__set_bit(INPUT_PROP_DIRECT, input->propbit);
	input->evbit[0] = BIT_MASK(EV_KEY) | BIT_MASK(EV_ABS);

	__set_bit(BTN_TOOL_PEN, input->keybit); /* In Range */
	__set_bit(BTN_TOOL_RUBBER, input->keybit); /* Invert */
	__set_bit(BTN_STYLUS, input->keybit); /* Barrel Swtich */
	__set_bit(BTN_TOUCH, input->keybit); /* Tip Switch */

	//	input->name = "ILITEK STYLUS";
	//	input->id.bustype = BUS_I2C;
	//	input->dev.parent = ts->device;

	input_set_abs_params(input, ABS_X, ts->dev->screen_info.pen_x_min,
			     ts->dev->screen_info.pen_x_max, 0, 0);
	input_set_abs_params(input, ABS_Y, ts->dev->screen_info.pen_y_min,
			     ts->dev->screen_info.pen_y_max, 0, 0);

	input_set_abs_params(input, ABS_PRESSURE,
			     ts->dev->screen_info.pressure_min,
			     ts->dev->screen_info.pressure_max, 0, 0);
	input_set_abs_params(input, ABS_TILT_X, ts->dev->screen_info.x_tilt_min,
			     ts->dev->screen_info.x_tilt_max, 0, 0);
	input_set_abs_params(input, ABS_TILT_Y, ts->dev->screen_info.y_tilt_min,
			     ts->dev->screen_info.y_tilt_max, 0, 0);

	if ((error = input_register_device(input))) {
		tp_err("register pen device failed, err: %d\n", error);
		input_free_device(input);
		return error;
	}

	ts->pen_input_dev = input;

	return 0;
}

static int ilitek_request_input_dev(void)
{
	int error;
	int i;
	struct input_dev *input;

	int x_min = ts->dev->screen_info.x_min;
	int y_min = ts->dev->screen_info.y_min;
	int x_max = ts->dev->screen_info.x_max;
	int y_max = ts->dev->screen_info.y_max;

	if (!(input = input_allocate_device()))
		return -ENOMEM;

	tp_dbg("registering touch input device\n");

#ifdef ILITEK_TOUCH_PROTOCOL_B
	INPUT_MT_INIT_SLOTS(input, ts->dev->tp_info.max_fingers);
#else
	input_set_abs_params(input, ABS_MT_TRACKING_ID, 0,
			     ts->dev->tp_info.max_fingers, 0, 0);
#endif

#ifdef ILITEK_REPORT_PRESSURE
	input_set_abs_params(input, ABS_MT_PRESSURE, 0, 255, 0, 0);
#endif

	for (i = 0; i < ts->dev->tp_info.key_num; i++)
		set_bit(ts->dev->key.info.keys[i].id & KEY_MAX, input->keybit);

//	input_set_capability(input, EV_KEY, KEY_POWER);

#ifndef ILITEK_USE_MTK_INPUT_DEV
	//input->name = ILITEK_TS_NAME;
	//input->id.bustype = BUS_I2C;
	//input->dev.parent = ts->device;

	__set_bit(INPUT_PROP_DIRECT, input->propbit);
	input->evbit[0] = BIT_MASK(EV_KEY) | BIT_MASK(EV_ABS);
	input->keybit[BIT_WORD(BTN_TOUCH)] = BIT_MASK(BTN_TOUCH);

#ifdef ILITEK_USE_LCM_RESOLUTION
	x_min = 0; y_min = 0;
	x_max = TOUCH_SCREEN_X_MAX; y_max = TOUCH_SCREEN_Y_MAX;
#endif

#if ILITEK_ROTATE_FLAG
	swap(x_min, y_min);
	swap(x_max, y_max);
#endif

	input_set_abs_params(input, ABS_MT_POSITION_X, x_min, x_max, 0, 0);
	input_set_abs_params(input, ABS_MT_POSITION_Y, y_min, y_max, 0, 0);
	input_set_abs_params(input, ABS_MT_TOUCH_MAJOR, 0, 255, 0, 0);
	input_set_abs_params(input, ABS_MT_WIDTH_MAJOR, 0, 255, 0, 0);

	if ((error = input_register_device(input))) {
		tp_err("input_register_device failed, err: %d\n", error);
		input_free_device(input);
		return error;
	}
#endif

	ts->input_dev = input;

	if (ts->dev->tp_info.pen_modes)
		ilitek_request_pen_input_dev();

	return 0;
}

static int hc_ts_probe(void)
{
	int ret, np;

	ts = kzalloc(sizeof(*ts), GFP_KERNEL);

	np = fdt_get_node_offset_by_path("/hcrtos/ilitek");
	if (np < 0) {
		printf("no find ilitek\n");
		ret = -ENOMEM;
	}

	if (fdt_get_property_u_32_index(np, "irq-gpio", 0, (u32 *)&ts->irq_gpio))
		return -1;
	if (fdt_get_property_u_32_index(np, "i2c-addr", 0, (u32 *)&ts->addr))
		return -1;
	if (fdt_get_property_string_index(np, "i2c-devpath", 0, &ts->i2c_devpath))
		return -1;

	ts->fd = open(ts->i2c_devpath, O_RDWR);

	ts->dev = ilitek_dev_init(interface_i2c, &dev_cb, ts);
	if (!ts->dev)
		goto err_probe;

	if (ts->dev->protocol.flag == PTL_V6) {
		ts->process_and_report = ilitek_read_data_and_report_6XX;
		ts->irq_trigger_type = GPIO_DIR_INPUT | GPIO_IRQ_RISING;
	} else {
		ts->process_and_report = ilitek_read_data_and_report_3XX;
		ts->irq_trigger_type = GPIO_DIR_INPUT | GPIO_IRQ_FALLING;
	}

	ilitek_request_input_dev();

	gpio_configure(ts->irq_gpio, ts->irq_trigger_type);
	ret = gpio_irq_request(ts->irq_gpio, ilitek_irq_callback, (uint32_t)ts);
	if (ret < 0) {
		printf("ERROR: gpio_irq_request() failed: %d\n", ret);
		goto err_probe;
	}

	return 0;

err_probe:
	return ret;
}

static int hc_ts_init(void)
{
	int ret = 0;

	ret = hc_ts_probe();

	return ret;
}

module_driver(hc_ilitek_driver, hc_ts_init,NULL,1)
