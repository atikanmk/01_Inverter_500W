#ifndef PWM_H
#define PWM_H

#include <stdint.h>
#include "com.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PWM_TIMER_CLOCK_HZ 8000000u
#define PWM_PERIOD_US      100u
#define PWM_DUTY_MAX       1000u
#define PWM_DEADTIME_US    4u

/* Output active level configuration: 1 = active high, 0 = active low. */
#define PWM_HI_ACTIVE_LEVEL 1u
#define PWM_LO_ACTIVE_LEVEL 0u

/* TIM1 default pin mapping on STM32F103:
 * UH: PA8, UL: PB13, VH: PA9, VL: PB14, WH: PA10, WL: PB15
 */
void PWM_Init(void);
void eng_pwm_set_duty(uint16_t duty_u, uint16_t duty_v, uint16_t duty_w);
void vdg_pwm_on_100_us(void);
void PWM_EnableOutput(void);
void PWM_DisableOutput(void);

#ifdef __cplusplus
}
#endif

#endif /* PWM_H */
