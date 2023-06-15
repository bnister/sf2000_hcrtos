// SPDX-License-Identifier: GPL-2.0
// rc-main.c - Remote Controller core module
//
// Copyright (C) 2009-2010 by Mauro Carvalho Chehab
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <kernel/types.h>
#include <kernel/module.h>
#include <kernel/drivers/input.h>
#include <kernel/io.h>

#include <hcuapi/input-event-codes.h>
#include <hcuapi/input.h>
#include <linux/export.h>
#include <linux/printk.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/log2.h>
#include <linux/mutex.h>
#include <linux/spinlock.h>
#include <linux/limits.h>
#include <linux/jiffies.h>
#include <linux/timer.h>
#include "rc-core.h"
#include "rc-core-priv.h"

/* Sizes are in bytes, 256 bytes allows for 32 entries on x64 */
#define IR_TAB_MIN_SIZE	256
#define IR_TAB_MAX_SIZE	8192

static const struct {
	const char *name;
	unsigned int repeat_period;
	unsigned int scancode_bits;
} protocols[] = {
	[RC_PROTO_UNKNOWN] = { .name = "unknown", .repeat_period = 125 },
	[RC_PROTO_OTHER] = { .name = "other", .repeat_period = 125 },
	[RC_PROTO_RC5] = { .name = "rc-5",
		.scancode_bits = 0x1f7f, .repeat_period = 114 },
	[RC_PROTO_RC5X_20] = { .name = "rc-5x-20",
		.scancode_bits = 0x1f7f3f, .repeat_period = 114 },
	[RC_PROTO_RC5_SZ] = { .name = "rc-5-sz",
		.scancode_bits = 0x2fff, .repeat_period = 114 },
	[RC_PROTO_JVC] = { .name = "jvc",
		.scancode_bits = 0xffff, .repeat_period = 125 },
	[RC_PROTO_SONY12] = { .name = "sony-12",
		.scancode_bits = 0x1f007f, .repeat_period = 100 },
	[RC_PROTO_SONY15] = { .name = "sony-15",
		.scancode_bits = 0xff007f, .repeat_period = 100 },
	[RC_PROTO_SONY20] = { .name = "sony-20",
		.scancode_bits = 0x1fff7f, .repeat_period = 100 },
	[RC_PROTO_NEC] = { .name = "nec",
		.scancode_bits = 0xffff, .repeat_period = 110 },
	[RC_PROTO_NECX] = { .name = "nec-x",
		.scancode_bits = 0xffffff, .repeat_period = 110 },
	[RC_PROTO_NEC32] = { .name = "nec-32",
		.scancode_bits = 0xffffffff, .repeat_period = 110 },
	[RC_PROTO_SANYO] = { .name = "sanyo",
		.scancode_bits = 0x1fffff, .repeat_period = 125 },
	[RC_PROTO_MCIR2_KBD] = { .name = "mcir2-kbd",
		.scancode_bits = 0xffffff, .repeat_period = 100 },
	[RC_PROTO_MCIR2_MSE] = { .name = "mcir2-mse",
		.scancode_bits = 0x1fffff, .repeat_period = 100 },
	[RC_PROTO_RC6_0] = { .name = "rc-6-0",
		.scancode_bits = 0xffff, .repeat_period = 114 },
	[RC_PROTO_RC6_6A_20] = { .name = "rc-6-6a-20",
		.scancode_bits = 0xfffff, .repeat_period = 114 },
	[RC_PROTO_RC6_6A_24] = { .name = "rc-6-6a-24",
		.scancode_bits = 0xffffff, .repeat_period = 114 },
	[RC_PROTO_RC6_6A_32] = { .name = "rc-6-6a-32",
		.scancode_bits = 0xffffffff, .repeat_period = 114 },
	[RC_PROTO_RC6_MCE] = { .name = "rc-6-mce",
		.scancode_bits = 0xffff7fff, .repeat_period = 114 },
	[RC_PROTO_SHARP] = { .name = "sharp",
		.scancode_bits = 0x1fff, .repeat_period = 125 },
	[RC_PROTO_XMP] = { .name = "xmp", .repeat_period = 125 },
	[RC_PROTO_CEC] = { .name = "cec", .repeat_period = 0 },
	[RC_PROTO_IMON] = { .name = "imon",
		.scancode_bits = 0x7fffffff, .repeat_period = 114 },
	[RC_PROTO_RCMM12] = { .name = "rc-mm-12",
		.scancode_bits = 0x00000fff, .repeat_period = 114 },
	[RC_PROTO_RCMM24] = { .name = "rc-mm-24",
		.scancode_bits = 0x00ffffff, .repeat_period = 114 },
	[RC_PROTO_RCMM32] = { .name = "rc-mm-32",
		.scancode_bits = 0xffffffff, .repeat_period = 114 },
	[RC_PROTO_XBOX_DVD] = { .name = "xbox-dvd", .repeat_period = 64 },
};

/* Used to keep track of known keymaps */
static LIST_HEAD(rc_map_list);
static DEFINE_SPINLOCK(rc_map_lock);

static struct rc_map_list *seek_rc_map(const char *name)
{
	struct rc_map_list *map = NULL;

	spin_lock(&rc_map_lock);
	list_for_each_entry(map, &rc_map_list, list) {
		if (!strcmp(name, map->map.name)) {
			spin_unlock(&rc_map_lock);
			return map;
		}
	}
	spin_unlock(&rc_map_lock);

	return NULL;
}

struct rc_map *rc_map_get(const char *name)
{

	struct rc_map_list *map;

	map = seek_rc_map(name);
	if (!map) {
		pr_err("IR keymap %s not found\n", name);
		return NULL;
	}

	pr_info("Registered IR keymap %s\n", map->map.name);

	return &map->map;
}
EXPORT_SYMBOL_GPL(rc_map_get);

int rc_map_register(struct rc_map_list *map)
{
	spin_lock(&rc_map_lock);
	list_add_tail(&map->list, &rc_map_list);
	spin_unlock(&rc_map_lock);
	return 0;
}
EXPORT_SYMBOL_GPL(rc_map_register);

void rc_map_unregister(struct rc_map_list *map)
{
	spin_lock(&rc_map_lock);
	list_del(&map->list);
	spin_unlock(&rc_map_lock);
}
EXPORT_SYMBOL_GPL(rc_map_unregister);


static struct rc_map_table empty[] = {
	{ 0x2a, KEY_COFFEE },
};

static struct rc_map_list empty_map = {
	.map = {
		.scan     = empty,
		.size     = ARRAY_SIZE(empty),
		.rc_proto = RC_PROTO_UNKNOWN,	/* Legacy IR type */
		.name     = RC_MAP_EMPTY,
	}
};

/**
 * scancode_to_u64() - converts scancode in &struct input_keymap_entry
 * @ke: keymap entry containing scancode to be converted.
 * @scancode: pointer to the location where converted scancode should
 *	be stored.
 *
 * This function is a version of input_scancode_to_scalar specialized for
 * rc-core.
 */
static int scancode_to_u64(const struct input_keymap_entry *ke, u64 *scancode)
{
	switch (ke->len) {
	case 1:
		*scancode = *((u8 *)ke->scancode);
		break;

	case 2:
		*scancode = *((u16 *)ke->scancode);
		break;

	case 4:
		*scancode = *((u32 *)ke->scancode);
		break;

	case 8:
		*scancode = *((u64 *)ke->scancode);
		break;

	default:
		return -EINVAL;
	}

	return 0;
}

/**
 * ir_create_table() - initializes a scancode table
 * @dev:	the rc_dev device
 * @rc_map:	the rc_map to initialize
 * @name:	name to assign to the table
 * @rc_proto:	ir type to assign to the new table
 * @size:	initial size of the table
 *
 * This routine will initialize the rc_map and will allocate
 * memory to hold at least the specified number of elements.
 *
 * return:	zero on success or a negative error code
 */
static int ir_create_table(struct rc_dev *dev, struct rc_map *rc_map,
			   const char *name, u64 rc_proto, size_t size)
{
	rc_map->name = kstrdup(name, GFP_KERNEL);
	if (!rc_map->name)
		return -ENOMEM;
	rc_map->rc_proto = rc_proto;
	rc_map->alloc = roundup_pow_of_two(size * sizeof(struct rc_map_table));
	rc_map->size = rc_map->alloc / sizeof(struct rc_map_table);
	rc_map->scan = kmalloc(rc_map->alloc, GFP_KERNEL);
	if (!rc_map->scan) {
		kfree(rc_map->name);
		rc_map->name = NULL;
		return -ENOMEM;
	}

	dev_dbg(&dev->dev, "Allocated space for %u keycode entries (%u bytes)\n",
		rc_map->size, rc_map->alloc);
	return 0;
}

/**
 * ir_free_table() - frees memory allocated by a scancode table
 * @rc_map:	the table whose mappings need to be freed
 *
 * This routine will free memory alloctaed for key mappings used by given
 * scancode table.
 */
static void ir_free_table(struct rc_map *rc_map)
{
	rc_map->size = 0;
	kfree(rc_map->name);
	rc_map->name = NULL;
	kfree(rc_map->scan);
	rc_map->scan = NULL;
}

/**
 * ir_resize_table() - resizes a scancode table if necessary
 * @dev:	the rc_dev device
 * @rc_map:	the rc_map to resize
 * @gfp_flags:	gfp flags to use when allocating memory
 *
 * This routine will shrink the rc_map if it has lots of
 * unused entries and grow it if it is full.
 *
 * return:	zero on success or a negative error code
 */
static int ir_resize_table(struct rc_dev *dev, struct rc_map *rc_map,
			   gfp_t gfp_flags)
{
	unsigned int oldalloc = rc_map->alloc;
	unsigned int newalloc = oldalloc;
	struct rc_map_table *oldscan = rc_map->scan;
	struct rc_map_table *newscan;

	if (rc_map->size == rc_map->len) {
		/* All entries in use -> grow keytable */
		if (rc_map->alloc >= IR_TAB_MAX_SIZE)
			return -ENOMEM;

		newalloc *= 2;
		dev_dbg(&dev->dev, "Growing table to %u bytes\n", newalloc);
	}

	if ((rc_map->len * 3 < rc_map->size) && (oldalloc > IR_TAB_MIN_SIZE)) {
		/* Less than 1/3 of entries in use -> shrink keytable */
		newalloc /= 2;
		dev_dbg(&dev->dev, "Shrinking table to %u bytes\n", newalloc);
	}

	if (newalloc == oldalloc)
		return 0;

	newscan = kmalloc(newalloc, gfp_flags);
	if (!newscan)
		return -ENOMEM;

	memcpy(newscan, rc_map->scan, rc_map->len * sizeof(struct rc_map_table));
	rc_map->scan = newscan;
	rc_map->alloc = newalloc;
	rc_map->size = rc_map->alloc / sizeof(struct rc_map_table);
	kfree(oldscan);
	return 0;
}

/**
 * ir_update_mapping() - set a keycode in the scancode->keycode table
 * @dev:	the struct rc_dev device descriptor
 * @rc_map:	scancode table to be adjusted
 * @index:	index of the mapping that needs to be updated
 * @new_keycode: the desired keycode
 *
 * This routine is used to update scancode->keycode mapping at given
 * position.
 *
 * return:	previous keycode assigned to the mapping
 *
 */
static unsigned int ir_update_mapping(struct rc_dev *dev,
				      struct rc_map *rc_map,
				      unsigned int index,
				      unsigned int new_keycode)
{
	u32 old_keycode = rc_map->scan[index].keycode;
	int i;

	/* Did the user wish to remove the mapping? */
	if (new_keycode == KEY_RESERVED || new_keycode == KEY_UNKNOWN) {
		dev_dbg(&dev->dev, "#%d: Deleting scan 0x%04llx\n",
			index, rc_map->scan[index].scancode);
		rc_map->len--;
		memmove(&rc_map->scan[index], &rc_map->scan[index+ 1],
			(rc_map->len - index) * sizeof(struct rc_map_table));
	} else {
		dev_dbg(&dev->dev, "#%d: %s scan 0x%04llx with key 0x%04x\n",
			index,
			old_keycode == KEY_RESERVED ? "New" : "Replacing",
			rc_map->scan[index].scancode, new_keycode);
		rc_map->scan[index].keycode = new_keycode;
	}

	if (old_keycode != KEY_RESERVED) {
		/* A previous mapping was updated... */
		/* ... but another scancode might use the same keycode */
		for (i = 0; i < rc_map->len; i++) {
			if (rc_map->scan[i].keycode == old_keycode) {
				break;
			}
		}

		/* Possibly shrink the keytable, failure is not a problem */
		ir_resize_table(dev, rc_map, GFP_ATOMIC);
	}

	return old_keycode;
}

/**
 * ir_establish_scancode() - set a keycode in the scancode->keycode table
 * @dev:	the struct rc_dev device descriptor
 * @rc_map:	scancode table to be searched
 * @scancode:	the desired scancode
 * @resize:	controls whether we allowed to resize the table to
 *		accommodate not yet present scancodes
 *
 * This routine is used to locate given scancode in rc_map.
 * If scancode is not yet present the routine will allocate a new slot
 * for it.
 *
 * return:	index of the mapping containing scancode in question
 *		or -1U in case of failure.
 */
static unsigned int ir_establish_scancode(struct rc_dev *dev,
					  struct rc_map *rc_map,
					  u64 scancode, bool resize)
{
	unsigned int i;

	/*
	 * Unfortunately, some hardware-based IR decoders don't provide
	 * all bits for the complete IR code. In general, they provide only
	 * the command part of the IR code. Yet, as it is possible to replace
	 * the provided IR with another one, it is needed to allow loading
	 * IR tables from other remotes. So, we support specifying a mask to
	 * indicate the valid bits of the scancodes.
	 */
	if (dev->scancode_mask)
		scancode &= dev->scancode_mask;

	/* First check if we already have a mapping for this ir command */
	for (i = 0; i < rc_map->len; i++) {
		if (rc_map->scan[i].scancode == scancode)
			return i;

		/* Keytable is sorted from lowest to highest scancode */
		if (rc_map->scan[i].scancode >= scancode)
			break;
	}

	/* No previous mapping found, we might need to grow the table */
	if (rc_map->size == rc_map->len) {
		if (!resize || ir_resize_table(dev, rc_map, GFP_ATOMIC))
			return -1U;
	}

	/* i is the proper index to insert our new keycode */
	if (i < rc_map->len)
		memmove(&rc_map->scan[i + 1], &rc_map->scan[i],
			(rc_map->len - i) * sizeof(struct rc_map_table));
	rc_map->scan[i].scancode = scancode;
	rc_map->scan[i].keycode = KEY_RESERVED;
	rc_map->len++;

	return i;
}

/**
 * ir_setkeycode() - set a keycode in the scancode->keycode table
 * @idev:	the struct input_dev device descriptor
 * @ke:		Input keymap entry
 * @old_keycode: result
 *
 * This routine is used to handle evdev EVIOCSKEY ioctl.
 *
 * return:	-EINVAL if the keycode could not be inserted, otherwise zero.
 */
static int ir_setkeycode(struct input_dev *idev,
			 const struct input_keymap_entry *ke,
			 unsigned int *old_keycode)
{
	struct rc_dev *rdev = input_get_drvdata(idev);
	struct rc_map *rc_map = &rdev->rc_map;
	unsigned int index;
	u64 scancode;
	int retval = 0;
	unsigned long flags;

	spin_lock_irqsave(&rc_map->lock, flags);

	if (ke->flags & INPUT_KEYMAP_BY_INDEX) {
		index = ke->index;
		if (index >= rc_map->len) {
			retval = -EINVAL;
			goto out;
		}
	} else {
		retval = scancode_to_u64(ke, &scancode);
		if (retval)
			goto out;

		index = ir_establish_scancode(rdev, rc_map, scancode, true);
		if (index >= rc_map->len) {
			retval = -ENOMEM;
			goto out;
		}
	}

	*old_keycode = ir_update_mapping(rdev, rc_map, index, ke->keycode);

out:
	spin_unlock_irqrestore(&rc_map->lock, flags);
	return retval;
}

/**
 * ir_setkeytable() - sets several entries in the scancode->keycode table
 * @dev:	the struct rc_dev device descriptor
 * @from:	the struct rc_map to copy entries from
 *
 * This routine is used to handle table initialization.
 *
 * return:	-ENOMEM if all keycodes could not be inserted, otherwise zero.
 */
static int ir_setkeytable(struct rc_dev *dev, const struct rc_map *from)
{
	struct rc_map *rc_map = &dev->rc_map;
	unsigned int i, index;
	int rc;

	rc = ir_create_table(dev, rc_map, from->name, from->rc_proto,
			     from->size);
	if (rc)
		return rc;

	for (i = 0; i < from->size; i++) {
		index = ir_establish_scancode(dev, rc_map,
					      from->scan[i].scancode, false);
		if (index >= rc_map->len) {
			rc = -ENOMEM;
			break;
		}

		ir_update_mapping(dev, rc_map, index,
				  from->scan[i].keycode);
	}

	if (rc)
		ir_free_table(rc_map);

	return rc;
}

static int rc_map_cmp(const void *key, const void *elt)
{
	const u64 *scancode = key;
	const struct rc_map_table *e = elt;

	if (*scancode < e->scancode)
		return -1;
	else if (*scancode > e->scancode)
		return 1;
	return 0;
}

/**
 * ir_lookup_by_scancode() - locate mapping by scancode
 * @rc_map:	the struct rc_map to search
 * @scancode:	scancode to look for in the table
 *
 * This routine performs binary search in RC keykeymap table for
 * given scancode.
 *
 * return:	index in the table, -1U if not found
 */
static unsigned int ir_lookup_by_scancode(const struct rc_map *rc_map,
					  u64 scancode)
{
	struct rc_map_table *res;

	res = bsearch(&scancode, rc_map->scan, rc_map->len,
		      sizeof(struct rc_map_table), rc_map_cmp);
	if (!res)
		return -1U;
	else
		return res - rc_map->scan;
}

/**
 * ir_getkeycode() - get a keycode from the scancode->keycode table
 * @idev:	the struct input_dev device descriptor
 * @ke:		Input keymap entry
 *
 * This routine is used to handle evdev EVIOCGKEY ioctl.
 *
 * return:	always returns zero.
 */
static int ir_getkeycode(struct input_dev *idev,
			 struct input_keymap_entry *ke)
{
	struct rc_dev *rdev = input_get_drvdata(idev);
	struct rc_map *rc_map = &rdev->rc_map;
	struct rc_map_table *entry;
	unsigned long flags;
	unsigned int index;
	u64 scancode;
	int retval;

	spin_lock_irqsave(&rc_map->lock, flags);

	if (ke->flags & INPUT_KEYMAP_BY_INDEX) {
		index = ke->index;
	} else {
		retval = scancode_to_u64(ke, &scancode);
		if (retval)
			goto out;

		index = ir_lookup_by_scancode(rc_map, scancode);
	}

	if (index < rc_map->len) {
		entry = &rc_map->scan[index];

		ke->index = index;
		ke->keycode = entry->keycode;
		ke->len = sizeof(entry->scancode);
		memcpy(ke->scancode, &entry->scancode, sizeof(entry->scancode));
	} else if (!(ke->flags & INPUT_KEYMAP_BY_INDEX)) {
		/*
		 * We do not really know the valid range of scancodes
		 * so let's respond with KEY_RESERVED to anything we
		 * do not have mapping for [yet].
		 */
		ke->index = index;
		ke->keycode = KEY_RESERVED;
	} else {
		retval = -EINVAL;
		goto out;
	}

	retval = 0;

out:
	spin_unlock_irqrestore(&rc_map->lock, flags);
	return retval;
}

/**
 * rc_g_keycode_from_table() - gets the keycode that corresponds to a scancode
 * @dev:	the struct rc_dev descriptor of the device
 * @scancode:	the scancode to look for
 *
 * This routine is used by drivers which need to convert a scancode to a
 * keycode. Normally it should not be used since drivers should have no
 * interest in keycodes.
 *
 * return:	the corresponding keycode, or KEY_RESERVED
 */
u32 rc_g_keycode_from_table(struct rc_dev *dev, u64 scancode)
{
	struct rc_map *rc_map = &dev->rc_map;
	unsigned int keycode;
	unsigned int index;
	unsigned long flags;

	spin_lock_irqsave(&rc_map->lock, flags);

	index = ir_lookup_by_scancode(rc_map, scancode);
	keycode = index < rc_map->len ?
			rc_map->scan[index].keycode : KEY_RESERVED;

	spin_unlock_irqrestore(&rc_map->lock, flags);

	if (keycode != KEY_RESERVED)
		dev_dbg(&dev->dev, "%s: scancode 0x%04llx keycode 0x%02x\n",
			dev->device_name, scancode, keycode);

	return keycode;
}
EXPORT_SYMBOL_GPL(rc_g_keycode_from_table);

/**
 * ir_do_keyup() - internal function to signal the release of a keypress
 * @dev:	the struct rc_dev descriptor of the device
 * @sync:	whether or not to call input_sync
 *
 * This function is used internally to release a keypress, it must be
 * called with keylock held.
 */
static void ir_do_keyup(struct rc_dev *dev, bool sync)
{
	if (!dev->keypressed)
		return;

	dev_dbg(&dev->dev, "keyup key 0x%04x\n", dev->last_keycode);
	input_report_key(dev->input_dev, dev->last_keycode, 0);
	if (sync)
		input_sync(dev->input_dev);
	dev->keypressed = false;
}

/**
 * rc_keyup() - signals the release of a keypress
 * @dev:	the struct rc_dev descriptor of the device
 *
 * This routine is used to signal that a key has been released on the
 * remote control.
 */
void rc_keyup(struct rc_dev *dev)
{
	unsigned long flags;

	spin_lock_irqsave(&dev->keylock, flags);
	ir_do_keyup(dev, true);
	spin_unlock_irqrestore(&dev->keylock, flags);
}
EXPORT_SYMBOL_GPL(rc_keyup);

/**
 * ir_timer_keyup() - generates a keyup event after a timeout
 *
 * @t:		a pointer to the struct timer_list
 *
 * This routine will generate a keyup event some time after a keydown event
 * is generated when no further activity has been detected.
 */
static void ir_timer_keyup(struct timer_list *t)
{
	struct rc_dev *dev = from_timer(dev, t, timer_keyup);
	unsigned long flags;

	/*
	 * ir->keyup_jiffies is used to prevent a race condition if a
	 * hardware interrupt occurs at this point and the keyup timer
	 * event is moved further into the future as a result.
	 *
	 * The timer will then be reactivated and this function called
	 * again in the future. We need to exit gracefully in that case
	 * to allow the input subsystem to do its auto-repeat magic or
	 * a keyup event might follow immediately after the keydown.
	 */
	spin_lock_irqsave(&dev->keylock, flags);
	if (time_is_before_eq_jiffies(dev->keyup_jiffies))
		ir_do_keyup(dev, true);
	spin_unlock_irqrestore(&dev->keylock, flags);
}

static unsigned int repeat_period(int protocol)
{
	if (protocol >= ARRAY_SIZE(protocols))
		return 100;

	return protocols[protocol].repeat_period;
}

/**
 * rc_repeat() - signals that a key is still pressed
 * @dev:	the struct rc_dev descriptor of the device
 *
 * This routine is used by IR decoders when a repeat message which does
 * not include the necessary bits to reproduce the scancode has been
 * received.
 */
void rc_repeat(struct rc_dev *dev)
{
	unsigned long flags;
	unsigned int timeout = usecs_to_jiffies(dev->timeout) +
		msecs_to_jiffies(repeat_period(dev->last_protocol));

	spin_lock_irqsave(&dev->keylock, flags);

	if (dev->last_scancode <= U32_MAX) {
		input_event(dev->input_dev, EV_MSC, MSC_SCAN,
			    (u32)dev->last_scancode);
		input_sync(dev->input_dev);
	}

	if (dev->keypressed) {
		dev->keyup_jiffies = jiffies + timeout;
		mod_timer(&dev->timer_keyup, dev->keyup_jiffies);
	}

	spin_unlock_irqrestore(&dev->keylock, flags);
}
EXPORT_SYMBOL_GPL(rc_repeat);

/**
 * ir_do_keydown() - internal function to process a keypress
 * @dev:	the struct rc_dev descriptor of the device
 * @protocol:	the protocol of the keypress
 * @scancode:   the scancode of the keypress
 * @keycode:    the keycode of the keypress
 * @toggle:     the toggle value of the keypress
 *
 * This function is used internally to register a keypress, it must be
 * called with keylock held.
 */
static void ir_do_keydown(struct rc_dev *dev, enum rc_proto protocol,
			  u64 scancode, u32 keycode, u8 toggle)
{
	bool new_event = (!dev->keypressed		 ||
			  dev->last_protocol != protocol ||
			  dev->last_scancode != scancode ||
			  dev->last_toggle   != toggle);

	if (new_event && dev->keypressed)
		ir_do_keyup(dev, false);

	if (scancode <= U32_MAX)
		input_event(dev->input_dev, EV_MSC, MSC_SCAN, (u32)scancode);

	dev->last_protocol = protocol;
	dev->last_scancode = scancode;
	dev->last_toggle = toggle;
	dev->last_keycode = keycode;

	if (new_event && keycode != KEY_RESERVED) {
		/* Register a keypress */
		dev->keypressed = true;

		dev_dbg(&dev->dev, "%s: key down event, key 0x%04x, protocol 0x%04x, scancode 0x%08llx\n",
			dev->device_name, keycode, protocol, scancode);
		input_report_key(dev->input_dev, keycode, 1);
	}

	input_sync(dev->input_dev);
}

/**
 * rc_keydown() - generates input event for a key press
 * @dev:	the struct rc_dev descriptor of the device
 * @protocol:	the protocol for the keypress
 * @scancode:	the scancode for the keypress
 * @toggle:     the toggle value (protocol dependent, if the protocol doesn't
 *              support toggle values, this should be set to zero)
 *
 * This routine is used to signal that a key has been pressed on the
 * remote control.
 */
void rc_keydown(struct rc_dev *dev, enum rc_proto protocol, u64 scancode,
		u8 toggle)
{
	unsigned long flags;
	u32 keycode = rc_g_keycode_from_table(dev, scancode);

	spin_lock_irqsave(&dev->keylock, flags);
	ir_do_keydown(dev, protocol, scancode, keycode, toggle);

	if (dev->keypressed) {
		dev->keyup_jiffies = jiffies + usecs_to_jiffies(dev->timeout) +
			msecs_to_jiffies(repeat_period(protocol));
		mod_timer(&dev->timer_keyup, dev->keyup_jiffies);
	}
	spin_unlock_irqrestore(&dev->keylock, flags);
}
EXPORT_SYMBOL_GPL(rc_keydown);

int rc_open(struct rc_dev *rdev)
{
	int rval = 0;

	if (!rdev)
		return -EINVAL;

	mutex_lock(&rdev->lock);

	if (!rdev->registered) {
		rval = -ENODEV;
	} else {
		if (!rdev->users++ && rdev->open)
			rval = rdev->open(rdev);

		if (rval)
			rdev->users--;
	}

	mutex_unlock(&rdev->lock);

	return rval;
}

static int ir_open(struct input_dev *idev)
{
	struct rc_dev *rdev = input_get_drvdata(idev);

	return rc_open(rdev);
}

void rc_close(struct rc_dev *rdev)
{
	if (rdev) {
		mutex_lock(&rdev->lock);

		if (!--rdev->users && rdev->close && rdev->registered)
			rdev->close(rdev);

		mutex_unlock(&rdev->lock);
	}
}

static void ir_close(struct input_dev *idev)
{
	struct rc_dev *rdev = input_get_drvdata(idev);
	rc_close(rdev);
}

/*
 * These are the protocol textual descriptions that are
 * used by the sysfs protocols file. Note that the order
 * of the entries is relevant.
 */
static const struct {
	u64	type;
	const char	*name;
	const char	*module_name;
} proto_names[] = {
	{ RC_PROTO_BIT_NONE,	"none",		NULL			},
	{ RC_PROTO_BIT_OTHER,	"other",	NULL			},
	{ RC_PROTO_BIT_UNKNOWN,	"unknown",	NULL			},
	{ RC_PROTO_BIT_RC5 |
	  RC_PROTO_BIT_RC5X_20,	"rc-5",		"ir-rc5-decoder"	},
	{ RC_PROTO_BIT_NEC |
	  RC_PROTO_BIT_NECX |
	  RC_PROTO_BIT_NEC32,	"nec",		"ir-nec-decoder"	},
	{ RC_PROTO_BIT_RC6_0 |
	  RC_PROTO_BIT_RC6_6A_20 |
	  RC_PROTO_BIT_RC6_6A_24 |
	  RC_PROTO_BIT_RC6_6A_32 |
	  RC_PROTO_BIT_RC6_MCE,	"rc-6",		"ir-rc6-decoder"	},
	{ RC_PROTO_BIT_JVC,	"jvc",		"ir-jvc-decoder"	},
	{ RC_PROTO_BIT_SONY12 |
	  RC_PROTO_BIT_SONY15 |
	  RC_PROTO_BIT_SONY20,	"sony",		"ir-sony-decoder"	},
	{ RC_PROTO_BIT_RC5_SZ,	"rc-5-sz",	"ir-rc5-decoder"	},
	{ RC_PROTO_BIT_SANYO,	"sanyo",	"ir-sanyo-decoder"	},
	{ RC_PROTO_BIT_SHARP,	"sharp",	"ir-sharp-decoder"	},
	{ RC_PROTO_BIT_MCIR2_KBD |
	  RC_PROTO_BIT_MCIR2_MSE, "mce_kbd",	"ir-mce_kbd-decoder"	},
	{ RC_PROTO_BIT_XMP,	"xmp",		"ir-xmp-decoder"	},
	{ RC_PROTO_BIT_CEC,	"cec",		NULL			},
	{ RC_PROTO_BIT_IMON,	"imon",		"ir-imon-decoder"	},
	{ RC_PROTO_BIT_RCMM12 |
	  RC_PROTO_BIT_RCMM24 |
	  RC_PROTO_BIT_RCMM32,	"rc-mm",	"ir-rcmm-decoder"	},
	{ RC_PROTO_BIT_XBOX_DVD, "xbox-dvd",	NULL			},
};

void ir_raw_load_modules(u64 *protocols)
{
	u64 available;
	int i, ret;

	for (i = 0; i < ARRAY_SIZE(proto_names); i++) {
		if (proto_names[i].type == RC_PROTO_BIT_NONE ||
		    proto_names[i].type & (RC_PROTO_BIT_OTHER |
					   RC_PROTO_BIT_UNKNOWN))
			continue;

		available = ir_raw_get_allowed_protocols();
		if (!(*protocols & proto_names[i].type & ~available))
			continue;

		if (!proto_names[i].module_name) {
			pr_err("Can't enable IR protocol %s\n",
			       proto_names[i].name);
			*protocols &= ~proto_names[i].type;
			continue;
		}

		usleep(20000);
		available = ir_raw_get_allowed_protocols();
		if (!(*protocols & proto_names[i].type & ~available))
			continue;

		pr_err("Loaded IR protocol module %s, but protocol %s still not available\n",
		       proto_names[i].module_name,
		       proto_names[i].name);
		*protocols &= ~proto_names[i].type;
	}
}

struct rc_dev *rc_allocate_device(enum rc_driver_type type)
{
	struct rc_dev *dev;

	dev = (struct rc_dev *)kzalloc(sizeof(*dev), GFP_KERNEL);
	if (!dev)
		return NULL;

	if (type != RC_DRIVER_IR_RAW_TX) {
		dev->input_dev = input_allocate_device();
		if (!dev->input_dev) {
			kfree(dev);
			return NULL;
		}

		dev->input_dev->getkeycode = ir_getkeycode;
		dev->input_dev->setkeycode = ir_setkeycode;
		input_set_drvdata(dev->input_dev, dev);

		dev->timeout = IR_DEFAULT_TIMEOUT;
		timer_setup(&dev->timer_keyup, ir_timer_keyup, 0);

		spin_lock_init(&dev->rc_map.lock);
		spin_lock_init(&dev->keylock);
	}
	mutex_init(&dev->lock);

	dev->driver_type = type;

	return dev;
}
EXPORT_SYMBOL_GPL(rc_allocate_device);

void rc_free_device(struct rc_dev *dev)
{
	if (!dev)
		return;

	input_free_device(dev->input_dev);

	kfree(dev);
}
EXPORT_SYMBOL_GPL(rc_free_device);

static int rc_prepare_rx_device(struct rc_dev *dev)
{
	int rc;
	struct rc_map *rc_map;
	u64 rc_proto;

	if (!dev->map_name)
		return -EINVAL;

	rc_map = rc_map_get(dev->map_name);
	if (!rc_map)
		rc_map = rc_map_get(RC_MAP_EMPTY);
	if (!rc_map || !rc_map->scan || rc_map->size == 0)
		return -EINVAL;

	rc = ir_setkeytable(dev, rc_map);
	if (rc)
		return rc;

	rc_proto = BIT_ULL(rc_map->rc_proto);

	if (dev->driver_type == RC_DRIVER_SCANCODE && !dev->change_protocol)
		dev->enabled_protocols = dev->allowed_protocols;

	if (dev->driver_type == RC_DRIVER_IR_RAW)
		ir_raw_load_modules(&rc_proto);

	if (dev->change_protocol) {
		rc = dev->change_protocol(dev, &rc_proto);
		if (rc < 0)
			goto out_table;
		dev->enabled_protocols = rc_proto;
	}

	if (dev->open)
		dev->input_dev->open = ir_open;
	if (dev->close)
		dev->input_dev->close = ir_close;

	return 0;

out_table:
	ir_free_table(&dev->rc_map);

	return rc;
}

static int rc_setup_rx_device(struct rc_dev *dev)
{
	int rc;

	/* rc_open will be called here */
	rc = input_register_device(dev->input_dev);
	if (rc)
		return rc;

	return 0;
}

static void rc_free_rx_device(struct rc_dev *dev)
{
	if (!dev)
		return;

	if (dev->input_dev) {
		input_unregister_device(dev->input_dev);
		dev->input_dev = NULL;
	}

	ir_free_table(&dev->rc_map);
}

int rc_register_device(struct rc_dev *dev)
{
	const char *path;
	int attr = 0;
	int minor;
	int rc;

	if (!dev)
		return -EINVAL;

	if (dev->driver_type == RC_DRIVER_IR_RAW) {
		rc = ir_raw_event_prepare(dev);
		if (rc < 0)
			goto out_minor;
	}

	if (dev->driver_type != RC_DRIVER_IR_RAW_TX) {
		rc = rc_prepare_rx_device(dev);
		if (rc)
			goto out_raw;
	}

	dev->registered = true;

	/*
	 * once the the input device is registered in rc_setup_rx_device,
	 * userspace can open the input device and rc_open() will be called
	 * as a result. This results in driver code being allowed to submit
	 * keycodes with rc_keydown, so lirc must be registered first.
	 */
	if (dev->driver_type != RC_DRIVER_IR_RAW_TX) {
		rc = rc_setup_rx_device(dev);
		if (rc)
			goto out_lirc;
	}

	if (dev->driver_type == RC_DRIVER_IR_RAW) {
		rc = ir_raw_event_register(dev);
		if (rc < 0)
			goto out_rx;
	}

	return 0;

out_rx:
	rc_free_rx_device(dev);
out_lirc:
out_dev:
out_rx_free:
	ir_free_table(&dev->rc_map);
out_raw:
	ir_raw_event_free(dev);
out_minor:
	return rc;
}
EXPORT_SYMBOL_GPL(rc_register_device);

void rc_unregister_device(struct rc_dev *dev)
{
	if (!dev)
		return;

	if (dev->driver_type == RC_DRIVER_IR_RAW)
		ir_raw_event_unregister(dev);

	del_timer_sync(&dev->timer_keyup);

	mutex_lock(&dev->lock);
	if (dev->users && dev->close)
		dev->close(dev);
	dev->registered = false;
	mutex_unlock(&dev->lock);

	rc_free_rx_device(dev);

	if (!dev->managed_alloc)
		rc_free_device(dev);
}

EXPORT_SYMBOL_GPL(rc_unregister_device);

/*
 * Init/exit code for the module. Basically, creates/removes /sys/class/rc
 */

static int rc_core_init(void)
{
	rc_map_register(&empty_map);
	return 0;
}

module_driver(rc_core, rc_core_init, NULL, 0)
