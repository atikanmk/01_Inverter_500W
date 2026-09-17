#include "pwm.h"

#define RCC_BASE_ADDR   0x40021000u
#define GPIOA_BASE_ADDR 0x40010800u
#define GPIOB_BASE_ADDR 0x40010C00u
#define TIM1_BASE_ADDR  0x40012C00u

#define REG32(addr) (*(volatile uint32_t *)(addr))

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

static volatile uint16_t g_pwm_duty_u;
static volatile uint16_t g_pwm_duty_v;
static volatile uint16_t g_pwm_duty_w;

static uint8_t pwm_encode_deadtime_us(uint32_t deadtime_us)
{
    uint32_t ticks;

    ticks = (PWM_TIMER_CLOCK_HZ / 1000000u) * deadtime_us;

    if (ticks <= 127u)
    {
        return (uint8_t)ticks;
    }

    if (ticks <= 254u)
    {
        return (uint8_t)(0x80u | ((ticks / 2u) - 64u));
    }

    if (ticks <= 504u)
    {
        return (uint8_t)(0xC0u | ((ticks / 8u) - 32u));
    }

    if (ticks > 1008u)
    {
        ticks = 1008u;
    }

    return (uint8_t)(0xE0u | ((ticks / 16u) - 32u));
}

static uint16_t pwm_limit_duty(uint16_t duty)
{
    if (duty > PWM_DUTY_MAX)
    {
        return PWM_DUTY_MAX;
    }
    return duty;
}

static uint16_t pwm_duty_to_ccr(uint16_t duty)
{
    uint32_t period_counts;
    uint32_t ccr;

    period_counts = TIM1_ARR + 1u;
    ccr = ((uint32_t)duty * period_counts) / PWM_DUTY_MAX;
    if (ccr > TIM1_ARR)
    {
        ccr = TIM1_ARR;
    }

    return (uint16_t)ccr;
}

void PWM_SetDutyUVW(uint16_t duty_u, uint16_t duty_v, uint16_t duty_w)
{
    g_pwm_duty_u = pwm_limit_duty(duty_u);
    g_pwm_duty_v = pwm_limit_duty(duty_v);
    g_pwm_duty_w = pwm_limit_duty(duty_w);
}

void PWM_On100us(void)
{
    TIM1_CCR1 = pwm_duty_to_ccr(g_pwm_duty_u);
    TIM1_CCR2 = pwm_duty_to_ccr(g_pwm_duty_v);
    TIM1_CCR3 = pwm_duty_to_ccr(g_pwm_duty_w);
}

void PWM_EnableOutput(void)
{
    TIM1_BDTR |= TIM_BDTR_MOE;
}

void PWM_DisableOutput(void)
{
    TIM1_BDTR &= ~TIM_BDTR_MOE;
}

void PWM_Init(void)
{
    uint32_t pin_cfg;
    uint32_t period_counts;
    uint32_t bdtr;

    RCC_APB2ENR |= (RCC_APB2ENR_AFIOEN | RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPBEN | RCC_APB2ENR_TIM1EN);

    /* PA8, PA9, PA10 -> TIM1_CH1/2/3 (high side). */
    pin_cfg = (GPIO_MODE_OUTPUT_50MHZ | (GPIO_CNF_AF_PP << 2));
    GPIOA_CRH &= ~((0xFu << 0) | (0xFu << 4) | (0xFu << 8));
    GPIOA_CRH |= (pin_cfg << 0) | (pin_cfg << 4) | (pin_cfg << 8);

    /* PB13, PB14, PB15 -> TIM1_CH1N/2N/3N (low side). */
    GPIOB_CRH &= ~((0xFu << 20) | (0xFu << 24) | (0xFu << 28));
    GPIOB_CRH |= (pin_cfg << 20) | (pin_cfg << 24) | (pin_cfg << 28);

    TIM1_CR1 = 0u;

    TIM1_PSC = 0u;
    period_counts = (PWM_TIMER_CLOCK_HZ / 1000000u) * PWM_PERIOD_US;
    TIM1_ARR = period_counts - 1u;

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

    bdtr = TIM_BDTR_MOE | (uint32_t)pwm_encode_deadtime_us(PWM_DEADTIME_US);
    TIM1_BDTR = bdtr;
    TIM1_CR1 |= TIM_CR1_ARPE;
    TIM1_EGR = TIM_EGR_UG;

    g_pwm_duty_u = 0u;
    g_pwm_duty_v = 0u;
    g_pwm_duty_w = 0u;

    TIM1_CR1 |= TIM_CR1_CEN;
}
