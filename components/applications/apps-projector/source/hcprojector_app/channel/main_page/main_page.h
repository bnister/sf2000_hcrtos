typedef enum {
	MAIN_PAGE_PROMPT_WIFI,
	MAIN_PAGE_PROMOT_BT,
} main_page_prompt_t;

typedef enum {
	MAIN_PAGE_PROMPT_STATUS_OFF,
	MAIN_PAGE_PROMPT_STATUS_ON
} main_page_prompt_v;

void main_page_prompt_status_set(main_page_prompt_t t,main_page_prompt_v  v);