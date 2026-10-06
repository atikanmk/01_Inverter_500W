/**
 ********************************************************************************
 * @file    system_clock.h
 * @brief   MCU clock setup (SystemInit / runtime clock query).
 ********************************************************************************
 */
#ifndef SYSTEM_CLOCK_H
#define SYSTEM_CLOCK_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* HSE 8 MHz * 9 = 72 MHz (preferred). */
#define SYSTEM_CLOCK_HSE_PLL_HZ 72000000u
/* HSI/2 * 16 = 64 MHz when the board has no crystal. */
#define SYSTEM_CLOCK_HSI_PLL_HZ 64000000u
/* Bare HSI if every PLL path fails. */
#define SYSTEM_CLOCK_HSI_HZ     8000000u

void SystemInit(void);
uint32_t SystemCoreClock_Get(void);
/* Timer kernel clock (TIM1 on APB2, TIM2 on APB1). Doubled when APB prescaler != 1. */
uint32_t SystemTimerClock_Get(void);

#ifdef __cplusplus
}
#endif

#endif /* SYSTEM_CLOCK_H */
