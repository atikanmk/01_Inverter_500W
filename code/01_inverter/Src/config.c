#include "config.h"

static const inverter_config_t g_inverter_config = {
    .motor_pole_pairs = 10u,
    .speed_max_rpm = 4000u,
    .dc_max = 1u,
    .phase_max = 3u,
    .tps_perc_start = 5u,
    .current_phase_v_gain_uv_per_a = 833333u,
    .current_phase_w_gain_uv_per_a = 833333u,
    .stt_adc = {
        [EN_CONFIG_ADC_IDC] = {.u8t_adc_group = 0, .u8t_adc_pin = 5}, /* PA5 */
        [EN_CONFIG_ADC_IV] = {.u8t_adc_group = 0, .u8t_adc_pin = 6},  /* PA6 */
        [EN_CONFIG_ADC_IW] = {.u8t_adc_group = 0, .u8t_adc_pin = 7},  /* PA7 */
        [EN_CONFIG_ADC_TPS] = {.u8t_adc_group = 0, .u8t_adc_pin = 3}  /* PA3 */
    }
};

const inverter_config_t *Config_Get(void)
{
    return &g_inverter_config;
}
