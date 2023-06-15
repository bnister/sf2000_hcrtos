#ifndef __HCCAST_IPTV__
#define __HCCAST_IPTV__

#include "list.h"

#define HCCAST_IPTV_APP_YT      (0)
#define HCCAST_IPTV_APP_YP      (1)
#define HCCAST_IPTV_APP_MAX     (10)

#define HCCAST_IPTV_THUMB_MAX   (5)

#define BIT(nr) (1UL << (nr))

// iptv events
typedef enum
{
    HCCAST_IPTV_EVT_TIMEOUT,                    //action timeout
    HCCAST_IPTV_EVT_NETWORK_ERR,                //network error
    HCCAST_IPTV_EVT_NOT_SUPPORT,                //not support action
    HCCAST_IPTV_EVT_CATEGORY_FETCH_ERR = 1,    //cannot fetch category
    HCCAST_IPTV_EVT_CATEGORY_FETCH_OK,         //fetch category success
    HCCAST_IPTV_EVT_CATEGORY_OPEN_ERR,          //cannot open category
    HCCAST_IPTV_EVT_CATEGORY_OPEN_OK,           //open category success
    HCCAST_IPTV_EVT_VIDEO_NOT_EXIST,            //no video link detected
    HCCAST_IPTV_EVT_VIDEO_FETCH_OK,            //get video link done
    HCCAST_IPTV_EVT_VIDEO_FETCH_ERR,           //cannot get the video link
    HCCAST_IPTV_EVT_NEXT_PAGE_ERR,              //cannot goto next page
    HCCAST_IPTV_EVT_NEXT_PAGE_OK,               //goto next page done
    HCCAST_IPTV_EVT_NO_PAGES,
    HCCAST_IPTV_EVT_SEARCH_ERR,
    HCCAST_IPTV_EVT_SEARCH_OK,
} hccast_iptv_evt_e;

// iptv source list order type
typedef enum
{
    HCCAST_IPTV_ORDER_DATE = 1,
    HCCAST_IPTV_ORDER_RATING,
    HCCAST_IPTV_ORDER_TITLE,
    HCCAST_IPTV_ORDER_RELEVANCE,
} hccast_iptv_list_order_e;

typedef enum
{
    HCCAST_IPTV_VIDEO_144P      = BIT(0),
    HCCAST_IPTV_VIDEO_240P      = BIT(1),
    HCCAST_IPTV_VIDEO_360P      = BIT(2),
    HCCAST_IPTV_VIDEO_480P      = BIT(3),
    HCCAST_IPTV_VIDEO_720P      = BIT(4),
    HCCAST_IPTV_VIDEO_1080P     = BIT(5),
    HCCAST_IPTV_VIDEO_1440P     = BIT(6),   // unsupported
    HCCAST_IPTV_VIDEO_2160P     = BIT(7),   // unsupported

    HCCAST_IPTV_VIDEO_720P60    = BIT(16),  // unsupported
    HCCAST_IPTV_VIDEO_1080P60   = BIT(17),  // unsupported
    HCCAST_IPTV_VIDEO_1440P60   = BIT(18),  // unsupported
    HCCAST_IPTV_VIDEO_2160P60   = BIT(19),  // unsupported
} hccast_iptv_video_quality_e;

typedef enum {
	HCCAST_IPTV_CODECS_NONE     = 0,
	HCCAST_IPTV_CODECS_AVC      = BIT(1),
	HCCAST_IPTV_CODECS_VP9      = BIT(2),   // unsupported
	HCCAST_IPTV_CODECS_AV1      = BIT(3),   // unsupported
	HCCAST_IPTV_CODECS_MPEG4    = BIT(4),   // mp4a.40.2: means audio codecs AAC LC (Low Complexity) 34。
	HCCAST_IPTV_CODECS_OPUS     = BIT(5),
} hccast_iptv_video_mimeType_e;

typedef struct
{
    char region[16];
    char api_key[64];

    char api_secret[64];
    char page_num;
    char page_max;
    char timeout;
} hccast_iptv_app_config_st;

typedef struct
{
    int  category_id;
    int  list_max_num;
    char cid[64];           //channel id
    char page_token[16];    //page token
    char region[16];
    char lang[16];
    hccast_iptv_list_order_e  order_type;
} hccast_iptv_search_param_st;

typedef struct
{
    void  *priv;
    char   page_token[16];
    char   video_count;
} hccast_iptv_page_st;

typedef struct
{
} hccast_iptv_video_st;

typedef struct {
    char id[32];                            // [input] video id
    char cate_id[16];                       // [input] category id
    char page_token[16];                    // page token
    char *query;
    int query_type;
} hccast_iptv_request_st;

typedef struct {
    struct list_head list;
    char cate_id[16];                       // category id
    char cate_name[32];                     // category name
    char cate_etag[32];                     // category etag
} hccast_iptv_cate_node_st;

typedef struct {
    struct list_head list;
    int cate_num;
    hccast_iptv_cate_node_st cate_node;
} hccast_iptv_category_node_st;

typedef struct {
    char quality[16];                       // quality label
    char *url;                              // url
} hccast_iptv_thumb_node_st;

typedef struct {
    struct list_head list;
    char id[32];
    char quality[16];                       // quality label
    char quality_label[16];                 // quality label
    unsigned quality_option;                // quality option items
    int  itag;                              // reserve
    unsigned bitrate;                       // bitrate (formate)
    unsigned avg_bitrate;                   // average bitrate (hls)
    char *mimeType;                         // mime info
    char *projectionType;                   // reserve
    char *url;                              // url
} hccast_iptv_links_node_st;

typedef struct {
    struct list_head list;
    char id[32];                            // video id
    char ch_id[32];                         // channel id
    char cate_id[16];                       // category id
    char lang[8];                           // audio default lang
    char *title;                            // video title
    char *ch_title;                         // channel title
    char *descr;                            // video description
    char *tags;                             // video tags (unused)
    int  url_parse;                         // video parsse flag
    hccast_iptv_thumb_node_st thumb[HCCAST_IPTV_THUMB_MAX];
    hccast_iptv_links_node_st *links;
} hccast_iptv_info_node_st;

typedef struct {
    struct list_head list;
    char cate_id[16];                       // [input]
    int  info_num;                          // [output], video info per page
    int  info_total_num;                    // [output], video info total num
    char next_page_token[16];               // next page token
    char prev_page_token[16];               // prev page token
    char curr_page_token[16];               // current page token
    hccast_iptv_info_node_st info_node;     // [output], video detail info
} hccast_iptv_list_node_st;

typedef struct {
    char cate_id[16];                       // [input] category total num
    unsigned option;                        // [input] option
    hccast_iptv_list_node_st *list_node;    // [output], category detail info
} hccast_iptv_cate_req_st;

typedef struct {
    char *key_word;                         // [input] key word
    unsigned option;                        // [input] option
    hccast_iptv_list_node_st *list_node;    // [output], category detail info
} hccast_iptv_search_req_st;

typedef struct {
    char id[32];                            // [input] category total num
    unsigned option;                        // [input] option
    char url[2048];                         // [output], category detail info
} hccast_iptv_info_req_st;

typedef struct {
    char id[32];                            // [input] category total num
    unsigned option;                        // [input] option
    char url[2048];                         // [output], category detail info
} hccast_iptv_links_req_st;

typedef void (*hccast_iptv_notifier)(hccast_iptv_evt_e evt, void *arg);

typedef struct
{
    unsigned int initialized;
    char app_title[16];
    int (*init)(hccast_iptv_app_config_st *config, hccast_iptv_notifier notifier);
    int (*deinit)();
    int (*category_get)(hccast_iptv_category_node_st **cate_out);
    int (*category_open)(int index);
    int (*category_count)();
    int (*category_fetch)(hccast_iptv_cate_req_st *req, hccast_iptv_list_node_st **list_out);
    int (*search)(hccast_iptv_search_req_st *req, hccast_iptv_list_node_st **list_out);
    int (*page_get)(hccast_iptv_request_st *req, hccast_iptv_list_node_st **list_out);
    int (*page_next)(int direction);
    int (*info_fetch)(hccast_iptv_info_req_st *req, hccast_iptv_links_node_st **links);
    int (*link_get)(hccast_iptv_links_req_st *req, hccast_iptv_links_node_st **links);
} hccast_iptv_app_instance_st;

#ifdef __cplusplus
extern "C" {
#endif

// hccast iptv api for plugin, implement by hccast. should after service init.
int hccast_iptv_app_register(int app_id, hccast_iptv_app_instance_st *inst);

// hccast iptv api for user initialized, implement by plugin.
// call only once at system initialized stage.
int hccast_iptv_attach_yt();

// hccast iptv api for user initialized, implement by hccast.
// call only once at system initialized stage.
int hccast_iptv_service_init();
void *hccast_iptv_app_open(int app_id);

// hccast iptv api for user app, implement by hccast.
int hccast_iptv_app_init(void *inst, hccast_iptv_app_config_st *config, hccast_iptv_notifier notifier);
int hccast_iptv_app_deinit(void *inst);

/**
 * The function retrieves IPTV categories from an initialized app instance.
 * 
 * @param inst A void pointer to an instance of the hccast_iptv_app_instance_st struct, which contains
 * information about the IPTV application instance.
 * @param cate A pointer to a pointer of hccast_iptv_category_node_st, which is a struct representing a
 * node in a linked list of IPTV categories. This function is used to retrieve the current category
 * node from the IPTV app instance.
 * 
 * @return an integer value. If the function execution is successful, it will return 0. If there is an
 * error, it will return -1.
 */
int hccast_iptv_category_get(void *inst, hccast_iptv_category_node_st **cate);

int hccast_iptv_category_open(void *inst, int index);

int hccast_iptv_category_count(void *inst);

/**
 * This function fetches IPTV categories and returns a list of nodes.
 * 
 * @param inst A void pointer to an instance of the hccast_iptv_app_instance_st struct, which contains
 * information about the IPTV application instance.
 * @param req hccast_iptv_cate_req_st is a structure that contains the request parameters for fetching
 * IPTV categories. The exact contents of this structure are not shown in the code snippet provided.
 * @param list_out A pointer to a pointer of hccast_iptv_list_node_st, which is the output parameter
 * that will contain the fetched IPTV category list.
 * 
 * @return an integer value. The specific value depends on the execution of the function. If the
 * function executes successfully, it will return a value of 0 or a positive integer. If there is an
 * error, it will return a negative integer.
 */
int hccast_iptv_category_fetch(void *inst, hccast_iptv_cate_req_st *req, hccast_iptv_list_node_st **list_out);

int hccast_iptv_search(void *inst, hccast_iptv_search_req_st *req, hccast_iptv_list_node_st **list_out);

int hccast_iptv_page_get(void *inst, hccast_iptv_request_st *req, hccast_iptv_list_node_st **list_out);

/**
 * This function retrieves IPTV links based on a request and returns them through a pointer.
 * 
 * @param inst The "inst" parameter is a void pointer to an instance of a structure
 * "hccast_iptv_app_instance_st".
 * @param req req is a pointer to a structure of type hccast_iptv_links_req_st, which contains the
 * request parameters for retrieving IPTV links.
 * @param links The "links" parameter is a double pointer to a structure of type
 * "hccast_iptv_links_node_st". This function is expected to populate this structure with the IPTV
 * links requested by the "req" parameter. The function returns an integer value indicating success or
 * failure.
 * 
 * @return an integer value, which could be either a success or error code. If the function is
 * successful, it will return a value of 0 or a positive integer. If there is an error, it will return
 * a negative integer.
 */
int hccast_iptv_info_fetch(void *inst, hccast_iptv_info_req_st *req, hccast_iptv_links_node_st **links);

int hccast_iptv_link_get(void *inst, hccast_iptv_links_req_st *req, hccast_iptv_links_node_st **links);

#ifdef __cplusplus
}
#endif

#endif
