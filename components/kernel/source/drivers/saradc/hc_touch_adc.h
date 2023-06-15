#ifndef __HC_TOUCH_ADC_H
#define __HC_TOUCH_ADC_H

#ifdef __cplusplus
extern "C" {
#endif

#define SET_CLK_200_HZ _IOW(SARADC_IOCBASE, 1, int)
#define SET_CLK_250_HZ _IOW(SARADC_IOCBASE, 2, int)
#define SET_CLK_300_HZ _IOW(SARADC_IOCBASE, 3, int)
#define SET_CLK_350_HZ _IOW(SARADC_IOCBASE, 4, int)
#define SET_CLK_400_HZ _IOW(SARADC_IOCBASE, 5, int)

#define GET_CURRENT_CLK_HZ _IOR(SARADC_IOCBASE, 0, int)

#define CH0_1_CEPI_ST 0x300

#define ch_num		4

struct touch_adc_priv
{
	struct input_dev 		*input;
	struct device			*dev;
	void __iomem			*base;
	void __iomem			*base_wait;
	int 					irq;
	int						ch_id[ch_num];

};

#define  ch_buf_len 3
#define  x_len 		100 
#define  y_len 		150 

struct touch_adc_param
{
	uint32_t ch_buf[ch_buf_len];
	uint32_t num ;
};

#ifdef __cplusplus
}
#endif

#endif // !__HC_TOUCH_ADC_H
