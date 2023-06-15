#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/mman.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <poll.h>

#include "hc_pq_reg.h"

#define PQ_REG_BASE (PQ_REG_BASE_ADDR & ~(0x7 << 29)) //(0x18836000)
#define MAP_SIZE 0xFF
#define PQ_BUF_SIZE (4 * 1024)
#define PQ_DATA_LEN 1280

/***************************************
* pq data mapping (unit: uint16_t)
***************************************/
#define VERSION 0 //size:8
#define DATE 8 //size:8
#define IS_OVERWRITE 16 //size:1

#define PQ_EN 17 //size:1
#define IGAM_EN 18 //size:1
#define MTX_EN 19 //size:1
#define GNTF_EN 20 //size:1
#define SLMT_EN 21 //size:1
#define GAM_EN 22 //size:1

#define LINEAR_REGION_EN 23 //size:1
#define LINEAR_REGION 24 //size:1
#define LINEAR_SLOPE 25 //size:1

#define MTX_C00 26 //size:1
#define MTX_C01 27 //size:1
#define MTX_C02 28 //size:1
#define MTX_C10 29 //size:1
#define MTX_C11 30 //size:1
#define MTX_C12 31 //size:1
#define MTX_C20 32 //size:1
#define MTX_C21 33 //size:1
#define MTX_C22 34 //size:1

#define GNTF_GAIN_R 35 //size:1
#define GNTF_GAIN_G 36 //size:1
#define GNTF_GAIN_B 37 //size:1
#define GNTF_OFST_R 38 //size:1
#define GNTF_OFST_G 39 //size:1
#define GNTF_OFST_B 40 //size:1

#define SLMT_NEG_EN 41 //size:1
#define SLMT_MAX_EN 42 //size:1
#define SLMT_NEG_OFTGN 43 //size:1
#define SLMT_SLOPE 44 //size:1
#define SLMT_DELTA 45 //size:1

#define INVGAMMA_R_ARRAY 46 //size:67
#define INVGAMMA_G_ARRAY 113 //size:67
#define INVGAMMA_B_ARRAY 180 //size:67
#define GAMMA_R_ARRAY 247 //size:131
#define GAMMA_G_ARRAY 378 //size:131
#define GAMMA_B_ARRAY 509 //size:131

typedef struct __pq_app {
    int uart_fd;
    int console_fd;
    int mem_fd;
    uint16_t *pq_data;
    int pq_data_length;
    uint8_t *pq_reg_map_base;
    bool is_option;
} pq_app_t;

typedef union {
    uint32_t value;
    struct __invgamma_st {
        uint32_t data0 : 12;
        uint32_t data1 : 12;
        uint32_t addr : 6;
        uint32_t reserve : 1;
        uint32_t is_write : 1;
    } bits;
} __invgamma;

typedef union {
    uint32_t value;
    struct __gamma_st {
        uint32_t data0 : 12;
        uint32_t data1 : 12;
        uint32_t addr : 7;
        uint32_t is_write : 1;
    } bits;
} __gamma;

#define pq_debug(format, ...)                                                  \
    do {                                                                   \
        if (g_is_debug)                                                \
            printf("[PQ] " format, ##__VA_ARGS__);                 \
    } while (0)

/* for pq_debug control */
bool g_is_debug = false;

/* put terminal in raw mode - see termio(7I) for modes */
static void tty_raw(int fd)
{
    struct termios raw;
    static struct termios orig_termios;

    if (tcgetattr(fd, &orig_termios) < 0)
         printf("can't get tty settings\n");

    raw = orig_termios;  /* copy original and then modify below */

    /* input modes - clear indicated ones giving: no break, no CR to NL,
        no parity check, no strip char, no start/stop output (sic) control */
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);

    /* output modes - clear giving: no post processing such as NL to CR+NL */
    raw.c_oflag &= ~(OPOST);

    /* control modes - set 8 bit chars */
    raw.c_cflag |= (CS8);

    /* local modes - clear giving: echoing off, canonical off (no erase with
        backspace, ^U,...),  no extended functions, no signal chars (^Z,^C) */
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);

    /* control chars - set return condition: min number of bytes and timer */
    // raw.c_cc[VMIN] = 1; raw.c_cc[VTIME] = 8; /* after 1 bytes or 0.8 seconds
    //                                             after first byte seen      */
    // raw.c_cc[VMIN] = 0; raw.c_cc[VTIME] = 0; /* immediate - anything       */
    raw.c_cc[VMIN] = 4; raw.c_cc[VTIME] = 0; /* after 4 bytes, no timer  */
    // raw.c_cc[VMIN] = 0; raw.c_cc[VTIME] = 8; /* after a byte or 0.8 seconds */

    /* put terminal in raw mode after flushing */
    if (tcsetattr(fd,TCSAFLUSH,&raw) < 0)
        printf("can't set raw mode\n");
}


static void pq_tool_dump(pq_app_t *ins)
{
    int index;
    uint8_t *buf = (uint8_t *)ins->pq_data;
    int len = ins->pq_data_length;

    if (!g_is_debug)
        return;

    printf("\n================= PQ raw data (len:%d) ===============", len);
    for (index = 0; index < len; index++) {
        if (index % 16 == 0)
            printf("\n%8.8x: ", index);
        printf("%2.2x ", buf[index]);
    }
    printf("\n================= PQ raw data =====================\n\n");
}

static uint32_t GET_PARAM(uint16_t *ptr)
{
    uint16_t val = *ptr;
    uint32_t ret = ((val & 0xff) << 8) | ((val & 0xff00) >> 8);
    return ret;
}

static void pq_configure_gamma(pq_app_t *ins)
{
#define GAMMA_CNT 131
    int index;
    uint16_t *gamma;
    __gamma reg;

    /* gamma -- R */
    gamma = ins->pq_data + GAMMA_R_ARRAY;
    for (index = 0; index < GAMMA_CNT / 2; index++) {
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
    gamma = ins->pq_data + GAMMA_G_ARRAY;
    for (index = 0; index < GAMMA_CNT / 2; index++) {
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
    gamma = ins->pq_data + GAMMA_B_ARRAY;
    for (index = 0; index < GAMMA_CNT / 2; index++) {
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

static void pq_configure_invgamma(pq_app_t *ins)
{
#define INVEGAMMA_CNT 67
    int index;
    uint16_t *invgamma;
    __invgamma reg;

    /* invgamma -- R */
    invgamma = ins->pq_data + INVGAMMA_R_ARRAY;
    for (index = 0; index < INVEGAMMA_CNT / 2; index++) {
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
    invgamma = ins->pq_data + INVGAMMA_G_ARRAY;
    for (index = 0; index < INVEGAMMA_CNT / 2; index++) {
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
    invgamma = ins->pq_data + INVGAMMA_B_ARRAY;
    for (index = 0; index < INVEGAMMA_CNT / 2; index++) {
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

static void pq_tool_configure(pq_app_t *ins)
{
    uint16_t *pq_data = ins->pq_data;

    /* configure PQ register */
    // pq_hal_init(); //setup PQ register basse address
    __pq_hal_init(ins->pq_reg_map_base);

    pq_hal_set_en(GET_PARAM(pq_data + PQ_EN));
    pq_hal_set_igam_en(GET_PARAM(pq_data + IGAM_EN));
    pq_hal_set_mtx_en(GET_PARAM(pq_data + MTX_EN));
    pq_hal_set_gntf_en(GET_PARAM(pq_data + GNTF_EN));
    pq_hal_set_slmt_en(GET_PARAM(pq_data + SLMT_EN));
    pq_hal_set_gam_en(GET_PARAM(pq_data + GAM_EN));

    pq_hal_set_linear_slope(GET_PARAM(pq_data + LINEAR_SLOPE));
    pq_hal_set_linear_region(GET_PARAM(pq_data + LINEAR_REGION));
    pq_hal_set_linear_en(GET_PARAM(pq_data + LINEAR_REGION_EN));

    if (!ins->is_option)
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

    if (!ins->is_option)
        pq_configure_invgamma(ins);

    pq_configure_gamma(ins);
}


static uint32_t get_param_from_dts(const char *path)
{
	int fd = open(path, O_RDONLY);
	uint32_t value = 0;;
	if(fd >= 0){
		uint8_t buf[4];
		if(read(fd, buf, 4) != 4){
			close(fd);
			return value;
		}
		close(fd);
		value = (buf[0] & 0xff) << 24 | (buf[1] & 0xff) << 16 | (buf[2] & 0xff) << 8 | (buf[3] & 0xff);
	}
	return value;
}

static void __pq_start(void)
{
#define SYS_VIDEO_SRC_CTRL_BASE 0xb8800000
#if 1
#define SYS_VIDEO_SRC_CTRL_VAL  (*(volatile uint32_t *)(reg_base + 0x444))
#define SYS_VIDEO_SRC_CTRL_VAL1 (*(volatile uint32_t *)(reg_base + 0x448))
#define SYS_VIDEO_SRC_CTRL_VAL2 (*(volatile uint32_t *)(reg_base + 0x44c))
#else
#define SYS_VIDEO_SRC_CTRL_VAL (*(volatile unsigned long *)0xb8800444)
#define SYS_VIDEO_SRC_CTRL_VAL1 (*(volatile unsigned long *)0xb8800448)
#define SYS_VIDEO_SRC_CTRL_VAL2 (*(volatile unsigned long *)0xb880044c)
#endif

#define E_SRC_SEL_HDMI_PQ (0x3)
#define E_SRC_SEL_FXDE (0x0)
#define E_SRC_SEL_4KDE (0x1)

    int fd;
    uint32_t de4k;
    uint8_t *reg_base;
    bool g_b_rgb_high_to_low[3] = { false, false, false };
    int rgb_r_lowhigh_source_sel = 0;
    int rgb_g_lowhigh_source_sel = 0;
    int rgb_b_lowhigh_source_sel = 0;


    fd = open("/dev/mem", O_RDWR | O_NDELAY);
    if (fd < 0) {
        printf("open(/dev/mem) failed.");
        return ;
    }
    
    reg_base = (uint8_t *)mmap(NULL, 0x500,  PROT_READ | PROT_WRITE,
                        MAP_SHARED, fd, SYS_VIDEO_SRC_CTRL_BASE);
    if(reg_base == NULL){
        printf("cannot mmap for SYS_VIDEO_SRC_CTRL_BASE\n");
        return;
    }

    // printf("before pq start...\n");
    // printf("==============================\n");
    // printf("reg0xb8800444: 0x%8.8lx\n", SYS_VIDEO_SRC_CTRL_VAL);
    // printf("reg0xb8800448: 0x%8.8lx\n", SYS_VIDEO_SRC_CTRL_VAL1);
    // printf("reg0xb880044c: 0x%8.8lx\n", SYS_VIDEO_SRC_CTRL_VAL2);
    // printf("==============================\n");    

    de4k = get_param_from_dts("/proc/device-tree/hcrtos/de-engine/de4k-output");

    rgb_r_lowhigh_source_sel = (SYS_VIDEO_SRC_CTRL_VAL2 >> 0) & 0x7;
    rgb_g_lowhigh_source_sel = (SYS_VIDEO_SRC_CTRL_VAL2 >> 4) & 0x7;
    rgb_b_lowhigh_source_sel = (SYS_VIDEO_SRC_CTRL_VAL2 >> 8) & 0x7;

    if ((rgb_r_lowhigh_source_sel >= 0 && rgb_r_lowhigh_source_sel <= 3) ||
        ((SYS_VIDEO_SRC_CTRL_VAL2 >> 28) & 0x1 == 1)) {
        g_b_rgb_high_to_low[0] = true;
    }

    if ((rgb_g_lowhigh_source_sel >= 0 && rgb_g_lowhigh_source_sel <= 3) ||
        ((SYS_VIDEO_SRC_CTRL_VAL2 >> 29) & 0x1 == 1)) {
        g_b_rgb_high_to_low[1] = true;
    }

    if ((rgb_g_lowhigh_source_sel >= 0 && rgb_g_lowhigh_source_sel <= 3) ||
        ((SYS_VIDEO_SRC_CTRL_VAL2 >> 30) & 0x1 == 1)) {
        g_b_rgb_high_to_low[2] = true;
    }

    SYS_VIDEO_SRC_CTRL_VAL &= ~(0x3 << 20); //PQ CLK SRC
    SYS_VIDEO_SRC_CTRL_VAL1 &= ~(0x3 << 4); //PQ DATA SRC
    SYS_VIDEO_SRC_CTRL_VAL1 &= ~(0x3 << 0); //HDMI TX SRC
    SYS_VIDEO_SRC_CTRL_VAL1 &= ~(0xF << 12); //LVDS SRC
    SYS_VIDEO_SRC_CTRL_VAL1 &= ~(0x3 << 16); //MIPI SRC
    SYS_VIDEO_SRC_CTRL_VAL1 &= ~(0x3 << 18); //RGB SRC
    SYS_VIDEO_SRC_CTRL_VAL2 &= ~(0x70000777); //RGB DATA SRC

    if (de4k == 0) {
        SYS_VIDEO_SRC_CTRL_VAL |= (E_SRC_SEL_FXDE << 20); //PQ CLK SRC
        SYS_VIDEO_SRC_CTRL_VAL1 |= (E_SRC_SEL_FXDE << 4); //PQ DATA SRC
    } else {
        SYS_VIDEO_SRC_CTRL_VAL |= (E_SRC_SEL_4KDE << 20); //PQ CLK SRC
        SYS_VIDEO_SRC_CTRL_VAL1 |= (E_SRC_SEL_4KDE << 4); //PQ DATA SRC
    }

    SYS_VIDEO_SRC_CTRL_VAL1 |= (E_SRC_SEL_HDMI_PQ << 0); //HDMI TX SRC
    SYS_VIDEO_SRC_CTRL_VAL1 |= (E_SRC_SEL_HDMI_PQ << 12); //LVDS SRC
    SYS_VIDEO_SRC_CTRL_VAL1 |= (E_SRC_SEL_HDMI_PQ << 14); //LVDS SRC
    SYS_VIDEO_SRC_CTRL_VAL1 |= (E_SRC_SEL_HDMI_PQ << 16); //MIPI SRC
    SYS_VIDEO_SRC_CTRL_VAL1 |= (E_SRC_SEL_HDMI_PQ << 18); //RGB SRC

    SYS_VIDEO_SRC_CTRL_VAL2 |= (0x0000777);
    SYS_VIDEO_SRC_CTRL_VAL2 |= g_b_rgb_high_to_low[0] << 28;
    SYS_VIDEO_SRC_CTRL_VAL2 |= g_b_rgb_high_to_low[1] << 29;
    SYS_VIDEO_SRC_CTRL_VAL2 |= g_b_rgb_high_to_low[2] << 30;

    // printf("pq start...\n");
    // printf("==============================\n");
    // printf("reg0xb8800444: 0x%8.8lx\n", SYS_VIDEO_SRC_CTRL_VAL);
    // printf("reg0xb8800448: 0x%8.8lx\n", SYS_VIDEO_SRC_CTRL_VAL1);
    // printf("reg0xb880044c: 0x%8.8lx\n", SYS_VIDEO_SRC_CTRL_VAL2);
    // printf("==============================\n");

    munmap(reg_base, 0x500);
    close(fd);
}

static int pq_tool_init(pq_app_t *ins)
{
    const char *path = NULL;
    int np;

    memset(ins, 0, sizeof(pq_app_t));

    /* setup PQ-tool environment */
    ins->mem_fd = open("/dev/mem", O_RDWR | O_NDELAY);
    if (ins->mem_fd < 0) {
        printf("open(/dev/mem) failed.");
        goto __exit1;
    }

    ins->uart_fd = open("/dev/tty", O_RDWR | O_SYNC);
    // ins->uart_fd = open("/dev/ttyGS0", O_RDWR);
    if (ins->uart_fd < 0) {
        printf("open(/dev/tty) failed.");
        goto __exit1;
    }

    tcflush(ins->uart_fd, TCIOFLUSH);
    tty_raw(ins->uart_fd);

    /* mmap for PQ regsiter mapping */
    ins->pq_reg_map_base = (uint8_t *)mmap(NULL,
                        MAP_SIZE, PROT_READ | PROT_WRITE,
                        MAP_SHARED, ins->mem_fd, PQ_REG_BASE);


    ins->pq_data = malloc(PQ_BUF_SIZE);
    if (ins->pq_data == NULL) {
        printf("cannot malloc enough memory for PQ data buffer\n");
        return -1;
    }
    memset((uint8_t *)ins->pq_data, 0, PQ_BUF_SIZE);

    /* close STDOUT and STDIN */
    // fclose(stdout);
    // fclose(stdin);
    
    /* setup pq module driver */
    __pq_start();

    return 0;

__exit1:
    if (ins->uart_fd >= 0) 
        close(ins->uart_fd);
    if(ins->mem_fd >= 0)
        close(ins->mem_fd);

    return -1; 
}

#if 0
static size_t console_receive(int fd, char *buf, size_t len, int wait_ms,
            int timeout_ms)
{
    struct pollfd fds[1];
    int r;
    char *p_buf = buf;
    size_t rd_cnt = 0;

    if(fd < 0)
        return 0;

    fds[0].fd = (int)fd;
    fds[0].events = POLLIN | POLLRDNORM;

    r = poll(fds, 1, wait_ms);
    if (r <= 0)
        return 0;

    while (1) {
        read(fd, p_buf++, 1);
        if(++rd_cnt >= len)
            return rd_cnt;

        r = poll(fds, 1, timeout_ms);
        if (r <= 0) {
            break;
        }
    }
    return rd_cnt;
}
#else
static size_t console_receive(int fd, char *buf, size_t len, int wait_ms,
            int timeout_ms)
{
    int total = 0;
    int rd;

    while(1){
        rd = read(fd, buf + total, len);
        if( rd > 0){
            // dump_pq_data(buf, rd);
            total += rd;
            // printf("===> read %u, total:%u\n", rd, total);
            usleep(1000 * 10);
        }else{
            // printf("===> read finish\n");
            break;
        }

        // !note: 这里是预计所传输的数量大于1280, 之后需要把这一块做完善
        if(total >= 1280) 
            break;
    }

    return total;
}
#endif

uint8_t g_debug_buf[4096] = {0};
uint32_t g_debug_index = 0;
uint32_t g_debug_size[10] = {0};

static size_t console_send(int fd, char *buf, size_t len)
{
    return write(fd, buf, len);
}

static int pq_tool_receive(pq_app_t *ins)
{
    uint32_t recv_total;
    uint8_t *p_buf = (uint8_t *)ins->pq_data;
    uint8_t feedback[8] = {0};

    g_debug_index++;
    g_debug_size[0] = g_debug_index;
    g_debug_size[g_debug_index] = 0;

    memset(p_buf, 0, PQ_BUF_SIZE);
    ins->pq_data_length = console_receive(ins->uart_fd, 
                            (uint8_t *)ins->pq_data,
                            PQ_BUF_SIZE,
                            -1, 1000);
    if(ins->pq_data_length == PQ_DATA_LEN){
    /*  note: this is old version PQ-tool, not support data validation */
        g_debug_size[g_debug_index] = ins->pq_data_length;
        return 0;
    }else{
        printf("--> error receive : %u\n", ins->pq_data_length);
        return -1;
    }
    #if 0
    else if (recv_bytes != 8){
        return -1;
    }

    /* note: this is new version PQ-tool, support data validation */
    /* step1: */
    console_send(ins->uart_fd,
            (uint8_t *)ins->pq_data,
            recv_bytes);
    
    /* step2: */
    recv_total = 0;
    memset(p_buf, 0, PQ_BUF_SIZE);
    while(1){
        recv_bytes = console_receive(ins->uart_fd, 
                                p_buf,
                                PQ_BUF_SIZE,
                                -1, 2000);
        if(recv_bytes == 0)
            break;

        p_buf += recv_bytes;
        recv_total += recv_bytes;
        g_debug_size[g_debug_index] = recv_total;

        memset(&feedback[0], 0, 8);
        #if 1
        feedback[7] = recv_bytes & 0xff;
        feedback[5] = (recv_bytes >> 16) & 0xff;
        feedback[4] = (recv_bytes >> 24) & 0xff;
        #else
        feedback[0] = recv_bytes & 0xff;
        feedback[1] = (recv_bytes >> 8) & 0xff;
        feedback[2] = (recv_bytes >> 16) & 0xff;
        feedback[3] = (recv_bytes >> 24) & 0xff;
        #endif
    console_send(ins ins->pq_data_length>uart_fd,
                &feedback[0],
                8);
        
        if(ins->pq_data_length == PQ_DATA_LEN)
            break;
    }

    memcpy(g_debug_buf,ins->pq_data_length->pq_data, recv_total);

    if(recv_total == PQ_DATA_LEN)
        ins->pq_data_length0;
    else
        return -1;
    #endif
}

#if 1
static void __pq_tool(void *p)
{
    pq_app_t pq_ins;
    pq_app_t *ins = &pq_ins;
    int ret, pq_fd;

    if (pq_tool_init(ins)) {
        printf("[Error] pq_tool init fail !!!\n");
        return;
    }

    while (1) {
        if(pq_tool_receive(ins))
            continue;

        pq_tool_dump(ins);
        pq_tool_configure(ins);
    }

    printf("PQ tool exit....\n");
}
#else
static void __pq_tool(void *p)
{
    pq_app_t pq_ins;
    pq_app_t *ins = &pq_ins;
    int ret, pq_fd;
    size_t recv_total;
    uint8_t feedback[8] = {0};

    if (pq_tool_init(ins)) {
        printf("[Error] pq_tool init fail !!!\n");
        return;
    }
    while(1){
        memset(&feedback[0], 0, 8);
        feedback[0] = 'H';
        feedback[1] = 'C';
        feedback[2] = 'P';
        feedback[3] = 'Q';
        console_send(ins->uart_fd,
                &feedback[0],
                8);
        sleep(1);

        recv_total = 400;
        memset(&feedback[0], 0, 8);
        feedback[7] = recv_total & 0xff;
        feedback[6] = (recv_total >> 8) & 0xff;
        feedback[5] = (recv_total >> 16) & 0xff;
        feedback[4] = (recv_total >> 24) & 0xff;
        console_send(ins->uart_fd,
                &feedback[0],
                8);
        sleep(1);

        recv_total = 400;
        memset(&feedback[0], 0, 8);
        feedback[7] = recv_total & 0xff;
        feedback[6] = (recv_total >> 8) & 0xff;
        feedback[5] = (recv_total >> 16) & 0xff;
        feedback[4] = (recv_total >> 24) & 0xff;
        console_send(ins->uart_fd,
                &feedback[0],
                8);
        sleep(1);

        recv_total = 400;
        memset(&feedback[0], 0, 8);
        feedback[7] = recv_total & 0xff;
        feedback[6] = (recv_total >> 8) & 0xff;
        feedback[5] = (recv_total >> 16) & 0xff;
        feedback[4] = (recv_total >> 24) & 0xff;
        console_send(ins->uart_fd,
                &feedback[0],
                8);
        sleep(1);

        recv_total = 80;
        memset(&feedback[0], 0, 8);
        feedback[7] = recv_total & 0xff;
        feedback[6] = (recv_total >> 8) & 0xff;
        feedback[5] = (recv_total >> 16) & 0xff;
        feedback[4] = (recv_total >> 24) & 0xff;
        console_send(ins->uart_fd,
                &feedback[0],
                8);
        sleep(1);
    }
}
#endif

int main(int argc, char **argv)
{
    int c, ret;

    while ((c = getopt(argc, argv, "hd")) != -1) {
        switch (c) {
        case 'd':
            g_is_debug = true;
            break;
        case 'h':
            printf("\nPQ tool -- complierd at %s %s\n", __DATE__,
                   __TIME__);
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

    printf("pq-tool: console task will be halted, you should use hichip pq_tool to send pq configuration date\n");

    __pq_tool(NULL);

    while (1)
        sleep(100);

    printf("pq-tool: exit...\n");
    return 0;
}

// ///////////////////////////////////////////////////////////////////////////

// int tt_test(int argc, char **argv)
// {
//     int fd;
//     const char *path = NULL;
//     int np;
//     size_t index;
//     size_t rd_cnt;
//     char buf[100] = { 0 };
//     char *p_buf = &buf[0];
//     struct pollfd fds[2];
//     int r;

//     printf("---> %s:line-%u\n", __FUNCTION__, __LINE__);

//     np = fdt_get_node_offset_by_path("/hcrtos/stdio");
//     assert(np >= 0);
//     assert(fdt_get_property_string_index(np, "serial0", 0, &path) == 0);
//     assert(path != NULL);
//     np = fdt_node_probe_by_path(path);
//     if (np < 0)
//         np = fdt_node_probe_by_path("/hcrtos/uart_dummy");
//     assert(np >= 0);
//     assert(fdt_get_property_string_index(np, "devpath", 0, &path) == 0);

//     printf("uart console path : %s\n", path);

//     fd = open(path, O_RDWR);
//     if (fd < 0)
//         assert(0);

//     write(fd, "hello world\n", sizeof("hello world\n"));

//     printf("--> wait receive...\n");

//     fds[0].fd = (int)fd;
//     fds[0].events = POLLIN | POLLRDNORM;

//     r = poll(fds, 1, -1);
//     read(fd, p_buf++, 1);

//     printf("--> try to received !!!\n");

//     while (1) {
//         r = poll(fds, 1, 100);
//         if (r <= 0) {
//             printf("...\n");
//             break;
//         }
//         read(fd, p_buf++, 1);
//         printf("++++\n");
//     }

//     rd_cnt = (size_t)(p_buf - &buf[0]);
//     if (rd_cnt) {
//         printf("dump: \n");
//         for (index = 0; index < rd_cnt; index++) {
//             printf("%2.2x ", buf[index]);
//             if (index && (index % 16 == 0))
//                 printf("\n");
//         }
//         printf("\n");
//     }

//     close(fd);

//     return 0;
// }

// CONSOLE_CMD(tt_test, NULL, tt_test, CONSOLE_CMD_MODE_SELF,
//         "pq configuration tool : tt_test")