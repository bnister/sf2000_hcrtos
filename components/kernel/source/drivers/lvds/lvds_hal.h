#ifndef _LVDS_HAL_H_
#define _LVDS_HAL_H_

#include "lvds.h"
#include <hcuapi/lvds.h>

typedef struct _T_LVDS_CNTR_ {
	uint32_t channel_mode : 1;
	uint32_t map_mode : 1;
	uint32_t reserved_1 : 2;
	uint32_t ch0_src_sel : 1;
	uint32_t ch1_src_sel : 1;
	uint32_t reserved_2 : 2;
	uint32_t ch0_invert_clk_sel : 1;
	uint32_t ch1_invert_clk_sel : 1;
	uint32_t ch0_clk_gate : 1;
	uint32_t ch1_clk_gate : 1;
	uint32_t reserved_3 : 4;
	uint32_t hsync_polarity : 1;
	uint32_t vsync_polarity : 1;
	uint32_t even_odd_adjust_mode : 2;
	uint32_t even_odd_init_value : 1;
	uint32_t reserved_4 : 11;
} T_LVDS_CNTR;

typedef struct _T_LVDS_TRIGGER_ {
	uint32_t trigger_en : 1;
	uint32_t reserved_1 : 31;
} T_LVDS_TRIGGER;

//PHY reg0
typedef struct _T_LVDS_MODE_EN_ {
	unsigned char lane_clk_en : 6;
	unsigned char ttl_mode_en : 1;
	unsigned char lvds_mode_en : 1;
} T_LVDS_MODE_EN;

//PHY reg1
typedef struct _T_LVDS_LANE_CLK_BIAS_EN_ {
	unsigned char lane_clk_bias_en : 6;
	unsigned char reserve : 2;
} T_LVDS_LANE_CLK_BIAS_EN;

//PHY reg2
typedef struct _T_LVDS_LANE_CLK_LVDS_MODE_EN_ {
	unsigned char reserve : 1;

	unsigned char lane_clk_lvds_mode_en : 6;
	unsigned char reserve_1 : 1;
} T_LVDS_LANE_CLK_LVDS_MODE_EN;

//PHY reg4
typedef struct _T_LVDS_LANE_CLK_TTL_MODE_EN_ {
	unsigned char lane_clk_ttl_mode_en : 6;
	unsigned char reserve : 2;
} T_LVDS_LANE_CLK_TTL_MODE_EN;

typedef struct _T_LVDS_LANE_CLK_TTL_DATA_TRANS_EN_ {
	unsigned char lane_clk_ttl_data_trans_en : 6;
	unsigned char reserve_1 : 2;
} T_LVDS_LANE_CLK_TTL_DATA_TRANS_EN;

typedef struct _T_LVDS_PLL_EN_ {
	unsigned char reserve : 3;
	unsigned char pll_en : 1;
	unsigned char reserve_1 : 4;
} T_LVDS_PLL_EN;

typedef struct _T_LVDS_PHY_DIGITER_INTERNAL_ {
	unsigned char reserve : 7;

	unsigned char digital_internal_en : 1;
} T_LVDS_PHY_DIGITER_INTERNAL;

typedef struct _T_SYS_LVDS_PHY_ {
	uint32_t lvds_phy_ch0_en : 1;
	uint32_t lvds_phy_ch1_en : 1;
	uint32_t lvds_phy_pwr_down : 1;
	uint32_t revert : 28;
	uint32_t lvds_phy_pll_lock : 1;
} T_SYS_LVDS_PHY;
typedef enum _E_LVDS_IO_TTL_SEL
{
	LVDS_IO_TTL_SEL_RGB888,
	LVDS_IO_TTL_SEL_RGB666,
	LVDS_IO_TTL_SEL_RGB565,
	LVDS_IO_TTL_SEL_I2SO,
	LVDS_IO_TTL_SEL_SAR_ADC_TEST,
	LVDS_IO_TTL_SEL_LVDS_GPIO1,
	LVDS_IO_TTL_SEL_LVDS_GPIO2,
	LVDS_IO_TTL_SEL_LVDS_GPIO3
}LVDS_IO_TTL_SEL;

typedef enum RGB_HAL_SET_SRC_SELECT
{
	RGB_HAL_SET_SRC_SELECT_R=0,
	RGB_HAL_SET_SRC_SELECT_G,
	RGB_HAL_SET_SRC_SELECT_b,
}rgb_hal_set_src_e;

void lvds_hal_reset(void);
void lvds_io_ttl_sel_set(LVDS_IO_TTL_SEL lvds_ttl);
void lvds_hal_init(void *reg_base, void *sys_base);
void lvds_hal_set_pll_en(uint32_t ch, unsigned char b_enable);
void lvds_hal_set_pll_en_reserve_1(uint32_t ch, unsigned char b_enable);
void lvds_hal_set_phy_dig_en(uint32_t ch, unsigned char b_enable);
void lvds_hal_power_on(unsigned char b_on);
void lvds_hal_phy_ch_enable(uint32_t ch, uint8_t b_enable);
void lvds_hal_phy_ch_disable(uint32_t ch);
void lvds_hal_wait_pll_lock(void);
void lvds_hal_set_trigger_en(void);
void lvds_hal_set_lane_clk_data_trans_en(uint32_t ch, unsigned char b_enable);
void lvds_hal_set_lane_clk_ttl_mode_en(uint32_t ch, unsigned char b_enable);
void lvds_hal_set_lane_clk_data_reserve_1(uint32_t ch, unsigned char b_enable);
void lvds_hal_set_src_sel(E_VIDEO_SRC_SEL src_sel);
void rgb_hal_set_src_video_sel(E_VIDEO_SRC_SEL src_sel,E_VIDEO_2_SRC_SEL src_sel_2);
void rgb_hal_set_src_sel2(rgb_hal_set_src_e r,rgb_hal_set_src_e g,rgb_hal_set_src_e b);
void lvds_hal_set_channel_mode(lvds_channel_mode_e  channel_mode);
void lvds_hal_set_data_map_format(E_LVDS_MAP_MODE map_mode);
void lvds_hal_set_ch0_src_sel(uint32_t src_sel);
void lvds_hal_set_ch1_src_sel(uint32_t src_sel);
void lvds_hal_set_lane_clk_en_en(uint32_t ch, unsigned char b_enable);
void lvds_hal_set_lane_clk_bias_en(uint32_t ch, unsigned char b_enable);
void lvds_hal_set_ttl_mode_en(uint32_t ch, unsigned char b_enable);
void lvds_hal_set_lvds_mode_en(uint32_t ch, unsigned char b_enable);
void lvds_hal_set_lane_clk_lvds_mode_en(uint32_t ch, unsigned char b_enable);
void lvds_hal_set_even_odd_init_value(uint32_t int_value);
void lvds_hal_set_even_odd_adjust_mode(E_LVDS_EVEN_ODD_ADJUST_MODE adjust_mode);
void lvds_hal_set_vsync_polarity(uint32_t polarity);
void lvds_hal_set_hsync_polarity(uint32_t polarity);
void lvds_hal_set_ch1_invert_clk_sel(bool clock_invert);
void lvds_hal_set_ch0_invert_clk_sel(bool clock_invert);
void lvds_hal_set_ch0_clk_gate(bool clk_gate);
void lvds_hal_set_ch1_clk_gate(bool clk_gate);

uint32_t lvds_hal_get_channel_mode(void);
uint32_t lvds_hal_get_data_map_format(void);
uint32_t lvds_hal_get_ch0_src_sel(void);
uint32_t lvds_hal_get_ch1_src_sel(void);
uint32_t lvds_hal_get_ch0_invert_clk_sel(void);
uint32_t lvds_hal_get_ch1_invert_clk_sel(void);
uint32_t lvds_hal_get_ch0_clk_gate(void);
uint32_t lvds_hal_get_ch1_clk_gate(void);
uint32_t lvds_hal_get_hsync_polarity(void);
uint32_t lvds_hal_get_vsync_polarity(void);
uint32_t lvds_hal_get_even_odd_adjust_mode(void);
uint32_t lvds_hal_get_even_odd_init_value(void);
uint32_t lvds_hal_get_lvds_mode_en(uint32_t ch);
uint32_t lvds_hal_get_ttl_mode_en(uint32_t ch);
uint32_t lvds_hal_get_lane_clk_en_en(uint32_t ch);
uint32_t lvds_hal_get_lane_clk_bias_en(uint32_t ch);
uint32_t lvds_hal_get_lane_clk_lvds_mode_en(uint32_t ch);
uint32_t lvds_hal_get_lane_clk_ttl_mode_en(uint32_t ch);
uint32_t lvds_hal_get_lane_clk_data_trans_en(uint32_t ch);
uint32_t lvds_hal_get_pll_en(uint32_t ch);
uint32_t lvds_hal_get_phy_dig_en(uint32_t ch);
uint32_t lvds_hal_get_reg03(uint32_t ch);
uint32_t lvds_hal_get_reg0d(uint32_t ch);
uint32_t lvds_hal_get_reg08(uint32_t ch);
uint32_t lvds_hal_get_power_on(void);
uint32_t lvds_hal_phy_ch_get_enable(uint32_t ch);
uint32_t lvds_hal_get_src_sel(void);
uint32_t lvds_hal_get_trigger_en(void);
void lvds_set_gpio_output(struct lvds_set_gpio *pad);
void lvds_hal_rgb_clk_inv_sel(bool val);
#ifdef __cplusplus
}
#endif

#endif
