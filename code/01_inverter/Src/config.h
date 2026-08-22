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
    uint8_t motor_pole_pairs;
    uint16_t speed_max_rpm;
    uint16_t dc_max;
    uint16_t phase_max;
    uint8_t tps_perc_start;
    uint32_t current_phase_v_gain_uv_per_a;
    uint32_t current_phase_w_gain_uv_per_a;
    ST_CONFIG_ADC stt_adc[EN_CONFIG_ADC_COUNT];
} inverter_config_t;

const inverter_config_t *Config_Get(void);

#ifdef __cplusplus
}
#endif

#endif /* CONFIG_H */
