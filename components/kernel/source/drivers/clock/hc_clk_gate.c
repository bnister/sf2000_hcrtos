#include <kernel/io.h>
#include <stdio.h>
#include <kernel/drivers/hc_clk_gate.h>
#include <generated/br2_autoconf.h>

#define CLOCK_GATE_REG_0 0xb8800060
#define CLOCK_GATE_REG_1 0xb8800064
#define CLOCK_GATE_REG_2 0xb880008c

#define NONE -1

static int clk_cref[MAX_GATE] = {0};

#ifdef CONFIG_SOC_HC16XX
enum CLOCK_GATE_16 {
	HC16_SFLASH_CLK 			= 0,
	HC16_SD_MMC_CLK 			= 1,
	HC16_NAND_FLASH_CLK 			= 2,
	HC16_DESCRAMBLE_CLK 			= 3,
	HC16_TSG_CLK 				= 7,
	HC16_DEMUX1_CLK 			= 8,
	HC16_DEMUX2_CI_CLK 			= 9,
	HC16_TS_SWITCH_CLK 			= 10,
	HC16_ETH_MAC_CLK 			= 11,
	HC16_SAR_ADC_CLK 			= 12,
	HC16_VDEC_CLK 				= 13,
	HC16_DDR_PHY_APB_CLK 			= 14,
	HC16_DFI_CLK 				= 15,
	HC16_UART1_CLK 				= 16,
	HC16_UART2_CLK 				= 17,
	HC16_I2C1_CLK 				= 18,
	HC16_HDMI_TX_PHY_REFP_CLK 		= 19,
	HC16_WATCHDOG_CLK 			= 22,
	HC16_TIMER_CLK 				= 23,
	HC16_I2C2_CLK 				= 25,
	HC16_I2C3_CLK 				= 26,
	HC16_I2C4_CLK 				= 27,
	HC16_SB_APB_CLK 			= 28,
	HC16_SB_108M_CLK 			= 29,
	HC16_SB_1843M_CLK 			= 30,
	HC16_SB_27M_CLK 			= 31,
	HC16_VOU_HD_CLK 			= 0+32,
	HC16_VE_CLK 				= 1+32,
	HC16_DE_CLK 				= 2+32,
	HC16_TVENC_SD_CLK 			= 3+32,
	HC16_GE_CLK 				= 4+32,
	HC16_TVENC_HD_CLK 			= 5+32,
	HC16_VIN_CLK 				= 6+32,
	HC16_DE_HDMI_CLK 			= 7+32,
	HC16_VDAC1_CLK 				= 8+32,
	HC16_VDAC2_CLK 				= 9+32,
	HC16_VDAC3_CLK 				= 10+32,
	HC16_UART3_4_CLK 			= 11+32,
	HC16_SGDMA1_CLK 			= 12+32,
	HC16_SGDMA2_CLK 			= 13+32,
	HC16_HDMI_TX_CLK 			= 14+32,
	HC16_AUDIO_CLK 				= 15+32,
	HC16_SPI_MASTER_CLK 			= 16+32,
	HC16_DVI_CLK 				= 17+32,
	HC16_VOU_HD_EXT_CLK 			= 18+32,
	HC16_SPO_CLK 				= 20+32,
	HC16_DDP_SPO_CLK 			= 21+32,
	HC16_TSDMA_CLK 				= 22+32,
	HC16_JENC_CLK 				= 23+32,
	HC16_USB0_CLK 				= 24+32,
	HC16_USB1_CLK 				= 25+32,
	HC16_HDMI_TX_PHY_SYS_CLK  		= 27+32,
	HC16_MIPI_CLK 				= 28+32,
	HC16_BOOTROM_CLK 			= 29+32,
	HC16_TVDEC_CLK 				= 30+32,
	HC16_CPU1_CLK 				= 31+32,
	HC16_DE4K_HDMI_CLK 			= 0+352,
	HC16_DE4K_EXTO_CLK 			= 1+352,
	HC16_DE4K_CLK 				= 2+352,
	HC16_DE4K_DVI_CLK 			= 3+352,
	HC16_DE4K_VIN_CLK 			= 4+352,
	HC16_DE4K_VOU_HD_CLK 			= 5+352,
	HC16_LVDS_CH1_PIXEL_CLK 		= 7+352,
	HC16_LVDS_CLK 				= 8+352,
	HC16_VID_HDMI_CLK 			= 9+352,
	HC16_VID_CVBS_CLK 			= 10+352,
	HC16_HRX_CLK 				= 11+352,
	HC16_HRX_IAUDCLK 	 		= 12+352,
	HC16_RGB_CLK 				= 13+352,
	HC16_PQ_CLK 				= 14+352,
	HC16_AUD_EXTI_1_BCLK_P1 		= 16+352,
	HC16_AUD_EXTI_SP_CLK_P1 		= 17+352,
	HC16_AUD_EXTO_1_MCLK_P0 		= 18+352,
	HC16_AUD_EXTO_SP_CLK_P0 		= 19+352,
	HC16_AUD_EXTO_HDMI_ADAC_1_BCLK_P0 	= 20+352,
	HC16_AUD_EXTO_HDMI_ADAC_1_BCLK_P1 	= 21+352,
	HC16_AUD_EXTO_HDMI_ADAC_1_BCLK_P2 	= 22+352,
	HC16_AUD_HDMI_SP_CLK_P0 		= 23+352,
	HC16_AUD_HDMI_SP_CLK_P1 		= 24+352,
	HC16_HDMI_F128_LR_CLK 			= 25+352,
	HC16_AUD_PCMO_BCLK_CLK 			= 26+352,
};

static uint32_t clk_gate[MAX_GATE] = {
	HC16_SFLASH_CLK,
	HC16_SD_MMC_CLK,
	HC16_NAND_FLASH_CLK,
	HC16_DESCRAMBLE_CLK,
	HC16_TSG_CLK,
	HC16_DEMUX1_CLK,
	HC16_DEMUX2_CI_CLK,
	HC16_TS_SWITCH_CLK,
	HC16_ETH_MAC_CLK,
	HC16_SAR_ADC_CLK,
	HC16_VDEC_CLK,
	HC16_DDR_PHY_APB_CLK,
	HC16_DFI_CLK,
	HC16_UART1_CLK,
	HC16_UART2_CLK,
	HC16_I2C1_CLK,
	HC16_HDMI_TX_PHY_REFP_CLK,
	HC16_WATCHDOG_CLK,
	HC16_TIMER_CLK,
	HC16_I2C2_CLK,
	HC16_I2C3_CLK,
	HC16_I2C4_CLK,
	HC16_SB_APB_CLK,
	HC16_SB_108M_CLK,
	HC16_SB_1843M_CLK,
	HC16_SB_27M_CLK,
	HC16_VOU_HD_CLK,
	HC16_VE_CLK,
	HC16_DE_CLK,
	HC16_TVENC_SD_CLK,
	HC16_GE_CLK,
	HC16_TVENC_HD_CLK,
	HC16_VIN_CLK,
	HC16_DE_HDMI_CLK,
	HC16_VDAC1_CLK,
	HC16_VDAC2_CLK,
	HC16_VDAC3_CLK,
	NONE,
	HC16_UART3_4_CLK,
	HC16_SGDMA1_CLK,
	HC16_SGDMA2_CLK,
	HC16_HDMI_TX_CLK,
	HC16_AUDIO_CLK,
	HC16_SPI_MASTER_CLK,
	HC16_DVI_CLK,
	HC16_VOU_HD_EXT_CLK,
	HC16_SPO_CLK,
	HC16_DDP_SPO_CLK,
	HC16_TSDMA_CLK,
	HC16_JENC_CLK,
	HC16_USB0_CLK,
	HC16_USB1_CLK,
	HC16_HDMI_TX_PHY_SYS_CLK,
	HC16_MIPI_CLK,
	HC16_BOOTROM_CLK,
	HC16_TVDEC_CLK,
	HC16_CPU1_CLK,
	HC16_DE4K_HDMI_CLK,
        HC16_DE4K_EXTO_CLK, 			
        HC16_DE4K_CLK,		
        HC16_DE4K_DVI_CLK,
        HC16_DE4K_VIN_CLK,
        HC16_DE4K_VOU_HD_CLK,
        HC16_LVDS_CH1_PIXEL_CLK,
        HC16_LVDS_CLK,	
        HC16_VID_HDMI_CLK,
        HC16_VID_CVBS_CLK,			
        HC16_HRX_CLK,
	HC16_HRX_IAUDCLK,
        HC16_RGB_CLK,		
        HC16_PQ_CLK,			
        HC16_AUD_EXTI_1_BCLK_P1,
        HC16_AUD_EXTI_SP_CLK_P1,		
        HC16_AUD_EXTO_1_MCLK_P0,		
        HC16_AUD_EXTO_SP_CLK_P0,		
        HC16_AUD_EXTO_HDMI_ADAC_1_BCLK_P0,
        HC16_AUD_EXTO_HDMI_ADAC_1_BCLK_P1,	
        HC16_AUD_EXTO_HDMI_ADAC_1_BCLK_P2,	
        HC16_AUD_HDMI_SP_CLK_P0,
        HC16_AUD_HDMI_SP_CLK_P1,		
        HC16_HDMI_F128_LR_CLK,	
        HC16_AUD_PCMO_BCLK_CLK, 		
};

void hc_clk_disable_all(void)
{
	/*
	 * default enable Source Bridge Clock, Flash Clock,
	 * Timer Clock, DDR Clock ,sCpu Clock.
	 */

	*((uint32_t *)CLOCK_GATE_REG_0) = 0x0e0f3f8e;
	*((uint32_t *)CLOCK_GATE_REG_1) = 0x3fffffff;
	*((uint32_t *)CLOCK_GATE_REG_2) = 0xffffffff;

#if 0
	/* SDIO-LDO */
	REG32_CLR_BIT(0xb8800184, BIT25);
	REG32_CLR_BIT(0xb8800184, BIT23);
	REG32_CLR_BIT(0xb8800184, BIT24);

	/* VDAC */
	REG32_CLR_BIT(0xb8804004, BIT0);
	REG32_CLR_BIT(0xb8804084, BIT8);
	REG32_CLR_BIT(0xb8804084, BIT9);
	REG32_CLR_BIT(0xb8804084, BIT10);

	/* LVDS_PHY */
	//REG32_SET_BIT(0xb8800440, BIT2);
	//REG32_CLR_BIT(0xb886010f, BIT3);
	//REG32_CLR_BIT(0xb886014f, BIT3);

	/* CVBS-ADC */
	REG32_CLR_BIT(0xb8800180, BIT24);
	REG32_CLR_BIT(0xb8800180, BIT25);
	REG32_SET_BIT(0xb8800180, BIT26);
	REG32_CLR_BIT(0xb8800180, BIT27);
	REG32_CLR_BIT(0xb8800180, BIT28);
	REG32_CLR_BIT(0xb8800180, BIT29);

	/* HDMI_RX_PHT */
	REG32_SET_BIT(0xb8814030, BIT3);
	REG32_SET_BIT(0xb8814030, BIT4);
	REG32_SET_BIT(0xb8814010, BIT20);
	REG32_CLR_BIT(0xb8814010, BIT18);
	REG32_SET_BIT(0xb881401c, BIT31);
	REG32_CLR_BIT(0xb881401c, BIT24);
	REG32_SET_BIT(0xb8814014, BIT23);
	REG32_CLR_BIT(0xb8814014, BIT20);
	REG32_CLR_BIT(0xb8814014, BIT28);
	REG32_CLR_BIT(0xb8814018, BIT4);
	REG32_SET_BIT(0xb8814030, BIT23);
	REG32_CLR_BIT(0xb8814030, BIT16);
	REG32_CLR_BIT(0xb8814030, BIT17);
	REG32_CLR_BIT(0xb8814030, BIT18);
	REG32_SET_BIT(0xb8814000, BIT7);
	REG32_CLR_BIT(0xb8814020, BIT6);
	REG32_CLR_BIT(0xb8814020, BIT14);
	REG32_CLR_BIT(0xb8814020, BIT22);
	REG32_CLR_BIT(0xb8814010, BIT11);
	REG32_CLR_BIT(0xb8814010, BIT8);
	REG32_CLR_BIT(0xb8814010, BIT9);
	REG32_CLR_BIT(0xb8814010, BIT10);
#endif	
}

void hc_clk_enable(enum CLOCK_GATE gate)
{
	int clock = clk_gate[gate];
	volatile uint32_t *clock_gate_reg_0 = (volatile uint32_t *)CLOCK_GATE_REG_0;
	volatile uint32_t *clock_gate_reg_1 = (volatile uint32_t *)CLOCK_GATE_REG_1;
	volatile uint32_t *clock_gate_reg_2 = (volatile uint32_t *)CLOCK_GATE_REG_2;
	volatile uint32_t tmp_val;

	if (clock < 0)
		return;

	if (((clock > HC16_CPU1_CLK) && (clock < HC16_DE4K_HDMI_CLK)) |
	    (clock > HC16_AUD_PCMO_BCLK_CLK) | (clock < HC16_SFLASH_CLK)) {
		return;
	}

	if (clock < HC16_VOU_HD_CLK) {
		tmp_val = *clock_gate_reg_0;
		*clock_gate_reg_0 = (tmp_val & (~(1 << clock)));
	} else if (clock < HC16_DE4K_HDMI_CLK) {
		tmp_val = *clock_gate_reg_1;
		*clock_gate_reg_1 = (tmp_val & (~(1 << (clock - 32))));
	} else {
		tmp_val = *clock_gate_reg_2;
		*clock_gate_reg_2 = (tmp_val & (~(1 << (clock - 352))));
	}

	clk_cref[gate]++;
}

void hc_clk_disable(enum CLOCK_GATE gate)
{
	int clock = clk_gate[gate];
	volatile uint32_t *clock_gate_reg_0 = (volatile uint32_t *)CLOCK_GATE_REG_0;
	volatile uint32_t *clock_gate_reg_1 = (volatile uint32_t *)CLOCK_GATE_REG_1;
	volatile uint32_t *clock_gate_reg_2 = (volatile uint32_t *)CLOCK_GATE_REG_2;
	volatile uint32_t tmp_val;

	if ((clock < 0) || (clk_cref[gate] == 0)) {
		return;
	}

	if (((clock > HC16_CPU1_CLK) && (clock < HC16_DE4K_HDMI_CLK)) |
	    (clock > HC16_AUD_PCMO_BCLK_CLK) | (clock < HC16_SFLASH_CLK)) {
		return;
	}

	clk_cref[gate]--;
	if (clk_cref[gate] != 0)
		return;

	if (clock < HC16_VOU_HD_CLK) {
		tmp_val = *clock_gate_reg_0;
		*clock_gate_reg_0 = (tmp_val | (1 << clock));
	} else if (clock < HC16_DE4K_HDMI_CLK) {
		tmp_val = *clock_gate_reg_1;
		*clock_gate_reg_1 = (tmp_val | (1 << (clock - 32)));
	} else {
		tmp_val = *clock_gate_reg_2;
		*clock_gate_reg_2 = (tmp_val | (1 << (clock - 352)));
	}
}

#elif defined(CONFIG_SOC_HC15XX)
enum CLOCK_GATE_15 {
	HC15_SFLASH_CLK 		= 0,
	HC15_SDIO_CLK 			= 1,
	HC15_DESCRAMBLE_CLK 		= 3,
	HC15_TSG_CLK 			= 7,
	HC15_DEMUX1_CLK 		= 8,
	HC15_DEMUX2_CLK 		= 9,
	HC15_TS_SWITCH_CLK 		= 10,
	HC15_SAR_ADC_CLK 		= 12,
	HC15_DDR3PHY_APB_CLK 		= 14,
	HC15_DM_CTRL_DFI_CLK		= 15,
	HC15_UART1_CLK 			= 16,
	HC15_UART2_CLK 			= 17,
	HC15_I2C1_CLK 			= 18,
	HC15_HDMI_TX_PHY_REFP_CLK 	= 19,
	HC15_WATCHDOG_CLK 		= 22,
	HC15_REALTIMER_CLK 		= 23,
	HC15_I2C2_CLK 			= 25,
	HC15_I2C3_CLK 			= 26,
	HC15_SB_MEM_ILB_CLK		= 28,
	HC15_SB_108M_CLK 		= 29,
	HC15_SB_1843M_CLK 		= 30,
	HC15_SB_27M_CLK 		= 31,
	HC15_VOU_HD_CLK 		= 0+32,
	HC15_VE_CLK 			= 1+32,
	HC15_DE_CLK			= 2+32,
	HC15_TVENC_SD_CLK 		= 3+32,
	HC15_GE_CLK 			= 4+32,
	HC15_TVENC_HD_CLK 		= 5+32,
	HC15_VIN_CLK 			= 6+32,
	HC15_DE_HDMI_CLK 		= 7+32,
	HC15_VDAC1_CLK 			= 8+32,
	HC15_VDAC2_CLK 			= 9+32,
	HC15_VDAC3_CLK 			= 10+32,
	HC15_VDAC4_CLK 			= 11+32,
	HC15_SGDMA1_CLK 		= 12+32,
	HC15_HDMI_TX_CLK 		= 14+32,
	HC15_AUDIO_CLK 			= 15+32,
	HC15_DVI_CLK 			= 17+32,
	HC15_VOU_HD_EXT_CLK 		= 18+32,
	HC15_SPO_CLK 			= 20+32,
	HC15_DDP_SPO_CLK 		= 21+32,
	HC15_TSDMA_CLK 			= 22+32,
	HC15_JENC_CLK 			= 23+32,
	HC15_USB0_CLK 			= 24+32,
	HC15_USB1_CLK 			= 25+32,
	HC15_HDMI_TX_PHY_SYS_CLK  	= 27+32,
	HC15_BOOTROM_CLK 		= 29+32,
};

static uint32_t clk_gate[MAX_GATE] = {
	HC15_SFLASH_CLK,
	HC15_SDIO_CLK,
	NONE,
	HC15_DESCRAMBLE_CLK,
	HC15_TSG_CLK,
	HC15_DEMUX1_CLK,
	HC15_DEMUX2_CLK,
	HC15_TS_SWITCH_CLK,
	NONE,
	HC15_SAR_ADC_CLK,
	NONE,
	HC15_DDR3PHY_APB_CLK,
	HC15_DM_CTRL_DFI_CLK,
	HC15_UART1_CLK,
	HC15_UART2_CLK,
	HC15_I2C1_CLK,
	HC15_HDMI_TX_PHY_REFP_CLK,
	HC15_WATCHDOG_CLK,
	HC15_REALTIMER_CLK,
	HC15_I2C2_CLK,
	HC15_I2C3_CLK,
	NONE,
	HC15_SB_MEM_ILB_CLK,
	HC15_SB_108M_CLK,
	HC15_SB_1843M_CLK,
	HC15_SB_27M_CLK,
	HC15_VOU_HD_CLK,
	HC15_VE_CLK,
	HC15_DE_CLK,
	HC15_TVENC_SD_CLK,
	HC15_GE_CLK,
	HC15_TVENC_HD_CLK,
	HC15_VIN_CLK,
	HC15_DE_HDMI_CLK,
	HC15_VDAC1_CLK,
	HC15_VDAC2_CLK,
	HC15_VDAC3_CLK,
	HC15_VDAC4_CLK,
	NONE,
	HC15_SGDMA1_CLK,
	NONE,
	HC15_HDMI_TX_CLK,
	HC15_AUDIO_CLK,
	NONE,
	HC15_DVI_CLK,
	HC15_VOU_HD_EXT_CLK,
	HC15_SPO_CLK,
	HC15_DDP_SPO_CLK,
	HC15_TSDMA_CLK,
	HC15_JENC_CLK,
	HC15_USB0_CLK,
	HC15_USB1_CLK,
	HC15_HDMI_TX_PHY_SYS_CLK,
	NONE,
	HC15_BOOTROM_CLK,
	NONE,
	NONE,
	NONE,
	NONE,
	NONE,
	NONE,
	NONE,
	NONE,
	NONE,
	NONE,
	NONE,
	NONE,
	NONE,
	NONE,
	NONE,
	NONE,
	NONE,
	NONE,
	NONE,
	NONE,
	NONE,
	NONE,
	NONE,
	NONE,
	NONE,
	NONE,
	NONE,
};

void hc_clk_disable_all(void)
{
	//*((uint32_t *)CLOCK_GATE_REG_0) = 0x060fc7fe;
	//*((uint32_t *)CLOCK_GATE_REG_0) = 0x060f07fe;
	//*((uint32_t *)CLOCK_GATE_REG_1) = 0xffffffff;
}

void hc_clk_enable(enum CLOCK_GATE gate)
{
	int clock = clk_gate[gate];
	volatile uint32_t *clock_gate_reg_0 = (volatile uint32_t *)CLOCK_GATE_REG_0;
	volatile uint32_t *clock_gate_reg_1 = (volatile uint32_t *)CLOCK_GATE_REG_1;
	volatile uint32_t tmp_val;

	if (clock < 0)
		return;

	if ((clock > HC15_BOOTROM_CLK) | (clock < HC15_SFLASH_CLK))
		return;

	if (clock < HC15_VOU_HD_CLK) {
		tmp_val = *clock_gate_reg_0;
		*clock_gate_reg_0 = (tmp_val & (~(1 << clock)));
	} else {
		tmp_val = *clock_gate_reg_1;
		*clock_gate_reg_1 = (tmp_val & (~(1 << (clock - 32))));
	}

	clk_cref[gate]++;
}

void hc_clk_disable(enum CLOCK_GATE gate)
{
	int clock = clk_gate[gate];
	volatile uint32_t *clock_gate_reg_0 = (volatile uint32_t *)CLOCK_GATE_REG_0;
	volatile uint32_t *clock_gate_reg_1 = (volatile uint32_t *)CLOCK_GATE_REG_1;
	volatile uint32_t tmp_val;

	if ((clock < 0) || (clk_cref[gate] == 0))
		return;

	if ((clock > HC15_BOOTROM_CLK) | (clock < HC15_SFLASH_CLK))
		return;

	clk_cref[gate]--;
	if (clk_cref[gate] != 0)
		return;

	if (clock < HC15_VOU_HD_CLK) {
		tmp_val = *clock_gate_reg_0;
		*clock_gate_reg_0 = (tmp_val | (1 << clock));
	} else {
		tmp_val = *clock_gate_reg_1;
		*clock_gate_reg_1 = (tmp_val | (1 << (clock - 32)));
	}

	clk_cref[gate]--;
}
#endif
