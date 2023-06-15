#ifndef _YOUTUBE_API_H_
#define _YOUTUBE_API_H_

#include <stdbool.h>
#include <stdint.h>
#include <cjson/cJSON.h>

typedef enum {
    YOUTUBE_RET_STREAM_LIMIT            =  2,
    YOUTUBE_RET_STREAM_CIPHER           =  1,
	YOUTUBE_RET_NO_ERROR                =  0,
	YOUTUBE_RET_MEM                     = -1,
    YOUTUBE_RET_GET_DATA_FAILED         = -2,
    YOUTUBE_RET_POST_DATA_FAILED        = -3,
	YOUTUBE_RET_PARAMS_ERROR            = -4,
	YOUTUBE_RET_USER_ABORT_HANDLE       = -5,

	YOUTUBE_RET_JSON_PARSE_ERROR        = -10,

    YOUTUBE_RET_PCRE_COMPILE_ERROR      = -20,
	YOUTUBE_RET_PCRE_EXEC_ERROR         = -21,
	YOUTUBE_RET_PCRE_NOMATCH            = -22,

    YOUTUBE_RET_DECRYPT_PARAM_ERROR     = -30,

	YOUTUBE_RET_RESPONES_ERRORS         = -400,
} YOUTUBE_RET_E;

typedef enum {
	YOUTUBE_SEARCH_TYPE_VIDEO           = 0x1,
	YOUTUBE_SEARCH_TYPE_CHANNEL         = 0x2,
	YOUTUBE_SEARCH_TYPE_PLAYLIST        = 0x4,
	YOUTUBE_SEARCH_TYPE_ALL             = 0xF,

    YOUTUBE_VIDEO_TYPE_ALL              = 0x1F,
} youtube_option_type_e;

typedef enum _youtube_ioctl_cmd_ {
    YOUTUBE_CMD_SET_BUF_SIZE,
    YOUTUBE_CMD_GET_BUF_SIZE,
    YOUTUBE_CMD_ABORT_HANDLE,
    YOUTUBE_CMD_SET_LOG_LEVEL,
    YOUTUBE_CMD_GET_LOG_LEVEL,
    YOUTUBE_CMD_RESET_CIPHER_DECODE,
}youtube_ioctl_cmd_e;

typedef struct _youtube_video_info_st_
{
	char id[32];
    unsigned option;
    char res[4];
    char ext[0];
} youtube_video_info_st;

typedef struct youtube_category_list_st_
{
    char *part;
    char *region_code;
    char *key;
    char res[5];
} youtube_category_list_st;

typedef struct youtube_video_list_st_
{
    char *part;
    char *region_code;
    char *page_token;
    char *category_id;
    char *max_results;
    char *key;
    char res[4];
} youtube_video_list_st;

typedef struct youtube_search_list_st_ {
    char *key_word;
    char *part;
    char *region_code;
    char *page_token;
    char *category_id;
    char *max_results;
    char *key;
    unsigned option;
    char res[4];
} youtube_search_list_st;

/**
 * It gets the category list from youtube.
 * 
 * @param in a pointer to a structure that contains the parameters for the API call.
 * @param out The output of the results json object.
 * out.items: results list; out.items.id: category id; out.items.snippet: category info;
 * out.items.snippet.title: Category name;
 * 
 * @return YOUTUBE_RET_E.
 */
YOUTUBE_RET_E youtube_get_category_list(youtube_category_list_st *in, cJSON **out);

/**
 * It takes a struct as input, makes a call to the YouTube API, and returns a cJSON object
 * 
 * @param in the input parameters
 * @param out The output JSON object.
 * out.items: results list; out.items.id: video id, play video key info;
 * out.items.snippet: video info; out.items.snippet.title: video title;
 * out.items.snippet.description: video context description;
 * out.items.snippet.thumbnails: video preview thumbnails info.
 * out.nextPageToken: next page key info;
 * out.prevPageToken: prev page key info;
 * out.pageInfo.totalResults: total results;
 * out.pageInfo.resultsPerPage: current return results num per page.
 * 
 * @return YOUTUBE_RET_E.
 */
YOUTUBE_RET_E youtube_get_video_list(youtube_video_list_st *in, cJSON **out);

/**
 * It gets the search list from youtube.
 * 
 * @param in a pointer to a structure that contains the parameters for the search.
 * @param out The output JSON object.
 * out.items: results list; out.items.id: video id, play video key info;
 * out.items.snippet: video info; out.items.snippet.title: video title;
 * out.items.snippet.description: video context description;
 * out.items.snippet.thumbnails: video preview thumbnails info;
 * out.nextPageToken: next page key info;
 * out.prevPageToken: prev page key info;
 * out.pageInfo.totalResults: total results;
 * out.pageInfo.resultsPerPage: current return results num per page.
 * 
 * @return YOUTUBE_RET_E.
 */
YOUTUBE_RET_E youtube_get_search_list(youtube_search_list_st *in, cJSON **out);

/**
 * It downloads the video page, parses it, and then downloads the javascript file that contains the
 * decryption code
 * 
 * @param in the video info
 * @param out the output parameter, which is a JSON object.
 * out.streamingData.expiresInSeconds: links expires time;
 * out.streamingData.formats/adaptiveFormats: normal video info/HLS video info;
 * out.streamingData.formats/adaptiveFormats.url: video link (normal);
 * out.streamingData.formats/adaptiveFormats.signatureCipher: video links (cipher); 
 * out.streamingData.formats/adaptiveFormats.mimeType: video mimetype(include video/audio);
 * out.streamingData.formats/adaptiveFormats.fps: video fps;
 * out.streamingData.formats/adaptiveFormats.qualityLabel: video quality label;
 * out.streamingData.formats/adaptiveFormats.width/height: video width/height.
 * 
 * @return YOUTUBE_RET_E.
 */
YOUTUBE_RET_E youtube_get_video_links(youtube_video_info_st *in, cJSON **out);

/**
 * It returns a string the version number of the youtube library.
 * 
 * @return The version of the String.
 */
char *youtube_get_version();

/**
 * this function conf lib feature
 * 
 * @param cmd The command to be executed.
 * @param in the cmds with data in
 * @param out the cmds with data out
 */
YOUTUBE_RET_E youtube_ioctl(int cmd, void *in, void *out);

/**
 * > youtube_set_log_level() sets the log level for the youtube library
 * 
 * @param level The log level.
 * 
 * @return The return value is an enumeration of the type YOUTUBE_RET_E.
 */
YOUTUBE_RET_E youtube_set_log_level(int level);

/**
 * Youtube_get_log_level() returns the current log level
 * 
 * @return The return value is the current log level.
 */
YOUTUBE_RET_E youtube_get_log_level();

bool youtube_handle_get_abort_flag();
bool youtube_handle_set_abort_flag();
bool youtube_key_set_update_flag();
bool youtube_js_set_update_flag();

#endif
