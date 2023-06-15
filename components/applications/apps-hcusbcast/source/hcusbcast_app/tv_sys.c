/*
tv_sys.c: process the tv system/lcd, scale, rotation, etc
 */

#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <fcntl.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include <hcuapi/dis.h>
#include <hcuapi/hdmi_tx.h>
#include <sys/ioctl.h>
#ifdef __linux__
#include <linux/fb.h>
#else
#include <kernel/fb.h>
#include "lv_drivers/display/fbdev.h"
#endif
#include <hcuapi/fb.h>

#include "com_api.h"
#include "osd_com.h"
#include "data_mgr.h"
#include "tv_sys.h"

#define     DEV_HDMI        "/dev/hdmi"
#define     DEV_DIS         "/dev/dis"
#define     DEV_FB          "/dev/fb0"

static int m_best_tv_type = TV_LINE_1080_60;

static int reg_dac(int dac_tpye, int fd)
{
    struct dis_dac_param uvhance = { 0 };

    uvhance.distype = DIS_TYPE_SD;
    uvhance.info.type = dac_tpye;
    uvhance.info.dac.dacidx.cvbs.cv = (1 << 0);
    uvhance.info.dac.enable = 1;
    uvhance.info.dac.progressive = false;

    ioctl(fd, DIS_REGISTER_DAC, &uvhance);
    return 0;
}

static int unreg_dac(int dac_tpye, int fd)
{
    struct dis_dac_param vhance = { 0 };
    vhance.distype = DIS_TYPE_SD;
    vhance.info.type = dac_tpye;
    ioctl(fd, DIS_UNREGISTER_DAC, &vhance);
    return 0;
}

static int sd_set_tvsys(struct dis_tvsys* tvsys, int fd)
{
    tvsys->distype = DIS_TYPE_SD;
    if(tvsys->tvtype == TV_LINE_720_25 || tvsys->tvtype == TV_LINE_1080_25 ||
       tvsys->tvtype == TV_LINE_1080_50 || tvsys->tvtype == TV_PAL)
    {
        tvsys->tvtype = TV_PAL;
    }
    else if (tvsys->tvtype == TV_LINE_720_30 || tvsys->tvtype == TV_LINE_1080_30 ||
             tvsys->tvtype == TV_LINE_1080_60 || tvsys->tvtype == TV_LINE_4096X2160_30 ||
             tvsys->tvtype == TV_LINE_3840X2160_30 || tvsys->tvtype == TV_NTSC)
    {
        tvsys->tvtype = TV_NTSC;
    }
    else
    {
        printf("tvtype not common value, please do other choice again\n");
        return 0;
    }
    tvsys->progressive = false;
    printf("sd tvsys->tvtype = %d\n",tvsys->tvtype);
    ioctl(fd, DIS_SET_TVSYS, tvsys);   //SD
    return 0;
}

static int tvsys_to_tvtype(enum TVSYS tv_sys, enum TVTYPE *tvtype)
{
    switch(tv_sys)
    {
        case TVSYS_480I:
            *tvtype = TV_NTSC;
            break;
        case TVSYS_480P:
            *tvtype = TV_NTSC;
            break;
        case TVSYS_576I :
            *tvtype = TV_PAL;
            break;
        case TVSYS_576P:
            *tvtype = TV_PAL;
            break;
        case TVSYS_720P_50:
            *tvtype = TV_LINE_720_30;
            break;
        case TVSYS_720P_60:
            *tvtype = TV_LINE_720_30;
            break;
        case TVSYS_1080I_25:
            *tvtype = TV_LINE_1080_25;
            break;
        case TVSYS_1080I_30:
            *tvtype = TV_LINE_1080_30;
            break;
        case TVSYS_1080P_24:
            *tvtype = TV_LINE_1080_24;
            break;
        case TVSYS_1080P_25:
            *tvtype = TV_LINE_1080_25;
            break;
        case TVSYS_1080P_30:
            *tvtype = TV_LINE_1080_30;
            break;
        case TVSYS_1080P_50:
            *tvtype = TV_LINE_1080_50;
            break;
        case TVSYS_1080P_60:
            *tvtype = TV_LINE_1080_60;
            break;
        case TVSYS_3840X2160P_30:
            *tvtype = TV_LINE_3840X2160_30;
            break;
        case TVSYS_4096X2160P_30:
            //*tvtype = TV_LINE_4096X2160_30;
            *tvtype = TV_LINE_3840X2160_30;
            break;
        default:
            *tvtype = TV_LINE_720_30;
            break;
    }

    printf("%s:%d: tvtype=%d\n", __func__, __LINE__, *tvtype);

    return 0;
}


static hcfb_scale_t scale_param = { OSD_MAX_WIDTH, OSD_MAX_HEIGHT, 1920, 1080 };// to do later...
static void tv_sys_scale_out(int fd_dis)
{
    int fd_fb = -1;
    struct dis_screen_info screen = { 0 };

    screen.distype = DIS_TYPE_HD;
    if(ioctl(fd_dis, DIS_GET_SCREEN_INFO, &screen))
    {
        return;
    }

    fd_fb = open(DEV_FB, O_RDWR);
    if (fd_fb < 0)
    {
        printf("%s(), line:%d. open device: %s error!\n",
               __func__, __LINE__, DEV_FB);
        return;
    }

    scale_param.h_mul = screen.area.w;
    scale_param.v_mul = screen.area.h;

    ioctl(fd_fb, HCFBIOSET_SCALE, &scale_param);
    close(fd_fb);
}


static void tv_sys_best_tv_type_set(int tv_type)
{
    m_best_tv_type = tv_type;
}

int tv_sys_best_tv_type_get(void)
{
    return m_best_tv_type;
}


static int tv_sys_edid_get(uint32_t timeout)
{
    int fd_hdmi = -1;
    enum TVSYS tv_sys = 0;
    enum TVTYPE tv_type = TV_LINE_1080_60;
    uint32_t loop_cnt = timeout/100;
    int ret = -1;

    fd_hdmi = open(DEV_HDMI, O_RDWR);
    if (fd_hdmi < 0)
    {
        printf("%s(), line:%d. open device: %s error!\n",
               __func__, __LINE__, DEV_HDMI);
        return -1;
    }
    else
    {
        do
        {
            ret = ioctl(fd_hdmi, HDMI_TX_GET_EDID_TVSYS, &tv_sys);
            if (ret >= 0)
                break;
            api_sleep_ms(100);

        }
        while(loop_cnt --);

        if (ret < 0)
        {
            printf("%s(), line:%d, HDMI_TX_GET_EDID_TVSYS error\n", __func__, __LINE__);
        }
        else
        {
            tvsys_to_tvtype(tv_sys, &tv_type);
            tv_sys_best_tv_type_set(tv_type);
        }
    }

    close(fd_hdmi);
    if (ret < 0)
        return -1;

    return tv_type;
}


static int tv_sys_set(bool dual_out, int set_tv_type, bool auto_set)
{
    int fd_dis = -1;
    struct dis_tvsys tvsys = { 0 };
    int dac_tpye = DIS_DAC_CVBS;
    enum TVTYPE tv_type = TV_LINE_1080_60;
    enum TVTYPE real_tv_type = 0;

    tv_type = tv_sys_edid_get(200);
    if ((int)tv_type < 0)
        return API_FAILURE;

    fd_dis = open(DEV_DIS, O_WRONLY);
    if(fd_dis < 0)
    {
        printf("%s(), line:%d. open device: %s error!\n",
               __func__, __LINE__, DEV_DIS);
        return API_FAILURE;
    }

    struct dis_tvsys get_tvsys = { 0 };
    ioctl(fd_dis, DIS_GET_TVSYS, &get_tvsys);  //HD
    printf("%s(), line:%d, current true TV sys: %d!\n",
           __FUNCTION__, __LINE__, get_tvsys.tvtype);


    if (auto_set)
    {
#if 0
        if (get_tvsys.tvtype == tv_type)
        {
            printf("%s(), line:%d, same TV SYS, not need to change!\n",
                   __FUNCTION__, __LINE__);
            return API_SUCCESS;
        }
#endif
        real_tv_type = tv_type;
    }
    else
    {
        if (tv_type < set_tv_type)
        {
            printf("tv not support tvtype: tv_type=%d, set_tv_type:%d\n",
                   tv_type, set_tv_type);
            return API_FAILURE;
        }

#if 0
        if (get_tvsys.tvtype == set_tv_type)
        {
            printf("%s(), line:%d, same TV SYS, not need to change!\n",
                   __FUNCTION__, __LINE__);
            return API_SUCCESS;
        }
#endif

        real_tv_type = set_tv_type;
    }

    tvsys.distype = DIS_TYPE_HD;

    tvsys.layer = DIS_LAYER_MAIN;//1;
    tvsys.tvtype = real_tv_type;//TV_LINE_1080_25;
    switch (real_tv_type)
    {
        case TV_LINE_720_25:
        case TV_LINE_720_30:
        case TV_LINE_1080_50:
        case TV_LINE_1080_60:
        case TV_LINE_4096X2160_30:
        case TV_LINE_3840X2160_30:
            tvsys.progressive = 1;
            break;
        default:
            break;
    }


    if (dual_out)
    {
        unreg_dac(dac_tpye, fd_dis);
        printf("%s(), line: %d, dual output set TV sys: %d\n", __FUNCTION__, __LINE__, tvsys.tvtype);
        ioctl(fd_dis, DIS_SET_TVSYS, &tvsys);  //HD
        sd_set_tvsys(&tvsys, fd_dis); //SD
        reg_dac(dac_tpye, fd_dis);

    }
    else
    {
        unreg_dac(dac_tpye, fd_dis);
        printf("%s(), line: %d, single output set TV sys: %d\n", __FUNCTION__, __LINE__, tvsys.tvtype);
        ioctl(fd_dis, DIS_SET_TVSYS, &tvsys);
        reg_dac(dac_tpye, fd_dis);
    }

    tv_sys_scale_out(fd_dis);

    close(fd_dis);

    return API_SUCCESS;
}


int tv_sys_app_sys_to_de_sys(int app_tv_sys)
{
    int tv_type = TV_LINE_1080_60;

    switch(app_tv_sys)
    {
        case APP_TV_SYS_480P:
            tv_type = TV_NTSC;
            break;
        case APP_TV_SYS_576P:
            tv_type = TV_PAL;
            break;
        case APP_TV_SYS_720P:
            tv_type = TV_LINE_720_30;
            break;
        case APP_TV_SYS_1080P:
            tv_type = TV_LINE_1080_60;
            break;
        case APP_TV_SYS_4K:
            // tv_type = TV_LINE_4096X2160_30;
            tv_type = TV_LINE_3840X2160_30;
            break;
        default:
            break;
    }

    return tv_type;
}

static int tv_sys_de_sys_to_app_sys(int tv_type)
{
    int app_tv_sys = APP_TV_SYS_1080P;

    switch(tv_type)
    {
        case TV_NTSC:
            app_tv_sys = APP_TV_SYS_480P;
            break;
        case TV_PAL:
            app_tv_sys = APP_TV_SYS_576P;
            break;
        case TV_LINE_720_30:
            app_tv_sys = APP_TV_SYS_720P;
            break;
        case TV_LINE_1080_60:
            app_tv_sys = APP_TV_SYS_1080P;
            break;
        case TV_LINE_3840X2160_30:
            app_tv_sys = APP_TV_SYS_4K;
            break;
        default:
            break;
    }
    return app_tv_sys;
}




//Set TV system, if TV can not support the tv system, return API_FAILURE.
//return API_SUCCESS if set/change TV sys OK
int tv_sys_app_set(int app_tv_sys)
{
    int convert_tv_type = TV_LINE_1080_60;
    bool dual_out = false;
    bool auto_set = false;

    if (APP_TV_SYS_AUTO == app_tv_sys)
        auto_set = true;
    convert_tv_type = tv_sys_app_sys_to_de_sys(app_tv_sys);

    return tv_sys_set(dual_out, convert_tv_type, auto_set);
}

static int tv_sys_match_tv_type_get(int tv_type){

	int match_tvtype = TV_LINE_1080_60;
	switch (tv_type)
	{
	case TV_PAL: 
	case TV_PAL_M: 
	case TV_PAL_N: 
	case TV_PAL_60: 
		match_tvtype = TV_PAL;
		break;
	case TV_NTSC: 
	case TV_NTSC_443: 
	case TV_LINE_640X480_50:
		match_tvtype = TV_NTSC;
		break;
	case TV_LINE_720_25: 
	case TV_LINE_720_30: 
	case TV_LINE_800X480_60:
	case TV_LINE_1024X768_60:
	case TV_LINE_1360X768_60:
	case TV_LINE_1280X960_60:
	case TV_LINE_1280X1024_60:
	case TV_LINE_1024X768_50:
	case TV_LINE_1080_55:
	case TV_LINE_768X1024_60:
		match_tvtype = TV_LINE_720_30;
		break;
	case TV_LINE_1080_25: 
	case TV_LINE_1080_30: 
	case TV_LINE_1080_50: 
	case TV_LINE_1080_60: 
	case TV_LINE_1080_24: 
		match_tvtype = TV_LINE_1080_60;
		break;
	case TV_LINE_4096X2160_30: 
	case TV_LINE_3840X2160_30: 
		match_tvtype = TV_LINE_3840X2160_30;
		break;
	default: 
		break;
	}
	return match_tvtype;
}



static int _tv_sys_auto_set(bool dual_out, int set_tv_type, bool auto_set, uint32_t timeout)
{
    int fd_dis = -1;
    struct dis_tvsys tvsys = { 0 };
    int dac_tpye = DIS_DAC_CVBS;

    enum TVTYPE tv_type = TV_LINE_1080_60;
    enum TVTYPE real_tv_type = 0;

    printf("%s(), line:%d, set_tv_type: %d, auto_set: %d!\n",
           __FUNCTION__, __LINE__, set_tv_type, auto_set);

    tv_type = tv_sys_edid_get(timeout);
    if ((int)tv_type < 0)
    {
        return -1;
    }

    fd_dis = open(DEV_DIS, O_WRONLY);
    if(fd_dis < 0)
    {
        printf("%s(), line:%d, open device: %s error!\n",
               __func__, __LINE__, DEV_DIS);
        return -1;
    }

    struct dis_tvsys get_tvsys = { 0 };
    ioctl(fd_dis, DIS_GET_TVSYS, &get_tvsys);  //HD
    printf("%s(), line:%d, current true TV sys: %d!\n",
           __FUNCTION__, __LINE__, get_tvsys.tvtype);


    if (auto_set)
    {
#if 0
        if (get_tvsys.tvtype == tv_type)
        {
            printf("%s(), line:%d, same TV SYS, not need to change!\n",
                   __FUNCTION__, __LINE__);
            return API_SUCCESS;
        }
#endif
        real_tv_type = tv_type;
    }
    else
    {
        if (tv_type < set_tv_type)
        {
            printf("tv not support tvtype: tv_type=%d, set_tv_type:%d\n",
                   tv_type, set_tv_type);

            int match_tvtpye;
            match_tvtpye = tv_sys_match_tv_type_get(tv_type);
            printf("%s(), line:%d, set TV type: %d!\n",
                   __FUNCTION__, __LINE__, match_tvtpye);

            real_tv_type = match_tvtpye;
        }
        else
            real_tv_type = set_tv_type;


#if 0
        if (get_tvsys.tvtype == set_tv_type)
        {
            printf("%s(), line:%d, same TV SYS, not need to change!\n",
                   __FUNCTION__, __LINE__);
            return -1;
        }
#endif

    }

    tvsys.distype = DIS_TYPE_HD;

    tvsys.layer = DIS_LAYER_MAIN;//1;
    tvsys.tvtype = real_tv_type;//TV_LINE_1080_25;
    switch (real_tv_type)
    {
        case TV_LINE_720_25:
        case TV_LINE_720_30:
        case TV_LINE_1080_50:
        case TV_LINE_1080_60:
        case TV_LINE_4096X2160_30:
        case TV_LINE_3840X2160_30:
            tvsys.progressive = 1;
            break;
        default:
            break;
    }


    if (dual_out)
    {
        unreg_dac(dac_tpye, fd_dis);
        printf("%s(), line: %d, dual output set TV sys: %d\n", __FUNCTION__, __LINE__, tvsys.tvtype);
        ioctl(fd_dis, DIS_SET_TVSYS, &tvsys);  //HD
        sd_set_tvsys(&tvsys, fd_dis); //SD
        reg_dac(dac_tpye, fd_dis);

    }
    else
    {
        unreg_dac(dac_tpye, fd_dis);
        printf("%s(), line: %d, single output set TV sys: %d\n", __FUNCTION__, __LINE__, tvsys.tvtype);
        ioctl(fd_dis, DIS_SET_TVSYS, &tvsys);
        reg_dac(dac_tpye, fd_dis);
    }

    tv_sys_scale_out(fd_dis);

    close(fd_dis);

    return real_tv_type;
}


//Auto set TV system, if TV can not support the tv system, auto set the supported TV sys
//(by EDID) return the supported TV sys. In general, the api used in HDMI plug in
int tv_sys_app_auto_set(int app_tv_sys, uint32_t timeout)
{
    int convert_tv_type = TV_LINE_1080_60;
    bool dual_out = false;
    bool auto_set = false;
    int support_tv_type;

    if (APP_TV_SYS_AUTO == app_tv_sys)
        auto_set = true;
    convert_tv_type = tv_sys_app_sys_to_de_sys(app_tv_sys);

	support_tv_type = _tv_sys_auto_set(dual_out, convert_tv_type, auto_set, timeout);
	if (support_tv_type < 0)
		return -1;
	return tv_sys_de_sys_to_app_sys(support_tv_type);
}


//Auto check if need to reset tv system, it use in bootup application.
//Because the bootloader would not set TV sys by EDID. So application
//must check it, if the TV sys in sys data is less than TV sys in EDIT,
//not need to reset TV sys.
int tv_sys_app_start_set(int check)
{
    int ap_tv_sys;
    int ap_tv_sys_ret;

    ap_tv_sys = data_mgr_app_tv_sys_get();

    if (check)
    {
        enum TVTYPE edid_tv_type = TV_LINE_1080_60;
        enum TVTYPE de_tv_type;
        edid_tv_type = tv_sys_edid_get(200);
        if ((int)edid_tv_type < 0)
            return -1;

        de_tv_type = data_mgr_de_tv_sys_get();

        if (APP_TV_SYS_AUTO == ap_tv_sys)
        {
            if (edid_tv_type == de_tv_type)
            {
                printf("%s(), line: %d, auto mode: de_tv_type=%d, not reset tv sys!\n",
                       __func__, __LINE__, de_tv_type);

                return 0;
            }
        }
        else
        {
            if (edid_tv_type >= de_tv_type)
            {
                printf("%s(), line: %d, edid_tv_type=%d, de_tv_type=%d, not reset tv sys!\n",
                       __func__, __LINE__, edid_tv_type, de_tv_type);

                return 0;
            }
        }
    }

    ap_tv_sys_ret = tv_sys_app_auto_set(ap_tv_sys, 2000);
    if (ap_tv_sys_ret >= 0)
    {
        if (APP_TV_SYS_AUTO == ap_tv_sys)
            data_mgr_app_tv_sys_set(APP_TV_SYS_AUTO);
        else
            data_mgr_app_tv_sys_set(ap_tv_sys_ret);
        data_mgr_save();
    }

    return 0;
}


