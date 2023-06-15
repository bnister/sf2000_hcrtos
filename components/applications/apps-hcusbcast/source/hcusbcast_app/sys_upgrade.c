/*
sys_upgrade.c
 */


#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include <dirent.h>
#include <sys/stat.h>
#include <hcfota.h>
#include "com_api.h"
#include "hotplug_mgr.h"
#include "data_mgr.h"

#define ROOT_DIR    "/media"
#define USB_UPG_FILE_WILDCARD	"HCFOTA_"//"hc_upgrade"
#define MAX_UPG_LENGTH      16*1024*1024
#define READ_LENGTH      200*1024

volatile int m_usb_upgrade = 0;
static bool sys_usb_dev_wait_plugout(uint32_t timeout)
{
    int check_count = timeout/500;

#if 1
    do{
        if (USB_STAT_MOUNT != hotplug_usb_get())
            return true;
        api_sleep_ms(500);
    }while(check_count --);

    return false;
#else    
    DIR *dirp = NULL;
    struct dirent *entry = NULL;
    int sub_dev_found = 0;

    do{
        sub_dev_found = 0;
        if ((dirp = opendir(ROOT_DIR)) == NULL) {
            printf("%s(), line: %d. open dir:%s error!\n", __func__, __LINE__, ROOT_DIR);
            return true;
        }
        
        for(;;){
            entry = readdir(dirp);
            if (!entry)
                break;

            if(!strcmp(entry->d_name, ".") ||
                !strcmp(entry->d_name, "..")){
                //skip the upper dir
                continue;
            }

            if (strlen(entry->d_name) && entry->d_type == 4){ //dir
                printf("%s(), line: %d. found USB device: %s!\n", __func__, __LINE__,entry->d_name);        
                sub_dev_found = 1;
                break;
            }

        }
        if (dirp)
            closedir(dirp);

        if (!sub_dev_found)
            return true;

        api_sleep_ms(500);
    }while(check_count --);
    return false;
#endif

}

/*
Get the USB updreade file
*/
static int sys_upg_usb_filepath_get(uint32_t timeout, char *file_path)
{
    int check_count;
    DIR *dirp = NULL;
    struct dirent *entry = NULL;
    check_count = timeout/100;
    char usb_path[512];
    int ret = API_FAILURE;
    int usb_dev_found = 0;

    unsigned int local_version;
    sys_data_t* sys_data = data_mgr_sys_get();
    char upg_file_path[256] = {0};
    char *filename = upg_file_path;

    local_version = (unsigned int)sys_data->firmware_version;
    
    //step 1: get USB dir
    while(check_count --){

        if ((dirp = opendir(ROOT_DIR)) == NULL) {
            api_sleep_ms(100);
            continue;
        }

        while (1) {
            entry = readdir(dirp);
            if (!entry)
                break;

            if(!strcmp(entry->d_name, ".") ||
                !strcmp(entry->d_name, "..")){
                //skip the upper dir
                continue;
            }

            if (strlen(entry->d_name) && entry->d_type == 4){ //dir
                sprintf(usb_path, "%s/%s", ROOT_DIR, entry->d_name);
                usb_dev_found = 1;
                printf("%s(), line: %d. found USB device: %s!\n", __func__, __LINE__,usb_path);        
                break;
            }

        }
        if (usb_dev_found)
            break;
        api_sleep_ms(100);
    }
    if (check_count <= 0){
        printf("%s(), line: %d. No USB device!\n", __func__, __LINE__);        
        ret = API_FAILURE;
        goto usb_check_exit;
    }
    if (dirp)
        closedir(dirp);

    //step 2: found USB updated file
    if ((dirp = opendir(usb_path)) == NULL) {
        printf("%s(), line: %d. open dir:%s error!\n", __func__, __LINE__, usb_path);
        ret = API_FAILURE;
        goto usb_check_exit;
    }
    while (1) {
        entry = readdir(dirp);
        if (!entry)
            break;

        if(!strcmp(entry->d_name, ".") ||
            !strcmp(entry->d_name, "..")){
            //skip the upper dir
            continue;
        }
        
        if (0 == strncasecmp(entry->d_name, USB_UPG_FILE_WILDCARD, strlen(USB_UPG_FILE_WILDCARD)))
        {
            struct hcfota_header fw_header = {0};
            FILE* fp = NULL;
            char file_name[512] = {0};
            int header_len = sizeof(struct hcfota_header);
            int file_len;
            int len;

            sprintf(file_name, "%s/%s", usb_path, entry->d_name);
            printf("%s(), line: %d. file_name:%s!\n", __func__, __LINE__,file_name);
            fp = fopen(file_name, "rb+");
            if(fp == NULL)
            {
                printf("%s fopen:%s error.\n", __func__,file_name);
                goto usb_check_exit;
            }

            fseek(fp,0,SEEK_END);
            file_len = ftell(fp);
            fseek(fp,0,SEEK_SET);

            if(file_len > header_len)
            {
                int read_len = header_len;
                int red_cnt = 0;
                while(read_len)
                {
                    len = fread(&fw_header+red_cnt, 1, read_len, fp);
                    if (len <= 0)
                    {
                        break;
                    }

                    red_cnt += len;
                    read_len -= len;
                }

                if(red_cnt != header_len)
                {
                    printf("%s fw header len not match.\n", __func__);
                    fclose(fp);
                    goto usb_check_exit; 
                }

            }
            else
            {
                printf("%s fw header len error.\n", __func__);
                fclose(fp);
                goto usb_check_exit; 
            }
    
            fclose(fp);
            
            printf("local product:%s, fw product:%s\n",sys_data->product_id, fw_header.board);
            printf("local version:%u, fw version:%u\n",local_version, fw_header.version);

            if(memcmp(sys_data->product_id, fw_header.board, sizeof(fw_header.board)) == 0)
            {
                if(fw_header.version != local_version)
                {
                    sprintf(file_path, "%s/%s", usb_path, entry->d_name);
                    printf("%s(), line: %d. found upg file:%s!\n", __func__, __LINE__,file_path);
                    ret = API_SUCCESS;
                    
                    goto usb_check_exit;
                }
            } 
        }
    }

usb_check_exit:
    if (dirp)
        closedir(dirp);
            
    return ret;
}


static int mtd_upgrade_callback(hcfota_report_event_e event, unsigned long param, unsigned long usrdata)
{
    control_msg_t msg = {0};

    if (HCFOTA_REPORT_EVENT_UPGRADE_PROGRESS == event){
        msg.msg_type = MSG_TYPE_UPG_BURN_PROGRESS;
        msg.msg_code = (uint32_t)param;
        api_control_send_msg(&msg);
        printf("burning %d ...\n", (int)param);

    }else if (HCFOTA_REPORT_EVENT_DOWNLOAD_PROGRESS == event){
        printf("download %d ...\n", (int)param);
    }
    return 0;
}

static void *sys_upg_usb_task(void *arg)
{
	uint32_t timeout = (uint32_t)arg;
    char file_path[512];
    uint32_t file_len = 0;
    char *upg_buf = NULL;
    uint32_t read_len;
    FILE *fp = NULL;

    control_msg_t msg = {0};

	//step 1: checking if exit USB file.
    if (API_FAILURE == sys_upg_usb_filepath_get(timeout, file_path))
	   goto usb_upg_ext;	

    msg.msg_type = MSG_TYPE_USB_UPGRADE;
    api_control_send_msg(&msg);
    api_sleep_ms(500);

    //step 2: read USB data to memory
    fp = fopen(file_path, "rb+");
    if (NULL == fp) {
        printf("%s(), line: %d. open %s fail!\n", __func__, __LINE__, file_path);
        msg.msg_type = MSG_TYPE_UPG_STATUS;
        msg.msg_code = UPG_STATUS_USB_OPEN_FILE_ERR;
        api_control_send_msg(&msg);
        goto usb_upg_ext;
    }
    fseek(fp,0,SEEK_END);
    file_len = ftell(fp);
    printf("%s(), line: %d. file length: %d\n", __func__, __LINE__, file_len);
    if (file_len >= MAX_UPG_LENGTH){
        printf("%s(), line: %d. file too big!\n", __func__, __LINE__);
        msg.msg_type = MSG_TYPE_UPG_STATUS;
        msg.msg_code = UPG_STATUS_USB_FILE_TOO_LARGE;
        api_control_send_msg(&msg);
        goto usb_upg_ext;
    }
    upg_buf = (char*)malloc(file_len);
    if (NULL == upg_buf){
        printf("%s(), line: %d. malloc %d buffer fail!\n", 
            __func__, __LINE__, file_len);
        goto usb_upg_ext;
    }

    fseek(fp,0,SEEK_SET);
    read_len = 0;
    int red_cnt = 0;
    while(read_len < file_len){
        red_cnt = fread(upg_buf+read_len, 1, READ_LENGTH, fp);
        if (red_cnt <= 0){
            break;
        }

        read_len += red_cnt;
        //here send message to show
        msg.msg_type = MSG_TYPE_UPG_DOWNLOAD_PROGRESS;
        msg.msg_code = read_len * 100 / file_len;
        api_control_send_msg(&msg);

        api_sleep_ms(50);
        printf("read %d(%d%%)\n",read_len, msg.msg_code);
    }
    if (read_len < file_len){
        printf("%s(), line: %d. read ugrade file error, length: %d/%d!\n", 
            __func__, __LINE__, read_len, file_len);    
        msg.msg_type = MSG_TYPE_UPG_STATUS;
        msg.msg_code = UPG_STATUS_USB_READ_ERR;
        api_control_send_msg(&msg);
        goto usb_upg_ext;

    }
    printf("%s(), line: %d. read ugrade file done: %d!\n", __func__, __LINE__, read_len);


    //step 3: burn USB data to flash
    sys_upg_flash_burn(upg_buf, file_len);

usb_upg_ext:
    if (fp)
        fclose(fp);
    if(upg_buf)
        free(upg_buf);

    printf("%s(), line: %d. exit!\n", __func__, __LINE__);    
    m_usb_upgrade = 0;

    return NULL;
}


static int sys_upg_fail_proc(int fail_ret)
{
    control_msg_t msg = {0};

    if (!fail_ret)
        return API_SUCCESS;

    msg.msg_type = MSG_TYPE_UPG_STATUS;

    switch (fail_ret)
    {
    // case HCFOTA_ERR_PRODUCT_ID:
    //     msg.msg_code = UPG_STATUS_PRODUCT_ID_MISMATCH;
    //     break;
    case HCFOTA_ERR_VERSION:
        msg.msg_code = UPG_STATUS_VERSION_IS_OLD;
        break;
    case HCFOTA_ERR_HEADER_CRC:
        msg.msg_code = UPG_STATUS_FILE_CRC_ERROR;
        break;
    case HCFOTA_ERR_PAYLOAD_CRC:
        msg.msg_code = UPG_STATUS_FILE_CRC_ERROR;
        break;
    case HCFOTA_ERR_DECOMPRESSS:
        msg.msg_code = UPG_STATUS_FILE_UNZIP_ERROR;
        break;
    case HCFOTA_ERR_UPGRADE:
        msg.msg_code = UPG_STATUS_BURN_FAIL;
        break;
    // case HCFOTA_ERR_DOWNLOAD:
    //     break;
    // case HCFOTA_ERR_LOADFOTA:
    //     break;
    default:
        break;
    }

    printf("%s(), line: %d. ugrade fail:%d\n", __func__, __LINE__, fail_ret);
    if (msg.msg_code){
        api_control_send_msg(&msg);

        if (HCFOTA_ERR_UPGRADE == fail_ret){
            api_sleep_ms(3000);        
            api_system_reboot();
        }
    }

    return API_SUCCESS;
}

int sys_upg_flash_burn(char *buff, uint32_t length)
{
    int ret;
    control_msg_t msg = {0};

    msg.msg_type = MSG_TYPE_UPG_STATUS;
    msg.msg_code = UPG_STATUS_BURN_START;
    api_control_send_msg(&msg);
    api_sleep_ms(200);

    //burn USB data to flash
#if 1
    ret = hcfota_memory(buff, length, mtd_upgrade_callback, 0);
#else
    //simulate to burn flash
    int sum_cont = 0;
    while(sum_cont++ < 100){
        mtd_upgrade_callback(HCFOTA_REPORT_EVENT_UPGRADE_PROGRESS, sum_cont, 0);
        api_sleep_ms(100);
    }
    ret = 0;
#endif    

    msg.msg_type = MSG_TYPE_UPG_STATUS;
    if (0 == ret){
        if (m_usb_upgrade){
            printf("%s(), line: %d. ugrade OK. Plugout U-disk, then reboot ...\n", __func__, __LINE__);
            msg.msg_code = UPG_STATUS_BURN_OK;
            api_control_send_msg(&msg);

            //sys_usb_dev_wait_plugout(30000); //wait 30s timeout
            api_sleep_ms(3000);        
            api_system_reboot();
        }else{
            printf("%s(), line: %d. ugrade OK, reboot now ......\n", __func__, __LINE__);
            msg.msg_code = UPG_STATUS_BURN_OK;
            api_control_send_msg(&msg);
            api_sleep_ms(3000);        
            api_system_reboot();
        }
    }else{
        ret = sys_upg_fail_proc(ret);
    }

    return ret;

}

int sys_upg_usb_check(uint32_t timeout)
{
    pthread_t thread_id = 0;
    pthread_attr_t attr;

    if (m_usb_upgrade){
        printf("%s(), line: %d. usb uprade task is running!\n", __func__, __LINE__);
        return API_SUCCESS;
    }
    //create the message task
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 0x2000);
    pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED); //release task resource itself
    if(pthread_create(&thread_id, &attr, sys_upg_usb_task, (void*)timeout)) {
        return API_FAILURE;
    }
    m_usb_upgrade = 1;
    return API_SUCCESS;
}