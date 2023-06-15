#ifndef HC16XX_FB_FILTER_H
#define HC16XX_FB_FILTER_H

#define NO_WINDOW		(0)
#define HAMMING_WINDOW		(1)
#define LANCZOS_WINDOW		(2)

#define WINDOW_TYPE		(LANCZOS_WINDOW)

#define FIXED_BITS		(32)
#define FRAC_BITS		(16)
#define FRAC_MASK		((1 << (FRAC_BITS)) - 1)
#define INT_BITS		((FIXED_BITS) - (FRAC_BITS))

typedef int			frac;

#define FRAC_MAX		((frac)(0x7fffffff))
#define FRAC_MIN		((frac)(0x80000000))
#define FRAC_ONE		(1 << (FRAC_BITS))
#define FRAC_TWO		((FRAC_ONE) << 1)
#define FRAC_HALF		((FRAC_ONE) >> 1)
#define FRAC_QUARTER		((FRAC_ONE) >> 2)

#define FLOAT_TO_FRAC(x)	((frac)((x) * (FRAC_ONE)))
#define DOUBLE_TO_FRAC(x)	((frac)((x) * (FRAC_ONE)))
#define INT_TO_FRAC(x)		((x) << (FRAC_BITS))
#define FRAC_TO_DOUBLE(x)	((double)(x) / (FRAC_ONE))
#define PI 3.141592653589793238462643383279502884197169399375105820974944592
#define INV_PI (1/PI)
#define FRAC_INV_PI		(DOUBLE_TO_FRAC(INV_PI))
#define FRAC_INV_2PI_SQ		(DOUBLE_TO_FRAC(1/(4*PI*PI)))

#define MY_BITS			(16)
#define MY0			(93.484)
#define MY1			(314.486)
#define MY2			(31.7476)
#define MY3			(439.7176)

#define MC0_1			(51.542)
#define MC0_2			(224.8784)
#define MC1_1			(173.336)
#define MC1_2			(204.2796)
#define MC2_1			(224.8784)
#define MC2_2			(20.5989)
#define FLOOR(x)		((int)((x) + 0.5))

#ifdef  __cplusplus
extern "C" {
#endif

int generate_enhance_coef(int *coef, hcfb_enhance_t *enhance);

int generate_scale_coef(short *coef, uint32_t isamples,
			 uint32_t osamples, uint8_t tap, uint8_t ngroup,
			 bool duplicate);

#ifdef  __cplusplus
}
#endif

#endif /* HC16XX_FB_FILTER_H */
