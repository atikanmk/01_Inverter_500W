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
EN_COM_STS_T eng_cnv_init(void);
EN_COM_STS_T eng_cnv_100us(void);
EN_COM_STS_T eng_cnv_get_phase_currents(int32_t *phase_u_ca, int32_t *phase_v_ca, int32_t *phase_w_ca);

#ifdef __cplusplus
}
#endif

#endif /* CNV_H */
