#ifndef HALL_H
#define HALL_H

#include <stdint.h>
#include "com.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Hall channels are mapped to PA1, PA2, and PA3. */
#define HALL_CH1_PIN 1u
#define HALL_CH2_PIN 2u
#define HALL_CH3_PIN 3u

/* Debounce time for Hall EXTI edges in microseconds. */
#define HALL_DEBOUNCE_US 5u

/* CPU clock used for debounce cycle calculation. */
#define HALL_CPU_CLOCK_HZ 72000000u

/* Global monitor variables (0/1) for each Hall channel. */
extern volatile uint8_t g_hall1_state;
extern volatile uint8_t g_hall2_state;
extern volatile uint8_t g_hall3_state;

/* Global hall state and mapped electrical angle (degree). */
extern volatile uint8_t g_hall_state;
extern volatile uint16_t g_hall_angle_deg;

extern volatile uint16_t hall_est_curr_ang;
extern volatile uint16_t hall_est_ang_ref;
extern volatile uint16_t hall_est_ang_next;

/* Index uses hall_state = (h3 << 2) | (h2 << 1) | h1. */
extern const uint16_t g_hall_state_angle_deg[8];

void Hall_Init(void);
uint8_t Hall_GetLevel(uint8_t channel);
uint8_t Hall_GetPattern(void);
uint32_t Hall_GetEdgeCount(uint8_t channel);
EN_COM_STS_T eng_hall_ang_est(u16 *pu16t_angle_deg);
int32_t Hall_GetElectricalRpm(void);
int32_t Hall_GetMechanicalRpm(void);

/* Optional callback; override in user code if needed. */
void Hall_OnEdge(uint8_t channel, uint8_t level);

#ifdef __cplusplus
}
#endif

#endif /* HALL_H */
