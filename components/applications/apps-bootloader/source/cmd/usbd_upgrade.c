#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <poll.h>
#include <kernel/lib/console.h>
#include <kernel/module.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <freertos/queue.h>
#include <kernel/lib/console.h>

#include <sys/mount.h>
#include <hcuapi/ramdisk.h>
#include <kernel/drivers/hcusb.h>
#include <nuttx/mtd/mtd.h>
#include <nuttx/drivers/ramdisk.h>
#include <nuttx/fs/fs.h>
#include <fsutils/mkfatfs.h>
#include <uapi/hcuapi/mmz.h>
#include <dirent.h>
#include <errno.h>
#include <showlogo.h>
#include <backlight.h>
#include <mtdload.h>
#include <bootm.h>
#include <kernel/lib/libfdt/libfdt.h>
#include <kernel/io.h>
#include <kernel/delay.h>

#include <kernel/elog.h>
#define ferr  log_e
#define fwarn log_w
#define finfo log_i

#include "usbd_upgrade.h"
#include "hcfota.h"
#include "show_upgrade_way.h"

#include <hcuapi/fb.h>
#include <kernel/fb.h>
#include <sys/mman.h>
#include <hcuapi/pinpad.h>
#include <hcuapi/gpio.h>
#include <dt-bindings/gpio/gpio.h>

static char * ram_path = "/dev/ram0";
static char * mnt_path = "/mnt/ram0";
static long long old_time[64] = {0};
static int total_dnum = 0;
static int last_dnum = -1;
static int reset_num = 0;
static int times = 0;

extern int sysdata_update_dt(void *dtb);

FAR uint8_t *buffer;

struct usbd_hcfota_progress {
	long long current;
	long long total;
	hcfota_report_t report_cb;
	unsigned long usrdata;
}usbd_hcfota_progress;

static void usbd_refresh(uint time){
    umount(mnt_path);
    msleep(time);
    mount(ram_path, mnt_path, "vfat", 0, NULL);
}


static int hcfota_update(FAR struct dirent *entryp)
{
    int ret = 0;
    int i = 0;
    char mnt_path_backup[32]; ;
    clock_t start,end;

    strcpy(mnt_path_backup,mnt_path);
    char *mnt_entryp_path = mnt_path_backup;

    strcat(mnt_entryp_path,"/");
    strcat(mnt_entryp_path,entryp->d_name);

    printf("\n\t=====%s start update!=====\n",entryp->d_name);

    start = clock();
    ret = hcfota_url(mnt_entryp_path, hcfota_report,0);
    if(ret < 0)
    {
        printf("%s update failed,try again!\n",entryp->d_name);
        return ret;
    }

    end = clock();

    printf("\n\tupdate %s use time=%f\n",entryp->d_name,(double)(end-start)/CLK_TCK);
    printf("\n\t=====%s finsh update!=====\n",entryp->d_name);
    printf("\n\trestart is coming...................!\n\n");

    /* reset code */
    reset();

    return 0;
}

static int partition_load_file(const char *path, void **loadbuf, unsigned long file_size)
{
    struct stat sb;
    FILE *file_p;
    void *partition_buf;

	*loadbuf = NULL;

	if (stat(path, &sb) == -1) {
		perror("stat");
		return -2;
	}

    file_p = fopen(path, "rb");
    if(!file_p) {
        printf("open %s failed\n", path);
		return -2;
    }

    partition_buf = malloc(file_size);
    if (!partition_buf) {
		printf("Not enough memory!\n");
		return -12;
	}

	fseek(file_p, 0, SEEK_SET);
	if (fread(partition_buf, 1, file_size, file_p) != file_size) {
		printf("read %s failed\n", path);
		free(partition_buf);
		fclose(file_p);
		return -5;
	}

	*loadbuf = partition_buf;

	fclose(file_p);

	return 0;
}

static int partition_do_upgrade(void *buf,long size,u32 offset,struct usbd_hcfota_progress *progress)
{
    struct mtd_geometry_s geo = { 0 };
    struct mtd_eraseinfo_s erase;
    int fd, err, i;
    void *tmp;
    ssize_t res;
    size_t  written, eraseoff;
    long segment;

    fd = open("/dev/mtdblock0", O_SYNC | O_RDWR);
    if (fd < 0) {
        return -errno;
    }
    err = ioctl(fd, MTDIOC_GEOMETRY, &geo);
    if (err < 0) {
        close(fd);
        return -errno;
    }

    segment = 0x10000;

    tmp = malloc(segment);
    if (!tmp) {
        close(fd);
        return -ENOMEM;
    }

    written = 0;
    eraseoff = 0;
    i = segment;
    while (size) {
        if (size < segment)
            i = size;

        erase.start = offset + written;
        erase.length = (i + geo.erasesize - 1) / geo.erasesize;
        erase.length *= geo.erasesize;

        if (lseek(fd, (off_t)erase.start, SEEK_SET) < 0) {
            close(fd);
            free(tmp);
            return -errno;
        }

        if (i != (int)segment || (read(fd, tmp, i) != i) ||
            memcmp(buf + written, tmp, i)) {
            if (i != (int)segment) {
                ioctl(fd, MTDIOC_MEMERASE, &erase);
            }

            lseek(fd, (off_t)erase.start, SEEK_SET);
            res = write(fd, buf + written, i);
            if (i != res) {
                free(tmp);
                close(fd);
                return -errno;
            }
        }

        written += i;
        size -= i;
        eraseoff += erase.length;

        progress->current += i;
        if (progress->report_cb) {
            progress->report_cb(
                HCFOTA_REPORT_EVENT_UPGRADE_PROGRESS,
                progress->current * 100 / progress->total,
                progress->usrdata);
        }
    }

    if (0x10000 > eraseoff) {
        erase.start = offset + eraseoff;
        erase.length = 0x10000 - eraseoff;;
        if (ioctl(fd, MTDIOC_MEMERASE, &erase) < 0) {
            close(fd);
            free(tmp);
            return -errno;
        }
    }
    close(fd);
    free(tmp);
    return 0;
}

static int partition_upgrade(const char *filename,u32 offset, u32 size)
{

    int rc;
    char mnt_path_backup[32]; ;
	void *buf;
	struct usbd_hcfota_progress progress = { 0 };
    clock_t start,end;

    strcpy(mnt_path_backup,mnt_path);
    char *mnt_entryp_path = mnt_path_backup;

    strcat(mnt_entryp_path,"/");
    strcat(mnt_entryp_path,filename);
    
	progress.report_cb = hcfota_report;
	progress.usrdata = 0;
    progress.total = size;

    printf("\n\t=====%s start update!=====\n",filename);

    start = clock();

    rc = partition_load_file(mnt_entryp_path, &buf, size);
	if (rc) {
        printf("load file failed: %s\n", mnt_entryp_path);
		return rc;
	}
    
    rc = partition_do_upgrade(buf,size,offset,&progress);
    if (rc < 0) {
        printf("partition_do_upgrade failed, try again\n");
        return rc;
    }

    end = clock();
    printf("\n\tupdate %s use time=%f\n",filename,(double)(end-start)/CLK_TCK);

    printf("\t=====%s finsh update!=====\n\n",filename);

    free(buf);

    return 0;
}

static int file_judgment(FAR struct dirent *entryp,long long n_time,long file_size)
{
    char *HCFOTA_BUF = "HCFOTA";
    char *hcfota_bin_buf = "hcfota.bin";
    char bin_buf[3] = "bin";
    char BIN_BUF[3] = "BIN";
    char *dname_buf = entryp->d_name;
    char tmp_buf[4] = { 0 };
    int i = 0;
    int ret = 0;

    int dir_name_len = strlen(entryp->d_name);
    
    int np;
	u32  npart;
    u32 * start;
    char charname[32];
    const char *filename;

    np = fdt_node_probe_by_path("/hcrtos/sfspi/spi_nor_flash/partitions");

	if (np < 0)
		return -1;

    npart = 0;
	fdt_get_property_u_32_index(np, "part-num", 0, &npart);

    start = (u32 *)malloc(sizeof(u32));

    for(i = 1; i <= (int)npart; i++)
    {
        memset(charname, 0, sizeof(charname));

		snprintf(charname, 32, "part%d-reg", i);
		fdt_get_property_u_32_index(np, charname, 0, start);

        snprintf(charname, 32, "part%d-filename", i);
		fdt_get_property_string_index(np, charname, 0, &filename);

        if((strcmp(entryp->d_name,filename))==0)
        {
            if(old_time[i] != n_time)
            {
                old_time[i] = n_time;

                msleep(1000);

                total_dnum++;
                last_dnum = total_dnum;
                reset_num = 0;
                times = 0;
                ret = partition_upgrade(filename,*start,file_size);
                if(ret != 0 )
                    return -1;
            }
        }
    }

    if((strncmp(HCFOTA_BUF,dname_buf,6))==0)
    {        
        for (i = 0;i < 3;i++)
        {
            tmp_buf[i] = entryp->d_name[dir_name_len-3+i];
        }
        if(!((strncmp(tmp_buf,bin_buf,3)!=0)||(strncmp(tmp_buf,BIN_BUF,3)!= 0)))
            return 0;
        
        if((old_time[npart+1] != n_time))
        {
            old_time[npart+1] = n_time;

            usbd_refresh(1000);

            if(hcfota_update(entryp))
                return -1;

            return 0;
        }
    }

    if((strncmp(hcfota_bin_buf,dname_buf,10))==0)
    {    
        if((old_time[npart+2] != n_time))
        {
            old_time[npart+2] = n_time;

            usbd_refresh(1000);

            if(hcfota_update(entryp))
                return -1;

            return 0;
        }
    }

    free(start);
    
    return 0;
}

static int update_search_all_entryp_num(void)
{
    int ret = 0;
    DIR *dirp;
    struct stat st;

    dirp = opendir(mnt_path);
    if(dirp == NULL)
        printf("open dir %s failed\n", mnt_path);

    FAR struct dirent *entryp = readdir(dirp);

    for (; ; )
    {
        
        entryp = readdir(dirp);
        if (entryp == NULL)
        {
            break;
        }
        
        total_dnum++;

    }

    closedir(dirp);
   
    return ret;

}

static int update_main(void)
{
    int ret = 0;
    DIR *dirp;
    struct stat st;
    char path[512];

    dirp = opendir(mnt_path);
    if(dirp == NULL)
        printf("open dir %s failed\n", mnt_path);
      /* Read each directory entry */

    FAR struct dirent *entryp = readdir(dirp);

    for (; ; )
    {
        entryp = readdir(dirp);
        if (entryp == NULL)
        {
            /* Finished with this directory */
            break;
        }

        memset(path, 0, sizeof(path));
        strcat(path, mnt_path);
        strcat(path, "/");
        strcat(path, entryp->d_name);

        ret = stat(path, &st);
        if(ret < 0)
        {
            printf("stat error\n");
            return ret;
        }

        ret = file_judgment(entryp,st.st_mtime,st.st_size);
        if(ret < 0)
            return ret;

        if(last_dnum == total_dnum)
        {
            reset_num++;
            if (reset_num % 4==0)
            {
                reset_num == 0;
                times++;

                printf("\tAbout to restart automatically ==> %d \n",4-times);
                if(times == 3)
                {                
                    printf("\n\trestart is coming..................!\n\n");
                    reset();
                }
            }
            
        }

    }

    closedir(dirp);
   
    return ret;
}

static int usbd_mkrd(int minor, uint32_t nsectors, uint16_t sectsize, uint8_t rdflags)
{
  int ret = 0;

  /* Allocate the memory backing up the ramdisk from the kernel heap */

  buffer = (FAR uint8_t *)mmz_malloc(0,sectsize * nsectors);
  if (buffer == NULL)
    {
      ferr("ERROR: mmz_malloc() failed: %d\n", ret);
      return -ENOMEM;
    }

#ifdef CONFIG_DEBUG_INFO
  memset(buffer, 0, sectsize * nsectors);
#endif
  finfo("RAMDISK at %p\n", buffer);

  /* Then register the ramdisk */

  ret = ramdisk_register(minor, buffer, nsectors, sectsize, rdflags);
  if (ret < 0)
    {
      ferr("ERROR: ramdisk_register() failed: %d\n", ret);
      mmz_free(0,buffer);
    }

  return ret;
}

int update_init(void)
{
    struct fat_format_s fmt = FAT_FORMAT_INITIALIZER;
    int ret = 0;

    int usb_port =  CONFIG_BOOT_USBD_UPGRADE_USBD_PORT_NUM;
    char *path[] = {
        "/dev/ram0",
    };
    
    ret = usbd_mkrd(0,36864,512,RDFLAG_WRENABLED | RDFLAG_FUNLINK);

    if (ret < 0 )
        printf("error mkrd\n");

    ret = mkfatfs(ram_path, &fmt);
    if (ret < 0)
        printf("error mkfatfs\n");

    hcusb_set_mode(usb_port, MUSB_PERIPHERAL);
    hcusb_gadget_msg_specified_init(
                             get_udc_name(usb_port),
                             path, 1);

    return 0;
}

int wait_any_key_pressed(char *tip)
{
	char buf;
	struct pollfd pfd = { 0 };
	char *excludes[] = {
		/* mmc/sd support */
		"hc15_mmc_device",
		"hcmmc_device",
	};
	int wait_timeout = CONFIG_BOOT_USBD_UPGRADE_REQUEST_TIME;

	pfd.fd = STDIN_FILENO;
	pfd.events = POLLIN | POLLRDNORM;

	while (wait_timeout--)  
	{
		printf("%s ==> %d\n",tip, wait_timeout + 1);

		if (poll(&pfd, 1, 1000) > 0) {
			assert(module_init2("all", 2, excludes) == 0);
			read(STDIN_FILENO,&buf,sizeof(buf));
			return 1;
		}
	}

	return 0;
}

static int get_mtdblock_devpath(char *devpath, int len, const char *partname)
{
	static int np = -1;
	static u32 part_num = 0;
	u32 i = 1;
	const char *label;
	char property[32];

	if (np < 0) {
		np = fdt_get_node_offset_by_path("/hcrtos/sfspi/spi_nor_flash/partitions");
	}

	if (np < 0)
		return -1;

	if (part_num == 0)
		fdt_get_property_u_32_index(np, "part-num", 0, &part_num);

	for (i = 1; i <= part_num; i++) {
		snprintf(property, sizeof(property), "part%d-label", i);
		if (!fdt_get_property_string_index(np, property, 0, &label) &&
		    !strcmp(label, partname)) {
			memset(devpath, 0, len);
			snprintf(devpath, len, "/dev/mtdblock%d", i);
			return i;
		}
	}

	return -1;
}

static int file_exist(const char *filename_path)
{
    int ret = -1;
    DIR *dirp;
    char entryp_path[512];

    dirp = opendir(mnt_path);
    if(dirp == NULL)
        printf("open dir %s failed\n", mnt_path);
      /* Read each directory entry */

    FAR struct dirent *entryp = readdir(dirp);

    for (; ; )
    {
        entryp = readdir(dirp);
        if (entryp == NULL)
        {
            /* Finished with this directory */
            break;
        }

        memset(entryp_path, 0, sizeof(entryp_path));
        strcat(entryp_path, mnt_path);
        strcat(entryp_path, "/");
        strcat(entryp_path, entryp->d_name);
        if(!(strcmp(entryp_path,filename_path)))
        {
            closedir(dirp);
            return 0;
        }
    }

    closedir(dirp);

    return ret;
}

static int usbd_upgrade_stop(int argc, char *argv[]);

static int execute_from_ram_main(void)
{
    int ret = 0;
    char devpath[64];
    struct stat st;
    char tmp_buf[256];

#if defined(CONFIG_BOOT_BACKLIGHT)
	open_lcd_backlight(1,((char *[]){"backlight"}));
#endif

#if defined(CONFIG_BOOT_HCRTOS)

    int np;
    int i, filename_len;
    u32 npart;
    char charname[32];
    const char *filename;
    char path[512];

    np = fdt_node_probe_by_path("/hcrtos/sfspi/spi_nor_flash/partitions");
    if (np < 0)
		return -1;

    memset(charname, 0, sizeof(charname));
    npart = 0;
	fdt_get_property_u_32_index(np, "part-num", 0, &npart);
	for(i = 1; i <= (int)npart;i++)
	{
        snprintf(charname, 32, "part%d-filename", i);
		fdt_get_property_string_index(np, charname, 0, &filename);
        filename_len = strlen(filename);
        if (filename_len < 6)
            continue;
	if(!strcmp(&filename[filename_len -6],"uImage"))
	{
		break;
	}

	}

    memset(path, 0, sizeof(path));
    strcat(path, "/mnt/ram0/");
    strcat(path, filename);

    if(!file_exist(path))
    {
        umount(mnt_path);
        msleep(2000);
        mount(ram_path, mnt_path, "vfat", 0, NULL);

        mtdloaduImage(2, ((char *[]){ "mtdloaduImage", path }));
        bootm(NULL, 0, 1, ((char *[]){ "bootm" }));
    }

#elif defined(CONFIG_BOOT_HCLINUX_DUALCORE)
	void *dtb = malloc(0x10000);
	char loadaddr[16] = { 0 };
	char dtbaddr[16] = { 0 };

	sprintf(dtbaddr, "0x%08x", (unsigned int)dtb);
	REG8_WRITE(0xb880006b, 0x1);
	REG32_WRITE(0xb8800004, (uint32_t)dtb);

	if (!file_exist("/mnt/ram0/dtb.bin")) {

        usbd_refresh(500);
        ret = stat("/mnt/ram0/dtb.bin", &st);
        sprintf(tmp_buf,"%d", (int)st.st_size);
		mtdloadraw(4, ((char *[]){ "mtdloadraw", dtbaddr, "/mnt/ram0/dtb.bin", tmp_buf }));
		sysdata_update_dt(dtb);
		cache_flush(dtb, fdt_totalsize(dtb));

        if ((!file_exist("/mnt/ram0/avp.uImage"))&&(!file_exist("/mnt/ram0/vmlinux.uImage"))) 
        {
            usbd_refresh(1000);
            mtdloaduImage(2, ((char *[]){ "mtdloaduImage", "/mnt/ram0/avp.uImage" }));
            bootm(NULL, 0, 1, ((char *[]){ "bootm" }));

            if (REG8_READ(0xb880006b) != 0x2) {
                /* scpu boot fail */
                reset();
            }

            mtdloaduImage(2, ((char *[]){ "mtdloaduImage", "/mnt/ram0/vmlinux.uImage" }));
            sprintf(loadaddr, "0x%08lx", image_load_addr);
            bootm(NULL, 0, 4, ((char *[]){ "bootm", loadaddr, "-", dtbaddr }));

	    }

	}
#elif defined(CONFIG_BOOT_HCLINUX_SINGLECORE)
	void *dtb = malloc(0x10000);
	char loadaddr[16] = { 0 };
	char dtbaddr[16] = { 0 };

	sprintf(dtbaddr, "0x%08x", (unsigned int)dtb);
	REG32_WRITE(0xb8800004, (uint32_t)dtb);
    if (!file_exist("/mnt/ram0/dtb.bin"))
    {
        usbd_refresh(500);
        ret = stat("/mnt/ram0/dtb.bin", &st);
        sprintf(tmp_buf,"%d", (int)st.st_size);
		mtdloadraw(4, ((char *[]){ "mtdloadraw", dtbaddr, "/mnt/ram0/dtb.bin", tmp_buf }));
		sysdata_update_dt(dtb);
		cache_flush(dtb, fdt_totalsize(dtb));

        if(!file_exist("/mnt/ram0/vmlinux.uImage"))
        {
            usbd_refresh(1000);
            mtdloaduImage(2, ((char *[]){ "mtdloaduImage", "/mnt/ram0/vmlinux.uImage" }));
            sprintf(loadaddr, "0x%08lx", image_load_addr);
            bootm(NULL, 0, 4, ((char *[]){ "bootm", loadaddr, "-", dtbaddr }));
        }
    }

#endif

}

static TaskHandle_t  usbd_upgarde_main_t = {0};

static void usbd_upgrade_main(void *pvParameters)
{

    int ret = 0;
    printf("\n\t### USBD UPGRADE MODE ###\n\n");
    update_init();
#ifdef CONFIG_BOOT_UPGRADE_SHOW_WITH_SCREEN
    init_progress_bar();
	fill_progress_bar(1280,0xffffffff);
#endif

#ifdef CONFIG_BOOT_UPGRADE_SHOW_WITH_LED
    led_init();
#endif
    while (1) {

        mount(ram_path, mnt_path, "vfat", 0, NULL);
        // update_search_all_entryp_num();

        ret = update_main();
        if(ret < 0)
        {
            printf("update error\n");
        }
        msleep(500);

        umount(mnt_path);
        msleep(500);

    }

}

static void execute_from_ram(void *pvParameters)
{
    int ret ;
    char devpath[64];

    printf("\n\t### EXECUTE FROM RAM MODE ###\n\n");
    update_init();

    while(1) {
        mount(ram_path, mnt_path, "vfat", 0, NULL);

        msleep(500);
        execute_from_ram_main();

        umount(mnt_path);
        msleep(500);
    }
}

void create_usbd_upgarde_task(void)
{
    xTaskCreate(usbd_upgrade_main, (const char *)"usbd_upgrade_main", configTASK_STACK_DEPTH,
                    NULL, portPRI_TASK_NORMAL, (TaskHandle_t *)&usbd_upgarde_main_t);
}

void create_execute_from_ram_task(void)
{
    xTaskCreate(execute_from_ram, (const char *)"execute_from_ram", configTASK_STACK_DEPTH,
                    NULL, portPRI_TASK_NORMAL, (TaskHandle_t *)&usbd_upgarde_main_t);
}

void usbd_upgrade(void)
{
    if(wait_any_key_pressed("PRESS ANY KEY ON KEYBOARD TO ENTER EXECUTE FROM RAM MODE,ELSE WILL ENTER USBD UPGRADE MODE"))
    {
        create_execute_from_ram_task();
    }
    else
    {
        create_usbd_upgarde_task();
    }
}

static int usbd_upgrade_stop(int argc, char *argv[])
{
    int ret = 0;

    if(buffer != NULL)
    {
        vTaskDelete(usbd_upgarde_main_t);

        hcusb_gadget_msg_deinit();

        ret = unregister_blockdriver(ram_path);
        if (ret < 0)
        {
            printf("unregister_blockdriver failed\n");
            return -1;
        }
        mmz_free(0,buffer);

        printf("usbd upgrade stopped\n");

        buffer = NULL;
    }

    return 0;
}

CONSOLE_CMD(usbd_upgrade_stop, NULL, usbd_upgrade_stop, CONSOLE_CMD_MODE_SELF, "stop usbd upgrade task")
