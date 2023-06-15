#ifndef _AVDRIVER_AVINIT_H
#define _AVDRIVER_AVINIT_H

int llav_dis_init(void);
int llav_hdmi_init(void);
int hdmi_rx_dev_init(void);
int llav_vdec_init(void);

int viddec_driver_init(void);
int viddec_driver_exit(void);
int auddec_driver_init(void);
int auddec_driver_exit(void);
int audsink_driver_init(void);
int apll_dai_init(void);
int i2so_platform_init(void);
int i2si_platform_init(void);
int i2si0_platform_init(void);
int i2si1_platform_init(void);
int i2si2_platform_init(void);

int pcmi_platform_init(void);
int pcmi0_platform_init(void);
int pcmi1_platform_init(void);
int pcmi2_platform_init(void);
int hc16xx_link_init(void);
int i2s_dai_init(void);
int i2si0_dai_init(void);
int i2si1_dai_init(void);
int i2si2_dai_init(void);
int pcmi0_dai_init(void);
int pcmi1_dai_init(void);
int pcmi2_dai_init(void);
int cs4344_dai_init(void);
int pwm_dai_init(void);
int cjc8990_dai_init(void);
int cjc8988_dai_init(void);
int tdmi_platform_init(void);
int tdmi_dai_init(void);
int pdmi0_platform_init(void);
int pdmi0_dai_init(void);
int spin_platform_init(void);
int spo_dai_init(void);
int i2so_spo_dai_init(void);
int spo_platform_init(void);
int i2so_spo_platform_init(void);
int pcmo_dai_init(void);
int pcmo_platform_init(void);
int htx_dai_init(void);
int htx_platform_init(void);
int wm8960_dai_init(void);
int wm8960i_dai_init(void);

int vidsink_init(void);
int vindvp_init(void);
int tv_decoder_init(void);

int mipi_init(void);
int mipi_exit(void);
int pq_init(void);
int avsync_driver_init(void);

#endif
