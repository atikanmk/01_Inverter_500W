#include "config.h"

static const ST_INVERTER_CONFIG g_inverter_config = {
    .u8t_motor_pole_pairs = 10u,
    .u16t_speed_max_rpm = 4000u,
    .u16t_dc_max = 1u,
    .u16t_phase_max = 3u,
    .u8t_tps_perc_start = 5u,
    .u32t_current_phase_v_gain_mv_per_ca = 833333u,
    .u32t_current_phase_w_gain_mv_per_ca = 833333u,
    .stt_adc = {
        [EN_CONFIG_ADC_IDC] = {.u8t_adc_group = 0, .u8t_adc_pin = 5}, /* PA5 */
        [EN_CONFIG_ADC_IV] = {.u8t_adc_group = 0, .u8t_adc_pin = 6},  /* PA6 */
        [EN_CONFIG_ADC_IW] = {.u8t_adc_group = 0, .u8t_adc_pin = 7},  /* PA7 */
        [EN_CONFIG_ADC_TPS] = {.u8t_adc_group = 0, .u8t_adc_pin = 3}  /* PA3 */
    }
};

const ST_INVERTER_CONFIG *Config_Get(void)
{
    return &g_inverter_config;
}
