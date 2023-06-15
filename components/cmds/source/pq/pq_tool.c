#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
// #include <termios.h>
// #include <sys/mman.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <kernel/lib/fdt_api.h>
#include <sys/ioctl.h>
#include <hcuapi/pq.h>

#include <kernel/lib/console.h>
#include <kernel/completion.h>
#include <hcuapi/common.h>
#include <poll.h>

#include <errno.h>
#include <nuttx/compiler.h>
#include <nuttx/semaphore.h>

#include "hc_pq_reg.h"


#define PQ_REG_BASE     (PQ_REG_BASE_ADDR & ~(0x7<<29)) //(0x18836000)
#define MAP_SIZE 0xFF
#define PQ_BUF_SIZE    (4 * 1024)

/***************************************
* pq data mapping (unit: uint16_t)
***************************************/
#define VERSION              0  //size:8
#define DATE                 8  //size:8
#define IS_OVERWRITE         16  //size:1

#define PQ_EN               17  //size:1
#define IGAM_EN             18  //size:1
#define MTX_EN              19  //size:1
#define GNTF_EN             20  //size:1
#define SLMT_EN             21  //size:1
#define GAM_EN              22  //size:1

#define LINEAR_REGION_EN    23  //size:1
#define LINEAR_REGION       24  //size:1
#define LINEAR_SLOPE        25  //size:1

#define MTX_C00             26  //size:1
#define MTX_C01             27  //size:1
#define MTX_C02             28  //size:1
#define MTX_C10             29  //size:1
#define MTX_C11             30  //size:1
#define MTX_C12             31  //size:1
#define MTX_C20             32  //size:1
#define MTX_C21             33  //size:1
#define MTX_C22             34  //size:1

#define GNTF_GAIN_R         35  //size:1
#define GNTF_GAIN_G         36  //size:1
#define GNTF_GAIN_B         37  //size:1
#define GNTF_OFST_R         38  //size:1
#define GNTF_OFST_G         39  //size:1
#define GNTF_OFST_B         40  //size:1

#define SLMT_NEG_EN         41  //size:1
#define SLMT_MAX_EN         42  //size:1
#define SLMT_NEG_OFTGN      43  //size:1
#define SLMT_SLOPE          44  //size:1
#define SLMT_DELTA          45  //size:1

#define INVGAMMA_R_ARRAY    46  //size:67
#define INVGAMMA_G_ARRAY    113 //size:67
#define INVGAMMA_B_ARRAY    180 //size:67
#define GAMMA_R_ARRAY       247 //size:131
#define GAMMA_G_ARRAY       378 //size:131
#define GAMMA_B_ARRAY       509 //size:131



// #if 1 //for debug
// struct g_pq_bak_t{
//     u8 *pq_data;
//     u32 pq_data_len;
//     u32 fail;
//     u32 total;
//     u32 index;
// }g_pq_bak;
// #endif


static uint32_t GET_PARAM(uint16_t *ptr)
{
    uint16_t val = *ptr;
    uint32_t ret = ((val & 0xff) << 8) | ((val & 0xff00) >> 8);
    return ret;
}

/*************************************/

static void dump_pq_data(uint8_t *buf, int len);



typedef struct __pq_app{
    // int mem_fd;
    int uart_fd;
    // int console_fd;
    uint16_t *pq_data;
    int pq_data_length;
    uint8_t *pq_reg_map_base;
    bool is_option;
}pq_app_t;


/* for pq_debug control */
bool is_debug = false;

#define pq_debug(format, ...)  do{\
                                if(is_debug) \
                                    printf("[PQ] "format, ##__VA_ARGS__);\
                            }while(0)

/* put terminal in raw mode - see termio(7I) for modes */
// static void tty_raw(int fd)
// {
//     struct termios raw;
//     static struct termios orig_termios;

//     if (tcgetattr(fd, &orig_termios) < 0)
//          printf("can't get tty settings\n");

//     raw = orig_termios;  /* copy original and then modify below */

//     /* input modes - clear indicated ones giving: no break, no CR to NL,
//         no parity check, no strip char, no start/stop output (sic) control */
//     raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);

//     /* output modes - clear giving: no post processing such as NL to CR+NL */
//     raw.c_oflag &= ~(OPOST);

//     /* control modes - set 8 bit chars */
//     raw.c_cflag |= (CS8);

//     /* local modes - clear giving: echoing off, canonical off (no erase with
//         backspace, ^U,...),  no extended functions, no signal chars (^Z,^C) */
//     raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);

//     /* control chars - set return condition: min number of bytes and timer */
//     // raw.c_cc[VMIN] = 1; raw.c_cc[VTIME] = 8; /* after 1 bytes or 0.8 seconds
//     //                                             after first byte seen      */
//     // raw.c_cc[VMIN] = 0; raw.c_cc[VTIME] = 0; /* immediate - anything       */
//     raw.c_cc[VMIN] = 4; raw.c_cc[VTIME] = 0; /* after 4 bytes, no timer  */
//     // raw.c_cc[VMIN] = 0; raw.c_cc[VTIME] = 8; /* after a byte or 0.8 seconds */

//     /* put terminal in raw mode after flushing */
//     if (tcsetattr(fd,TCSAFLUSH,&raw) < 0)
//         printf("can't set raw mode\n");
// }


static int get_pq_data(int fd, uint8_t *buf, int buf_len)
{
    int total = 0;
    int rd;
    int n = 1;
    fd_set rfds;
    struct timeval tv;

#define PQ_DATA_LEN 1280
    int r, index;
	struct pollfd fds[] = {
		{ .fd = 0,
		  .events = POLLIN },
	};

    while(1){
        // r = poll(fds, 1, -1);
        // if(r == -1){
        //     printf("uart poll fail!!!\n");
        //     g_pq_bak.fail++;
        //     break;
        // }
        
        for(index = 0; index < PQ_DATA_LEN; index++){
            read(0, buf + index, 1);
            // g_pq_bak.index++;
            // printf("==> index:%d -- 0x%x, %c\n", 
            //             index, buf[index], buf[index]);
        }

        if(index >= PQ_DATA_LEN)
            break;
    }
    total = index;

    printf("===> read finish, total:%u\n", total);

    return total;
}

static void decode_pq_data(pq_app_t *p_pq_app)
{
    uint32_t index;
    uint8_t ver_str[32];
    uint8_t date_str[32];
    uint16_t *pq_data = p_pq_app->pq_data;

    memset(&ver_str[0], 0, 32);
    memset(&date_str[0], 0, 32);
    memcpy(&ver_str[0], (uint8_t *)(pq_data + VERSION), 16);
    memcpy(&date_str[0], (uint8_t *)(pq_data + DATE), 16);

    printf("PQ data Version: %s\n", ver_str);
    printf("PQ data Date: %s\n", date_str);

    if(!is_debug)
        return;

    pq_debug("\n================= Decode PQ data ======================\n");
    pq_debug("is overwrite: %s\n", GET_PARAM(pq_data + IS_OVERWRITE) ? "true" : "false");
    pq_debug("PQ EN:%s\n", GET_PARAM(pq_data + PQ_EN) ? "true" : "false");
    pq_debug("IGAM EN:%s\n", GET_PARAM(pq_data + IGAM_EN) ? "true" : "false");
    pq_debug("MTX EN:%s\n", GET_PARAM(pq_data + MTX_EN) ? "true" : "false");
    pq_debug("GNTF EN:%s\n", GET_PARAM(pq_data + GNTF_EN) ? "true" : "false");
    pq_debug("SLMT EN:%s\n", GET_PARAM(pq_data + SLMT_EN) ? "true" : "false");
    pq_debug("GAM EN:%s\n", GET_PARAM(pq_data + GAM_EN) ? "true" : "false");
    pq_debug("\n");
    pq_debug("linear EN:%s\n", GET_PARAM(pq_data + LINEAR_REGION_EN) ? "true" : "false");
    pq_debug("linear region:%lu\n", GET_PARAM(pq_data + LINEAR_REGION));
    pq_debug("linear slope:%lu\n", GET_PARAM(pq_data + LINEAR_SLOPE));
    pq_debug("\n");
    if(GET_PARAM(pq_data + IS_OVERWRITE)){
        pq_debug("mtx_c00:%lu\n", GET_PARAM(pq_data + MTX_C00));
        pq_debug("mtx_c01:%lu\n", GET_PARAM(pq_data + MTX_C01));
        pq_debug("mtx_c02:%lu\n", GET_PARAM(pq_data + MTX_C02));
        pq_debug("mtx_c10:%lu\n", GET_PARAM(pq_data + MTX_C10));
        pq_debug("mtx_c11:%lu\n", GET_PARAM(pq_data + MTX_C11));
        pq_debug("mtx_c12:%lu\n", GET_PARAM(pq_data + MTX_C12));
        pq_debug("mtx_c20:%lu\n", GET_PARAM(pq_data + MTX_C20));
        pq_debug("mtx_c21:%lu\n", GET_PARAM(pq_data + MTX_C21));
        pq_debug("mtx_c22:%lu\n", GET_PARAM(pq_data + MTX_C22));
        pq_debug("\n");
    }
    pq_debug("gntf_gain-R:%lu\n", GET_PARAM(pq_data + GNTF_GAIN_R));
    pq_debug("gntf_gain-G:%lu\n", GET_PARAM(pq_data + GNTF_GAIN_G));
    pq_debug("gntf_gain-B:%lu\n", GET_PARAM(pq_data + GNTF_GAIN_B));
    pq_debug("gntf_offset-R:%lu\n", GET_PARAM(pq_data + GNTF_OFST_R));
    pq_debug("gntf_offset-G:%lu\n", GET_PARAM(pq_data + GNTF_OFST_G));
    pq_debug("gntf_offset-B:%lu\n", GET_PARAM(pq_data + GNTF_OFST_B));
    pq_debug("\n");
    pq_debug("slmt_neg_en:%s\n", GET_PARAM(pq_data + SLMT_NEG_EN) ? "true" : "false");
    pq_debug("slmt_max_en:%s\n", GET_PARAM(pq_data + SLMT_MAX_EN) ? "true" : "false");
    pq_debug("slmt_neg_oftgn:%lu\n", GET_PARAM(pq_data + SLMT_NEG_OFTGN));
    pq_debug("slmt_neg_slope:%lu\n", GET_PARAM(pq_data + SLMT_SLOPE));
    pq_debug("slmt_neg_delta:%lu\n", GET_PARAM(pq_data + SLMT_DELTA));
    pq_debug("\n");
    if(GET_PARAM(pq_data + IS_OVERWRITE)){
        pq_debug("invgamma -- R (total:67): \n");
        for(index = 0; index < 60; index += 10)
            pq_debug("\t %d, %d, %d, %d, %d, %d, %d, %d, %d, %d\n",
                    (short)GET_PARAM(pq_data + INVGAMMA_R_ARRAY + index), 
                    (short)GET_PARAM(pq_data + INVGAMMA_R_ARRAY + index + 1), 
                    (short)GET_PARAM(pq_data + INVGAMMA_R_ARRAY + index + 2), 
                    (short)GET_PARAM(pq_data + INVGAMMA_R_ARRAY + index + 3), 
                    (short)GET_PARAM(pq_data + INVGAMMA_R_ARRAY + index + 4), 
                    (short)GET_PARAM(pq_data + INVGAMMA_R_ARRAY + index + 5), 
                    (short)GET_PARAM(pq_data + INVGAMMA_R_ARRAY + index + 6), 
                    (short)GET_PARAM(pq_data + INVGAMMA_R_ARRAY + index + 7), 
                    (short)GET_PARAM(pq_data + INVGAMMA_R_ARRAY + index + 8), 
                    (short)GET_PARAM(pq_data + INVGAMMA_R_ARRAY + index + 9));
        pq_debug("\t %d, %d, %d, %d, %d, %d, %d\n",
            (short)GET_PARAM(pq_data + INVGAMMA_R_ARRAY + index + 0),
            (short)GET_PARAM(pq_data + INVGAMMA_R_ARRAY + index + 1),
            (short)GET_PARAM(pq_data + INVGAMMA_R_ARRAY + index + 2),
            (short)GET_PARAM(pq_data + INVGAMMA_R_ARRAY + index + 3),
            (short)GET_PARAM(pq_data + INVGAMMA_R_ARRAY + index + 4),
            (short)GET_PARAM(pq_data + INVGAMMA_R_ARRAY + index + 5),
            (short)GET_PARAM(pq_data + INVGAMMA_R_ARRAY + index + 6));

        pq_debug("invgamma -- G (total:67): \n");
        for(index = 0; index < 60; index += 10)
            pq_debug("\t %d, %d, %d, %d, %d, %d, %d, %d, %d, %d\n",
                    (short)GET_PARAM(pq_data + INVGAMMA_G_ARRAY + index), 
                    (short)GET_PARAM(pq_data + INVGAMMA_G_ARRAY + index + 1), 
                    (short)GET_PARAM(pq_data + INVGAMMA_G_ARRAY + index + 2), 
                    (short)GET_PARAM(pq_data + INVGAMMA_G_ARRAY + index + 3), 
                    (short)GET_PARAM(pq_data + INVGAMMA_G_ARRAY + index + 4), 
                    (short)GET_PARAM(pq_data + INVGAMMA_G_ARRAY + index + 5), 
                    (short)GET_PARAM(pq_data + INVGAMMA_G_ARRAY + index + 6), 
                    (short)GET_PARAM(pq_data + INVGAMMA_G_ARRAY + index + 7), 
                    (short)GET_PARAM(pq_data + INVGAMMA_G_ARRAY + index + 8), 
                    (short)GET_PARAM(pq_data + INVGAMMA_G_ARRAY + index + 9));
        pq_debug("\t %d, %d, %d, %d, %d, %d, %d\n",
            (short)GET_PARAM(pq_data + INVGAMMA_G_ARRAY + index + 0),
            (short)GET_PARAM(pq_data + INVGAMMA_G_ARRAY + index + 1),
            (short)GET_PARAM(pq_data + INVGAMMA_G_ARRAY + index + 2),
            (short)GET_PARAM(pq_data + INVGAMMA_G_ARRAY + index + 3),
            (short)GET_PARAM(pq_data + INVGAMMA_G_ARRAY + index + 4),
            (short)GET_PARAM(pq_data + INVGAMMA_G_ARRAY + index + 5),
            (short)GET_PARAM(pq_data + INVGAMMA_G_ARRAY + index + 6));

        pq_debug("invgamma -- B (total:67): \n");
        for(index = 0; index < 60; index += 10)
            pq_debug("\t %d, %d, %d, %d, %d, %d, %d, %d, %d, %d\n",
                    (short)GET_PARAM(pq_data + INVGAMMA_B_ARRAY + index), 
                    (short)GET_PARAM(pq_data + INVGAMMA_B_ARRAY + index + 1), 
                    (short)GET_PARAM(pq_data + INVGAMMA_B_ARRAY + index + 2), 
                    (short)GET_PARAM(pq_data + INVGAMMA_B_ARRAY + index + 3), 
                    (short)GET_PARAM(pq_data + INVGAMMA_B_ARRAY + index + 4), 
                    (short)GET_PARAM(pq_data + INVGAMMA_B_ARRAY + index + 5), 
                    (short)GET_PARAM(pq_data + INVGAMMA_B_ARRAY + index + 6), 
                    (short)GET_PARAM(pq_data + INVGAMMA_B_ARRAY + index + 7), 
                    (short)GET_PARAM(pq_data + INVGAMMA_B_ARRAY + index + 8), 
                    (short)GET_PARAM(pq_data + INVGAMMA_B_ARRAY + index + 9));
        pq_debug("\t %d, %d, %d, %d, %d, %d, %d\n",
            (short)GET_PARAM(pq_data + INVGAMMA_B_ARRAY + index + 0),
            (short)GET_PARAM(pq_data + INVGAMMA_B_ARRAY + index + 1),
            (short)GET_PARAM(pq_data + INVGAMMA_B_ARRAY + index + 2),
            (short)GET_PARAM(pq_data + INVGAMMA_B_ARRAY + index + 3),
            (short)GET_PARAM(pq_data + INVGAMMA_B_ARRAY + index + 4),
            (short)GET_PARAM(pq_data + INVGAMMA_B_ARRAY + index + 5),
            (short)GET_PARAM(pq_data + INVGAMMA_B_ARRAY + index + 6));
    }
    pq_debug("\n");


    pq_debug("gamma -- R (total:131): \n");
    for(index = 0; index < 130; index += 10)
        pq_debug("\t %d, %d, %d, %d, %d, %d, %d, %d, %d, %d\n",
                (short)GET_PARAM(pq_data + GAMMA_R_ARRAY + index), 
                (short)GET_PARAM(pq_data + GAMMA_R_ARRAY + index + 1), 
                (short)GET_PARAM(pq_data + GAMMA_R_ARRAY + index + 2), 
                (short)GET_PARAM(pq_data + GAMMA_R_ARRAY + index + 3), 
                (short)GET_PARAM(pq_data + GAMMA_R_ARRAY + index + 4), 
                (short)GET_PARAM(pq_data + GAMMA_R_ARRAY + index + 5), 
                (short)GET_PARAM(pq_data + GAMMA_R_ARRAY + index + 6), 
                (short)GET_PARAM(pq_data + GAMMA_R_ARRAY + index + 7), 
                (short)GET_PARAM(pq_data + GAMMA_R_ARRAY + index + 8), 
                (short)GET_PARAM(pq_data + GAMMA_R_ARRAY + index + 9));
    pq_debug("\t %d\n",
        (short)GET_PARAM(pq_data + GAMMA_R_ARRAY + index + 0));

    pq_debug("gamma -- G (total:131): \n");
    for(index = 0; index < 130; index += 10)
        pq_debug("\t %d, %d, %d, %d, %d, %d, %d, %d, %d, %d\n",
                (short)GET_PARAM(pq_data + GAMMA_G_ARRAY + index), 
                (short)GET_PARAM(pq_data + GAMMA_G_ARRAY + index + 1), 
                (short)GET_PARAM(pq_data + GAMMA_G_ARRAY + index + 2), 
                (short)GET_PARAM(pq_data + GAMMA_G_ARRAY + index + 3), 
                (short)GET_PARAM(pq_data + GAMMA_G_ARRAY + index + 4), 
                (short)GET_PARAM(pq_data + GAMMA_G_ARRAY + index + 5), 
                (short)GET_PARAM(pq_data + GAMMA_G_ARRAY + index + 6), 
                (short)GET_PARAM(pq_data + GAMMA_G_ARRAY + index + 7), 
                (short)GET_PARAM(pq_data + GAMMA_G_ARRAY + index + 8), 
                (short)GET_PARAM(pq_data + GAMMA_G_ARRAY + index + 9));
    pq_debug("\t %d\n",
        (short)GET_PARAM(pq_data + GAMMA_G_ARRAY + index + 0));

    pq_debug("gamma -- B (total:131): \n");
    for(index = 0; index < 130; index += 10)
        pq_debug("\t %d, %d, %d, %d, %d, %d, %d, %d, %d, %d\n",
                (short)GET_PARAM(pq_data + GAMMA_B_ARRAY + index), 
                (short)GET_PARAM(pq_data + GAMMA_B_ARRAY + index + 1), 
                (short)GET_PARAM(pq_data + GAMMA_B_ARRAY + index + 2), 
                (short)GET_PARAM(pq_data + GAMMA_B_ARRAY + index + 3), 
                (short)GET_PARAM(pq_data + GAMMA_B_ARRAY + index + 4), 
                (short)GET_PARAM(pq_data + GAMMA_B_ARRAY + index + 5), 
                (short)GET_PARAM(pq_data + GAMMA_B_ARRAY + index + 6), 
                (short)GET_PARAM(pq_data + GAMMA_B_ARRAY + index + 7), 
                (short)GET_PARAM(pq_data + GAMMA_B_ARRAY + index + 8), 
                (short)GET_PARAM(pq_data + GAMMA_B_ARRAY + index + 9));
    pq_debug("\t %d\n",
        (short)GET_PARAM(pq_data + GAMMA_B_ARRAY + index + 0));
    pq_debug("=========================================================\n\n");

}


static void dump_pq_data(uint8_t *buf, int len)
{
    int index;
    printf("\n================= PQ raw data (len:%d) ===============", len);
    for(index = 0; index < len; index++){
        if(index % 16 == 0)
            printf("\n%8.8x: ", index);
        printf("%2.2x ", buf[index]);
    }
    printf("\n================= PQ raw data =====================\n\n");
}


static int init_pq_env(pq_app_t *p_pq_app)
{
	const char *path = NULL;
	int np;

	np = fdt_get_node_offset_by_path("/hcrtos/stdio");
	assert(np >= 0);
	assert(fdt_get_property_string_index(np, "serial0", 0, &path) == 0);
	assert(path != NULL);
	np = fdt_node_probe_by_path(path);
	if (np < 0)
		np = fdt_node_probe_by_path("/hcrtos/uart_dummy");
	assert(np >= 0);
	assert(fdt_get_property_string_index(np, "devpath", 0, &path) == 0);

    printf("uart console path : %s\n", path);

    p_pq_app->uart_fd = 0; // stdio // open(path, O_RDWR);
    if (p_pq_app->uart_fd < 0) {
        printf("open(%s) failed\n", path);
        goto __exit1;
    }
    printf("open(%s) successfully, fd=%d\n", path, p_pq_app->uart_fd);

    printf("Waiting for data to be retrieved from %s\n", path);
    // printf("(note: Press ctrl+c to exit....)\n");
    p_pq_app->pq_data = malloc(PQ_BUF_SIZE);
    if(p_pq_app->pq_data == NULL){
        printf("cannot malloc enough memory for PQ data buffer\n");
        goto __exit1;
    }
    memset((uint8_t *)p_pq_app->pq_data, 0, PQ_BUF_SIZE);

    /* Get PQ raw data from usb uart(/dev/ttyGS0) */
    p_pq_app->pq_data_length = get_pq_data(p_pq_app->uart_fd,
                                 (uint8_t *)p_pq_app->pq_data,
                                 PQ_BUF_SIZE);
    if(p_pq_app->pq_data_length <= 0){
        printf("Error when reading from %s\n", path);
        goto __exit2;
    }
    printf("Successfully read PQ raw data, length is %d bytes\n",
                                        p_pq_app->pq_data_length);

    if(is_debug)
        dump_pq_data((uint8_t *)p_pq_app->pq_data, p_pq_app->pq_data_length);

    return 0;

__exit2:
    free(p_pq_app->pq_data);
    // munmap(p_pq_app->pq_reg_map_base, MAP_SIZE);
__exit1:
    // if (p_pq_app->console_fd > 0)
    //     close(p_pq_app->console_fd);
    if (p_pq_app->uart_fd > 0)
        close(p_pq_app->uart_fd);
    // if (p_pq_app->mem_fd > 0)
    //     close(p_pq_app->mem_fd);

    return -1;
}


static void deinit_pq_env(pq_app_t *p_pq_app)
{
    // struct termios console_term;
    // tcgetattr(p_pq_app->console_fd, &console_term);
    // tcsetattr(p_pq_app->uart_fd, TCSANOW,  &console_term);
    // tcflush(p_pq_app->uart_fd, TCIOFLUSH);

    free(p_pq_app->pq_data);
    // munmap(p_pq_app->pq_reg_map_base, MAP_SIZE);
    // if (p_pq_app->console_fd > 0)
    //     close(p_pq_app->console_fd);
    if (p_pq_app->uart_fd > 0)
        close(p_pq_app->uart_fd);
    // if (p_pq_app->mem_fd > 0)
    //     close(p_pq_app->mem_fd);
}

typedef union {
    uint32_t value;
    struct __invgamma_st{
        uint32_t data0      : 12;
        uint32_t data1      : 12;
        uint32_t addr       : 6;
        uint32_t reserve    : 1;
        uint32_t is_write   : 1;
    }bits;
}__invgamma;

typedef union {
    uint32_t value;
    struct __gamma_st{
        uint32_t data0      : 12;
        uint32_t data1      : 12;
        uint32_t addr       : 7;
        uint32_t is_write   : 1;
    }bits;
}__gamma;

static void pq_configure_invgamma(pq_app_t *p_pq_app)
{
#define INVEGAMMA_CNT  67
    int index;
    uint16_t *invgamma;
    __invgamma reg;

    /* invgamma -- R */
    invgamma = p_pq_app->pq_data + INVGAMMA_R_ARRAY;
    for(index = 0; index < INVEGAMMA_CNT / 2; index++){
        reg.bits.data0 = GET_PARAM(invgamma + (2 * index));
        reg.bits.data1 = GET_PARAM(invgamma + (2 * index + 1));
        reg.bits.addr = index;
        reg.bits.reserve = 0;
        reg.bits.is_write = 1;
        pq_debug("invagmma-R(%u): 0x%lx\n", index, reg.value);
        pq_hal_write_invgamma_lut_r_data(reg.value);
    }
    reg.bits.data0 = GET_PARAM(invgamma + 2 * index);
    reg.bits.data1 = 0;
    reg.bits.addr = index;
    reg.bits.reserve = 0;
    reg.bits.is_write = 1;
    pq_debug("invagmma-R(%u): 0x%lx\n", index, reg.value);
    pq_hal_write_invgamma_lut_r_data(reg.value);


    /* invgamma -- G */
    invgamma = p_pq_app->pq_data + INVGAMMA_G_ARRAY;
    for(index = 0; index < INVEGAMMA_CNT / 2; index++){
        reg.bits.data0 = GET_PARAM(invgamma + (2 * index));
        reg.bits.data1 = GET_PARAM(invgamma + (2 * index + 1));
        reg.bits.addr = index;
        reg.bits.reserve = 0;
        reg.bits.is_write = 1;
        pq_debug("invagmma-G(%u): 0x%lx\n", index, reg.value);
        pq_hal_write_invgamma_lut_g_data(reg.value);
    }
    reg.bits.data0 = GET_PARAM(invgamma + 2 * index);
    reg.bits.data1 = 0;
    reg.bits.addr = index;
    reg.bits.reserve = 0;
    reg.bits.is_write = 1;
    pq_debug("invagmma-G(%u): 0x%lx\n", index, reg.value);
    pq_hal_write_invgamma_lut_g_data(reg.value);


    /* invgamma -- B */
    invgamma = p_pq_app->pq_data + INVGAMMA_B_ARRAY;
    for(index = 0; index < INVEGAMMA_CNT / 2; index++){
        reg.bits.data0 = GET_PARAM(invgamma + (2 * index));
        reg.bits.data1 = GET_PARAM(invgamma + (2 * index + 1));
        reg.bits.addr = index;
        reg.bits.reserve = 0;
        reg.bits.is_write = 1;
        pq_debug("invagmma-B(%u): 0x%lx\n", index, reg.value);
        pq_hal_write_invgamma_lut_b_data(reg.value);
    }
    reg.bits.data0 = GET_PARAM(invgamma + 2 * index);
    reg.bits.data1 = 0;
    reg.bits.addr = index;
    reg.bits.reserve = 0;
    reg.bits.is_write = 1;
    pq_debug("invagmma-B(%u): 0x%lx\n", index, reg.value);
    pq_hal_write_invgamma_lut_b_data(reg.value);
}



static void pq_configure_gamma(pq_app_t *p_pq_app)
{
#define GAMMA_CNT  131
    int index;
    uint16_t *gamma;
    __gamma reg;

    /* gamma -- R */
    gamma = p_pq_app->pq_data + GAMMA_R_ARRAY;
    for(index = 0; index < GAMMA_CNT / 2; index++){
        reg.bits.data0 = GET_PARAM(gamma + (2 * index));
        reg.bits.data1 = GET_PARAM(gamma + (2 * index + 1));
        reg.bits.addr = index;
        reg.bits.is_write = 1;
        pq_debug("gmma-R(%u): 0x%lx\n", index, reg.value);
        pq_hal_write_gamma_lut_r_data(reg.value);
    }
    reg.bits.data0 = GET_PARAM(gamma + 2 * index);
    reg.bits.data1 = 0;
    reg.bits.addr = index;
    reg.bits.is_write = 1;
    pq_debug("gmma-R(%u): 0x%lx\n", index, reg.value);
    pq_hal_write_gamma_lut_r_data(reg.value);

    /* gamma -- G */
    gamma = p_pq_app->pq_data + GAMMA_G_ARRAY;
    for(index = 0; index < GAMMA_CNT / 2; index++){
        reg.bits.data0 = GET_PARAM(gamma + (2 * index));
        reg.bits.data1 = GET_PARAM(gamma + (2 * index + 1));
        reg.bits.addr = index;
        reg.bits.is_write = 1;
        pq_debug("gmma-G(%u): 0x%lx\n", index, reg.value);
        pq_hal_write_gamma_lut_g_data(reg.value);
    }
    reg.bits.data0 = GET_PARAM(gamma + 2 * index);
    reg.bits.data1 = 0;
    reg.bits.addr = index;
    reg.bits.is_write = 1;
    pq_debug("gmma-G(%u): 0x%lx\n", index, reg.value);
    pq_hal_write_gamma_lut_g_data(reg.value);

    /* gamma -- B */
    gamma = p_pq_app->pq_data + GAMMA_B_ARRAY;
    for(index = 0; index < GAMMA_CNT / 2; index++){
        reg.bits.data0 = GET_PARAM(gamma + (2 * index));
        reg.bits.data1 = GET_PARAM(gamma + (2 * index + 1));
        reg.bits.addr = index;
        reg.bits.is_write = 1;
        pq_debug("gmma-B(%u): 0x%lx\n", index, reg.value);
        pq_hal_write_gamma_lut_b_data(reg.value);
    }
    reg.bits.data0 = GET_PARAM(gamma + 2 * index);
    reg.bits.data1 = 0;
    reg.bits.addr = index;
    reg.bits.is_write = 1;
    pq_debug("gmma-B(%u): 0x%lx\n", index, reg.value);
    pq_hal_write_gamma_lut_b_data(reg.value);
}


static void pq_configure(pq_app_t *p_pq_app)
{
    uint16_t *pq_data = p_pq_app->pq_data;

    /* configure PQ register */
    pq_hal_init();  //setup PQ register basse address

    pq_hal_set_en(GET_PARAM(pq_data + PQ_EN));
    pq_hal_set_igam_en(GET_PARAM(pq_data + IGAM_EN));
    pq_hal_set_mtx_en(GET_PARAM(pq_data + MTX_EN));
    pq_hal_set_gntf_en(GET_PARAM(pq_data + GNTF_EN));
    pq_hal_set_slmt_en(GET_PARAM(pq_data + SLMT_EN));
    pq_hal_set_gam_en(GET_PARAM(pq_data + GAM_EN));

    pq_hal_set_linear_slope(GET_PARAM(pq_data + LINEAR_SLOPE));
    pq_hal_set_linear_region(GET_PARAM(pq_data + LINEAR_REGION));
    pq_hal_set_linear_en(GET_PARAM(pq_data + LINEAR_REGION_EN));

    if(!p_pq_app->is_option)
        pq_hal_set_mtx_coef(GET_PARAM(pq_data + MTX_C00),
                            GET_PARAM(pq_data + MTX_C01),
                            GET_PARAM(pq_data + MTX_C02),
                            GET_PARAM(pq_data + MTX_C10),
                            GET_PARAM(pq_data + MTX_C11),
                            GET_PARAM(pq_data + MTX_C12),
                            GET_PARAM(pq_data + MTX_C20),
                            GET_PARAM(pq_data + MTX_C21),
                            GET_PARAM(pq_data + MTX_C22));

    pq_hal_set_gntf_gain(GET_PARAM(pq_data + GNTF_GAIN_R),
                            GET_PARAM(pq_data + GNTF_GAIN_G),
                            GET_PARAM(pq_data + GNTF_GAIN_B));

    pq_hal_set_gntf_ofst(GET_PARAM(pq_data + GNTF_OFST_R),
                            GET_PARAM(pq_data + GNTF_OFST_G),
                            GET_PARAM(pq_data + GNTF_OFST_B));

    pq_hal_set_slmt_neg_en(GET_PARAM(pq_data + SLMT_NEG_EN));
    pq_hal_set_slmt_max_en(GET_PARAM(pq_data + SLMT_MAX_EN));
    pq_hal_set_slmt_neg_oftgn(GET_PARAM(pq_data + SLMT_NEG_OFTGN));
    pq_hal_set_slmt_slope(GET_PARAM(pq_data + SLMT_SLOPE));
    pq_hal_set_slmt_delta(GET_PARAM(pq_data + SLMT_DELTA));

    if(!p_pq_app->is_option)
        pq_configure_invgamma(p_pq_app);

    pq_configure_gamma(p_pq_app);
}


int pq_config_cnt = 0;



// static struct pq_settings pqReg = {
//     // // VERSION
//     // "1.0.0", "20220628093552",
//     // // COMMON
//     // true,
//     // // COLOR DEVICE
//     // 0.640000, 0.330000, 0.300000, 0.600000, 0.150000, 0.060000, 0.312700, 0.329000,

//     "dummy",
//     // 0x00
//     true, true, true, true, true, true,
//     // 0x10
//     true, 21, 57,
//     // 0x20
//     1024, 0,
//     // 0x24
//     0, 0,
//     // 0x28
//     1024, 0,
//     // 0x2c
//     0, 0,
//     // 0x30
//     1024,
//     // 0x34
//     128, 128, 128,
//     // 0x38
//     0, 0, 0,
//     // 0x40
//     true, true, 6, 8, 29,
   
//     // 0x60
//     { -3, 0, 4, 7, 11, 14, 18, 21, 25, 30, 35, 40, 46, 52, 58, 65, 72, 80, 88, 97, 106, 115, 125, 136, 147, 158, 170, 182, 195, 208, 222, 236, 250, 266, 281, 297, 314, 331, 349, 367, 385, 405, 424, 444, 465, 486, 508, 530, 553, 576, 600, 625, 650, 675, 701, 728, 755, 782, 810, 839, 868, 898, 929, 959, 991, 1023, 1056 },
//     { -3, 0, 4, 7, 11, 14, 18, 21, 25, 30, 35, 40, 46, 52, 58, 65, 72, 80, 88, 97, 106, 115, 125, 136, 147, 158, 170, 182, 195, 208, 222, 236, 250, 266, 281, 297, 314, 331, 349, 367, 385, 405, 424, 444, 465, 486, 508, 530, 553, 576, 600, 625, 650, 675, 701, 728, 755, 782, 810, 839, 868, 898, 929, 959, 991, 1023, 1056 },
//     { -3, 0, 4, 7, 11, 14, 18, 21, 25, 30, 35, 40, 46, 52, 58, 65, 72, 80, 88, 97, 106, 115, 125, 136, 147, 158, 170, 182, 195, 208, 222, 236, 250, 266, 281, 297, 314, 331, 349, 367, 385, 405, 424, 444, 465, 486, 508, 530, 553, 576, 600, 625, 650, 675, 701, 728, 755, 782, 810, 839, 868, 898, 929, 959, 991, 1023, 1056 },
    
//     // 0x70
//     { 0, 0, 28, 39, 46, 53, 58, 63, 68, 72, 76, 80, 84, 87, 90, 93, 96, 99, 102, 105, 107, 110, 112, 115, 117, 119, 121, 124, 126, 128, 130, 132, 134, 136, 138, 140, 141, 143, 145, 147, 149, 150, 152, 154, 155, 157, 159, 160, 162, 163, 165, 166, 168, 169, 171, 172, 174, 175, 177, 178, 179, 181, 182, 183, 185, 186, 187, 189, 190, 191, 193, 194, 195, 196, 198, 199, 200, 201, 202, 204, 205, 206, 207, 208, 209, 211, 212, 213, 214, 215, 216, 217, 218, 219, 221, 222, 223, 224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239, 240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 249, 250, 251, 252, 253, 254, 255, 256 },
//     { 0, 0, 28, 39, 46, 53, 58, 63, 68, 72, 76, 80, 84, 87, 90, 93, 96, 99, 102, 105, 107, 110, 112, 115, 117, 119, 121, 124, 126, 128, 130, 132, 134, 136, 138, 140, 141, 143, 145, 147, 149, 150, 152, 154, 155, 157, 159, 160, 162, 163, 165, 166, 168, 169, 171, 172, 174, 175, 177, 178, 179, 181, 182, 183, 185, 186, 187, 189, 190, 191, 193, 194, 195, 196, 198, 199, 200, 201, 202, 204, 205, 206, 207, 208, 209, 211, 212, 213, 214, 215, 216, 217, 218, 219, 221, 222, 223, 224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239, 240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 249, 250, 251, 252, 253, 254, 255, 256 },
//     { 0, 0, 28, 39, 46, 53, 58, 63, 68, 72, 76, 80, 84, 87, 90, 93, 96, 99, 102, 105, 107, 110, 112, 115, 117, 119, 121, 124, 126, 128, 130, 132, 134, 136, 138, 140, 141, 143, 145, 147, 149, 150, 152, 154, 155, 157, 159, 160, 162, 163, 165, 166, 168, 169, 171, 172, 174, 175, 177, 178, 179, 181, 182, 183, 185, 186, 187, 189, 190, 191, 193, 194, 195, 196, 198, 199, 200, 201, 202, 204, 205, 206, 207, 208, 209, 211, 212, 213, 214, 215, 216, 217, 218, 219, 221, 222, 223, 224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239, 240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 249, 250, 251, 252, 253, 254, 255, 256 }
// };

void __pq_start(void)
{
#define SYS_VIDEO_SRC_CTRL_VAL (*(volatile unsigned long *)0xb8800444)
#define SYS_VIDEO_SRC_CTRL_VAL1 (*(volatile unsigned long *)0xb8800448)
#define SYS_VIDEO_SRC_CTRL_VAL2 (*(volatile unsigned long *)0xb880044c)

#define E_SRC_SEL_HDMI_PQ (0x3)
#define E_SRC_SEL_FXDE  (0x0)
#define E_SRC_SEL_4KDE  (0x1)

	int np;
    u32 de4k;
    bool g_b_rgb_high_to_low[3] = { false, false, false };
    int rgb_r_lowhigh_source_sel = 0;
    int rgb_g_lowhigh_source_sel = 0;
    int rgb_b_lowhigh_source_sel = 0;

    np = fdt_node_probe_by_path("/hcrtos/de-engine");
	if (np < 0){
        printf("PQ-TOOL error: cannot read 'de-engine' from DTS\n");
		return;
    }
    if(fdt_get_property_u_32_index(np, "de4k-output", 0, &de4k)){
        printf("PQ-TOOL error: cannot read 'de4k-output' from DTS\n");
        return;
    }

    rgb_r_lowhigh_source_sel = (SYS_VIDEO_SRC_CTRL_VAL2 >> 0) & 0x7;
    rgb_g_lowhigh_source_sel = (SYS_VIDEO_SRC_CTRL_VAL2 >> 4) & 0x7;
    rgb_b_lowhigh_source_sel = (SYS_VIDEO_SRC_CTRL_VAL2 >> 8) & 0x7;


    if((rgb_r_lowhigh_source_sel >= 0 && rgb_r_lowhigh_source_sel <= 3) ||
       ((SYS_VIDEO_SRC_CTRL_VAL2>>28) & 0x1 ==1))
    {
        g_b_rgb_high_to_low[0] = true;
    }

    if((rgb_g_lowhigh_source_sel >= 0 && rgb_g_lowhigh_source_sel <= 3)||
       ((SYS_VIDEO_SRC_CTRL_VAL2 >> 29) & 0x1 == 1))
    {
        g_b_rgb_high_to_low[1] = true;
    }

    if((rgb_g_lowhigh_source_sel >= 0 && rgb_g_lowhigh_source_sel <= 3)||
       ((SYS_VIDEO_SRC_CTRL_VAL2 >> 30) & 0x1 == 1))
    {
        g_b_rgb_high_to_low[2] = true;
    }

    SYS_VIDEO_SRC_CTRL_VAL &= ~(0x3 << 20);//PQ CLK SRC
    SYS_VIDEO_SRC_CTRL_VAL1 &= ~(0x3 << 4);//PQ DATA SRC
    SYS_VIDEO_SRC_CTRL_VAL1 &= ~(0x3 << 0);//HDMI TX SRC
    SYS_VIDEO_SRC_CTRL_VAL1 &= ~(0xF << 12);//LVDS SRC
    SYS_VIDEO_SRC_CTRL_VAL1 &= ~(0x3 << 16);//MIPI SRC
    SYS_VIDEO_SRC_CTRL_VAL1 &= ~(0x3 << 18);//RGB SRC
    SYS_VIDEO_SRC_CTRL_VAL2 &= ~(0x70000777); //RGB DATA SRC

    if(de4k == 0)
    {
        SYS_VIDEO_SRC_CTRL_VAL |= (E_SRC_SEL_FXDE << 20);//PQ CLK SRC
        SYS_VIDEO_SRC_CTRL_VAL1 |= (E_SRC_SEL_FXDE << 4);//PQ DATA SRC
    }
    else
    {
        SYS_VIDEO_SRC_CTRL_VAL |= (E_SRC_SEL_4KDE << 20);//PQ CLK SRC
        SYS_VIDEO_SRC_CTRL_VAL1 |= (E_SRC_SEL_4KDE << 4);//PQ DATA SRC
    }

    SYS_VIDEO_SRC_CTRL_VAL1 |= (E_SRC_SEL_HDMI_PQ << 0);//HDMI TX SRC
    SYS_VIDEO_SRC_CTRL_VAL1 |= (E_SRC_SEL_HDMI_PQ << 12);//LVDS SRC
    SYS_VIDEO_SRC_CTRL_VAL1 |= (E_SRC_SEL_HDMI_PQ << 14);//LVDS SRC
    SYS_VIDEO_SRC_CTRL_VAL1 |= (E_SRC_SEL_HDMI_PQ << 16);//MIPI SRC
    SYS_VIDEO_SRC_CTRL_VAL1 |= (E_SRC_SEL_HDMI_PQ << 18);//RGB SRC

    SYS_VIDEO_SRC_CTRL_VAL2 |= (0x0000777);
    SYS_VIDEO_SRC_CTRL_VAL2 |= g_b_rgb_high_to_low[0] << 28;
    SYS_VIDEO_SRC_CTRL_VAL2 |= g_b_rgb_high_to_low[1] << 29;
    SYS_VIDEO_SRC_CTRL_VAL2 |= g_b_rgb_high_to_low[2] << 30;

    printf("pq start...\n");
    printf("==============================\n");
    printf("reg0xb8800444: 0x%8.8lx\n", SYS_VIDEO_SRC_CTRL_VAL);
    printf("reg0xb8800448: 0x%8.8lx\n", SYS_VIDEO_SRC_CTRL_VAL1);
    printf("reg0xb880044c: 0x%8.8lx\n", SYS_VIDEO_SRC_CTRL_VAL2);
    printf("==============================\n");
}



void __pq_tool(void *p)
{
    pq_app_t pq_app;
    pq_app_t *p_pq_app = &pq_app;
    int ret, pq_fd;
    bool pq_start = false;

    __pq_start();

    // #if 1 //for debug
    // g_pq_bak.pq_data = malloc(2000);
    // g_pq_bak.fail = 0;
    // g_pq_bak.total = 0;
    // g_pq_bak.index = 0;
    // #endif

    do{
        memset(p_pq_app, 0, sizeof(pq_app_t));
        ret = init_pq_env(p_pq_app);
        if(ret < 0){
            printf("Error when perparing PQ envirenment\n");
            deinit_pq_env(p_pq_app);
            break;
        }
        decode_pq_data(p_pq_app);

        // #if 1 //for debug
        // memset(g_pq_bak.pq_data, 0xa5, 2000);
        // g_pq_bak.pq_data_len = p_pq_app->pq_data_length;
        // g_pq_bak.total++;
        // memcpy(g_pq_bak.pq_data,
        //         p_pq_app->pq_data, 
        //         p_pq_app->pq_data_length);
        // #endif

        pq_configure(p_pq_app);
        deinit_pq_env(p_pq_app);
        pq_config_cnt++;
    }while(1);

    printf("PQ tool exit....\n");
}

int pq_tool(int argc, char **argv)
{
    int c, ret;
    // sem_t semphr;

    while ((c = getopt (argc, argv, "hd")) != -1){
        switch (c)
        {
            case 'd':
                is_debug = true;
                break;
            case 'h':
                printf("\nPQ tool -- complierd at %s %s\n", __DATE__, __TIME__);
                printf("usage: \n");
                printf("  pq_tool    ## run PQ tool\n");
                printf("  pq_tool -h ## help information\n");
                printf("  pq_tool -d ## run PQ tool, and list PQ raw data received from /dev/ttySA0\n");
                return 0;
                break;
            default:
                printf("Error parameter (-%c), please check.\n", c);
                break;
        }
    }

    // nxsem_init(&semphr, 0, 0);

	xTaskCreate(__pq_tool, (const char *)"pq_tool", configTASK_STACK_DEPTH,
		    NULL, portPRI_TASK_NORMAL, NULL);

    printf("pq-tool: console task will be halted, you should use hichip pq_tool to send pq configuration date\n");

    while(1)
        sleep(100);
    // nxsem_wait_uninterruptible(&semphr);

    printf("pq-tool: exit...\n");
    // nxsem_destroy(&semphr);

    return 0;
}



CONSOLE_CMD(pq_tool, NULL, pq_tool, CONSOLE_CMD_MODE_SELF, "pq configuration tool")
