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
/* Must match SystemInit (HSE+PLL 72 MHz). If HSE fails, timers run slow. */
#define TASK_TIMER_CLOCK_HZ   72000000u

/************************************
 * TYPEDEFS
 ************************************/

/************************************
 * EXTERN VARIABLES
 ************************************/

/************************************
 * FUNCTION PROTOTYPES
 ************************************/
EN_COM_STS_T eng_task_init(void);
u4 u4g_task_get_tick_100us(void);
EN_COM_STS_T eng_task_on_100us(void);
EN_COM_STS_T eng_task_on_1ms(void);
EN_COM_STS_T eng_task_main(void);

/* Debug: increments if TIM2 period is overrun (watch alongside cnt). */
extern volatile u4 u4g_task_isr_overrun;

#ifdef __cplusplus
}
#endif

#endif /* TASK_H */
