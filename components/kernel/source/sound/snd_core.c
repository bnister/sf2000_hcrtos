#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <kernel/list.h>
#include <kernel/io.h>
#include <nuttx/fs/fs.h>
#include <kernel/drivers/snd.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <nuttx/wqueue.h>
#include <kernel/drivers/hc_clk_gate.h>

static LIST_HEAD(__dais);
static LIST_HEAD(__platforms);
static uint32_t card_bitmap = 0xffffffff;
static int ref_cnt = 0;
#ifndef max
#define max(a, b) (((a) > (b)) ? (a) : (b))
#endif

struct snd_soc_card {
	struct list_head platforms;
	struct list_head dais;

	int id;
};

struct snd_platform_device {
	struct snd_soc_platform *platform;
	snd_pcm_uframes_t avail_min;
	wait_queue_head_t wait;
	SemaphoreHandle_t mutex;
	int ntkey;
};

static inline void __lock(struct snd_platform_device *dev)
{
	xSemaphoreTake(dev->mutex, portMAX_DELAY);
}

static inline void __unlock(struct snd_platform_device *dev)
{
	xSemaphoreGive(dev->mutex);
}

static int snd_open(struct file *filep)
{
	struct inode *inode = filep->f_inode;
	struct snd_soc_platform *platform = inode->i_private;
	struct snd_platform_device *dev = malloc(sizeof(*dev));

	if (!dev)
		return -ENOMEM;

	memset(dev, 0, sizeof(*dev));

	dev->mutex = xSemaphoreCreateMutex();
	if (!dev->mutex) {
		free(dev);
		return -ENOMEM;
	}

	if (ref_cnt == 0) {
		hc_clk_enable(AUDIO_CLK);
		hc_clk_enable(SPO_CLK);
		hc_clk_enable(DDP_SPO_CLK);
		hc_clk_enable(AUD_EXTI_1_BCLK_P1); //i2si bck enable
		hc_clk_enable(AUD_EXTI_SP_CLK_P1);//spi /ddpspi clk
		hc_clk_enable(AUD_EXTO_1_MCLK_P0); //i2so mclk output
		hc_clk_enable(AUD_EXTO_SP_CLK_P0); //spo /ddpspo clk
		hc_clk_enable(AUD_EXTO_HDMI_ADAC_1_BCLK_P0);//i2s bclk
		hc_clk_enable(AUD_EXTO_HDMI_ADAC_1_BCLK_P1);//i2s bclk
		hc_clk_enable(AUD_EXTO_HDMI_ADAC_1_BCLK_P2);//i2s bclk
		hc_clk_enable(AUD_HDMI_SP_CLK_P0);//hdmi-spo clk
		hc_clk_enable(AUD_HDMI_SP_CLK_P1);//hdmi-spo clk
		hc_clk_enable(HDMI_F128_LR_CLK);//hdmi-spo clk
		hc_clk_enable(AUD_PCMO_BCLK_CLK);//pcmo bclk
	}

	ref_cnt++;
	dev->platform = platform;
	dev->ntkey = -1;
	init_waitqueue_head(&dev->wait);
	filep->f_priv = dev;

	return 0;
}

static int snd_close(struct file *filep)
{
	struct snd_platform_device *dev = filep->f_priv;
	__lock(dev);

	//ref_cnt--;
	if (ref_cnt == 0) {
		hc_clk_disable(SPO_CLK);
		hc_clk_disable(DDP_SPO_CLK);
		hc_clk_disable(AUD_EXTI_1_BCLK_P1); //i2si bck enable
		hc_clk_disable(AUD_EXTI_SP_CLK_P1);//spi /ddpspi clk
		hc_clk_disable(AUD_EXTO_1_MCLK_P0); //i2so mclk output
		hc_clk_disable(AUD_EXTO_SP_CLK_P0); //spo /ddpspo clk
		hc_clk_disable(AUD_EXTO_HDMI_ADAC_1_BCLK_P0);//i2s bclk
		hc_clk_disable(AUD_EXTO_HDMI_ADAC_1_BCLK_P1);//i2s bclk
		hc_clk_disable(AUD_EXTO_HDMI_ADAC_1_BCLK_P2);//i2s bclk
		hc_clk_disable(AUD_HDMI_SP_CLK_P0);//hdmi-spo clk
		hc_clk_disable(AUD_HDMI_SP_CLK_P1);//hdmi-spo clk
		hc_clk_disable(HDMI_F128_LR_CLK);//hdmi-spo clk
		hc_clk_disable(AUD_PCMO_BCLK_CLK);//pcmo bclk
	}
	if (dev->ntkey > 0) {
		work_notifier_teardown(dev->ntkey);
	}

	wake_up(&dev->wait);

	__unlock(dev);

	vSemaphoreDelete(dev->mutex);
	free(dev);

	return 0;
}

static ssize_t snd_read(struct file *filep, char *buf, size_t size)
{
	return 0;
}

static ssize_t snd_write(struct file *filep, const char *buf, size_t size)
{
	return 0;
}

static int snd_get_cap(struct snd_soc_platform *platform,
		       struct snd_capability *cap)
{
	struct snd_soc_dai *dai, *next;
	struct snd_capability tmp = { 0 };
	struct snd_soc_card *card = platform->card;
	bool first = true;

	if (cap == NULL)
		return -EINVAL;

	list_for_each_entry_safe (dai, next, &platform->dais, head) {
		if (!dai->driver->get_capability ||
		    dai->driver->get_capability(dai, &tmp))
			continue;

		if (first) {
			cap->rates = tmp.rates;
			cap->formats = tmp.formats;
			first = false;
		} else {
			cap->rates &= tmp.rates;
			cap->formats &= tmp.formats;
		}
	}

	list_for_each_entry_safe (dai, next, &card->dais, head) {
		if (!dai->driver->get_capability ||
		    dai->driver->get_capability(dai, &tmp))
			continue;

		if (first) {
			cap->rates = tmp.rates;
			cap->formats = tmp.formats;
			first = false;
		} else {
			cap->rates &= tmp.rates;
			cap->formats &= tmp.formats;
		}
	}

	return 0;
}

static int snd_hw_params(struct snd_soc_platform *platform,
			 struct snd_pcm_params *params)
{
	struct snd_soc_dai *dai, *next;
	struct snd_soc_card *card = platform->card;
	unsigned int rate;
	snd_pcm_format_t fmt;
	unsigned int channels;
	uint8_t align;
	int ret = -EFAULT;

	if (params == NULL)
		return -EINVAL;

	rate = params->rate;
	fmt = params->format;
	channels = params->channels;
	align = params->align;

	list_for_each_entry_safe (dai, next, &platform->dais, head) {
		if (!dai->driver->hw_params)
			continue;

		if (dai->driver->ioctl) {
			ret = dai->driver->ioctl(dai, SND_IOCTL_SRC_SEL, &params->pcm_source);
			if (ret) {
				return ret;
			}
		}

		ret = dai->driver->hw_params(dai, rate, fmt, channels, align);
		if (ret) {
			if (dai->driver->ioctl) {
				dai->driver->ioctl(dai, SND_IOCTL_SRC_SEL_CLEAR, NULL);
			}
			return ret;
		}
	}

	list_for_each_entry_safe (dai, next, &card->dais, head) {
		if (!dai->driver->hw_params)
			continue;

		ret = dai->driver->hw_params(dai, rate, fmt, channels, align);
		if (ret) {
			return ret;
		}
	}

	ret = platform->driver->hw_params(platform, params);

	return ret;
}

static int snd_hw_free(struct snd_soc_platform *platform)
{
	struct snd_soc_dai *dai, *next;
	struct snd_soc_card *card = platform->card;
	int ret = -EFAULT;

	if (platform->driver->volume_mute) {
		platform->driver->volume_mute(platform, 1);
	}

	ret = platform->driver->trigger(platform, SND_TRIGGER_STOP);
	if (ret)
		return ret;

	ret = platform->driver->hw_free(platform);

	list_for_each_entry_safe (dai, next, &platform->dais, head) {
		ret = dai->driver->trigger(dai, SND_TRIGGER_STOP);
		if (ret) {
			return ret;
		}
		ret = dai->driver->hw_free(dai);
		if (ret) {
			return ret;
		}
	}

	list_for_each_entry_safe (dai, next, &card->dais, head) {
		ret = dai->driver->trigger(dai, SND_TRIGGER_STOP);
		if (ret) {
			return ret;
		}
		ret = dai->driver->hw_free(dai);
		if (ret) {
			return ret;
		}
	}

	return ret;
}

static int snd_start(struct snd_soc_platform *platform)
{
	struct snd_soc_dai *dai, *next;
	int ret = -EFAULT;

	list_for_each_entry_safe (dai, next, &platform->dais, head) {
		ret = dai->driver->trigger(dai, SND_TRIGGER_START);
		if (ret) {
			return ret;
		}
	}

	return platform->driver->trigger(platform, SND_TRIGGER_START);
}

static int snd_drop(struct snd_soc_platform *platform)
{
	struct snd_soc_dai *dai, *next;
	int ret = -EFAULT;

	list_for_each_entry_safe (dai, next, &platform->dais, head) {
		ret = dai->driver->trigger(dai, SND_TRIGGER_STOP);
		if (ret) {
			return ret;
		}
	}

	return platform->driver->trigger(platform, SND_TRIGGER_STOP);
}

static int snd_delay(struct snd_soc_platform *platform, snd_pcm_uframes_t *frames)
{
	if (!frames)
		return -EINVAL;

	return platform->driver->ioctl(platform, SND_IOCTL_DELAY, frames);
}

static int snd_drain(struct snd_soc_platform *platform)
{
	return platform->driver->ioctl(platform, SND_IOCTL_DRAIN, 0);
}

static int snd_pause(struct snd_soc_platform *platform)
{
	return platform->driver->trigger(platform, SND_TRIGGER_PAUSE_PUSH);
}

static int snd_resume(struct snd_soc_platform *platform)
{
	return platform->driver->trigger(platform, SND_TRIGGER_PAUSE_RELEASE);
}

static int snd_xfer(struct snd_soc_platform *platform, struct snd_xfer *xfer)
{
	snd_pcm_uframes_t frames = 0;

	frames = platform->driver->get_avail(platform);
	if (frames < xfer->frames)
		return -1;

	return platform->driver->transfer(platform, xfer);
}

static int snd_set_volume(struct snd_soc_platform *platform, uint8_t *pvol)
{
	return platform->driver->set_volume(platform, pvol);
}

static int snd_get_volume(struct snd_soc_platform *platform, uint8_t *pvol)
{
	return platform->driver->get_volume(platform, pvol);
}

static int snd_set_mute(struct snd_soc_platform *platform, bool mute)
{
	return platform->driver->mute(platform, mute);
}

static int snd_get_hw_info(struct snd_soc_platform *platform, struct snd_hw_info *hw_info)
{
	return platform->driver->get_hw_info(platform, hw_info);
}

static int snd_set_record(struct snd_soc_platform *platform, int kshm_buf_size)
{
	return platform->driver->set_record(platform, kshm_buf_size);
}

static int snd_set_free_record(struct snd_soc_platform *platform)
{
	return platform->driver->set_free_record(platform);
}

static int snd_ioctl_default(struct snd_soc_platform *platform,
	int cmd, unsigned long arg)
{
	struct snd_soc_dai *dai, *next;
	struct snd_soc_card *card = platform->card;
	int ret = -EFAULT;

	list_for_each_entry_safe (dai, next, &platform->dais, head) {
		if (!dai->driver->ioctl)
			continue;

		ret = dai->driver->ioctl(dai, cmd, (void *)arg);
		if (ret) {
			return ret;
		}
	}

	list_for_each_entry_safe (dai, next, &card->dais, head) {
		if (!dai->driver->ioctl)
			continue;

		ret = dai->driver->ioctl(dai, cmd, (void *)arg);
		if (ret) {
			return ret;
		}
	}

	ret = platform->driver->ioctl(platform, cmd, (void *)arg);

	return ret;
}

static int snd_ioctl(struct file *filep, int cmd, unsigned long arg)
{
	struct inode *inode = filep->f_inode;
	struct snd_soc_platform *platform = inode->i_private;
	struct snd_platform_device *dev = filep->f_priv;
	int ret = -EFAULT;

	__lock(dev);

	switch (cmd) {
	case SND_IOCTL_GETCAP: {
		ret = snd_get_cap(platform, (struct snd_capability *)arg);
		break;
	}
	case SND_IOCTL_HW_PARAMS: {
		ret = snd_hw_params(platform, (struct snd_pcm_params *)arg);
		break;
	}
	case SND_IOCTL_HW_FREE: {
		ret = snd_hw_free(platform);
		break;
	}
	case SND_IOCTL_START: {
		ret = snd_start(platform);
		break;
	}
	case SND_IOCTL_DROP: {
		ret = snd_drop(platform);
		break;
	}
	case SND_IOCTL_DELAY: {
		ret = snd_delay(platform, (uint32_t *)arg);
		break;
	}
	case SND_IOCTL_DRAIN: {
		ret = snd_drain(platform);
		break;
	}
	case SND_IOCTL_PAUSE: {
		ret = snd_pause(platform);
		break;
	}
	case SND_IOCTL_RESUME: {
		ret = snd_resume(platform);
		break;
	}
	case SND_IOCTL_XFER: {
		ret = snd_xfer(platform, (struct snd_xfer *)arg);
		break;
	}
	case SND_IOCTL_AVAIL_MIN: {
		dev->avail_min = *(snd_pcm_uframes_t *)arg;
		ret = 0;
		break;
	}
	case SND_IOCTL_SET_VOLUME: {
		ret = snd_set_volume(platform, (uint8_t *)arg);
		break;
	}
	case SND_IOCTL_GET_VOLUME: {
		ret = snd_get_volume(platform, (uint8_t *)arg);
		break;
	}
	case SND_IOCTL_SET_MUTE: {
		ret = snd_set_mute(platform, (bool)arg);
		break;
	}
	case SND_IOCTL_GET_HW_INFO: {
		ret = snd_get_hw_info(platform, (struct snd_hw_info *)arg);
		break;
	}
	case SND_IOCTL_SET_RECORD: {
		ret = snd_set_record(platform, (int)arg);
		break;
	}
	case SND_IOCTL_SET_FREE_RECORD: {
		ret = snd_set_free_record(platform);
		break;
	}

	default: {
		ret = snd_ioctl_default(platform, cmd, arg);
		break;
	}
	}

	__unlock(dev);

	return ret;
}

static void snd_wakeup_poll(void *arg, unsigned long param)
{
	struct snd_platform_device *dev = (struct snd_platform_device *)arg;

	wake_up(&dev->wait);
	dev->ntkey = -1;
}

static int snd_poll(struct file *filep, poll_table *wait)
{
	struct inode *inode = filep->f_inode;
	struct snd_soc_platform *platform = inode->i_private;
	struct snd_platform_device *dev = filep->f_priv;
	snd_pcm_uframes_t frames = 0;
	struct work_notifier_s info = { 0 };
	int mask = 0;

	if (!dev)
		return POLLERR;

	__lock(dev);

	poll_wait(filep, &dev->wait, wait);

	frames = platform->driver->get_avail(platform);

	if (frames >= dev->avail_min) {
		if (dev->platform->direction == SND_STREAM_PLAYBACK)
			mask |= POLLOUT | POLLWRNORM;
		else
			mask |= POLLIN | POLLRDNORM;
	} else if (dev->ntkey <= 0) {
		info.evtype = SND_EVENT_AVAIL;
		info.qid = HPWORK;
		info.remote = false;
		info.oneshot = true;
		info.qualifier = platform;
		info.arg = dev;
		info.worker2 = snd_wakeup_poll;
		dev->ntkey = work_notifier_setup(&info);
	}

	__unlock(dev);

	return mask;
}

static const struct file_operations snd_fops = {
	.open = snd_open,
	.close = snd_close,
	.read = snd_read,
	.write = snd_write,
	.poll = snd_poll,
	.ioctl = snd_ioctl,
};

int snd_soc_register_dai(struct snd_soc_dai *dai)
{
	int res = 0;

	if (!dai || !dai->name || !dai->driver) {
		return -ENODEV;
	}

	if (dai->driver->probe) {
		res = dai->driver->probe(dai);
		if (res) {
			return res;
		}
	}

	taskENTER_CRITICAL();
	list_add_tail(&dai->head, &__dais);
	taskEXIT_CRITICAL();

	return res;
}

int snd_soc_unregister_dai(struct snd_soc_dai *dai)
{
	int res = 0;

	if (!dai || !dai->name || !dai->driver) {
		return -ENODEV;
	}

	if (dai->driver->remove) {
		res = dai->driver->remove(dai);
	}

	taskENTER_CRITICAL();
	list_del(&dai->head);
	taskEXIT_CRITICAL();

	return res;
}

int snd_soc_register_platform(struct snd_soc_platform *platform)
{
	struct snd_soc_dai *dai, *dcurr, *dnext;
	int res = 0;
	unsigned int i;

	if (!platform || !platform->name || !platform->driver) {
		return -ENODEV;
	}

	if (platform->driver->probe) {
		res = platform->driver->probe(platform);
		if (res) {
			return res;
		}
	}

	taskENTER_CRITICAL();

	list_add_tail(&platform->head, &__platforms);

	for (i = 0; i < platform->num_dailinks; i++) {
		dai = NULL;
		const char *name = platform->dailinks[i].name;
		list_for_each_entry_safe (dcurr, dnext, &__dais, head) {
			if (!strncmp(dcurr->name, name,
				     max(strlen(dcurr->name), strlen(name)))) {
				dai = dcurr;
				break;
			}
		}
		if (dai != NULL) {
			list_del(&dai->head);
			list_add_tail(&dai->head, &platform->dais);
		}
	}

	taskEXIT_CRITICAL();

	return res;
}

int snd_soc_unregister_platform(struct snd_soc_platform *platform)
{
	int res = 0;

	if (!platform || !platform->name || !platform->driver) {
		return -ENODEV;
	}

	if (platform->driver->remove) {
		res = platform->driver->remove(platform);
	}

	taskENTER_CRITICAL();
	list_del(&platform->head);
	taskEXIT_CRITICAL();

	return res;
}

int snd_soc_register_card(struct snd_soc_dai_link *links)
{
	struct snd_soc_dai *dai, *dcurr, *dnext;
	struct snd_soc_platform *platform, *pcurr, *pnext;
	struct snd_soc_card *card;
	int cardid = ffs(card_bitmap);
	unsigned int i;

	if (cardid == 0) {
		return -ENODEV;
	}

	cardid--;

	card = malloc(sizeof(*card));
	if (card == NULL) {
		return -ENOMEM;
	} else {
		INIT_LIST_HEAD(&card->dais);
		INIT_LIST_HEAD(&card->platforms);
		card->id = cardid;
		card_bitmap &= ~BIT(cardid);
	}

	taskENTER_CRITICAL();
	for (i = 0; i < links->num_dais; i++) {
		dai = NULL;
		const char *name = links->dais[i].name;
		list_for_each_entry_safe (dcurr, dnext, &__dais, head) {
			if (!strncmp(dcurr->name, name,
				     max(strlen(dcurr->name), strlen(name)))) {
				dai = dcurr;
				break;
			}
		}
		if (dai != NULL) {
			list_del(&dai->head);
			list_add_tail(&dai->head, &card->dais);
		}
	}

	for (i = 0; i < links->num_platforms; i++) {
		platform = NULL;
		const char *name = links->platforms[i].name;
		list_for_each_entry_safe (pcurr, pnext, &__platforms, head) {
			if (!strncmp(pcurr->name, name,
				     max(strlen(pcurr->name), strlen(name)))) {
				platform = pcurr;
				break;
			}
		}
		if (platform != NULL) {
			list_del(&platform->head);
			list_add_tail(&platform->head, &card->platforms);
		}
	}
	taskEXIT_CRITICAL();

	list_for_each_entry_safe (pcurr, pnext, &card->platforms, head) {
		char *name = NULL;

		asprintf(&name, "/dev/sndC%d%s", cardid, pcurr->devname);
		pcurr->card = card;
		register_driver(name, &snd_fops, 0666, pcurr);
	}

	return 0;
}

int snd_soc_unregister_card(struct snd_soc_card *card)
{
	return 0;
}
