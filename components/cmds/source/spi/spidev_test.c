#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <hcuapi/spidev.h>
#include <kernel/lib/console.h>

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

#if defined TEST_SPI_SF
static const char *device = "/dev/spidev0";
#elif defined TEST_SPI_MASTER
static const char *device = "/dev/spidev1";
#elif defined TEST_SPI_GPIO
static const char *device = "/dev/spidev2";
#endif

static uint32_t mode;
static uint8_t bits = 8;
static uint32_t speed = 500000;
static uint16_t delay;
static int verbose;
static char flash_name[128];
static uint32_t jedec_id = 0;

#define NONE -1

struct flash_type {
	char name[128];
	uint32_t id;
	int8_t read_id_cmd;		int8_t read_id_dummy_num; 		uint32_t id_len;
	int8_t read_unique_id_cmd;	int8_t read_unique_id_dummy_num; 	uint32_t unique_id_len;	int32_t uniqueid_read_start_addr;
	int8_t read_otp_cmd;		int8_t read_otp_dummy_num;		uint32_t otp_len;	int32_t otp_read_start_addr;
	int8_t enter_otp_mode_cmd;	int8_t exit_otp_mode_cmd;
};

struct flash_type FLASH_TYPE[] = {
	{
		"BY25Q40AV",
		0xef4018,
		0x9f, 0, 0x03,
		0x4b, 4, 16, NONE,
		0x48, 1, 16,0x1000,
		NONE, NONE,
	},

	{
		"w25q256",
		0xef4019,
		0x9f, 0, 0x03,
		0x4b, 4, 16, NONE,
		0x48, 1, 16,0x1000,
		NONE, NONE,
	},

	{
		"MX25L25645G",
		0xc22019,
		0x9f, 0, 0x03,
		0x5a, 1, 16, 0x01e0,
		0x03, 0, 16, 0x0000,
		0xb1, 0xc1,
	},

	{
		"w25q128",
		0xef4018,
		0x9f, 0, 0x03,
		0x4b, 4, 16, NONE,
		0x48, 1, 16,0x1000,
		NONE, NONE,
	},

	{
		"w25q64",
		0xef4017,
		0x9f, 0, 0x03,
		0x4b, 4, 16, NONE,
		0x48, 1, 16,0x1000,
		NONE, NONE,
	},

	{
		"w25q16",
		0xef4015,
		0x9f, 0, 0x03,
		0x4b, 4, 16, NONE,
		0x48, 1, 16,0x1000,
		NONE, NONE,
	},

	{
		"XM25QH128A",
		0x207018,
		0x9f, 0, 0x03,
		0x5a, 1, 16, 0x80, 
		0x03, 0, 16, 0xFFF000,
		0x3a, NONE,
	},

	{
  		"XM25QH32C",
		0x204016,
		0x9f, 0, 0x03,
		0x4b, 4, 8, NONE,
		0x48, 1, 32, 0x1000,
		NONE, NONE,
	},
};

uint8_t default_tx[] = {
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0x40, 0x00, 0x00, 0x00, 0x00, 0x95,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xF0, 0x0D,
};

uint8_t default_rx[ARRAY_SIZE(default_tx)] = {0, };
char *input_tx;

static void hex_dump(const void *src, size_t length, size_t line_size, char *prefix)
{
	int i = 0;
	const unsigned char *address = src;
	const unsigned char *line = address;
	unsigned char c;

	printf("%s | ", prefix);
	while (length-- > 0) {
		printf("%02X ", *address++);
		if (!(++i % line_size) || (length == 0 && i % line_size)) {
			if (length == 0) {
				while (i++ % line_size)
					printf("__ ");
			}
			printf(" | ");  /* right close */
			while (line < address) {
				c = *line++;
				printf("%c", (c < 33 || c == 255) ? 0x2E : c);
			}
			printf("\n");
			if (length > 0)
				printf("%s | ", prefix);
		}
	}
}

/*
 *  Unescape - process hexadecimal escape character
 *      converts shell input "\x23" -> 0x23
 */
static int unescape(char *_dst, char *_src, size_t len)
{
	int ret = 0;
	char *src = _src;
	char *dst = _dst;
	unsigned int ch;

	while (*src) {
		if (*src == '\\' && *(src+1) == 'x') {
			sscanf(src + 2, "%2x", &ch);
			src += 4;
			*dst++ = (unsigned char)ch;
		} else {
			*dst++ = *src++;
		}
		ret++;
	}
	return ret;
}

static void transfer(int fd, uint8_t const *tx, uint8_t const *rx, size_t len)
{
	int ret;

	struct spi_ioc_transfer tr = {
		.tx_buf = (unsigned long)tx,
		.rx_buf = (unsigned long)rx,
		.len = len,
		.delay_usecs = delay,
		.speed_hz = speed,
		.bits_per_word = bits,
	};

	if (mode & SPI_TX_QUAD)
		tr.tx_nbits = 4;
	else if (mode & SPI_TX_DUAL)
		tr.tx_nbits = 2;
	if (mode & SPI_RX_QUAD)
		tr.rx_nbits = 4;
	else if (mode & SPI_RX_DUAL)
		tr.rx_nbits = 2;
	if (!(mode & SPI_LOOP)) {
		if (mode & (SPI_TX_QUAD | SPI_TX_DUAL))
			tr.rx_buf = 0;
		else if (mode & (SPI_RX_QUAD | SPI_RX_DUAL))
			tr.tx_buf = 0;
	}

	ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
	if (ret < 1) {
		printf("can't send spi message");
		return;
	}

	if (verbose)
		hex_dump(tx, len, 32, "TX");
	printf("\r\n");
	hex_dump(rx, len, 32, "RX");
}

static void print_usage(const char *prog)
{
	printf("Usage: %s [-DsbdlHOLC3]\n", prog);
	puts("  -D --device   device to use (default /dev/spidev0)\n"
	     "  -s --speed    max speed (Hz)\n"
	     "  -d --delay    delay (usec)\n"
	     "  -b --bpw      bits per word \n"
	     "  -l --loop     loopback\n"
	     "  -H --cpha     clock phase\n"
	     "  -O --cpol     clock polarity\n"
	     "  -L --lsb      least significant bit first\n"
	     "  -C --cs-high  chip select active high\n"
	     "  -3 --3wire    SI/SO signals shared\n"
	     "  -v --verbose  Verbose (show tx buffer)\n"
	     "  -p            Send data (e.g. \"1234\\xde\\xad\")\n"
	     "  -N --no-cs    no chip select\n"
	     "  -R --ready    slave pulls low to pause\n"
	     "  -2 --dual     dual transfer\n"
	     "  -4 --quad     quad transfer\n");
}

static int parse_opts(int argc, char *argv[])
{
	while (1) {
		static const struct option lopts[] = {
			{ "device",  1, 0, 'D' },
			{ "speed",   1, 0, 's' },
			{ "delay",   1, 0, 'd' },
			{ "bpw",     1, 0, 'b' },
			{ "loop",    0, 0, 'l' },
			{ "cpha",    0, 0, 'H' },
			{ "cpol",    0, 0, 'O' },
			{ "lsb",     0, 0, 'L' },
			{ "cs-high", 0, 0, 'C' },
			{ "3wire",   0, 0, '3' },
			{ "no-cs",   0, 0, 'N' },
			{ "ready",   0, 0, 'R' },
			{ "dual",    0, 0, '2' },
			{ "verbose", 0, 0, 'v' },
			{ "quad",    0, 0, '4' },
			{ NULL, 0, 0, 0 },
		};
		int c;

		c = getopt_long(argc, argv, "D:s:d:b:lHOLC3NR24p:v", lopts, NULL);

		if (c == -1)
			break;

		switch (c) {
		case 'D':
			device = optarg;
			break;
		case 's':
			speed = atoi(optarg);
			break;
		case 'd':
			delay = atoi(optarg);
			break;
		case 'b':
			bits = atoi(optarg);
			break;
		case 'l':
			mode |= SPI_LOOP;
			break;
		case 'H':
			mode |= SPI_CPHA;
			break;
		case 'O':
			mode |= SPI_CPOL;
			break;
		case 'L':
			mode |= SPI_LSB_FIRST;
			break;
		case 'C':
			mode |= SPI_CS_HIGH;
			break;
		case '3':
			mode |= SPI_3WIRE;
			break;
		case 'N':
			mode |= SPI_NO_CS;
			break;
		case 'v':
			verbose = 1;
			break;
		case 'R':
			mode |= SPI_READY;
			break;
		case 'p':
			input_tx = optarg;
			break;
		case '2':
			mode |= SPI_TX_DUAL;
			break;
		case '4':
			mode |= SPI_TX_QUAD;
			break;
		default:
			print_usage(argv[0]);
			return -1;
		}
	}
	if (mode & SPI_LOOP) {
		if (mode & SPI_TX_DUAL)
			mode |= SPI_RX_DUAL;
		if (mode & SPI_TX_QUAD)
			mode |= SPI_RX_QUAD;
	}

	return 0;
}

int spi_cmds_spidev_test(int argc, char *argv[])
{
	int ret = 0;
	int fd;
	uint8_t *tx;
	uint8_t *rx;
	int size;

	opterr = 0;
	optind = 0;
	ret = parse_opts(argc, argv);
	if (ret)
		return -1;

	fd = open(device, O_RDWR);
	if (fd < 0) {
		printf("can't open device");
		return -1;
	}

	/*
	 * spi mode
	 */
	ret = ioctl(fd, SPI_IOC_WR_MODE32, &mode);
	if (ret == -1) {
		printf("can't set spi mode");
		return -1;
	}

	ret = ioctl(fd, SPI_IOC_RD_MODE32, &mode);
	if (ret == -1) {
		printf("can't get spi mode");
		return -1;
	}

	/*
	 * bits per word
	 */
	ret = ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
	if (ret == -1) {
		printf("can't set bits per word");
		return -1;
	}

	ret = ioctl(fd, SPI_IOC_RD_BITS_PER_WORD, &bits);
	if (ret == -1) {
		printf("can't get bits per word");
		return -1;
	}

	/*
	 * max speed hz
	 */
	ret = ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
	if (ret == -1) {
		printf("can't set max speed hz");
		return -1;
	}

	ret = ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed);
	if (ret == -1) {
		printf("can't get max speed hz");
		return -1;
	}

	printf("spi mode: 0x%lx\n", mode);
	printf("bits per word: %d\n", bits);
	printf("max speed: %ld Hz (%ld KHz)\n", speed, speed/1000);

	close(fd);
	return 0;

	if (input_tx) {
		size = strlen(input_tx+1);
		tx = malloc(size);
		rx = malloc(size);
		size = unescape((char *)tx, input_tx, size);
		transfer(fd, tx, rx, size);
		free(rx);
		free(tx);
	} else {
		transfer(fd, default_tx, default_rx, sizeof(default_tx));
	}

	close(fd);

	return ret;
}

int get_flash_id_param(char flash_name[])
{
	int number = 0;
	int flash_number = 0;
	flash_number = sizeof(FLASH_TYPE) / sizeof(struct flash_type);
	for (number = 0; number < flash_number; number++)
		if (strcmp(flash_name, FLASH_TYPE[number].name) == 0)
			return number;

	printf("Flash of this model is not supported: %s\n", flash_name);
	return -1;
}

int hc_norflash_read_id(uint8_t cmd, uint8_t dummy_num, uint8_t *data,
		        uint32_t len)
{
	int ret, fd;
	uint8_t tx[10] = {0};
	uint8_t *rx = data;
	tx[0] = cmd;
	fd = open(device, O_RDWR);
	if (fd < 0) {
		printf("can't open device");
		return -1;
	}

	struct spi_ioc_transfer xfer[2] = {
		{
			.tx_buf = (unsigned long)tx,
			.rx_buf = (unsigned long)NULL,
			.len = 1+dummy_num,
			.delay_usecs = delay,
			.speed_hz = speed,
			.bits_per_word = bits,
		},{
			.tx_buf = (unsigned long)NULL,
			.rx_buf = (unsigned long)rx,
			.len = len,
			.delay_usecs = delay,
			.speed_hz = speed,
			.bits_per_word = bits,
		}
	};

	ret = ioctl(fd, SPI_IOC_MESSAGE(2), &xfer);
	if (ret < 1) {
		printf("can't send  spi message");
		return -1;
	}

	close(fd);
	return 0;
}

int hc_norflash_read_otp_data(uint8_t cmd, uint32_t addr, uint8_t dummy_num,
			      uint8_t *data,uint32_t len)
{
	int ret, fd;
	uint8_t tx[10] = {0};
	uint8_t *rx = data;
	tx[0] = cmd;
	tx[1] = ((addr >> 16) & 0xff);
	tx[2] = ((addr >> 8)  & 0xff);
	tx[3] = (addr & 0xff);

	fd = open(device, O_RDWR);
	if (fd < 0) {
		printf("can't open device");
		return -1;
	}

	struct spi_ioc_transfer xfer[2] = {
		{
			.tx_buf = (unsigned long)tx,
			.rx_buf = (unsigned long)NULL,
			.len = 4+dummy_num,
			.delay_usecs = delay,
			.speed_hz = speed,
			.bits_per_word = bits,
		},{
			.tx_buf = (unsigned long)NULL,
			.rx_buf = (unsigned long)rx,
			.len = len,
			.delay_usecs = delay,
			.speed_hz = speed,
			.bits_per_word = bits,
		}
	};

	ret = ioctl(fd, SPI_IOC_MESSAGE(2), &xfer);
	if (ret < 1) {
		printf("can't send  spi message");
		return -1;
	}

	close(fd);
	return 0;
}

void hc_enter_otp_mode(struct flash_type FLASH_TYPE)
{
        int ret, fd;
	uint8_t tx[10] = {0};
	tx[0] = FLASH_TYPE.enter_otp_mode_cmd;
	fd = open(device, O_RDWR);
	if (fd < 0) {
		printf("can't open device");
		return;
	}

	struct spi_ioc_transfer xfer[1] = {
		{
			.tx_buf = (unsigned long)tx,
			.rx_buf = (unsigned long)NULL,
			.len = 1,
			.delay_usecs = delay,
			.speed_hz = speed,
			.bits_per_word = bits,
		}
        };

	ret = ioctl(fd, SPI_IOC_MESSAGE(1), &xfer);
	if (ret < 1) {
		printf("can't send  spi message");
		return;
	}

	close(fd);

        return;
}

void hc_exit_otp_mode(struct flash_type FLASH_TYPE)
{
        int ret, fd;
	uint8_t tx[10] = {0};
	tx[0] = FLASH_TYPE.exit_otp_mode_cmd;
	fd = open(device, O_RDWR);
	if (fd < 0) {
		printf("can't open device");
		return;
	}

	struct spi_ioc_transfer xfer[1] = {
		{
			.tx_buf = (unsigned long)tx,
			.rx_buf = (unsigned long)NULL,
			.len = 1,
			.delay_usecs = delay,
			.speed_hz = speed,
			.bits_per_word = bits,
		}
        };

	ret = ioctl(fd, SPI_IOC_MESSAGE(1), &xfer);
	if (ret < 1) {
		printf("can't send  spi message");
		return;
	}

	close(fd);

        return;
}

int nor_flash_read_OTP(char flash_name[], uint8_t*otp_data)
{
	uint8_t i;
	int number = 0;

        if(flash_name == NULL){
                printf("err: flash_name is null, unkown jedec_id, jedec_id=%d\n", jedec_id);
                return -1;
        }

	number = get_flash_id_param(flash_name);

	if (FLASH_TYPE[number].enter_otp_mode_cmd != NONE)
		hc_enter_otp_mode(FLASH_TYPE[number]);

	hc_norflash_read_otp_data(FLASH_TYPE[number].read_otp_cmd,
				  FLASH_TYPE[number].otp_read_start_addr,
				  FLASH_TYPE[number].read_otp_dummy_num,
				  otp_data, FLASH_TYPE[number].otp_len);

	if (FLASH_TYPE[number].exit_otp_mode_cmd != NONE)
		hc_exit_otp_mode(FLASH_TYPE[number]);

	printf("OTP Date:");
	for (i = 0; i < FLASH_TYPE[number].otp_len; i++) {
		printf("0x%2x ", otp_data[i]);
	}
	printf("\n");

	return 0;
}


int nor_flash_read_Unique_ID(char flash_name[], uint8_t *unique_id)
{
	uint8_t i;
	int number = 0;

        if(flash_name == NULL){
                printf("err: flash_name is null, unkown jedec_id, jedec_id=%d\n", jedec_id);
                return -1;
        }

	number = get_flash_id_param(flash_name);

	if (FLASH_TYPE[number].uniqueid_read_start_addr == NONE) {
		hc_norflash_read_id(FLASH_TYPE[number].read_unique_id_cmd,   
			     	    FLASH_TYPE[number].read_unique_id_dummy_num, 
				    unique_id, FLASH_TYPE[number].unique_id_len);
	} else {
		hc_norflash_read_otp_data(FLASH_TYPE[number].read_unique_id_cmd,
					  FLASH_TYPE[number].uniqueid_read_start_addr,	
					  FLASH_TYPE[number].read_unique_id_dummy_num,
					  unique_id, FLASH_TYPE[number].unique_id_len);
	}

	printf("Unique ID:");
	for (i = 0; i < FLASH_TYPE[number].unique_id_len; i++) {
		printf("0x%2x ", unique_id[i]);
	}
	printf("\n");
	return 0;
}

int nor_flash_read_ID(char flash_name[], uint8_t *id)
{
	uint8_t i;
	int number = 0;

        if(flash_name == NULL){
                printf("err: flash_name is null, unkown jedec_id, jedec_id=%d\n", jedec_id);
                return -1;
        }

	number = get_flash_id_param(flash_name);
	hc_norflash_read_id(FLASH_TYPE[number].read_id_cmd,
			FLASH_TYPE[number].read_id_dummy_num, id,
			FLASH_TYPE[number].id_len);

	printf("FLASH ID:");
	for (i = 0; i < FLASH_TYPE[number].id_len; i++) {
		printf("0x%2x ", id[i]);
	}
	printf("\n");
	return 0;
}

int read_nor_flash_ID_test(int argc, char **argv)
{
	uint8_t flash_id[16] = {0};
	nor_flash_read_ID(flash_name, flash_id);
	return 0;
}

int read_nor_flash_UniqueID_test(int argc, char **argv)
{
	uint8_t flash_unique_id[16] = {0};
	nor_flash_read_Unique_ID(flash_name, flash_unique_id);
	return 0;
}

int read_nor_flash_OTP_data_test(int argc, char **argv)
{
	uint8_t flash_otp_data[128] = {0};
	nor_flash_read_OTP(flash_name, flash_otp_data);
	return 0;
}

static int spi_cmds(int argc, char **argv)
{
        unsigned char id[6] = {0};
        int ret = 0;
        int number = 0;
        int flash_number = 0;

        memset(flash_name, 0, sizeof(flash_name));

        ret = hc_norflash_read_id(0x9f, 0, id, 3);
        if(ret != 0){
                printf("err: hc_norflash_read_id failed\n");
                jedec_id = 0;
                return -1;
        }else{
                jedec_id = (id[0] << 16) | (id[1] << 8) | (id[2] << 0);
        }
        printf("jedec_id=0x%x\n", jedec_id);

        flash_number = sizeof(FLASH_TYPE) / sizeof(struct flash_type);
        for (number = 0; number < flash_number; number++) {
                if (jedec_id == FLASH_TYPE[number].id) {
                        strcpy(flash_name, FLASH_TYPE[number].name);
                }
        }
        printf("flash_name=%s\n", flash_name);

        return 0;
}

CONSOLE_CMD(spi, NULL, spi_cmds, CONSOLE_CMD_MODE_SELF,
	    "spi commands")

CONSOLE_CMD(spidev_test, "spi", spi_cmds_spidev_test, CONSOLE_CMD_MODE_SELF,
	    "spidev_test commands")

CONSOLE_CMD(read_otp, "spi", read_nor_flash_OTP_data_test,
		CONSOLE_CMD_MODE_SELF, "read  nor flash OTP")

CONSOLE_CMD(read_unique_id, "spi", read_nor_flash_UniqueID_test,
		CONSOLE_CMD_MODE_SELF, "read  nor flash Unique ID")

CONSOLE_CMD(read_jedec_id, "spi", read_nor_flash_ID_test,
		CONSOLE_CMD_MODE_SELF, "read  nor flash ID")

