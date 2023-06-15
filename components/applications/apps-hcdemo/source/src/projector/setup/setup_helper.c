#include "setup_helper.h"


static lv_ll_t update_list;
static aspect_ratio_update_node *node;


void aspect_ratio_update_init(){
    _lv_ll_init(&update_list, sizeof(aspect_ratio_update_node));
}

void aspect_ratio_update_add_node(lv_obj_t *obj, aspect_ratio_update_func func){
    aspect_ratio_update_node *node1 = _lv_ll_ins_tail(&update_list);
    node1->obj = obj;
    node1->func = func;
}

void aspect_ratio_update_run(){
    _LV_LL_READ(&update_list, node){
        node->func(node->obj);
    }
}

