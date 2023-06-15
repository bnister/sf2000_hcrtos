#ifndef __OBJ_MGR_H__
#define __OBJ_MGR_H__

#include "list.h"
#ifdef __cplusplus
extern "C" {
#endif


typedef enum{
	OBJ_LV_TYPE_IMG = 0,
	OBJ_LV_TYPE_LABEL,
	OBJ_LV_TYPE_BTN ,
	OBJ_LV_TYPE_LIST,
}obj_lv_type_t;


typedef struct{
	struct list_head node;
	void *obj;
	obj_lv_type_t obj_type;

    void *show;   //No focus on the object(nomal), show the image/style resource
    void *high; //Focus on the object(highlight), show the image/style resource
    void *grey;   // The object is disabled, show the image/style resource
    void *select; //The object is selcted, but focus on other object, show the image/style resource
}obj_style_t;

typedef struct{
	struct list_head head;
	void *obj;
	obj_lv_type_t obj_type;

	char id;	//the ID of the object, base on 1.
	char up_id;	 // the ID of the up object, up key can navigate to it
	char down_id; // the ID of the down object, down key can navigate to it
	char left_id; // the ID of the left object, left key can navigate to it
	char right_id; // the ID of the right object, right key can navigate to it

	void *show;   //No focus on the object(nomal), show the image/style resource
	void *high; //Focus on the object(highlight), show the image/style resource
	void *grey;   // The object is disabled, show the image/style resource
	void *select; //The object is selcted, but focus on other object, show the image/style resource
}obj_link_t;

typedef enum{
	OBJ_DRAW_SHOW = 0,
	OBJ_DRAW_HIGH,
	OBJ_DRAW_GREY,
	OBJ_DRAW_SELECT,
}obj_draw_type_t;

#define obj_is_valid_key(key) ((key==V_KEY_UP)||(key==V_KEY_DOWN)||(key==V_KEY_LEFT)||(key==V_KEY_RIGHT))

void *obj_mgr_open(void);
int obj_mgr_close(void *handle);
int obj_mgr_add_link(void *handle, obj_link_t *link);
int obj_mgr_add_obj(obj_link_t *link, obj_style_t *obj_style);
obj_link_t *obj_mgr_get_link(void *handle, void *obj);
int obj_mgr_remove_link(void *handle, void *obj);
void *obj_mgr_get_obj_by_key(void *handle, void *obj, uint8_t key);
void *obj_mgr_draw_obj_by_key(void *handle, void *obj, uint8_t key);
int obj_mgr_draw_link_by_id(void *handle, char id, obj_draw_type_t type);
void *obj_mgr_get_obj_by_id(void *handle, char id);

#ifdef __cplusplus
} /*extern "C"*/
#endif


#endif