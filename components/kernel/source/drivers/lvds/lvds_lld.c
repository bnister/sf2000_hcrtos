#include "lvds.h"
#include "lvds_hal.h"

void lvds_lld_init(void *reg_base, void *sys_base, bool reset)
{
	lvds_hal_init(reg_base, sys_base);

	if (reset)
		lvds_hal_reset();
}

void lvds_lld_set_cfg(T_LVDS_CFG *lvds_cfg,uint8_t ch)
{
	lvds_hal_set_src_sel(lvds_cfg->src_sel);
	lvds_hal_set_channel_mode(lvds_cfg->channel_mode);
	lvds_hal_set_data_map_format(lvds_cfg->map_mode);

	if(ch){
		lvds_hal_set_ch1_invert_clk_sel(lvds_cfg->ch1_invert_clk_sel);
		lvds_hal_set_ch1_src_sel(lvds_cfg->ch1_src_sel);
	}
	else{
		lvds_hal_set_ch0_src_sel(lvds_cfg->ch0_src_sel);
		lvds_hal_set_ch0_invert_clk_sel(lvds_cfg->ch0_invert_clk_sel);
	}

	lvds_hal_set_hsync_polarity(lvds_cfg->hsync_polarity);
	lvds_hal_set_vsync_polarity(lvds_cfg->vsync_polarity);
	lvds_hal_set_even_odd_adjust_mode(lvds_cfg->even_odd_adjust_mode);
	lvds_hal_set_even_odd_init_value(lvds_cfg->even_odd_init_value);
}

void lvds_lld_phy_mode_init(uint8_t ch,uint8_t mode)
{
	static char status_frist=1;
	if(status_frist)
	lvds_hal_power_on(0);
	if(mode)
	{
		lvds_hal_phy_ch_enable(0, 0);
		lvds_hal_phy_ch_enable(1, 0);
		usleep(10000);
		if(status_frist)
		lvds_hal_power_on(1);
		lvds_hal_phy_ch_enable(0, 1);
		lvds_hal_phy_ch_enable(1, 1);
		lvds_hal_set_phy_dig_en(0, 0);
		lvds_hal_set_phy_dig_en(1, 0);
		lvds_hal_set_pll_en_reserve_1(0,1);
		lvds_hal_set_pll_en_reserve_1(1,1);
		lvds_hal_set_pll_en(0, 1);
		lvds_hal_set_pll_en(1, 1);
		lvds_hal_wait_pll_lock();
		lvds_hal_set_phy_dig_en(0, 1);
		lvds_hal_set_phy_dig_en(1, 1);
		lvds_hal_set_trigger_en();
	}
	else
	{
		lvds_hal_phy_ch_enable(ch, 0);
		usleep(10000);
		if(status_frist)
		lvds_hal_power_on(1);
		lvds_hal_phy_ch_enable(ch, 1);
		lvds_hal_set_phy_dig_en(ch, 0);
		lvds_hal_set_pll_en_reserve_1(ch,1);
		lvds_hal_set_pll_en(ch, 1);
		lvds_hal_wait_pll_lock();
		lvds_hal_set_phy_dig_en(ch, 1);
		lvds_hal_set_trigger_en();
	}
	status_frist=0;
}

void lvds_lld_phy_ttl_mode_init(uint8_t ch)
{
		lvds_hal_set_lane_clk_lvds_mode_en(ch, 0);

		//disable lvds mode
		lvds_hal_set_lvds_mode_en(ch, 0);

		//enable ttl mode
		lvds_hal_set_ttl_mode_en(ch, 1);

		//disable lane clk bias en
		lvds_hal_set_lane_clk_bias_en(ch, 0x0);

		lvds_hal_set_lane_clk_en_en(ch, 0x3f);
		//enable lane clk ttl mode
		lvds_hal_set_lane_clk_ttl_mode_en(ch, 0x3f);

		//enable lane clk ttl data trans
		lvds_hal_set_lane_clk_data_trans_en(ch, 0x3f);
}
