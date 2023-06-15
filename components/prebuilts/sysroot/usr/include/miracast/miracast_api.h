#ifndef _MIRACAST_API_H_
#define _MIRACAST_API_H_

typedef enum
{
    WFD_DEFAULT = 0,
    WFD_DISCONNECTED,                       //+
    WFD_DISCOVERY,                          //+
    WFD_TRIGGERING,                         //+
    WFD_SECURITY_CHECK,                     //+
    WFD_SECURITY_SUCCESSFUL,                //+
    WFD_SMOOTH_HANDOFF,
    WFD_CONNECTED,                          //+
    WFD_START_PLAYER,                       //+
    WFD_STOP_PLAYER,                        //+
    WFD_SCAN_START,                         //10

    WFD_START_TO_DISPLAY,                   //+
    WFD_SHUTDOWN,
    WFD_DHCP_ERROR,
    WFD_WIFI_LOAD,
    WFD_INIT_DONE,
    WFD_NO_WIFI,
    WFD_SET_SSID_DONE,
    WFD_DLNA_SUPPORT,
    WFD_MIRACAST_SUPPORT,
    WFD_HDCP_PORT, //report HDCP port

    WFD_WIFI_FREQUENCY24,
    WFD_WIFI_FREQUENCY5,
    WFD_EVENT_P2P_RECV_PROV_REQ,            // RECV_PROV_REQ
    WFD_EVENT_P2P_RECV_PROV_RSP,            //RT_P2P_RECV_PROV_RSP
    WFD_EVENT_P2P_RECV_GO_NEGO_REQ,         //RT_P2P_RECV_GO_NEGO_REQ
    WFD_EVENT_P2P_CONNECT_FAIL,             //RT_P2P_CONNECT_FAIL
    WFD_EVENT_WPS_COMPLETED,                // WPS_COMPLETED
    WFD_EVENT_P2P_CONNECTED,                // RT_P2P_AP_STA_CONNECTED
    WFD_EVENT_P2P_GET_IPADDR,               // GET_IPADDR
    WFD_EVENT_P2P_GET_IPADDR_DONE,          // GET_IPADDR

    WFD_EVENT_RTSP_PAUSE,
    WFD_EVENT_RTSP_PLAY,
    WFD_EVENT_P2P_DEV_PLUGOUT //detect p2p plug-out
} wfd_status_t;

typedef enum _E_P2P_STATE {
	P2P_STATE_NONE = 0,                     // P2P disable
	P2P_STATE_IDLE = 1,                     // P2P had enabled and do nothing
	P2P_STATE_LISTEN = 2,                   // In pure listen state
	P2P_STATE_SCAN = 3,                     // In scan phase
	P2P_STATE_FIND_PHASE_LISTEN = 4,        // In the listen state of find phase
	P2P_STATE_FIND_PHASE_SEARCH = 5,        // In the search state of find phase
	P2P_STATE_TX_PROVISION_DIS_REQ = 6,     // In P2P provisioning discovery
	P2P_STATE_RX_PROVISION_DIS_RSP = 7,
	P2P_STATE_RX_PROVISION_DIS_REQ = 8,
	P2P_STATE_GONEGO_ING = 9,               // Doing the group owner negoitation handshake
	P2P_STATE_GONEGO_OK = 10,               // finish the group negoitation handshake with success
	P2P_STATE_GONEGO_FAIL = 11,             // finish the group negoitation handshake with failure
	P2P_STATE_RECV_INVITE_REQ_MATCH = 12,   // receiving the P2P Inviation request and match with the profile.
	P2P_STATE_PROVISIONING_ING = 13,        // Doing the P2P WPS
	P2P_STATE_PROVISIONING_DONE = 14,       // Finish the P2P WPS
	P2P_STATE_TX_INVITE_REQ = 15,           // Transmit the P2P Invitation request
	P2P_STATE_RX_INVITE_RESP_OK = 16,       // Receiving the P2P Invitation response
	P2P_STATE_RECV_INVITE_REQ_DISMATCH = 17, //receiving the P2P Inviation request and dismatch with the profile.
	P2P_STATE_RECV_INVITE_REQ_GO = 18,      // receiving the P2P Inviation request and this wifi is GO.
	P2P_STATE_RECV_INVITE_REQ_JOIN = 19,    // receiving the P2P Inviation request to join an existing P2P Group.
	P2P_STATE_RX_INVITE_RESP_FAIL = 20,     // recveing the P2P Inviation response with failure
	P2P_STATE_RX_INFOR_NOREADY = 21,        // receiving p2p negoitation response with information is not available
	P2P_STATE_TX_INFOR_NOREADY = 22,        // sending p2p negoitation response with information is not available
} E_P2P_STATE;

typedef enum
{
    WFD_720P30,
    WFD_720P60,
    WFD_1080P30,
    WFD_1080P60,
    WFD_480P60,
    WFD_VESA_1400 = 0x100,
} wfd_resolution_t;

typedef enum
{
    WFD_CMD_SET_AV_FUNC = 1,
    WFD_CMD_ENABLE_AAC,
    WFD_CMD_SET_DSC_FUNC,
    WFD_CMD_SET_MANAGE_FUNC,
    WFD_CMD_DISABLE_AUDIO,
} wfd_cmd_e;

typedef struct wfd_configuration
{
    char    edid_file[64];
    char    sta_interface_name[64];
    char    p2p_interface_name[64];
    char    rtp_port[16];
    char    hdcp2_port[16];
    char    wfd_is_go[16];
    char    p2p_go_intent[16];
    char    wps_config_method[16];
    char    listen_channel[16];
    char    operate_channel[16];
    char    ssid[64];
    char    psk[64];
    char    hdmi_dongle_mode[16];
} wfd_config_t;

typedef struct
{
    int (*_video_open)();
    void (*_video_close)();
    int (*_video_feed)(unsigned char *data, unsigned int len,
                       unsigned int pts, int last_slice,
                       unsigned int width, unsigned int height);
    int (*_audio_open)(int codec_tag);
    void (*_audio_close)();
    int (*_audio_feed)(int type, unsigned char *buf, int length, unsigned int pts);
    void (*_av_state)(char *s);
    void (*_av_reset)();
} miracast_av_func_t;

typedef struct
{
	void* (*dsc_aes_ctr_open)();
	void* (*dsc_aes_cbc_open)();
	void (*dsc_ctx_destroy)(void*ctx);
	int (*dsc_aes_decrypt)(void* ctx, unsigned char *key, unsigned char* iv, unsigned char *input, unsigned char *output, int size);
	int (*dsc_aes_encrypt)(void* ctx, unsigned char *key, unsigned char* iv, unsigned char *input, unsigned char *output, int size);
} miracast_dsc_func_t;

// audio codec
#ifndef CODEC_ID_AAC
#define CODEC_ID_AAC 0x15002
#endif
#ifndef CODEC_ID_PCM_S16BE
#define CODEC_ID_PCM_S16BE 0x10001
#endif

typedef unsigned int (*wfd_fn)();
typedef unsigned int (*wfd_fn_event)(const wfd_status_t event, const void* data);

typedef struct _wfd_manage_func_t {
    wfd_fn _p2p_device_init;
    wfd_fn _p2p_device_set_enable;
    wfd_fn _p2p_device_get_enable;
    wfd_fn _p2p_device_get_ip;
    wfd_fn _p2p_device_get_rtsp_port;
    wfd_fn_event _p2p_device_event;
} wfd_manage_func_t;

/**
 * It start the miracast module function
 *
 *
 * @return The return value 0: success; -1: failed.
 */
int miracast_start();

/**
 * It stops the Miracast session
 * 
 * @return The return value 0: success; -1: failed.
 */
int miracast_stop();

/**
 * It's a function reg a av player callback
 *
 * @param cmd WFD_CMD_SET_AV_FUNC WFD_CMD_ENABLE_AAC
 * @param para1 the pointer to the para
 * @param para2
 */
void miracast_ioctl(wfd_cmd_e cmd, unsigned long para1, unsigned long para2);

int miracast_update_keyset(void* data);
void miracast_disconnect();
int miracast_is_player_started();
void miracast_enable_p2p(int stat, int channel);

void miracast_set_resolution(wfd_resolution_t res);

wfd_resolution_t miracast_get_resolution();

void miracast_update_p2p_status(E_P2P_STATE status);

/**
 * It returns the miracast version.
 * 
 * @return The version string of miracast
 */
char* miracast_get_version();

/**
 *  set miracast log leve
 * 
 * @param level, default: LL_NOTICE.
 */
int miracast_set_log_leve(int level);

void miracast_player_show_state();

#endif
