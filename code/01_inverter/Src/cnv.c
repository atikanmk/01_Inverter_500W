#include "cnv.h"

#include "adc.h"
#include "config.h"

#define CNV_ADC_REFERENCE_MV 3300u

static volatile uint16_t g_cnv_phase_v_mv;
static volatile uint16_t g_cnv_phase_w_mv;
static volatile uint16_t g_cnv_phase_v_offset_mv;
static volatile uint16_t g_cnv_phase_w_offset_mv;
static volatile int32_t g_cnv_phase_u_current_ca;
static volatile int32_t g_cnv_phase_v_current_ca;
static volatile int32_t g_cnv_phase_w_current_ca;
static volatile uint32_t g_cnv_phase_v_offset_sum_mv;
static volatile uint32_t g_cnv_phase_w_offset_sum_mv;
static volatile uint16_t g_cnv_offset_sample_count;
static volatile uint8_t g_cnv_offset_ready;

static uint16_t cnv_adc_raw_to_mv(uint16_t raw_value)
{
    return (uint16_t)(((uint32_t)raw_value * CNV_ADC_REFERENCE_MV) / ADC_RAW_MAX_12BIT);
}

static void cnv_update_adc_values(void)
{
    u16 raw_array[EN_CONFIG_ADC_COUNT];

    if (ADC_ReadAll12bit(raw_array) == EN_COM_STS_OK)
    {
        g_cnv_phase_v_mv = cnv_adc_raw_to_mv(raw_array[EN_CONFIG_ADC_IV]);
        g_cnv_phase_w_mv = cnv_adc_raw_to_mv(raw_array[EN_CONFIG_ADC_IW]);
    }
}

EN_COM_STS_T eng_cnv_curr(void);

static int32_t cnv_voltage_to_current_ca(int32_t delta_mv, uint32_t gain_uv_per_a)
{
    if (gain_uv_per_a == 0u)
    {
        return 0;
    }

    return (int32_t)(((int64_t)delta_mv * 100000LL) / (int64_t)gain_uv_per_a);
}

void CNV_Init(void)
{
    g_cnv_phase_v_mv = 0u;
    g_cnv_phase_w_mv = 0u;
    g_cnv_phase_v_offset_mv = 0u;
    g_cnv_phase_w_offset_mv = 0u;
    g_cnv_phase_u_current_ca = 0;
    g_cnv_phase_v_current_ca = 0;
    g_cnv_phase_w_current_ca = 0;
    g_cnv_phase_v_offset_sum_mv = 0u;
    g_cnv_phase_w_offset_sum_mv = 0u;
    g_cnv_offset_sample_count = 0u;
    g_cnv_offset_ready = 0u;
}


EN_COM_STS_T eng_cnv_curr_offset(void)
{
    EN_COM_STS_T ent_status = EN_COM_STS_OK;

     /* Read ADC  */

    if (g_cnv_offset_ready == 0u)
    {
        g_cnv_phase_v_offset_sum_mv += g_cnv_phase_v_mv;
        g_cnv_phase_w_offset_sum_mv += g_cnv_phase_w_mv;
        g_cnv_offset_sample_count++;

        if (g_cnv_offset_sample_count >= CNV_OFFSET_SAMPLE_COUNT)
        {
            g_cnv_phase_v_offset_mv = (uint16_t)(g_cnv_phase_v_offset_sum_mv / CNV_OFFSET_SAMPLE_COUNT);
            g_cnv_phase_w_offset_mv = (uint16_t)(g_cnv_phase_w_offset_sum_mv / CNV_OFFSET_SAMPLE_COUNT);
            g_cnv_offset_ready = 1u;
            g_cnv_phase_v_offset_sum_mv = 0;
            g_cnv_phase_w_offset_sum_mv = 0;
            g_cnv_offset_sample_count = 0;

            ent_status = EN_COM_STS_OK;
        }
        else
        {
            /* Do nothing */
            ent_status = EN_COM_STS_RUNNING;
        }
    }
    else
    {
        g_cnv_phase_v_offset_sum_mv = 0;
        g_cnv_phase_w_offset_sum_mv = 0;
        g_cnv_offset_sample_count = 0;

        ent_status = EN_COM_STS_OK;
    }

    return ent_status;
}


void CNV_On100us(void)
{
    cnv_update_adc_values();
    eng_cnv_curr_offset();
     eng_cnv_curr();


}
EN_COM_STS_T eng_cnv_curr(void)
{
    const inverter_config_t *cfg;
    int32_t phase_v_delta_mv;
    int32_t phase_w_delta_mv;

    cfg = Config_Get();

    phase_v_delta_mv = (int32_t)g_cnv_phase_v_mv - (int32_t)g_cnv_phase_v_offset_mv;
    phase_w_delta_mv = (int32_t)g_cnv_phase_w_mv - (int32_t)g_cnv_phase_w_offset_mv;

    g_cnv_phase_v_current_ca = cnv_voltage_to_current_ca(phase_v_delta_mv, cfg->current_phase_v_gain_uv_per_a);
    g_cnv_phase_w_current_ca = cnv_voltage_to_current_ca(phase_w_delta_mv, cfg->current_phase_w_gain_uv_per_a);
    g_cnv_phase_u_current_ca = -(g_cnv_phase_v_current_ca + g_cnv_phase_w_current_ca);
    return EN_COM_STS_OK;
}


EN_COM_STS_T eng_cnv_get_phase_currents(int32_t *phase_u_ca, int32_t *phase_v_ca, int32_t *phase_w_ca)
{
    *phase_u_ca = g_cnv_phase_u_current_ca;
    *phase_v_ca = g_cnv_phase_v_current_ca;
    *phase_w_ca = g_cnv_phase_w_current_ca;

    return EN_COM_STS_OK;
}
