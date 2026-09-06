/**
 ********************************************************************************
 * @file    cnv.h
 * @author  Atikan
 * @date    2026-08-23
 * @brief   
 ********************************************************************************
 */

#ifndef CNV_H
#define CNV_H

/************************************
 * INCLUDES
 ************************************/
#include <stdint.h>
#include "com.h"

#ifdef __cplusplus
extern "C" {
#endif

/************************************
 * MACROS AND DEFINES
 ************************************/
#define CNV_OFFSET_SAMPLE_COUNT 1024u

/************************************
 * TYPEDEFS
 ************************************/

/************************************
 * EXTERN VARIABLES
 ************************************/

/************************************
 * FUNCTION PROTOTYPES
 ************************************/
extern EN_COM_STS_T eng_cnv_init(void);
extern EN_COM_STS_T eng_cnv_100us(void);
extern EN_COM_STS_T eng_cnv_1ms(void);
extern EN_COM_STS_T eng_cnv_get_phase_currents(s4 *ps4t_phase_u_ca, s4 *ps4t_phase_v_ca, s4 *ps4t_phase_w_ca);
extern EN_COM_STS_T eng_cnv_get_throttle_perc(u2 *pu2t_throttle_perc);


#ifdef __cplusplus
}
#endif

#endif /* CNV_H */
