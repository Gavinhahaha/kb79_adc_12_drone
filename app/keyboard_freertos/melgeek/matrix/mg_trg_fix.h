#ifndef __MG_TRG_FIX_H__
#define __MG_TRG_FIX_H__



void fix_special_point_update(uint8_t ki, key_attr_t *p_attr);
int  fix_process(uint8_t ki, uint16_t new_point);
void fix_init(kh_t *p_kh);
void fix_set_dead_para(uint8_t ki, uint8_t topDead, uint8_t bottomDead);
void fix_set_para(uint8_t ki, uint8_t triggerLevel, uint8_t raiseLevel, uint8_t mode);
void fix_set_default_dead_para(void);
void fix_set_default_ap_para(void);
void fix_check_para(key_attr_t *key_attr);

#endif

