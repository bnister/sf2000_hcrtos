#include <nuttx/config.h>
#include <sys/types.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>
#include <errno.h>


#ifdef CONFIG_DEV_PIPEDUMP

void lib_dumpbuffer(FAR const char *msg, FAR const uint8_t *buffer,
                    unsigned int buflen)
{
    unsigned int index;
	portENTER_CRITICAL();
    
    printf("%s (len:%u): \n", msg, buflen);
    for(index = 0; index < buflen; index++){
        if(index % 16 == 0)
            printf("%3.3u: ", index);
        printf("%2.2x ", buffer[index]);
        if(index % 16 == 15)
            printf("\r\n");
    }
    if(index % 16)
        printf("\r\n");
    portEXIT_CRITICAL();
}
#endif /* CONFIG_DEV_PIPEDUMP */
