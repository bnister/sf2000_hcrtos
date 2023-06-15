#ifndef _HCFOTA_H_
#define _HCFOTA_H_

#include <hcuapi/sysdata.h>

typedef enum HCFOTA_REPORT_EVENT {
	HCFOTA_REPORT_EVENT_DOWNLOAD_PROGRESS,
	HCFOTA_REPORT_EVENT_UPGRADE_PROGRESS,
} hcfota_report_event_e;

typedef enum HCFOTA_EXTERNAL_FLASH_TYPE {
	HCFOTA_EXTERNAL_FLASH_NULL,
	HCFOTA_EXTERNAL_FLASH_EMMC,
	HCFOTA_EXTERNAL_FLASH_NAND,
	HCFOTA_EXTERNAL_FLASH_SDCARD,
    HCFOTA_EXTERNAL_FLASH_UDISK,
} hcfota_external_flash_type_e;

struct hcfota_header {
	uint32_t crc;
	uint8_t reserved1;
	uint8_t compress_type;
	uint8_t ignore_version_check;
	uint8_t ignore_version_update;
	uint32_t version;
	uint32_t uncompressed_length;
	uint8_t board[16];
	uint32_t payload_size;
	uint8_t reserved3[12];
	uint8_t reserved4[16];
};

union hcfota_entry {
	struct {
		uint8_t index;
		uint8_t dev_index;
		uint8_t dev_type;
		uint8_t upgrade_enable;
		uint32_t offset_in_payload;
		uint32_t length;
		uint32_t erase_length;
		uint8_t entry_type;
		uint8_t reserved1[3];
		uint32_t offset_in_dev;
		uint32_t offset_in_blkdev;
		uint8_t reserved2[4];
	} upgrade;

	struct {
		uint8_t index;
		uint8_t old_dev_index;
		uint8_t old_dev_type;
		uint8_t upgrade_enable;
		uint32_t old_offset_in_dev;
		uint32_t old_length;
		uint8_t reserved1[4];
		uint8_t entry_type;
		uint8_t new_dev_index;
		uint8_t new_dev_type;
		uint8_t reserved2[1];
		uint32_t new_offset_in_dev;
		uint32_t new_length;
		uint8_t reserved3[4];
	} backup;
};

struct hcfota_payload_header {
	uint32_t crc;
	uint32_t entry_number;
	uint8_t reserved1[8];
	uint8_t reserved2[16];
	union hcfota_entry entry[31];
};

#define HCFOTA_ERR_HEADER_CRC	(-10)
#define HCFOTA_ERR_PAYLOAD_CRC	(-11)
#define HCFOTA_ERR_DOWNLOAD	(-12)
#define HCFOTA_ERR_LOADFOTA	(-13)
#define HCFOTA_ERR_DECOMPRESSS	(-14)
#define HCFOTA_ERR_UPGRADE	(-15)
#define HCFOTA_ERR_VERSION	(-16)

/*
 * hcboot will detect the OTA upgrade mode with priority
 * priority 0 : highest priority
 * priority 7 : lowest priority
 * Each priority needs 4 bits to store the corresponding mode. There are maximum 8 modes.
 * One priority has only one mode. It doesn't support multiple modes with the same priority.
 */
static inline unsigned long hcfota_reboot_ota_detect_mode_priority(hcfota_reboot_ota_detect_mode_e mode, int priority)
{
	if (priority >= 8 || priority < 0)
		return mode & 0xf;
	return (mode & 0xf) << (priority * 4);
}

static inline unsigned long hcfota_reboot_get_ota_detect_mode_priority(unsigned long modes, int priority)
{
	if (priority >= 8 || priority < 0)
		return modes & 0xf;

	return (modes >> (priority * 4)) & 0xf;
}

typedef int (*hcfota_report_t)(hcfota_report_event_e event, unsigned long param, unsigned long usrdata);

/*
 * Download firmware from the url and store in path
 * The application may
 *   1. call this API to download upgrade file from *url* and store into local file *path*,
 * such as local file path of removable USB/MMC/SD storage.
 *   2. update flag that stored in /dev/persistentmem to inform the hcboot to finish the upgrade into flash
 */
int hcfota_download(const char *url, const char *path, hcfota_report_t report_cb, unsigned long usrdata);

/*
 * The modes may contain different mode with different priority. The hcboot will detect OTA from the
 * highest priority mode to lowest priority mode.
 */
int hcfota_reboot(unsigned long modes);

/*
 * Upgrade firmware from the url, e.g http://, https://, ftp://
 *    For hclinux platform:
 *       This API will first download upgrade file from url and store in /tmp/, then upgrade it into flash
 *       Please close the unused function to save memory to store the upgrade file. This API won't check
 *       the current usable memory.
 *    For hcrtos  platform:
 *       This API will first download upgrade file from url and store in local malloc memory, then upgrade it into flash
 *       Please close the unused function to save memory to store the upgrade file. User can change the size
 *       for the memory allocation if the hcfota.bin is larger than the default malloc-size
 *
 * Upgrade from local file:
 *    If the *url* is a local file path, such as /path/to/hcfota.bin, this API
 *    will just read from local and upgrade into flash.
 *
 * This version just report the upgrade progress. The download progress is not implemented yet.
 *
 * Once the upgrade into flash is in progress, the flag stored in /dev/persistentmem is changed till the whole
 * upgrade is finished. So that the hcboot is able to know whether the previous upgrade is fully finished or not.
 */
int hcfota_url(const char *url, hcfota_report_t report_cb, unsigned long usrdata);

/*
 * Upgrade firmware from the memory with already downloaded hcfota.bin
 *
 * This version just report the upgrade progress. The download progress is not implemented yet.
 *
 * Once the upgrade into flash is in progress, the flag stored in /dev/persistentmem is changed till the whole
 * upgrade is finished. So that the hcboot is able to know whether the previous upgrade is fully finished or not.
 */
int hcfota_memory(const char *buf, unsigned long size, hcfota_report_t report_cb, unsigned long usrdata);

int hcfota_memory_b2b(const char *buf, unsigned long size, hcfota_report_t report_cb, unsigned long usrdata);
/*
 * List the information of hcfota.bin
 * *url* is the absolute path or relative path if it is a localfile
 * *url* is the network address, http://, https://, ftp://, if it is a network address
 */
int hcfota_info_url(const char *url);
int hcfota_info_memory(const char *buf, unsigned long size);

int hcfota_url_extern_flash_data(const char *url, hcfota_external_flash_type_e flash_type,
    hcfota_report_t report_cb, unsigned long usrdata);
int hcfota_url_extern_flash(const char *url, hcfota_external_flash_type_e flash_type,
    hcfota_report_t report_cb, unsigned long usrdata);

int hcfota_from_path(const char *path, hcfota_report_t report_cb, unsigned long usrdata);

int hcfota_from_mem(union hcfota_entry *entry, void *buf,uint32_t buf_read_size,uint32_t *finsh_len);

enum {
	BURNER_GET_DATA = 0,
	BURNER_SEND_STATUS = 1,
	BURNER_FAIL  = 2,
};

typedef int (*get_data_from_burner_t)(int flag, unsigned long param, uint32_t offsize, void **buf, uint32_t *size);

#endif
