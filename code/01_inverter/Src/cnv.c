/**
 ********************************************************************************
 * @file    cnv.c
 * @author  Atikan
 * @date    2026-08-23
 * @brief   
 ********************************************************************************
 */

/************************************
 * INCLUDES
 ************************************/
#include "cnv.h"
#include "adc.h"
#include "config.h"

/************************************
 * EXTERN VARIABLES
 ************************************/

/************************************
 * PRIVATE MACROS AND DEFINES
 ************************************/
#define CNV_ADC_REFERENCE_MV 3300u

/************************************
 * PRIVATE TYPEDEFS
 ************************************/

/************************************
 * STATIC VARIABLES
 ************************************/
static u2 u16gv_cnv_phase_v_mv;
static u2 u16gv_cnv_phase_w_mv;
static u2 u16gv_cnv_phase_v_offset_mv;
static u2 u16gv_cnv_phase_w_offset_mv;
static s4 s32gv_cnv_phase_u_current_ca;
static s4 s32gv_cnv_phase_v_current_ca;
static s4 s32gv_cnv_phase_w_current_ca;
static u4 u32gv_cnv_phase_v_offset_sum_mv;
static u4 u32gv_cnv_phase_w_offset_sum_mv;
static u2 u16gv_cnv_offset_sample_count;
static u1 u8gv_cnv_offset_ready;

/************************************
 * GLOBAL VARIABLES
 ************************************/

/************************************
 * STATIC FUNCTION PROTOTYPES
 ************************************/
static uint16_t cnv_adc_raw_to_mv(uint16_t raw_value);
static EN_COM_STS_T ens_cnv_update_adc_values(void);
static EN_COM_STS_T ens_cnv_voltage_to_current_ca(u4 u32t_delta_mv, u4 u32t_gain_mv_per_ca,s4* s32t_out);
static EN_COM_STS_T ens_cnv_curr_offset(void);
static EN_COM_STS_T ens_cnv_curr(void);

/************************************
 * STATIC FUNCTIONS
 ************************************/
/**
 * @fn     cnv_adc_raw_to_mv
 * @id     CNV-001
 * @brief  Convert 12-bit ADC raw value to millivolts
 * @param  raw_value: 12-bit ADC raw value
 * @return Voltage in millivolts
 */
static uint16_t cnv_adc_raw_to_mv(uint16_t raw_value)
{
    return (uint16_t)(((uint32_t)raw_value * CNV_ADC_REFERENCE_MV) / ADC_RAW_MAX_12BIT);
}

/**
 * @fn     ens_cnv_update_adc_values
 * @id     CNV-002
 * @brief  Read ADC and update phase voltage raw values
 * @param  None
 * @return EN_COM_STS_OK
 */
static EN_COM_STS_T ens_cnv_update_adc_values(void)
{
    u2 raw_array[EN_CONFIG_ADC_COUNT];
    /*==========INPUT==========*/
    /*========OPERATION========*/
    if (ADC_ReadAll12bit(raw_array) == EN_COM_STS_OK)
    {
    /*=========OUTPUT==========*/
        u16gv_cnv_phase_v_mv = cnv_adc_raw_to_mv(raw_array[EN_CONFIG_ADC_IV]);
        u16gv_cnv_phase_w_mv = cnv_adc_raw_to_mv(raw_array[EN_CONFIG_ADC_IW]);
    }

    return EN_COM_STS_OK;
}

/**
 * @fn     cnv_voltage_to_current_ca
 * @id     CNV-003
 * @brief  Convert voltage delta to current in centi amperes
 * @param  u32t_delta_mv:  Voltage delta in millivolts (u32)
 * @param  u32t_gain_mv_per_ca:  Sensor gain in millivolts per centi ampere (u32)
 * @return Current in centi amperes
 */
static EN_COM_STS_T ens_cnv_voltage_to_current_ca(u4 u32t_delta_mv, u4 u32t_gain_mv_per_ca,s4* s32t_out)
{
    EN_COM_STS_T ent_ret;
   /*==========INPUT==========*/
    /*========OPERATION========*/
    if (u32t_gain_mv_per_ca == 0u)
    {
        ent_ret = EN_COM_STS_ERR;
    }
    else
    {
    /*=========OUTPUT==========*/
        s32t_out[0] = (s4)((u32t_delta_mv * u32t_gain_mv_per_ca) / 1000u);
        ent_ret = EN_COM_STS_OK;
    }

    return ent_ret;
}

/**
 * @fn     ens_cnv_curr_offset
 * @id     CNV-004
 * @brief  Accumulate ADC samples to calculate current sensor offset
 * @param  None
 * @return EN_COM_STS_OK when offset is ready, EN_COM_STS_RUNNING while sampling
 */
static EN_COM_STS_T ens_cnv_curr_offset(void)
{
    EN_COM_STS_T ent_status = EN_COM_STS_OK;

    /* Read ADC  */
    /*==========INPUT==========*/
    /*========OPERATION========*/
    if (u8gv_cnv_offset_ready == 0u)
    {
        u32gv_cnv_phase_v_offset_sum_mv += u16gv_cnv_phase_v_mv;
        u32gv_cnv_phase_w_offset_sum_mv += u16gv_cnv_phase_w_mv;
        u16gv_cnv_offset_sample_count++;

        if (u16gv_cnv_offset_sample_count >= CNV_OFFSET_SAMPLE_COUNT)
        {
            u16gv_cnv_phase_v_offset_mv = (u2)(u32gv_cnv_phase_v_offset_sum_mv / CNV_OFFSET_SAMPLE_COUNT);
            u16gv_cnv_phase_w_offset_mv = (u2)(u32gv_cnv_phase_w_offset_sum_mv / CNV_OFFSET_SAMPLE_COUNT);
            u8gv_cnv_offset_ready = 1u;
            u32gv_cnv_phase_v_offset_sum_mv = 0;
            u32gv_cnv_phase_w_offset_sum_mv = 0;
            u16gv_cnv_offset_sample_count = 0;

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
        u32gv_cnv_phase_v_offset_sum_mv = 0;
        u32gv_cnv_phase_w_offset_sum_mv = 0;
        u16gv_cnv_offset_sample_count = 0;

        ent_status = EN_COM_STS_OK;
    }
    /*=========OUTPUT==========*/
    return ent_status;
}


/**
 * @fn     ens_cnv_curr
 * @id     CNV-005
 * @brief  Calculate phase currents (U, V, W) from ADC values and offset
 * @param  None
 * @return EN_COM_STS_OK
 */
EN_COM_STS_T ens_cnv_curr(void)
{
    const ST_INVERTER_CONFIG *cfg;
    int32_t phase_v_delta_mv;
    int32_t phase_w_delta_mv;

    cfg = Config_Get();

    phase_v_delta_mv = (s4)u16gv_cnv_phase_v_mv - (s4)u16gv_cnv_phase_v_offset_mv;
    phase_w_delta_mv = (s4)u16gv_cnv_phase_w_mv - (s4)u16gv_cnv_phase_w_offset_mv;

    ens_cnv_voltage_to_current_ca(phase_v_delta_mv, cfg->u4t_curr_ph_v_gain_mv_per_ca, &s32gv_cnv_phase_v_current_ca);
    ens_cnv_voltage_to_current_ca(phase_w_delta_mv, cfg->u4t_curr_ph_w_gain_mv_per_ca, &s32gv_cnv_phase_w_current_ca);
    s32gv_cnv_phase_u_current_ca = -(s32gv_cnv_phase_v_current_ca + s32gv_cnv_phase_w_current_ca);
    return EN_COM_STS_OK;
}

/**
 * @fn     eng_cnv_get_phase_currents
 * @id     CNV-006
 * @brief  Get all three phase currents
 * @param  phase_u_ca: Pointer to store phase U current in centi amperes
 * @param  phase_v_ca: Pointer to store phase V current in centi amperes
 * @param  phase_w_ca: Pointer to store phase W current in centi amperes
 * @return EN_COM_STS_OK
 */
EN_COM_STS_T eng_cnv_get_phase_currents(s4 *ps4t_phase_u_ca, s4 *ps4t_phase_v_ca, s4 *ps4t_phase_w_ca)
{
    *ps4t_phase_u_ca = s32gv_cnv_phase_u_current_ca;
    *ps4t_phase_v_ca = s32gv_cnv_phase_v_current_ca;
    *ps4t_phase_w_ca = s32gv_cnv_phase_w_current_ca;

    return EN_COM_STS_OK;
}

/************************************
 * GLOBAL FUNCTIONS
 ************************************/
/**
 * @fn     eng_cnv_init
 * @id     CNV-007
 * @brief  Initialize CNV module and calibrate current sensor offset
 * @param  None
 * @return EN_COM_STS_OK
 */
EN_COM_STS_T eng_cnv_init(void)
{
    /*==========INPUT==========*/
    /*========OPERATION========*/
    /*=========OUTPUT==========*/
    u16gv_cnv_phase_v_mv = 0u;
    u16gv_cnv_phase_w_mv = 0u;
    u16gv_cnv_phase_v_offset_mv = 0u;
    u16gv_cnv_phase_w_offset_mv = 0u;
    s32gv_cnv_phase_u_current_ca = 0;
    s32gv_cnv_phase_v_current_ca = 0;
    s32gv_cnv_phase_w_current_ca = 0;
    u32gv_cnv_phase_v_offset_sum_mv = 0u;
    u32gv_cnv_phase_w_offset_sum_mv = 0u;
    u16gv_cnv_offset_sample_count = 0u;
    u8gv_cnv_offset_ready = 0u;
    while(u8gv_cnv_offset_ready == 0u)
    {
        ens_cnv_update_adc_values();
        ens_cnv_curr_offset();
    }

    return EN_COM_STS_OK;
}

/**
 * @fn     eng_cnv_100us
 * @id     CNV-008
 * @brief  CNV 100us cyclic task: read ADC and compute phase currents
 * @param  None
 * @return EN_COM_STS_OK
 */
EN_COM_STS_T eng_cnv_100us(void)
{
    /*==========INPUT==========*/
    /*========OPERATION========*/
    ens_cnv_update_adc_values();
    ens_cnv_curr();
    /*=========OUTPUT==========*/


    return EN_COM_STS_OK;
}
