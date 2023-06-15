#include <stdint.h>
#include <stdbool.h>
#include <hcuapi/fb.h>
#include "hc16xx_fb_filter.h"

#define ENTRY_BITS		(8)
static const unsigned short cos_lut[1 << ENTRY_BITS] = {
	65535, 65531, 65525, 65516, 65505, 65492, 65476, 65457, 65436, 65413,
	65387, 65358, 65328, 65294, 65259, 65220, 65180, 65137, 65091, 65043,
	64993, 64940, 64884, 64827, 64766, 64704, 64639, 64571, 64501, 64429,
	64354, 64277, 64197, 64115, 64031, 63944, 63854, 63763, 63668, 63572,
	63473, 63372, 63268, 63162, 63054, 62943, 62830, 62714, 62596, 62476,
	62353, 62228, 62101, 61971, 61839, 61705, 61568, 61429, 61288, 61145,
	60999, 60851, 60700, 60547, 60392, 60235, 60075, 59914, 59750, 59583,
	59415, 59244, 59071, 58896, 58718, 58538, 58356, 58172, 57986, 57798,
	57607, 57414, 57219, 57022, 56823, 56621, 56418, 56212, 56004, 55794,
	55582, 55368, 55152, 54934, 54714, 54491, 54267, 54040, 53812, 53581,
	53349, 53114, 52878, 52639, 52398, 52156, 51911, 51665, 51417, 51166,
	50914, 50660, 50404, 50146, 49886, 49624, 49361, 49095, 48828, 48559,
	48288, 48015, 47741, 47464, 47186, 46906, 46624, 46341, 46056, 45769,
	45480, 45190, 44898, 44604, 44308, 44011, 43713, 43412, 43110, 42806,
	42501, 42194, 41886, 41576, 41264, 40951, 40636, 40320, 40002, 39683,
	39362, 39040, 38716, 38391, 38064, 37736, 37407, 37076, 36744, 36410,
	36075, 35738, 35401, 35062, 34721, 34380, 34037, 33692, 33347, 33000,
	32652, 32303, 31952, 31600, 31248, 30893, 30538, 30182, 29824, 29466,
	29106, 28745, 28383, 28020, 27656, 27291, 26925, 26558, 26190, 25821,
	25451, 25080, 24708, 24335, 23961, 23586, 23210, 22834, 22457, 22078,
	21699, 21320, 20939, 20557, 20175, 19792, 19409, 19024, 18639, 18253,
	17867, 17479, 17091, 16703, 16314, 15924, 15534, 15143, 14751, 14359,
	13966, 13573, 13180, 12785, 12391, 11996, 11600, 11204, 10808, 10411,
	10014, 9616,  9218,  8820,  8421,  8022,  7623,  7224,  6824,  6424,
	6023,  5623,  5222,  4821,  4420,  4019,  3617,  3216,  2814,  2412,
	2010,  1608,  1206,  804,   402,   0,
};

/* csc matrix coef */
static int enhance_coef_default_bt709[] = {
	93, 314, 32, 0, -52, -173, 225, 0, 225, -204, -21, 0,
};

static int enhance_coef_default_bt601[] = {
	131, 258, 50, 0, -76, -149, 225, 0, 225, -188, -37, 0,
};

static frac cosffn(frac x)
{
	int neg = 0;
	int index, alpha;
	const int residue_nbit = FRAC_BITS - (ENTRY_BITS + 1);
	frac prev, next, val;

	x = x < 0 ? -x : x;

	// fold to [0, 2)*pi
	x &= (2 << FRAC_BITS) - 1;

	// fold to [0..1)*pi
	if (x >= FRAC_ONE)
		x = FRAC_TWO - x;

	// fold x to [0..0.5]*pi
	if (x > FRAC_HALF) {
		x = FRAC_ONE - x;
		neg = 1;
	}

	index = x >> residue_nbit;
	alpha = x & ((1 << residue_nbit) - 1);

	if (index == 0)
		prev = FRAC_ONE;
	else
		prev = cos_lut[index - 1];

	if (index == (1 << ENTRY_BITS))
		next = -cos_lut[index - 2];
	else
		next = cos_lut[index];

	val = (prev * ((1 << residue_nbit) - alpha) + next * alpha + (1 << (residue_nbit - 1))) >> residue_nbit;

	return neg ? -val : val;
}

static frac sinffn(frac x)
{
	return cosffn(FRAC_HALF - x);
}

static void tunefilter(int nphase, int ntap, int scale, short *const pquant)
{
	int i, j;
	const int ref_tap = ntap >> 1;

	for (i = 0; i < nphase; i++) {
		int sum = 0;
		short *pq = pquant + i;
		short *pq_ref = pq + ref_tap * nphase;
		short *pq_adj = pq + (ref_tap - 1) * nphase;
		for (j = 0; j < ntap; j++) {
			sum += *pq;
			pq += nphase;
		}
		if (sum != scale) {
			int adjustment = scale - sum;
			/*  central 2 taps always be the greatest. in case of odd taps,
			    central tap is the greatest.
			 */
			if (*pq_ref > *pq_adj) {
				*pq_ref += adjustment;
			} else if (*pq_ref < *pq_adj) {
				*pq_adj += adjustment;
			} else {
				*pq_ref += (adjustment + 1) / 2;
				*pq_adj += adjustment / 2;
			}
		}

		if (*pq_ref > scale) {
			int adjustment = *pq_ref - scale;
			*pq_ref -= adjustment;
			*pq_adj += adjustment;
		} else if (*pq_adj > scale) {
			int adjustment = *pq_adj - scale;
			*pq_adj -= adjustment;
			*pq_ref += adjustment;
		}
	}
}

static inline frac mul(frac a, frac b)
{
	long long c = a;

	c *= b;

	return (frac)(c >> FRAC_BITS);
}

static inline frac mul_frac_int(frac a, int b)
{
	return a * b;
}

static frac reciprocal(frac x)
{
	int neg = x < 0;
	int i, n = 2;
	/* reciprocal_lut[(x>>(FRAC_BITS-5))&((1<<10)-1)]; */
	frac y;
	unsigned mask = 0x40000000;
	int count = 1, e;

	if (x < 0)
		x = -x;

	while ((mask & x) == 0) {
		mask >>= 1;
		count++;
		if (count == 32)
			return neg ? FRAC_MIN : FRAC_MAX;
	}

	e = FRAC_BITS - count;

	// normalize to [0.5, 1)
	if (e > 0) {
		x >>= e;
	} else {
		x <<= -e;
	}

	y = DOUBLE_TO_FRAC(2.9282032302755091741) - (x << 1);

	// y = y*(2-x*y)
	for (i = 0; i < n; i++)
		y = mul(y, FRAC_TWO - mul(x, y));

	y = e > 0 ? y >> e : y << -e;

	return neg ? -y : y;
}

static frac sincff(frac x)
{
	if (x == 0) {
		return FRAC_ONE;
	} else {
		return mul(sinffn(x), mul(reciprocal(x), FRAC_INV_PI));
	}
}

static void firlsff(frac *const pcoef, const frac cutoff, const int len)
{
	int i;
	unsigned s;
	frac b0 = 0, b1, m, k;
	frac *b; /* b(x) */

	int oddlen = len % 2;
	int halflen = len / 2;

	frac amp[4] = { FRAC_ONE, FRAC_ONE, 0, 0 };
	frac bb[4] = { 0, 0, 0, FRAC_HALF }; /* band bound/2 */

	bb[1] = bb[2] = cutoff >> 1;

	for (i = 0; i < len; i++) {
		pcoef[i] = 0;
	}

	b = pcoef + len / 2 + (oddlen ? 1 : 0);
	for (s = 0; s < sizeof(bb) / sizeof(bb[0]); s += 2) {
		//m = DOUBLE_TO_FRAC(FRAC_TO_DOUBLE(amp[s+1] - amp[s]) / FRAC_TO_DOUBLE(bb[s+1] - bb[s]));
		m = mul((amp[s + 1] - amp[s]), reciprocal(bb[s + 1] - bb[s]));
		b1 = amp[s] - mul(m, bb[s]);
		if (oddlen) {
			b0 += mul(b1, (bb[s + 1] - bb[s])) +
			      mul((m >> 1), (mul(bb[s + 1], bb[s + 1]) -
					     mul(bb[s], bb[s])));
		}

		for (i = 0; i < halflen; i++) {
			frac t0, t1, t2;
			k = INT_TO_FRAC(i) + (oddlen ? FRAC_ONE : FRAC_HALF);
			//t0 = DOUBLE_TO_FRAC(cos(2*PI*FRAC_TO_DOUBLE(mul(k,bb[s+1])))-cos(2*PI*FRAC_TO_DOUBLE(mul(k,bb[s]))));
			t0 = cosffn(mul(k, bb[s + 1]) << 1) -
			     cosffn(mul(k, bb[s]) << 1);
			//t1 = mul(mul(bb[s+1],(mul(m,bb[s+1])+b1)),DOUBLE_TO_FRAC(sinc(FRAC_TO_DOUBLE(mul(2*k,bb[s+1])))));
			t1 = mul(mul(bb[s + 1], (mul(m, bb[s + 1]) + b1)),
				 sincff(mul(2 * k, bb[s + 1])));
			//t2 = mul(mul(bb[s],(mul(m,bb[s])+b1)),DOUBLE_TO_FRAC(sinc(FRAC_TO_DOUBLE(mul(2*k,bb[s])))));
			t2 = mul(mul(bb[s], (mul(m, bb[s]) + b1)),
				 sincff(mul(2 * k, bb[s])));
			b[i] += mul(mul(mul(m, FRAC_INV_2PI_SQ), t0),
				    reciprocal(mul(k, k)));
			b[i] += t1 - t2;
		}
	}

	if (oddlen) {
		pcoef[len / 2] = b0;
		for (i = 0; i < halflen; i++) {
			pcoef[len / 2 - 1 - i] = pcoef[len / 2 + 1 + i];
		}
	} else {
		for (i = 0; i < halflen; i++) {
			pcoef[(len - 1) / 2 - i] = pcoef[len / 2 + i];
		}
	}
	for (i = 0; i < len; i++)
		pcoef[i] <<= 1;
}

/*
 * calculate hamming window
 * n - window length
 * k - position in the window
 * formula: 0.54 - 0.46 * cos(2 * PI * k / (n-1))
 */
static frac hammingff(int n, int k)
{
	static int last_n = 0;
	static frac den = 0;

	if (last_n != n) {
		last_n = n;
		den = reciprocal(INT_TO_FRAC(n - 1));
	}
	return DOUBLE_TO_FRAC(0.54) -
	       mul(DOUBLE_TO_FRAC(0.46), cosffn(mul(INT_TO_FRAC(k) << 1, den)));
}

static int convergentff(frac x)
{
	int neg = x < 0;
	int intpart, fracpart;

	x = x > 0 ? x : -x;

	intpart = x >> FRAC_BITS;
	fracpart = x & FRAC_MASK;

	if (fracpart > FRAC_HALF ||
	    ((fracpart == FRAC_HALF) && ((intpart & 1) != 0)))
		intpart++;

	return neg ? -intpart : intpart;
}

/*
 * calculate Lanczos window
 * n - window length
 * k - position in the window, [0, n-1]
 */
static frac lanczosff(int n, int k)
{
	static int last_n = 0;
	static frac den = 0;
	frac x, y;

	if (last_n != n) {
		last_n = n;
		den = reciprocal(INT_TO_FRAC(n - 1));
	}
	x = mul(INT_TO_FRAC(k << 1), den) - INT_TO_FRAC(1);
	y = sincff(x);
	return y;
}

static void quantizeff(short *pquant, frac *preal, int len, int nfracbits)
{
	int i;
	for (i = 0; i < len; i++) {
		*pquant++ = convergentff(*preal++ << nfracbits);
	}
}

/* normalize sum of each phase to 1.0 */
static void normalizeff(int nphase, int ntap, frac *const preal)
{
	int i, j;
	frac *prcoef, sum, a;

	for (i = 0; i < nphase; i++) {
		sum = 0;
		prcoef = preal + i;
		for (j = 0; j < ntap; j++) {
			sum += *prcoef;
			prcoef += nphase;
		}
		prcoef = preal + i;
		a = reciprocal(sum);
		for (j = 0; j < ntap; j++) {
			*prcoef = mul(*prcoef, a); /* *prcoef /= sum; */
			prcoef += nphase;
		}
	}
}

#if (WINDOW_TYPE == HAMMING_WINDOW)
#define WIN_FUNC(len, idx)		(hamming(len, idx))
#define WIN_FUNC_FF(len, idx)		(hammingff(len, idx))
#elif (WINDOW_TYPE == LANCZOS_WINDOW)
#define WIN_FUNC(len, idx)		(lanczos(len, idx))
#define WIN_FUNC_FF(len, idx)		(lanczosff(len, idx))
#else
#define WIN_FUNC(len, idx)		(1)
#define WIN_FUNC_FF(len, idx)		(FRAC_ONE)
#endif
static void designfilterff(int srcsampling, int dstsampling, int nphase,
			   int ntap, int centered, frac cutoffscale,
			   int nfracbits, frac *const preal,
			   short *const pquant)
{
	int i, filtlen, scale;
	frac cutoff;
	filtlen = nphase * ntap + centered;

	if (dstsampling >= srcsampling) {
		/* scaleup */
		cutoff = reciprocal(INT_TO_FRAC(nphase));
	} else {
		/* scaledown */
		cutoff = mul(reciprocal(INT_TO_FRAC(nphase)),
			     mul_frac_int(reciprocal(INT_TO_FRAC(srcsampling)),
					  dstsampling));
	}

	cutoff = mul(cutoff, cutoffscale);

	// design filter
	firlsff(preal, cutoff, filtlen);

	// windowing
	for (i = 0; i < filtlen; i++) {
		preal[i] = mul(preal[i], WIN_FUNC_FF(filtlen, i));
	}

	normalizeff(nphase, ntap, preal);

	scale = 1 << nfracbits;

	quantizeff(pquant, preal, filtlen, nfracbits);
	tunefilter(nphase, ntap, scale, pquant);
}

int generate_enhance_coef(int *coef, hcfb_enhance_t *enhance)
{
	double Ly, Sy, Sc;
	double tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7, tmp8, tmp9;
	frac sin = 0, cos = 0;
	int i = 0;
	int brightness, contrast, saturation, hue, sharpness;
	hcfb_enhance_cscmode_e csc_mode;

	brightness = enhance->brightness;
	contrast = enhance->contrast;
	saturation = enhance->saturation;
	hue = enhance->hue;
	sharpness = enhance->sharpness;
	csc_mode = enhance->cscmode;

	if ((brightness == 50) && (contrast == 50) && (saturation == 50) &&
	    (hue == 50)) {
		if (csc_mode == HCFB_ENHANCE_CSCMODE_BT601) {
			for (i = 0; i < 12; i++)
				coef[i] = enhance_coef_default_bt601[i];
		} else {
			for (i = 0; i < 12; i++)
				coef[i] = enhance_coef_default_bt709[i];
		}
	} else {
		/* brightness, contrast, saturation value to double*/
		Ly = ((double)brightness - 50) * 64 / 50;

		/* Sy replace Sr, Sg, Sr : [0, 2] */
		if (contrast >= 50) {
			Sy = 1 + ((double)contrast - 50) / 50;
		} else {
			//Sy = -2 + ((double)contrast) * 3 / 50;
			Sy = ((double)contrast) / 50;
		}

		/* Sc : [0, 2] */
		if (saturation >= 50) {
			Sc = 1 + ((double)saturation - 50) / 50;
		} else {
			//Sc = -2 + ((double)saturation) * 3 / 50;
			Sc = ((double)saturation) / 50;
		}

		Ly = Ly + (1 - Sy) * 64;

		/* sin and cos value for hue */
		if (hue == 50) {
			sin = 0;
			cos = FRAC_ONE;
		} else if ((hue == 100) || (hue == 0)) {
			sin = 0;
			cos = INT_TO_FRAC(-1);
		} else {
			int hue_in = 0;
			double fval;
			fval = hue;
			double hue_dou = (fval / 50.0) - 1.0;

			hue_in = DOUBLE_TO_FRAC(hue_dou);
			sin = sinffn(hue_in);
			cos = cosffn(hue_in);
		}

		tmp1 = Sc * MC0_1;
		tmp2 = Sc * MC0_2;
		tmp3 = Sc * MC1_1;
		tmp4 = Sc * MC1_2;
		tmp5 = Sc * MC2_1;
		tmp6 = Sc * MC2_2;

		/* generate coef table */
		coef[0] = FLOOR(Sy * MY0);
		coef[1] = FLOOR(Sy * MY1);
		coef[2] = FLOOR(Sy * MY2);
		coef[3] = FLOOR(Ly * MY3);

		tmp7 = -tmp1 * cos + tmp2 * sin;
		tmp8 = -tmp3 * cos - tmp4 * sin;
		tmp9 = tmp5 * cos - tmp6 * sin;

		coef[4] = FLOOR(tmp7 * Sy) >> FRAC_BITS;
		coef[5] = FLOOR(tmp8 * Sy) >> FRAC_BITS;
		coef[6] = FLOOR(tmp9 * Sy) >> FRAC_BITS;
		coef[7] = FLOOR((tmp7 + tmp8 + tmp9) * Ly) >> FRAC_BITS;

		tmp7 = tmp1 * sin + tmp2 * cos;
		tmp8 = tmp3 * sin - tmp4 * cos;
		tmp9 = -tmp5 * sin - tmp6 * cos;
		coef[8] = FLOOR(tmp7 * Sy) >> FRAC_BITS;
		coef[9] = FLOOR(tmp8 * Sy) >> FRAC_BITS;
		coef[10] = FLOOR(tmp9 * Sy) >> FRAC_BITS;
		coef[11] = FLOOR((tmp7 + tmp8 + tmp9) * Ly) >> FRAC_BITS;
	}

	/* sharpness value */
	sharpness = sharpness - 5;

	if (sharpness > 5) {
		sharpness = 5;
	} else if (sharpness < -5) {
		sharpness = -5;
	}
	sharpness = sharpness & 0x0000000f;

	coef[12] = sharpness;

	return 0;
}

int generate_scale_coef(short *coef, uint32_t isamples,
			 uint32_t osamples, uint8_t tap, uint8_t ngroup,
			 bool duplicate)
{
	frac coeff_real[16 * 4 + 1];
	frac cutoffscale = DOUBLE_TO_FRAC(1.0);

	if (tap * ngroup > 16 * 4)
		return -1;

	if (osamples >= isamples) {
		/* scaleup */
		cutoffscale = DOUBLE_TO_FRAC(0.9);
	} else {
		/* scaledown */
		cutoffscale = DOUBLE_TO_FRAC(0.9);
	}

	if (duplicate) {
		cutoffscale = DOUBLE_TO_FRAC(1.0);
	}

	designfilterff(isamples, osamples, ngroup, tap, 1, cutoffscale, 8,
		       coeff_real, coef);

	return 0;
}
