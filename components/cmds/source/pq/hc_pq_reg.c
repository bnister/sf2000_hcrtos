#include "hc_pq_reg.h"

typedef struct _T_PQ_HAL_
{
    volatile  T_PQ_CNTR *reg_pq_cntr;
    volatile  T_PQ_LINEAR *reg_pq_linear;
    volatile  T_PQ_MTX_C00_C01 *reg_mtx_c00_c01;
    volatile  T_PQ_MTX_C02_C10 *reg_mtx_c02_c10;
    volatile  T_PQ_MTX_C11_C12 *reg_mtx_c11_c12;
    volatile  T_PQ_MTX_C20_C21 *reg_mtx_c20_c21;
    volatile  T_PQ_MTX_C22 *reg_mtx_c22;
    volatile  T_PQ_GNTF_GAIN *reg_gntf_gain;
    volatile  T_PQ_GNTF_OFST *reg_gntf_ofst;
    volatile  T_PQ_SLMT *reg_pq_slmt;
    volatile  T_PQ_INVGMAMA_LUT_R *reg_pq_invgmama_lut_r;
    volatile  T_PQ_INVGMAMA_LUT_G *reg_pq_invgmama_lut_g;
    volatile  T_PQ_INVGMAMA_LUT_B *reg_pq_invgmama_lut_b;
    volatile  T_PQ_GMAMA_LUT_R *reg_pq_gmama_lut_r;
    volatile  T_PQ_GMAMA_LUT_G *reg_pq_gmama_lut_g;
    volatile  T_PQ_GMAMA_LUT_B *reg_pq_gmama_lut_b;
}T_PQ_HAL;

T_PQ_HAL pq_hal_instant;



void pq_hal_init(void)
{
    pq_hal_instant.reg_pq_cntr = (volatile  T_PQ_CNTR *)(PQ_REG_BASE_ADDR + PQ_REG_CNTR);
    pq_hal_instant.reg_pq_linear = (volatile  T_PQ_LINEAR *)(PQ_REG_BASE_ADDR + PQ_REG_LINEAR);
    pq_hal_instant.reg_mtx_c00_c01 = (volatile  T_PQ_MTX_C00_C01 *)(PQ_REG_BASE_ADDR + PQ_REG_MTX_C00_C01);
    pq_hal_instant.reg_mtx_c02_c10 = (volatile  T_PQ_MTX_C02_C10 *)(PQ_REG_BASE_ADDR + PQ_REG_MTX_C02_C10);
    pq_hal_instant.reg_mtx_c11_c12 = (volatile  T_PQ_MTX_C11_C12 *)(PQ_REG_BASE_ADDR + PQ_REG_MTX_C11_C12);
    pq_hal_instant.reg_mtx_c20_c21 = (volatile  T_PQ_MTX_C20_C21 *)(PQ_REG_BASE_ADDR + PQ_REG_MTX_C20_C21);
    pq_hal_instant.reg_mtx_c22 = (volatile  T_PQ_MTX_C22 *)(PQ_REG_BASE_ADDR + PQ_REG_MTX_C30);
    pq_hal_instant.reg_gntf_gain = (volatile  T_PQ_GNTF_GAIN *)(PQ_REG_BASE_ADDR + PQ_REG_GNTF_GAIN);
    pq_hal_instant.reg_gntf_ofst = (volatile  T_PQ_GNTF_OFST *)(PQ_REG_BASE_ADDR + PQ_REG_GNTF_OFST);
    pq_hal_instant.reg_pq_slmt = (volatile  T_PQ_SLMT *)(PQ_REG_BASE_ADDR + PQ_REG_SLMT);
    pq_hal_instant.reg_pq_invgmama_lut_r = (volatile  T_PQ_INVGMAMA_LUT_R *)(PQ_REG_BASE_ADDR + PQ_REG_INVGMAMA_LUT_R);
    pq_hal_instant.reg_pq_invgmama_lut_g = (volatile  T_PQ_INVGMAMA_LUT_G *)(PQ_REG_BASE_ADDR + PQ_REG_INVGMAMA_LUT_G);
    pq_hal_instant.reg_pq_invgmama_lut_b = (volatile  T_PQ_INVGMAMA_LUT_B *)(PQ_REG_BASE_ADDR + PQ_REG_INVGMAMA_LUT_B);
    pq_hal_instant.reg_pq_gmama_lut_r = (volatile  T_PQ_GMAMA_LUT_R *)(PQ_REG_BASE_ADDR + PQ_REG_GMAMA_LUT_R);
    pq_hal_instant.reg_pq_gmama_lut_g = (volatile  T_PQ_GMAMA_LUT_G *)(PQ_REG_BASE_ADDR + PQ_REG_GMAMA_LUT_G);
    pq_hal_instant.reg_pq_gmama_lut_b = (volatile  T_PQ_GMAMA_LUT_B *)(PQ_REG_BASE_ADDR + PQ_REG_GMAMA_LUT_B);
    return;
}


// void __pq_hal_init(uint8_t *reg_base)
// {
//     pq_hal_instant.reg_pq_cntr = (volatile  T_PQ_CNTR *)(reg_base + PQ_REG_CNTR);
//     pq_hal_instant.reg_pq_linear = (volatile  T_PQ_LINEAR *)(reg_base + PQ_REG_LINEAR);
//     pq_hal_instant.reg_mtx_c00_c01 = (volatile  T_PQ_MTX_C00_C01 *)(reg_base + PQ_REG_MTX_C00_C01);
//     pq_hal_instant.reg_mtx_c02_c10 = (volatile  T_PQ_MTX_C02_C10 *)(reg_base + PQ_REG_MTX_C02_C10);
//     pq_hal_instant.reg_mtx_c11_c12 = (volatile  T_PQ_MTX_C11_C12 *)(reg_base + PQ_REG_MTX_C11_C12);
//     pq_hal_instant.reg_mtx_c20_c21 = (volatile  T_PQ_MTX_C20_C21 *)(reg_base + PQ_REG_MTX_C20_C21);
//     pq_hal_instant.reg_mtx_c22 = (volatile  T_PQ_MTX_C22 *)(reg_base + PQ_REG_MTX_C30);
//     pq_hal_instant.reg_gntf_gain = (volatile  T_PQ_GNTF_GAIN *)(reg_base + PQ_REG_GNTF_GAIN);
//     pq_hal_instant.reg_gntf_ofst = (volatile  T_PQ_GNTF_OFST *)(reg_base + PQ_REG_GNTF_OFST);
//     pq_hal_instant.reg_pq_slmt = (volatile  T_PQ_SLMT *)(reg_base + PQ_REG_SLMT);
//     pq_hal_instant.reg_pq_invgmama_lut_r = (volatile  T_PQ_INVGMAMA_LUT_R *)(reg_base + PQ_REG_INVGMAMA_LUT_R);
//     pq_hal_instant.reg_pq_invgmama_lut_g = (volatile  T_PQ_INVGMAMA_LUT_G *)(reg_base + PQ_REG_INVGMAMA_LUT_G);
//     pq_hal_instant.reg_pq_invgmama_lut_b = (volatile  T_PQ_INVGMAMA_LUT_B *)(reg_base + PQ_REG_INVGMAMA_LUT_B);
//     pq_hal_instant.reg_pq_gmama_lut_r = (volatile  T_PQ_GMAMA_LUT_R *)(reg_base + PQ_REG_GMAMA_LUT_R);
//     pq_hal_instant.reg_pq_gmama_lut_g = (volatile  T_PQ_GMAMA_LUT_G *)(reg_base + PQ_REG_GMAMA_LUT_G);
//     pq_hal_instant.reg_pq_gmama_lut_b = (volatile  T_PQ_GMAMA_LUT_B *)(reg_base + PQ_REG_GMAMA_LUT_B);
//     return;
// }


void pq_hal_set_en(uint32_t enable)
{
    pq_hal_instant.reg_pq_cntr->pq_en = enable;
}

void pq_hal_set_igam_en(uint32_t enable)
{
    pq_hal_instant.reg_pq_cntr->igam_en = enable;
}

void pq_hal_set_mtx_en(uint32_t enable)
{
    pq_hal_instant.reg_pq_cntr->mtx_en = enable;
}

void pq_hal_set_gntf_en(uint32_t enable)
{
    pq_hal_instant.reg_pq_cntr->gntf_en = enable;
}

void pq_hal_set_slmt_en(uint32_t enable)
{
    pq_hal_instant.reg_pq_cntr->slmt_en = enable;
}

void pq_hal_set_gam_en(uint32_t enable)
{
    pq_hal_instant.reg_pq_cntr->gam_en = enable;
}

void pq_hal_set_linear_en(uint32_t enable)
{
    pq_hal_instant.reg_pq_linear->linear_region_enable = enable;
}

void pq_hal_set_linear_region(uint32_t region)
{
    pq_hal_instant.reg_pq_linear->linear_region = region;
}

void pq_hal_set_linear_slope(uint32_t slope)
{
    pq_hal_instant.reg_pq_linear->linear_slope = slope;
}

void pq_hal_set_mtx_coef(uint32_t coef_00 , uint32_t coef_01 , uint32_t coef_02 ,
                         uint32_t coef_10 , uint32_t coef_11 , uint32_t coef_12 ,
                         uint32_t coef_20 , uint32_t coef_21 , uint32_t coef_22)
{
    pq_hal_instant.reg_mtx_c00_c01->mtx_c00 = coef_00;
    pq_hal_instant.reg_mtx_c00_c01->mtx_c01 = coef_01;
    pq_hal_instant.reg_mtx_c02_c10->mtx_c02 = coef_02;

    pq_hal_instant.reg_mtx_c02_c10->mtx_c10 = coef_10;
    pq_hal_instant.reg_mtx_c11_c12->mtx_c11 = coef_11;
    pq_hal_instant.reg_mtx_c11_c12->mtx_c12 = coef_12;

    pq_hal_instant.reg_mtx_c20_c21->mtx_c20 = coef_20;
    pq_hal_instant.reg_mtx_c20_c21->mtx_c21 = coef_21;
    pq_hal_instant.reg_mtx_c22->mtx_c22 = coef_22;
}

void pq_hal_set_gntf_gain(uint32_t gain_r , uint32_t gain_g , uint32_t gain_b)
{
    pq_hal_instant.reg_gntf_gain->gntf_gain_r = gain_r;
    pq_hal_instant.reg_gntf_gain->gntf_gain_g = gain_g;
    pq_hal_instant.reg_gntf_gain->gntf_gain_b = gain_b;
}

void pq_hal_set_gntf_ofst(uint32_t ofst_r , uint32_t ofst_g , uint32_t ofst_b)
{
    pq_hal_instant.reg_gntf_ofst->gntf_ofst_r = ofst_r;
    pq_hal_instant.reg_gntf_ofst->gntf_ofst_g = ofst_g;
    pq_hal_instant.reg_gntf_ofst->gntf_ofst_b = ofst_b;
}

void pq_hal_set_slmt_neg_en(uint32_t enable)
{
    pq_hal_instant.reg_pq_slmt->slmt_neg_en = enable;
}

void pq_hal_set_slmt_max_en(uint32_t enable)
{
    pq_hal_instant.reg_pq_slmt->slmt_max_en = enable;
}

void pq_hal_set_slmt_neg_oftgn(uint32_t slmt_neg_oftgn)
{
    pq_hal_instant.reg_pq_slmt->slmt_neg_oftgn = slmt_neg_oftgn;
}

void pq_hal_set_slmt_slope(uint32_t slmt_slope)
{
    pq_hal_instant.reg_pq_slmt->slmt_slope = slmt_slope;
}

void pq_hal_set_slmt_delta(uint32_t slmt_delta)
{
    pq_hal_instant.reg_pq_slmt->slmt_delta = slmt_delta;
}

void pq_hal_write_invgamma_lut_r_data(uint32_t data)
{
    *(uint32_t *)(pq_hal_instant.reg_pq_invgmama_lut_r) = data;
}

void pq_hal_write_invgamma_lut_g_data(uint32_t data)
{
    *(uint32_t *)(pq_hal_instant.reg_pq_invgmama_lut_g) = data;
}

void pq_hal_write_invgamma_lut_b_data(uint32_t data)
{
    *(uint32_t *)(pq_hal_instant.reg_pq_invgmama_lut_b) = data;
}

void pq_hal_write_gamma_lut_r_data(uint32_t data)
{
    *(uint32_t *)(pq_hal_instant.reg_pq_gmama_lut_r) = data;
}

void pq_hal_write_gamma_lut_g_data(uint32_t data)
{
    *(uint32_t *)(pq_hal_instant.reg_pq_gmama_lut_g) = data;
}

void pq_hal_write_gamma_lut_b_data(uint32_t data)
{
    *(uint32_t *)(pq_hal_instant.reg_pq_gmama_lut_b) = data;
}
