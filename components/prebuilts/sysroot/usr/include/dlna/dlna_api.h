#ifndef _DLNA_API_H
#define _DLNA_API_H

typedef enum
{
    DMA_STATUS_STOPPED = 0,
    DMA_STATUS_PLAYING = 1,
    DMA_STATUS_PAUSED = 2,
    DMA_STATUS_ERROR = 3,
    DMA_STATUS_BUFFERING = 64,
    DMA_STATUS_INVALID = -1
} DmaStatus_E;

struct output_module
{
    const char *shortname;
    const char *description;
    //int (*add_options)(GOptionContext *ctx);

    // Commands.
    int (*init)(void);
    void (*deinit)(void);
    void (*set_uri)(const char *uri, void* meta_info, const char *meta);
    void (*set_next_uri)(const char *uri, const char *meta);
    int (*play)(void* transition_callback);
    int (*stop)(void);
    int (*pause)(void);
    int (*loop)(void);
    int (*seek)(uint64_t position_nanos);

    // parameters
    int (*get_position)(uint64_t *track_duration, uint64_t *track_pos);
    int (*get_volume)(int *);
    int (*set_volume)(int);
    int (*get_mute)(int *);
    int (*set_mute)(int);
    int (*get_status)(int *);
};

struct dlna_svr_param {
    struct output_module *output;   // media output cb, required
    const char *ifname;             // upnp/dlna svr listen ifname, default "wlan0", option
    const char *svrname;            // upnp/dlna svr name, required
    int svrport;                    // upnp/dlna svr listen port, default 49494, option
} ;

/****************************************************************************************************/

/**
 * It initializes the output module, initializes the UPnP device, initializes the UPnP transport and
 * control services, and returns
 * 
 * @param service_name The name of the service.
 * @param media_output_module This is the output module that will be used to play the media.
 * 
 * @return The return value is the result of the function.
 */
int dlna_service_start(char* service_name, struct output_module * media_output_module);

/**
 * It stops the DLNA server
 */
void dlna_sevice_stop(void);

/**
 * > This function is used to register the mime type supported by the DLNA server
 * 
 * @param mime_num the number of mime types you want to support
 * @param mime_support_list the list of mime types that you want to support.
 */
void dlna_init_mime_list(int mime_num, char mime_support_list[][64]);

/**
 * > This function is used to set the device description
 * 
 * @param manufacturer The manufacturer of the device.
 * @param manufacturer_url The URL of the manufacturer of the device.
 * @param model_name The name of the device.
 */
void dlna_setDescriptor(unsigned char *manufacturer, unsigned char *manufacturer_url, unsigned char *model_name);

/**
 * > Set the log level for the DLNA library
 * 
 * @param level The log level to set.  This is a bitmask of the following values:
 * 
 * @return The return value is the log level.
 */
int dlna_set_log_level(int level);

/**
 * It initializes the UPnP device and starts the UPnP server
 * 
 * @param param the parameter structure of the DLNA server
 * 
 * @return The return value is the result of the function.
 */
int dlna_service_start_ex(struct dlna_svr_param* param);

/**
 * It get the dlna lib build version string
 * 
 * @return The return value is tbuild version string.
 */
char* dlna_get_version();

#endif
