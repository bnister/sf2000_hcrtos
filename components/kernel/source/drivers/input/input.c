#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <kernel/vfs.h>
#include <kernel/io.h>
#include <kernel/drivers/input.h>
#include <hcuapi/input.h>

#include <linux/timer.h>

static uint32_t id_bitmap = 0xffffffff;
static uint32_t mouse_id_bitmap = 0xffffffff;
static uint32_t kbd_id_bitmap = 0xffffffff;

static int input_get_disposition(struct input_dev *dev,
			  unsigned int type, unsigned int code, int *pval);
static void input_start_autorepeat(struct input_dev *dev, int code);
static void input_stop_autorepeat(struct input_dev *dev);
static inline int is_event_supported(unsigned int code,
				     unsigned long *bm, unsigned int max);


static int input_open(struct file *filep)
{
	struct inode *inode = filep->f_inode;
	struct input_dev *dev = inode->i_private;

	if (dev->open) {
		dev->open(dev);
	}

	return 0;
}

static int input_close(struct file *filep)
{
	struct inode *inode = filep->f_inode;
	struct input_dev *dev = inode->i_private;

	if (dev->close) {
		dev->close(dev);
	}

	return 0;
}

static ssize_t input_read(struct file *filep, char *buffer, size_t buflen)
{
	struct inode *inode = filep->f_inode;
	struct input_dev *dev = inode->i_private;

	if (kfifo_out(&dev->kfifo, (struct input_event *)buffer, 1))
		return sizeof(struct input_event);
	else
		return 0;
}

static ssize_t input_write(struct file *filep, const char *buffer,
			   size_t buflen)
{
	return 0;
}

static int input_poll(struct file *filep, poll_table *wait)
{
	struct inode *inode = filep->f_inode;
	struct input_dev *dev = inode->i_private;
	int mask = 0;

	poll_wait(filep, &dev->wait, wait);

	if (!kfifo_is_empty(&dev->kfifo))
		mask |= POLLIN | POLLRDNORM;

	return mask;
}

int input_get_keycode(struct input_dev *dev, struct input_keymap_entry *ke)
{
	unsigned long flags;
	int retval;

	spin_lock_irqsave(&dev->event_lock, flags);
	retval = dev->getkeycode(dev, ke);
	spin_unlock_irqrestore(&dev->event_lock, flags);

	return retval;
}

int input_set_keycode(struct input_dev *dev,
		      const struct input_keymap_entry *ke)
{
	unsigned long flags;
	unsigned int old_keycode;
	int retval;

	if (ke->keycode > KEY_MAX)
		return -EINVAL;

	spin_lock_irqsave(&dev->event_lock, flags);
	retval = dev->setkeycode(dev, ke, &old_keycode);
	spin_unlock_irqrestore(&dev->event_lock, flags);

	return retval;
}

static int evdev_handle_get_keycode_v2(struct input_dev *dev, void *p)
{
	struct input_keymap_entry *ke = (struct input_keymap_entry *)p;
	int error;

	error = input_get_keycode(dev, ke);
	if (error)
		return error;

	return 0;
}

static int evdev_handle_set_keycode_v2(struct input_dev *dev, void *p)
{
	struct input_keymap_entry *ke = (struct input_keymap_entry *)p;

	if (ke->len > sizeof(ke->scancode))
		return -EINVAL;

	return input_set_keycode(dev, ke);
}

static int input_ioctl(struct file *filep, int cmd, unsigned long arg)
{
	struct inode *inode = filep->f_inode;
	struct input_dev *dev = inode->i_private;

	switch (cmd) {
	case EVIOCGKEYCODE:
		return evdev_handle_get_keycode_v2(dev, (void *)arg);
	case EVIOCSKEYCODE:
		return evdev_handle_set_keycode_v2(dev, (void *)arg);
	default:
		break;
	}
	if (dev->ioctl) {
		dev->ioctl(filep,cmd,arg);
	}
	return 0;
}

static const struct file_operations input_ops = {
	.open = input_open, /* open */
	.close = input_close, /* close */
	.read = input_read, /* read */
	.write = input_write, /* write */
	.seek = NULL, /* seek */
	.ioctl = input_ioctl, /* ioctl */
	.poll = input_poll /* poll */
#ifndef CONFIG_DISABLE_PSEUDOFS_OPERATIONS
	,
	.unlink = NULL /* unlink */
#endif
};

static void input_pass_event(struct input_dev *dev, unsigned int type, unsigned int code,
		 int value)
{
	struct input_event event = { type, code, value };

	kfifo_put(&dev->kfifo, event);
	wake_up(&dev->wait);
}

static void input_pass_events(struct input_dev *dev, struct input_event *vals, unsigned int count)
{
	unsigned int i;
	struct input_event *v = vals;

	if(!count)
		return;

	for(i = 0; i < count; i++){
		v = vals + i;
		// printf("\t==> send [%u] type:%2.2x, code:%2.2x, val:%2.2lx\n", 
		// 		i, v->type, v->code, v->value);
		kfifo_put(&dev->kfifo, *v);
	}
	wake_up(&dev->wait);

	/* trigger auto repeat for key events */
	if (test_bit(EV_REP, dev->evbit) && test_bit(EV_KEY, dev->evbit)) {
		for (v = vals; v != vals + count; v++) {
			if (v->type == EV_KEY && v->value != 2) {
				if (v->value)
					input_start_autorepeat(dev, v->code);
				else
					input_stop_autorepeat(dev);
			}
		}
	}
}

#define INPUT_IGNORE_EVENT	0
#define INPUT_PASS_TO_HANDLERS	1
#define INPUT_PASS_TO_DEVICE	2
#define INPUT_SLOT		4
#define INPUT_FLUSH		8
#define INPUT_PASS_TO_ALL	(INPUT_PASS_TO_HANDLERS | INPUT_PASS_TO_DEVICE)

static int input_defuzz_abs_event(int value, int old_val, int fuzz)
{
	if (fuzz) {
		if (value > old_val - fuzz / 2 && value < old_val + fuzz / 2)
			return old_val;

		if (value > old_val - fuzz && value < old_val + fuzz)
			return (old_val * 3 + value) / 4;

		if (value > old_val - fuzz * 2 && value < old_val + fuzz * 2)
			return (old_val + value) / 2;
	}

	return value;
}

static int input_handle_abs_event(struct input_dev *dev,
				  unsigned int code, int *pval)
{
	struct input_mt *mt = dev->mt;
	bool is_mt_event;
	int *pold;

	if (code == ABS_MT_SLOT) {
		/*
		 * "Stage" the event; we'll flush it later, when we
		 * get actual touch data.
		 */
		if (mt && *pval >= 0 && *pval < mt->num_slots)
			mt->slot = *pval;

		return INPUT_IGNORE_EVENT;
	}

	is_mt_event = input_is_mt_value(code);

	if (!is_mt_event) {
		pold = &dev->absinfo[code].value;
	} else if (mt) {
		pold = &mt->slots[mt->slot].abs[code - ABS_MT_FIRST];
	} else {
		/*
		 * Bypass filtering for multi-touch events when
		 * not employing slots.
		 */
		pold = NULL;
	}

	if (pold) {
		*pval = input_defuzz_abs_event(*pval, *pold,
						dev->absinfo[code].fuzz);
		if (*pold == *pval)
			return INPUT_IGNORE_EVENT;

		*pold = *pval;
	}

	/* Flush pending "slot" event */
	if (is_mt_event && mt && mt->slot != input_abs_get_val(dev, ABS_MT_SLOT)) {
		input_abs_set_val(dev, ABS_MT_SLOT, mt->slot);
		input_pass_event(dev, EV_ABS, ABS_MT_SLOT, mt->slot);
	}

	return INPUT_PASS_TO_HANDLERS;
}


void input_event(struct input_dev *dev, unsigned int type, unsigned int code,
		 int value)
{
	int disposition = INPUT_PASS_TO_HANDLERS;
	struct input_event event;

	// printf("==> input_event: type %2.2x, code %2.2x, value %2.2x\n",
	// 	 type, code, value);

	disposition = input_get_disposition(dev, type, code, &value);
	if (disposition & INPUT_PASS_TO_HANDLERS){
		event.type = type;
		event.code = code;
		event.value = value;
		input_pass_events(dev, &event, 1);
	}
}

int input_register_device(struct input_dev *dev)
{
	int id = ffs(id_bitmap) - 1;
	char path[128];

	dev->id = id;
	id_bitmap &= ~BIT(id);

	memset(path, 0, sizeof(path));
	sprintf(path, "/dev/input/event%d", id);

	/*
	 * If delay and period are pre-set by the driver, then autorepeating
	 * is handled by the driver itself and we don't do it in input.c.
	 */
	if (!dev->rep[REP_DELAY] && !dev->rep[REP_PERIOD])
		input_enable_softrepeat(dev, 250, 33);

	register_driver(path, &input_ops, 0666, dev);

	return 0;
}

int input_register_mouse_device(struct input_dev *dev)
{
	int id = ffs(mouse_id_bitmap) - 1;
	char path[128];

	dev->id = id;
	mouse_id_bitmap &= ~BIT(id);

	memset(path, 0, sizeof(path));
	sprintf(path, "/dev/input/mouse%d", id);

	printf("register input device : %s\n", path);

	/*
	 * If delay and period are pre-set by the driver, then autorepeating
	 * is handled by the driver itself and we don't do it in input.c.
	 */
	if (!dev->rep[REP_DELAY] && !dev->rep[REP_PERIOD])
		input_enable_softrepeat(dev, 250, 33);

	register_driver(path, &input_ops, 0666, dev);

	return 0;
}

int input_register_kbd_device(struct input_dev *dev)
{
	int id = ffs(kbd_id_bitmap) - 1;
	char path[128];

	dev->id = id;
	kbd_id_bitmap &= ~BIT(id);

	memset(path, 0, sizeof(path));
	sprintf(path, "/dev/input/kbd%d", id);

	printf("register input device : %s\n", path);

	/*
	 * If delay and period are pre-set by the driver, then autorepeating
	 * is handled by the driver itself and we don't do it in input.c.
	 */
	if (!dev->rep[REP_DELAY] && !dev->rep[REP_PERIOD])
		input_enable_softrepeat(dev, 250, 33);

	register_driver(path, &input_ops, 0666, dev);

	return 0;
}

void input_unregister_device(struct input_dev *dev)
{
	char path[128];
	int id = dev->id;

	memset(path, 0, sizeof(path));
	sprintf(path, "/dev/input/event%d", id);
	unregister_driver(path);
	id_bitmap |= ~BIT(id);
}

void input_unregister_mouse_device(struct input_dev *dev)
{
	char path[128];
	int id = dev->id;

	memset(path, 0, sizeof(path));
	sprintf(path, "/dev/input/mouse%d", id);
	unregister_driver(path);
	mouse_id_bitmap |= BIT(id);
}

void input_unregister_kbd_device(struct input_dev *dev)
{
	char path[128];
	int id = dev->id;

	memset(path, 0, sizeof(path));
	sprintf(path, "/dev/input/kbd%d", id);
	unregister_driver(path);
	kbd_id_bitmap |= BIT(id);
}

struct input_dev *input_allocate_device(void)
{
	struct input_dev *dev = malloc(sizeof(*dev));

	if (!dev)
		return NULL;

	memset(dev, 0, sizeof(*dev));

	INIT_KFIFO(dev->kfifo);
	init_waitqueue_head(&dev->wait);
	init_timer(&dev->timer);

	return dev;
}

void input_free_device(struct input_dev *dev)
{
	free(dev);
}

void input_alloc_absinfo(struct input_dev *dev)
{
	if (!dev->absinfo)
		dev->absinfo = kcalloc(ABS_CNT, sizeof(struct input_absinfo),
					GFP_KERNEL);
}

void input_set_abs_params(struct input_dev *dev, unsigned int axis,
			  int min, int max, int fuzz, int flat)
{
	struct input_absinfo *absinfo;

	input_alloc_absinfo(dev);
	if (!dev->absinfo)
		return;

	absinfo = &dev->absinfo[axis];
	absinfo->minimum = min;
	absinfo->maximum = max;
	absinfo->fuzz = fuzz;
	absinfo->flat = flat;
}

static int input_get_disposition(struct input_dev *dev,
			  unsigned int type, unsigned int code, int *pval)
{
	int disposition = INPUT_IGNORE_EVENT;
	int value = *pval;

	switch (type) {

	case EV_SYN:
		switch (code) {
		case SYN_CONFIG:
			disposition = INPUT_PASS_TO_ALL;
			break;

		case SYN_REPORT:
			disposition = INPUT_PASS_TO_HANDLERS | INPUT_FLUSH;
			break;
		case SYN_MT_REPORT:
			disposition = INPUT_PASS_TO_HANDLERS;
			break;
		}
		break;

	case EV_KEY:
		/* auto-repeat bypasses state updates */
		if (value == 2) {
			disposition = INPUT_PASS_TO_HANDLERS;
			break;
		}

		if (!!test_bit(code, dev->key) != !!value) {

			__change_bit(code, dev->key);
			disposition = INPUT_PASS_TO_HANDLERS;
		}
			disposition = INPUT_PASS_TO_HANDLERS;
		break;

	// case EV_SW:
	// 	if (is_event_supported(code, dev->swbit, SW_MAX) &&
	// 	    !!test_bit(code, dev->sw) != !!value) {

	// 		__change_bit(code, dev->sw);
	// 		disposition = INPUT_PASS_TO_HANDLERS;
	// 	}
	// 	break;

	case EV_ABS:
		disposition = input_handle_abs_event(dev, code, &value);
		break;

	case EV_REL:
		disposition = INPUT_PASS_TO_HANDLERS;

		break;

	case EV_MSC:
		disposition = INPUT_PASS_TO_ALL;
		break;

	// case EV_LED:
	// 	if (is_event_supported(code, dev->ledbit, LED_MAX) &&
	// 	    !!test_bit(code, dev->led) != !!value) {

	// 		__change_bit(code, dev->led);
	// 		disposition = INPUT_PASS_TO_ALL;
	// 	}
	// 	break;

	// case EV_SND:
	// 	if (is_event_supported(code, dev->sndbit, SND_MAX)) {

	// 		if (!!test_bit(code, dev->snd) != !!value)
	// 			__change_bit(code, dev->snd);
	// 		disposition = INPUT_PASS_TO_ALL;
	// 	}
	// 	break;

	// case EV_REP:
	// 	if (code <= REP_MAX && value >= 0 && dev->rep[code] != value) {
	// 		dev->rep[code] = value;
	// 		disposition = INPUT_PASS_TO_ALL;
	// 	}
	// 	break;

	// case EV_FF:
	// 	if (value >= 0)
	// 		disposition = INPUT_PASS_TO_ALL;
	// 	break;

	// case EV_PWR:
	// 	disposition = INPUT_PASS_TO_ALL;
	// 	break;
	}

	*pval = value;
	return disposition;
}


static const struct input_event input_value_sync = { EV_SYN, SYN_REPORT, 1 };

static inline int is_event_supported(unsigned int code,
				     unsigned long *bm, unsigned int max)
{
	return code <= max && test_bit(code, bm);
}


static void input_start_autorepeat(struct input_dev *dev, int code)
{
	if (test_bit(EV_REP, dev->evbit) &&
	    dev->rep[REP_PERIOD] && dev->rep[REP_DELAY] &&
	    dev->timer.data) {
		dev->repeat_key = code;
		mod_timer(&dev->timer,
			  jiffies + msecs_to_jiffies(dev->rep[REP_DELAY]));
	}
}

static void input_stop_autorepeat(struct input_dev *dev)
{
	del_timer(&dev->timer);
}

/*
 * Generate software autorepeat event. Note that we take
 * dev->event_lock here to avoid racing with input_event
 * which may cause keys get "stuck".
 */
static void input_repeat_key(unsigned long data)
{
	struct input_dev *dev = (void *) data;
	unsigned long flags;

	spin_lock_irqsave(&dev->event_lock, flags);

	if (test_bit(dev->repeat_key, dev->key) &&
	    is_event_supported(dev->repeat_key, dev->keybit, KEY_MAX)) {
		struct input_event vals[] =  {
			{ EV_KEY, dev->repeat_key, 2 },
			input_value_sync
		};

		input_pass_events(dev, vals, ARRAY_SIZE(vals));
		
		if (dev->rep[REP_PERIOD])
			mod_timer(&dev->timer, jiffies +
					msecs_to_jiffies(dev->rep[REP_PERIOD]));
	}

	spin_unlock_irqrestore(&dev->event_lock, flags);
}

/**
 * input_enable_softrepeat - enable software autorepeat
 * @dev: input device
 * @delay: repeat delay
 * @period: repeat period
 *
 * Enable software autorepeat on the input device.
 */
void input_enable_softrepeat(struct input_dev *dev, int delay, int period)
{
	dev->timer.data = (unsigned long) dev;
	dev->timer.function = input_repeat_key;
	dev->rep[REP_DELAY] = delay;
	dev->rep[REP_PERIOD] = period;
}
