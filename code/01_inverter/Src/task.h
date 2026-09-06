/**
 ********************************************************************************
 * @file    task.h
 * @author  Atikan
 * @date    2026-09-06
 * @brief   Cyclic task scheduler interface.
 ********************************************************************************
 */

#ifndef TASK_H
#define TASK_H

/************************************
 * INCLUDES
 ************************************/
#include <stdint.h>
#include "com.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TASK_100US_PERIOD_US  100u
#define TASK_TIMER_CLOCK_HZ   8000000u

/************************************
 * TYPEDEFS
 ************************************/

/************************************
 * EXTERN VARIABLES
 ************************************/
extern volatile s4 s4g_task_speed_elec_rpm;
extern volatile s4 s4g_task_speed_mech_rpm;

/************************************
 * FUNCTION PROTOTYPES
 ************************************/
EN_COM_STS_T eng_task_init(void);
u4 u4g_task_get_tick_100us(void);
EN_COM_STS_T eng_task_on_100us(void);
EN_COM_STS_T eng_task_on_1ms(void);
EN_COM_STS_T eng_task_main(void);

#ifdef __cplusplus
}
#endif

#endif /* TASK_H */
