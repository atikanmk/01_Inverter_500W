#include "task.h"

#include "adc.h"
#include "cnv.h"
#include "hall.h"
#include "pwm.h"

#define RCC_BASE_ADDR   0x40021000u
#define TIM2_BASE_ADDR  0x40000000u
#define NVIC_ISER0_ADDR 0xE000E100u

#define REG32(addr) (*(volatile uint32_t *)(addr))

#define RCC_APB1ENR REG32(RCC_BASE_ADDR + 0x1Cu)

#define TIM2_CR1  REG32(TIM2_BASE_ADDR + 0x00u)
#define TIM2_DIER REG32(TIM2_BASE_ADDR + 0x0Cu)
#define TIM2_SR   REG32(TIM2_BASE_ADDR + 0x10u)
#define TIM2_EGR  REG32(TIM2_BASE_ADDR + 0x14u)
#define TIM2_PSC  REG32(TIM2_BASE_ADDR + 0x28u)
#define TIM2_ARR  REG32(TIM2_BASE_ADDR + 0x2Cu)

#define NVIC_ISER0 REG32(NVIC_ISER0_ADDR)

#define RCC_APB1ENR_TIM2EN (1u << 0)
#define TIM_DIER_UIE       (1u << 0)
#define TIM_SR_UIF         (1u << 0)
#define TIM_CR1_CEN        (1u << 0)
#define TIM_EGR_UG         (1u << 0)

#define TIM2_IRQ_BIT (1u << 28)

static volatile uint32_t g_task_tick_100us;
static uint8_t g_task_div_1ms;

/* Optional 100us callback; override in user code later. */
EN_COM_STS_T eng_task_on_100us(void);
EN_COM_STS_T eng_task_on_1ms(void);

void Task_Init(void)
{
    uint32_t psc;
    uint32_t arr;

    RCC_APB1ENR |= RCC_APB1ENR_TIM2EN;

    TIM2_CR1 = 0u;
    TIM2_DIER = 0u;

    psc = (TASK_TIMER_CLOCK_HZ / 1000000u) - 1u;
    arr = TASK_100US_PERIOD_US - 1u;

    TIM2_PSC = psc;
    TIM2_ARR = arr;
    TIM2_EGR = TIM_EGR_UG;
    TIM2_SR = 0u;

    g_task_tick_100us = 0u;
    g_task_div_1ms = 0u;



    TIM2_DIER |= TIM_DIER_UIE;
    NVIC_ISER0 = TIM2_IRQ_BIT;
    TIM2_CR1 |= TIM_CR1_CEN;
}

uint32_t Task_GetTick100us(void)
{
    return g_task_tick_100us;
}

EN_COM_STS_T eng_task_on_100us(void)
{
    //g_task_hall_angle_est = eng_hall_ang_est();
    (void)ADC_TriggerFromPwmCycle();
    eng_cnv_100us();
    PWM_On100us();

    g_task_div_1ms++;
    if (g_task_div_1ms >= 10u)
    {
        g_task_div_1ms = 0u;
        eng_task_on_1ms();
    }
    return EN_COM_STS_OK;
}
volatile int32_t g_task_speed_elec_rpm;
volatile int32_t g_task_speed_mech_rpm;
EN_COM_STS_T eng_task_on_1ms(void)
{
    g_task_speed_elec_rpm = Hall_GetElectricalRpm();
    g_task_speed_mech_rpm = Hall_GetMechanicalRpm();

    PWM_SetDutyUVW(500u, 500u, 500u);
    return EN_COM_STS_OK;
}

void TIM2_IRQHandler(void)
{
    if ((TIM2_SR & TIM_SR_UIF) != 0u)
    {
        TIM2_SR &= ~TIM_SR_UIF;
        g_task_tick_100us++;
        eng_task_on_100us();
    }
}
