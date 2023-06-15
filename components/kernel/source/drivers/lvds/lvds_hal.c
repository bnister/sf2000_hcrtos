#include "lvds_hal.h"
#include "lvds.h"
#define ELOG_OUTPUT_LVL ELOG_LVL_ERROR

#define LVDS_REG_DIGTAL_BASE_ADDR ((uint8_t *)reg_base)
#define LVDS_REG_PHY_CH0_BASE (LVDS_REG_DIGTAL_BASE_ADDR + 0x100)
#define LVDS_REG_PHY_CH1_BASE (LVDS_REG_DIGTAL_BASE_ADDR + 0x140)

#define p_lvds_reg_cntr  (T_LVDS_CNTR *)(LVDS_REG_DIGTAL_BASE_ADDR + LVDS_REG_DIGTAL_CNTR)
#define p_lvds_reg_trigger  (T_LVDS_TRIGGER *)(LVDS_REG_DIGTAL_BASE_ADDR + LVDS_REG_DIGTAL_TIGGER)

#define p_lvds_reg_ch0_mode_en  (T_LVDS_MODE_EN *)(LVDS_REG_PHY_CH0_BASE + LVDS_REG_PHY_0x0)
#define p_lvds_reg_ch0_lvds_lane_clk_bias_en  (T_LVDS_LANE_CLK_BIAS_EN *)(LVDS_REG_PHY_CH0_BASE + LVDS_REG_PHY_0x1)
#define p_lvds_reg_ch0_lvds_lane_clk_lvds_mode_en  (T_LVDS_LANE_CLK_LVDS_MODE_EN *)(LVDS_REG_PHY_CH0_BASE + LVDS_REG_PHY_0x2)
#define p_lvds_reg_ch0_lvds_lane_clk_ttl_mode_en  (T_LVDS_LANE_CLK_TTL_MODE_EN *)(LVDS_REG_PHY_CH0_BASE + LVDS_REG_PHY_0x4)
#define p_lvds_reg_ch0_lvds_lane_clk_data_trans_en  (T_LVDS_LANE_CLK_TTL_DATA_TRANS_EN *)(LVDS_REG_PHY_CH0_BASE + LVDS_REG_PHY_0x5)
#define p_lvds_reg_ch0_pll_en  (T_LVDS_PLL_EN *)(LVDS_REG_PHY_CH0_BASE + LVDS_REG_PHY_0xc)
#define p_lvds_reg_ch0_phy_digital_en  (T_LVDS_PHY_DIGITER_INTERNAL *)(LVDS_REG_PHY_CH0_BASE + LVDS_REG_PHY_DIG_01)
#define p_lvds_reg_ch0_03  (uint8_t *)(LVDS_REG_PHY_CH0_BASE + 0X3)
#define p_lvds_reg_ch0_0d  (uint8_t *)(LVDS_REG_PHY_CH0_BASE + 0Xd)
#define p_lvds_reg_ch0_08  (uint8_t *)(LVDS_REG_PHY_CH0_BASE + LVDS_REG_PHY_0x08)

#define p_lvds_reg_ch1_mode_en  (T_LVDS_MODE_EN *)(LVDS_REG_PHY_CH1_BASE + LVDS_REG_PHY_0x0)
#define p_lvds_reg_ch1_lvds_lane_clk_bias_en  (T_LVDS_LANE_CLK_BIAS_EN *)(LVDS_REG_PHY_CH1_BASE + LVDS_REG_PHY_0x1)
#define p_lvds_reg_ch1_lvds_lane_clk_lvds_mode_en  (T_LVDS_LANE_CLK_LVDS_MODE_EN *)(LVDS_REG_PHY_CH1_BASE + LVDS_REG_PHY_0x2)
#define p_lvds_reg_ch1_lvds_lane_clk_ttl_mode_en  (T_LVDS_LANE_CLK_TTL_MODE_EN *)(LVDS_REG_PHY_CH1_BASE + LVDS_REG_PHY_0x4)
#define p_lvds_reg_ch1_lvds_lane_clk_data_trans_en  (T_LVDS_LANE_CLK_TTL_DATA_TRANS_EN *)(LVDS_REG_PHY_CH1_BASE + LVDS_REG_PHY_0x5)
#define p_lvds_reg_ch1_pll_en  (T_LVDS_PLL_EN *)(LVDS_REG_PHY_CH1_BASE + LVDS_REG_PHY_0xc)
#define p_lvds_reg_ch1_phy_digital_en  (T_LVDS_PHY_DIGITER_INTERNAL *)(LVDS_REG_PHY_CH1_BASE + LVDS_REG_PHY_DIG_01)
#define p_lvds_reg_ch1_03  (uint8_t *)(LVDS_REG_PHY_CH1_BASE + 0X3)
#define p_lvds_reg_ch1_0d  (uint8_t *)(LVDS_REG_PHY_CH1_BASE + 0Xd)
#define p_lvds_reg_ch1_08  (uint8_t *)(LVDS_REG_PHY_CH1_BASE + LVDS_REG_PHY_0x08)

/*#define SYS_RST_CTRL_VAL (*(volatile uint32_t *)(0xB8800080))*/
/*#define SYS_RST1_CTRL_VAL (*(volatile uint32_t *)(0xB8800084))*/
#define SYS_RST1_CTRL_VAL 0x84
#define STRAP_PIN_CTRL 		0x94
/*#define SYS_VIDEO_SRC_CTRL_VAL (*(volatile uint32_t *)(0xB8800444))*/
/*#define SYS_VIDEO_SRC_CTRL_VAL1 (*(volatile uint32_t *)(0xB8800448))*/
#define SYS_LVDS_GPIO_REGISTER 0x174
#define SYS_VIDEO_SRC_CTRL_VAL 0x444
#define SYS_VIDEO_SRC_CTRL_VAL1 0x448
#define SYS_VIDEO_SRC_CTRL_VAL2 0x44c
#define SYS_RGB_CLK_INV_SEL 0x71C
/*#define SYS_VIDEO_SRC_CTRL_VAL2 (*(volatile uint32_t *)(0xB880044c))*/

typedef struct _T_LVDS_PHY_HAL_ {
	volatile T_LVDS_MODE_EN *reg_mode_en;
	volatile T_LVDS_LANE_CLK_BIAS_EN *reg_lvds_lane_clk_bias_en;
	volatile T_LVDS_LANE_CLK_LVDS_MODE_EN *reg_lvds_lane_clk_lvds_mode_en;
	volatile T_LVDS_LANE_CLK_TTL_MODE_EN *reg_lvds_lane_clk_ttl_mode_en;
	volatile T_LVDS_LANE_CLK_TTL_DATA_TRANS_EN *reg_lvds_lane_clk_ttl_data_trans_en;
	volatile T_LVDS_PLL_EN *reg_pll_en;

	volatile T_LVDS_PHY_DIGITER_INTERNAL *reg_phy_digital_en;
	volatile uint8_t *reg_03;
	volatile uint8_t *reg_0d;
	volatile uint8_t *reg_08;
} T_LVDS_PHY_HAL;

typedef struct _T_LVDS_HAL_ {
	volatile T_LVDS_CNTR *reg_cntr;
	volatile T_LVDS_TRIGGER *reg_trigger;
	T_LVDS_PHY_HAL reg_phy[2];
	volatile T_SYS_LVDS_PHY *lvds_reg_phy_sys;
	volatile uint32_t *sys_strap_pin_ctrl;
	volatile uint32_t *sys_rst1_ctrl_val;
	volatile uint32_t *sys_lvds_gpio;
	volatile uint32_t *sys_video_src_ctrl_val;
	volatile uint32_t *sys_video_src_ctrl_val1;
	volatile uint32_t *sys_video_src_ctrl_val2;
	volatile uint32_t *sys_rgb_clk_inv_sel;
} T_LVDS_HAL;

T_LVDS_HAL lvds_hal_instant;

void lvds_hal_reset(void)
{
	*lvds_hal_instant.sys_rst1_ctrl_val |= (1 << 8);
	usleep(1000);
	*lvds_hal_instant.sys_rst1_ctrl_val &= ~(1 << 8);
}
void lvds_io_ttl_sel_set(LVDS_IO_TTL_SEL lvds_ttl)
{
	*lvds_hal_instant.sys_strap_pin_ctrl=(*(lvds_hal_instant.sys_strap_pin_ctrl)&0x0fffffff)|(lvds_ttl<<28);
}
void lvds_hal_init(void *reg_base, void *sys_base)
{
	lvds_hal_instant.reg_cntr = p_lvds_reg_cntr;
	lvds_hal_instant.reg_trigger = p_lvds_reg_trigger;

	lvds_hal_instant.reg_phy[0].reg_mode_en = p_lvds_reg_ch0_mode_en;
	lvds_hal_instant.reg_phy[0].reg_lvds_lane_clk_bias_en = p_lvds_reg_ch0_lvds_lane_clk_bias_en;
	lvds_hal_instant.reg_phy[0].reg_lvds_lane_clk_lvds_mode_en = p_lvds_reg_ch0_lvds_lane_clk_lvds_mode_en;
	lvds_hal_instant.reg_phy[0].reg_lvds_lane_clk_ttl_mode_en = p_lvds_reg_ch0_lvds_lane_clk_ttl_mode_en;
	lvds_hal_instant.reg_phy[0].reg_lvds_lane_clk_ttl_data_trans_en = p_lvds_reg_ch0_lvds_lane_clk_data_trans_en;
	lvds_hal_instant.reg_phy[0].reg_pll_en = p_lvds_reg_ch0_pll_en;
	lvds_hal_instant.reg_phy[0].reg_phy_digital_en = p_lvds_reg_ch0_phy_digital_en; ;
	lvds_hal_instant.reg_phy[0].reg_03 = p_lvds_reg_ch0_03;
	lvds_hal_instant.reg_phy[0].reg_0d = p_lvds_reg_ch0_0d;
	lvds_hal_instant.reg_phy[0].reg_08 = p_lvds_reg_ch0_08;

	lvds_hal_instant.reg_phy[1].reg_mode_en = p_lvds_reg_ch1_mode_en;
	lvds_hal_instant.reg_phy[1].reg_lvds_lane_clk_bias_en = p_lvds_reg_ch1_lvds_lane_clk_bias_en;
	lvds_hal_instant.reg_phy[1].reg_lvds_lane_clk_lvds_mode_en = p_lvds_reg_ch1_lvds_lane_clk_lvds_mode_en;
	lvds_hal_instant.reg_phy[1].reg_lvds_lane_clk_ttl_mode_en = p_lvds_reg_ch1_lvds_lane_clk_ttl_mode_en;
	lvds_hal_instant.reg_phy[1].reg_lvds_lane_clk_ttl_data_trans_en = p_lvds_reg_ch1_lvds_lane_clk_data_trans_en;
	lvds_hal_instant.reg_phy[1].reg_pll_en = p_lvds_reg_ch1_pll_en;
	lvds_hal_instant.reg_phy[1].reg_phy_digital_en = p_lvds_reg_ch1_phy_digital_en;
	lvds_hal_instant.reg_phy[1].reg_03 = p_lvds_reg_ch1_03;
	lvds_hal_instant.reg_phy[1].reg_0d = p_lvds_reg_ch1_0d;
	lvds_hal_instant.reg_phy[1].reg_08 = p_lvds_reg_ch1_08;

	lvds_hal_instant.lvds_reg_phy_sys = sys_base + LVDS_REG_PHY_SYS;
	lvds_hal_instant.sys_rst1_ctrl_val = sys_base + SYS_RST1_CTRL_VAL;
	lvds_hal_instant.sys_strap_pin_ctrl = sys_base +	STRAP_PIN_CTRL;
	lvds_hal_instant.sys_lvds_gpio = sys_base +	SYS_LVDS_GPIO_REGISTER;
	lvds_hal_instant.sys_video_src_ctrl_val = sys_base + SYS_VIDEO_SRC_CTRL_VAL;
	lvds_hal_instant.sys_video_src_ctrl_val1 = sys_base + SYS_VIDEO_SRC_CTRL_VAL1;
	lvds_hal_instant.sys_video_src_ctrl_val2 = sys_base + SYS_VIDEO_SRC_CTRL_VAL2;
	lvds_hal_instant.sys_rgb_clk_inv_sel = sys_base+ SYS_RGB_CLK_INV_SEL;
	return;
}

void lvds_hal_set_channel_mode(lvds_channel_mode_e channel_mode)
{
	lvds_hal_instant.reg_cntr->channel_mode = channel_mode;
}

uint32_t lvds_hal_get_channel_mode(void)
{
	return lvds_hal_instant.reg_cntr->channel_mode;
}

void lvds_hal_set_data_map_format(E_LVDS_MAP_MODE map_mode)
{
	lvds_hal_instant.reg_cntr->map_mode = map_mode;
}

void lvds_hal_rgb_clk_inv_sel(bool val)
{
	*lvds_hal_instant.sys_rgb_clk_inv_sel &= 0xFBFFFFFF;
	*lvds_hal_instant.sys_rgb_clk_inv_sel |= (val<<30);
}

uint32_t lvds_hal_get_data_map_format(void)
{
	return lvds_hal_instant.reg_cntr->map_mode;
}

void lvds_hal_set_ch0_src_sel(uint32_t src_sel)
{
	lvds_hal_instant.reg_cntr->ch0_src_sel = src_sel;
}

uint32_t lvds_hal_get_ch0_src_sel(void)
{
	return lvds_hal_instant.reg_cntr->ch0_src_sel;
}

void lvds_hal_set_ch1_src_sel(uint32_t src_sel)
{
	lvds_hal_instant.reg_cntr->ch1_src_sel = src_sel;
}

uint32_t lvds_hal_get_ch1_src_sel(void)
{
	return lvds_hal_instant.reg_cntr->ch1_src_sel;
}

void lvds_hal_set_ch0_invert_clk_sel(bool clock_invert)
{
	lvds_hal_instant.reg_cntr->ch0_invert_clk_sel = clock_invert;
}

uint32_t lvds_hal_get_ch0_invert_clk_sel(void)
{
	return lvds_hal_instant.reg_cntr->ch0_invert_clk_sel;
}

void lvds_hal_set_ch1_invert_clk_sel(bool clock_invert)
{
	lvds_hal_instant.reg_cntr->ch1_invert_clk_sel = clock_invert;
}

uint32_t lvds_hal_get_ch1_invert_clk_sel(void)
{
	return lvds_hal_instant.reg_cntr->ch1_invert_clk_sel;
}

void lvds_hal_set_ch0_clk_gate(bool clk_gate)
{
	lvds_hal_instant.reg_cntr->ch0_clk_gate = clk_gate;
}

uint32_t lvds_hal_get_ch0_clk_gate(void)
{
	return lvds_hal_instant.reg_cntr->ch0_clk_gate;
}

void lvds_hal_set_ch1_clk_gate(bool clk_gate)
{
	lvds_hal_instant.reg_cntr->ch1_clk_gate = clk_gate;
}


uint32_t lvds_hal_get_ch1_clk_gate(void)
{
	return lvds_hal_instant.reg_cntr->ch1_clk_gate;
}

void lvds_hal_set_hsync_polarity(uint32_t polarity)
{
	lvds_hal_instant.reg_cntr->hsync_polarity = polarity;
}


uint32_t lvds_hal_get_hsync_polarity(void)
{
	return lvds_hal_instant.reg_cntr->hsync_polarity;
}

void lvds_hal_set_vsync_polarity(uint32_t polarity)
{
	lvds_hal_instant.reg_cntr->vsync_polarity = polarity;
}

uint32_t lvds_hal_get_vsync_polarity(void)
{
	return lvds_hal_instant.reg_cntr->vsync_polarity;
}

void lvds_hal_set_even_odd_adjust_mode(E_LVDS_EVEN_ODD_ADJUST_MODE adjust_mode)
{
	lvds_hal_instant.reg_cntr->even_odd_adjust_mode = adjust_mode;
}

uint32_t lvds_hal_get_even_odd_adjust_mode(void)
{
	return lvds_hal_instant.reg_cntr->even_odd_adjust_mode;
}

void lvds_hal_set_even_odd_init_value(uint32_t int_value)
{
	lvds_hal_instant.reg_cntr->even_odd_init_value = int_value;
}


uint32_t lvds_hal_get_even_odd_init_value(void)
{
	return lvds_hal_instant.reg_cntr->even_odd_init_value;
}

void lvds_hal_set_lvds_mode_en(uint32_t ch, unsigned char b_enable)
{
	lvds_hal_instant.reg_phy[ch].reg_mode_en->lvds_mode_en = b_enable;
}

uint32_t lvds_hal_get_lvds_mode_en(uint32_t ch)
{
	return lvds_hal_instant.reg_phy[ch].reg_mode_en->lvds_mode_en;
}

void lvds_hal_set_ttl_mode_en(uint32_t ch, unsigned char b_enable)
{
	lvds_hal_instant.reg_phy[ch].reg_mode_en->ttl_mode_en = b_enable;
}

uint32_t lvds_hal_get_ttl_mode_en(uint32_t ch)
{
	return lvds_hal_instant.reg_phy[ch].reg_mode_en->ttl_mode_en;
}

void lvds_hal_set_lane_clk_en_en(uint32_t ch, unsigned char b_enable)
{
	lvds_hal_instant.reg_phy[ch].reg_mode_en->lane_clk_en = b_enable;
}


uint32_t lvds_hal_get_lane_clk_en_en(uint32_t ch)
{
	return lvds_hal_instant.reg_phy[ch].reg_mode_en->lane_clk_en;
}

void lvds_hal_set_lane_clk_bias_en(uint32_t ch, unsigned char b_enable)
{
	lvds_hal_instant.reg_phy[ch].reg_lvds_lane_clk_bias_en->lane_clk_bias_en = b_enable;
}

uint32_t lvds_hal_get_lane_clk_bias_en(uint32_t ch)
{
	return lvds_hal_instant.reg_phy[ch].reg_lvds_lane_clk_bias_en->lane_clk_bias_en;
}

void lvds_hal_set_lane_clk_lvds_mode_en(uint32_t ch, unsigned char b_enable)
{
	lvds_hal_instant.reg_phy[ch].reg_lvds_lane_clk_lvds_mode_en->lane_clk_lvds_mode_en = b_enable;
}

uint32_t lvds_hal_get_lane_clk_lvds_mode_en(uint32_t ch)
{
	return lvds_hal_instant.reg_phy[ch].reg_lvds_lane_clk_lvds_mode_en->lane_clk_lvds_mode_en;
}

void lvds_hal_set_lane_clk_ttl_mode_en(uint32_t ch, unsigned char b_enable)
{
	lvds_hal_instant.reg_phy[ch].reg_lvds_lane_clk_ttl_mode_en->lane_clk_ttl_mode_en = b_enable;
}

uint32_t lvds_hal_get_lane_clk_ttl_mode_en(uint32_t ch)
{
	return lvds_hal_instant.reg_phy[ch].reg_lvds_lane_clk_ttl_mode_en->lane_clk_ttl_mode_en;
}

void lvds_hal_set_lane_clk_data_trans_en(uint32_t ch, unsigned char b_enable)
{
	lvds_hal_instant.reg_phy[ch].reg_lvds_lane_clk_ttl_data_trans_en->lane_clk_ttl_data_trans_en = b_enable;
}

void lvds_hal_set_lane_clk_data_reserve_1(uint32_t ch, unsigned char b_enable)
{
	lvds_hal_instant.reg_phy[ch].reg_lvds_lane_clk_ttl_data_trans_en->reserve_1 = b_enable;
}

uint32_t lvds_hal_get_lane_clk_data_trans_en(uint32_t ch)
{
	return lvds_hal_instant.reg_phy[ch].reg_lvds_lane_clk_ttl_data_trans_en->lane_clk_ttl_data_trans_en;
}

void lvds_hal_set_pll_en(uint32_t ch, unsigned char b_enable)
{
	lvds_hal_instant.reg_phy[ch].reg_pll_en->pll_en = !b_enable;
}

void lvds_hal_set_pll_en_reserve_1(uint32_t ch, unsigned char b_enable)
{
	lvds_hal_instant.reg_phy[ch].reg_pll_en->reserve_1 = b_enable;
}

uint32_t lvds_hal_get_pll_en(uint32_t ch)
{
	return lvds_hal_instant.reg_phy[ch].reg_pll_en->pll_en;
}

void lvds_hal_set_phy_dig_en(uint32_t ch, unsigned char b_enable)
{
	lvds_hal_instant.reg_phy[ch].reg_phy_digital_en->digital_internal_en = b_enable;
}

uint32_t lvds_hal_get_phy_dig_en(uint32_t ch)
{
	return lvds_hal_instant.reg_phy[ch].reg_phy_digital_en->digital_internal_en;
}

void lvds_hal_set_reg03(uint32_t ch, uint8_t val)
{
	*(lvds_hal_instant.reg_phy[ch].reg_03) = val;
}

uint32_t lvds_hal_get_reg03(uint32_t ch)
{
	return *(lvds_hal_instant.reg_phy[ch].reg_03);
}

void lvds_hal_set_reg0d(uint32_t ch, uint8_t val)
{
	*(lvds_hal_instant.reg_phy[ch].reg_0d) = val;
}

uint32_t lvds_hal_get_reg0d(uint32_t ch)
{
	return *(lvds_hal_instant.reg_phy[ch].reg_0d);
}

void lvds_hal_set_reg08(uint32_t ch, uint8_t val)
{
	*(lvds_hal_instant.reg_phy[ch].reg_08) = val;
}


uint32_t lvds_hal_get_reg08(uint32_t ch)
{
	return *(lvds_hal_instant.reg_phy[ch].reg_08);
}

void lvds_hal_power_on(unsigned char b_on)
{
	lvds_hal_instant.lvds_reg_phy_sys->lvds_phy_pwr_down = !b_on;
}

uint32_t lvds_hal_get_power_on(void)
{
	return lvds_hal_instant.lvds_reg_phy_sys->lvds_phy_pwr_down;
}


void lvds_hal_phy_ch_enable(uint32_t ch, uint8_t b_enable)
{
	if(ch == 0) {
		lvds_hal_instant.lvds_reg_phy_sys->lvds_phy_ch0_en = !!b_enable;
	} else {
		lvds_hal_instant.lvds_reg_phy_sys->lvds_phy_ch1_en = !!b_enable;
	}
}

uint32_t lvds_hal_phy_ch_get_enable(uint32_t ch) {
	if(ch == 0) {
		return lvds_hal_instant.lvds_reg_phy_sys->lvds_phy_ch0_en;
	} else {
		return lvds_hal_instant.lvds_reg_phy_sys->lvds_phy_ch1_en;
	}
}

void lvds_hal_wait_pll_lock()
{
	uint32_t cnt = 0;

	while(lvds_hal_instant.lvds_reg_phy_sys->lvds_phy_pll_lock == 0) {
		cnt++;
		if(cnt == 1000) {
			log_e("lvds_hal_wait_pll_lock T O \n");
			break;
		}
		usleep(1000);
	}
}

void lvds_gpio_set_output_high(unsigned int padctl)
{
	uint32_t bit = BIT(padctl % 32);
	void *reg= (void *)((uint32_t)lvds_hal_instant.sys_lvds_gpio);
	REG32_SET_BIT(reg,bit);
}

void lvds_gpio_set_output_low(unsigned int padctl)
{
	uint32_t bit = BIT(padctl % 32);
	void *reg= (void *)((uint32_t)lvds_hal_instant.sys_lvds_gpio);
	REG32_CLR_BIT(reg,bit);
}

void lvds_set_gpio_output(struct lvds_set_gpio *pad)
{
	if(pad==NULL)
	{
		log_e("error PAD NULL\n");
		return;
	}
	if(pad->padctl>PINPAD_LVDS_RESERVED12)
	{
		log_e("padctl Maximum exceeded\n");
		return;
	}
	pad->padctl = pad->padctl - PINPAD_LVDS_DP0;
	if(pad->value)
		lvds_gpio_set_output_high(pad->padctl);
	else
		lvds_gpio_set_output_low(pad->padctl);
}

void lvds_hal_set_src_sel(E_VIDEO_SRC_SEL src_sel)
{
	*lvds_hal_instant.sys_video_src_ctrl_val &= 0xFFFFCCFF;
	*lvds_hal_instant.sys_video_src_ctrl_val |= (src_sel << 8) | (src_sel << 12);

	*lvds_hal_instant.sys_video_src_ctrl_val1 &= 0xFFFFCCFF;
	*lvds_hal_instant.sys_video_src_ctrl_val1 |= (src_sel << 8) | (src_sel << 12);

}

void rgb_hal_set_src_video_sel(E_VIDEO_SRC_SEL src_sel,E_VIDEO_2_SRC_SEL src_sel_2)
{
	*lvds_hal_instant.sys_video_src_ctrl_val &= 0xFFFFFFCF;
	*lvds_hal_instant.sys_video_src_ctrl_val |=src_sel<<4;

	*lvds_hal_instant.sys_video_src_ctrl_val1 &= 0xFFF3FFFF;
	*lvds_hal_instant.sys_video_src_ctrl_val1 |= (src_sel << 18);

	*lvds_hal_instant.sys_video_src_ctrl_val2 &= 0xFFFFF888;
	*lvds_hal_instant.sys_video_src_ctrl_val2 |= (src_sel_2<<0)|(src_sel_2<<4)|(src_sel_2<<8);
}

void rgb_hal_set_src_sel2(rgb_hal_set_src_e r,rgb_hal_set_src_e g,rgb_hal_set_src_e b)
{
	uint32_t tmp=0;
	tmp=(r<<24)|(b<<20)|(g<<16);
	*lvds_hal_instant.sys_video_src_ctrl_val2 &= 0xF000FFFF;
	*lvds_hal_instant.sys_video_src_ctrl_val2 |= tmp;
}

uint32_t lvds_hal_get_src_sel(void)
{
	return (*lvds_hal_instant.sys_video_src_ctrl_val >> 8) & 0x1;
}

void lvds_hal_set_trigger_en(void)
{
	lvds_hal_instant.reg_trigger->trigger_en = 1;
}

uint32_t lvds_hal_get_trigger_en(void)
{
	return lvds_hal_instant.reg_trigger->trigger_en;
}
