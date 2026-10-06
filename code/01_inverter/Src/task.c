/**
 ********************************************************************************
 * @file    task.c
 * @author  Atikan
 * @date    2026-09-06
 * @brief   Cyclic task scheduler implementation.
 ********************************************************************************
 */

#include "task.h"

/************************************
 * INCLUDES
 ************************************/
#include "adc.h"
#include "cnv.h"
#include "drv_mng.h"
#include "hall.h"
#include "pwm.h"
#include "foc.h"
#include "system_clock.h"

/************************************
 * PRIVATE MACROS AND DEFINES
 ************************************/
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
#define TASK_IRQ_PERIOD 100u  /* 100 microseconds */
#define TASK_1000_USEC  1000u  /* 1000 microseconds */

/************************************
 * STATIC VARIABLES
 ************************************/
static volatile u4 u4sg_task_tick_100us;
/* Accumulated us from TIM2; eng_task_main() drains 1 ms work in background. */
static volatile u4 u4sg_task_div_1ms_us;
/* Main drains ADC/cnv when set by TIM2 (keeps ISR short). */
static volatile u1 u1sg_task_do_adc_cnv;
/* Debug: how many times TIM2 nested/overran (UIF already set on entry). */
volatile u4 u4g_task_isr_overrun;

/************************************
 * GLOBAL VARIABLES
 ************************************/

/************************************
 * FUNCTION PROTOTYPES
 ************************************/
EN_COM_STS_T eng_task_on_100us(void);
EN_COM_STS_T eng_task_on_1ms(void);

/************************************
 * FUNCTIONS
 ************************************/
/**
 * @fn     eng_task_init
 * @id     TASK-001
 * @brief  Initialize the TIM2 cyclic task scheduler
 * @return Task initialization status
 */
EN_COM_STS_T eng_task_init(void)
{
    u4 u4t_psc;
    u4 u4t_arr;

    RCC_APB1ENR |= RCC_APB1ENR_TIM2EN;

    TIM2_CR1 = 0u;
    TIM2_DIER = 0u;

    /* Use the clock SystemInit actually achieved (64 or 72 MHz), not a hard 72. */
    u4t_psc = (SystemTimerClock_Get() / 1000000u) - 1u;
    u4t_arr = TASK_100US_PERIOD_US - 1u;

    TIM2_PSC = u4t_psc;
    TIM2_ARR = u4t_arr;
    TIM2_EGR = TIM_EGR_UG;
    TIM2_SR = 0u;

    u4sg_task_tick_100us = 0u;
    u4sg_task_div_1ms_us = 0u;
    u1sg_task_do_adc_cnv = 0u;
    u4g_task_isr_overrun = 0u;

    TIM2_DIER |= TIM_DIER_UIE;
    NVIC_ISER0 = TIM2_IRQ_BIT;
    TIM2_CR1 |= TIM_CR1_CEN;

    return EN_COM_STS_OK;
}

EN_COM_STS_T eng_task_main(void)
{
    u4 u4t_due_slots;
    u4 u4t_i;
    u1 u1t_do_adc;

    /*
     * ADC + current conversion in normal context (not TIM2).
     * At 8 MHz / heavy -Og, blocking ADC inside 100 us ISR freezes main cnt.
     */
    __asm volatile ("cpsid i" ::: "memory");
    u1t_do_adc = u1sg_task_do_adc_cnv;
    u1sg_task_do_adc_cnv = 0u;
    __asm volatile ("cpsie i" ::: "memory");

    if (u1t_do_adc != 0u)
    {
        (void)ADC_TriggerFromPwmCycle();
        (void)eng_cnv_100us();
        (void)eng_foc_main();
    }

    /*
     * Normal context 1 ms task (not inside TIM2_IRQHandler).
     * Claim due slots under brief IRQ mask, then run work with IRQ enabled
     * so FOC/Hall can still preempt a long 1 ms burst.
     */
    __asm volatile ("cpsid i" ::: "memory");
    u4t_due_slots = u4sg_task_div_1ms_us / TASK_1000_USEC;
    u4sg_task_div_1ms_us -= (u4t_due_slots * TASK_1000_USEC);
    __asm volatile ("cpsie i" ::: "memory");

    /* Cap catch-up so a long stall does not block main forever. */
    if (u4t_due_slots > 5u)
    {
        u4t_due_slots = 5u;
    }

    for (u4t_i = 0u; u4t_i < u4t_due_slots; u4t_i++)
    {
        (void)eng_task_on_1ms();
    }

    return EN_COM_STS_OK;
}

/**
 * @fn     u4g_task_get_tick_100us
 * @id     TASK-002
 * @brief  Get elapsed 100 microsecond task ticks
 * @return Number of elapsed 100 microsecond ticks
 */
u4 u4g_task_get_tick_100us(void)
{
    return u4sg_task_tick_100us;
}

/**
 * @fn     eng_task_on_100us
 * @id     TASK-003
 * @brief  Execute the 100 microsecond cyclic task
 * @return 100 microsecond task status
 */
EN_COM_STS_T eng_task_on_100us(void)
{
    /*
     * CCR update only. FOC/ADC/cnv run in main.
     * At the real 8 MHz HSI a full FOC call overruns 100 us (overrun ~= tick).
     */
    vdg_pwm_on_100_us();
    u1sg_task_do_adc_cnv = 1u;

    return EN_COM_STS_OK;
}

/**
 * @fn     eng_task_on_1ms
 * @id     TASK-004
 * @brief  Execute the 1 millisecond cyclic task
 * @return 1 millisecond task status
 */
EN_COM_STS_T eng_task_on_1ms(void)
{
    (void)eng_cnv_1ms();
    (void)eng_drv_mng_1ms();

    return EN_COM_STS_OK;
}

/**
 * @fn     TIM2_IRQHandler
 * @id     TASK-005
 * @brief  Handle TIM2 update interrupts for the cyclic task scheduler
 * @return None
 */
void TIM2_IRQHandler(void)
{
    if ((TIM2_SR & TIM_SR_UIF) != 0u)
    {
        TIM2_SR &= ~TIM_SR_UIF;

        u4sg_task_tick_100us++;
        (void)eng_task_on_100us();

        /* Timebase only — do not call eng_task_on_1ms() / ADC / FOC here. */
        u4sg_task_div_1ms_us += TASK_IRQ_PERIOD;

        /* UIF already set again => this ISR took longer than one period. */
        if ((TIM2_SR & TIM_SR_UIF) != 0u)
        {
            u4g_task_isr_overrun++;
            TIM2_SR &= ~TIM_SR_UIF;
        }
    }
}


