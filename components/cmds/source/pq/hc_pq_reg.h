#ifndef _HC_PQ_REG_H_
#define _HC_PQ_REG_H_
#endif

#ifdef __cplusplus
extern "c" {
#endif

#include <stdint.h>
#include <unistd.h>

#define PQ_REG_BASE_ADDR (0xB8836000)
#define PQ_REG_CNTR               0x00
#define PQ_REG_LINEAR             0x10
#define PQ_REG_MTX_C00_C01        0x20
#define PQ_REG_MTX_C02_C10        0x24
#define PQ_REG_MTX_C11_C12        0x28
#define PQ_REG_MTX_C20_C21        0x2C
#define PQ_REG_MTX_C30            0x30
#define PQ_REG_GNTF_GAIN          0x34
#define PQ_REG_GNTF_OFST          0x38
#define PQ_REG_SLMT               0x40
#define PQ_REG_INVGMAMA_LUT_R     0x60
#define PQ_REG_INVGMAMA_LUT_G     0x64
#define PQ_REG_INVGMAMA_LUT_B     0x68
#define PQ_REG_GMAMA_LUT_R        0x70
#define PQ_REG_GMAMA_LUT_G        0x74
#define PQ_REG_GMAMA_LUT_B        0x78

    typedef struct _T_PQ_CNTR_
    {
        uint32_t pq_en : 1;
        uint32_t igam_en : 1;
        uint32_t mtx_en : 1;
        uint32_t gntf_en : 1;
        uint32_t slmt_en : 1;
        uint32_t gam_en : 1;
    }T_PQ_CNTR;

    typedef struct _T_PQ_LINEAR_
    {
        uint32_t linear_slope : 8;
        uint32_t linear_region : 8;
        uint32_t reserve : 15;
        uint32_t linear_region_enable : 1;
    }T_PQ_LINEAR;

    typedef struct _T_PQ_MTX_C00_C01_
    {
        uint32_t mtx_c00 : 13;
        uint32_t reserve0 : 3;
        uint32_t mtx_c01 : 13;
        uint32_t reserve1 : 3;
    }T_PQ_MTX_C00_C01;

    typedef struct _T_PQ_MTX_C02_C10_
    {
        uint32_t mtx_c02 : 13;
        uint32_t reserve0 : 3;
        uint32_t mtx_c10 : 13;
        uint32_t reserve1 : 3;
    }T_PQ_MTX_C02_C10;

    typedef struct _T_PQ_MTX_C11_C12_
    {
        uint32_t mtx_c11 : 13;
        uint32_t reserve0 : 3;
        uint32_t mtx_c12 : 13;
        uint32_t reserve1 : 3;
    }T_PQ_MTX_C11_C12;

    typedef struct _T_PQ_MTX_C20_C21_
    {
        uint32_t mtx_c20 : 13;
        uint32_t reserve0 : 3;
        uint32_t mtx_c21 : 13;
        uint32_t reserve1 : 3;
    }T_PQ_MTX_C20_C21;

    typedef struct _T_PQ_MTX_C22_
    {
        uint32_t mtx_c22 : 13;
        uint32_t reserve0 : 19;
    }T_PQ_MTX_C22;

    typedef struct _T_PQ_GNTF_GAIN_
    {
        uint32_t  gntf_gain_r : 8;
        uint32_t  gntf_gain_g : 8;
        uint32_t  gntf_gain_b : 8;
        uint32_t  reserve0 : 8;
    }T_PQ_GNTF_GAIN;

    typedef struct _T_PQ_GNTF_OFST_
    {
        uint32_t  gntf_ofst_r : 8;
        uint32_t  gntf_ofst_g : 8;
        uint32_t  gntf_ofst_b : 8;
        uint32_t  reserve0 : 8;
    }T_PQ_GNTF_OFST;

    typedef struct _T_PQ_SLMT_
    {
        uint32_t    slmt_delta : 8;
        uint32_t  	slmt_slope : 7;
        uint32_t    reserve0 : 1;
        uint32_t  	slmt_neg_oftgn : 4;
        uint32_t    reserve1 : 10;
        uint32_t  	slmt_max_en : 1;
        uint32_t    slmt_neg_en : 1;
    }T_PQ_SLMT;

    typedef struct _T_PQ_INVGMAMA_LUT_R_
    {
        uint32_t    invgamma_lut_r_data : 24;
        uint32_t  	invgamma_lut_r_addr : 6;
        uint32_t    reserve0 : 1;
        uint32_t    invgamma_lut_r_wen : 1;
    }T_PQ_INVGMAMA_LUT_R;

    typedef struct _T_PQ_INVGMAMA_LUT_G_
    {
        uint32_t    invgamma_lut_g_data : 24;
        uint32_t  	invgamma_lut_g_addr : 6;
        uint32_t    reserve0 : 1;
        uint32_t    invgamma_lut_g_wen : 1;
    }T_PQ_INVGMAMA_LUT_G;

    typedef struct _T_PQ_INVGMAMA_LUT_B_
    {
        uint32_t    invgamma_lut_b_data : 24;
        uint32_t  	invgamma_lut_b_addr : 6;
        uint32_t    reserve0 : 1;
        uint32_t    invgamma_lut_b_wen : 1;
    }T_PQ_INVGMAMA_LUT_B;

    typedef struct _T_PQ_GMAMA_LUT_R_
    {
        uint32_t    gamma_lut_r_data : 24;
        uint32_t  	gamma_lut_r_addr : 7;
        uint32_t    gamma_lut_r_wen : 1;
    }T_PQ_GMAMA_LUT_R;

    typedef struct _T_PQ_GMAMA_LUT_G_
    {
        uint32_t    gamma_lut_g_data : 24;
        uint32_t  	gamma_lut_g_addr : 7;
        uint32_t    gamma_lut_g_wen : 1;
    }T_PQ_GMAMA_LUT_G;

    typedef struct _T_PQ_GMAMA_LUT_B_
    {
        uint32_t    gamma_lut_b_data : 24;
        uint32_t  	gamma_lut_b_addr : 7;
        uint32_t    gamma_lut_b_wen : 1;
    }T_PQ_GMAMA_LUT_B;



    // void __pq_hal_init(uint8_t *reg_base);
    void pq_hal_init(void);


    /* reg 0x00 */
    void pq_hal_set_en(uint32_t enable);
    void pq_hal_set_igam_en(uint32_t enable);
    void pq_hal_set_mtx_en(uint32_t enable);
    void pq_hal_set_gntf_en(uint32_t enable);
    void pq_hal_set_slmt_en(uint32_t enable);
    void pq_hal_set_gam_en(uint32_t enable);

    /* reg 0x10 */
    void pq_hal_set_linear_en(uint32_t enable);
    void pq_hal_set_linear_region(uint32_t region);
    void pq_hal_set_linear_slope(uint32_t slope);

    /* reg 0x20 ~ 0x30 */
    void pq_hal_set_mtx_coef(uint32_t coef_00 , uint32_t coef_01 , uint32_t coef_02 ,
                             uint32_t coef_10 , uint32_t coef_11 , uint32_t coef_12 ,
                             uint32_t coef_20 , uint32_t coef_21 , uint32_t coef_22);

    /* reg 0x30 & 0x34 */
    void pq_hal_set_gntf_gain(uint32_t gain_r , uint32_t gain_g , uint32_t gain_b);
    void pq_hal_set_gntf_ofst(uint32_t ofst_r , uint32_t ofst_g , uint32_t ofst_b);

    /* reg 0x40 */
    void pq_hal_set_slmt_neg_en(uint32_t enable);
    void pq_hal_set_slmt_max_en(uint32_t enable);
    void pq_hal_set_slmt_neg_oftgn(uint32_t slmt_neg_oftgn);
    void pq_hal_set_slmt_slope(uint32_t slmt_slope);
    void pq_hal_set_slmt_delta(uint32_t slmt_delta);

    /* reg 0x60/0x64/0x68 */
    void pq_hal_write_invgamma_lut_r_data(uint32_t data);
    void pq_hal_write_invgamma_lut_g_data(uint32_t data);
    void pq_hal_write_invgamma_lut_b_data(uint32_t data);

    /* reg 0x70/0x74/0x78 */
    void pq_hal_write_gamma_lut_r_data(uint32_t data);
    void pq_hal_write_gamma_lut_g_data(uint32_t data);
    void pq_hal_write_gamma_lut_b_data(uint32_t data);

#ifdef __cplusplus
}
#endif
