#ifndef _KERNEL_SND_H_
#define _KERNEL_SND_H_

#include <stdint.h>
#include <stdbool.h>
#include <hcuapi/snd.h>
#include <kernel/list.h>

#define SND_TRIGGER_START		0
#define SND_TRIGGER_STOP		1
#define SND_TRIGGER_PAUSE_PUSH		2
#define SND_TRIGGER_PAUSE_RELEASE	3

#define SND_EVENT_AVAIL			0

struct snd_soc_card;
struct snd_soc_platform;
struct snd_soc_dai;
struct snd_soc_dai_link_component;

enum {
	SND_STATUS_STOPPED = 0,
	SND_STATUS_STARTED = 1,
	SND_STATUS_PAUSED = 2,
};

enum {
	SND_STREAM_PLAYBACK = 0,
	SND_STREAM_CAPTURE,
	SND_STREAM_LAST = SND_STREAM_CAPTURE,
};

struct snd_soc_platform_driver {
	int (*probe)(struct snd_soc_platform *platform);
	int (*remove)(struct snd_soc_platform *platform);

	int (*trigger)(struct snd_soc_platform *platform, unsigned int cmd);

	int (*ioctl)(struct snd_soc_platform *platform, unsigned int cmd,
		     void *arg);
	int (*hw_params)(struct snd_soc_platform *platform,
			 struct snd_pcm_params *params);
	int (*hw_free)(struct snd_soc_platform *platform);

	int (*transfer)(struct snd_soc_platform *platform,
			struct snd_xfer *xfer);

	int (*set_volume)(struct snd_soc_platform *platform,
			uint8_t *pvol);
	int (*get_volume)(struct snd_soc_platform *platform,
			uint8_t *pvol);
	int (*volume_mute)(struct snd_soc_platform *platform,
			bool mute);
	int (*mute)(struct snd_soc_platform *platform,
			bool mute);
	int (*get_hw_info)(struct snd_soc_platform *platform,
			struct snd_hw_info *hw_info);
	int (*set_record)(struct snd_soc_platform *platform,
		int kshm_buf_size);
	int (*set_free_record)(struct snd_soc_platform *platform);

	snd_pcm_uframes_t (*get_avail)(struct snd_soc_platform *platform);
};

struct snd_soc_platform {
	const char *name;
	const char *devname;
	struct list_head head;
	struct list_head dais;
	uint8_t direction;
	const struct snd_soc_platform_driver *driver;
	struct snd_soc_card *card;
	void *priv;

	const struct snd_soc_dai_link_component *dailinks;
	unsigned int num_dailinks;
};

struct snd_soc_dai_driver {
	int (*probe)(struct snd_soc_dai *dai);
	int (*remove)(struct snd_soc_dai *dai);

	int (*hw_params)(struct snd_soc_dai *dai, unsigned int rate,
			 snd_pcm_format_t fmt, unsigned int channels,
			 uint8_t align);
	int (*hw_free)(struct snd_soc_dai *dai);

	int (*trigger)(struct snd_soc_dai *dai, unsigned int cmd);

	int (*ioctl)(struct snd_soc_dai *dai, unsigned int cmd,
		     void *arg);

	int (*get_capability)(struct snd_soc_dai *dai,
			      struct snd_capability *cap);
};

struct snd_soc_dai {
	const char *name;
	struct list_head head;

	const struct snd_soc_dai_driver *driver;
	void *priv;
};

struct snd_soc_dai_link_component {
	const char *name;
};

struct snd_soc_dai_link {
	const struct snd_soc_dai_link_component *dais;
	unsigned int num_dais;

	const struct snd_soc_dai_link_component *platforms;
	unsigned int num_platforms;
};

int snd_soc_register_dai(struct snd_soc_dai *dai);
int snd_soc_unregister_dai(struct snd_soc_dai *dai);

int snd_soc_register_platform(struct snd_soc_platform *platform);
int snd_soc_unregister_platform(struct snd_soc_platform *platform);

int snd_soc_register_card(struct snd_soc_dai_link *links);
int snd_soc_unregister_card(struct snd_soc_card *card);

#endif	/* _KERNEL_SND_H_ */
