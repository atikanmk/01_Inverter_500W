#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>
#include "com.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    EN_CONFIG_ADC_IDC = 0,
    EN_CONFIG_ADC_IV = 1,
    EN_CONFIG_ADC_IW = 2,
    EN_CONFIG_ADC_TPS = 3,
    EN_CONFIG_ADC_COUNT
} EN_CONFIG_ADC_T;
typedef struct
{
    u1 u1t_adc_group;
    u1 u1t_adc_pin;
}ST_CONFIG_ADC;
typedef struct
{
    u1 u1t_motor_pole_pairs;
    u2 u2t_speed_max_rpm;
    u2 u2t_dc_max;
    u2 u2t_phase_max;
    u1 u1t_tps_perc_start;
    u4 u4t_curr_ph_v_gain_mv_per_ca;
    u4 u4t_curr_ph_w_gain_mv_per_ca;
    ST_CONFIG_ADC stt_adc[EN_CONFIG_ADC_COUNT];
} ST_INVERTER_CONFIG;

const ST_INVERTER_CONFIG *Config_Get(void);

#ifdef __cplusplus
}
#endif

#endif /* CONFIG_H */
