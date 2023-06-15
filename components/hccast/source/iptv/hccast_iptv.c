#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <hccast_iptv.h>

static hccast_iptv_app_instance_st g_iptv_inst[HCCAST_IPTV_APP_MAX];

int hccast_iptv_service_init()
{
    memset(g_iptv_inst, 0, sizeof(hccast_iptv_app_instance_st) * HCCAST_IPTV_APP_MAX);

    return 0;
}

int hccast_iptv_app_register(int app_id, hccast_iptv_app_instance_st *inst)
{
    if (app_id >= HCCAST_IPTV_APP_MAX || !inst)
    {
        return -1;
    }

    memcpy(&g_iptv_inst[app_id], inst, sizeof(hccast_iptv_app_instance_st));
    g_iptv_inst[app_id].initialized = 1;

    return 0;
}

void *hccast_iptv_app_open(int app_id)
{
    if (app_id >= HCCAST_IPTV_APP_MAX || !g_iptv_inst[app_id].initialized)
    {
        return NULL;
    }

    return (void *)(&g_iptv_inst[app_id]);
}

int hccast_iptv_app_init(void *inst,
                         hccast_iptv_app_config_st *config,
                         hccast_iptv_notifier notifier)
{
    hccast_iptv_app_instance_st *app_inst = (hccast_iptv_app_instance_st *)inst;

    if (!app_inst || !app_inst->initialized || !app_inst->init)
    {
        return -1;
    }

    return app_inst->init(config, notifier);
}

int hccast_iptv_app_deinit(void *inst)
{
    hccast_iptv_app_instance_st *app_inst = (hccast_iptv_app_instance_st *)inst;

    if (!app_inst || !app_inst->initialized || !app_inst->deinit)
    {
        return -1;
    }

    return app_inst->deinit();
}


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
int hccast_iptv_category_get(void *inst, hccast_iptv_category_node_st **cate)
{
    hccast_iptv_app_instance_st *app_inst = (hccast_iptv_app_instance_st *)inst;

    if (!app_inst || !app_inst->initialized || !app_inst->category_fetch)
    {
        return -1;
    }

    return app_inst->category_get(cate);
}

int hccast_iptv_category_open(void *inst, int index)
{
    hccast_iptv_app_instance_st *app_inst = (hccast_iptv_app_instance_st *)inst;

    if (!app_inst || !app_inst->initialized || !app_inst->category_open)
    {
        return -1;
    }

    return app_inst->category_open(index);
}

int hccast_iptv_category_count(void *inst)
{
    hccast_iptv_app_instance_st *app_inst = (hccast_iptv_app_instance_st *)inst;

    if (!app_inst || !app_inst->initialized || !app_inst->category_count)
    {
        return -1;
    }

    return app_inst->category_count();
}

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
int hccast_iptv_category_fetch(void *inst, hccast_iptv_cate_req_st *req, hccast_iptv_list_node_st **list_out)
{
    hccast_iptv_app_instance_st *app_inst = (hccast_iptv_app_instance_st *)inst;

    if (!app_inst || !app_inst->initialized || !app_inst->category_get)
    {
        return -1;
    }

    return app_inst->category_fetch(req, list_out);
}

int hccast_iptv_search(void *inst, hccast_iptv_search_req_st *req, hccast_iptv_list_node_st **list_out)
{
    hccast_iptv_app_instance_st *app_inst = (hccast_iptv_app_instance_st *)inst;

    if (!app_inst || !app_inst->initialized || !app_inst->search)
    {
        return -1;
    }

    return app_inst->search(req, list_out);
}

int hccast_iptv_page_next(void *inst, int direction)
{
    hccast_iptv_app_instance_st *app_inst = (hccast_iptv_app_instance_st *)inst;

    if (!app_inst || !app_inst->initialized || !app_inst->page_next)
    {
        return -1;
    }

    return app_inst->page_next(direction);
}

int hccast_iptv_page_get(void *inst, hccast_iptv_request_st *req, hccast_iptv_list_node_st **list_out)
{
    hccast_iptv_app_instance_st *app_inst = (hccast_iptv_app_instance_st *)inst;

    if (!app_inst || !app_inst->initialized || !app_inst->page_get)
    {
        return -1;
    }

    return app_inst->page_get(req, list_out);
}

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
int hccast_iptv_info_fetch(void *inst, hccast_iptv_info_req_st *req, hccast_iptv_links_node_st **links)
{
    hccast_iptv_app_instance_st *app_inst = (hccast_iptv_app_instance_st *)inst;

    if (!app_inst || !app_inst->initialized || !app_inst->info_fetch)
    {
        return -1;
    }

    return app_inst->info_fetch(req, links);
}

int hccast_iptv_link_get(void *inst, hccast_iptv_links_req_st *req, hccast_iptv_links_node_st **links)
{
    hccast_iptv_app_instance_st *app_inst = (hccast_iptv_app_instance_st *)inst;

    if (!app_inst || !app_inst->initialized || !app_inst->link_get)
    {
        return -1;
    }

    return app_inst->link_get(req, links);
}
