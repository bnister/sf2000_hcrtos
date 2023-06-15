#ifndef _LVDS_LLD_H_
#define _LVDS_LLD_H_
#include "lvds.h"
#ifdef __cplusplus
extern "C" {
#endif


void lvds_lld_init(void *reg_base, void *sys_base, int reset);
void lvds_lld_set_cfg(T_LVDS_CFG *lvds_cfg,uint8_t ch);
void lvds_lld_phy_mode_init(uint8_t ch,uint8_t mode);
void lvds_lld_phy_ttl_mode_init(uint8_t ch);

#ifdef __cplusplus
}
#endif


#endif


