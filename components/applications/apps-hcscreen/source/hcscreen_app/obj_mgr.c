#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include "lvgl/lvgl.h"
#include "com_api.h"
#include "key.h"

//#include "typedef.h"

#include "obj_mgr.h"

typedef struct{
	struct list_head node;
	obj_link_t	link;
}obj_des_t;

typedef struct{
	struct list_head head;
	char focus_id;
	pthread_mutex_t api_lock;
}obj_mgr_handle_t;


static int obj_mgr_clear_link(void *handle);
static int obj_mgr_clear_obj(obj_link_t *link);
/**
 * open an object manage
 * @return object manage handle
 */
void *obj_mgr_open()
{
	obj_mgr_handle_t *handle = NULL;
	handle = malloc(sizeof(obj_mgr_handle_t));
	pthread_mutex_init(&handle->api_lock, NULL);

	INIT_LIST_HEAD(&handle->head);
	return (void*)handle;
}

/**
 * close an object manage
 * @param  handle : the object handle to be closed
 * @return        API_SUCCESS/API_FAILURE
 */
int obj_mgr_close(void *handle)
{
	if (!handle)
		return API_FAILURE;

	obj_mgr_handle_t *hld = (obj_mgr_handle_t *)handle;
	pthread_mutex_lock(&hld->api_lock);

	obj_mgr_clear_link(hld);

	free(handle);
	pthread_mutex_unlock(&hld->api_lock);
	pthread_mutex_destroy(&hld->api_lock);

	return API_SUCCESS;
}

/**
 * add the object link to the handle
 * @param  handle : the object handle to be added, the handle is get by obj_mgr_open()
 *
 * @param  link   : the adding objeck link
 * @return        AP_SUCCESS/AP_FAILURE
 */
int obj_mgr_add_link(void *handle, obj_link_t *link)
{
	if (!handle)
		return API_FAILURE;

	obj_mgr_handle_t *hld = (obj_mgr_handle_t *)handle;
	pthread_mutex_lock(&hld->api_lock);

	obj_des_t *obj_des = NULL;
    list_for_each_entry (obj_des, &hld->head, node) {
    	//already add to obj handle.
    	if (obj_des->link.obj == link->obj){
    		pthread_mutex_unlock(&hld->api_lock);
    		return API_SUCCESS;
    	}
    }

	obj_des = malloc(sizeof(obj_des_t));
	//do not copy node.
	obj_des->link.obj = link->obj;
	obj_des->link.obj_type = link->obj_type;
	obj_des->link.id = link->id;
	obj_des->link.up_id = link->up_id;
	obj_des->link.down_id = link->down_id;
	obj_des->link.left_id = link->left_id;
	obj_des->link.right_id = link->right_id;
	obj_des->link.show = link->show;
	obj_des->link.high = link->high;
	obj_des->link.grey = link->grey;
	obj_des->link.select = link->select;

	INIT_LIST_HEAD(&obj_des->link.head);

	list_add_tail(&obj_des->node, &hld->head);
	pthread_mutex_unlock(&hld->api_lock);

	return API_SUCCESS;
}

int obj_mgr_add_obj(obj_link_t *link, obj_style_t *obj_style)
{
	if (!link || !obj_style){
		return API_FAILURE;
	}
	obj_style_t *style = NULL;
    list_for_each_entry (style, &link->head, node) {
    	//already add to link.
    	if (style->obj == obj_style->obj)
    		return API_SUCCESS;
    }

	style = malloc(sizeof(obj_style_t));
	style->obj = obj_style->obj;
	style->obj_type = obj_style->obj_type;
	style->show = obj_style->show;
	style->high = obj_style->high;
	style->grey = obj_style->grey;
	style->select = obj_style->select;

	list_add_tail(&style->node, &link->head);

	return API_SUCCESS;
}


static int obj_mgr_clear_obj(obj_link_t *link)
{
	if (!link)
		return API_FAILURE;

	obj_style_t *style = NULL;
	obj_style_t *style_tmp = NULL;
    list_for_each_entry_safe (style, style_tmp, &link->head, node) {
		list_del(&style->node);
		free(style);
    }
	return API_SUCCESS;
}


/**
 * Get the object link by current object
 * @param  handle :the handle is get by obj_mgr_open()
 * @param  obj    : the current object
 * @return        : the object link
 */
obj_link_t *obj_mgr_get_link(void *handle, void *obj)
{
	obj_des_t *obj_des = NULL;
	obj_link_t *link = NULL;
	if (!handle || !obj)
		return NULL;

	obj_mgr_handle_t *hld = (obj_mgr_handle_t *)handle;
	pthread_mutex_lock(&hld->api_lock);	

	obj_des = NULL;
    list_for_each_entry (obj_des, &hld->head, node) {
    	if (obj_des->link.obj == obj){
    		link = &obj_des->link;
    		break;
    	}
	}
	pthread_mutex_unlock(&hld->api_lock);

	return link;
}

/**
 * Get the object by id
 * @param  handle :the handle is get by obj_mgr_open()
 * @param  obj    : the current object
 * @return        : the object link
 */
void *obj_mgr_get_obj_by_id(void *handle, char id)
{
	obj_des_t *obj_des = NULL;
	void *obj = NULL;
	if (!handle)
		return NULL;

	obj_mgr_handle_t *hld = (obj_mgr_handle_t *)handle;
	pthread_mutex_lock(&hld->api_lock);

	obj_des = NULL;
    list_for_each_entry (obj_des, &hld->head, node) {
    	if (obj_des->link.id == id){
    		obj = obj_des->link.obj;
    		break;
    	}
	}
	pthread_mutex_unlock(&hld->api_lock);

	return obj;
}


/**
 * Remove the object link from the object handle by the object
 * @param  handle : the handle is get by obj_mgr_open()
 * @param  obj    : the object to be removed
 * @return        AP_SUCCESS/AP_FAILURE
 */
int obj_mgr_clear_link(void *handle)
{
	if (!handle)
		return API_FAILURE;

	obj_mgr_handle_t *hld = (obj_mgr_handle_t *)handle;

	obj_des_t *obj_des = NULL;
	obj_des_t *obj_tmp = NULL;
    list_for_each_entry_safe (obj_des, obj_tmp, &hld->head, node) {
    	obj_mgr_clear_obj(&obj_des->link);
		list_del(&obj_des->node);
		free(obj_des);
    }

	return API_SUCCESS;
}

/**
 * Get the around object by the key(UP/DOWN/LEFT/RIGHT)
 * @param  handle :the handle is get by obj_mgr_open()
 * @param  obj    : the current object
 * @param  key    : the keypad send the key(LV_KEY_UP/LV_KEY_DOWN/LV_KEY_LEFT/LV_KEY_RIGHT)
 * @return        : the around object of the main object
 */
void *obj_mgr_get_obj_by_key(void *handle, void *obj, uint8_t key)
{
	char next_id = 0;
	void *n_obj = NULL;

	obj_des_t *obj_des = NULL;
	if (!handle || !obj)
		return NULL;

	if (!obj_is_valid_key(key)){
		printf("%s(), not support the key:%d\n", __FUNCTION__, key);
		return NULL;
	}

	obj_mgr_handle_t *hld = (obj_mgr_handle_t *)handle;
	pthread_mutex_lock(&hld->api_lock);

	obj_des = NULL;
    list_for_each_entry (obj_des, &hld->head, node) {
    	if (obj_des->link.obj == obj){
    		if (LV_KEY_LEFT == key){
    			next_id = obj_des->link.left_id;
    			break;
    		}
    		if (LV_KEY_RIGHT == key){
    			next_id =  obj_des->link.right_id;
    			break;
    		}
    		if (LV_KEY_UP == key){
    			next_id = obj_des->link.up_id;
    			break;
    		}
    		if (LV_KEY_DOWN == key){
    			next_id = obj_des->link.down_id;
    			break;
    		}
    	}
    }
    if (0 == next_id){
    	printf("%s(), no object found!\n", __FUNCTION__);
    	pthread_mutex_unlock(&hld->api_lock);
    	return NULL;
    }

	obj_des = NULL;
    list_for_each_entry (obj_des, &hld->head, node) {
    	if (obj_des->link.id == next_id){
    		n_obj = obj_des->link.obj;
    		break;
    	}
    }
	pthread_mutex_unlock(&hld->api_lock);

    return n_obj;
}

static void obj_draw_by_type(void *obj, obj_lv_type_t obj_type, void *style)
{
	if (!obj || !style)
		return;

    if (OBJ_LV_TYPE_IMG == obj_type){
	    lv_img_set_src((lv_obj_t*)obj, (const char*)style);
	} else if (OBJ_LV_TYPE_LABEL == obj_type){
		lv_obj_add_style((lv_obj_t*)obj, (lv_style_t*)style, 0);
	} else if (OBJ_LV_TYPE_BTN == obj_type){

	}

}

/**
 * Draw che current object to nomal image(clear focus), draw next object to highlight image(focus on)
 * @param  handle :the handle is get by obj_mgr_open()
 * @param  obj    : The current object
 * @param  key    : the keypad send the key(LV_KEY_UP/LV_KEY_DOWN/LV_KEY_LEFT/LV_KEY_RIGHT)
 * @return        : the next object 
 */
void *obj_mgr_draw_obj_by_key(void *handle, void *obj, uint8_t key)
{
	char cur_id = 0;
	char next_id = 0;
	obj_link_t *cur_link = NULL;
	obj_link_t *next_link = NULL;
	obj_des_t *obj_des = NULL;
	obj_style_t *style = NULL;
	void *next_obj = NULL;

	if (!handle || !obj)
		return NULL;

	if (!obj_is_valid_key(key)) {
		printf("%s(), not support the key:%d\n", __FUNCTION__, key);
		return NULL;
	}

	obj_mgr_handle_t *hld = (obj_mgr_handle_t *)handle;
	pthread_mutex_lock(&hld->api_lock);


	//step 1: find the id of next object by the key
	obj_des = NULL;
    list_for_each_entry (obj_des, &hld->head, node) {
    	if (obj_des->link.obj == obj){
    		cur_id = obj_des->link.id;
    		if (0 == cur_id)
    			break;

    		cur_link = &obj_des->link;
    		if (V_KEY_LEFT == key){
    			//no need draw
    			if (0 == obj_des->link.left_id || obj_des->link.left_id == cur_id)
    				break;
    			next_id = obj_des->link.left_id;
    			break;
    		}
    		if (V_KEY_RIGHT == key){
    			if (0 == obj_des->link.right_id || obj_des->link.right_id == cur_id)
    				break;
    			next_id =  obj_des->link.right_id;
    			break;
    		}
    		if (V_KEY_UP == key){
    			if (0 == obj_des->link.up_id || obj_des->link.up_id == cur_id)
    				break;
    			next_id = obj_des->link.up_id;
    			break;
    		}
    		if (V_KEY_DOWN == key){
    			if (0 == obj_des->link.down_id || obj_des->link.down_id == cur_id)
    				break;
    			next_id = obj_des->link.down_id;
    			break;
    		}
    	}
    }
    do{
	    if (0 == next_id)
	    	break;

	    //step2: get the next object by the id.
		obj_des = NULL;
	    list_for_each_entry (obj_des, &hld->head, node) {
	    	if (obj_des->link.id == next_id){
			    //step 2: draw
	    		next_link = &obj_des->link;
	    		break;
	    	}
	    }
	    if (NULL == next_link)
	    	break;

	    next_obj = next_link->obj;

	    //step 3: draw current object with normal image(clear focus)
	    obj_draw_by_type(obj, cur_link->obj_type, cur_link->show);
	    style = NULL;
	    list_for_each_entry (style, &cur_link->head, node) {
		   obj_draw_by_type(style->obj, style->obj_type, style->show);
	    }

		//step 4: draw next object with highlight image
		obj_draw_by_type(next_link->obj, next_link->obj_type, next_link->high);
		style = NULL;
	    list_for_each_entry (style, &next_link->head, node) {
	    	obj_draw_by_type(style->obj, style->obj_type, style->high);
		}
	}while(0);

	pthread_mutex_unlock(&hld->api_lock);

    //step 5: return next object
    return next_obj;
}

/**
 * Draw the object by id
 * @param  handle : the handle is get by obj_mgr_open()
 * @param  id     : the object id
 * @param  type   : true: draw the normal/highlight/grey/select image
 * @return        [description]
 */
int obj_mgr_draw_link_by_id(void *handle, char id, obj_draw_type_t type)
{
	obj_link_t *cur_link = NULL;
	obj_des_t *obj_des = NULL;

	if (!handle || !id)
		return API_FAILURE;

	obj_mgr_handle_t *hld = (obj_mgr_handle_t *)handle;

	pthread_mutex_lock(&hld->api_lock);

	obj_des = NULL;
    list_for_each_entry (obj_des, &hld->head, node) {
    	if (obj_des->link.id == id){
    		cur_link = &obj_des->link;
    		break;
    	}
    }

	if (NULL == cur_link){
		printf("No link!\n");
		pthread_mutex_unlock(&hld->api_lock);
		return API_FAILURE;
	}

	if (type == OBJ_DRAW_SHOW && cur_link->show)
		obj_draw_by_type(cur_link->obj, cur_link->obj_type, cur_link->show);
	if (type == OBJ_DRAW_HIGH && cur_link->high)
		obj_draw_by_type(cur_link->obj, cur_link->obj_type, cur_link->high);
	if (type == OBJ_DRAW_GREY && cur_link->grey)
		obj_draw_by_type(cur_link->obj, cur_link->obj_type, cur_link->grey);
	if (type == OBJ_DRAW_SELECT && cur_link->select)
		obj_draw_by_type(cur_link->obj, cur_link->obj_type, cur_link->select);

    obj_style_t *style = NULL;
    list_for_each_entry (style, &cur_link->head, node) {
		if (type == OBJ_DRAW_SHOW && cur_link->show)
			obj_draw_by_type(style->obj, style->obj_type, style->show);
		if (type == OBJ_DRAW_HIGH && cur_link->high)
			obj_draw_by_type(style->obj, style->obj_type, style->high);
		if (type == OBJ_DRAW_GREY && cur_link->grey)
			obj_draw_by_type(style->obj, style->obj_type, style->grey);
		if (type == OBJ_DRAW_SELECT && cur_link->select)
			obj_draw_by_type(style->obj, style->obj_type, style->select);
    }

	pthread_mutex_unlock(&hld->api_lock);

	return API_SUCCESS;
}