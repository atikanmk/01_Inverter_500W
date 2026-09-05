


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

/************************************
 * EXTERN VARIABLES
 ************************************/

/************************************
 * PRIVATE MACROS AND DEFINES
 ************************************/

/************************************
 * PRIVATE TYPEDEFS
 ************************************/

/************************************
 * STATIC VARIABLES
 ************************************/

/************************************
 * GLOBAL VARIABLES
 ************************************/
/* FOC angle */
s4 s4g_foc_angle_est_cdeg;  /* Estimated electrical angle in centi degrees */
/* FOC Current */
s4 s4g_foc_iu_ca;         /* Iu centi amperes */
s4 s4g_foc_iv_ca;         /* Iv centi amperes */
s4 s4g_foc_iw_ca;         /* Iw centi amperes */
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
 * STATIC FUNCTION PROTOTYPES
 ************************************/

/************************************
 * STATIC FUNCTIONS
 ************************************/

/************************************
 * GLOBAL FUNCTIONS
 ************************************/
EN_COM_STS_T eng_foc_init(void)
{
    s4g_foc_angle_est_cdeg = 0;
    s4g_foc_iu_ca = 0;
	s4g_foc_iv_ca = 0;
	s4g_foc_iw_ca = 0;
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
  
	return EN_COM_STS_OK;
}



u2 u16g_foc_main(void)
{
	u2 u2t_angle_est = 0;
	s4 s4t_u_ca = 0;
	s4 s4t_v_ca = 0;
	s4 s4t_w_ca = 0;
	s4 s4t_alpha_ca = 0;
	s4 s4t_beta_ca = 0;
	s4 s4t_d_ca = 0;
	s4 s4t_q_ca = 0;
	s4 s4t_vd_cpct = 0;
	s4 s4t_vq_cpct = 0;
	s4 s4t_valpha_cpct = 0;
	s4 s4t_vbeta_cpct = 0;
	s4 s4t_svm_u_cpct = 0;
	s4 s4t_svm_v_cpct = 0;
	s4 s4t_svm_w_cpct = 0;

	/*==========INPUT==========*/
    eng_cnv_get_phase_currents(&s4t_u_ca, &s4t_v_ca, &s4t_w_ca);
	



	/*==========OPERATION==========*/
	eng_hall_ang_est(&u2t_angle_est);
	eng_foc_clarke_transform(s4t_u_ca, s4t_v_ca, &s4t_alpha_ca, &s4t_beta_ca);
	eng_foc_park_transform(s4t_alpha_ca, s4t_beta_ca, u2t_angle_est, &s4t_d_ca, &s4t_q_ca);





	/*==========OUTPUT==========*/

	return u2t_angle_est;
}

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