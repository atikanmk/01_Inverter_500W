/**
 ********************************************************************************
 * @file    foc.h
 * @author  Atikan
 * @date    2026-07-19
 * @brief   FOC control public interface.
 ********************************************************************************
 */

#ifndef FOC_H
#define FOC_H

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

/************************************
 * TYPEDEFS
 ************************************/

/************************************
 * EXTERN VARIABLES
 ************************************/

/************************************
 * FUNCTION PROTOTYPES
 ************************************/
extern EN_COM_STS_T eng_foc_init(void);
extern EN_COM_STS_T eng_foc_main(void);

#ifdef __cplusplus
}
#endif

#endif /* FOC_H */
