#ifndef TASK_H
#define TASK_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TASK_100US_PERIOD_US 100u
#define TASK_TIMER_CLOCK_HZ 8000000u

void Task_Init(void);
uint32_t Task_GetTick100us(void);

extern volatile uint16_t g_task_hall_angle_est;
extern volatile int32_t g_task_speed_elec_rpm;
extern volatile int32_t g_task_speed_mech_rpm;
extern volatile uint16_t g_task_phase_v_mv;
extern volatile uint16_t g_task_phase_w_mv;
extern volatile uint16_t g_task_phase_v_offset_mv;
extern volatile uint16_t g_task_phase_w_offset_mv;
extern volatile int32_t g_task_phase_u_current_ca;
extern volatile int32_t g_task_phase_v_current_ca;
extern volatile int32_t g_task_phase_w_current_ca;
extern volatile uint8_t g_task_phase_offset_ready;

/* Optional 100us callback; override in user code later. */
void Task_On100us(void);
void Task_On1ms(void);

#ifdef __cplusplus
}
#endif

#endif /* TASK_H */
