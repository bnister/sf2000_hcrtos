#include <generated/br2_autoconf.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <kernel/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <kernel/module.h>
#include <kernel/lib/console.h>
#include <hcuapi/sys-blocking-notify.h>
#include <hcfota.h>
#include <kernel/completion.h>
#include <kernel/notify.h>
#include <linux/notifier.h>
#include <hcuapi/persistentmem.h>
#include <hcuapi/input.h>
#include <poll.h>
#include <kernel/lib/fdt_api.h>
#include <kernel/lib/libfdt/libfdt.h>
#include "usbd_upgrade.h"
#include "show_upgrade_way.h"

struct hcfota_upgrade_key{
    int enable;

    int adc_key_fd;
    char adc_key_path[32];
    int adc_key_value;
};

#define HCFOTA_BIN "hcfota.bin"
#define HCFOTA_SYSTEM_DATA_BIN "system_data.bin"
static uint32_t HCFOTA_TIMEOUT = CONFIG_BOOT_HCFOTA_TIMEOUT;

static struct completion upgrade_done;
static struct completion storage_ready;
static struct hcfota_upgrade_key upgrade_key = {0};

unsigned int mode;
int do_hcfota_upgrade(unsigned int ota_mode);

static int get_upgrade_key(void)
{
	static int np = -1;
	const char *status;
	int ret = 0;

    np = fdt_get_node_offset_by_path("/hcrtos/hcfota-upgrade");
    if(np < 0){
        printf("%s:%d: fdt_get_node_offset_by_path failed\n", __func__, __LINE__);
        return -1;
    }

    ret = fdt_get_property_string_index(np, "status", 0, &status);
    if(ret != 0){
        printf("%s:%d: fdt_get_property_u_32_index ir_key failed\n", __func__, __LINE__);
    }

    if(strcmp(status, "okay")){
        upgrade_key.enable = 0;
        printf("%s:%d: hcfota-upgrade is disable\n", __func__, __LINE__);
        return -1;
    }else{
        upgrade_key.enable = 1;
    }

    ret = fdt_get_property_u_32_index(np, "adc_key", 0, &upgrade_key.adc_key_value);
    if(ret != 0){
        printf("%s:%d: fdt_get_property_u_32_index ir_key failed\n", __func__, __LINE__);
    }

    printf("%s:%d: adc_key=%d\n", __func__, __LINE__, upgrade_key.adc_key_value);

    return 0;
}

int upgrade_detect_key(void)
{
	int fd = 0;
	struct input_event t = {0};
	int detect = 0;
	int time = 150;
	int ret = 0;

    upgrade_key.adc_key_fd = -1;

    ret = get_upgrade_key();
    if(ret != 0){
        printf("%s:%d: get_upgrade_key failed\n", __func__, __LINE__);
        goto end;
    }

    if(upgrade_key.enable == 0){
        printf("%s:%d: hcfota-upgrade key is disable\n", __func__, __LINE__);
        goto end;
    }

#if defined(CONFIG_RC_HC) && defined(CONFIG_KEY_ADC)
    strcpy(upgrade_key.adc_key_path, "/dev/input/event1");
#elif defined(CONFIG_KEY_ADC)
    strcpy(upgrade_key.adc_key_path, "/dev/input/event0");
#endif

    printf("%s:%d: adc_key_path=%s\n", __func__, __LINE__, upgrade_key.adc_key_path);

    if(strlen(upgrade_key.adc_key_path)){
        upgrade_key.adc_key_fd = open(upgrade_key.adc_key_path, O_RDONLY);
        if(upgrade_key.adc_key_fd < 0){
            printf("%s:%d: open %s failed\n", __func__, __LINE__, upgrade_key.adc_key_path);
            goto end;
        }
    }

    while(time--){
        if(upgrade_key.adc_key_fd >= 0){
            if (read(upgrade_key.adc_key_fd, &t, sizeof(t)) == sizeof(t)) {
                printf("adc_key: type:%d, code:%d, value:%ld\n", t.type, t.code, t.value);
                if (t.type == EV_KEY) {
                    if (t.code == upgrade_key.adc_key_value && t.value) {
                        printf("%s:%d: adc upgrade key is Pressed\n", __func__, __LINE__);
                        detect = 1;
                        break;
                    }
                }
            }
        }

        usleep(1000);
    }

    if(time <= 0){
        printf("%s:%d: detect upgrade key timeout\n", __func__, __LINE__);
    }

end:
    if(upgrade_key.adc_key_fd < 0){
        close(upgrade_key.adc_key_fd);
    }

	return detect;
}


int upgrade_force(void)
{
	unsigned long ota_mode = 0;

	ota_mode |= hcfota_reboot_ota_detect_mode_priority(HCFOTA_REBOOT_OTA_DETECT_USB_HOST, 0);
	ota_mode |= hcfota_reboot_ota_detect_mode_priority(HCFOTA_REBOOT_OTA_DETECT_SD, 1);
	ota_mode |= hcfota_reboot_ota_detect_mode_priority(HCFOTA_REBOOT_OTA_DETECT_NETWORK, 2);
	ota_mode |= hcfota_reboot_ota_detect_mode_priority(HCFOTA_REBOOT_OTA_DETECT_USB_DEVICE, 3);

	return do_hcfota_upgrade(ota_mode);
}

static int get_externel_flash_type(void)
{
    hcfota_external_flash_type_e flash_type = HCFOTA_EXTERNAL_FLASH_NULL;

#ifdef CONFIG_BOOT_UPGRADE_EXTERNAL_FLASH_EMMC
    flash_type = HCFOTA_EXTERNAL_FLASH_EMMC;
#endif

#ifdef CONFIG_BOOT_UPGRADE_EXTERNAL_FLASH_NAND
    flash_type = HCFOTA_EXTERNAL_FLASH_NAND;
#endif

#ifdef CONFIG_BOOT_UPGRADE_EXTERNAL_FLASH_SDCARD
    flash_type = HCFOTA_EXTERNAL_FLASH_SDCARD;
#endif

    return flash_type;
}

static unsigned long get_ota_detect_mode(void)
{
	int fd;
	struct persistentmem_node_create new_node;
	struct persistentmem_node node;
	struct sysdata sysdata = { 0 };

	fd = open("/dev/persistentmem", O_SYNC | O_RDWR);
	if (fd < 0) {
		printf("open /dev/persistentmem failed\n");
		return HCFOTA_REBOOT_OTA_DETECT_NONE;
	}

	node.id = PERSISTENTMEM_NODE_ID_SYSDATA;
	node.offset = 0;
	node.size = sizeof(struct sysdata);
	node.buf = &sysdata;
	if (ioctl(fd, PERSISTENTMEM_IOCTL_NODE_GET, &node) < 0) {
		new_node.id = PERSISTENTMEM_NODE_ID_SYSDATA;
		new_node.size = sizeof(struct sysdata);
		if (ioctl(fd, PERSISTENTMEM_IOCTL_NODE_CREATE, &new_node) < 0) {
			printf("get sysdata failed\n");
			close(fd);
			return HCFOTA_REBOOT_OTA_DETECT_NONE;
		}
	}

	close(fd);

	return sysdata.ota_detect_modes;
}

static int set_ota_detect_mode(unsigned long mode)
{
	int fd;
	struct persistentmem_node node;
	struct sysdata sysdata = { 0 };

	fd = open("/dev/persistentmem", O_SYNC | O_RDWR);
	if (fd < 0) {
		printf("open /dev/persistentmem failed\n");
		return -1;
	}

	sysdata.ota_detect_modes = mode;
	node.id = PERSISTENTMEM_NODE_ID_SYSDATA;
	node.offset = offsetof(struct sysdata, ota_detect_modes);
	node.size = sizeof(sysdata.ota_detect_modes);
	node.buf = &sysdata.ota_detect_modes;
	if (ioctl(fd, PERSISTENTMEM_IOCTL_NODE_PUT, &node) < 0) {
		printf("put sysdata failed\n");
		close(fd);
		return -1;
	}

	return 0;
}


#if defined(CONFIG_BOOT_UPGRADE_SUPPORT_USBHOST) || defined(CONFIG_BOOT_UPGRADE_SUPPORT_SD)

static int usb_mmc_upgrade_main(void *dev)
{
	int ret = 0, i = 0;
	DIR *dirp;
	struct stat st;
	char entryp_path[512];
	char dir_path[512];
	char tmp_buf[4] = { 0 };

	int dir_name_len = 0;
	snprintf(dir_path, sizeof(dir_path), "/media/%s", (char *)dev);

	dirp = opendir(dir_path);
	if (dirp == NULL)
		printf("open dir %s failed\n", dir_path);

	/* Read each directory entry */
	FAR struct dirent *entryp = readdir(dirp);

	for (;;) {
		entryp = readdir(dirp);
		if (entryp == NULL) {
			/* Finished with this directory */
			break;
		}

		memset(entryp_path, 0, sizeof(entryp_path));
		strcat(entryp_path, dir_path);
		strcat(entryp_path, "/");
		strcat(entryp_path, entryp->d_name);

		if ((strncmp("HCFOTA", entryp->d_name, 6) == 0 ) || (strncmp("hcfota", entryp->d_name, 6) == 0)) {
			dir_name_len = strlen(entryp->d_name);
			for (i = 0; i < 3; i++) {
				tmp_buf[i] = entryp->d_name[dir_name_len - 3 + i];
			}
			if (((strncmp(tmp_buf, "bin", 3) == 0) || (strncmp(tmp_buf, "BIN", 3) == 0))) {
				printf("==> upgrade from = %s\n", entryp_path);
#ifdef CONFIG_BOOT_UPGRADE_SHOW_WITH_SCREEN
				init_progress_bar();
				fill_progress_bar(1280, 0xffffffff);
#endif
#ifdef CONFIG_BOOT_UPGRADE_SHOW_WITH_LED
				led_init();
#endif
				ret = hcfota_from_path(entryp_path, hcfota_report, 0);
				if (ret == 0)
					return ret;
				else
					return -HCFOTA_ERR_UPGRADE;
			}
		}
	}

	closedir(dirp);

	return -ENOENT;
}

static int fs_mount_notify(struct notifier_block *self, unsigned long action,
			   void *dev)
{
	char system_data_path[64] = { 0 };
	int upgrade = -1;

	switch (action) {
	case USB_MSC_NOTIFY_MOUNT: {
		printf("%s:%d ota mode = %d\n", __func__, __LINE__, mode);
		switch (mode) {
		case HCFOTA_REBOOT_OTA_DETECT_USB_HOST: {
			if (strncmp((void *)dev, "sd", 2) == 0) {
				complete(&storage_ready);
				upgrade = usb_mmc_upgrade_main(dev);
			}
			break;
		}
		case HCFOTA_REBOOT_OTA_DETECT_SD: {
			if (strncmp((void *)dev, "mmc", 3) == 0) {
				complete(&storage_ready);
				upgrade = usb_mmc_upgrade_main(dev);
			}
			break;
		}
		default:
			break;
		}

		if (upgrade == 0) {
			printf("%s:%d: upgrade success, just reboot to launch new firmware\n",
			       __func__, __LINE__);
			/* upgrade success, just reboot to launch new firmware */
			set_ota_detect_mode(HCFOTA_REBOOT_OTA_DETECT_NONE);

			printf("%s:%d: begin to reset system\n", __func__, __LINE__);

			complete(&upgrade_done);
			reset();
		} else {
			/* No firmware to upgrade */
			printf("%s:%d:%s No firmware to upgrade\n", __func__, __LINE__, (char *)dev);
		}
	}
	}

	return NOTIFY_OK;
}

static struct notifier_block fs_mount = {
       .notifier_call = fs_mount_notify,
};

#endif

int do_hcfota_upgrade(unsigned int ota_mode)
{
	int i = 0;
	char *excludes_mmc[] = {
		/* usb support */
		"usb",
		"musb_driver",
		"hc16xx_driver",
		"hcdisk_driver",
		"usb_storage_driver",
		/* usb gadget support */
		"mass_storage",
		/* usb host support */
		"usb_core",
		};

	char *excludes_usb[] = {
		/* mmc/sd support */
		"hc15_mmc_device",
		"hcmmc_device",
		};

	if (ota_mode == HCFOTA_REBOOT_OTA_DETECT_NONE)
		return 0;

	/* do upgrade */
 	for (i = 0; i < 8; i++) {
		mode = hcfota_reboot_get_ota_detect_mode_priority(ota_mode, i);

		switch (mode) {
#if defined(CONFIG_BOOT_UPGRADE_SUPPORT_USBDEVICE)
		case HCFOTA_REBOOT_OTA_DETECT_USB_DEVICE: {
			if (module_init2("all", 2, excludes_usb) != 0) {
				break;
			}
			create_usbd_upgarde_task();
			break;
		}
#endif
#if defined(CONFIG_BOOT_UPGRADE_SUPPORT_USBHOST)
		case HCFOTA_REBOOT_OTA_DETECT_USB_HOST: {
			init_completion(&upgrade_done);
			init_completion(&storage_ready);
			sys_register_notify(&fs_mount);
			if (module_init2("all", 2, excludes_usb) != 0) {
				break;
			}
			if (wait_for_completion_timeout(&storage_ready,
							HCFOTA_TIMEOUT) == 0) {
				printf("usbhost upgrade timeout!\n");
				break;
			}
			wait_for_completion(&upgrade_done);
			break;
		}
#endif
#if defined(CONFIG_BOOT_STARTUP_FROM_EMMC_SD)
		case HCFOTA_REBOOT_OTA_DETECT_SD: {
			printf("flash + sd/mmc case no support sd ota upgrade\n");
			break;
		}
#else
#if defined (CONFIG_BOOT_UPGRADE_SUPPORT_SD)
		case HCFOTA_REBOOT_OTA_DETECT_SD: {
			init_completion(&upgrade_done);
			init_completion(&storage_ready);
			sys_register_notify(&fs_mount);
			if (module_init2("all", 7, excludes_mmc) != 0) {
				break;
			}
			if (wait_for_completion_timeout(&storage_ready, HCFOTA_TIMEOUT) == 0) {
				printf("sd/emmc upgrade timeout\n");
				break;
			}
			wait_for_completion(&upgrade_done);
			break;
		}
#endif
#endif
#ifdef CONFIG_BOOT_UPGRADE_SUPPORT_NETWORK
		case HCFOTA_REBOOT_OTA_DETECT_NETWORK: {
			printf("Not support upgrade from network!\n");
			break;
		}
#endif
		default:
			break;
		}
	}

	return 0;
}

int upgrade_detect(void)
{
	int fd;
	struct persistentmem_node_create new_node;
	struct persistentmem_node node;
	struct sysdata sysdata = { 0 };

	if (upgrade_detect_key()) {
		return upgrade_force();
	}

	return do_hcfota_upgrade(get_ota_detect_mode());
}

static int do_upgrade(int argc, char **argv)
{

	if (argc == 1)
		return upgrade_detect();

	if (argc == 2) {
		module_init("usb_core");
		module_init("hcmmc_device");
		module_init("hc15_mmc_device");
		return hcfota_url(argv[1], hcfota_report, 0);
	}

	return 0;
}

CONSOLE_CMD(upgrade, NULL, do_upgrade, CONSOLE_CMD_MODE_SELF, "upgrade firmware")
