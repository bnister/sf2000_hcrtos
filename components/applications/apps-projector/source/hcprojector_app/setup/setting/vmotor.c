#include "app_config.h"
#ifdef PROJECTOR_VMOTOR_SUPPORT
#include <string.h>
#include <hcuapi/gpio.h>
#include <sys/ioctl.h>
#include <hcuapi/lvds.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <stddef.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "vmotor.h"
#include "factory_setting.h"
#include "screen.h"
#include <sys/poll.h>
#include <hcuapi/input-event-codes.h>
#include <hcuapi/input.h>
#include "com_api.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <freertos/event_groups.h>
#include <kernel/delay.h>
#include <kernel/completion.h>
#include <kernel/lib/fdt_api.h>

#define VMOTOR_STEP_MAX_STEP    7

struct vmotor_module_priv
{
    int lvdsfd;
	int cref;
    int step_count;
    unsigned int step_time;
    unsigned int padout_in_1;
    unsigned int padout_in_2;
    unsigned int padout_in_3;
    unsigned int padout_in_4;
    unsigned int padin_lens_l;
    unsigned int padin_lens_r;
    unsigned int padin_critical_point;
    unsigned char roll_cocus_flag;//0 disabled 1 enabled
    vmotor_direction_e direction;//0 Forward 1 reverse
};

static struct vmotor_module_priv *gvmotor = NULL;
static struct completion vmotor_task_completion;
static int vmotor_task_start_flag = 0;

static void vmotor_read_thread(void *args)
{
    while(!vmotor_task_start_flag)
    {
        if(projector_get_some_sys_param(P_VMOTOR_COUNT)<=VMOTOR_COUNT_SETP_FORWARD_MAX)
        {
            while(vMotor_get_step_count()>0)
                vMotor_Roll();
        }
        msleep(200);
    }
    if(gvmotor!=NULL)
        free(gvmotor);
    usleep(1000);
    complete(&vmotor_task_completion);
    vTaskDelete(NULL);
}

int vMotor_init(void)
{
    int ret = VMOTOR_RET_SUCCESS;
    int np = 0;
    u32 tmpVal = 0;
    if(gvmotor==NULL)
    {
        gvmotor = (struct vmotor_module_priv *)malloc(sizeof(struct vmotor_module_priv));
        if(gvmotor==NULL)goto error;
        memset(gvmotor,0,sizeof(struct vmotor_module_priv));
    }
    if (gvmotor->cref == 0) {
        np = fdt_node_probe_by_path("/hcrtos/vmotor");
        if(np)
        {
			gvmotor->padout_in_1 = INVALID_VALUE_8;//PINPAD_LVDS_DP7;
			gvmotor->padout_in_2 = INVALID_VALUE_8;//PINPAD_LVDS_DN7;
			gvmotor->padout_in_3 = INVALID_VALUE_8;//PINPAD_LVDS_CP1;
			gvmotor->padout_in_4 = INVALID_VALUE_8;//PINPAD_LVDS_CN1;
			gvmotor->padin_lens_l = (unsigned char)INVALID_VALUE_8;//PINPAD_T01;
			gvmotor->padin_lens_r = (unsigned char)INVALID_VALUE_8;//PINPAD_T00;
			gvmotor->padin_critical_point = (unsigned char)INVALID_VALUE_8;
            fdt_get_property_u_32_index(np,"padout-in-1",           0,&gvmotor->padout_in_1);
            fdt_get_property_u_32_index(np,"padout-in-2",           0,&gvmotor->padout_in_2);
            fdt_get_property_u_32_index(np,"padout-in-3",           0,&gvmotor->padout_in_3);
            fdt_get_property_u_32_index(np,"padout-in-4",           0,&gvmotor->padout_in_4);
            fdt_get_property_u_32_index(np,"padout-lens-l",         0,&gvmotor->padin_lens_l);
            fdt_get_property_u_32_index(np,"padout-lens-r",         0,&gvmotor->padin_lens_r);
            fdt_get_property_u_32_index(np,"padin-critical-point",  0,&gvmotor->padin_critical_point);
        }
		else
		{
			printf("No devices found /hcrtos/vmotor\n");
			goto error;
		}
		vmotor_task_start_flag = 0;
        init_completion(&vmotor_task_completion);
        ret = xTaskCreate(vmotor_read_thread , "vmotor_read_thread" ,
                          0x1000 , &gvmotor , portPRI_TASK_NORMAL , NULL);
        if(ret != pdTRUE)
        {
            printf("kshm recv thread create failed\n");
            goto taskcreate_error;
        }
        gvmotor->step_count = 0;//192 * VMOTOR_COUNT_SETP_FORWARD_MAX;
        gvmotor->step_time = 2000;
        gvmotor->direction = BMOTOR_STEP_FORWARD;
        if(gvmotor->padout_in_1  > PINPAD_MAX || gvmotor->padout_in_2  > PINPAD_MAX || gvmotor->padout_in_3 > PINPAD_MAX || gvmotor->padout_in_4 > PINPAD_MAX)
        {
            gvmotor->lvdsfd=open("/dev/lvds",O_RDWR);
            if(gvmotor->lvdsfd<0)
            {
                printf("%s %d lvds open error\n",__FUNCTION__,__LINE__);
                goto lvds_error;
            }
        }
        else
        {
            gpio_configure(gvmotor->padout_in_1,GPIO_DIR_OUTPUT);//LENS-R
            gpio_configure(gvmotor->padout_in_2,GPIO_DIR_OUTPUT);//LENS-R
            gpio_configure(gvmotor->padout_in_3,GPIO_DIR_OUTPUT);//LENS-R
            gpio_configure(gvmotor->padout_in_4,GPIO_DIR_OUTPUT);//LENS-R
        }
        gpio_configure(gvmotor->padin_lens_l,GPIO_DIR_INPUT);//LENS-L
        gpio_configure(gvmotor->padin_lens_r,GPIO_DIR_INPUT);//LENS-R
        gpio_configure(gvmotor->padin_critical_point,GPIO_DIR_INPUT);//LENS-R
        gvmotor->cref++;
        // if(projector_get_some_sys_param(P_VMOTOR_COUNT)==0)
        // {
        //     gvmotor->step_count = 192*VMOTOR_COUNT_SETP_FORWARD_MAX;
        //     vMotor_Roll();
        // }
        printf("%s %d in_1 :%din_2: %din_3: %din_4: %dlens_l: %dlens_r: %dcritical_point:%d\n",__func__,__LINE__,gvmotor->padout_in_1,gvmotor->padout_in_2,gvmotor->padout_in_3,gvmotor->padout_in_4,gvmotor->padin_lens_l,gvmotor->padin_lens_r,gvmotor->padin_critical_point);
    }
    return VMOTOR_RET_SUCCESS;
lvds_error:
taskcreate_error:
    vmotor_task_start_flag = 1;
    wait_for_completion_timeout(&vmotor_task_completion , 3000);
    free(gvmotor);
error:
    gvmotor = NULL;
    printf("%s %d init error\n",__FUNCTION__,__LINE__);
    return VMOTOR_RET_ERROR;
}

int vMotor_deinit(void)
{
    if(gvmotor==NULL)
        return VMOTOR_RET_ERROR;

	if (gvmotor->cref == 0)
		return VMOTOR_RET_ERROR;

	gvmotor->cref--;
	if (gvmotor->cref == 0) {
        vmotor_task_start_flag = 1;
        wait_for_completion_timeout(&vmotor_task_completion , 3000);
        close(gvmotor->lvdsfd);
        free(gvmotor);
        gvmotor = NULL;
    }
    return VMOTOR_RET_SUCCESS;
}

void vMotor_set_lvds_in_out(unsigned char padctl,bool value)
{
    struct lvds_set_gpio pad;

    if(padctl == INVALID_VALUE_8)
        return;

    if(padctl > PINPAD_MAX)
    {
        if(gvmotor->lvdsfd<0)
        {
            printf("open error%s %d\n",__FUNCTION__,__LINE__);
            return;
        }
        pad.padctl=padctl;
        pad.value=value;
        ioctl(gvmotor->lvdsfd,LVDS_SET_GPIO_OUT,&pad);
    }
    else
    {
        gpio_set_output(padctl,value);
    }
}

int vMotor_set_direction(vmotor_direction_e val)
{
    uint8_t vmotor_count = 0;
    if(gvmotor==NULL)
        return VMOTOR_RET_ERROR;
    vmotor_count = projector_get_some_sys_param(P_VMOTOR_COUNT);
    if(val == BMOTOR_STEP_FORWARD)
    {
        vmotor_count++;
        if(vmotor_count>VMOTOR_COUNT_SETP_FORWARD_MAX)
            vmotor_count = VMOTOR_COUNT_SETP_FORWARD_MAX +1;
    }
    else
    {
        if(vmotor_count>VMOTOR_COUNT_SETP_FORWARD_MAX)
            vmotor_count = VMOTOR_COUNT_SETP_FORWARD_MAX;
        if(vmotor_count == 0)
            vmotor_count = 0;
        else
            vmotor_count--;
    }
    projector_set_some_sys_param(P_VMOTOR_COUNT,vmotor_count);
    // printf("vmotor_count = %d\n",vmotor_count);
    gvmotor->direction = val;
    return VMOTOR_RET_SUCCESS;
}

int vMotor_set_step_time(unsigned int val)
{
    if(gvmotor==NULL)return VMOTOR_RET_ERROR;
    gvmotor->step_time = val;
    return VMOTOR_RET_SUCCESS;
}

int vMotor_set_step_count(int val)
{
    if(gvmotor==NULL)return VMOTOR_RET_ERROR;
    gvmotor->step_count = val;
    return VMOTOR_RET_SUCCESS;
}

int vMotor_get_step_count(void)
{
    if(gvmotor==NULL)return -VMOTOR_RET_ERROR;
    return gvmotor->step_count;
}

int vMotor_Roll_set_cocus_flag(unsigned char val)
{
    if(gvmotor==NULL)return VMOTOR_RET_ERROR;
    gvmotor->roll_cocus_flag = val;
    return VMOTOR_RET_SUCCESS;
}

int vMotor_Roll_get_cocus_flag(void)
{
    if(gvmotor==NULL)return -VMOTOR_RET_ERROR;
    return gvmotor->roll_cocus_flag;
}

int vMotor_Standby(void)
{
    if(gvmotor==NULL)return VMOTOR_RET_ERROR;
    vMotor_set_lvds_in_out(gvmotor->padout_in_1,0);//IN_1
    vMotor_set_lvds_in_out(gvmotor->padout_in_2,0);//IN_2
    vMotor_set_lvds_in_out(gvmotor->padout_in_3,0);//IN_3
    vMotor_set_lvds_in_out(gvmotor->padout_in_4,0);//IN_4
   //OUT1--HiZ  OUT2--HiZ   OUT3--HiZ   OUT4--HiZ
   return VMOTOR_RET_SUCCESS;
}

int vMotor_break(void)
{
    if(gvmotor==NULL)return VMOTOR_RET_ERROR;
    vMotor_set_lvds_in_out(gvmotor->padout_in_1,1);//IN_1
    vMotor_set_lvds_in_out(gvmotor->padout_in_2,1);//IN_2
    vMotor_set_lvds_in_out(gvmotor->padout_in_3,1);//IN_3
    vMotor_set_lvds_in_out(gvmotor->padout_in_4,1);//IN_4
   //OUT1--HiZ  OUT2--HiZ   OUT3--HiZ   OUT4--HiZ
   return VMOTOR_RET_SUCCESS;
}

int vMotor_Roll(void)
{
    static int bMotor_Step=0;
    int i;
    static char ready_to_exit_flag = 0;
    static struct pollfd pfd;
    struct input_event t;
    if(gvmotor==NULL) goto error;
    if(gvmotor->step_count<=0) goto vmotor_end;

    gvmotor->step_count--;
    if(vMotor_Roll_get_cocus_flag()==1)
    {
        if(gvmotor->step_count%192==0)
        {
            if(projector_get_some_sys_param(P_VMOTOR_COUNT)>0)
                projector_set_some_sys_param(P_VMOTOR_COUNT,projector_get_some_sys_param(P_VMOTOR_COUNT)-1);
            pfd.fd = get_ir_key();
            pfd.events = POLLIN | POLLRDNORM;
            if(poll(&pfd, 1, 1) > 0)
            {
                if (read(get_ir_key(), &t, sizeof(t)) == sizeof(t))
                {
                    goto vmotor_end;
                }
            }
        }
    }
    if(ready_to_exit_flag == 0)
    {
        if(gpio_get_input(gvmotor->padin_critical_point)&&gvmotor->direction==BMOTOR_STEP_BACKWARD)
        {
            ready_to_exit_flag = 1;
            gvmotor->step_count = 192;
            gvmotor->direction = BMOTOR_STEP_FORWARD;
        }
    }
    if(gvmotor->direction==BMOTOR_STEP_FORWARD)
    {
        // if(gpio_get_input(gvmotor->padin_lens_r)==0)
        // {
        //     gvmotor->step_count = 0;
        //     goto vmotor_end;
        // }
        bMotor_Step--;
        if(bMotor_Step < 0)
            bMotor_Step=VMOTOR_STEP_MAX_STEP;
    }
    else
    {
        // if(gpio_get_input(gvmotor->padin_lens_l)==0)
        // {
        //     gvmotor->step_count = 0;
        //     goto vmotor_end;
        // }
        bMotor_Step++;
        if(bMotor_Step > VMOTOR_STEP_MAX_STEP)
            bMotor_Step=0;
    }

    switch(bMotor_Step)
    {
        case 0:
            //OUT1--1     OUT2--1     OUT3--1     OUT4--0
            vMotor_set_lvds_in_out(gvmotor->padout_in_1,0);//IN_1
            vMotor_set_lvds_in_out(gvmotor->padout_in_2,0);//IN_2
            vMotor_set_lvds_in_out(gvmotor->padout_in_3,1);//IN_3
            vMotor_set_lvds_in_out(gvmotor->padout_in_4,0);//IN_4
            break;
        //OUT1--1     OUT2--1     OUT3--0     OUT4--0
        case 1:
            vMotor_set_lvds_in_out(gvmotor->padout_in_1,0);//IN_1
            vMotor_set_lvds_in_out(gvmotor->padout_in_2,0);//IN_2
            vMotor_set_lvds_in_out(gvmotor->padout_in_3,1);//IN_3
            vMotor_set_lvds_in_out(gvmotor->padout_in_4,1);//IN_4
            break;
        //OUT1--1     OUT2--1     OUT3--0     OUT4--1
        case 2:
            vMotor_set_lvds_in_out(gvmotor->padout_in_1,0);//IN_1
            vMotor_set_lvds_in_out(gvmotor->padout_in_2,0);//IN_2
            vMotor_set_lvds_in_out(gvmotor->padout_in_3,0);//IN_3
            vMotor_set_lvds_in_out(gvmotor->padout_in_4,1);//IN_4
            break;
        //OUT1--1     OUT2--0     OUT3--0     OUT4--1
        case 3:
            vMotor_set_lvds_in_out(gvmotor->padout_in_1,1);//IN_1
            vMotor_set_lvds_in_out(gvmotor->padout_in_2,0);//IN_2
            vMotor_set_lvds_in_out(gvmotor->padout_in_3,0);//IN_3
            vMotor_set_lvds_in_out(gvmotor->padout_in_4,1);//IN_4
            break;
        //OUT1--1     OUT2--0     OUT3--1     OUT4--1
        case 4:
            vMotor_set_lvds_in_out(gvmotor->padout_in_1,1);//IN_1
            vMotor_set_lvds_in_out(gvmotor->padout_in_2,0);//IN_2
            vMotor_set_lvds_in_out(gvmotor->padout_in_3,0);//IN_3
            vMotor_set_lvds_in_out(gvmotor->padout_in_4,0);//IN_4
            break;
        //OUT1--0     OUT2--0     OUT3--1     OUT4--1
        case 5:
            vMotor_set_lvds_in_out(gvmotor->padout_in_1,1);//IN_1
            vMotor_set_lvds_in_out(gvmotor->padout_in_2,1);//IN_2
            vMotor_set_lvds_in_out(gvmotor->padout_in_3,0);//IN_3
            vMotor_set_lvds_in_out(gvmotor->padout_in_4,0);//IN_4
            break;
        //OUT1--0     OUT2--1     OUT3--1     OUT4--1
        case 6:
            vMotor_set_lvds_in_out(gvmotor->padout_in_1,0);//IN_1
            vMotor_set_lvds_in_out(gvmotor->padout_in_2,1);//IN_2
            vMotor_set_lvds_in_out(gvmotor->padout_in_3,0);//IN_3
            vMotor_set_lvds_in_out(gvmotor->padout_in_4,0);//IN_4
            break;
        //OUT1--0     OUT2--1     OUT3--1     OUT4--0
        case 7:
            vMotor_set_lvds_in_out(gvmotor->padout_in_1,0);//IN_1
            vMotor_set_lvds_in_out(gvmotor->padout_in_2,1);//IN_2
            vMotor_set_lvds_in_out(gvmotor->padout_in_3,1);//IN_3
            vMotor_set_lvds_in_out(gvmotor->padout_in_4,0);//IN_4
            break;
        default:
            vMotor_break();
            break;
    }
    usleep(gvmotor->step_time);

    if(gvmotor->step_count==0)
    {
        if(ready_to_exit_flag == 1)
            projector_set_some_sys_param(P_VMOTOR_COUNT,0);
        goto vmotor_end;
    }

    return VMOTOR_RET_SUCCESS;

vmotor_end:
    vMotor_Standby();
    vMotor_Roll_set_cocus_flag(0);
    ready_to_exit_flag = 0;
    gvmotor->step_count = 0;
    return VMOTOR_RET_SUCCESS;

error:
    return VMOTOR_RET_ERROR;
}

int vMotor_Roll_test(void)
{
    int i;
    vMotor_set_step_count(192);
    vMotor_set_direction(BMOTOR_STEP_FORWARD);
    while(vMotor_get_step_count()>0)
        vMotor_Roll();
    return VMOTOR_RET_SUCCESS;
}

int vMotor_Roll_cocus(void)
{
    struct pollfd pfd;
    struct input_event t;
    pfd.fd = get_ir_key();
    pfd.events = POLLIN | POLLRDNORM;
read_key:
    if(poll(&pfd, 1, 500) > 0)
    {
        printf("get_ir_key() = %d %s %d\n",get_ir_key(),__FUNCTION__,__LINE__);
        if(read(get_ir_key(), &t, sizeof(t)) == sizeof(t))
            goto read_key;
    }
    vMotor_set_step_count(192*VMOTOR_COUNT_SETP_FORWARD_MAX);
    vMotor_set_direction(BMOTOR_STEP_BACKWARD);
    vMotor_Roll_set_cocus_flag(1);
}
#endif
