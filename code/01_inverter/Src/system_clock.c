/**
 ********************************************************************************
 * @file    system_clock.c
 * @brief   STM32F103 clock: HSE*9=72 MHz, else HSI/2*16=64 MHz.
 *
 * Blue Pill / inverter boards often have no working HSE. Staying on bare
 * 8 MHz HSI while timers are programmed for 72 MHz makes 100 us ticks ~900 us.
 ********************************************************************************
 */

#include "system_clock.h"

#define RCC_BASE   0x40021000u
#define FLASH_BASE 0x40022000u
#define REG32(a) (*(volatile uint32_t *)(a))

#define RCC_CR      REG32(RCC_BASE + 0x00u)
#define RCC_CFGR    REG32(RCC_BASE + 0x04u)
#define FLASH_ACR   REG32(FLASH_BASE + 0x00u)

#define RCC_CR_HSION     (1u << 0)
#define RCC_CR_HSIRDY    (1u << 1)
#define RCC_CR_HSEON     (1u << 16)
#define RCC_CR_HSERDY    (1u << 17)
#define RCC_CR_PLLON     (1u << 24)
#define RCC_CR_PLLRDY    (1u << 25)

#define RCC_CFGR_SW_MASK      (3u << 0)
#define RCC_CFGR_SW_PLL       (2u << 0)
#define RCC_CFGR_SWS_MASK     (3u << 2)
#define RCC_CFGR_SWS_PLL      (2u << 2)
#define RCC_CFGR_HPRE_DIV1    (0u << 4)
#define RCC_CFGR_PPRE1_DIV2   (4u << 8)  /* APB1 max 36 MHz */
#define RCC_CFGR_PPRE2_DIV1   (0u << 11)
#define RCC_CFGR_ADCPRE_DIV6  (2u << 14) /* SYSCLK/6, ADC <= 14 MHz */
#define RCC_CFGR_PLLSRC_HSE   (1u << 16)
#define RCC_CFGR_PLLMUL_X9    (7u << 18) /* 0111: x9  (HSE 8 -> 72) */
#define RCC_CFGR_PLLMUL_X16   (0xEu << 18) /* 1110: x16 (HSI/2 4 -> 64) */

#define FLASH_ACR_LATENCY_MASK (7u << 0)
#define FLASH_ACR_LATENCY_2WS  (2u << 0)
#define FLASH_ACR_PRFTBE       (1u << 4)

#define CLOCK_WAIT_TIMEOUT 200000u

static volatile uint32_t s_system_core_clock_hz = SYSTEM_CLOCK_HSI_HZ;
static volatile uint32_t s_system_timer_clock_hz = SYSTEM_CLOCK_HSI_HZ;

static void system_clock_pll_off(void)
{
    uint32_t wait = CLOCK_WAIT_TIMEOUT;

    RCC_CR &= ~RCC_CR_PLLON;
    while (((RCC_CR & RCC_CR_PLLRDY) != 0u) && (wait > 0u))
    {
        wait--;
    }
}

static uint32_t system_clock_wait_flag(uint32_t mask)
{
    uint32_t wait = CLOCK_WAIT_TIMEOUT;

    while (((RCC_CR & mask) == 0u) && (wait > 0u))
    {
        wait--;
    }

    return wait;
}

static uint32_t system_clock_switch_pll(uint32_t core_hz)
{
    uint32_t wait;

    RCC_CR |= RCC_CR_PLLON;
    if (system_clock_wait_flag(RCC_CR_PLLRDY) == 0u)
    {
        return 0u;
    }

    RCC_CFGR = (RCC_CFGR & ~RCC_CFGR_SW_MASK) | RCC_CFGR_SW_PLL;
    wait = CLOCK_WAIT_TIMEOUT;
    while (((RCC_CFGR & RCC_CFGR_SWS_MASK) != RCC_CFGR_SWS_PLL) && (wait > 0u))
    {
        wait--;
    }
    if (wait == 0u)
    {
        return 0u;
    }

    /* APB1 is /2, so TIM2 kernel clock = 2 * PCLK1 = SYSCLK. TIM1 APB2 = SYSCLK. */
    s_system_core_clock_hz = core_hz;
    s_system_timer_clock_hz = core_hz;
    return 1u;
}

static uint32_t system_clock_try_hse_72(void)
{
    uint32_t cfgr;

    RCC_CR |= RCC_CR_HSEON;
    if (system_clock_wait_flag(RCC_CR_HSERDY) == 0u)
    {
        RCC_CR &= ~RCC_CR_HSEON;
        return 0u;
    }

    system_clock_pll_off();

    cfgr = RCC_CFGR;
    cfgr &= ~((0xFu << 18) | (1u << 17) | (1u << 16) |
              (3u << 14) | (7u << 11) | (7u << 8) | (0xFu << 4) | RCC_CFGR_SW_MASK);
    cfgr |= RCC_CFGR_PLLMUL_X9 | RCC_CFGR_PLLSRC_HSE |
            RCC_CFGR_ADCPRE_DIV6 | RCC_CFGR_PPRE2_DIV1 | RCC_CFGR_PPRE1_DIV2 |
            RCC_CFGR_HPRE_DIV1;
    RCC_CFGR = cfgr;

    if (system_clock_switch_pll(SYSTEM_CLOCK_HSE_PLL_HZ) == 0u)
    {
        system_clock_pll_off();
        RCC_CR &= ~RCC_CR_HSEON;
        return 0u;
    }

    return 1u;
}

static uint32_t system_clock_try_hsi_64(void)
{
    uint32_t cfgr;

    /* PLLSRC=0 selects HSI/2. Must be off before changing PLL bits. */
    system_clock_pll_off();
    RCC_CR &= ~RCC_CR_HSEON;

    cfgr = RCC_CFGR;
    cfgr &= ~((0xFu << 18) | (1u << 17) | (1u << 16) |
              (3u << 14) | (7u << 11) | (7u << 8) | (0xFu << 4) | RCC_CFGR_SW_MASK);
    cfgr |= RCC_CFGR_PLLMUL_X16 |
            RCC_CFGR_ADCPRE_DIV6 | RCC_CFGR_PPRE2_DIV1 | RCC_CFGR_PPRE1_DIV2 |
            RCC_CFGR_HPRE_DIV1;
    RCC_CFGR = cfgr;

    return system_clock_switch_pll(SYSTEM_CLOCK_HSI_PLL_HZ);
}

void SystemInit(void)
{
    uint32_t wait;

    RCC_CR |= RCC_CR_HSION;
    wait = CLOCK_WAIT_TIMEOUT;
    while (((RCC_CR & RCC_CR_HSIRDY) == 0u) && (wait > 0u))
    {
        wait--;
    }

    /* 2 wait states covers both 64 and 72 MHz. */
    FLASH_ACR = (FLASH_ACR & ~FLASH_ACR_LATENCY_MASK) | FLASH_ACR_LATENCY_2WS | FLASH_ACR_PRFTBE;

    /* This board has no HSE. Go straight to HSI/2 * 16 = 64 MHz. */
    if (system_clock_try_hsi_64() != 0u)
    {
        return;
    }

    s_system_core_clock_hz = SYSTEM_CLOCK_HSI_HZ;
    s_system_timer_clock_hz = SYSTEM_CLOCK_HSI_HZ;
}

/*
 * Do not trust RAM copies of the clock. SystemInit runs before .data/.bss
 * init, so those copies are wiped back to 8 MHz even after PLL is running.
 * Decode RCC->CFGR instead.
 */
static uint32_t system_clock_from_rcc(void)
{
    uint32_t sws;
    uint32_t pllmul;
    uint32_t hz;

    sws = RCC_CFGR & RCC_CFGR_SWS_MASK;
    if (sws != RCC_CFGR_SWS_PLL)
    {
        return SYSTEM_CLOCK_HSI_HZ;
    }

    pllmul = (RCC_CFGR >> 18) & 0xFu;
    if (pllmul == 0xFu)
    {
        pllmul = 16u;
    }
    else
    {
        pllmul += 2u;
    }

    if ((RCC_CFGR & RCC_CFGR_PLLSRC_HSE) != 0u)
    {
        hz = 8000000u * pllmul;
    }
    else
    {
        hz = 4000000u * pllmul; /* HSI/2 */
    }

    return hz;
}

uint32_t SystemCoreClock_Get(void)
{
    return system_clock_from_rcc();
}

uint32_t SystemTimerClock_Get(void)
{
    /* APB prescalers are /1 on APB2 and /2 on APB1, so both timer kernels = SYSCLK. */
    return system_clock_from_rcc();
}
