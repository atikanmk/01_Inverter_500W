


/**
 ********************************************************************************
 * @file    foc.c
 * @author  Atikan
 * @date    2026-07-19
 * @brief   
 ********************************************************************************
 */

/************************************
 * INCLUDES
 ************************************/
#include "foc.h"
#include "hall.h"
#include "math.h"
#include "cnv.h"
#include "pwm.h"

/************************************
 * EXTERN VARIABLES
 ************************************/

/************************************
 * PRIVATE MACROS AND DEFINES
 ************************************/
#define FOC_PI_SCALE                 1024
#define FOC_CURRENT_PI_KP_Q10        512
#define FOC_CURRENT_PI_KI_Q10        16
#define FOC_VOLTAGE_LIMIT_CPCT       10000

/************************************
 * PRIVATE TYPEDEFS
 ************************************/

/************************************
 * STATIC VARIABLES
 ************************************/
static s4 s4g_foc_iq_pi_integral_q10;
static s4 s4g_foc_id_pi_integral_q10;

/************************************
 * GLOBAL VARIABLES
 ************************************/
/* FOC angle */
s4 s4g_foc_angle_est_deg;  /* Estimated electrical angle in centi degrees */
/* FOC Current */
s4 s4g_foc_ialpha_ca;     /* IALPHA centi amperes */
s4 s4g_foc_ibeta_ca;      /* IBETA centi amperes */
s4 s4g_foc_iq_ref_ca;     /* IQ centi amperes */
s4 s4g_foc_id_ref_ca;     /* ID centi amperes */
s4 s4g_foc_iq_feb_ca;     /* IQ feedback centi amperes */
s4 s4g_foc_id_feb_ca;     /* ID feedback centi amperes */
/* FOC Voltage */
s4 s4g_foc_vq_ref_cpct;     /* VQ centi percent */
s4 s4g_foc_vd_ref_cpct;     /* VD centi percent */
s4 s4g_foc_valpha_cpct;     /* VALPHA centi percent */
s4 s4g_foc_vbeta_cpct;      /* VBETA centi percent */
s4 s4g_foc_svm_u_cpct;      /* SVM U centi percent */
s4 s4g_foc_svm_v_cpct;      /* SVM V centi percent */
s4 s4g_foc_svm_w_cpct;      /* SVM W centi percent */

/************************************
 * FUNCTION PROTOTYPES
 ************************************/
EN_COM_STS_T eng_foc_park_transform(s4 s4t_alpha, s4 s4t_beta, u2 u16t_angle_deg, s4 *s4t_d, s4 *s4t_q);
EN_COM_STS_T eng_foc_clarke_transform(s4 s4t_u, s4 s4t_v, s4 *s4t_alpha, s4 *s4t_beta);
EN_COM_STS_T eng_foc_inverse_park_transform(s4 s4t_d, s4 s4t_q, u2 u16t_angle_deg, s4 *s4t_alpha, s4 *s4t_beta);
EN_COM_STS_T eng_foc_svm(s4 s4t_valpha, s4 s4t_vbeta, s4 *s4t_svm_u, s4 *s4t_svm_v, s4 *s4t_svm_w);
/************************************
 * FUNCTIONS
 ************************************/

/**
 * @fn     eng_foc_init
 * @id     FOC-001
 * @brief  Initialize FOC states and PI controller integrators
 * @return FOC initialization status
 */
EN_COM_STS_T eng_foc_init(void)
{
    s4g_foc_angle_est_deg = 0;
	s4g_foc_ialpha_ca = 0;
	s4g_foc_ibeta_ca = 0;
	s4g_foc_iq_ref_ca = 0;
	s4g_foc_id_ref_ca = 0;
	s4g_foc_iq_feb_ca = 0;
	s4g_foc_id_feb_ca = 0;
	s4g_foc_vq_ref_cpct = 0;
	s4g_foc_vd_ref_cpct = 0;
	s4g_foc_valpha_cpct = 0;
	s4g_foc_vbeta_cpct = 0;
	s4g_foc_svm_u_cpct = 0;
	s4g_foc_svm_v_cpct = 0;
	s4g_foc_svm_w_cpct = 0;
	s4g_foc_iq_pi_integral_q10 = 0;
	s4g_foc_id_pi_integral_q10 = 0;
  
	return EN_COM_STS_OK;
}

/**
 * @fn     u16g_foc_main
 * @id     FOC-002
 * @brief  Execute one FOC calculation cycle
 * @return Estimated electrical angle in degrees
 */
u2 u16g_foc_main(void)
{
	s4 s4t_iq_ref_ca;
	s4 s4t_id_ref_ca;
	s4 s4t_id_feb_ca;
	s4 s4t_iq_feb_ca;
	u2 u2t_angle_est;
	s4 s4t_u_ca;
	s4 s4t_v_ca;
	s4 s4t_w_ca;
	s4 s4t_alpha_ca;
	s4 s4t_beta_ca;
	s4 s4t_vd_cpct = 0;
	s4 s4t_vq_cpct = 0;
	s4 s4t_valpha_cpct = 0;
	s4 s4t_vbeta_cpct = 0;
	s4 s4t_svm_u_cpct = 0;
	s4 s4t_svm_v_cpct = 0;
	s4 s4t_svm_w_cpct = 0;
	s4 s4t_trq = 0;

	/*==========INPUT==========*/
    eng_cnv_get_phase_currents(&s4t_u_ca, &s4t_v_ca, &s4t_w_ca);
	eng_hall_ang_est(&u2t_angle_est);

	/*==========OPERATION==========*/
	/* Input calcaulate */
	eng_foc_clarke_transform(s4t_u_ca, s4t_v_ca, &s4t_alpha_ca, &s4t_beta_ca);
	eng_foc_park_transform(s4t_alpha_ca, s4t_beta_ca, u2t_angle_est, &s4t_id_feb_ca, &s4t_iq_feb_ca);
	eng_foc_curr_ref(s4t_trq,s4t_iq_feb_ca,&s4t_iq_ref_ca,&s4t_id_ref_ca);
	eng_foc_curr_pi_cal(s4t_iq_ref_ca,s4t_id_ref_ca,s4t_iq_feb_ca,s4t_id_feb_ca,&s4t_vq_cpct,&s4t_vd_cpct);
	eng_foc_inverse_park_transform(s4t_vd_cpct,s4t_vq_cpct,u2t_angle_est, &s4t_valpha_cpct, &s4t_vbeta_cpct);
	eng_foc_svm(s4t_valpha_cpct, s4t_vbeta_cpct, &s4t_svm_u_cpct, &s4t_svm_v_cpct, &s4t_svm_w_cpct);
	eng_pwm_set_duty(s4t_svm_u_cpct, s4t_svm_v_cpct, s4t_svm_w_cpct);

	/*==========OUTPUT==========*/
    s4g_foc_angle_est_deg = u2t_angle_est;
	s4g_foc_ialpha_ca = s4t_alpha_ca;
	s4g_foc_ibeta_ca = s4t_beta_ca;
	s4g_foc_iq_ref_ca = s4t_iq_ref_ca;
	s4g_foc_id_ref_ca = s4t_id_ref_ca;
	s4g_foc_iq_feb_ca = s4t_iq_feb_ca;
	s4g_foc_id_feb_ca = s4t_id_feb_ca;
	s4g_foc_vq_ref_cpct = s4t_vq_cpct;
	s4g_foc_vd_ref_cpct = s4t_vd_cpct;
	s4g_foc_valpha_cpct = s4t_valpha_cpct;
	s4g_foc_vbeta_cpct = s4t_vbeta_cpct;
	s4g_foc_svm_u_cpct = s4t_svm_u_cpct;
	s4g_foc_svm_v_cpct = s4t_svm_v_cpct;
	s4g_foc_svm_w_cpct = s4t_svm_w_cpct;

	return u2t_angle_est;
}

/**
 * @fn     eng_foc_curr_ref
 * @id     FOC-003
 * @brief  Generate d-axis and q-axis current references from torque command
 * @param  s4t_trq_cmd_ca: Torque command represented as q-axis current in centi amperes (s4)
 * @param  s4t_iq_feb_ca: Q-axis current feedback in centi amperes (s4)
 * @param  s4t_iq_ref_ca: Pointer to q-axis current reference in centi amperes (s4)
 * @param  s4t_id_ref_ca: Pointer to d-axis current reference in centi amperes (s4)
 * @return Current reference calculation status
 */
EN_COM_STS_T eng_foc_curr_ref(s4 s4t_trq_cmd_ca, s4 s4t_iq_feb_ca, s4 *s4t_iq_ref_ca, s4 *s4t_id_ref_ca)
{

	*s4t_iq_ref_ca = s4t_trq_cmd_ca;
	*s4t_id_ref_ca = 0;

	return EN_COM_STS_OK;
}

/**
 * @fn     eng_foc_curr_pi_cal
 * @id     FOC-004
 * @brief  Calculate d-axis and q-axis voltage commands using PI control
 * @param  s4t_iq_ref_ca: Q-axis current reference in centi amperes (s4)
 * @param  s4t_id_ref_ca: D-axis current reference in centi amperes (s4)
 * @param  s4t_iq_feb_ca: Q-axis current feedback in centi amperes (s4)
 * @param  s4t_id_feb_ca: D-axis current feedback in centi amperes (s4)
 * @param  s4t_vq_cpct: Pointer to q-axis voltage command in centi percent (s4)
 * @param  s4t_vd_cpct: Pointer to d-axis voltage command in centi percent (s4)
 * @return Current PI calculation status
 */
EN_COM_STS_T eng_foc_curr_pi_cal(s4 s4t_iq_ref_ca, s4 s4t_id_ref_ca, s4 s4t_iq_feb_ca, s4 s4t_id_feb_ca, s4 *s4t_vq_cpct, s4 *s4t_vd_cpct)
{
	s4 s4t_iq_error_ca;
	s4 s4t_id_error_ca;
	s4 s4t_iq_integral_next_q10;
	s4 s4t_id_integral_next_q10;
	s4 s4t_vq_unlimited_cpct;
	s4 s4t_vd_unlimited_cpct;

	s4t_iq_error_ca = s4t_iq_ref_ca - s4t_iq_feb_ca;
	s4t_id_error_ca = s4t_id_ref_ca - s4t_id_feb_ca;
	s4t_iq_integral_next_q10 = s4g_foc_iq_pi_integral_q10 + (s4t_iq_error_ca * FOC_CURRENT_PI_KI_Q10);
	s4t_id_integral_next_q10 = s4g_foc_id_pi_integral_q10 + (s4t_id_error_ca * FOC_CURRENT_PI_KI_Q10);
	s4t_vq_unlimited_cpct = ((s4t_iq_error_ca * FOC_CURRENT_PI_KP_Q10) + s4t_iq_integral_next_q10) / FOC_PI_SCALE;
	s4t_vd_unlimited_cpct = ((s4t_id_error_ca * FOC_CURRENT_PI_KP_Q10) + s4t_id_integral_next_q10) / FOC_PI_SCALE;

	if (s4t_vq_unlimited_cpct > FOC_VOLTAGE_LIMIT_CPCT)
	{
		*s4t_vq_cpct = FOC_VOLTAGE_LIMIT_CPCT;
	}
	else if (s4t_vq_unlimited_cpct < -FOC_VOLTAGE_LIMIT_CPCT)
	{
		*s4t_vq_cpct = -FOC_VOLTAGE_LIMIT_CPCT;
	}
	else
	{
		s4g_foc_iq_pi_integral_q10 = s4t_iq_integral_next_q10;
		*s4t_vq_cpct = s4t_vq_unlimited_cpct;
	}

	if (s4t_vd_unlimited_cpct > FOC_VOLTAGE_LIMIT_CPCT)
	{
		*s4t_vd_cpct = FOC_VOLTAGE_LIMIT_CPCT;
	}
	else if (s4t_vd_unlimited_cpct < -FOC_VOLTAGE_LIMIT_CPCT)
	{
		*s4t_vd_cpct = -FOC_VOLTAGE_LIMIT_CPCT;
	}
	else
	{
		s4g_foc_id_pi_integral_q10 = s4t_id_integral_next_q10;
		*s4t_vd_cpct = s4t_vd_unlimited_cpct;
	}

	return EN_COM_STS_OK;
}

/**
 * @fn     eng_foc_park_transform
 * @id     FOC-005
 * @brief  Convert alpha-beta quantities to d-q rotating reference frame
 * @param  s4t_alpha: Alpha-axis quantity (s4)
 * @param  s4t_beta: Beta-axis quantity (s4)
 * @param  u16t_angle_deg: Electrical angle in degrees (u2)
 * @param  s4t_d: Pointer to d-axis result (s4)
 * @param  s4t_q: Pointer to q-axis result (s4)
 * @return Park transformation status
 */
EN_COM_STS_T eng_foc_park_transform(s4 s4t_alpha, s4 s4t_beta, u2 u16t_angle_deg, s4 *s4t_d, s4 *s4t_q)
{
	f4 angle_rad;
    f4 sin_angle;
    f4 cos_angle;
	/*==========INPUT==========*/
	/*========OPERATION========*/
	/* Convert angle from degrees to radians. */
    angle_rad = (f4)u16t_angle_deg * (3.14159265358979323846f / 180.0f);
	/* Calculate sine and cosine of the angle. */
	sin_angle = sinf(angle_rad);
	cos_angle = cosf(angle_rad);
	/* Perform Park transformation. */
	*s4t_d = (s4)((s4)s4t_alpha * cos_angle + (s4)s4t_beta * sin_angle);
	*s4t_q = (s4)((-((s4)s4t_alpha) * sin_angle) + ((s4)s4t_beta * cos_angle));
    /*=========OUTPUT==========*/

	return EN_COM_STS_OK;
}

/**
 * @fn     eng_foc_clarke_transform
 * @id     FOC-006
 * @brief  Convert two phase quantities to alpha-beta stationary reference frame
 * @param  s4t_u: U-phase quantity (s4)
 * @param  s4t_v: V-phase quantity (s4)
 * @param  s4t_alpha: Pointer to alpha-axis result (s4)
 * @param  s4t_beta: Pointer to beta-axis result (s4)
 * @return Clarke transformation status
 */
EN_COM_STS_T eng_foc_clarke_transform(s4 s4t_u, s4 s4t_v, s4 *s4t_alpha, s4 *s4t_beta)
{
	/*==========INPUT==========*/
	/*========OPERATION========*/
	/* Perform Clarke transformation. */
	*s4t_alpha = s4t_u;
	*s4t_beta = (s4)((s4)s4t_u * 0.57735026919f + (s4)s4t_v * 1.15470053838f);
	/*=========OUTPUT==========*/

	return EN_COM_STS_OK;
}

/**
 * @fn     eng_foc_inverse_park_transform
 * @id     FOC-007
 * @brief  Convert d-q quantities to alpha-beta stationary reference frame
 * @param  s4t_d: D-axis quantity (s4)
 * @param  s4t_q: Q-axis quantity (s4)
 * @param  u16t_angle_deg: Electrical angle in degrees (u2)
 * @param  s4t_alpha: Pointer to alpha-axis result (s4)
 * @param  s4t_beta: Pointer to beta-axis result (s4)
 * @return Inverse Park transformation status
 */
EN_COM_STS_T eng_foc_inverse_park_transform(s4 s4t_d, s4 s4t_q, u2 u16t_angle_deg, s4 *s4t_alpha, s4 *s4t_beta)
{
	f4 angle_rad;
	f4 sin_angle;
	f4 cos_angle;
	/*==========INPUT==========*/
	/*========OPERATION========*/
	/* Convert angle from degrees to radians. */
	angle_rad = (f4)u16t_angle_deg * (3.14159265358979323846f / 180.0f);
	/* Calculate sine and cosine of the angle. */
	sin_angle = sinf(angle_rad);
	cos_angle = cosf(angle_rad);
	/* Perform Inverse Park transformation. */
	*s4t_alpha = (s4)((s4)s4t_d * cos_angle - (s4)s4t_q * sin_angle);
	*s4t_beta = (s4)((s4)s4t_d * sin_angle + (s4)s4t_q * cos_angle);
	/*=========OUTPUT==========*/

	return EN_COM_STS_OK;
}

/**
 * @fn     eng_foc_svm
 * @id     FOC-008
 * @brief  Calculate three phase voltage commands using space vector modulation
 * @param  s4t_valpha: Alpha-axis voltage command in centi percent (s4)
 * @param  s4t_vbeta: Beta-axis voltage command in centi percent (s4)
 * @param  s4t_svm_u: Pointer to U-phase voltage command in centi percent (s4)
 * @param  s4t_svm_v: Pointer to V-phase voltage command in centi percent (s4)
 * @param  s4t_svm_w: Pointer to W-phase voltage command in centi percent (s4)
 * @return Space vector modulation status
 */
EN_COM_STS_T eng_foc_svm(s4 s4t_valpha, s4 s4t_vbeta, s4 *s4t_svm_u, s4 *s4t_svm_v, s4 *s4t_svm_w)
{
	/*==========INPUT==========*/
	/*========OPERATION========*/
	/* Perform Space Vector Modulation (SVM) */
	*s4t_svm_u = (s4)(s4t_valpha + 0.5f * s4t_vbeta);
	*s4t_svm_v = (s4)(-0.5f * s4t_valpha + 0.86602540378f * s4t_vbeta);
	*s4t_svm_w = (s4)(-0.5f * s4t_valpha - 0.86602540378f * s4t_vbeta);
	/*=========OUTPUT==========*/

	return EN_COM_STS_OK;
}
