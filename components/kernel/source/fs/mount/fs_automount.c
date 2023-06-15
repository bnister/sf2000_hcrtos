#define ELOG_OUTPUT_LVL ELOG_LVL_ERROR
#define LOG_TAG "AUTOMNT"

#include <string.h>
#include <stdlib.h>
#include <hcuapi/sys-blocking-notify.h>
#include <kernel/list.h>
#include <kernel/module.h>
#include <kernel/elog.h>
#include <sys/mount.h>
#include <nuttx/wqueue.h>
#include <linux/ctype.h>
#include <kernel/notify.h>

static LIST_HEAD(__mounters);

struct automounter_state_s {
	struct list_head list;
	char devname[DISK_NAME_LEN];
	bool notify_umount;
	struct work_s umount_dwork;
};

static void autoumount_fs(void *arg)
{
	struct removable_notify_info *info = (struct removable_notify_info *)arg;
	struct automounter_state_s *curr, *next;
	struct automounter_state_s *state = NULL;
	char mntname[DISK_NAME_LEN + 7];

	list_for_each_entry_safe (curr, next, &__mounters, list) {
		if(!strcmp(info->devname, curr->devname)) {
			state = curr;
			break;
		}
	}

	if (state == NULL)
		return;

	if (state->notify_umount == false) {
		sys_notify_event(USB_MSC_NOTIFY_UMOUNT, (void *)state->devname);
		state->notify_umount = true;
	}

	snprintf(mntname, sizeof(mntname), "/media/%s", info->devname);
	if (umount(mntname) < 0) {
		log_e("umount %s fail, try 2 seconds later\n", mntname);
		sys_notify_event(USB_MSC_NOTIFY_UMOUNT_FAIL, (void *)state->devname);
		work_queue(LPWORK, &state->umount_dwork, autoumount_fs, (void *)state->devname, 2000);
		return;
	}

	log_e("umount %s\n", mntname);
	list_del_init(&state->list);
	free(state);

	return;
}

static void automount_fs(struct removable_notify_info *info)
{
	struct automounter_state_s *state, *next;
	char devname[DISK_NAME_LEN + 7];
	char mntname[DISK_NAME_LEN + 7];

	list_for_each_entry_safe (state, next, &__mounters, list) {
		assert(strcmp(info->devname, state->devname));
	}

	snprintf(devname, sizeof(devname), "/dev/%s", info->devname);
	snprintf(mntname, sizeof(mntname), "/media/%s", info->devname);

	if (mount(devname, mntname, "vfat", 0, NULL) < 0) {
		log_e("mount vfat fail %s, try ntfs\n", devname);
		if (mount(devname, mntname, "ntfs", 0, NULL) < 0) {
			sys_notify_event(USB_MSC_NOTIFY_MOUNT_FAIL, (void *)info->devname);
			log_e("mount ntfs fail %s\n", devname);
			return;
		}
	}

	state = malloc(sizeof(*state));
	if (!state) {
		umount(mntname);
		log_e("No memory!\n");
		return;
	}

	memset(state, 0, sizeof(*state));

	INIT_LIST_HEAD(&state->list);
	memcpy(state->devname, info->devname, sizeof(state->devname));
	list_add_tail(&state->list, &__mounters);

	log_e("mounted %s on %s\n", info->devname, mntname);

	sys_notify_event(USB_MSC_NOTIFY_MOUNT, (void *)state->devname);
}

static int automount_notify(struct notifier_block *self,
			       unsigned long action, void *param)
{
	switch (action) {
	case USB_MSC_NOTIFY_CONNECT:
	case SDMMC_NOTIFY_CONNECT:
		automount_fs((struct removable_notify_info *)param);
		break;
	case USB_MSC_NOTIFY_DISCONNECT:
	case SDMMC_NOTIFY_DISCONNECT:
		autoumount_fs((struct removable_notify_info *)param);
		break;
	}

	return NOTIFY_OK;
}

static struct notifier_block automount_nb = {
	.notifier_call = automount_notify,
};

static int automount_initialize(void)
{
	sys_register_notify(&automount_nb);
	return 0;
}

module_system(automount, automount_initialize, NULL, 4)
