#ifndef _HC_PQ_H_
#define _HC_PQ_H_

#include <hcuapi/iocbase.h>
#include <stdint.h>
#include <unistd.h>
#include <stdbool.h>

struct pq_settings {
	char *name;
	bool pq_en;
	bool igam_en;
	bool mtx_en;
	bool gntf_en;
	bool slmt_en;
	bool gam_en;
	// 0x10
	bool linear_region_enable;
	unsigned int linear_region;
	unsigned int linear_slope;
	// 0x20
	int mtx_c00;
	int mtx_c01;
	// 0x24
	int mtx_c02;
	int mtx_c10;
	// 0x28
	int mtx_c11;
	int mtx_c12;
	// 0x2c
	int mtx_c20;
	int mtx_c21;
	// 0x30
	int mtx_c22;
	// 0x34
	unsigned int gntf_gain_r;
	unsigned int gntf_gain_g;
	unsigned int gntf_gain_b;
	// 0x38
	int gntf_offset_r;
	int gntf_offset_g;
	int gntf_offset_b;
	// 0x40
	bool slmt_neg_en;
	bool slmt_max_en;
	unsigned int slmt_neg_oftgn;
	unsigned int slmt_slope;
	int slmt_delta;
	// 0x60
	int invgamma_lut_r[67];
	int invgamma_lut_g[67];
	int invgamma_lut_b[67];
	// 0x70
	int gamma_lut_r[131];
	int gamma_lut_g[131];
	int gamma_lut_b[131];
};

#define PQ_START		_IO (PQ_IOCBASE, 0)
#define PQ_STOP			_IO (PQ_IOCBASE, 1)
#define PQ_SET_PARAM		_IOW(PQ_IOCBASE, 2, struct pq_settings)

#endif
