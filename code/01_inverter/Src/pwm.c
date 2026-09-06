/**
 ********************************************************************************
 * @file    pwm.c
 * @author  Atikan
 * @date    2026-09-07
 * @brief
 ********************************************************************************
 */

/************************************
 * INCLUDES
 ************************************/
#include "pwm.h"

/************************************
 * EXTERN VARIABLES
 ************************************/

/************************************
 * PRIVATE MACROS AND DEFINES
 ************************************/
#define RCC_BASE_ADDR   0x40021000u
#define GPIOA_BASE_ADDR 0x40010800u
#define GPIOB_BASE_ADDR 0x40010C00u
#define TIM1_BASE_ADDR  0x40012C00u

#define REG32(addr) (*(volatile u4 *)(addr))

#define RCC_APB2ENR REG32(RCC_BASE_ADDR + 0x18u)

#define GPIOA_CRH REG32(GPIOA_BASE_ADDR + 0x04u)
#define GPIOB_CRH REG32(GPIOB_BASE_ADDR + 0x04u)

#define TIM1_CR1   REG32(TIM1_BASE_ADDR + 0x00u)
#define TIM1_EGR   REG32(TIM1_BASE_ADDR + 0x14u)
#define TIM1_CCMR1 REG32(TIM1_BASE_ADDR + 0x18u)
#define TIM1_CCMR2 REG32(TIM1_BASE_ADDR + 0x1Cu)
#define TIM1_CCER  REG32(TIM1_BASE_ADDR + 0x20u)
#define TIM1_PSC   REG32(TIM1_BASE_ADDR + 0x28u)
#define TIM1_ARR   REG32(TIM1_BASE_ADDR + 0x2Cu)
#define TIM1_CCR1  REG32(TIM1_BASE_ADDR + 0x34u)
#define TIM1_CCR2  REG32(TIM1_BASE_ADDR + 0x38u)
#define TIM1_CCR3  REG32(TIM1_BASE_ADDR + 0x3Cu)
#define TIM1_BDTR  REG32(TIM1_BASE_ADDR + 0x44u)

#define RCC_APB2ENR_AFIOEN  (1u << 0)
#define RCC_APB2ENR_IOPAEN  (1u << 2)
#define RCC_APB2ENR_IOPBEN  (1u << 3)
#define RCC_APB2ENR_TIM1EN  (1u << 11)

#define TIM_CR1_CEN  (1u << 0)
#define TIM_CR1_ARPE (1u << 7)

#define TIM_EGR_UG (1u << 0)

#define TIM_CCER_CC1E  (1u << 0)
#define TIM_CCER_CC1P  (1u << 1)
#define TIM_CCER_CC1NE (1u << 2)
#define TIM_CCER_CC1NP (1u << 3)
#define TIM_CCER_CC2E  (1u << 4)
#define TIM_CCER_CC2P  (1u << 5)
#define TIM_CCER_CC2NE (1u << 6)
#define TIM_CCER_CC2NP (1u << 7)
#define TIM_CCER_CC3E  (1u << 8)
#define TIM_CCER_CC3P  (1u << 9)
#define TIM_CCER_CC3NE (1u << 10)
#define TIM_CCER_CC3NP (1u << 11)

#define TIM_BDTR_MOE (1u << 15)

#define GPIO_MODE_OUTPUT_50MHZ 0x3u
#define GPIO_CNF_AF_PP         0x2u

#if (PWM_HI_ACTIVE_LEVEL == 1u)
#define PWM_HI_POLARITY_BITS 0u
#else
#define PWM_HI_POLARITY_BITS (TIM_CCER_CC1P | TIM_CCER_CC2P | TIM_CCER_CC3P)
#endif

#if (PWM_LO_ACTIVE_LEVEL == 0u)
#define PWM_LO_POLARITY_BITS (TIM_CCER_CC1NP | TIM_CCER_CC2NP | TIM_CCER_CC3NP)
#else
#define PWM_LO_POLARITY_BITS 0u
#endif

/************************************
 * PRIVATE TYPEDEFS
 ************************************/

/************************************
 * STATIC VARIABLES
 ************************************/
static volatile u2 u2sv_pwm_duty_u;
static volatile u2 u2sv_pwm_duty_v;
static volatile u2 u2sv_pwm_duty_w;

/************************************
 * GLOBAL VARIABLES
 ************************************/

/************************************
 * FUNCTION PROTOTYPES
 ************************************/
static u1 pwm_encode_deadtime_us(u4 u4t_deadtime_us);
static u2 pwm_limit_duty(u2 u2t_duty);
static u2 pwm_duty_to_ccr(u2 u2t_duty);

/************************************
 * FUNCTIONS
 ************************************/
/**
 * @fn     pwm_encode_deadtime_us
 * @id     PWM-001
 * @brief  Convert deadtime in microseconds to the TIM1 deadtime register format
 * @param  u4t_deadtime_us: Required deadtime in microseconds (u4)
 * @return TIM1 deadtime register value
 */
static u1 pwm_encode_deadtime_us(u4 u4t_deadtime_us)
{
    u4 u4t_ticks;

    u4t_ticks = (PWM_TIMER_CLOCK_HZ / 1000000u) * u4t_deadtime_us;

    if (u4t_ticks <= 127u)
    {
        return (u1)u4t_ticks;
    }

    if (u4t_ticks <= 254u)
    {
        return (u1)(0x80u | ((u4t_ticks / 2u) - 64u));
    }

    if (u4t_ticks <= 504u)
    {
        return (u1)(0xC0u | ((u4t_ticks / 8u) - 32u));
    }

    if (u4t_ticks > 1008u)
    {
        u4t_ticks = 1008u;
    }

    return (u1)(0xE0u | ((u4t_ticks / 16u) - 32u));
}

/**
 * @fn     pwm_limit_duty
 * @id     PWM-002
 * @brief  Limit a PWM duty command to the configured maximum
 * @param  u2t_duty: Requested PWM duty command (u2)
 * @return Limited PWM duty command
 */
static u2 pwm_limit_duty(u2 u2t_duty)
{
    if (u2t_duty > PWM_DUTY_MAX)
    {
        return PWM_DUTY_MAX;
    }

    return u2t_duty;
}

/**
 * @fn     pwm_duty_to_ccr
 * @id     PWM-003
 * @brief  Convert a PWM duty command to a TIM1 compare register value
 * @param  u2t_duty: PWM duty command from 0 to PWM_DUTY_MAX (u2)
 * @return TIM1 capture compare register value
 */
static u2 pwm_duty_to_ccr(u2 u2t_duty)
{
    u4 u4t_period_counts;
    u4 u4t_ccr;

    u4t_period_counts = TIM1_ARR + 1u;
    u4t_ccr = ((u4)u2t_duty * u4t_period_counts) / PWM_DUTY_MAX;
    if (u4t_ccr > TIM1_ARR)
    {
        u4t_ccr = TIM1_ARR;
    }

    return (u2)u4t_ccr;
}

/**
 * @fn     eng_pwm_set_duty
 * @id     PWM-004
 * @brief  Set PWM duty commands for U, V, and W phases
 * @param  u2t_duty_u: U-phase PWM duty command (u2)
 * @param  u2t_duty_v: V-phase PWM duty command (u2)
 * @param  u2t_duty_w: W-phase PWM duty command (u2)
 * @return None
 */
void eng_pwm_set_duty(u2 u2t_duty_u, u2 u2t_duty_v, u2 u2t_duty_w)
{
    u2sv_pwm_duty_u = pwm_limit_duty(u2t_duty_u);
    u2sv_pwm_duty_v = pwm_limit_duty(u2t_duty_v);
    u2sv_pwm_duty_w = pwm_limit_duty(u2t_duty_w);
}

/**
 * @fn     vdg_pwm_on_100_us
 * @id     PWM-005
 * @brief  Update TIM1 compare registers from the phase duty commands
 * @return None
 */
void vdg_pwm_on_100_us(void)
{
    TIM1_CCR1 = pwm_duty_to_ccr(u2sv_pwm_duty_u);
    TIM1_CCR2 = pwm_duty_to_ccr(u2sv_pwm_duty_v);
    TIM1_CCR3 = pwm_duty_to_ccr(u2sv_pwm_duty_w);
}

/**
 * @fn     PWM_EnableOutput
 * @id     PWM-006
 * @brief  Enable TIM1 main and complementary PWM outputs
 * @return None
 */
void PWM_EnableOutput(void)
{
    TIM1_BDTR |= TIM_BDTR_MOE;
}

/**
 * @fn     PWM_DisableOutput
 * @id     PWM-007
 * @brief  Disable TIM1 main and complementary PWM outputs
 * @return None
 */
void PWM_DisableOutput(void)
{
    TIM1_BDTR &= ~TIM_BDTR_MOE;
}

/**
 * @fn     PWM_Init
 * @id     PWM-008
 * @brief  Initialize TIM1 and PWM GPIOs for three phase complementary output
 * @return None
 */
void PWM_Init(void)
{
    u4 u4t_pin_cfg;
    u4 u4t_period_counts;
    u4 u4t_bdtr;

    RCC_APB2ENR |= (RCC_APB2ENR_AFIOEN | RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPBEN | RCC_APB2ENR_TIM1EN);

    /* PA8, PA9, PA10 -> TIM1_CH1/2/3 (high side). */
    u4t_pin_cfg = (GPIO_MODE_OUTPUT_50MHZ | (GPIO_CNF_AF_PP << 2));
    GPIOA_CRH &= ~((0xFu << 0) | (0xFu << 4) | (0xFu << 8));
    GPIOA_CRH |= (u4t_pin_cfg << 0) | (u4t_pin_cfg << 4) | (u4t_pin_cfg << 8);

    /* PB13, PB14, PB15 -> TIM1_CH1N/2N/3N (low side). */
    GPIOB_CRH &= ~((0xFu << 20) | (0xFu << 24) | (0xFu << 28));
    GPIOB_CRH |= (u4t_pin_cfg << 20) | (u4t_pin_cfg << 24) | (u4t_pin_cfg << 28);

    TIM1_CR1 = 0u;

    TIM1_PSC = 0u;
    u4t_period_counts = (PWM_TIMER_CLOCK_HZ / 1000000u) * PWM_PERIOD_US;
    TIM1_ARR = u4t_period_counts - 1u;

    /* PWM mode 1 with preload on CH1, CH2, CH3. */
    TIM1_CCMR1 = (6u << 4) | (1u << 3) | (6u << 12) | (1u << 11);
    TIM1_CCMR2 = (6u << 4) | (1u << 3);

    TIM1_CCR1 = 0u;
    TIM1_CCR2 = 0u;
    TIM1_CCR3 = 0u;

    TIM1_CCER = TIM_CCER_CC1E | TIM_CCER_CC1NE |
                TIM_CCER_CC2E | TIM_CCER_CC2NE |
                TIM_CCER_CC3E | TIM_CCER_CC3NE |
                PWM_HI_POLARITY_BITS |
                PWM_LO_POLARITY_BITS;

    u4t_bdtr = TIM_BDTR_MOE | (u4)pwm_encode_deadtime_us(PWM_DEADTIME_US);
    TIM1_BDTR = u4t_bdtr;
    TIM1_CR1 |= TIM_CR1_ARPE;
    TIM1_EGR = TIM_EGR_UG;

    u2sv_pwm_duty_u = 0u;
    u2sv_pwm_duty_v = 0u;
    u2sv_pwm_duty_w = 0u;

    TIM1_CR1 |= TIM_CR1_CEN;
}
