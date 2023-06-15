#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/socket.h>
#include <cjson/cJSON.h>
#include <unistd.h>

#include "hccast_web_package.h"
#include "hccast_web_interface.h"
#include "hccast_wifi_mgr.h"
#include "hccast_httpd.h"
#include "hccast_mira.h"
#include "hccast_air.h"
#include "hccast_net.h"
#include "hccast_log.h"
#include "hccast_web_upgrade_download.h"
#include "hccast_net.h"
#include "hccast_wifi_ctrl.h"

#define WIFI_SUPPORT

extern unsigned int httpd_get_content_length(void);
extern hccast_httpd_callback httpd_callback;

static unsigned char upg_server[SERVER_LEN] = { 0 };

#ifndef RET_SUCCESS
    #define RET_SUCCESS     0
#endif

static char *hccast_wifi_encrypt_str[] =
{
    "NONE",
    "WEP_OPEN",
    "WEP_AUTO",
    "WPAPSK_TKIP",
    "WPAPSK_AES",
    "WPA2PSK_TKIP",
    "WPA2PSK_AES",
    "WPA2PSK_SAE",
};

void urldecode(const char *str, char *dStr)
{
    int d = 0; /* whether or not the string is decoded */
    char eStr[] = "00"; /* for a hex code */
    strcpy(dStr, str);
    while (!d)
    {
        d = 1;
        int i; /* the counter for the string */

        for (i = 0; (size_t)i < strlen(dStr); ++i)
        {
            if (dStr[i] == '%')
            {
                if (dStr[i + 1] == 0)
                    return ;
                if (isxdigit(dStr[i + 1]) && isxdigit(dStr[i + 2]))
                {
                    d = 0;
                    /* combine the next to numbers into one */
                    eStr[0] = dStr[i + 1];
                    eStr[1] = dStr[i + 2];

                    /* convert it to decimal */
                    long int x = strtol(eStr, NULL, 16);

                    /* remove the hex */
                    memmove(&dStr[i + 1], &dStr[i + 3], strlen(&dStr[i + 3]) + 1);

                    dStr[i] = x;
                }
            }
        }
    }
}

char *wifi_page_get_encrypt_str(int index)
{
    return (hccast_wifi_encrypt_str[index]);
}

static void http_response_json(int client, unsigned char *json)
{
    web_headers_ex(client, NULL, "application/json");

    char buf[BUF_LEN];
    snprintf(buf, BUF_LEN, "Content-Length: %d\r\n", strlen(json));
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "\r\n");
    send(client, buf, strlen(buf), 0);

    send(client, json, strlen(json), 0);
}

static int web_interface_setting(int client, char *param)
{
    char key[32] = {0};
    char value[32] = {0};
    char *str1 = NULL;
    char *str2 = NULL;
    int temp = 0;
    int ratio = 0;

    hccast_log(LL_DEBUG, "%s: %s\n", __func__, param);

    str1 = strstr(param, "key=");
    if (!str1)
    {
        return -1;
    }

    str1 += strlen("key=");
    str2 = strstr(str1, "&");
    strncpy(key, str1, str2 - str1);
    str1 = strstr(param, "value=");
    if (!str1)
    {
        return -1;
    }
    str1 += strlen("value=");
    strncpy(value, str1, 32);
    hccast_log(LL_DEBUG, "key: %s, value: %s\n", key, value);

    if (!strcmp(key, "device_name"))
    {
        if (0 != httpd_callback(HCCAST_HTTPD_SET_DEVICE_NAME, (void *)value, NULL))
        {
            return -1;
        }
    }
    else if (!strcmp(key, "device_password"))
    {
        if (0 != httpd_callback(HCCAST_HTTPD_SET_DEVICE_PSK, (void *)value, NULL))
        {
            return -1;
        }
    }
    else if (!strcmp(key, "resolution"))
    {
        if (!strcmp(value, "1") || !strcmp(value, "2") || !strcmp(value, "3") || !strcmp(value, "4") || !strcmp(value, "5") || !strcmp(value, "6"))
        {
            temp = atoi(value);
            if (0 != httpd_callback(HCCAST_HTTPD_SET_SYS_RESOLUTION, (void *)temp, NULL))
            {
                return -1;
            }
        }
        else
        {
            return -1;
        }
    }
    else if (!strcmp(key, "mirror_model"))
    {
        temp = atoi(value);
        if (!strcmp(value, "1"))
        {
            if (0 != httpd_callback(HCCAST_HTTPD_SET_MIRROR_MODE, (void *)temp, NULL))
            {
                return -1;
            }
        }
        else
        {
            return -1;
        }
    }
    else if (!strcmp(key, "lang"))
    {
        int language = atoi(value);
        if (0 != httpd_callback(HCCAST_HTTPD_SET_BROWSER_LANGUAGE, (void *)language, NULL)) //
        {
            return -1;
        }
    }
    else if (!strcmp(key, "mirror_frame"))
    {
        if (!strcmp(value, "1"))    //30
        {
            temp = 0;
            if (0 != httpd_callback(HCCAST_HTTPD_SET_MIRROR_FRAME, (void *)temp, NULL))
            {
                return -1;
            }
        }
        else if (!strcmp(value, "2"))   // 60
        {
            temp = 1;
            if (0 != httpd_callback(HCCAST_HTTPD_SET_MIRROR_FRAME, (void *)temp, NULL))
            {
                return -1;
            }
        }
        else
        {
            return -1;
        }
    }
    else if (!strcmp(key, "aircast_mode"))
    {
        if (!strcmp(value, "1"))    // mirror+stream
        {
            temp = 0;
            if (0 != httpd_callback(HCCAST_HTTPD_SET_AIRCAST_MODE, (void *)temp, NULL))
            {
                return -1;
            }
        }
        else if (!strcmp(value, "2"))   // mirror only
        {
            temp = 1;
            if (0 != httpd_callback(HCCAST_HTTPD_SET_AIRCAST_MODE, (void *)temp, NULL))
            {
                return -1;
            }
        }
        else if (!strcmp(value, "3"))   // Auto
        {
            temp = 2;
            if (0 != httpd_callback(HCCAST_HTTPD_SET_AIRCAST_MODE, (void *)temp, NULL))
            {
                return -1;
            }
        }
        else
        {
            return -1;
        }
    }
    else if (!strcmp(key, "wifi_mode"))
    {
        temp = strtol(value, NULL, 10);

        int next_ch = httpd_callback(HCCAST_HTTPD_GET_WIFI_HS_CHANNEL_BY_FREQ_MODE, (void *)temp, NULL);

        cJSON *changes = cJSON_CreateObject();
        cJSON *obj = cJSON_CreateObject();

        cJSON_AddNumberToObject(obj, "wifi_channel", next_ch);
        cJSON_AddItemToObject(changes, "changes", obj);

        unsigned char *str = (unsigned char *)cJSON_Print(changes);
        if (str)
        {
            http_response_json(client, str);
            free(str);  // equivalent cJSON_Free(str)
        }

        cJSON_Delete(changes);

        if (0 != httpd_callback(HCCAST_HTTPD_SET_WIFI_FREQ_MODE, (void *)temp, NULL))
        {
            return -1;
        }

        return -1;
    }
    else if (!strcmp(key, "wifi_channel"))
    {
        web_response_ok(client, NULL);

        temp = strtol(value, NULL, 10);
        if (0 != httpd_callback(HCCAST_HTTPD_SET_WIFI_HS_CHANNEL, (void *)temp, NULL))
        {
            return -1;
        }
    }

#if 0
    else if (!strcmp(key, "tv_ratio"))
    {
        ratio = atoi(value);
        if (ratio == 1 || ratio == 2)
        {
            httpd_callback(HCCAST_HTTPD_SET_RATIO_DISPLAY_MODE, (void *)ratio, NULL);
        }
        else
        {
            return -1;
        }
    }
#endif
    else if (!strcmp(key, "bright"))
    {
        temp = atoi(value);
        httpd_callback(HCCAST_HTTPD_SET_SYS_BRIGHT, (void *)temp, NULL);

    }
    else if (!strcmp(key, "contrast"))
    {
        temp = atoi(value);
        httpd_callback(HCCAST_HTTPD_SET_SYS_CONTRAST, (void *)temp, NULL);
    }
    else if (!strcmp(key, "saturation"))
    {
        temp = atoi(value);
        httpd_callback(HCCAST_HTTPD_SET_SYS_SATURATION, (void *)temp, NULL);
    }
    else if (!strcmp(key, "hue"))
    {
        temp = atoi(value);
        httpd_callback(HCCAST_HTTPD_SET_SYS_HUE, (void *)temp, NULL);
    }
    else if (!strcmp(key, "sharpness"))
    {
        temp = atoi(value);
        httpd_callback(HCCAST_HTTPD_SET_SYS_SHARPNESS, (void *)temp, NULL);
    }
    else if (!strcmp(key, "mirror_rotation"))
    {
        if (!strcmp(value, "0"))
        {
            temp = 0;//rotate_0
            if (0 != httpd_callback(HCCAST_HTTPD_SET_MIRROR_ROTATION, (void *)temp, NULL))
            {
                return -1;
            }
        }
        else if (!strcmp(value, "1"))
        {
            temp = 1;//rotate_270
            if (0 != httpd_callback(HCCAST_HTTPD_SET_MIRROR_ROTATION, (void *)temp, NULL))
            {
                return -1;
            }
        }
        else if (!strcmp(value, "2"))
        {
            temp = 2;//rotate_90
            if (0 != httpd_callback(HCCAST_HTTPD_SET_MIRROR_ROTATION, (void *)temp, NULL))
            {
                return -1;
            }
        }
        else if (!strcmp(value, "3"))
        {
            temp = 3;//rotate_180
            if (0 != httpd_callback(HCCAST_HTTPD_SET_MIRROR_ROTATION, (void *)temp, NULL))
            {
                return -1;
            }
        }
        else
        {
            return -1;
        }
    }
    else if (!strcmp(key, "auto_rotation"))
    {
        if (!strcmp(value, "0"))
        {
            temp = 0;
            if (0 != httpd_callback(HCCAST_HTTPD_SET_MIRROR_VSCREEN_AUTO_ROTATION, (void *)temp, NULL))
            {
                return -1;
            }
        }
        else if (!strcmp(value, "1"))
        {
            temp = 1;
            if (0 != httpd_callback(HCCAST_HTTPD_SET_MIRROR_VSCREEN_AUTO_ROTATION, (void *)temp, NULL))
            {
                return -1;
            }
        }
        else
        {
            return -1;
        }
    }
    else
    {
        hccast_log(LL_DEBUG, "Don have this option.\n");
        return -1;
    }

    web_response_ok(client, NULL);
    return 0;
}

static int web_interface_connect(int client, char *param)
{
    int ret = 0;
    bool wifi_connected = 0;
    hccast_wifi_ap_info_t save_ap;
    hccast_wifi_ap_info_t con_ap;
    char pwd[WIFI_MAX_PWD_LEN] = {0};
    char ssid[WIFI_MAX_SSID_LEN] = {0};
    char bssid[WIFI_MAX_SSID_LEN] = {0};
    char entype[32] = {0};
    int index = 0;
    char *token = NULL;
    char *str = NULL;
    int len = 0;
    const char d[] = "&";

    hccast_log(LL_DEBUG, "%s: %s\n", __func__, param);

    token = strtok(param, d);
    while (token != NULL)
    {
        if ((str = strstr(token, "index=")) != NULL)
        {
            len = strlen("index=");
            str += len;
            index = atoi(str);
            str = NULL;
        }
        else if ((str = strstr(token, "bssid=")) != NULL)
        {
            len = strlen("bssid=");
            str += len;
            urldecode(str, bssid);
            str = NULL;
        }
        else if ((str = strstr(token, "ssid=")) != NULL)
        {
            len = strlen("ssid=");
            str += len;
            urldecode(str, ssid);
            str = NULL;
        }
        else if ((str = strstr(token, "password=")) != NULL)
        {
            len = strlen("password=");
            str += len;
            urldecode(str, pwd);
            str = NULL;
        }
        else if ((str = strstr(token, "encryption_type=")) != NULL)
        {
            len = strlen("encryption_type=");
            str += len;
            urldecode(str, entype);
            str = NULL;
        }

        token = strtok(NULL, d);
    }

    hccast_log(LL_NOTICE, "%s: index = %d, ssid = \"%s\", bssid = \"%s\", pwd = \"%s\", entype = \"%s\"\n", __func__, index, ssid, bssid, pwd, entype);

    char cur_ssid[WIFI_MAX_SSID_LEN] = {0};

    wifi_connected = httpd_callback(HCCAST_HTTPD_GET_CUR_WIFI_SSID, (void *)cur_ssid, NULL);

    if ((wifi_connected == true)  && (strcmp(cur_ssid, ssid) == 0))
    {
        ret = -1;
        return ret;
    }

    if(wifi_connected == true)
    {
        httpd_callback(HCCAST_HTTPD_WIFI_SWITCH, NULL, NULL);
    }

    memset(&con_ap, 0, sizeof(hccast_wifi_ap_info_t));
    memset(&save_ap, 0, sizeof(hccast_wifi_ap_info_t));
    strcpy(con_ap.ssid, ssid);

    for (int i = 0; i < 10; i++)
    {
        if (!strcmp(entype, hccast_wifi_encrypt_str[i]))
        {
            con_ap.encryptMode = i;
            con_ap.keyIdx = 1;
            break;
        }
    }

    con_ap.special_ap = 0;

    if (httpd_callback(HCCAST_HTTPD_CHECK_AP_SAVE, (void *)&con_ap, (void *)&save_ap) == RET_SUCCESS)
    {
        hccast_log(LL_INFO, "has been save psk: %s\n", save_ap.pwd);
        strcpy(con_ap.pwd, save_ap.pwd);
    }
    else
    {
        strcpy(con_ap.pwd, pwd);
    }

    hccast_log(LL_DEBUG, "con_ap info: ssid = %s, pwd = %s, entype = %d\n", con_ap.ssid, con_ap.pwd, con_ap.encryptMode);

    ret = httpd_callback(HCCAST_HTTPD_WIFI_AP_CONNECT, (void *)&con_ap, NULL);


    if (ret == 0)
        web_response_ok(client, NULL);

    return ret;
}

static int web_interface_wifi_disconnect(int client, char *param)
{
    int ret = 0;
    bool wifi_connected = 0;
    hccast_wifi_ap_info_t *sel_ap = NULL;
    hccast_wifi_ap_info_t con_ap;
    char ssid[WIFI_MAX_SSID_LEN] = {0};
    int index = 0;
    char *token = NULL;
    char *str = NULL;
    int len = 0;
    const char d[] = "&";

    hccast_log(LL_DEBUG, "%s: %s\n", __func__, param);

    token = strtok(param, d);
    while (token != NULL)
    {
        if ((str = strstr(token, "index=")) != NULL)
        {
            len = strlen("index=");
            str += len;
            index = atoi(str);
            str = NULL;
        }
        else if ((str = strstr(token, "ssid=")) != NULL)
        {
            len = strlen("ssid=");
            str += len;
            urldecode(str, ssid);
            str = NULL;
        }
        token = strtok(NULL, d);
    }

    hccast_log(LL_DEBUG, "%s : index = %d, ssid = \"%s\"\n", __func__, index, ssid);

    httpd_callback(HCCAST_HTTPD_WIFI_AP_DISCONNECT, NULL, NULL);

    if (ret == 0)
        web_response_ok(client, NULL);

    return 0;
}

static int web_interface_wifi_forget(int client, char *param)
{
    int ret = 0;
    bool wifi_connected = false;
    hccast_wifi_ap_info_t save_ap;
    hccast_wifi_ap_info_t check_ap;

    char ssid[WIFI_MAX_SSID_LEN] = {0};
    char bssid[WIFI_MAX_SSID_LEN] = {0};
    char cur_ssid[WIFI_MAX_SSID_LEN] = {0};
    int index = 0, net_index = -1;
    char *token = NULL;
    char *str = NULL;
    int len = 0;
    const char d[] = "&";

    hccast_log(LL_DEBUG, "%s: %s\n", __func__, param);

    token = strtok(param, d);
    while (token != NULL)
    {
        if ((str = strstr(token, "index=")) != NULL)
        {
            len = strlen("index=");
            str += len;
            index = atoi(str);
            str = NULL;
        }
        else if ((str = strstr(token, "bssid=")) != NULL)
        {
            len = strlen("bssid=");
            str += len;
            urldecode(str, bssid);
            str = NULL;
        }
        else if ((str = strstr(token, "ssid=")) != NULL)
        {
            len = strlen("ssid=");
            str += len;
            urldecode(str, ssid);
            str = NULL;
        }
        token = strtok(NULL, d);
    }

    memset(&check_ap, 0, sizeof(hccast_wifi_ap_info_t));
    memset(&save_ap, 0, sizeof(hccast_wifi_ap_info_t));
    strncpy(check_ap.ssid, ssid, sizeof(check_ap.ssid));

    hccast_log(LL_DEBUG, "index = %d, ssid = \"%s\"\n", index, ssid);
    wifi_connected = httpd_callback(HCCAST_HTTPD_GET_CUR_WIFI_SSID, (void *)cur_ssid, NULL);
    hccast_log(LL_DEBUG, "cur ssid: \"%s\", select ssid: \"%s\"\n", cur_ssid, ssid);

    if (httpd_callback(HCCAST_HTTPD_CHECK_AP_SAVE, (void *)&check_ap, (void *)&save_ap) == 0)
    {
        if (wifi_connected)
        {
            if (strcmp(cur_ssid, ssid) == 0)
            {
                httpd_callback(HCCAST_HTTPD_WIFI_AP_DISCONNECT, NULL, NULL);
                httpd_callback(HCCAST_HTTPD_DELETE_WIFI_INFO, (void *)&check_ap, NULL);
            }
            else
            {
                httpd_callback(HCCAST_HTTPD_DELETE_WIFI_INFO, (void *)&check_ap, NULL);
            }
        }
        else
        {
            httpd_callback(HCCAST_HTTPD_DELETE_WIFI_INFO, (void *)&check_ap, NULL);
        }
    }

    if (ret == 0)
        web_response_ok(client, NULL);

    return ret;
}

static int web_interface_upgrade(int client, char *param)
{
    char *token = NULL;
    int ret = 0;
    char deurl[SERVER_LEN] = {0};

    hccast_log(LL_DEBUG, "%s: %s\n", __func__, param);

    if (param == NULL)
    {
        hccast_log(LL_ERROR, "input is NULL!\n");
        return -1;
    }

    token = strstr(param, "url=");
    if (!token)
    {
        hccast_log(LL_ERROR, "input is NULL!\n");
        return -1;
    }

    urldecode(token + strlen("url="), deurl);

    hccast_log(LL_DEBUG, "%s: upgrade url = \"%s\"\n", __func__, deurl);

    if (hccast_web_get_upgrade_status() == 0)
    {
        strncpy(upg_server, deurl, SERVER_LEN);
        hccast_web_uprade_download_start(upg_server);
    }

    if (ret == 0)
        web_response_ok(client, NULL);

    return ret;
}

static int web_interface_upgrade_interrupt(int client, char *param)
{
    int ret = 0;

    hccast_log(LL_DEBUG, "%s: %s\n", __func__, param);

    if (hccast_web_get_upgrade_status() == 1)
    {
        hccast_web_set_user_abort(1);
        ret = 1;
    }

    if (ret == 1)
        web_response_ok(client, NULL);
    else
        web_server_error(client, ret);

    return ret;
}

static int web_interface_misc(int client, char *param)
{
    char *token = NULL;
    const char d[2] = "=";
    int ret = 0;

    hccast_log(LL_DEBUG, "%s: %s\n", __func__, param);

    if (param == NULL)
    {
        hccast_log(LL_ERROR, "input is NULL!\n");
        return -1;
    }

    token = strtok(param, d);
    if (token == NULL)
        return -1;

    token = strtok(NULL, d);
    if (token == NULL)
        return -1;

    hccast_log(LL_DEBUG, "%s: token = \"%s\"\n", __func__, token);

    if (!strcmp(token, "restart"))
    {
        web_response_ok(client, NULL);

        if (httpd_callback)
        {
            httpd_callback(HCCAST_HTTPD_SET_SYS_RESTART, NULL, NULL);
        }
    }
    else if (!strcmp(token, "reset"))
    {
        web_response_ok(client, NULL);

        if (httpd_callback)
        {
            httpd_callback(HCCAST_HTTPD_SET_SYS_RESET, NULL, NULL);
        }
    }
    else
    {
        hccast_log(LL_NOTICE, "Don have this option.\n");
        return -1;
    }

    return 0;
}

static int web_interface_get_info(int client, char *param)
{
    int mirror_mode;
    int tv_sys;
    unsigned char dev_name[128] = {0};
    unsigned char dev_psk[128] = {0};
    unsigned char mac[7] = {0};
    unsigned char upgrade_path[256] = {0};

    unsigned char mac_str[18] = {0};
    unsigned char *str = NULL;
    cJSON *obj = NULL;
    int ret = 0;
    bool wifi_connected = false;

    int resolution = 3;
    int ratio = 0;
    int lang = 0;
    int mirror_frame = 0;
    int aircast_mode = 0;
    int mirror_rotation = 0;
    int mirror_auto_rotation = 0;

    int bright = 0;
    int contrast = 0;
    int saturation = 0;
    int hue = 0;
    int sharpness = 0;
    int wifi_mode_en = 0;
    int wifi_mode    = 0;
    int wifi_hs_channel    = 0;
    char cur_fireware_ver[128] = {0};
    char product_id[16] = {0};
    int not_4k = 0;
    //1.0.0-150000
    snprintf(cur_fireware_ver, 128, "%s-%s", "1.0.0", "270100");

    httpd_callback(HCCAST_HTTPD_GET_DEVICE_NAME, (void *)dev_name, NULL);
    httpd_callback(HCCAST_HTTPD_GET_DEVICE_PSK, (void *)dev_psk, NULL);
    httpd_callback(HCCAST_HTTPD_GET_UPGRADE_URL, (void *)upgrade_path, NULL);
    resolution = httpd_callback(HCCAST_HTTPD_GET_SYS_RESOLUTION, NULL, NULL);
    mirror_mode = httpd_callback(HCCAST_HTTPD_GET_MIRROR_MODE, NULL, NULL);
    //wifi_connected = hccast_wifi_mgr_get_connect_status();
    wifi_connected = httpd_callback(HCCAST_HTTPD_GET_WIFI_CONNECT_STATUS, NULL, NULL);
    ret = httpd_callback(HCCAST_HTTPD_GET_DEVICE_MAC, (void *)mac, NULL);
    lang = httpd_callback(HCCAST_HTTPD_GET_BROWSER_LANGUAGE, NULL, NULL);
    mirror_frame = httpd_callback(HCCAST_HTTPD_GET_MIRROR_FRAME, NULL, NULL);
    aircast_mode = httpd_callback(HCCAST_HTTPD_GET_AIRCAST_MODE, NULL, NULL);

    bright = httpd_callback(HCCAST_HTTPD_GET_SYS_BRIGHT, NULL, NULL);
    contrast = httpd_callback(HCCAST_HTTPD_GET_SYS_CONTRAST, NULL, NULL);
    saturation = httpd_callback(HCCAST_HTTPD_GET_SYS_SATURATION, NULL, NULL);
    hue = httpd_callback(HCCAST_HTTPD_GET_SYS_HUE, NULL, NULL);
    sharpness = httpd_callback(HCCAST_HTTPD_GET_SYS_SHARPNESS, NULL, NULL);

    mirror_rotation = httpd_callback(HCCAST_HTTPD_GET_MIRROR_ROTATION, NULL, NULL);
    mirror_auto_rotation = httpd_callback(HCCAST_HTTPD_GET_MIRROR_VSCREEN_AUTO_ROTATION, NULL, NULL);

    httpd_callback(HCCAST_HTTPD_GET_DEV_PRODUCT_ID, (void *)product_id, NULL);
    httpd_callback(HCCAST_HTTPD_GET_DEV_VERSION, (void *)cur_fireware_ver, NULL);

    wifi_mode_en = httpd_callback(HCCAST_HTTPD_GET_WIFI_FREQ_MODE_EN, NULL, NULL);
    wifi_mode_en &= !wifi_connected;
    wifi_hs_channel = httpd_callback(HCCAST_HTTPD_GET_WIFI_HS_CHANNEL, NULL, NULL);

    wifi_mode = httpd_callback(HCCAST_HTTPD_GET_WIFI_FREQ_MODE, NULL, NULL);
    not_4k = httpd_callback(HCCAST_HTTPD_GET_NOT_SUPPORT_4K, NULL, NULL);

    if (ret == RET_SUCCESS)
    {
        snprintf((char *)mac_str, 18, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    }
    else
    {
        snprintf((char *)mac_str, 18, "%s", "00:00:00:00:00:00");
    }

    obj = cJSON_CreateObject();
    cJSON_AddItemToObject(obj, "product", cJSON_CreateString((char *)product_id)); //for upgrade
    cJSON_AddItemToObject(obj, "version", cJSON_CreateString((char *)cur_fireware_ver)); //for upgrade
    cJSON_AddItemToObject(obj, "chip", cJSON_CreateString("5799c753092289da"));//for upgrade
    cJSON_AddItemToObject(obj, "vendor", cJSON_CreateString("hichip"));//for upgrade

    cJSON_AddItemToObject(obj, "device_name", cJSON_CreateString((char *)dev_name));
    cJSON_AddItemToObject(obj, "device_password", cJSON_CreateString((char *)dev_psk));
    cJSON_AddItemToObject(obj, "resolution", cJSON_CreateNumber(resolution));
    cJSON_AddItemToObject(obj, "mirror_model", cJSON_CreateNumber(mirror_mode));

    //cJSON_AddItemToObject(obj, "upgrade_server", cJSON_CreateString(HTTP_UPG_SERVER));//for upgrade
    cJSON_AddItemToObject(obj, "upgrade_api", cJSON_CreateString(upgrade_path));//for upgrade
    cJSON_AddItemToObject(obj, "mac", cJSON_CreateString((char *)mac_str));
    cJSON_AddItemToObject(obj, "networking", cJSON_CreateBool(wifi_connected));
    cJSON_AddItemToObject(obj, "lang", cJSON_CreateNumber(lang));

    //cJSON_AddItemToObject(obj, "tv_ratio", cJSON_CreateNumber(dev_setting.ratio));

    if (0 == mirror_frame)   //30
    {
        cJSON_AddItemToObject(obj, "mirror_frame", cJSON_CreateNumber(1));
    }
    else
    {
        cJSON_AddItemToObject(obj, "mirror_frame", cJSON_CreateNumber(2));
    }

    if (0 == aircast_mode)   // mirror+stream
    {
        cJSON_AddItemToObject(obj, "aircast_mode", cJSON_CreateNumber(1));
    }
    else if (1 == aircast_mode) //mirror only
    {
        cJSON_AddItemToObject(obj, "aircast_mode", cJSON_CreateNumber(2));
    }
    else if (2 == aircast_mode)
    {
        cJSON_AddItemToObject(obj, "aircast_mode", cJSON_CreateNumber(3));
    }

    cJSON_AddItemToObject(obj, "bright", cJSON_CreateNumber(bright));
    cJSON_AddItemToObject(obj, "contrast", cJSON_CreateNumber(contrast));
    cJSON_AddItemToObject(obj, "saturation", cJSON_CreateNumber(saturation));
    cJSON_AddItemToObject(obj, "hue", cJSON_CreateNumber(hue));
    cJSON_AddItemToObject(obj, "sharpness", cJSON_CreateNumber(sharpness));

    if (0 == wifi_mode_en)
    {
        cJSON_AddItemToObject(obj, "wifi_mode_enabled", cJSON_CreateFalse());
    }
    else
    {
        cJSON_AddItemToObject(obj, "wifi_mode_enabled", cJSON_CreateTrue());
    }

    int wifi_channel_enabled = !wifi_connected;

    if (wifi_channel_enabled)
    {
        cJSON_AddItemToObject(obj, "wifi_channel_enabled", cJSON_CreateTrue());
    }
    else
    {
        cJSON_AddItemToObject(obj, "wifi_channel_enabled", cJSON_CreateFalse());
    }

#ifdef HC_RTOS
    cJSON_AddItemToObject(obj, "wifi_channel_auto_support", cJSON_CreateFalse());
#else
    cJSON_AddItemToObject(obj, "wifi_channel_auto_support", cJSON_CreateFalse());
#endif

    cJSON_AddItemToObject(obj, "wifi_mode", cJSON_CreateNumber(wifi_mode));

    if (1 == wifi_mode)         // 24G
    {
        cJSON_AddItemToObject(obj, "wifi_channel_list", cJSON_CreateString("list1"));
    }
    else if (2 == wifi_mode)    // 5G
    {
        cJSON_AddItemToObject(obj, "wifi_channel_list", cJSON_CreateString("list2"));
    }
    else if (3 == wifi_mode)    // 60G
    {
        //cJSON_AddItemToObject(obj, "wifi_channel_list", cJSON_CreateNumber(wifi_mode));
    }

    cJSON_AddItemToObject(obj, "wifi_channel", cJSON_CreateNumber(wifi_hs_channel));
    cJSON_AddItemToObject(obj, "mirror_rotation", cJSON_CreateNumber(mirror_rotation));
    cJSON_AddItemToObject(obj, "auto_rotation", cJSON_CreateNumber(mirror_auto_rotation));

    if (not_4k)
    {
        cJSON_AddItemToObject(obj, "resolution_4k", cJSON_CreateString("disenabled"));
    }
    else
    {
        cJSON_AddItemToObject(obj, "resolution_4k", cJSON_CreateString("enabled"));
    }

    str = cJSON_Print(obj);
    if (str)
    {
        http_response_json(client, str);
        free(str);  // equivalent cJSON_Free(str)
    }

    cJSON_Delete(obj);

    return 0;
}

static int web_interface_get_wifi(int client, char *param)
{
    int  i = 0, ret = -1;
    unsigned char *str = NULL;
    hccast_wifi_scan_result_t *scan_res = NULL;
    hccast_wifi_ap_info_t *ap_info;
    hccast_wifi_ap_info_t save_ap;
    int cnt = 0;
    cJSON *aplist_obj, *ap_obj;
    bool wifi_connected = 0;
    int wifi_stat = 0;
    char cur_ssid[WIFI_MAX_SSID_LEN] = {0};

    if (httpd_callback)
    {
        httpd_callback(HCCAST_HTTPD_STOP_MIRA_SERVICE, NULL, NULL);
    }

    scan_res = (hccast_wifi_scan_result_t *)calloc(sizeof(hccast_wifi_scan_result_t), 1);
    if (scan_res == NULL)
    {
        hccast_log(LL_ERROR, "%s %d scan_res calloc fail\n", __func__, __LINE__);
        return -1;
    }

    wifi_stat = httpd_callback(HCCAST_HTTPD_WIFI_STAT_CHECK_BUSY, (void *)cur_ssid, NULL);
    if (wifi_stat)
    {
        //return 0;
    }

    wifi_connected = httpd_callback(HCCAST_HTTPD_GET_CUR_WIFI_SSID, (void *)cur_ssid, NULL);
    ret = httpd_callback(HCCAST_HTTPD_WIFI_SCAN, (void *)scan_res, NULL);

    if (scan_res->ap_num > 0)
    {
        aplist_obj = cJSON_CreateArray();

        for (i = 0; i < scan_res->ap_num; i++)
        {
            //fiter some bad ap.
            if (strlen(scan_res->apinfo[i].ssid) > 0)
            {
                char save_flag = 0;

                ap_obj = cJSON_CreateObject();
                cJSON_AddItemToObject(ap_obj, "index", cJSON_CreateNumber(i));//for index.
                cJSON_AddItemToObject(ap_obj, "ssid", cJSON_CreateString(scan_res->apinfo[i].ssid));

                if ((wifi_connected == true) && strlen(scan_res->apinfo[i].ssid) > 0 && (!strcmp(cur_ssid, scan_res->apinfo[i].ssid)))
                {
                    cJSON_AddItemToObject(ap_obj, "state", cJSON_CreateNumber(1));
                }
                else
                {
                    cJSON_AddItemToObject(ap_obj, "state", cJSON_CreateNumber(0));
                }

                cJSON_AddItemToObject(ap_obj, "intensity", cJSON_CreateNumber(scan_res->apinfo[i].quality));

                if (httpd_callback(HCCAST_HTTPD_CHECK_AP_SAVE, &scan_res->apinfo[i], (void *)&save_ap) == RET_SUCCESS)
                {
                    cJSON_AddItemToObject(ap_obj, "saved", cJSON_CreateTrue());
                }
                else
                {
                    cJSON_AddItemToObject(ap_obj, "saved", cJSON_CreateFalse());
                }

                if (scan_res->apinfo[i].encryptMode)
                {
                    cJSON_AddItemToObject(ap_obj, "encryption", cJSON_CreateTrue());
                }
                else
                {
                    cJSON_AddItemToObject(ap_obj, "encryption", cJSON_CreateFalse());
                }

                if (wifi_page_get_encrypt_str(scan_res->apinfo[i].encryptMode))
                {
                    cJSON_AddItemToObject(ap_obj, "encryption_type", cJSON_CreateString(wifi_page_get_encrypt_str(scan_res->apinfo[i].encryptMode)));
                }
                else
                {
                    cJSON_AddItemToObject(ap_obj, "encryption_type", cJSON_CreateString(""));
                }

                cJSON_AddItemToArray(aplist_obj, ap_obj);
            }
        }

        str = (unsigned char *)cJSON_Print(aplist_obj);
        if (str)
        {
            http_response_json(client, str);
            free(str);  // equivalent cJSON_Free(str)
        }

        cJSON_Delete(aplist_obj);
    }
    else
    {
        if (httpd_callback)
        {
            httpd_callback(HCCAST_HTTPD_START_MIRA_SERVICE, NULL, NULL);
        }
        if (scan_res)
        {
            free(scan_res);
            scan_res = NULL;
        }
        return -1;
    }

    if (httpd_callback)
    {
        httpd_callback(HCCAST_HTTPD_START_MIRA_SERVICE, NULL, NULL);
    }

    if (scan_res)
    {
        free(scan_res);
        scan_res = NULL;
    }
    return 0;
}

void send_upload_reply(int client)
{
    char reply_buf[BUF_LEN] = {0};
    web_response_ok(client, NULL);

    snprintf(reply_buf, BUF_LEN, "<HTML><TITLE>Upload success</TITLE>\r\n");
    send(client, reply_buf, strlen(reply_buf), 0);

    snprintf(reply_buf, BUF_LEN, "<BODY style=\"background-color:white;\">\r\n");
    send(client, reply_buf, strlen(reply_buf), 0);

    snprintf(reply_buf, BUF_LEN, "<p style=\"font-family:arial;font-size:40px;text-align:left;\">Upload success </p>\r\n");
    send(client, reply_buf, strlen(reply_buf), 0);

    snprintf(reply_buf, BUF_LEN, "</BODY></HTML>\r\n");
    send(client, reply_buf, strlen(reply_buf), 0);
}

static int web_interface_upload_submit(int client, char *param)
{
    unsigned int length = httpd_get_content_length();
    char *buf = NULL;
    //char *buf = NULL;
    unsigned int recv_len = 0;
    int ret = 0;
    char *fw_buf = NULL;
    unsigned int fw_len = 0;
    int ui_progress = -1;

    buf = (char *) malloc(length);
    if (buf == NULL)
    {
        hccast_log(LL_ERROR, "malloc upload buf fail!\n");
        return -1;
    }
    memset(buf, 0, length);

    httpd_callback(HCCAST_HTTPD_GET_UPLOAD_DATA_START, NULL, NULL);
    while (recv_len < length)
    {
        ret = recv(client, &buf[recv_len], length - recv_len, 0);
        if (ret <= 0)
        {
            httpd_callback(HCCAST_HTTPD_GET_UPLOAD_DATA_FAILED, NULL, NULL);
            free(buf);
            break;
        }
        recv_len += ret;
        int progress = recv_len * 100 / length;
        hccast_log(LL_INFO, "recv_len: %d/%d, %d\n", recv_len, length, progress);
        if (progress != ui_progress)
        {
            httpd_callback(HCCAST_HTTPD_SHOW_PROGRESS, (void *)progress, NULL);
            ui_progress = progress;
        }
    }

    if (recv_len == length)
    {
        fw_buf = buf;
        fw_len = length;

        if (memcmp(buf, "------", 6) == 0)
        {
            char *search = buf;
            while (search < (buf + length))
            {
                search = strchr(search, '\r');
                if (!search)
                    break;

                if (search[1] == '\n')
                {
                    if (search[2] == '\r' && search[3] == '\n')
                    {
                        search += 4;
                        fw_buf = search;
                        fw_len = length - (search - buf);
                        break;
                    }
                }
                search++;
            }
        }

        send_upload_reply(client);

        hccast_web_upload_info_st upload_info;
        upload_info.buf = fw_buf;
        upload_info.length = fw_len;
        httpd_callback(HCCAST_HTTPD_MSG_UPLOAD_DATA_SUC, (void *)&upload_info, NULL);

        if (buf)
        {
            free(buf);
        }
    }
    return 0;
}

//extern char *get_app_release_ver(void);
static int web_interface_device_info_get(int client, char *param)
{
    cJSON *info_jobj = NULL;
    info_jobj = cJSON_CreateObject();
    if (!info_jobj)
    {
        hccast_log(LL_WARNING, "json_object_new_object failed!\n");
        return -1;
    }

    if (httpd_callback)
    {
        httpd_callback(HCCAST_HTTPD_GET_DEV_INFO, NULL, NULL);
    }

    cJSON_AddItemToObject(info_jobj, "product", cJSON_CreateString("todo"));
    cJSON_AddItemToObject(info_jobj, "version", cJSON_CreateString("SDK 1.0"));
    cJSON_AddItemToObject(info_jobj, "date", cJSON_CreateString("AUTO_GEN_SW_COMPILE_TIME_STR"));

    char *str = cJSON_Print(info_jobj);
    if (str)
    {
        http_response_json(client, str);
        free(str);  // equivalent cJSON_Free(str)
    }

    cJSON_Delete(info_jobj);

    return 0;
}


static int web_interface_add_hide_wifi(int client, char *param)
{
    int ret = -1;
#ifdef WIFI_SUPPORT
    int wifi_connected = 0;
    hccast_wifi_ap_info_t sel_ap ;

    char pwd[WIFI_MAX_PWD_LEN] = {0};
    char ssid[WIFI_MAX_SSID_LEN] = {0};
    int mode = 0;
    char *token = NULL;
    char *str = NULL;
    int len = 0;
    const char d[] = "&";
    char safety[20] = {0};
    int encryptMode = 0;
    char cur_ssid[WIFI_MAX_SSID_LEN] = {0};

    token = strtok(param, d);
    while (token != NULL)
    {
        if ((str = strstr(token, "ssid=")) != NULL)
        {
            len = strlen("ssid=");
            str += len;
            urldecode(str, ssid);
            str = NULL;
        }
        else if ((str = strstr(token, "password=")) != NULL)
        {
            len = strlen("password=");
            str += len;
            urldecode(str, pwd);
            str = NULL;
        }
        else if ((str = strstr(token, "safety=")) != NULL)
        {
            len = strlen("safety=");
            str += len;
            urldecode(str, safety);
            str = NULL;
        }
        token = strtok(NULL, d);
    }

    memset(&sel_ap, 0, sizeof(sel_ap));
    strncpy(sel_ap.ssid, ssid, sizeof(ssid));
    sel_ap.special_ap  = 1;//hidden wifi

    if (strstr(safety, "None"))
    {
        sel_ap.encryptMode = HCCAST_WIFI_ENCRYPT_MODE_NONE;
    }
    else if ((strstr(safety, "WEP")) \
             || strstr(safety, "WEP_OPEN") \
             || strstr(safety, "WEP_AUTO"))
    {
        sel_ap.encryptMode = HCCAST_WIFI_ENCRYPT_MODE_SHARED_WEP;
    }
    else if (strstr(safety, "WPA_WPA2") \
             || strstr(safety, "WPAPSK_TKIP") \
             || strstr(safety, "WPAPSK_AES") \
             || strstr(safety, "WPA2PSK_TKIP") \
             || strstr(safety, "WPA2PSK_AES"))
    {
        sel_ap.encryptMode = HCCAST_WIFI_ENCRYPT_MODE_WPA2PSK_AES;
    }
    else if (strstr(safety, "WPA3") \
             || strstr(safety, "WPA2PSK_SAE"))
    {
        sel_ap.encryptMode = HCCAST_WIFI_ENCRYPT_MODE_WPA2PSK_SAE;
    }
    else
        return ret;

    if (sel_ap.encryptMode != 0)
    {
        strncpy(sel_ap.pwd, pwd, sizeof(sel_ap.pwd));
    }

    hccast_log(LL_DEBUG, "%s: mode= %d, ssid= \"%s\", pwd = \"%s\", entype = %d\n", \
               __func__, sel_ap.encryptMode, sel_ap.ssid, sel_ap.pwd, sel_ap.encryptMode);

    wifi_connected = httpd_callback(HCCAST_HTTPD_GET_CUR_WIFI_SSID, (void *)cur_ssid, NULL);
    do
    {
        if ((wifi_connected) && (strcmp(cur_ssid, sel_ap.ssid) == 0))
        {
            ret = -1;
            break;
        }

        ret = httpd_callback(HCCAST_HTTPD_WIFI_AP_CONNECT, (void *)&sel_ap, NULL); //no block the http page display
    }
    while (0);
#endif

    if (ret == 0)
        web_response_ok(client, NULL);

    return ret;
}

static web_interface_st g_interface[] =
{
    {"setting_submit", web_interface_setting},
    {"wifi_submit", web_interface_connect},
    {"upgrade_submit", web_interface_upgrade},
    {"upgrade_interrupt", web_interface_upgrade_interrupt},
    {"misc_submit", web_interface_misc},
    {"setting_get", web_interface_get_info},
    {"wifi_get", web_interface_get_wifi},
    {"wifi_unconnect", web_interface_wifi_disconnect},
    {"wifi_unsaved", web_interface_wifi_forget},
    {"upload_submit", web_interface_upload_submit},
    {"device_info", web_interface_device_info_get},
    {"add_hiddenwifi", web_interface_add_hide_wifi},
};

static web_request_blacklist_st g_request_blacklist[] = 
{
    {"wifi_submit", NULL, 0},
    {"upgrade_submit", NULL, 0},
    {"upgrade_interrupt", NULL, 0},
    {"wifi_get", NULL, 0},
    {"wifi_unconnect", NULL, 0},
    {"wifi_unsaved", NULL, 0},
    {"upload_submit", NULL, 0},
    {"add_hiddenwifi", NULL, 0},
    {"wlan.html", NULL, 1},
    {"wlan_add.html", NULL, 1},
    {"wlan_info.html", NULL, 1},
    {"setting_submit", "wifi_channel", 0},
    {"setting_submit", "wifi_mode", 0},
    {"setting_submit", "aircast_mode", 0},
};

int web_interface_request(int client, char *request, char *param)
{
    int interface_num = sizeof(g_interface) / sizeof(g_interface[0]);
    int i;

    if (!httpd_callback)
    {
        return 0xFF;
    }

    for (i = 0; i < interface_num; i ++)
    {
        if (!strcmp(request, (char *)g_interface[i].request))
        {
            return g_interface[i].process_func(client, param);
        }
    }

    return 0xFF;
}

int web_interface_request_blacklist_check(int client, char *request, char *param)
{
    int blacklist_num = sizeof(g_request_blacklist) / sizeof(g_request_blacklist[0]);
    int i;
    int found = 0;
    
    for (i = 0; i < blacklist_num; i ++)
    {
        if (!strcmp(request, (char *)g_request_blacklist[i].request))
        {
            //check parm
            if(g_request_blacklist[i].param_key)
            {
                if (strstr(param, (char *)g_request_blacklist[i].param_key))
                {
                    found = 1;
                    break;
                }
            }
            else
            {
                found = 1;
                break;
            }
        }
    }

    if(found)
    {
        if(g_request_blacklist[i].resp_type)
        {
            page_hint(client, "System is playing,not allow access server!");
        }
        else
        {
            web_response_server_str(client, "System is playing,not allow access server!");
        }
    }

    return found;
}

