#include <getopt.h>
#include "com_api.h"
#include "menu_mgr.h"

#define MAX_WINS_CNT	10
static win_des_t *m_menu_stack[MAX_WINS_CNT];
static uint8_t m_cur_win_num = 0;

/**
 * @brief menu stack initialization
 * 
 */
void menu_mgr_init(void)
{
    memset(m_menu_stack, 0, sizeof(win_des_t*) * MAX_WINS_CNT);
    m_cur_win_num = 0;
}

/**
 * @brief push menu to the top of menu stack
 * 
 * @param win 
 */
void menu_mgr_push(win_des_t *win)
{
	if(m_cur_win_num>=MAX_WINS_CNT)
		return;
	m_menu_stack[m_cur_win_num++] = win;
}

/**
 * @brief push menu to menu stack, special the push postion of menu stack
 * 
 * @param win 
 * @param shift 
 */
void menu_mgr_push_ext(win_des_t *win, int8_t shift)
{
	uint8_t	i;
	
	if(m_cur_win_num >= MAX_WINS_CNT)
		return;

	if(shift >= 0)
		menu_mgr_push(win);
	else
	{
		if((0 - shift) > m_cur_win_num)
			return;
		for(i = m_cur_win_num + shift; i < m_cur_win_num ; i++)
		{
			m_menu_stack[i + 1] = m_menu_stack[i];
		}
		m_menu_stack[(uint8_t)((int8_t)m_cur_win_num + shift)] = win;
		m_cur_win_num ++;
	}
}

/**
 * @brief pop up the top menu of menu stack
 * 
 */
void menu_mgr_pop(void)
{
	if(m_cur_win_num>0)
		m_cur_win_num --;
}


/**
 * @brief pop all menu from menu stack
 * 
 */
void menu_mgr_pop_all(void)
{
	m_cur_win_num = 0;
}

/**
 * @brief get the top menu of menu stack
 * 
 * @return win_des_t* 
 */
win_des_t *menu_mgr_get_top(void)
{
	if(m_cur_win_num==0)
		return NULL;

	return m_menu_stack[m_cur_win_num - 1];
}

uint8_t menu_mgr_get_cnt(void)
{
	return m_cur_win_num;
}
