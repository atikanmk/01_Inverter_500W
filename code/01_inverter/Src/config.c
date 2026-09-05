#include "config.h"

static const ST_INVERTER_CONFIG g_inverter_config = {
    .u1t_motor_pole_pairs = 10u,
    .u2t_speed_max_rpm = 4000u,
    .u2t_dc_max = 1u,
    .u2t_phase_max = 3u,
    .u1t_tps_perc_start = 5u,
    .u4t_curr_ph_v_gain_mv_per_ca = 1200,
    .u4t_curr_ph_w_gain_mv_per_ca = 1200,
    .stt_adc = {
        [EN_CONFIG_ADC_IDC] = {.u1t_adc_group = 0, .u1t_adc_pin = 5}, /* PA5 */
        [EN_CONFIG_ADC_IV] = {.u1t_adc_group = 0, .u1t_adc_pin = 6},  /* PA6 */
        [EN_CONFIG_ADC_IW] = {.u1t_adc_group = 0, .u1t_adc_pin = 7},  /* PA7 */
        [EN_CONFIG_ADC_TPS] = {.u1t_adc_group = 0, .u1t_adc_pin = 3}  /* PA3 */
    }
};

const ST_INVERTER_CONFIG *Config_Get(void)
{
    return &g_inverter_config;
}
