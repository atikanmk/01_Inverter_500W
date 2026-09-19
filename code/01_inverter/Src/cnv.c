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
#define CNV_THROTTLE_MAX_PERC 100u
#define CNV_THROTTLE_MAX_CPERC 10000u

/************************************
 * PRIVATE TYPEDEFS
 ************************************/

/************************************
 * STATIC VARIABLES
 ************************************/
static u2 u2gv_cnv_phase_u_offset_mv;
static u2 u2gv_cnv_phase_v_offset_mv;
static s4 s4gv_cnv_phase_u_current_ca;
static s4 s4gv_cnv_phase_v_current_ca;
static s4 s4gv_cnv_phase_w_current_ca;
static u4 u32gv_cnv_phase_u_offset_sum_mv;
static u4 u32gv_cnv_phase_v_offset_sum_mv;
static u2 u16gv_cnv_offset_sample_count;
static u1 u8gv_cnv_offset_ready;
static u2 u2gv_cnv_throttle_perc;
static u2 u2gv_cnv_bemf_u_mv;
static u2 u2gv_cnv_bemf_v_mv;
static u2 u2gv_cnv_bemf_w_mv;
static u2 u2gv_cnv_bemf_u_dv;
static u2 u2gv_cnv_bemf_v_dv;
static u2 u2gv_cnv_bemf_w_dv;
static u2 u2sv_cnv_adc_mv[EN_CONFIG_ADC_COUNT];

/************************************
 * GLOBAL VARIABLES
 ************************************/

/************************************
 * FUNCTION PROTOTYPES
 ************************************/
static EN_COM_STS_T cnv_adc_raw_to_mv(uint16_t raw_value, u2 * pu2t_mv);
static EN_COM_STS_T ens_cnv_update_adc_values(void);
static EN_COM_STS_T ens_cnv_voltage_to_current_ca(s4 s4t_delta_mv, u4 u4t_gain_mv_per_ca,s4* ps4t_out);
static EN_COM_STS_T ens_cnv_curr_offset(void);
static EN_COM_STS_T ens_cnv_curr(void);
static EN_COM_STS_T ens_cnv_bemf(void);
static EN_COM_STS_T ens_cnv_throttle_adc_to_perc(void);
static EN_COM_STS_T ens_cnv_bemf_mv_to_dv(u2 u2t_bemf_sensor_mv, u2 *pu2t_bemf_dv);

/************************************
 * FUNCTIONS
 ************************************/
 /**
 * @fn     eng_cnv_init
 * @id     CNV-007
 * @brief  Initialize CNV module and calibrate current sensor offset
 * @param  None
 * @return EN_COM_STS_OK
 */
extern EN_COM_STS_T eng_cnv_init(void)
{
    /*==========INPUT==========*/
    /*========OPERATION========*/
    /*=========OUTPUT==========*/
    u2gv_cnv_phase_u_offset_mv = 0u;
    u2gv_cnv_phase_v_offset_mv = 0u;
    s4gv_cnv_phase_u_current_ca = 0;
    s4gv_cnv_phase_v_current_ca = 0;
    s4gv_cnv_phase_w_current_ca = 0;
    u32gv_cnv_phase_u_offset_sum_mv = 0u;
    u32gv_cnv_phase_v_offset_sum_mv = 0u;
    u16gv_cnv_offset_sample_count = 0u;
    u8gv_cnv_offset_ready = 0u;
    u2gv_cnv_bemf_u_mv = 0u;
    u2gv_cnv_bemf_v_mv = 0u;
    u2gv_cnv_bemf_w_mv = 0u;
    u2gv_cnv_bemf_u_dv = 0u;
    u2gv_cnv_bemf_v_dv = 0u;
    u2gv_cnv_bemf_w_dv = 0u;

    return EN_COM_STS_OK;
}

/**
 * @fn     eng_cnv_100us
 * @id     CNV-008
 * @brief  CNV 100us cyclic task: read ADC and compute phase currents
 * @param  None
 * @return EN_COM_STS_OK
 */
extern EN_COM_STS_T eng_cnv_100us(void)
{
    /*==========INPUT==========*/
    /*========OPERATION========*/
	ens_cnv_update_adc_values();
    ens_cnv_curr_offset();
    ens_cnv_curr();
    ens_cnv_bemf();
    /*=========OUTPUT==========*/


    return EN_COM_STS_OK;
}

/**
 * @fn     eng_cnv_1ms
 * @id     CNV-009
 * @brief  CNV 1ms cyclic task: read ADC and compute phase currents
 * @param  None
 * @return EN_COM_STS_OK
 */
extern EN_COM_STS_T eng_cnv_1ms(void)
{
    /*==========INPUT==========*/
    /*========OPERATION========*/
    ens_cnv_throttle_adc_to_perc();

    /*=========OUTPUT==========*/
    return EN_COM_STS_OK;
}

/**
 * @fn     cnv_adc_raw_to_mv
 * @id     CNV-001
 * @brief  Convert 12-bit ADC raw value to millivolts
 * @param  raw_value: 12-bit ADC raw value
 * @return Voltage in millivolts
 */
static EN_COM_STS_T cnv_adc_raw_to_mv(uint16_t raw_value, u2 * pu2t_mv)
{
    *pu2t_mv = (uint16_t)(((uint32_t)raw_value * CNV_ADC_REFERENCE_MV) / ADC_RAW_MAX_12BIT);

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
static EN_COM_STS_T ens_cnv_voltage_to_current_ca(s4 s4t_delta_mv, u4 u4t_gain_mv_per_ca,s4* ps4t_out)
{
    EN_COM_STS_T ent_ret;
   /*==========INPUT==========*/
    /*========OPERATION========*/
    if (u4t_gain_mv_per_ca == 0u)
    {
        ent_ret = EN_COM_STS_ERR;
    }
    else
    {
    /*=========OUTPUT==========*/
        ps4t_out[0] = (s4)((s4t_delta_mv * (s4)u4t_gain_mv_per_ca) / 1000);
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
        u32gv_cnv_phase_u_offset_sum_mv += u2sv_cnv_adc_mv[EN_CONFIG_ADC_IU];
        u32gv_cnv_phase_v_offset_sum_mv += u2sv_cnv_adc_mv[EN_CONFIG_ADC_IV];
        u16gv_cnv_offset_sample_count++;

        if (u16gv_cnv_offset_sample_count >= CNV_OFFSET_SAMPLE_COUNT)
        {
            u2gv_cnv_phase_u_offset_mv = (u2)(u32gv_cnv_phase_u_offset_sum_mv / CNV_OFFSET_SAMPLE_COUNT);
            u2gv_cnv_phase_v_offset_mv = (u2)(u32gv_cnv_phase_v_offset_sum_mv / CNV_OFFSET_SAMPLE_COUNT);
            u8gv_cnv_offset_ready = 1u;
            u32gv_cnv_phase_u_offset_sum_mv = 0u;
            u32gv_cnv_phase_v_offset_sum_mv = 0u;
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
        u32gv_cnv_phase_u_offset_sum_mv = 0u;
        u32gv_cnv_phase_v_offset_sum_mv = 0u;
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
    s4 s4t_phase_u_delta_mv;
    s4 s4t_phase_v_delta_mv;

    cfg = config_get();

    s4t_phase_u_delta_mv = (s4)u2sv_cnv_adc_mv[EN_CONFIG_ADC_IU] - (s4)u2gv_cnv_phase_u_offset_mv;
    s4t_phase_v_delta_mv = (s4)u2sv_cnv_adc_mv[EN_CONFIG_ADC_IV] - (s4)u2gv_cnv_phase_v_offset_mv;

    ens_cnv_voltage_to_current_ca(s4t_phase_u_delta_mv, cfg->u4t_curr_ph_u_gain_mv_per_ca, &s4gv_cnv_phase_u_current_ca);
    ens_cnv_voltage_to_current_ca(s4t_phase_v_delta_mv, cfg->u4t_curr_ph_v_gain_mv_per_ca, &s4gv_cnv_phase_v_current_ca);
        s4gv_cnv_phase_u_current_ca *= (s4)cfg->s1t_curr_ph_u_direction;
        s4gv_cnv_phase_v_current_ca *= (s4)cfg->s1t_curr_ph_v_direction;
    s4gv_cnv_phase_w_current_ca = -(s4gv_cnv_phase_u_current_ca + s4gv_cnv_phase_v_current_ca);
    return EN_COM_STS_OK;
}

/**
 * @fn     ens_cnv_bemf
 * @id     CNV-015
 * @brief  Calculate U, V, and W phase BEMF voltages from ADC readings
 * @return BEMF calculation status
 */
static EN_COM_STS_T ens_cnv_bemf(void)
{

    if (ens_cnv_bemf_mv_to_dv(u2sv_cnv_adc_mv[EN_CONFIG_ADC_BEMF_U], &u2gv_cnv_bemf_u_dv) != EN_COM_STS_OK)
    {
        return EN_COM_STS_ERR;
    }

    if (ens_cnv_bemf_mv_to_dv(u2sv_cnv_adc_mv[EN_CONFIG_ADC_BEMF_V], &u2gv_cnv_bemf_v_dv) != EN_COM_STS_OK)
    {
        return EN_COM_STS_ERR;
    }

    return ens_cnv_bemf_mv_to_dv(u2sv_cnv_adc_mv[EN_CONFIG_ADC_BEMF_W], &u2gv_cnv_bemf_w_dv);
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
extern EN_COM_STS_T eng_cnv_get_phase_currents(s4 *ps4t_phase_u_ca, s4 *ps4t_phase_v_ca, s4 *ps4t_phase_w_ca)
{
    *ps4t_phase_u_ca = s4gv_cnv_phase_u_current_ca;
    *ps4t_phase_v_ca = s4gv_cnv_phase_v_current_ca;
    *ps4t_phase_w_ca = s4gv_cnv_phase_w_current_ca;

    return EN_COM_STS_OK;
}

/**
 * @fn     eng_cnv_get_throttle_perc
 * @id     CNV-010
 * @brief  Get the throttle position in percent
 * @param  pu2t_throttle_perc: Pointer to store throttle position from 0 to 100 percent (u2)
 * @return Throttle position read status
 */
extern EN_COM_STS_T eng_cnv_get_throttle_perc(u2 *pu2t_throttle_perc)
{
    *pu2t_throttle_perc = u2gv_cnv_throttle_perc;

    return EN_COM_STS_OK;
}

/**
 * @fn     eng_cnv_get_bemf_mv
 * @id     CNV-012
 * @brief  Get U, V, and W phase BEMF voltages sampled every 100 microseconds
 * @param  pu2t_bemf_u_mv: Pointer to store U-phase BEMF in millivolts (u2)
 * @param  pu2t_bemf_v_mv: Pointer to store V-phase BEMF in millivolts (u2)
 * @param  pu2t_bemf_w_mv: Pointer to store W-phase BEMF in millivolts (u2)
 * @return BEMF voltage read status
 */
extern EN_COM_STS_T eng_cnv_get_bemf_mv(u2 *pu2t_bemf_u_mv, u2 *pu2t_bemf_v_mv, u2 *pu2t_bemf_w_mv)
{
    if ((pu2t_bemf_u_mv == NULL) || (pu2t_bemf_v_mv == NULL) || (pu2t_bemf_w_mv == NULL))
    {
        return EN_COM_STS_ERR;
    }

    *pu2t_bemf_u_mv = u2gv_cnv_bemf_u_mv;
    *pu2t_bemf_v_mv = u2gv_cnv_bemf_v_mv;
    *pu2t_bemf_w_mv = u2gv_cnv_bemf_w_mv;

    return EN_COM_STS_OK;
}

/**
 * @fn     eng_cnv_get_bemf_dv
 * @id     CNV-013
 * @brief  Get U, V, and W phase BEMF voltages before the resistor divider
 * @param  pu2t_bemf_u_dv: Pointer to store U-phase BEMF in deci-volts (u2)
 * @param  pu2t_bemf_v_dv: Pointer to store V-phase BEMF in deci-volts (u2)
 * @param  pu2t_bemf_w_dv: Pointer to store W-phase BEMF in deci-volts (u2)
 * @return BEMF voltage read status
 */
extern EN_COM_STS_T eng_cnv_get_bemf_dv(u2 *pu2t_bemf_u_dv, u2 *pu2t_bemf_v_dv, u2 *pu2t_bemf_w_dv)
{
    if ((pu2t_bemf_u_dv == NULL) || (pu2t_bemf_v_dv == NULL) || (pu2t_bemf_w_dv == NULL))
    {
        return EN_COM_STS_ERR;
    }

    *pu2t_bemf_u_dv = u2gv_cnv_bemf_u_dv;
    *pu2t_bemf_v_dv = u2gv_cnv_bemf_v_dv;
    *pu2t_bemf_w_dv = u2gv_cnv_bemf_w_dv;

    return EN_COM_STS_OK;
}

/**
 * @fn     ens_cnv_bemf_mv_to_dv
 * @id     CNV-014
 * @brief  Convert BEMF voltage after the resistor divider to source deci-volts
 * @param  u2t_bemf_sensor_mv: BEMF sensor voltage at the ADC pin in millivolts (u2)
 * @param  pu2t_bemf_dv: Pointer to store BEMF source voltage in deci-volts (u2)
 * @return BEMF conversion status
 */
static EN_COM_STS_T ens_cnv_bemf_mv_to_dv(u2 u2t_bemf_sensor_mv, u2 *pu2t_bemf_dv)
{
    const ST_INVERTER_CONFIG *pst_config;
    u4 u4t_divider_total_ohm;

    if (pu2t_bemf_dv == NULL)
    {
        return EN_COM_STS_ERR;
    }

    pst_config = config_get();
    if (pst_config->u4t_bemf_divider_bottom_ohm == 0u)
    {
        return EN_COM_STS_ERR;
    }

    u4t_divider_total_ohm = pst_config->u4t_bemf_divider_top_ohm + pst_config->u4t_bemf_divider_bottom_ohm;
    *pu2t_bemf_dv = (u2)(((u4)u2t_bemf_sensor_mv * u4t_divider_total_ohm) /
                          (pst_config->u4t_bemf_divider_bottom_ohm * 100u));

    return EN_COM_STS_OK;
}

/**
 * @fn     ens_cnv_throttle_adc_to_perc
 * @id     CNV-011
 * @brief  Map throttle voltage between configured endpoints to 0 to 100 percent
 * @return Throttle conversion status
 */
static EN_COM_STS_T ens_cnv_throttle_adc_to_perc(void)
{
	static u2 u2s_throttle_centi_perc = 0;
    const ST_INVERTER_CONFIG *pst_config;
    u2 u2t_throttle_mv;
    u2 u2t_throttle_cperc;
    /*==========INPUT==========*/
    pst_config = config_get();
    u2t_throttle_mv = u2sv_cnv_adc_mv[EN_CONFIG_ADC_TPS];

    if (pst_config->u2t_throttle_max_mv <= pst_config->u2t_throttle_min_mv)
    {
        return EN_COM_STS_ERR;
    }

    /*========OPERATION========*/
    if (u2t_throttle_mv <= pst_config->u2t_throttle_min_mv)
    {
    	u2t_throttle_cperc = 0u;
    }
    else if (u2t_throttle_mv >= pst_config->u2t_throttle_max_mv)
    {
    	u2t_throttle_cperc = CNV_THROTTLE_MAX_CPERC;
    }
    else
    {
    	u2t_throttle_cperc = (u2)(((u4)(u2t_throttle_mv - pst_config->u2t_throttle_min_mv) * CNV_THROTTLE_MAX_CPERC) /
                                      (u4)(pst_config->u2t_throttle_max_mv - pst_config->u2t_throttle_min_mv));

    	u2s_throttle_centi_perc = (u2s_throttle_centi_perc * 0.95) + (u2t_throttle_cperc * 0.05);
    }
    /*=========OUTPUT==========*/
	u2gv_cnv_throttle_perc = u2s_throttle_centi_perc/CNV_THROTTLE_MAX_PERC;
    return EN_COM_STS_OK;
}

/**
 * @fn     ens_cnv_update_adc_values
 * @id     CNV-009
 * @brief  Update ADC values and compute phase currents
 * @param  None
 * @return EN_COM_STS_OK
 */
static EN_COM_STS_T ens_cnv_update_adc_values(void)
{
    u1 u1t_adc_cnt;
    u2 u2t_adc_raw;
    /*==========INPUT==========*/
    /*========OPERATION========*/
    for (u1t_adc_cnt = 0u; u1t_adc_cnt < EN_CONFIG_ADC_COUNT; u1t_adc_cnt++)
    {
        ADC_GetValue(u1t_adc_cnt,&u2t_adc_raw);
        cnv_adc_raw_to_mv(u2t_adc_raw, &u2sv_cnv_adc_mv[u1t_adc_cnt]);
    }

    /*=========OUTPUT==========*/

    return EN_COM_STS_OK;
}


