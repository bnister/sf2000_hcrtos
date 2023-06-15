#ifndef __HCCAST_COM_H__
#define __HCCAST_COM_H__

#ifdef __cplusplus
extern "C" {
#endif

enum{
	HCCAST_CMD_SND_DEVS_SET = 1,	
	HCCAST_CMD_SND_DEVS_GET,	
};


uint32_t hccast_com_media_control(int cmd, uint32_t param);

#ifdef __cplusplus
}
#endif


#endif