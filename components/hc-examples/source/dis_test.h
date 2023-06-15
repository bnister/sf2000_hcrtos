#ifndef __DIS_TEST__
#define __DIS_TEST__

#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <hcuapi/dis.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <unistd.h>

int enter_dis_test(int argc , char *argv[]);
int tvsys_test(int argc , char *argv[]);
int zoom_test(int argc , char *argv[]);
int aspect_test(int argc , char *argv[]);
int rotate_test(int argc , char *argv[]);
int winon_test(int argc , char *argv[]);
int venhance_test(int argc , char *argv[]);
int enhance_enable_test(int argc , char *argv[]);
int hdmi_out_fmt_test(int argc , char *argv[]);
int layerorder_test(int argc , char *argv[]);
int backup_pic_test(int argc , char *argv[]);
int cutoff_test(int argc , char *argv[]);
int auto_win_onoff_test(int argc , char *argv[]);
int single_output_test(int argc , char *argv[]);
int keystone_param_test(int argc , char *argv[]);
int get_keystone_param_test(int argc , char *argv[]);
int bg_color_test(int argc , char *argv[]);
int reg_dac_test(int argc , char *argv[]);
int under_dac_test(int argc , char *argv[]);
int miracast_vscreen_cb_test(int argc , char *argv[]);
int dis_get_display_info_test(int argc , char *argv[]);
int dis_dump(int argc , char *argv[]);
int dis_get_video_buf_test(int argc , char *argv[]);
#endif
