#ifndef __HC_SCENE_SWITCH_H_
#define __HC_SCENE_SWITCH_H_

typedef enum
{
    HCCAST_SCENE_NONE,
    HCCAST_SCENE_AIRCAST_PLAY,
    HCCAST_SCENE_AIRCAST_MIRROR,
    HCCAST_SCENE_DLNA_PLAY,
    HCCAST_SCENE_MIRACAST,
    HCCAST_SCENE_IUMIRROR,
    HCCAST_SCENE_AUMIRROR,
    HCCAST_SCENE_MAX,
} hccast_scene_e;

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*hccast_scene_air_event_callback)(int event_type, void* param);

int hccast_scene_switch(int next_scene);
void hccast_scene_reset(int cur_scene, int next_scene);
int hccast_get_current_scene(void);
void hccast_set_current_scene(int scene);
void hccast_scene_playback_end_reset(void);
void hccast_scene_init(void);
void hccast_scene_mira_stop(void);
void hccast_scene_set_mira_restart_flag(int restart);
void hccast_scene_set_mira_is_restarting(int flag);
int hccast_scene_get_switching(void);
void hccast_scene_air_event_init(hccast_scene_air_event_callback air_cb);


#ifdef __cplusplus
}
#endif

#endif
