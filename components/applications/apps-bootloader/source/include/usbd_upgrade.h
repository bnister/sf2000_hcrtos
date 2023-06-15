#ifndef __UBOOT_UPDATE_H
#define __UBOOT_UPDATE_H

#include <kernel/lib/fdt_api.h>
int wait_any_key_pressed(char *tip);
void usbd_upgrade(void);
void create_usbd_upgarde_task(void);

#endif // !__UBOOT_UPDATE_H