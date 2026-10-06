/**
 ********************************************************************************
 * @file    foc.c
 * @author  Atikan
 * @date    2026-07-19
 * @brief   FOC control (integer Q15 path for 100 us ISR).
 ********************************************************************************
 */

/************************************
 * INCLUDES
 ************************************/
#include "foc.h"
#include "hall.h"
#include "cnv.h"
#include "pwm.h"
#include "config.h"

/************************************
 * EXTERN VARIABLES
 ************************************/

/************************************
 * PRIVATE MACROS AND DEFINES
 ************************************/
#define FOC_PI_SCALE                 1024
#define FOC_CURRENT_PI_KP_Q10        50
#define FOC_CURRENT_PI_KI_Q10        2
#define FOC_VOLTAGE_LIMIT_CPCT       10000
#define FOC_SVM_DUTY_CENTER_CPCT     5000
#define FOC_VS_POS_MAX_PERC          (5000)
#define FOC_VS_NEG_MAX_PERC          (-5000)
#define FOC_VQ_POS_MAX_PERC          (5000)
#define FOC_VQ_NEG_MAX_PERC          (-5000)
#define FOC_VD_POS_MAX_PERC          (100)
#define FOC_VD_NEG_MAX_PERC          (-5000)
#define FOC_CLARKE_K1_Q15            18919
#define FOC_CLARKE_K2_Q15            37837
#define FOC_SVM_HALF_Q15             16384
#define FOC_SVM_SQRT3_2_Q15          28378

/************************************
 * PRIVATE TYPEDEFS
 ************************************/

/************************************
 * STATIC VARIABLES
 ************************************/
static s4 s4g_foc_iq_pi_integral_q10;
static s4 s4g_foc_id_pi_integral_q10;
static s4 s4s_d_ft_static;
static s4 s4s_q_ft_static;

/************************************
 * GLOBAL VARIABLES
 ************************************/
s4 s4g_foc_angle_est_deg;
s4 s4g_foc_ialpha_ca;
s4 s4g_foc_ibeta_ca;
s4 s4g_foc_iq_ref_ca;
s4 s4g_foc_id_ref_ca;
s4 s4g_foc_is_feb_ca;
s4 s4g_foc_iq_feb_ca;
s4 s4g_foc_id_feb_ca;
s4 s4g_foc_iq_raw_ca;
s4 s4g_foc_id_raw_ca;
s4 s4g_foc_vq_ref_cpct;
s4 s4g_foc_vd_ref_cpct;
s4 s4g_foc_valpha_cpct;
s4 s4g_foc_vbeta_cpct;
s4 s4g_foc_svm_u_cpct;
s4 s4g_foc_svm_v_cpct;
s4 s4g_foc_svm_w_cpct;
s4 s4g_foc_trq_cmd;

/************************************
 * FUNCTION PROTOTYPES
 ************************************/
static EN_COM_STS_T ens_foc_park_transform(s4 s4t_alpha, s4 s4t_beta, u2 u16t_angle_deg, s4 *s4t_d, s4 *s4t_q, s4 *s4t_s);
static EN_COM_STS_T ens_foc_clarke_transform(s4 s4t_u, s4 s4t_v, s4 *s4t_alpha, s4 *s4t_beta);
static EN_COM_STS_T ens_foc_inverse_park_transform(s4 s4t_d, s4 s4t_q, u2 u16t_angle_deg, s4 *s4t_alpha, s4 *s4t_beta);
static EN_COM_STS_T ens_foc_svm(s4 s4t_valpha, s4 s4t_vbeta, s4 *s4t_svm_u, s4 *s4t_svm_v, s4 *s4t_svm_w);
static EN_COM_STS_T ens_foc_trq_ref(u2 u2t_tps_perc, s4 s4t_rpm, s4 *ps4t_trq_centi_nm);
static EN_COM_STS_T ens_foc_curr_ref(s4 s4t_trq_cmd_ca, s4 s4t_rpm, s4 s4t_iq_feb_ca, s4 *ps4t_iq_ref_ca, s4 *ps4t_id_ref_ca);
static EN_COM_STS_T ens_foc_curr_pi_cal(s4 s4t_iq_ref_ca, s4 s4t_id_ref_ca, s4 s4t_iq_feb_ca, s4 s4t_id_feb_ca, s4 *s4t_vq_cpct, s4 *s4t_vd_cpct);
static s2 ens_foc_sin_q15(u2 u2t_angle_deg);
static s2 ens_foc_cos_q15(u2 u2t_angle_deg);

/************************************
 * FUNCTIONS
 ************************************/
static const s2 s2s_foc_sin_q15[360] = {
    0, 572, 1144, 1715, 2286, 2856, 3425, 3993, 4560, 5126,
    5690, 6252, 6813, 7371, 7927, 8481, 9032, 9580, 10126, 10668,
    11207, 11743, 12275, 12803, 13328, 13848, 14364, 14876, 15383, 15886,
    16383, 16876, 17364, 17846, 18323, 18794, 19260, 19720, 20173, 20621,
    21062, 21497, 21925, 22347, 22762, 23170, 23571, 23964, 24351, 24730,
    25101, 25465, 25821, 26169, 26509, 26841, 27165, 27481, 27788, 28087,
    28377, 28659, 28932, 29196, 29451, 29697, 29934, 30162, 30381, 30591,
    30791, 30982, 31163, 31335, 31498, 31650, 31794, 31927, 32051, 32165,
    32269, 32364, 32448, 32523, 32587, 32642, 32687, 32722, 32747, 32762,
    32767, 32762, 32747, 32722, 32687, 32642, 32587, 32523, 32448, 32364,
    32269, 32165, 32051, 31927, 31794, 31650, 31498, 31335, 31163, 30982,
    30791, 30591, 30381, 30162, 29934, 29697, 29451, 29196, 28932, 28659,
    28377, 28087, 27788, 27481, 27165, 26841, 26509, 26169, 25821, 25465,
    25101, 24730, 24351, 23964, 23571, 23170, 22762, 22347, 21925, 21497,
    21062, 20621, 20173, 19720, 19260, 18794, 18323, 17846, 17364, 16876,
    16383, 15886, 15383, 14876, 14364, 13848, 13328, 12803, 12275, 11743,
    11207, 10668, 10126, 9580, 9032, 8481, 7927, 7371, 6813, 6252,
    5690, 5126, 4560, 3993, 3425, 2856, 2286, 1715, 1144, 572,
    0, -572, -1144, -1715, -2286, -2856, -3425, -3993, -4560, -5126,
    -5690, -6252, -6813, -7371, -7927, -8481, -9032, -9580, -10126, -10668,
    -11207, -11743, -12275, -12803, -13328, -13848, -14364, -14876, -15383, -15886,
    -16384, -16876, -17364, -17846, -18323, -18794, -19260, -19720, -20173, -20621,
    -21062, -21497, -21925, -22347, -22762, -23170, -23571, -23964, -24351, -24730,
    -25101, -25465, -25821, -26169, -26509, -26841, -27165, -27481, -27788, -28087,
    -28377, -28659, -28932, -29196, -29451, -29697, -29934, -30162, -30381, -30591,
    -30791, -30982, -31163, -31335, -31498, -31650, -31794, -31927, -32051, -32165,
    -32269, -32364, -32448, -32523, -32587, -32642, -32687, -32722, -32747, -32762,
    -32767, -32762, -32747, -32722, -32687, -32642, -32587, -32523, -32448, -32364,
    -32269, -32165, -32051, -31927, -31794, -31650, -31498, -31335, -31163, -30982,
    -30791, -30591, -30381, -30162, -29934, -29697, -29451, -29196, -28932, -28659,
    -28377, -28087, -27788, -27481, -27165, -26841, -26509, -26169, -25821, -25465,
    -25101, -24730, -24351, -23964, -23571, -23170, -22762, -22347, -21925, -21497,
    -21062, -20621, -20173, -19720, -19260, -18794, -18323, -17846, -17364, -16876,
    -16384, -15886, -15383, -14876, -14364, -13848, -13328, -12803, -12275, -11743,
    -11207, -10668, -10126, -9580, -9032, -8481, -7927, -7371, -6813, -6252,
    -5690, -5126, -4560, -3993, -3425, -2856, -2286, -1715, -1144, -572
};

/**
 * @fn     ens_foc_sin_q15
 * @id     FOC-001
 * @brief  Look up Q15 sine of an electrical angle in degrees
 * @param  u2t_angle_deg: Electrical angle in degrees (u2)
 * @return Q15 sine value
 */
static s2 ens_foc_sin_q15(u2 u2t_angle_deg)
{
    return s2s_foc_sin_q15[(u2t_angle_deg % 360u)];
}

/**
 * @fn     ens_foc_cos_q15
 * @id     FOC-002
 * @brief  Look up Q15 cosine of an electrical angle in degrees
 * @param  u2t_angle_deg: Electrical angle in degrees (u2)
 * @return Q15 cosine value
 */
static s2 ens_foc_cos_q15(u2 u2t_angle_deg)
{
    return s2s_foc_sin_q15[((u2t_angle_deg + 90u) % 360u)];
}

/**
 * @fn     eng_foc_init
 * @id     FOC-003
 * @brief  Initialize FOC states and integrators
 * @param  None
 * @return EN_COM_STS_OK
 */
EN_COM_STS_T eng_foc_init(void)
{
    /*==========INPUT==========*/
    /*========OPERATION========*/
    s4g_foc_angle_est_deg = 0;
    s4g_foc_ialpha_ca = 0;
    s4g_foc_ibeta_ca = 0;
    s4g_foc_iq_ref_ca = 0;
    s4g_foc_id_ref_ca = 0;
    s4g_foc_iq_feb_ca = 0;
    s4g_foc_id_feb_ca = 0;
    s4g_foc_iq_raw_ca = 0;
    s4g_foc_id_raw_ca = 0;
    s4g_foc_vq_ref_cpct = 0;
    s4g_foc_vd_ref_cpct = 0;
    s4g_foc_valpha_cpct = 0;
    s4g_foc_vbeta_cpct = 0;
    s4g_foc_svm_u_cpct = 0;
    s4g_foc_svm_v_cpct = 0;
    s4g_foc_svm_w_cpct = 0;
    s4g_foc_iq_pi_integral_q10 = 0;
    s4g_foc_id_pi_integral_q10 = 0;
    s4s_d_ft_static = 0;
    s4s_q_ft_static = 0;
    /*=========OUTPUT==========*/
    return EN_COM_STS_OK;
}

/**
 * @fn     eng_foc_main
 * @id     FOC-004
 * @brief  FOC cyclic task: Clarke/Park, open-loop voltage, inverse Park, SVM
 * @param  None
 * @return EN_COM_STS_OK
 */
EN_COM_STS_T eng_foc_main(void)
{
    s4 s4t_iq_ref_ca = 0;
    s4 s4t_id_ref_ca = 0;
    s4 s4t_id_feb_ca = 0;
    s4 s4t_iq_feb_ca = 0;
    s4 s4t_is_feb_ca = 0;
    u2 u2t_angle_est = 0u;
    s4 s4t_u_ca = 0;
    s4 s4t_v_ca = 0;
    s4 s4t_w_ca = 0;
    s4 s4t_alpha_ca = 0;
    s4 s4t_beta_ca = 0;
    s4 s4t_vd_cpct = 0;
    s4 s4t_vq_cpct = 0;
    s4 s4t_valpha_cpct = 0;
    s4 s4t_vbeta_cpct = 0;
    s4 s4t_svm_u_cpct = 0;
    s4 s4t_svm_v_cpct = 0;
    s4 s4t_svm_w_cpct = 0;
    s4 s4t_trq = 0;
    s4 s4t_spd_rpm = 0;
    u2 u2t_tps_centi_perc = 0;

    /*==========INPUT==========*/
    /* Runs from eng_task_main(), not TIM2. Clarke/Park/torque are safe here.
     * Voltage command stays open-loop (PI not enabled). */
    (void)eng_cnv_get_phase_currents(&s4t_u_ca, &s4t_v_ca, &s4t_w_ca);
    (void)eng_hall_ang_est(&u2t_angle_est);
    (void)eng_cnv_get_throttle_centi_perc(&u2t_tps_centi_perc);
    (void)eng_cnv_get_mtr_spd_rpm(&s4t_spd_rpm);

    /*========OPERATION========*/
    (void)ens_foc_clarke_transform(s4t_u_ca, s4t_v_ca, &s4t_alpha_ca, &s4t_beta_ca);
    (void)ens_foc_park_transform(s4t_alpha_ca, s4t_beta_ca, u2t_angle_est,
                                 &s4t_id_feb_ca, &s4t_iq_feb_ca, &s4t_is_feb_ca);

    (void)ens_foc_trq_ref(u2t_tps_centi_perc, s4t_spd_rpm, &s4t_trq);
    (void)ens_foc_curr_ref(s4t_trq, s4t_spd_rpm, s4t_iq_feb_ca, &s4t_iq_ref_ca, &s4t_id_ref_ca);

    /* Open-loop: Vq from throttle centi-%, Vd = 0 (tuning). PI stays off. */
    s4t_vq_cpct = (s4)u2t_tps_centi_perc;
    if (s4t_vq_cpct > FOC_VQ_POS_MAX_PERC)
    {
        s4t_vq_cpct = FOC_VQ_POS_MAX_PERC;
    }
    s4t_vd_cpct = 0;

    (void)ens_foc_inverse_park_transform(s4t_vd_cpct, s4t_vq_cpct, u2t_angle_est,
                                         &s4t_valpha_cpct, &s4t_vbeta_cpct);
    (void)ens_foc_svm(s4t_valpha_cpct, s4t_vbeta_cpct,
                      &s4t_svm_u_cpct, &s4t_svm_v_cpct, &s4t_svm_w_cpct);
    eng_pwm_set_duty(s4t_svm_u_cpct, s4t_svm_v_cpct, s4t_svm_w_cpct);

    /*=========OUTPUT==========*/
    s4g_foc_angle_est_deg = (s4)u2t_angle_est;
    s4g_foc_ialpha_ca = s4t_alpha_ca;
    s4g_foc_ibeta_ca = s4t_beta_ca;
    s4g_foc_iq_ref_ca = s4t_iq_ref_ca;
    s4g_foc_id_ref_ca = s4t_id_ref_ca;
    s4g_foc_iq_feb_ca = s4t_iq_feb_ca;
    s4g_foc_id_feb_ca = s4t_id_feb_ca;
    s4g_foc_is_feb_ca = s4t_is_feb_ca;
    s4g_foc_vq_ref_cpct = s4t_vq_cpct;
    s4g_foc_vd_ref_cpct = s4t_vd_cpct;
    s4g_foc_valpha_cpct = s4t_valpha_cpct;
    s4g_foc_vbeta_cpct = s4t_vbeta_cpct;
    s4g_foc_svm_u_cpct = s4t_svm_u_cpct;
    s4g_foc_svm_v_cpct = s4t_svm_v_cpct;
    s4g_foc_svm_w_cpct = s4t_svm_w_cpct;
    s4g_foc_trq_cmd = s4t_trq;

    return EN_COM_STS_OK;
}

/**
 * @fn     ens_foc_curr_ref
 * @id     FOC-005
 * @brief  Convert torque command to Iq/Id current references
 * @param  s4t_trq_cmd_ca: Torque command in centi-Nm (s4)
 * @param  s4t_rpm: Motor speed in rpm (s4)
 * @param  s4t_iq_feb_ca: Iq feedback in centi-amperes (s4)
 * @param  ps4t_iq_ref_ca: Pointer to Iq reference output (s4)
 * @param  ps4t_id_ref_ca: Pointer to Id reference output (s4)
 * @return EN_COM_STS_OK
 */
static EN_COM_STS_T ens_foc_curr_ref(s4 s4t_trq_cmd_ca, s4 s4t_rpm, s4 s4t_iq_feb_ca, s4 *ps4t_iq_ref_ca, s4 *ps4t_id_ref_ca)
{
    const ST_INVERTER_CONFIG *pst_config;
    s4 s4t_iq;
    s4 s4t_id;

    /*==========INPUT==========*/
    (void)s4t_rpm;
    (void)s4t_iq_feb_ca;
    pst_config = config_get();

    /*========OPERATION========*/
    s4t_iq = (s4t_trq_cmd_ca * pst_config->s4t_centi_amp_per_centi_nm_fac100pc) / 100;
    s4t_id = 0;

    /*=========OUTPUT==========*/
    *ps4t_iq_ref_ca = s4t_iq;
    *ps4t_id_ref_ca = s4t_id;

    return EN_COM_STS_OK;
}

/**
 * @fn     ens_foc_curr_pi_cal
 * @id     FOC-006
 * @brief  Current PI regulators for Vq and Vd
 * @param  s4t_iq_ref_ca: Iq reference in centi-amperes (s4)
 * @param  s4t_id_ref_ca: Id reference in centi-amperes (s4)
 * @param  s4t_iq_feb_ca: Iq feedback in centi-amperes (s4)
 * @param  s4t_id_feb_ca: Id feedback in centi-amperes (s4)
 * @param  s4t_vq_cpct: Pointer to Vq command in centi-percent (s4)
 * @param  s4t_vd_cpct: Pointer to Vd command in centi-percent (s4)
 * @return EN_COM_STS_OK
 */
static EN_COM_STS_T ens_foc_curr_pi_cal(s4 s4t_iq_ref_ca, s4 s4t_id_ref_ca, s4 s4t_iq_feb_ca, s4 s4t_id_feb_ca, s4 *s4t_vq_cpct, s4 *s4t_vd_cpct)
{
    s4 s4t_iq_error_ca;
    s4 s4t_id_error_ca;
    s4 s4t_iq_integral_next_q10;
    s4 s4t_id_integral_next_q10;
    s4 s4t_vq_unlimited_cpct;
    s4 s4t_vd_unlimited_cpct;

    /*==========INPUT==========*/
    if (u1g_pwm_get_pwm_enb_sts() == 0u)
    {
        s4g_foc_iq_pi_integral_q10 = 0;
        s4g_foc_id_pi_integral_q10 = 0;
        *s4t_vq_cpct = 0;
        *s4t_vd_cpct = 0;
        return EN_COM_STS_OK;
    }

    /*========OPERATION========*/
    s4t_iq_error_ca = s4t_iq_ref_ca - s4t_iq_feb_ca;
    s4t_id_error_ca = s4t_id_ref_ca - s4t_id_feb_ca;
    s4t_iq_integral_next_q10 = s4g_foc_iq_pi_integral_q10 + (s4t_iq_error_ca * FOC_CURRENT_PI_KI_Q10);
    s4t_id_integral_next_q10 = s4g_foc_id_pi_integral_q10 + (s4t_id_error_ca * FOC_CURRENT_PI_KI_Q10);
    s4t_vq_unlimited_cpct = ((s4t_iq_error_ca * FOC_CURRENT_PI_KP_Q10) + s4t_iq_integral_next_q10) / FOC_PI_SCALE;
    s4t_vd_unlimited_cpct = ((s4t_id_error_ca * FOC_CURRENT_PI_KP_Q10) + s4t_id_integral_next_q10) / FOC_PI_SCALE;

    if (s4t_vq_unlimited_cpct > FOC_VQ_POS_MAX_PERC)
    {
        *s4t_vq_cpct = FOC_VQ_POS_MAX_PERC;
    }
    else if (s4t_vq_unlimited_cpct < FOC_VQ_NEG_MAX_PERC)
    {
        *s4t_vq_cpct = FOC_VQ_NEG_MAX_PERC;
    }
    else
    {
        s4g_foc_iq_pi_integral_q10 = s4t_iq_integral_next_q10;
        *s4t_vq_cpct = s4t_vq_unlimited_cpct;
    }

    if (s4t_vd_unlimited_cpct > FOC_VD_POS_MAX_PERC)
    {
        *s4t_vd_cpct = FOC_VD_POS_MAX_PERC;
    }
    else if (s4t_vd_unlimited_cpct < FOC_VD_NEG_MAX_PERC)
    {
        *s4t_vd_cpct = FOC_VD_NEG_MAX_PERC;
    }
    else
    {
        s4g_foc_id_pi_integral_q10 = s4t_id_integral_next_q10;
        *s4t_vd_cpct = s4t_vd_unlimited_cpct;
    }

    /*=========OUTPUT==========*/
    return EN_COM_STS_OK;
}

/**
 * @fn     ens_foc_park_transform
 * @id     FOC-007
 * @brief  Park transform from alpha/beta currents to d/q with LPF
 * @param  s4t_alpha: Alpha-axis current (s4)
 * @param  s4t_beta: Beta-axis current (s4)
 * @param  u16t_angle_deg: Electrical angle in degrees (u2)
 * @param  s4t_d: Pointer to Id output (s4)
 * @param  s4t_q: Pointer to Iq output (s4)
 * @param  s4t_s: Pointer to signed magnitude estimate (s4)
 * @return EN_COM_STS_OK
 */
static EN_COM_STS_T ens_foc_park_transform(s4 s4t_alpha, s4 s4t_beta, u2 u16t_angle_deg, s4 *s4t_d, s4 *s4t_q, s4 *s4t_s)
{
    s2 s2t_sin_q15;
    s2 s2t_cos_q15;
    s4 s4t_d_ft;
    s4 s4t_q_ft;
    s4 s4t_s_ft;
    s4 s4t_abs_d;
    s4 s4t_abs_q;

    /*==========INPUT==========*/
    s2t_sin_q15 = ens_foc_sin_q15(u16t_angle_deg);
    s2t_cos_q15 = ens_foc_cos_q15(u16t_angle_deg);

    /*========OPERATION========*/
    s4t_d_ft = (s4)(((s4t_alpha * (s4)s2t_cos_q15) + (s4t_beta * (s4)s2t_sin_q15)) >> 15);
    s4t_q_ft = (s4)((((-s4t_alpha) * (s4)s2t_sin_q15) + (s4t_beta * (s4)s2t_cos_q15)) >> 15);
    s4g_foc_id_raw_ca = s4t_d_ft;
    s4g_foc_iq_raw_ca = s4t_q_ft;

    /* LPF ~0.95/0.05: y = (19*y + x)/20 */
    s4t_q_ft = ((s4s_q_ft_static * 19) + s4t_q_ft) / 20;
    s4t_d_ft = ((s4s_d_ft_static * 19) + s4t_d_ft) / 20;

    s4t_abs_d = (s4t_d_ft >= 0) ? s4t_d_ft : -s4t_d_ft;
    s4t_abs_q = (s4t_q_ft >= 0) ? s4t_q_ft : -s4t_q_ft;
    if (s4t_abs_d >= s4t_abs_q)
    {
        s4t_s_ft = s4t_abs_d + (s4t_abs_q / 2);
    }
    else
    {
        s4t_s_ft = s4t_abs_q + (s4t_abs_d / 2);
    }
    if (s4t_q_ft < 0)
    {
        s4t_s_ft = -s4t_s_ft;
    }

    /*=========OUTPUT==========*/
    *s4t_d = s4t_d_ft;
    *s4t_q = s4t_q_ft;
    *s4t_s = s4t_s_ft;
    s4s_d_ft_static = s4t_d_ft;
    s4s_q_ft_static = s4t_q_ft;

    return EN_COM_STS_OK;
}

/**
 * @fn     ens_foc_clarke_transform
 * @id     FOC-008
 * @brief  Clarke transform from U/V phase currents to alpha/beta
 * @param  s4t_u: U-phase current (s4)
 * @param  s4t_v: V-phase current (s4)
 * @param  s4t_alpha: Pointer to alpha-axis current (s4)
 * @param  s4t_beta: Pointer to beta-axis current (s4)
 * @return EN_COM_STS_OK
 */
static EN_COM_STS_T ens_foc_clarke_transform(s4 s4t_u, s4 s4t_v, s4 *s4t_alpha, s4 *s4t_beta)
{
    /*==========INPUT==========*/
    /*========OPERATION========*/
    *s4t_alpha = s4t_u;
    *s4t_beta = (s4)(((s4t_u * (s4)FOC_CLARKE_K1_Q15) + (s4t_v * (s4)FOC_CLARKE_K2_Q15)) >> 15);
    /*=========OUTPUT==========*/
    return EN_COM_STS_OK;
}

/**
 * @fn     ens_foc_inverse_park_transform
 * @id     FOC-009
 * @brief  Inverse Park transform from d/q voltages to alpha/beta
 * @param  s4t_d: D-axis voltage (s4)
 * @param  s4t_q: Q-axis voltage (s4)
 * @param  u16t_angle_deg: Electrical angle in degrees (u2)
 * @param  s4t_alpha: Pointer to alpha-axis voltage (s4)
 * @param  s4t_beta: Pointer to beta-axis voltage (s4)
 * @return EN_COM_STS_OK
 */
static EN_COM_STS_T ens_foc_inverse_park_transform(s4 s4t_d, s4 s4t_q, u2 u16t_angle_deg, s4 *s4t_alpha, s4 *s4t_beta)
{
    s2 s2t_sin_q15;
    s2 s2t_cos_q15;

    /*==========INPUT==========*/
    s2t_sin_q15 = ens_foc_sin_q15(u16t_angle_deg);
    s2t_cos_q15 = ens_foc_cos_q15(u16t_angle_deg);

    /*========OPERATION========*/
    *s4t_alpha = (s4)(((s4t_d * (s4)s2t_cos_q15) - (s4t_q * (s4)s2t_sin_q15)) >> 15);
    *s4t_beta = (s4)(((s4t_d * (s4)s2t_sin_q15) + (s4t_q * (s4)s2t_cos_q15)) >> 15);

    /*=========OUTPUT==========*/
    return EN_COM_STS_OK;
}

/**
 * @fn     ens_foc_svm
 * @id     FOC-010
 * @brief  Space-vector modulation from alpha/beta voltages to UVW duties
 * @param  s4t_valpha: Alpha-axis voltage in centi-percent (s4)
 * @param  s4t_vbeta: Beta-axis voltage in centi-percent (s4)
 * @param  s4t_svm_u: Pointer to U-phase duty (s4)
 * @param  s4t_svm_v: Pointer to V-phase duty (s4)
 * @param  s4t_svm_w: Pointer to W-phase duty (s4)
 * @return EN_COM_STS_OK
 */
static EN_COM_STS_T ens_foc_svm(s4 s4t_valpha, s4 s4t_vbeta, s4 *s4t_svm_u, s4 *s4t_svm_v, s4 *s4t_svm_w)
{
    s4 s4t_phase_u;
    s4 s4t_phase_v;
    s4 s4t_phase_w;
    s4 s4t_phase_min;
    s4 s4t_phase_max;
    s4 s4t_common_mode;

    /*==========INPUT==========*/
    /*========OPERATION========*/
    s4t_phase_u = s4t_valpha;
    s4t_phase_v = (s4)((((-(s4)FOC_SVM_HALF_Q15) * s4t_valpha) + ((s4)FOC_SVM_SQRT3_2_Q15 * s4t_vbeta)) >> 15);
    s4t_phase_w = (s4)((((-(s4)FOC_SVM_HALF_Q15) * s4t_valpha) - ((s4)FOC_SVM_SQRT3_2_Q15 * s4t_vbeta)) >> 15);

    s4t_phase_min = s4t_phase_u;
    s4t_phase_max = s4t_phase_u;
    if (s4t_phase_v < s4t_phase_min)
    {
        s4t_phase_min = s4t_phase_v;
    }
    if (s4t_phase_w < s4t_phase_min)
    {
        s4t_phase_min = s4t_phase_w;
    }
    if (s4t_phase_v > s4t_phase_max)
    {
        s4t_phase_max = s4t_phase_v;
    }
    if (s4t_phase_w > s4t_phase_max)
    {
        s4t_phase_max = s4t_phase_w;
    }

    s4t_common_mode = FOC_SVM_DUTY_CENTER_CPCT - ((s4t_phase_max + s4t_phase_min) / 2);

    /*=========OUTPUT==========*/
    *s4t_svm_u = s4t_phase_u + s4t_common_mode;
    *s4t_svm_v = s4t_phase_v + s4t_common_mode;
    *s4t_svm_w = s4t_phase_w + s4t_common_mode;

    return EN_COM_STS_OK;
}

static const s2 s2t_foc_perc_to_centi_nm[100][10] = {

    -10, -11, -12, -13, -14, -15, -16, -17, -18, -19,
    -8, -9, -10, -11, -12, -13, -14, -15, -16, -17,
    -6, -7, -8, -9, -10, -11, -12, -13, -14, -15,
    -4, -5, -6, -7, -8, -9, -10, -11, -12, -13,
    0, -1, -2, -3, -4, -5, -6, -7, -8, -9,
    0, -1, -2, -3, -4, -5, -6, -7, -8, -9,
    1, 0, -1, -2, -3, -4, -5, -6, -7, -8,
    1, 0, -1, -2, -3, -4, -5, -6, -7, -8,
    1, 0, -1, -2, -3, -4, -5, -6, -7, -8,
    2, 1, 0, -1, -2, -3, -4, -5, -6, -7,
    2, 1, 0, -1, -2, -3, -4, -5, -6, -7,
    2, 1, 0, -1, -2, -3, -4, -5, -6, -7,
    3, 2, 1, 0, -1, -2, -3, -4, -5, -6,
    3, 2, 1, 0, -1, -2, -3, -4, -5, -6,
    3, 2, 1, 0, -1, -2, -3, -4, -5, -6,
    4, 3, 2, 1, 0, -1, -2, -3, -4, -5,
    4, 3, 2, 1, 0, -1, -2, -3, -4, -5,
    4, 3, 2, 1, 0, -1, -2, -3, -4, -5,
    4, 3, 2, 1, 0, -1, -2, -3, -4, -5,
    5, 4, 3, 2, 1, 0, -1, -2, -3, -4,
    5, 4, 3, 2, 1, 0, -1, -2, -3, -4,
    5, 4, 3, 2, 1, 0, -1, -2, -3, -4,
    5, 4, 3, 2, 1, 0, -1, -2, -3, -4,
    6, 5, 4, 3, 2, 1, 0, -1, -2, -3,
    6, 5, 4, 3, 2, 1, 0, -1, -2, -3,
    6, 5, 4, 3, 2, 1, 0, -1, -2, -3,
    6, 5, 4, 3, 2, 1, 0, -1, -2, -3,
    7, 6, 5, 4, 3, 2, 1, 0, -1, -2,
    7, 6, 5, 4, 3, 2, 1, 0, -1, -2,
    7, 6, 5, 4, 3, 2, 1, 0, -1, -2,
    7, 6, 5, 4, 3, 2, 1, 0, -1, -2,
    8, 7, 6, 5, 4, 3, 2, 1, 0, -1,
    8, 7, 6, 5, 4, 3, 2, 1, 0, -1,
    8, 7, 6, 5, 4, 3, 2, 1, 0, -1,
    8, 7, 6, 5, 4, 3, 2, 1, 0, -1,
    8, 7, 6, 5, 4, 3, 2, 1, 0, -1,
    9, 8, 7, 6, 5, 4, 3, 2, 1, 0,
    9, 8, 7, 6, 5, 4, 3, 2, 1, 0,
    9, 8, 7, 6, 5, 4, 3, 2, 1, 0,
    9, 8, 7, 6, 5, 4, 3, 2, 1, 0,
    9, 8, 7, 6, 5, 4, 3, 2, 1, 0,
    10, 9, 8, 7, 6, 5, 4, 3, 2, 1,
    10, 10, 10, 9, 8, 7, 6, 5, 4, 3,
    10, 10, 10, 10, 9, 8, 7, 6, 5, 4,
    10, 10, 10, 10, 10, 9, 8, 7, 6, 5,
    10, 10, 10, 10, 10, 10, 9, 8, 7, 6,
    10, 10, 10, 10, 10, 10, 10, 9, 8, 7,
    10, 10, 10, 10, 10, 10, 10, 10, 9, 8,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 9,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
};

/**
 * @fn     ens_foc_trq_ref
 * @id     FOC-011
 * @brief  Look up torque command from throttle and speed
 * @param  u2t_tps_perc: Throttle in centi-percent (u2)
 * @param  s4t_rpm: Motor speed in rpm (s4)
 * @param  ps4t_trq_centi_nm: Pointer to torque command in centi-Nm (s4)
 * @return EN_COM_STS_OK or EN_COM_STS_ERR
 */
static EN_COM_STS_T ens_foc_trq_ref(u2 u2t_tps_perc, s4 s4t_rpm, s4 *ps4t_trq_centi_nm)
{
    u2 u2t_table_index;
    s4 s4t_cmd;
    s4 s4t_rpm_index;
    s4 s4t_rpm_abs;

    /*==========INPUT==========*/
    if (ps4t_trq_centi_nm == 0)
    {
        return EN_COM_STS_ERR;
    }

    /*========OPERATION========*/
    u2t_table_index = u2t_tps_perc / 100u;
    if (u2t_table_index >= 100u)
    {
        u2t_table_index = 99u;
    }

    s4t_rpm_abs = s4t_rpm;
    if (s4t_rpm_abs < 0)
    {
        s4t_rpm_abs = -s4t_rpm_abs;
    }

    if (s4t_rpm_abs < 500)
    {
        s4t_rpm_index = s4t_rpm_abs / 50;
    }
    else
    {
        s4t_rpm_index = s4t_rpm_abs / 400;
    }

    if (s4t_rpm_index < 0)
    {
        s4t_rpm_index = 0;
    }
    if (s4t_rpm_index > 9)
    {
        s4t_rpm_index = 9;
    }

    s4t_cmd = (s4)s2t_foc_perc_to_centi_nm[u2t_table_index][s4t_rpm_index];

    /*=========OUTPUT==========*/
    *ps4t_trq_centi_nm = s4t_cmd;

    return EN_COM_STS_OK;
}
