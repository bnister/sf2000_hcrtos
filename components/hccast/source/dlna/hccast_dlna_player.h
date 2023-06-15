#ifndef _OUTPUT_FFPLAYER_H
#define _OUTPUT_FFPLAYER_H

#include <ffplayer.h>
typedef enum
{
    MPLAYER_PLAYING,
    MPLAYER_PAUSE,
    MPLAYER_STOP,
    MPLAYER_EOS,
    MPLAYER_BUFFERING,
    MPLAYER_ERROR,
} dlna_mediarender_status_e;

typedef struct _dlna_renderer_observer_st_
{
    void (*play)(void);
    void (*pause)(void);
    void (*stop)(void);
    void (*buffer_progress)(int val);
    void (*volume_change)(int val);
    void (*player_event)(HCPlayerMsgType event, HCPlayerMsg);
    void *userdata;
} dlna_renderer_observer_st;

extern struct output_module ffplayer_output;

#endif /*  _OUTPUT_FFPLAYER_H */
