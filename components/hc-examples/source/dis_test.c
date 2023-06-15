#include "dis_test.h"
#include <hcuapi/hdmi_tx.h>
#include <sys/mman.h>


static unsigned char *buf_rgb = NULL;

int c_bt601_yuvL2rgbF[3][3] =
{ {298,0 ,  409},
  {298,-100 ,-208},
  {298,516,0} };

int c_bt709_yuvL2rgbF[3][3] =
{ {298,0 ,  459},
  {298,-55 ,-136},
  {298,541,0} };


int enter_dis_test(int argc , char *argv[])
{
	(void)argc;
	(void)argv;
	return 0;
}

static int unreg_dac(int dac_tpye, int fd)
{
	struct dis_dac_param dac_param = { 0 };
    dac_param.distype = DIS_TYPE_SD;
    dac_param.info.type = dac_tpye;
	ioctl(fd, DIS_UNREGISTER_DAC, &dac_param);
	return 0;
}

static int reg_dac(int dac_tpye, int fd)
{
	struct dis_dac_param dac_param = { 0 };

    dac_param.distype = DIS_TYPE_SD;
    dac_param.info.type = dac_tpye;
    dac_param.info.dac.dacidx.cvbs.cv = (1 << 0);
    dac_param.info.dac.enable = 1;
    dac_param.info.dac.progressive = false;

	ioctl(fd, DIS_REGISTER_DAC, &dac_param);
	return 0;
}

static int sd_set_tvsys(struct dis_tvsys* tvsys, int fd)
{
	tvsys->distype = DIS_TYPE_SD;
	if(tvsys->tvtype == TV_LINE_720_25 || tvsys->tvtype == TV_LINE_1080_25 ||
			tvsys->tvtype == TV_LINE_1080_50 || tvsys->tvtype == TV_PAL) {
		tvsys->tvtype = TV_PAL;
	} else if (tvsys->tvtype == TV_LINE_720_30 || tvsys->tvtype == TV_LINE_1080_30 ||
			tvsys->tvtype == TV_LINE_1080_60 || tvsys->tvtype == TV_LINE_4096X2160_30 ||
			tvsys->tvtype == TV_LINE_3840X2160_30 || tvsys->tvtype == TV_NTSC) {
		tvsys->tvtype = TV_NTSC;
	} else {
		printf("tvtype not common value, please do other choice again\n");
		return 0;
	}
	tvsys->progressive = false;
	printf("sd tvsys->tvtype = %d\n",tvsys->tvtype);
	ioctl(fd , DIS_SET_TVSYS , tvsys); //SD
	return 0;
}

static int dis_tvsys_to_tvtype(enum TVSYS tv_sys, enum TVTYPE *tvtype)
{
	switch(tv_sys){
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
		*tvtype = TV_LINE_4096X2160_30;
		break;

	default:
		*tvtype = TV_LINE_720_30;
	}

	printf("%s:%d: tvtype=%d\n", __func__, __LINE__, *tvtype);

	return 0;
}

int tvsys_test(int argc , char *argv[])
{
	struct dis_tvsys tvsys = { 0 };

	int fd,hdmi_fd = -1;
	long cmd = -1;
	int opt,ret;
	int dac_tpye = DIS_DAC_CVBS;
	int dual_out = 0;

	int set_tv =0;
	opterr = 0;
	optind = 0;
	enum TVSYS tv_sys = 0;
	enum TVTYPE tvtype = 0;


	hdmi_fd = open("/dev/hdmi" , O_RDWR);
	if (hdmi_fd < 0) {
		printf("open hdmi error\n");
		return -1;
	} else {
		ret = ioctl(hdmi_fd, HDMI_TX_GET_EDID_TVSYS, &tv_sys);
		if (ret <0) {
			printf("HDMI_TX_GET_EDID_TVSYS error\n");
			return -1;
		} else {
			dis_tvsys_to_tvtype(tv_sys, &tvtype);
		}
	}

	while((opt = getopt(argc , argv , "c:l:d:t:p:s:a:")) != EOF) {
		switch(opt) {
			case 'c':
				cmd = atoi(optarg);
				break;
			case 'l':
				tvsys.layer = atoi(optarg);
				break;
			case 't':
				tvsys.tvtype = atoi(optarg);
				break;
			case 'd':
				tvsys.distype = atoi(optarg);
				break;
			case 'p':
				tvsys.progressive = atoi(optarg);
				break;
			case 's':
				dual_out = atoi(optarg);
				break;
			case 'a':
				dac_tpye = atoi(optarg);
				break;
			default:
				break;
		}
	}

	if (hdmi_fd >= 0) {
		if (tvtype < tvsys.tvtype &&
            tvsys.tvtype != TV_LINE_1024X768_60&&
            tvsys.tvtype != TV_LINE_1280X960_60 &&
            tvsys.tvtype != TV_LINE_1280X1024_60 )
        {
			printf("tv not support tvtype\n");
			return -1;
		}
	}

	if(tvsys.layer != DIS_LAYER_MAIN && tvsys.layer != DIS_LAYER_AUXP) {
		printf("error layer %d\n" , tvsys.layer);
		return -1;
	}

	if(tvsys.distype != DIS_TYPE_SD && tvsys.distype != DIS_TYPE_HD &&
			tvsys.distype != DIS_TYPE_UHD) {
		printf("error display type %d\n" , tvsys.distype);
		return -1;
	}

	fd = open("/dev/dis" , O_WRONLY);
	if(fd < 0) {
		return -1;
	}

	switch(cmd) {
		case 0:
			cmd = DIS_SET_TVSYS;
			set_tv = 1;
			break;
		case 1:
			cmd = DIS_GET_TVSYS;
			break;
		default:
			break;
	}

	if (set_tv) {
		if (dual_out == 1) {
			unreg_dac(dac_tpye, fd);
			ioctl(fd , cmd , &tvsys); //HD
			sd_set_tvsys(&tvsys, fd); //SD
			reg_dac(dac_tpye, fd);

		} else {
			unreg_dac(dac_tpye, fd);
			ioctl(fd , cmd , &tvsys);
			reg_dac(dac_tpye, fd);
		}
	} else {
		ioctl(fd , cmd , &tvsys);
	}
	close(fd);
	return 0;
}

int zoom_test(int argc , char *argv[])
{
	struct dis_zoom dz = { 0 };
	int fd;
	int opt;

	opterr = 0;
	optind = 0;

	while((opt = getopt(argc , argv , "l:d:x:y:w:h:o:k:j:g:")) != EOF) {
		switch(opt) {
			case 'l':
				dz.layer = atoi(optarg);
				break;
			case 'd':
				dz.distype = atoi(optarg);
				break;
			case 'x':
				dz.src_area.x = atoi(optarg);
				break;
			case 'y':
				dz.src_area.y = atoi(optarg);
				break;
			case 'w':
				dz.src_area.w = atoi(optarg);
				break;
			case 'h':
				dz.src_area.h = atoi(optarg);
				break;
			case 'o':
				dz.dst_area.x = atoi(optarg);
				break;
			case 'k':
				dz.dst_area.y = atoi(optarg);
				break;
			case 'j':
				dz.dst_area.w = atoi(optarg);
				break;
			case 'g':
				dz.dst_area.h = atoi(optarg);
				break;
			default:
				break;
		}
	}

	if(dz.layer != DIS_LAYER_MAIN && dz.layer != DIS_LAYER_AUXP) {
		printf("error layer %d\n" , dz.layer);
		return -1;
	}

	if(dz.distype != DIS_TYPE_SD && dz.distype != DIS_TYPE_HD &&
			dz.distype != DIS_TYPE_UHD) {
		printf("error display type %d\n" , dz.distype);
		return -1;
	}

	fd = open("/dev/dis" , O_WRONLY);
	if(fd < 0) {
		return -1;
	}

	ioctl(fd , DIS_SET_ZOOM , &dz);

	close(fd);

	return 0;
}

int aspect_test(int argc , char *argv[])
{
	struct dis_aspect_mode aspect = { 0 };
	int fd;
	int opt;

	opterr = 0;
	optind = 0;

	while((opt = getopt(argc , argv , "d:t:m:")) != EOF) {
		switch(opt) {
			case 't':
				aspect.tv_mode = atoi(optarg);
				break;
			case 'd':
				aspect.distype = atoi(optarg);
				break;
			case 'm':
				aspect.dis_mode = atoi(optarg);
				break;
			default:
				break;
		}
	}

	if(aspect.distype != DIS_TYPE_SD && aspect.distype != DIS_TYPE_HD &&
			aspect.distype != DIS_TYPE_UHD) {
		printf("error display type %d\n" , aspect.distype);
		return -1;
	}

	if(aspect.distype != DIS_TYPE_SD && aspect.distype != DIS_TYPE_HD &&
			aspect.distype != DIS_TYPE_UHD) {
		printf("error display type %d\n" , aspect.distype);
		return -1;
	}

	fd = open("/dev/dis" , O_WRONLY);
	if(fd < 0) {
		return -1;
	}

	ioctl(fd , DIS_SET_ASPECT_MODE , &aspect);

	close(fd);

	return 0;
}

int rotate_test(int argc , char *argv[])
{
	struct dis_rotate yuv;
	int fd;
	int opt;

	opterr = 0;
	optind = 0;
	yuv.angle = 0;
	yuv.mirror_enable = false;
	yuv.distype = DIS_TYPE_HD;
	while((opt = getopt(argc , argv , "r:m:d")) != EOF) {
		switch(opt) {
			case 'r':
				yuv.angle = (rotate_type_e)atoi(optarg);
				break;
			case 'm':
				yuv.mirror_enable = (bool)atoi(optarg);
				break;
			case 'd':
				yuv.distype = (enum DIS_TYPE)atoi(optarg);
				break;
			default:
				break;
		}
	}

	if(yuv.distype != DIS_TYPE_SD && yuv.distype != DIS_TYPE_HD &&
			yuv.distype != DIS_TYPE_UHD) {
		printf("error display type %d\n" , yuv.distype);
		return -1;
	}

	fd = open("/dev/dis" , O_WRONLY);
	if(fd < 0)
		return -1;

	ioctl(fd,DIS_SET_ROTATE,&yuv);

	close(fd);

	return 0;
}

int winon_test(int argc , char *argv[])
{
	struct dis_win_onoff winon = { 0 };
	int fd;
	int opt;

	opterr = 0;
	optind = 0;

	while((opt = getopt(argc , argv , "l:d:o:")) != EOF) {
		switch(opt) {
			case 'l':
				winon.layer = atoi(optarg);
				break;
			case 'd':
				winon.distype = atoi(optarg);
				break;
			case 'o':
				winon.on = atoi(optarg);
				break;
			default:
				break;
		}
	}

	if(winon.distype != DIS_TYPE_SD && winon.distype != DIS_TYPE_HD &&
			winon.distype != DIS_TYPE_UHD) {
		printf("error display type %d\n" , winon.distype);
		return -1;
	}

	if(winon.layer != DIS_LAYER_MAIN && winon.layer != DIS_LAYER_AUXP) {
		printf("error display layer %d\n" , winon.layer);
		return -1;
	}

	if(winon.on != 0 && winon.layer != 1) {
		printf("error display on %d\n" , winon.on);
		return -1;
	}

	fd = open("/dev/dis" , O_WRONLY);
	if(fd < 0) {
		return -1;
	}

	ioctl(fd , DIS_SET_WIN_ONOFF , &winon);

	close(fd);

	return 0;
}

int venhance_test(int argc , char *argv[])
{
	struct dis_video_enhance vhance = { 0 };
	int fd;
	int opt;

	opterr = 0;
	optind = 0;

	while((opt = getopt(argc , argv , "d:c:g:")) != EOF) {
		switch(opt) {
			case 'c':
				vhance.enhance.enhance_type = atoi(optarg);
				printf("-c enhance_type %d \n",vhance.enhance.enhance_type);
				break;
			case 'd':
				vhance.distype = atoi(optarg);
				printf("-d distype %d \n",vhance.distype);
				break;
			case 'g':
				vhance.enhance.grade = atoi(optarg);
				printf("-g grade %d \n",vhance.enhance.grade);
				break;
			default:
				break;
		}
	}

	if(vhance.distype != DIS_TYPE_SD && vhance.distype != DIS_TYPE_HD &&
			vhance.distype != DIS_TYPE_UHD) {
		printf("error display type %d\n" , vhance.distype);
		return -1;
	}

	fd = open("/dev/dis" , O_WRONLY);
	if(fd < 0) {
		return -1;
	}

	printf("enter ioctl\n");
	ioctl(fd , DIS_SET_VIDEO_ENHANCE , &vhance);
	printf("ioctl done \n");

	close(fd);

	return 0;
}

int enhance_enable_test(int argc , char *argv[])
{
	struct dis_video_enhance vhance = { 0 };
	int fd;
	int opt;

	opterr = 0;
	optind = 0;

	while((opt = getopt(argc , argv , "d:e:")) != EOF) {
		switch(opt) {
			case 'e':
				vhance.enhance.enable = atoi(optarg);
				break;
			case 'd':
				vhance.distype = atoi(optarg);
				break;
			default:
				break;
		}
	}

	if(vhance.distype != DIS_TYPE_SD && vhance.distype != DIS_TYPE_HD &&
			vhance.distype != DIS_TYPE_UHD) {
		printf("error display type %d\n" , vhance.distype);
		return -1;
	}

	fd = open("/dev/dis" , O_WRONLY);
	if(fd < 0) {
		return -1;
	}

	ioctl(fd , DIS_SET_VIDEO_ENHANCE_ENABLE , &vhance);

	close(fd);

	return 0;
}

int hdmi_out_fmt_test(int argc , char *argv[])
{
	struct dis_hdmi_output_pix_fmt vhance = { 0 };
	int fd;
	int opt;
	int ret = 0;

	opterr = 0;
	optind = 0;

	while((opt = getopt(argc , argv, "d:f:")) != EOF) {
		switch(opt) {
			case 'f':
				vhance.pixfmt = atoi(optarg);
				break;
			case 'd':
				vhance.distype = atoi(optarg);
				break;
			default:
				break;
		}
	}

	if(vhance.distype != DIS_TYPE_SD && vhance.distype != DIS_TYPE_HD &&
			vhance.distype != DIS_TYPE_UHD) {
		printf("error display type %d\n" , vhance.distype);
		return -1;
	}
	if (vhance.pixfmt == HC_PIX_FMT_YCBCR_420 || vhance.pixfmt == HC_PIX_FMT_YCBCR_411) {
		 printf("not support hdmi output format %d\n", vhance.pixfmt);
		 return -1;
	}
	fd = open("/dev/dis" , O_WRONLY);
	if(fd < 0) {
		return -1;
	}

	ret = ioctl(fd , DIS_SET_HDMI_OUTPUT_PIXFMT , &vhance);

	if(ret >= 0) {
		if(vhance.pixfmt == HC_PIX_FMT_YCBCR_422)
			printf("SET_HDMI_OUTPUT_PIXFMT HC_PIX_FMT_YCBCR_422 success\n");
		if(vhance.pixfmt == HC_PIX_FMT_YCBCR_444)
			printf("SET_HDMI_OUTPUT_PIXFMT HC_PIX_FMT_YCBCR_444 success\n");
		if(vhance.pixfmt == HC_PIX_FMT_RGB_MODE1)
			printf("SET_HDMI_OUTPUT_PIXFMT HC_PIX_FMT_RGB_MODE1 success\n");
		if(vhance.pixfmt == HC_PIX_FMT_RGB_MODE2)
			printf("SET_HDMI_OUTPUT_PIXFMT HC_PIX_FMT_RGB_MODE2 success\n");
	}

	close(fd);

	return 0;
}

int layerorder_test(int argc , char *argv[])
{
	struct dis_layer_blend_order vhance = { 0 };
	int fd;
	int opt;

	opterr = 0;
	optind = 0;

	while((opt = getopt(argc , argv , "d:o:l:r:g:")) != EOF) {
		switch(opt) {
			case 'o':
				vhance.main_layer = atoi(optarg);
				break;
			case 'l':
				vhance.auxp_layer = atoi(optarg);
				break;
			case 'r':
				vhance.gmas_layer = atoi(optarg);
				break;
			case 'g':
				vhance.gmaf_layer = atoi(optarg);
				break;
			case 'd':
				vhance.distype = atoi(optarg);
				break;
			default:
				break;
		}
	}

	if(vhance.distype != DIS_TYPE_SD && vhance.distype != DIS_TYPE_HD &&
			vhance.distype != DIS_TYPE_UHD) {
		printf("error display type %d\n" , vhance.distype);
		return -1;
	}

	fd = open("/dev/dis" , O_WRONLY);
	if(fd < 0) {
		return -1;
	}

	ioctl(fd , DIS_SET_LAYER_ORDER , &vhance);

	close(fd);

	return 0;
}

int backup_pic_test(int argc , char *argv[])
{
	int distype = 0;

	int fd;
	int opt;

	opterr = 0;
	optind = 0;

	while((opt = getopt(argc , argv, "d:")) != EOF) {
		switch(opt) {
			case 'd':
				distype = atoi(optarg);
				break;

			default:
				break;
		}
	}

	if(distype != DIS_TYPE_SD && distype != DIS_TYPE_HD &&
			distype != DIS_TYPE_UHD) {
		printf("error display type %d\n" , distype);
		return -1;
	}

	fd = open("/dev/dis" , O_WRONLY);
	if(fd < 0) {
		return -1;
	}

	ioctl(fd ,DIS_BACKUP_MP , distype);

	close(fd);

	return 0;
}

int cutoff_test(int argc , char *argv[])
{
	struct dis_cutoff vhance = { 0 };
	int fd;
	int opt;

	opterr = 0;
	optind = 0;

	while((opt = getopt(argc , argv, "d:c:")) != EOF) {
		switch(opt) {
			case 'd':
				vhance.distype = atoi(optarg);
				break;
			case 'c':
				vhance.cutoff = atoi(optarg);
				break;
			default:
				break;
		}
	}

	if(vhance.distype != DIS_TYPE_SD && vhance.distype != DIS_TYPE_HD &&
			vhance.distype != DIS_TYPE_UHD) {
		printf("error display type %d\n" , vhance.distype);
		return -1;
	}

	fd = open("/dev/dis" , O_WRONLY);
	if(fd < 0) {
		return -1;
	}

	ioctl(fd , DIS_SET_CUTOFF , &vhance);

	close(fd);

	return 0;
}

int auto_win_onoff_test(int argc , char *argv[])
{
	struct dis_win_autoonoff vhance = { 0 };
	int fd;
	int opt;

	opterr = 0;
	optind = 0;

	while((opt = getopt(argc , argv, "d:w:")) != EOF) {
		switch(opt) {
			case 'd':
				vhance.distype = atoi(optarg);
				break;
			case 'w':
				vhance.enable = atoi(optarg);
				break;
			default:
				break;
		}
	}

	if(vhance.distype != DIS_TYPE_SD && vhance.distype != DIS_TYPE_HD &&
			vhance.distype != DIS_TYPE_UHD) {
		printf("error display type %d\n" , vhance.distype);
		return -1;
	}

	fd = open("/dev/dis" , O_WRONLY);
	if(fd < 0) {
		return -1;
	}

	ioctl(fd , DIS_SET_WIN_AUTOONOFF , &vhance);

	close(fd);

	return 0;
}

int single_output_test(int argc , char *argv[])
{
	struct dis_single_output vhance = { 0 };
	int fd;
	int opt;

	opterr = 0;
	optind = 0;

	while((opt = getopt(argc , argv, "d:o:")) != EOF) {
		switch(opt) {
			case 'd':
				vhance.distype = atoi(optarg);
				break;
			case 'o':
				vhance.enable = atoi(optarg);
				break;
			default:
				break;
		}
	}

	if(vhance.distype != DIS_TYPE_SD && vhance.distype != DIS_TYPE_HD &&
			vhance.distype != DIS_TYPE_UHD) {
		printf("error display type %d\n" , vhance.distype);
		return -1;
	}

	fd = open("/dev/dis" , O_WRONLY);
	if(fd < 0) {
		return -1;
	}

	ioctl(fd , DIS_SET_SINGLE_OUTPUT , &vhance);

	close(fd);

	return 0;
}

int keystone_param_test(int argc , char *argv[])
{
	struct dis_keystone_param vhance = { 0 };
	vhance.distype = 100;
	int fd;
	int opt;

	opterr = 0;
	optind = 0;

	while((opt = getopt(argc , argv, "d:e:w:i:c:y:b:r:")) != EOF) {
		switch(opt) {
			case 'd':
				vhance.distype = atoi(optarg);
				break;
			case 'c':
				vhance.info.bg_enable = atoi(optarg);
				break;
			case 'e':
				vhance.info.enable = atoi(optarg);
				break;
			case 'w':
				vhance.info.width_up = atoi(optarg);
				break;
			case 'i':
				vhance.info.width_down = atoi(optarg);
				break;
			case 'y':
				vhance.info.bg_ycbcr.y = atoi(optarg);
				break;
			case 'b':
				vhance.info.bg_ycbcr.cb = atoi(optarg);
				break;
			case 'r':
				vhance.info.bg_ycbcr.cr = atoi(optarg);
				break;
			default:
				break;
		}
	}

	if(vhance.distype != DIS_TYPE_SD && vhance.distype != DIS_TYPE_HD &&
			vhance.distype != DIS_TYPE_UHD) {
		printf("error display type or config no distype para %d\n" , vhance.distype);
		return -1;
	}

	fd = open("/dev/dis" , O_WRONLY);
	if(fd < 0) {
		return -1;
	}

	ioctl(fd , DIS_SET_KEYSTONE_PARAM , &vhance);

	close(fd);

	return 0;
}

int get_keystone_param_test(int argc , char *argv[])
{
	struct dis_keystone_param pparm = { 0 };
	int fd;
	int opt;

	opterr = 0;
	optind = 0;

	while((opt = getopt(argc , argv, "d:")) != EOF) {
		switch(opt) {
			case 'd':
				pparm.distype = atoi(optarg);
				break;
			default:
				break;
		}
	}

	fd = open("/dev/dis" , O_WRONLY);
	if(fd < 0) {
		return -1;
	}

	ioctl(fd , DIS_GET_KEYSTONE_PARAM , &pparm);

	close(fd);

	return 0;
}

int bg_color_test(int argc , char *argv[])
{
	struct dis_bg_color vhance = { 0 };
	int fd;
	int opt;

	opterr = 0;
	optind = 0;

	while((opt = getopt(argc , argv, "d:y:b:r:")) != EOF) {
		switch(opt) {
			case 'd':
				vhance.distype = atoi(optarg);
				break;
			case 'y':
				vhance.ycbcr.y = atoi(optarg);
				break;
			case 'b':
				vhance.ycbcr.cb = atoi(optarg);
				break;
			case 'r':
				vhance.ycbcr.cr = atoi(optarg);
				break;
			default:
				break;
		}
	}

	if(vhance.distype != DIS_TYPE_SD && vhance.distype != DIS_TYPE_HD &&
			vhance.distype != DIS_TYPE_UHD) {
		printf("error display type %d\n" , vhance.distype);
		return -1;
	}

	fd = open("/dev/dis" , O_WRONLY);
	if(fd < 0) {
		return -1;
	}

	ioctl(fd ,DIS_SET_BG_COLOR , &vhance);

	close(fd);

	return 0;
}

int reg_dac_test(int argc , char *argv[])
{
	struct dis_dac_param uvhance = { 0 };
	struct dis_dac_param vhance = { 0 };
	struct dis_tvsys tvsys = { 0 };
	int fd;
	int opt;

	opterr = 0;
	optind = 0;

	while((opt = getopt(argc , argv, "d:t:e:f:s:h:p:v:")) != EOF) {
		switch(opt) {
			case 'd':
				vhance.distype = atoi(optarg);
				break;
			case 't':
				vhance.info.type = atoi(optarg);
				break;
			case 'e':
				vhance.info.dac.enable = atoi(optarg);
				break;
			case 'f':
				if(vhance.info.type == DIS_DAC_CVBS) {
					vhance.info.dac.dacidx.cvbs.cv = atoi(optarg);
				} else if (vhance.info.type == DIS_DAC_SVIDEO) {
					vhance.info.dac.dacidx.svideo.sv1 = atoi(optarg);
				} else if (vhance.info.type == DIS_DAC_YUV) {
					vhance.info.dac.dacidx.yuv.y = atoi(optarg);
				} else {
					vhance.info.dac.dacidx.rgb.r = atoi(optarg);
				}
				break;
			case 's':
				if (vhance.info.type == DIS_DAC_SVIDEO) {
					vhance.info.dac.dacidx.svideo.sv2 = atoi(optarg);
				} else if (vhance.info.type == DIS_DAC_YUV) {
					vhance.info.dac.dacidx.yuv.u = atoi(optarg);
				} else if (vhance.info.type == DIS_DAC_RGB) {
					vhance.info.dac.dacidx.rgb.g = atoi(optarg);
				}
				break;
			case 'h':
				if (vhance.info.type == DIS_DAC_YUV) {
					vhance.info.dac.dacidx.yuv.v = atoi(optarg);
				} else if (vhance.info.type == DIS_DAC_RGB) {
					vhance.info.dac.dacidx.rgb.b = atoi(optarg);
				}
				break;
			case 'p':
				vhance.info.dac.progressive = atoi(optarg);
				break;
			case 'v':
				tvsys.tvtype = atoi(optarg);
				break;
			default:
				break;
		}
	}

	if(vhance.distype != DIS_TYPE_SD && vhance.distype != DIS_TYPE_HD &&
			vhance.distype != DIS_TYPE_UHD) {
		printf("error display type %d\n" , vhance.distype);
		return -1;
	}

	fd = open("/dev/dis" , O_WRONLY);
	if(fd < 0) {
		return -1;
	}

	ioctl(fd, DIS_UNREGISTER_DAC, &uvhance);

	tvsys.distype = DIS_TYPE_HD;
	tvsys.layer = DIS_LAYER_MAIN;
	tvsys.progressive = vhance.info.dac.progressive;

	ioctl(fd , DIS_SET_TVSYS , &tvsys);

	if(tvsys.tvtype == TV_LINE_720_25 || tvsys.tvtype == TV_LINE_1080_25 ||
			tvsys.tvtype == TV_LINE_1080_50 || tvsys.tvtype == TV_PAL) {
		tvsys.tvtype = TV_PAL;
	} else if (tvsys.tvtype == TV_LINE_720_30 || tvsys.tvtype == TV_LINE_1080_30 ||
			tvsys.tvtype == TV_LINE_1080_60 || tvsys.tvtype == TV_LINE_4096X2160_30 ||
			tvsys.tvtype == TV_LINE_3840X2160_30 || tvsys.tvtype == TV_NTSC) {
		tvsys.tvtype = TV_NTSC;
	} else {
		printf("tvtype not common value, please do other choice again\n");
		return 0;
	}
	tvsys.distype = DIS_TYPE_SD;

	ioctl(fd , DIS_SET_TVSYS , &tvsys);

	ioctl(fd, DIS_REGISTER_DAC, &vhance);

	close(fd);

	return 0;
}

int under_dac_test(int argc , char *argv[])
{
	struct dis_dac_param vhance = { 0 };
	int fd;
	int opt;

	opterr = 0;
	optind = 0;

	while((opt = getopt(argc , argv, "d:u:")) != EOF) {
		switch(opt) {
			case 'd':
				vhance.distype = atoi(optarg);
				break;
			case 'u':
				vhance.info.type = atoi(optarg);
				break;
			default:
				break;
		}
	}

	if(vhance.distype != DIS_TYPE_SD && vhance.distype != DIS_TYPE_HD &&
			vhance.distype != DIS_TYPE_UHD) {
		printf("error display type %d\n" , vhance.distype);
		return -1;
	}

	fd = open("/dev/dis" , O_WRONLY);
	if(fd < 0) {
		return -1;
	}

	ioctl(fd, DIS_UNREGISTER_DAC, &vhance);

	close(fd);

	return 0;
}

int miracast_vscreen_cb_test(int argc , char *argv[])
{
	struct dis_miracast_vscreen_detect_param mpara = { 0 };
	int fd = -1;
	int opt;

	opterr = 0;
	optind = 0;

	while((opt = getopt(argc , argv , "d:o:")) != EOF) {
		switch(opt) {
			case 'd':
				mpara.distype = atoi(optarg);
				break;
			case 'o':
				mpara.on = atoi(optarg);
				break;
			default:
				break;
		}
	}

	if(mpara.distype != DIS_TYPE_SD && mpara.distype != DIS_TYPE_HD &&
			mpara.distype != DIS_TYPE_UHD) {
		printf("error display type %d\n" , mpara.distype);
		return -1;
	}

	fd = open("/dev/dis" , O_WRONLY);
	if(fd < 0) {
		return -1;
	}

	ioctl(fd , DIS_SET_MIRACAST_VSRCEEN_DETECT , &mpara);

	close(fd);

	return 0;
}

static int dis_get_display_info(struct dis_display_info *display_info )
{
    int fd = -1;

    fd = open("/dev/dis" , O_WRONLY);
    if(fd < 0)
    {
        return -1;
    }

    display_info->distype = DIS_TYPE_HD;
    ioctl(fd , DIS_GET_DISPLAY_INFO , display_info);
    close(fd);
    //printf("w:%lu h:%lu\n", display_info->info.pic_width, display_info->info.pic_height);
    //printf("y_buf:0x%lx size = 0x%lx\n" , (long unsigned int)display_info->info.y_buf , (long unsigned int)display_info->info.y_buf_size);
    //printf("c_buf:0x%lx size = 0x%lx\n" , (long unsigned int)display_info->info.c_buf , (long unsigned int)display_info->info.c_buf_size);

    return 0;
}

int  dis_get_display_info_test(int argc , char *argv[])
{
    struct dis_display_info display_info = { 0 };
    int opt;

    opterr = 0;
    optind = 0;

    while((opt = getopt(argc , argv , "d:")) != EOF)
    {
        switch(opt)
        {
            case 'd':
                display_info.distype = atoi(optarg);
                break;
            default:
                break;
        }
    }

    dis_get_display_info(&display_info);

    return 0;

}


#define TILE_ROW_SWIZZLE_MASK 3
static inline int swizzle_tile_row(int dy)
{
    return (dy & ~TILE_ROW_SWIZZLE_MASK) | ((dy & 1) << 1) | ((dy & 2) >> 1);
}

static inline int calc_offset_mapping1(int stride_in_tile , int x , int y)
{
    int tile_x = x >> 4;
    const int tile_y = y >> 5;
    const int dx = x & 15;
    const int dy = y & 31;
    const int sdy = swizzle_tile_row(dy);
    stride_in_tile >>= 4;
    if(tile_x == stride_in_tile)
        tile_x--;
    return (stride_in_tile * tile_y + tile_x) * 512 + (sdy << 4) + dx;
}

static inline int calc_offset_mapping0(int stride_in_tile , int x , int y)
{
    return (y >> 4) * (stride_in_tile << 4) + (x >> 5) * 512 + (y & 15) * 32 + (x & 31);
}

int Trunc8(int v)
{
    return v >> 8;
}

int Clamp8(int v)
{
    if(v < 0)
    {
        return 0;
    }
    else if(v > 255)
    {
        return 255;
    }
    else
    {
        return v;
    }
}

static void pixel_ycbcrL_to_argbF(unsigned char y , unsigned char cb , unsigned char cr ,
                                  unsigned char *r , unsigned char *g , unsigned char *b ,
                                  int c[3][3])
{
    int  red = 0 , green = 0 , blue = 0;

    const int y1 = (y - 16) * c[0][0];
    const int pr = cr - 128;
    const int pb = cb - 128;
    red = Clamp8(Trunc8(y1 + (pr * c[0][2])));
    green = Clamp8(Trunc8(y1 + (pb * c[1][1] + pr * c[1][2])));
    blue = Clamp8(Trunc8(y1 + (pb * c[2][1])));

    *r = red;
    *g = green;
    *b = blue;
}

static void get_yuv(unsigned char *p_buf_y ,
                    unsigned char *p_buf_c ,
                    int ori_width ,
                    int ori_height ,
                    int src_x ,
                    int src_y ,
                    unsigned char *y ,
                    unsigned char *u ,
                    unsigned char *v ,
                    int tile_mode)
{
    int width = 0;
    //int height = 0;
    (void)ori_height;
    unsigned int src_offset = 0;

    if(tile_mode == 1)
    {
        width = (ori_width + 15) & 0xFFFFFFF0;
        //height = (ori_height + 31) & 0xFFFFFFE0;

        src_offset = calc_offset_mapping1(width , src_x , src_y);
        *y = *(p_buf_y + src_offset);

        src_offset = calc_offset_mapping1(width , (src_x & 0xFFFFFFFE) , src_y >> 1);
        *u = *(p_buf_c + src_offset);
        *v = *(p_buf_c + src_offset + 1);
    }
    else
    {
        width = (ori_width + 31) & 0xFFFFFFE0;
        //height = (ori_height + 15) & 0xFFFFFFF0;
        src_offset = calc_offset_mapping0(width , src_x , src_y);
        *y = *(p_buf_y + src_offset);
        src_offset = calc_offset_mapping0(width , (src_x & 0xFFFFFFFE) , src_y >> 1);
        *u = *(p_buf_c + src_offset);
        *v = *(p_buf_c + src_offset + 1);
    }
}

static void YUV420_RGB(unsigned char *p_buf_y ,
                unsigned  char *p_buf_c ,
                unsigned char *p_buf_out ,
                int ori_width ,
                int ori_height,
                int tile_mode)
{
    unsigned char *p_out = NULL;
    unsigned char r , g , b;
    unsigned char cur_y , cur_u , cur_v;

    int x = 0;
    int y = 0;

    p_out = p_buf_out;

    for(y = 0;y < ori_height;y++)
    {
        for(x = 0;x < ori_width;x++)
        {
            get_yuv(p_buf_y , p_buf_c ,
                    ori_width , ori_height ,
                    x , y ,
                    &cur_y , &cur_u , &cur_v ,
                    tile_mode);
            pixel_ycbcrL_to_argbF(cur_y , cur_u , cur_v , &r , &g , &b , c_bt709_yuvL2rgbF);

            *p_out++ = b;
            *p_out++ = g;
            *p_out++ = r;
            *p_out++ = 0xFF;
        }
    }
}

int dis_get_video_buf_test(int argc , char *argv[])
{
    (void)argc;
    (void)argv;
    int buf_size = 0;
    void *buf = NULL;
    int ret = 0;
    unsigned char *p_buf_y = NULL;
    unsigned char *p_buf_c = NULL;
    int fd = -1;

    struct dis_display_info display_info = { 0 };

    fd = open("/dev/dis" , O_RDWR);
    if(fd < 0)
    {
        return -1;
    }

    ret = dis_get_display_info(&display_info);
    if(ret <0)
    {
        return -1;
    }

    buf = mmap(0 , buf_size , PROT_READ | PROT_WRITE , MAP_SHARED , fd , 0);
    buf_size = display_info.info.y_buf_size * 4;
    buf_rgb = realloc(buf_rgb , buf_size);

    p_buf_y = buf;
    p_buf_c = p_buf_y + (display_info.info.c_buf - display_info.info.y_buf);
    printf("buf =0x%x buf_rgb = 0x%x rgb_buf_size = 0x%x\n", (int)buf, (int)buf_rgb, buf_size);

    YUV420_RGB(p_buf_y ,
               p_buf_c ,
               buf_rgb ,
               display_info.info.pic_width ,
               display_info.info.pic_height,
               display_info.info.de_map_mode);

    close(fd);
    return 0;

}

int dis_dump(int argc , char *argv[])
{
    int opt;
    opterr = 0;
    optind = 0;
    long buf_addr_y = 0;
    long buf_addr_c = 0;
    int len = 0;
    FILE *y_fd = 0;
    FILE *c_fd = 0;
    long long tmp = 0;

    char *m_y_file_name = "/media/sda1/dis_y.bin";
    char *m_c_file_name = "/media/sda1/dis_c.bin";
    y_fd = fopen(m_y_file_name , "w");
    c_fd = fopen(m_c_file_name , "w");

    while((opt = getopt(argc , argv , "y:c:s:")) != EOF)
    {
        switch(opt)
        {
            case 'y':
                tmp = strtoll(optarg , NULL , 16);
                buf_addr_y = (unsigned long)tmp;
                break;
            case 'c':
                tmp = strtoll(optarg , NULL , 16);
                buf_addr_c = (unsigned long)tmp;
                break;
            case 's':
                tmp = strtoll(optarg , NULL , 16);
                len = (unsigned long)tmp;
                break;
            default:
                break;
        }
    }


    printf("buf_addr_y = 0x%lx len = %d\n" , buf_addr_y , len);
    fwrite((char *)buf_addr_y , 1 , len , y_fd);
    fsync((int)y_fd);
    fclose(y_fd);

    if(buf_addr_c != 0)
    {
        printf("buf_addr_c = 0x%lx len = %d\n" , buf_addr_c , len);
        fwrite((char *)buf_addr_c , 1 , len , c_fd);
        fsync((int)c_fd);
        fclose(c_fd);
    }

    return 0;
}
