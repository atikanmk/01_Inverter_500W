


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
s32 s32g_foc_angle_est_cdeg;  /* Estimated electrical angle in centi degrees */
/* FOC Current */
s32 s32g_foc_iu_ca;         /* Iu centi amperes */
s32 s32g_foc_iv_ca;         /* Iv centi amperes */
s32 s32g_foc_iw_ca;         /* Iw centi amperes */
s32 s32g_foc_ialpha_ca;     /* IALPHA centi amperes */
s32 s32g_foc_ibeta_ca;      /* IBETA centi amperes */
s32 s32g_foc_iq_ref_ca;     /* IQ centi amperes */
s32 s32g_foc_id_ref_ca;     /* ID centi amperes */
s32 s32g_foc_iq_feb_ca;     /* IQ feedback centi amperes */
s32 s32g_foc_id_feb_ca;     /* ID feedback centi amperes */
/* FOC Voltage */
s32 s32g_foc_vq_ref_cpct;     /* VQ centi percent */
s32 s32g_foc_vd_ref_cpct;     /* VD centi percent */
s32 s32g_foc_valpha_cpct;     /* VALPHA centi percent */
s32 s32g_foc_vbeta_cpct;      /* VBETA centi percent */
s32 s32g_foc_svm_u_cpct;      /* SVM U centi percent */
s32 s32g_foc_svm_v_cpct;      /* SVM V centi percent */
s32 s32g_foc_svm_w_cpct;      /* SVM W centi percent */

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
    s32g_foc_angle_est_cdeg = 0;
	s32g_foc_iu_ca = 0;
	s32g_foc_iv_ca = 0;
	s32g_foc_iw_ca = 0;
	s32g_foc_ialpha_ca = 0;
	s32g_foc_ibeta_ca = 0;
	s32g_foc_iq_ref_ca = 0;
	s32g_foc_id_ref_ca = 0;
	s32g_foc_iq_feb_ca = 0;
	s32g_foc_id_feb_ca = 0;
	s32g_foc_vq_ref_cpct = 0;
	s32g_foc_vd_ref_cpct = 0;
	s32g_foc_valpha_cpct = 0;
	s32g_foc_vbeta_cpct = 0;
	s32g_foc_svm_u_cpct = 0;
	s32g_foc_svm_v_cpct = 0;
	s32g_foc_svm_w_cpct = 0;
  
	return EN_COM_STS_OK;
}



u16 u16g_foc_main(void)
{
	u16 u16t_angle_est = 0;
	s32 s32t_u_ca = 0;
	s32 s32t_v_ca = 0;
	s32 s32t_w_ca = 0;
	s32 s32t_alpha_ca = 0;
	s32 s32t_beta_ca = 0;
	s32 s32t_d_ca = 0;
	s32 s32t_q_ca = 0;
	s32 s32t_vd_cpct = 0;
	s32 s32t_vq_cpct = 0;
	s32 s32t_valpha_cpct = 0;
	s32 s32t_vbeta_cpct = 0;
	s32 s32t_svm_u_cpct = 0;
	s32 s32t_svm_v_cpct = 0;
	s32 s32t_svm_w_cpct = 0;

	/*==========INPUT==========*/
    //eng_cnv_get_phase_currents(&s32t_u_ca, &s32t_v_ca, &s32t_w_ca);
	



	/*==========OPERATION==========*/
	//eng_hall_ang_est(&u16t_angle_est);




	/*==========OUTPUT==========*/

	return u16t_angle_est;
}

EN_COM_STS_T eng_foc_park_transform(s32 s32t_alpha, s32 s32t_beta, u16 u16t_angle_deg, s32 *s32t_d, s32 *s32t_q)
{
	/* Convert angle from degrees to radians. */
	float angle_rad = (float)u16t_angle_deg * (3.14159265358979323846f / 180.0f);

	/* Calculate sine and cosine of the angle. */
	float sin_angle = sinf(angle_rad);
	float cos_angle = cosf(angle_rad);

	/* Perform Park transformation. */
	*s32t_d = (s32)((s32)s32t_alpha * cos_angle + (s32)s32t_beta * sin_angle);
	*s32t_q = (s32)((-((s32)s32t_alpha) * sin_angle) + ((s32)s32t_beta * cos_angle));

	return EN_COM_STS_OK;
}
