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
    u8 u8t_adc_group;
    u8 u8t_adc_pin;
}ST_CONFIG_ADC;
typedef struct
{
    u8 u8t_motor_pole_pairs;
    u16 u16t_speed_max_rpm;
    u16 u16t_dc_max;
    u16 u16t_phase_max;
    u8 u8t_tps_perc_start;
    u32 u32t_current_phase_v_gain_mv_per_ca;
    u32 u32t_current_phase_w_gain_mv_per_ca;
    ST_CONFIG_ADC stt_adc[EN_CONFIG_ADC_COUNT];
} ST_INVERTER_CONFIG;

const ST_INVERTER_CONFIG *Config_Get(void);

#ifdef __cplusplus
}
#endif

#endif /* CONFIG_H */
