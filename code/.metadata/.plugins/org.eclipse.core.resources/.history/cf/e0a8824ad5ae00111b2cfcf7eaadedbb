/**
 ********************************************************************************
 * @file    hall.h
 * @author  Atikan
 * @date    2026-08-23
 * @brief   Hall sensor interface.
 ********************************************************************************
 */

#ifndef HALL_H
#define HALL_H

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
/* Hall channels are mapped to PA1, PA2, and PA3. */
#define HALL_CH1_PIN 1u
#define HALL_CH2_PIN 2u
#define HALL_CH3_PIN 3u

/* Debounce time for Hall EXTI edges in microseconds. */
#define HALL_DEBOUNCE_US 5u

/* CPU clock used for debounce cycle calculation. */
#define HALL_CPU_CLOCK_HZ 72000000u

/************************************
 * TYPEDEFS
 ************************************/

/************************************
 * EXPORTED VARIABLES
 ************************************/

/************************************
 * GLOBAL FUNCTION PROTOTYPES
 ************************************/
void Hall_Init(void);
uint8_t Hall_GetLevel(uint8_t channel);
uint8_t Hall_GetPattern(void);
uint32_t Hall_GetEdgeCount(uint8_t channel);
EN_COM_STS_T eng_hall_ang_est(u2 *pu16t_angle_deg);
int32_t Hall_GetElectricalRpm(void);
int32_t Hall_GetMechanicalRpm(void);

/* Optional callback; override in user code if needed. */
void Hall_OnEdge(uint8_t channel, uint8_t level);

#ifdef __cplusplus
}
#endif

#endif /* HALL_H */
