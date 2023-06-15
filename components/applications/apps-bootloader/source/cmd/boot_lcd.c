#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <kernel/lib/console.h>

typedef enum LCD_CURRENT_INIT_STATUS
{
	LCD_CURRENT_NOT_INIT,
	LCD_CURRENT_IS_INIT,
}lcd_current_init_status_e;

void open_boot_lcd_init(int argc, char *argv[])
{
	int fd;
	int tmp = 0;
	fd = open("/dev/lcddev",O_RDWR);
	if(fd > 0)
	{
		/*get lcd init status*/
		if(read(fd, &tmp, 4))
		{
			if(tmp == LCD_CURRENT_NOT_INIT)
			{
				/*set lcd init status init*/
				tmp = LCD_CURRENT_IS_INIT;
				write(fd, &tmp, 4);
			}
			close(fd);
		}
	}
}

CONSOLE_CMD(boot_lcd, NULL, open_boot_lcd_init, CONSOLE_CMD_MODE_SELF, "boot open lcd init")
