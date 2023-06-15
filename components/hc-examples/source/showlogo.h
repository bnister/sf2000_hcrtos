#ifndef __HC_SHOWLOGO__
#define __HC_SHOWLOGO__

typedef int (*hc_avread) (void *buf, int size, void *file);

int start_show_logo(void *file, hc_avread avread);
void wait_show_logo(void);
void stop_show_logo(void);



#endif
