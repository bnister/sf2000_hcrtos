#ifndef _KERNEL_PLATFORM_REGISTER_H_
#define _KERNEL_PLATFORM_REGISTER_H_
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <malloc.h>
#include <sys/poll.h>
#include <kernel/soc/soc_common.h>
#include <kernel/drivers/snd.h>
#include <kernel/lib/console.h>
#include <kernel/io.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <freertos/event_groups.h>
#include <freertos/portable/portmacro.h>
#include <nuttx/queue.h>
#include <nuttx/wqueue.h>
#include <kernel/lib/fdt_api.h>
#include <kernel/completion.h>
#include <hcuapi/pinmux.h>

struct i2so_platform_device {
	const uint32_t irq;
	int status;

	uint8_t remain_buf[32];
	size_t remains;

	void *dma_buf;

	/* uint: byte */
	size_t dma_size;

	/* uint: byte */
	size_t wr;

	/* uint: byte */
	size_t rd;

	/* uint: byte */
	size_t avail;

	/* record the first pts */
	int32_t basetime;
	struct snd_pcm_params params;
	struct completion completion;

	struct snd_soc_platform *platform;
	struct work_s work;
	int ref_cnt;
	struct pinmux_setting *pinmux_data;
	struct pinmux_setting *pinmux_mute;
	bool mute_polar;
	uint8_t volume;
	uint8_t fade_en;
	uint8_t fade_step;
	uint8_t stc_checked;
	int stc_check_times;
	int dump_enable;
	uint32_t i2so_rec_size;
	void *record_kshm_hdl;
	void *drc_hdl;
	void *drc_outbuf;
	void *twotone_hdl;
	void *twotone_outbuf;
	int bass_idx;
	int treble_idx;
	int twotone_on_off;
	void *lrbalance_hdl;
	void *lrbalance_outbuf;
	int lrbalance_index;
	int lrbalance_onoff;
	void *eq6_hdl;
	void *eq6_outbuf;
	int eq6_onoff;
	int eq6_mode;
	int mute;
	int flush_time;
};

struct i2si_platform_device {
	const uint32_t irq;
	int status;
	void *dma_buf;
	/* uint: byte */
	int dma_size;

	/* uint: byte */
	int wr;

	/* uint: byte */
	int rd;

	/* record the first pts */
	struct snd_pcm_params params;

	struct snd_soc_platform *platform;
	struct work_s work;

	uint8_t volume;
	uint8_t fade_en;
	uint8_t fade_step;
	struct pinmux_setting *pinmux_data;
};

struct platform_device {
	const uint32_t irq;
	int status;
	void *dma_buf;
	/* uint: byte */
	int dma_size;
	int sub_dma_size;

	/* uint: byte */
	int *wr;

	/* uint: byte */
	int rd;

	/* record the first pts */
	struct snd_pcm_params params;

	struct snd_soc_platform *platform;
	struct work_s work;
	struct pinmux_setting *pinmux_data;
};

struct pcmi0_platform_device {
	const uint32_t irq;
	int status;
	void *dma_buf;
	/* uint: byte */
	int dma_size;
	int sub_dma_size;

	/* uint: byte */
	int *wr;

	/* uint: byte */
	int rd;

	/* record the first pts */
	struct snd_pcm_params params;
	EventGroupHandle_t event;

	struct snd_soc_platform *platform;
	struct work_s work;
	struct pinmux_setting *pinmux_data;
};

struct pcmi1_platform_device {
	const uint32_t irq;
	int status;
	void *dma_buf;
	/* uint: byte */
	int dma_size;
	int sub_dma_size;

	/* uint: byte */
	int *wr;

	/* uint: byte */
	int rd;

	/* record the first pts */
	struct snd_pcm_params params;
	EventGroupHandle_t event;

	struct snd_soc_platform *platform;
	struct work_s work;
	struct pinmux_setting *pinmux_data;
};

struct pcmi2_platform_device {
	const uint32_t irq;
	int status;
	void *dma_buf;
	/* uint: byte */
	int dma_size;
	int sub_dma_size;

	/* uint: byte */
	int *wr;

	/* uint: byte */
	int rd;

	/* record the first pts */
	struct snd_pcm_params params;
	EventGroupHandle_t event;

	struct snd_soc_platform *platform;
	struct work_s work;
	struct pinmux_setting *pinmux_data;
};

struct i2si012_platform_device {
	const uint32_t irq;
	int status;
	void *dma_buf;
	/* uint: byte */
	int dma_size;
	int sub_dma_size;

	/* uint: byte */
	int *wr;

	/* uint: byte */
	int rd;

	/* record the first pts */
	struct snd_pcm_params params;
	EventGroupHandle_t event;

	struct snd_soc_platform *platform;
	struct work_s work;
	struct pinmux_setting *pinmux_data;

};

extern struct i2si_platform_device i2si_platform_dev;
extern struct i2so_platform_device i2so_platform_dev;
extern struct platform_device tdmi_platform_dev;
extern struct platform_device pdmi0_platform_dev;
extern struct spin_platform_device spin_platform_dev;
extern struct i2si012_platform_device i2si0_platform_dev;
extern struct i2si012_platform_device i2si1_platform_dev;
extern struct i2si012_platform_device i2si2_platform_dev;
extern struct pcmi0_platform_device pcmi0_platform_dev;
extern struct pcmi1_platform_device pcmi1_platform_dev;
extern struct pcmi2_platform_device pcmi2_platform_dev;

extern struct snd_soc_platform_driver pcmi0_platform_driver;
extern struct snd_soc_platform_driver pcmi1_platform_driver;
extern struct snd_soc_platform_driver pcmi2_platform_driver;
extern struct snd_soc_platform_driver i2so_platform_driver;
extern struct snd_soc_platform_driver i2si_platform_driver;
extern struct snd_soc_platform_driver tdmi_platform_driver;
extern struct snd_soc_platform_driver pdmi0_platform_driver;
extern struct snd_soc_platform_driver spin_platform_driver;
extern struct snd_soc_platform_driver i2si0_platform_driver;
extern struct snd_soc_platform_driver i2si1_platform_driver;
extern struct snd_soc_platform_driver i2si2_platform_driver;

#endif
