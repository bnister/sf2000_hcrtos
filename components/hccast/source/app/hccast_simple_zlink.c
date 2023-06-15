#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <hcuapi/common.h>
#include <hcuapi/kshm.h>
#include <hcuapi/auddec.h>
#include <hcuapi/viddec.h>
#include <hcuapi/vidmp.h>
#include <hcuapi/codec_id.h>
#include <hcuapi/avsync.h>

#include <libzlink/libzlink.h>

static int g_dump_fd = -1;
static int g_video_started = 0;
static int g_video_fd = -1;

static int zlink_test_video_start(int width, int height)
{
    struct video_config cfg;

    memset(&cfg, 0, sizeof(struct video_config));

    cfg.codec_id = HC_AVCODEC_ID_H264;
    cfg.sync_mode = AVSYNC_TYPE_UPDATESTC;
    cfg.quick_mode = 3;
    cfg.decode_mode = VDEC_WORK_MODE_KSHM;

    cfg.pic_width = width;
    cfg.pic_height = height;
    cfg.frame_rate = 60 * 1000;

    cfg.pixel_aspect_x = 1;
    cfg.pixel_aspect_y = 1;
    cfg.preview = 0;
    cfg.extradata_size = 0;
    cfg.rotate_enable = 1;

    g_video_fd = open("/dev/viddec", O_RDWR);
    if (g_video_fd < 0)
    {
        return -1;
    }

    if (ioctl(g_video_fd, VIDDEC_INIT, &cfg) != 0)
    {
        close(g_video_fd);

        return -1;
    }

    ioctl(g_video_fd, VIDDEC_START, 0);

    return 0;
}

static int zlink_test_video_stop()
{
    if (g_video_fd >= 0)
    {
        close(g_video_fd);
        g_video_fd = -1;
    }

    return 0;
}

int zlink_test_zlink_ready_f(int is_zlink_ready, void *user_data)
{
    printf("[%s - %d] %d\n", __func__, __LINE__, is_zlink_ready);

    if (is_zlink_ready)
    {
        struct SESSION_DATA *session_data = malloc(sizeof(struct SESSION_DATA));

        memset(session_data, 0, sizeof(struct SESSION_DATA));
        session_data->width = 1280;
        session_data->height = 720;
        session_data->fps = 30;
        session_data->is_right_hand = 0;
        session_data->is_night_mode = 1;
        session_data->apple_wired = CARPLAY_WIRED_MODE;
        session_data->apple_wireless = CARPLAY_WIRELESS_MODE;
        session_data->android_wired = ANDROID_WIRED_LINK_NONE;
        session_data->android_wireless = AA_WIRELESS_MODE;
        session_data->platform_id = "hc";
        session_data->vendor_name = "hq";
        session_data->mfi_bus_num = -1;
        session_data->otg_bus_num = -1;

        libzlink_init_session_2(session_data);
    }
}

int zlink_test_session_state_f(enum LIBZLINK_SESSION_STATE session_state,
                               enum PHONE_TYPE phone_type, void *user_data)
{
    printf("[%s - %d] state %d, phone type %d\n", __func__, __LINE__, session_state, phone_type);
#if 0
    if (SESSION_END == session_state && g_dump_fd >= 0)
    {
        close(g_dump_fd);
    }
#endif
    if (SESSION_STARTED == session_state && !g_video_started)
    {
        zlink_test_video_start(1280, 720);
        g_video_started = 1;
    }
    else if (SESSION_END == session_state && g_video_started)
    {
        zlink_test_video_stop();
        g_video_started = 0;
    }

    return 0;
}

void zlink_test_log_f(enum ERROR_CODE error_num, char *log, void *user_data)
{
    printf("[log %d] %s", error_num, log);
}

int zlink_test_phone_time_f(uint64_t now_time_second, int time_zone_minute, void *user_data)
{
    printf("[%s - %d]\n", __func__, __LINE__);
}

int zlink_test_ota_data_f(char *file_name, char *file_md5, int file_total_lenght, char is_last_packet,
                          char *ota_data, int data_len, void *user_data)
{
    printf("[%s - %d]\n", __func__, __LINE__);
}

int zlink_test_video_data_f(char *data, int len, struct VIDEO_SCREEN_INFO *screen_info, void *user_data)
{
    AvPktHd pkthd = {0};

    printf("[%s - %d] %d\n", __func__, __LINE__, len);
#if 0
    if (g_dump_fd < 0)
    {
        g_dump_fd = open("/tmp/carplay.h264", O_RDWR | O_CREAT | O_TRUNC, 0666);
    }

    if (g_dump_fd >= 0)
    {
        write(g_dump_fd, data, len);
        fsync(g_dump_fd);
    }
#endif

    if (!g_video_started)
    {
        return -1;
    }

    pkthd.pts = 0;
    pkthd.dur = 0;
    pkthd.size = len;
    pkthd.flag = AV_PACKET_ES_DATA;
    int head_try_times = 100;//1s
    int data_try_times = 100;//1s

    while (head_try_times)
    {
        if (write(g_video_fd, (uint8_t *)&pkthd, sizeof(AvPktHd)) != sizeof(AvPktHd))
        {
            printf("Write VPktHd fail\n");
            head_try_times --;
            usleep(10 * 1000);
        }
        else
        {
            break;
        }
    }

    while (data_try_times)
    {
        if (write(g_video_fd, data, len) != len)
        {
            printf("Write video_frame error fail\n");
            data_try_times --;
            usleep(10 * 1000);
        }
        else
        {
            break;
        }
    }

    return 0;
}

int zlink_test_main_audio_start_f(enum MEDIA_TYPE media_type, void *user_data)
{
    printf("[%s - %d]\n", __func__, __LINE__);
}

int zlink_test_main_audio_stop_f(enum MEDIA_TYPE media_type, void *user_data)
{
    printf("[%s - %d]\n", __func__, __LINE__);
}

int zlink_test_alt_audio_start_f(int is_navi_audio, void *user_data)
{
    printf("[%s - %d]\n", __func__, __LINE__);
}

int zlink_test_alt_audio_stop_f(void *user_data)
{
    printf("[%s - %d]\n", __func__, __LINE__);
}

int zlink_test_main_audio_data_f(char *data, int len, enum MEDIA_TYPE media_type, int sample,
                                 int channels, int bits, void *user_data)
{
    printf("[%s - %d]\n", __func__, __LINE__);
}

int zlink_test_alt_audio_data_f(char *data, int len, int sample, int channels, int bits, void *user_data)
{
    printf("[%s - %d]\n", __func__, __LINE__);
}
int zlink_test_mic_start_f(enum MEDIA_TYPE media_type, int sample, int channels, int bits, void *user_data)
{
    printf("[%s - %d]\n", __func__, __LINE__);
}

int zlink_test_mic_stop_f(enum MEDIA_TYPE media_type, void *user_data)
{
    printf("[%s - %d]\n", __func__, __LINE__);
}

int zlink_test_video_focus_request_f(int is_hu_focus_on, void *user_data)
{
    printf("[%s - %d]\n", __func__, __LINE__);
}

int zlink_test_audio_focus_request_f(int is_hu_focus_on, void *user_data)
{
    printf("[%s - %d]\n", __func__, __LINE__);
}

int zlink_test_request_disableBT_f(void *user_data)
{
    printf("[%s - %d]\n", __func__, __LINE__);
}

int zlink_test_request_p2p_start_f(void *user_data)
{
    printf("[%s - %d]\n", __func__, __LINE__);
}

int zlink_test_request_wifi_info_f(void *user_data)
{
    printf("[%s - %d]\n", __func__, __LINE__);
}

int zlink_test_cp_mode_change_f(int is_cp_video_on, int is_cp_audio_on, int is_cp_speech_on,
                                enum CP_SPEECH_MODE speech_mode, int is_cp_phonecall_on,
                                int is_cp_turnbyturn_on, void *user_data)
{
    printf("[%s - %d]\n", __func__, __LINE__);
}

int zlink_test_AA_navi_focus_request_f(int is_hu_navi_start, void *user_data)
{
    printf("[%s - %d]\n", __func__, __LINE__);
}

int zlink_test_activate_state_f(enum ACTIVATE_STATE activate_state, void *user_data)
{
    printf("[%s - %d]\n", __func__, __LINE__);
}

int zlink_test_mfi_uuid_f(char *mfi_uuid, void *user_data)
{
    printf("[%s - %d]\n", __func__, __LINE__);
}

int zlink_test_zlink_version_f(char *zlink_ver, void *user_data)
{
    printf("[%s - %d]\n", __func__, __LINE__);
}

int zlink_test_mirror_phone_app_state_f(enum MIRROR_PHONE_APP_STATE app_state, void *user_data)
{
    printf("[%s - %d]\n", __func__, __LINE__);
}

int zlink_test_mirror_state_f(enum MIRROR_STATE mirror_state, void *user_data)
{
    printf("[%s - %d]\n", __func__, __LINE__);
}

static int zlink_test_register_cb()
{
    libzlink_ready_cb_init(zlink_test_zlink_ready_f);
    libzlink_session_state_cb_init(zlink_test_session_state_f);
    libzlink_log_cb_init(zlink_test_log_f);
    libzlink_phone_time_init(zlink_test_phone_time_f);
    libzlink_ota_data_init(zlink_test_ota_data_f);
    libzlink_video_data_cb_init(zlink_test_video_data_f);
    libzlink_main_audio_start_cb_init(zlink_test_main_audio_start_f);
    libzlink_main_data_cb_init(zlink_test_main_audio_data_f);
    libzlink_main_audio_stop_cb_init(zlink_test_main_audio_stop_f);
    libzlink_alt_audio_start_cb_init(zlink_test_alt_audio_start_f);
    libzlink_alt_data_cb_init(zlink_test_alt_audio_data_f);
    libzlink_alt_audio_stop_cb_init(zlink_test_alt_audio_stop_f);
    libzlink_mic_start_cb_init(zlink_test_mic_start_f);
    libzlink_mic_stop_cb_init(zlink_test_mic_stop_f);
    libzlink_video_focus_request_cb_init(zlink_test_video_focus_request_f);
    libzlink_audio_focus_request_cb_init(zlink_test_audio_focus_request_f);
    libzlink_request_disableBT_cb_init(zlink_test_request_disableBT_f);
    libzlink_request_p2p_start_cb_init(zlink_test_request_p2p_start_f);
    libzlink_request_wifi_info_cb_init(zlink_test_request_wifi_info_f);
    libzlink_AA_navi_focus_request_cb_init(zlink_test_AA_navi_focus_request_f);
    libzlink_cp_mode_change_cb_init(zlink_test_cp_mode_change_f);
    libzlink_activate_state_cb_init(zlink_test_activate_state_f);
    libzlink_mirror_phone_app_state_cb_init(zlink_test_mirror_phone_app_state_f);
    libzlink_mirror_state_cb_init(zlink_test_mirror_state_f);

    return 0;
}

int main(int argc, char *argv[])
{
    LIBZLINK_HANDLE zlink_hdl = NULL;
    int activate_type = 0;
    int ret = 0;
    int x = 320, y = 360;

    zlink_test_register_cb();

    zlink_hdl = libzlink_init(NULL);
    if (!zlink_hdl)
    {
        printf("zlink init fail\n");
        return -1;
    }

    while (0 != libzlink_check_ready(zlink_hdl))
    {
        usleep(200 * 1000);
    }
    printf("zlink ready\n");

    ret = libzlink_get_activated_function(&activate_type);
    if (0 == ret)
    {
        printf("activated %.8x\n", activate_type);
    }
    else
    {
        printf("cannot get activated function\n");
    }

    while (1)
    {
        getchar();

        libzlink_touch_event(x, y, 1);
        x += 5;
        y += 5;
    }

    return 0;
}
