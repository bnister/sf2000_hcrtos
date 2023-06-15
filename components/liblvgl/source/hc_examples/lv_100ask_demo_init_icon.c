/**
 * @file lv_100ask_demo_get_icon.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
/*#include "../../lv_100ask_modules.h"*/
#include <lvgl/lvgl.h>

#ifndef LV_USE_100ASK_DEMO_INIT_ICON
#define LV_USE_100ASK_DEMO_INIT_ICON 1
#endif

#if LV_USE_100ASK_DEMO_INIT_ICON && LV_BUILD_EXAMPLES

#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <dirent.h>
#include "lv_100ask_demo_init_icon.h"
#include <ffplayer.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>


#define ICON_PATH   ("./icon/")
#define BG_IMG_NAME ("net.ask100.lvgl.bg.png")
#define TAB_LEFT_APP_COUNT      (12)     // APP的直接个数
#define TAB_MAIN_APP_COUNT      (24)    // TAB_LEFT_APP_COUNT至TAB_MAIN_APP_COUNT之间APP的放在这里
#define TAB_RIGHT_APP_COUNT     (24)    // 剩下的APP都放到这里(大于TAB_MAIN_APP_COUNT)
#define MENU_TABLE_TIPS_PROMPT  (10)     // 菜单位置提示大小

#define ICON_SIZE           (64)
#define ICON_ROW_COUNT      (5)
#define ICON_COLUNM_COUNT   (6)
#define ICON_PAD_TOP        (40)
#define ICON_PAD_BOTTOM     (40)
#define ICON_PAD_LEFT       (115)
#define ICON_PAD_RIGHT      (115)

#define ICON_ROW_SPACE      (80)
#define ICON_COL_SPACE      (20)//((ICON_HOR_RES - (ICON_SIZE * ICON_COLUNM_COUNT)) / (ICON_COLUNM_COUNT - 1))
#define ICON_HOR_RES        (4 + (ICON_SIZE * ICON_COLUNM_COUNT) + (ICON_COL_SPACE * (ICON_COLUNM_COUNT - 1)))//((LV_HOR_RES - ICON_PAD_LEFT - ICON_PAD_RIGHT))        // 列间距
#define ICON_VER_RES        (4 + (ICON_SIZE * ICON_ROW_COUNT) + (ICON_ROW_SPACE * (ICON_ROW_COUNT - 1)))//((LV_VER_RES - ICON_PAD_TOP  - ICON_PAD_BOTTOM))       // 行间距

static uint16_t g_tab_act = 1;  // 更新tabview的索引，用于返回桌面时保持原来的位置

lv_obj_t * round_dot[3];

// 去掉最后的后缀名
static void strip_ext(char *fname)
{
	char *end = fname + strlen(fname);

	while (end > fname && *end != '.') {
		--end;
	}

	if (end > fname) {
		*end = '\0';
	}
}


// 替换给定字符串中所有出现的字符。
static void str_replace_all(char * str, char old_char, char new_char)
{
	int i = 0;

	/* Run till end of string */
	while(str[i] != '\0') {
		/* If occurrence of character is found */
		if(str[i] == old_char) {
			str[i] = new_char;
		}
		i++;
	}
}

static void event_handler(lv_event_t * e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t * obj = lv_event_get_target(e);
	//char * str_icon_name = lv_event_get_user_data(e);

	if(code == LV_EVENT_CLICKED) {
		char * str_icon_name = lv_label_get_text(lv_obj_get_child(obj, 0));
		//printf("str_icon_name: %s\n", str_icon_name);
		char str_dbus_path[64] = {0};
		sprintf(str_dbus_path, "/%s", str_icon_name);
		str_replace_all(str_dbus_path, '.', '/');
		/*dbus_method_call(str_icon_name, str_dbus_path, str_icon_name, "states", 1, 0);*/

		//char * str_dbus_path = malloc(sizeof(char) * strlen(str_icon_name) + 1);
		//free(str_dbus_path);
	}
}


static void lv_timer_update_time(lv_timer_t * timer)
{
	lv_obj_t * label = timer->user_data;

	// 获取系统时间
	char buf[32];
	time_t rawtime;
	struct tm *info;
	time(&rawtime);
	info = localtime(&rawtime);

	lv_label_set_text_fmt(label, "   %02d:%02d  %02d-%02d-%02d", info->tm_hour, info->tm_min, (info->tm_year + 2000 - 100), (info->tm_mon + 1), info->tm_mday);
}

static void lv_timer_update_tabview(lv_timer_t * timer)
{
	lv_obj_t *tabview_desktop = timer->user_data;
	g_tab_act++;
	g_tab_act %= 3;
	lv_tabview_set_act(tabview_desktop, g_tab_act, LV_ANIM_ON);

	if(round_dot[0])
		for(int i = 0; i < 3; i++) {
			if (i == g_tab_act)
				lv_obj_set_style_bg_opa(round_dot[i], LV_OPA_80, 0);
			else
				lv_obj_set_style_bg_opa(round_dot[i], LV_OPA_30, 0);
		}

}

static void lcd_top_widgets(lv_obj_t * parent)
{
	static lv_style_t obj_layout_style;   // 容器的样式

	lv_style_init(&obj_layout_style);
	lv_style_set_pad_all(&obj_layout_style, 0);
	lv_style_set_bg_opa(&obj_layout_style, LV_OPA_0);
	lv_style_set_text_font(&obj_layout_style, &lv_font_montserrat_16);
	lv_style_set_border_opa(&obj_layout_style, LV_OPA_0);
	lv_style_set_radius(&obj_layout_style, 0);
	lv_style_set_text_color(&obj_layout_style, lv_color_hex(0xffffff));

	/* Layout Init */
	lv_obj_t * panel = lv_obj_create(parent);
	lv_obj_set_size(panel,  LV_PCT(100), 30);
	lv_obj_add_style(panel, &obj_layout_style, 0);
	lv_obj_align(panel, LV_ALIGN_TOP_MID, 0, 5);

	/* 右上角小图标 */
	lv_obj_t * panel_icon = lv_obj_create(panel);
	lv_obj_set_size(panel_icon,  200, 25);
	lv_obj_set_layout(panel_icon, LV_LAYOUT_FLEX);
	lv_obj_set_style_base_dir(panel_icon, LV_BASE_DIR_RTL, 0);
	lv_obj_set_flex_flow(panel_icon, LV_FLEX_FLOW_ROW);
	lv_obj_align(panel_icon, LV_ALIGN_RIGHT_MID, 0, 0);
	lv_obj_add_style(panel_icon, &obj_layout_style, 0);

	lv_obj_t * label = lv_label_create(panel_icon);
	lv_label_set_text(label,  " ");

	lv_obj_t * label_bat = lv_label_create(panel_icon);
	lv_label_set_text(label_bat,  LV_SYMBOL_BATTERY_EMPTY);

	lv_obj_t * label_batchar = lv_label_create(label_bat);
	lv_obj_set_style_text_font(label_batchar, &lv_font_montserrat_14, 0);
	lv_label_set_text(label_batchar,  LV_SYMBOL_CHARGE);
	lv_obj_center(label_batchar);


	lv_obj_t * label_wifi = lv_label_create(panel_icon);
	lv_label_set_text(label_wifi, LV_SYMBOL_WIFI);

	lv_obj_t * label_time = lv_label_create(panel);
	lv_label_set_text(label_time, "  ");
	lv_obj_align(label_time, LV_ALIGN_LEFT_MID, 0, 0);

	lv_timer_t * timer = lv_timer_create(lv_timer_update_time, 1000,  label_time);
}


static void clean_screen_obj(lv_obj_t * parent)
{
	uint32_t i;
	for(i = 0; i < lv_obj_get_child_cnt(parent); i++) {
		lv_obj_t * child = lv_obj_get_child(parent, i);
		if (child != lv_scr_act())
			lv_obj_del(child);  // lv_obj_clean
		/*Do something with child*/
	}
}


static void set_menu_table_tips(lv_obj_t * parent, int count)
{
	lv_obj_t * round[3];

	/* 左菜单指示 */
	round[0] = lv_obj_create(parent);       // 左边
	lv_obj_set_style_border_opa(round[0], 0, 0);
	lv_obj_set_size(round[0], MENU_TABLE_TIPS_PROMPT, MENU_TABLE_TIPS_PROMPT);
	lv_obj_align(round[0], LV_ALIGN_CENTER, -20, 220);

	round[1] = lv_obj_create(parent);       // 中间
	lv_obj_set_style_border_opa(round[1], 0, 0);
	lv_obj_set_size(round[1], MENU_TABLE_TIPS_PROMPT, MENU_TABLE_TIPS_PROMPT);
	lv_obj_align(round[1], LV_ALIGN_CENTER, 0, 220);

	round[2] = lv_obj_create(parent);       // 右边
	lv_obj_set_style_border_opa(round[2], 0, 0);
	lv_obj_set_size(round[2], MENU_TABLE_TIPS_PROMPT, MENU_TABLE_TIPS_PROMPT);
	lv_obj_align(round[2], LV_ALIGN_CENTER, 20, 220);

	for(int i = 0; i < 3; i++) {
		if (i == count)
			lv_obj_set_style_bg_opa(round[i], LV_OPA_80, 0);
		else
			lv_obj_set_style_bg_opa(round[i], LV_OPA_30, 0);
	}

}


static void set_menu_table_tips_ext(lv_obj_t * parent)
{

	/* 左菜单指示 */
	round_dot[0] = lv_obj_create(parent);       // 左边
	lv_obj_set_style_border_opa(round_dot[0], 0, 0);
	lv_obj_set_size(round_dot[0], MENU_TABLE_TIPS_PROMPT, MENU_TABLE_TIPS_PROMPT);
	lv_obj_align(round_dot[0], LV_ALIGN_CENTER, -20, 240);

	round_dot[1] = lv_obj_create(parent);       // 中间
	lv_obj_set_style_border_opa(round_dot[1], 0, 0);
	lv_obj_set_size(round_dot[1], MENU_TABLE_TIPS_PROMPT, MENU_TABLE_TIPS_PROMPT);
	lv_obj_align(round_dot[1], LV_ALIGN_CENTER, 0, 240);

	round_dot[2] = lv_obj_create(parent);       // 右边
	lv_obj_set_style_border_opa(round_dot[2], 0, 0);
	lv_obj_set_size(round_dot[2], MENU_TABLE_TIPS_PROMPT, MENU_TABLE_TIPS_PROMPT);
	lv_obj_align(round_dot[2], LV_ALIGN_CENTER, 20, 240);


}

lv_obj_t *tabview_desktop;     // tab总页面
lv_obj_t *bg_img_gb;
static void get_tab_act_index_event_handler(lv_event_t * e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t * obj = lv_event_get_target(e);

	if(code == LV_EVENT_VALUE_CHANGED && obj == tabview_desktop) {
		if (obj) {
			g_tab_act = lv_tabview_get_tab_act(obj);
		} else g_tab_act = 1;
		printf("g_tab_act: %d\n", g_tab_act);
		if(round_dot[0])
			for(int i = 0; i < 3; i++) {
				if (i == g_tab_act)
					lv_obj_set_style_bg_opa(round_dot[i], LV_OPA_80, 0);
				else
					lv_obj_set_style_bg_opa(round_dot[i], LV_OPA_30, 0);
			}

	}

	if(code == LV_EVENT_SCROLL_BEGIN){
		/*printf("code: %d\n", code);*/
		/*if(bg_img_gb)*/
			/*lv_obj_add_flag(bg_img_gb, LV_OBJ_FLAG_HIDDEN);*/
	}
	if(code == LV_EVENT_SCROLL_END){
		/*printf("code: %d\n", code);*/
		/*if(bg_img_gb)*/
			/*lv_obj_clear_flag(bg_img_gb, LV_OBJ_FLAG_HIDDEN);*/
	}
}


void play_bg(char *path)
{
	int g_msgid = -1;
	HCPlayerInitArgs init_args = {0};
	g_msgid = msgget(MKTAG('h','c','p','l'), 0666 | IPC_CREAT);
	init_args.uri = path;
	init_args.sync_type = 0;
	init_args.play_attached_file = 1;
	init_args.msg_id = (int)g_msgid;
	init_args.callback = NULL;
	init_args.img_dis_hold_time = 10000000;

	void *player = hcplayer_create(&init_args);
	hcplayer_play(player);
}

//void lv_100ask_demo_init_icon(lv_anim_t * a)
void lv_100ask_demo_init_icon(char *dir_path)
{
	DIR *dr;
	struct dirent *de;              // Pointer for directory entry
	char bg_path_name[128];         // 存放桌面背景路径的缓冲区
	char icon_path_name[128];       // 存放APP图标路径的缓冲区
	int app_count = 1;              // 计数以区分不同的tab页面

	lv_obj_t * img_gb;              // 桌面背景
	lv_obj_t * img_icon;            // APP图标
	lv_obj_t * label_icon;          // 基于APP图标创建的名称，点击时图标时提取，不展示出来
	lv_obj_t * label_icon_name;     // 展示在图标下方的名称
	/*lv_obj_t * tabview_desktop;     // tab总页面*/
	lv_obj_t * img_bottom_icon;     // 展示在底部快速访问栏的图标
	lv_obj_t * label_bottom_icon;   // 基于底部快速访问栏的图标创建的名称，点击时图标时提取，不展示出来

	lv_obj_t * tab_left;            // 左边的tab页面
	lv_obj_t * tab_main;            // 中间的tab页面
	lv_obj_t * tab_right;           // 右边的tab页面
	lv_obj_t * icon_cont_left;      // 中间图标区域面板
	lv_obj_t * icon_cont_main;      // 中间图标区域面板
	lv_obj_t * icon_cont_right;     // 中间图标区域面板

	static lv_style_t style_tabview_desktop;    // 容器的样式
	static lv_style_t cont_style;               // 中间图标区域，容器的样式
	static lv_style_t icon_style;               // 中间图标区域，容器中的图标的样式
	static lv_style_t obj_bottom_panel_style;   // 底部容器的样式

	lv_img_cache_set_size(32);

	if(!dir_path)
		dir_path = ICON_PATH;

	if (lv_obj_get_child(lv_scr_act(), 0))
		lv_obj_del(lv_obj_get_child(lv_scr_act(), 0));
	lv_obj_t * screen = lv_obj_create(NULL);
	lv_scr_load(screen);

	//lv_style_reset(&cont_style);
	//lv_style_reset(&icon_style);
	//lv_style_reset(&obj_bottom_panel_style);

	lv_style_init(&style_tabview_desktop);
	//lv_style_set_pad_all(&style_tabview_desktop, 0);
	lv_style_set_bg_opa(&style_tabview_desktop, LV_OPA_0);
	//lv_style_set_shadow_opa(&style_tabview_desktop, 0);
	//lv_style_set_border_opa(&style_tabview_desktop, 0);

	/* 设置容器的样式 */
	lv_style_init(&cont_style);
	lv_style_set_bg_opa(&cont_style, LV_OPA_0);
	lv_style_set_border_opa(&cont_style, LV_OPA_0);
	lv_style_set_pad_column(&cont_style, ICON_COL_SPACE);
	lv_style_set_pad_row(&cont_style, ICON_ROW_SPACE);
	lv_style_set_pad_all(&cont_style, 0);
	lv_style_set_layout(&cont_style, LV_LAYOUT_FLEX);
	lv_style_set_base_dir(&cont_style, LV_BASE_DIR_LTR);
	lv_style_set_flex_flow(&cont_style, LV_FLEX_FLOW_ROW_WRAP);

	/* 容器中的图标的样式 */
	lv_style_init(&icon_style);
	lv_style_set_text_opa(&icon_style, LV_OPA_0);
	lv_style_set_text_font(&icon_style,  &lv_font_montserrat_8);
	//lv_style_set_radius(&icon_style, 0);
	//lv_style_set_border_width(&icon_style, 1);

	/* 底部面板区域 */
	lv_style_init(&obj_bottom_panel_style);
	lv_style_set_pad_all(&obj_bottom_panel_style, 0);
	lv_style_set_bg_opa(&obj_bottom_panel_style, LV_OPA_50);
	lv_style_set_pad_left(&obj_bottom_panel_style, 10);
	lv_style_set_pad_right(&obj_bottom_panel_style, 10);
	//lv_style_set_shadow_opa(&obj_bottom_panel_style, 0);
	lv_style_set_border_opa(&obj_bottom_panel_style, LV_OPA_0);
	lv_style_set_radius(&obj_bottom_panel_style, 22);

	// 桌面背景
#if 0
	bg_img_gb = lv_img_create(lv_scr_act());
	lv_snprintf(bg_path_name, sizeof(bg_path_name), "A:%s/%s", dir_path, BG_IMG_NAME);
	lv_img_set_src(bg_img_gb, bg_path_name);
	printf("bg_path_name: %s\n", bg_path_name);
#else
	lv_obj_set_style_bg_opa(lv_scr_act(), LV_OPA_TRANSP, 0);
	lv_snprintf(bg_path_name, sizeof(bg_path_name), "%s/%s", dir_path, BG_IMG_NAME);
	play_bg(bg_path_name);
#endif


	/* 屏幕顶部状态栏区域 */
	lcd_top_widgets(lv_scr_act());

	/*Create a Tab view object*/
	//tabview_desktop = lv_tabview_create(lv_layer_top(), LV_DIR_TOP, 0);
	tabview_desktop = lv_tabview_create(lv_scr_act(), LV_DIR_TOP, 0);
	lv_obj_add_style(tabview_desktop, &style_tabview_desktop, 0);
	/*lv_obj_add_event_cb(tabview_desktop, get_tab_act_index_event_handler, LV_EVENT_VALUE_CHANGED, NULL);*/
	lv_obj_add_event_cb(tabview_desktop, get_tab_act_index_event_handler, LV_EVENT_ALL, NULL);
	/*Add 3 tabs (the tabs are page (lv_page) and can be scrolled*/
	tab_left = lv_tabview_add_tab(tabview_desktop, "left_desktop");
	tab_main  = lv_tabview_add_tab(tabview_desktop, "main_desktop");
	tab_right = lv_tabview_add_tab(tabview_desktop, "right_desktop");
	lv_tabview_set_act(tabview_desktop, g_tab_act, LV_ANIM_OFF);

	/* 中间图标区域面板 */
	icon_cont_left = lv_obj_create(tab_left);
	/*lv_obj_set_size(icon_cont_left, ICON_HOR_RES, ICON_VER_RES);*/
	lv_obj_set_size(icon_cont_left, LV_PCT(100), LV_PCT(80));
	lv_obj_center(icon_cont_left);
	lv_obj_add_style(icon_cont_left, &cont_style, 0);

	icon_cont_main = lv_obj_create(tab_main);
	/*lv_obj_set_size(icon_cont_main, ICON_HOR_RES, ICON_VER_RES);*/
	lv_obj_set_size(icon_cont_main, LV_PCT(100), LV_PCT(80));
	lv_obj_center(icon_cont_main);
	lv_obj_add_style(icon_cont_main, &cont_style, 0);

	icon_cont_right = lv_obj_create(tab_right);
	/*lv_obj_set_size(icon_cont_right, ICON_HOR_RES, ICON_VER_RES);*/
	lv_obj_set_size(icon_cont_right, LV_PCT(100), LV_PCT(80));
	lv_obj_center(icon_cont_right);
	lv_obj_add_style(icon_cont_right, &cont_style, 0);
#if 0
	/* 底部快速访问栏面板 */
	lv_obj_t * bottom_panel = lv_obj_create(lv_scr_act());
	lv_obj_set_size(bottom_panel,  LV_PCT(70), 80);
	lv_obj_add_style(bottom_panel, &obj_bottom_panel_style, 0);
	lv_obj_set_layout(bottom_panel, LV_LAYOUT_FLEX);
	//lv_obj_set_style_base_dir(bottom_panel, LV_BASE_DIR_RTL, 0);
	lv_obj_set_flex_flow(bottom_panel, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(bottom_panel, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER);
	lv_obj_align(bottom_panel, LV_ALIGN_BOTTOM_MID, 0, -15);
#endif

	// 菜单位置提示大
#if 0
	set_menu_table_tips(tab_left, 0);
	set_menu_table_tips(tab_main, 1);
	set_menu_table_tips(tab_right, 2);
#else
	set_menu_table_tips_ext(lv_scr_act());
#endif

	// opendir() returns a pointer of DIR type.
	//DIR *dr = opendir("assets/icon");
	dr = opendir(dir_path);

	if (dr == NULL) { // opendir returns NULL if couldn't open directory
		printf("Could not open current directory!\n");
		return 0;
	}
	app_count = 1;  // 计数以区分不同的tab页面
	// Refer http://pubs.opengroup.org/onlinepubs/7990989775/xsh/readdir.html
	while ((de = readdir(dr)) != NULL) {
		//只存取 .png 扩展名的文件名
		if((strcmp(de->d_name,".") == 0)  ||\
		   (strcmp(de->d_name,"..") == 0) ||\
		   (strcmp((de->d_name + (strlen(de->d_name) - 4)), ".png") != 0)||\
		   (strcmp(de->d_name, "100ask_logo.png") == 0) ||\
		   (strcmp(de->d_name, "net.ask100.lvgl.bg.png") == 0)) {
			continue;
		}

		// 获取图片的： 路径+名称
		memset(icon_path_name, 0, sizeof(icon_path_name));
		lv_snprintf(icon_path_name, sizeof(icon_path_name), "A:%s/%s", dir_path, de->d_name);
		printf("icon_path_name: %s\n", icon_path_name);

		// 分页摆放APP
		if (app_count <= TAB_LEFT_APP_COUNT) {
			img_icon = lv_img_create(icon_cont_left);
			label_icon_name = lv_label_create(tab_left);    // 显示在图标下方的app名称
		} else if (app_count <= TAB_MAIN_APP_COUNT) {
			img_icon = lv_img_create(icon_cont_main);
			label_icon_name = lv_label_create(tab_main);    // 显示在图标下方的app名称
		} else if (app_count > TAB_RIGHT_APP_COUNT) {
			img_icon = lv_img_create(icon_cont_right);
			label_icon_name = lv_label_create(tab_right);    // 显示在图标下方的app名称
		}

		//printf("app_count:%d\n", app_count);
		app_count++;

		// 图标
		lv_img_set_src(img_icon, icon_path_name);
		lv_obj_add_flag(img_icon, LV_OBJ_FLAG_CLICKABLE);
		lv_obj_add_style(img_icon, &icon_style, 0);
		lv_obj_add_event_cb(img_icon, event_handler, LV_EVENT_CLICKED, NULL);
#if 0
		// 底部快速访问栏
		img_bottom_icon = lv_img_create(bottom_panel);
		lv_img_set_src(img_bottom_icon, icon_path_name);
		//lv_img_set_zoom(img_bottom_icon, 250);
		lv_obj_add_flag(img_bottom_icon, LV_OBJ_FLAG_CLICKABLE);
		lv_obj_add_style(img_bottom_icon, &icon_style, 0);
		lv_obj_add_event_cb(img_bottom_icon, event_handler, LV_EVENT_CLICKED, NULL);

		// 点击图标时调用的服务名(点击图标时父类提取)
		strip_ext(de->d_name);  // 去掉最后的后缀名 .png
		label_icon = lv_label_create(img_icon);
		lv_obj_set_width(label_icon, 64);
		lv_label_set_text(label_icon, de->d_name);

		// 点击图标时调用的服务名(点击图标时父类提取)
		label_bottom_icon = lv_label_create(img_bottom_icon);
		lv_obj_set_width(label_bottom_icon, 64);
		lv_label_set_text(label_bottom_icon, de->d_name);

#endif

		// 显示在图标下方的app名称
		lv_obj_set_style_text_font(label_icon_name, &lv_font_montserrat_28, 0);
		lv_obj_set_style_text_color(label_icon_name, lv_color_hex(0xffffff), 0);
		lv_obj_set_style_text_align(label_icon_name, LV_TEXT_ALIGN_CENTER, 0);
		lv_obj_set_width(label_icon_name, 128);
		/*lv_label_set_text(label_icon_name, "net.ask100.lvglnet.ask100.lvglnet.ask100.lvgl");*/
		lv_label_set_text(label_icon_name, "net.as");
		lv_label_set_long_mode(label_icon_name, LV_LABEL_LONG_SCROLL_CIRCULAR);     /*Circular scroll*/
		/*lv_label_set_text(label_icon_name, strrchr(de->d_name, '.') + 1);*/
		lv_obj_align_to(label_icon_name, img_icon, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);
		lv_obj_move_foreground(label_icon_name); // 防止被桌面背景图覆盖，将其移到前台
		//printf("%s\n", de->d_name);
	}
	//lv_obj_move_background(img_gb);  // 将背景移动到后台

	closedir(dr);
	lv_timer_t * timer = lv_timer_create(lv_timer_update_tabview, 2000,  tabview_desktop);
	return 0;
}


#if 0
void lv_100ask_demo_init_icon(void)
{
	DIR *dr;
	struct dirent *de;                          // Pointer for directory entry
	char icon_path_name[128];

	lv_obj_t * img_gb;
	lv_obj_t * img_icon;
	lv_obj_t * label_icon;
	lv_obj_t * label_icon_name;
	lv_obj_t * img_bottom_icon;
	lv_obj_t * label_bottom_icon;

	static lv_style_t cont_style;               // 中间图标区域，容器的样式
	static lv_style_t icon_style;               // 中间图标区域，容器中的图标的样式
	static lv_style_t obj_bottom_panel_style;   // 底部容器的样式

	if (lv_obj_get_child(lv_scr_act(), 0))
		lv_obj_del(lv_obj_get_child(lv_scr_act(), 0));
	lv_obj_t * screen = lv_obj_create(NULL);
	lv_scr_load(screen);

	//lv_style_reset(&cont_style);
	//lv_style_reset(&icon_style);
	//lv_style_reset(&obj_bottom_panel_style);

	/* 设置容器的样式 */
	lv_style_init(&cont_style);
	lv_style_set_bg_opa(&cont_style, 0);
	lv_style_set_border_opa(&cont_style, 0);
	lv_style_set_pad_column(&cont_style, ICON_COL_SPACE);
	lv_style_set_pad_row(&cont_style, ICON_ROW_SPACE);
	lv_style_set_pad_all(&cont_style, 0);


	/* 容器中的图标的样式 */
	lv_style_init(&icon_style);
	lv_style_set_text_opa(&icon_style, 0);
	lv_style_set_text_font(&icon_style,  &lv_font_montserrat_8);
	//lv_style_set_radius(&icon_style, 0);
	//lv_style_set_border_width(&icon_style, 1);

	/* 屏幕顶部状态栏区域 */
	lcd_top_widgets(lv_scr_act());

	/* 中间图标区域 */
	lv_obj_t * icon_cont = lv_obj_create(lv_scr_act());
	lv_obj_set_size(icon_cont, ICON_HOR_RES, ICON_VER_RES);
	lv_obj_set_layout(icon_cont, LV_LAYOUT_FLEX);
	lv_obj_set_style_base_dir(icon_cont, LV_BASE_DIR_LTR, 0);
	lv_obj_set_flex_flow(icon_cont, LV_FLEX_FLOW_ROW_WRAP);
	lv_obj_center(icon_cont);
	//lv_obj_align(icon_cont, LV_ALIGN_CENTER, 0, -10);
	lv_obj_add_style(icon_cont, &cont_style, 0);

	/* 底部面板区域 */
	lv_style_init(&obj_bottom_panel_style);
	lv_style_set_pad_all(&obj_bottom_panel_style, 0);
	lv_style_set_bg_opa(&obj_bottom_panel_style, LV_OPA_50);
	lv_style_set_pad_left(&obj_bottom_panel_style, 10);
	lv_style_set_pad_right(&obj_bottom_panel_style, 10);

	//lv_style_set_shadow_opa(&obj_bottom_panel_style, 0);
	lv_style_set_border_opa(&obj_bottom_panel_style, 0);
	lv_style_set_radius(&obj_bottom_panel_style, 22);

	/* Layout Init */
	lv_obj_t * bottom_panel = lv_obj_create(lv_scr_act());
	lv_obj_set_size(bottom_panel,  LV_PCT(70), 80);
	lv_obj_add_style(bottom_panel, &obj_bottom_panel_style, 0);
	lv_obj_set_layout(bottom_panel, LV_LAYOUT_FLEX);
	//lv_obj_set_style_base_dir(bottom_panel, LV_BASE_DIR_RTL, 0);
	lv_obj_set_flex_flow(bottom_panel, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(bottom_panel, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER);
	lv_obj_align(bottom_panel, LV_ALIGN_BOTTOM_MID, 0, -15);

	// opendir() returns a pointer of DIR type.
	//DIR *dr = opendir("assets/icon");
	dr = opendir(ICON_PATH);

	if (dr == NULL) { // opendir returns NULL if couldn't open directory
		printf("Could not open current directory!\n");
		return 0;
	}


	// Refer http://pubs.opengroup.org/onlinepubs/7990989775/xsh/readdir.html
	// for readdir()
	while ((de = readdir(dr)) != NULL) {
		if((strcmp(de->d_name,".") == 0)  ||\
		   (strcmp(de->d_name,"..") == 0) ||\
		   (strcmp((de->d_name + (strlen(de->d_name) - 4)), ".png") != 0)||\
		   (strcmp(de->d_name, "100ask_logo.png") == 0)) {
			continue;
		}
		//if(strcmp((de->d_name + (strlen(de->d_name) - 4)) , "100ask_logo.png") == 0)  //只存取 .png 扩展名的文件名
		//{
		//    continue;
		//}

		memset(icon_path_name, 0, sizeof(icon_path_name));
		lv_snprintf(icon_path_name, sizeof(icon_path_name), "A:%s/%s", dir_path, de->d_name);
		// 背景
		if(strcmp(de->d_name, "net.ask100.lvgl.bg.png") == 0) {
			img_gb = lv_img_create(lv_scr_act());
			lv_img_set_src(img_gb, icon_path_name);
		}
		// 图标
		else {
			img_icon = lv_img_create(icon_cont);
			lv_img_set_src(img_icon, icon_path_name);
			lv_obj_add_flag(img_icon, LV_OBJ_FLAG_CLICKABLE);
			lv_obj_add_style(img_icon, &icon_style, 0);
			lv_obj_add_event_cb(img_icon, event_handler, LV_EVENT_CLICKED, NULL);

			img_bottom_icon = lv_img_create(bottom_panel);
			lv_img_set_src(img_bottom_icon, icon_path_name);
			//lv_img_set_zoom(img_bottom_icon, 250);
			lv_obj_add_flag(img_bottom_icon, LV_OBJ_FLAG_CLICKABLE);
			lv_obj_add_style(img_bottom_icon, &icon_style, 0);
			lv_obj_add_event_cb(img_bottom_icon, event_handler, LV_EVENT_CLICKED, NULL);

			// 点击图标时调用的服务名(点击图标时父类提取)
			strip_ext(de->d_name);  // 去掉最后的后缀名 .png
			label_icon = lv_label_create(img_icon);
			lv_obj_set_width(label_icon, 64);
			lv_label_set_text(label_icon, de->d_name);

			// 点击图标时调用的服务名(点击图标时父类提取)
			label_bottom_icon = lv_label_create(img_bottom_icon);
			lv_obj_set_width(label_bottom_icon, 64);
			lv_label_set_text(label_bottom_icon, de->d_name);

			// 显示在图标下方的app名称
			label_icon_name = lv_label_create(lv_scr_act());
			lv_obj_set_style_text_font(label_icon_name, &lv_font_montserrat_14, 0);
			lv_obj_set_style_text_color(label_icon_name, lv_color_hex(0xffffff), 0);
			lv_obj_set_style_text_align(label_icon_name, LV_TEXT_ALIGN_CENTER, 0);
			lv_obj_set_width(label_icon_name, 64);
			lv_label_set_text(label_icon_name, strrchr(de->d_name, '.') + 1);
			lv_obj_align_to(label_icon_name, img_icon, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);
			lv_obj_move_foreground(label_icon_name); // 防止被桌面背景图覆盖，将其移到前台
		}

		//printf("%s\n", de->d_name);
	}
	lv_obj_move_background(img_gb);  // 将背景移动到后台

	closedir(dr);
	return 0;
}
#endif

#endif
