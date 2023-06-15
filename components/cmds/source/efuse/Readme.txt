/*
** hichip efuse 使用说明以及示例 : 
**  0. 使用hichip efuse驱动: make linux-menuconfig, 把 HC_EFUSE 打开即可
**
**  1. hichip efuse 总共有256bits, 共分为8个zone, 以及对应8个bit的保护位
**     * 初始默认值都为0, 而每个bit只能从0烧写为1,无法由1恢复为0;
**     * 其中最后8个bits为保护位, 烧写为1后, 对应zone里面的所有bit都无法再次被烧写
**     * 具体的bit map请见下面, 或者 <hcuapi/efuse.h>
**
**  2. 烧写/读取的API使用例子如下所示.
**      * 可以通过 write 或者 ioctl CMD (EFUSE_PROGRAM) 实现烧写, 为了避免误操作,
**        内部驱动限定了write 时所传入的长度必须为 sizeof(struct hc_efuse_bit_map)
**      * 可以通过 read 或者 ioctl CMD (EFUSE_DUMP) 实现读, 为了避免误操作,
**        内部驱动限定了read 时所传入的长度必须为 sizeof(struct hc_efuse_bit_map),
**        其中 ioctl CMD (EFUSE_DUMP) 会主动使用printf 打印出具体efuse bit map情况.
**      * 也可以通过  ioctl CMD (EFUSE_DUMP) EFUSE_BITS_PROGRAM, 分别通过 
**        struct hc_efuse_bits_program, 分别指定需要烧写的起始bits, 对应的mask, 
**        所要烧写的比特数量, 和具体要烧录的值.
**      * 具体使用实例, 可见下面
**
*/


/*
| Zone | Partition | bits |      Usage      |            Owner            |
| :--: | :-------: | :--: | :-------------: | :-------------------------: |
|  0   |  [31:0]   |  32  |    Unique ID    |         Chip Vendor         |
|  1   |  [63:32]  |  32  |    Unique ID    |         Chip Vendor         |
|  2   |  [71:64]  |  8   | hichip reserve0 |         Chip Vendor         |
|      |  [79:72]  |  8   | hichip reserve1 |         Chip Vendor         |
|      |  [95:80]  |  16  | hichip reserve2 |         Chip Vendor         |
|  3   | [127:96]  |  32  | Application use |          Customer           |
|  4   | [159:128] |  32  | Application use |          Customer           |
|  5   | [191:160] |  32  | Application use |          Customer           |
|  6   | [223:192] |  32  | Application use |          Customer           |
|  7   | [247:224] |  24  | Application use |          Customer           |
|  NA  | [250:248] |  3   |  Write Portect  |    HQ: zone[2] ~ zone[0]    |
|  NA  | [255:251] |  5   |  Write Portect  | Customer: zone[7] ~ zone[3] |
*/


#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <string.h>
#include <unistd.h>

#include <hcuapi/efuse.h>


void dump_efuse(struct hc_efuse_bit_map *bitmap)
{
    printf("\n================================================\r\n");
    printf(">>chip_vendor<<\r\n");
    printf("\tunique_id0 : 0x%x\r\n", bitmap->chip_vendor.unique_id0);
    printf("\tunique_id1 : 0x%x\r\n", bitmap->chipvendor.unique_id1);
    printf("\thichip_reserve0 : 0x%x\r\n", bitmap->chip_vendor.hichip_reserve0);
    printf("\tpart_number : 0x%x\r\n", bitmap->chip_vendor.hichip_reserve1);
    printf("\tfeature_list : 0x%x\r\n", bitmap->chip_vendor.hichip_reserve2);
    printf(">>customer<<\r\n");
    printf("\tcontent0 : 0x%x\r\n", bitmap->customer.content0);
    printf("\tcontent1 : 0x%x\r\n", bitmap->customer.content1);
    printf("\tcontent2 : 0x%x\r\n", bitmap->customer.content2);
    printf("\tcontent3 : 0x%x\r\n", bitmap->customer.content3);
    printf("\tcontent4 : 0x%x\r\n", bitmap->customer.content4);
    printf(">>write protect bit<<\r\n");
    printf("\twp_zone0 : %s\r\n", bitmap->wp.wp_zone0 ? "true" : "false");
    printf("\twp_zone1 : %s\r\n", bitmap->wp.wp_zone1 ? "true" : "false");
    printf("\twp_zone2 : %s\r\n", bitmap->wp.wp_zone2 ? "true" : "false");
    printf("\twp_zone3 : %s\r\n", bitmap->wp.wp_zone3 ? "true" : "false");
    printf("\twp_zone4 : %s\r\n", bitmap->wp.wp_zone4 ? "true" : "false");
    printf("\twp_zone5 : %s\r\n", bitmap->wp.wp_zone5 ? "true" : "false");
    printf("\twp_zone6 : %s\r\n", bitmap->wp.wp_zone6 ? "true" : "false");
    printf("\twp_zone7 : %s\r\n", bitmap->wp.wp_zone7 ? "true" : "false");
    printf("\n================================================\r\n");
}


int main(int argc, char **argv)
{
    int fd;
    struct hc_efuse_bit_map bitmap;

    /* open */
    printf("\n---> ready to open /dev/efuse\r\n");
    fd = open("/dev/efuse", O_RDWR);
    if(fd < 0){
        printf("[error] cannot open /dev/efuse, ret:%d\r\n", fd);
        return -1;
    }

    /* read */
    printf("\n---> ready to read efuse bit map \r\n");
    memset(&bitmap, 0, sizeof(struct hc_efuse_bit_map));
    if(0 > read(fd, &bitmap, sizeof(struct hc_efuse_bit_map))){
        printf("[error] cannot read /dev/efuse\r\n");
        return -1;
    }

    printf("\n---> read efuse bit finish, dump bit map\r\n");
    dump_efuse(&bitmap);

#if 1
    /* write */
    memset(&bitmap, 0, sizeof(struct hc_efuse_bit_map));
    // bitmap.chip_vendor.unique_id0 = 0x1;
    // bitmap.chip_vendor.unique_id1 = 0x4;
    bitmap.chip_vendor.hichip_reserve0 = 0x1;
    // bitmap.chip_vendor.hichip_reserve1 = 0x4;
    // bitmap.chip_vendor.hichip_reserve2 = 0x8;
    // bitmap.customer.content0 = 0x10;
    // bitmap.customer.content1 = 0x20;
    // bitmap.customer.content2 = 0x40;
    // bitmap.customer.content3 = 0x80;
    // bitmap.customer.content4 = 0x100;
    // bitmap.wp.wp_zone1 = 1;

    printf("\n---> ready to write efuse bit map \r\n");
    if(0 > write(fd, &bitmap, sizeof(struct hc_efuse_bit_map))){
        printf("[error] cannot write /dev/efuse\r\n");
        return -1;
    }

    /* read */
    printf("\n---> write efuse bit finish\r\n");
    memset(&bitmap, 0, sizeof(struct hc_efuse_bit_map));
    if(0 > read(fd, &bitmap, sizeof(struct hc_efuse_bit_map))){
        printf("[error] cannot read /dev/efuse\r\n");
        return -1;
    }
    printf("---> read efuse bit finish, dump bit map\r\n");
    dump_efuse(&bitmap);
#endif

#if 1
    /* ioctl -- EFUSE_PROGRAM */
    printf("\n---> write efuse bit by ioctl CMD: EFUSE_PROGRAM\r\n");
    memset(&bitmap, 0, sizeof(struct hc_efuse_bit_map));
    bitmap.chip_vendor.unique_id0 = 0x8;
    // bitmap.chip_vendor.unique_id1 = 0x8;
    // bitmap.chip_vendor.hichip_reserve0 = 0x4;
    // bitmap.chip_vendor.hichip_reserve1 = 0x2;
    // bitmap.chip_vendor.hichip_reserve2 = 0x1;
    // bitmap.customer.content0 = 0x100;
    // bitmap.customer.content1 = 0x80;
    // bitmap.customer.content2 = 0x40;
    // bitmap.customer.content3 = 0x20;
    // bitmap.customer.content4 = 0x10;
    ioctl(fd, EFUSE_PROGRAM, (uint32_t)&bitmap);

    /* ioctl -- EFUSE_DUMP */
    printf("\n---> read efuse bit by ioctl CMD: EFUSE_DUMP\r\n");
    memset(&bitmap, 0, sizeof(struct hc_efuse_bit_map));
    ioctl(fd, EFUSE_DUMP, (uint32_t)&bitmap);
#endif

#if 1
	/* ioctl -- EFUSE_BITS_PROGRAM */
    // 例子: 烧写 bit[184] 到 bit[194], 共11个bits, 这11个bits的mask为
    //       0x7ff, 具体要写为0x321
	struct hc_efuse_bits_program bits;
	bits.bits_offset = 184;
	bits.bits_length = 11;
	bits.bits_mask = 0x7ff;
	bits.bits_value = 0x321;
	printf("\n---> write efuse bit by ioctl CMD: EFUSE_BITS_PROGRAM\r\n");
	ioctl(fd, EFUSE_BITS_PROGRAM, (uint32_t)&bits);
	memset(&bitmap, 0, sizeof(struct hc_efuse_bit_map));
	if (0 > read(fd, &bitmap, sizeof(struct hc_efuse_bit_map))) {
		printf("[error] cannot read /dev/efuse\r\n");
		return -1;
	}
	dump_efuse(&bitmap);
#endif

    close(fd);

    return 0;
}

