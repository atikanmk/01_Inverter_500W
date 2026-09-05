#include "adc.h"

#define RCC_BASE_ADDR    0x40021000u
#define GPIOA_BASE_ADDR  0x40010800u
#define GPIOB_BASE_ADDR  0x40010C00u
#define ADC1_BASE_ADDR   0x40012400u
#define ADC2_BASE_ADDR   0x40012800u
#define DMA1_BASE_ADDR   0x40020000u
#define NVIC_ISER0_ADDR  0xE000E100u

#define REG32(addr) (*(volatile u4 *)(addr))

#define RCC_APB2ENR REG32(RCC_BASE_ADDR + 0x18u)
#define NVIC_ISER0   REG32(NVIC_ISER0_ADDR)

#define GPIOA_CRL REG32(GPIOA_BASE_ADDR + 0x00u)
#define GPIOA_CRH REG32(GPIOA_BASE_ADDR + 0x04u)
#define GPIOB_CRL REG32(GPIOB_BASE_ADDR + 0x00u)

#define RCC_APB2ENR_IOPAEN (1u << 2)
#define RCC_APB2ENR_IOPBEN (1u << 3)
#define RCC_APB2ENR_ADC1EN (1u << 9)
#define RCC_APB2ENR_ADC2EN (1u << 10)

#define ADC_SR_OFFSET     0x00u
#define ADC_CR1_OFFSET    0x04u
#define ADC_CR2_OFFSET    0x08u
#define ADC_SMPR1_OFFSET  0x0Cu
#define ADC_SMPR2_OFFSET  0x10u
#define ADC_SQR1_OFFSET   0x2Cu
#define ADC_SQR3_OFFSET   0x34u
#define ADC_DR_OFFSET     0x4Cu

#define ADC_SR_EOC      (1u << 1)
#define ADC_CR1_SCAN    (1u << 8)
#define ADC_CR1_EOCIE   (1u << 5)
#define ADC_CR2_ADON    (1u << 0)
#define ADC_CR2_CAL     (1u << 2)
#define ADC_CR2_RSTCAL  (1u << 3)
#define ADC_CR2_DMA     (1u << 8)
#define ADC_CR2_EXTTRIG (1u << 20)
#define ADC_CR2_SWSTART (1u << 22)
#define ADC_CR2_EXTSEL_SWSTART (0x7u << 17)
#define ADC_CR2_EXTSEL_MASK    (0x7u << 17)

#define DMA1_ISR   REG32(DMA1_BASE_ADDR + 0x00u)
#define DMA1_IFCR  REG32(DMA1_BASE_ADDR + 0x04u)
#define DMA1_CCR1  REG32(DMA1_BASE_ADDR + 0x08u)
#define DMA1_CNDTR1 REG32(DMA1_BASE_ADDR + 0x0Cu)
#define DMA1_CPAR1 REG32(DMA1_BASE_ADDR + 0x10u)
#define DMA1_CMAR1 REG32(DMA1_BASE_ADDR + 0x14u)

#define DMA_ISR_TCIF1   (1u << 1)
#define DMA_ISR_TEIF1   (1u << 3)
#define DMA_IFCR_CGIF1  (1u << 0)
#define DMA_IFCR_CTCIF1 (1u << 1)
#define DMA_IFCR_CHTIF1 (1u << 2)
#define DMA_IFCR_CTEIF1 (1u << 3)

#define DMA_CCR_EN      (1u << 0)
#define DMA_CCR_TCIE    (1u << 1)
#define DMA_CCR_MINC    (1u << 7)
#define DMA_CCR_PSIZE_16 (1u << 8)
#define DMA_CCR_MSIZE_16 (1u << 10)

#define ADC_SMP_55CYCLES 0x5u
#define ADC_WAIT_TIMEOUT 100000u
#define DMA1_CH1_IRQ_BIT (1u << 11)

static volatile u2 g_adc_raw_work[EN_CONFIG_ADC_COUNT];
static volatile u2 g_adc_raw_latest[EN_CONFIG_ADC_COUNT];
static volatile u1 g_adc_seq_channel[EN_CONFIG_ADC_COUNT];
static volatile u1 g_adc_active_group;
static volatile u1 g_adc_busy;
static volatile u1 g_adc_frame_ready;
static volatile u1 g_adc_valid;
static volatile u1 g_adc_sample_index;

static u4 adc_base_from_group(u1 adc_group)
{
    if (adc_group == 1u)
    {
        return ADC2_BASE_ADDR;
    }

    return ADC1_BASE_ADDR;
}

static void adc_set_channel_sample_time(u4 adc_base, u1 channel, u4 sample_code)
{
    u4 shift;
    u4 reg;

    if (channel <= 9u)
    {
        shift = (u4)channel * 3u;
        reg = REG32(adc_base + ADC_SMPR2_OFFSET);
        reg &= ~(0x7u << shift);
        reg |= (sample_code << shift);
        REG32(adc_base + ADC_SMPR2_OFFSET) = reg;
    }
    else if (channel <= 17u)
    {
        shift = (u4)(channel - 10u) * 3u;
        reg = REG32(adc_base + ADC_SMPR1_OFFSET);
        reg &= ~(0x7u << shift);
        reg |= (sample_code << shift);
        REG32(adc_base + ADC_SMPR1_OFFSET) = reg;
    }
}

static void adc_config_gpio_analog_from_channel(u1 channel)
{
    u4 shift;

    if (channel <= 7u)
    {
        shift = (u4)channel * 4u;
        GPIOA_CRL &= ~(0xFu << shift);
    }
    else if (channel <= 9u)
    {
        shift = (u4)(channel - 8u) * 4u;
        GPIOB_CRL &= ~(0xFu << shift);
    }
    else if ((channel >= 10u) && (channel <= 15u))
    {
        shift = (u4)(channel - 8u) * 4u;
        GPIOA_CRH &= ~(0xFu << shift);
    }
}

static void adc_calibrate(u4 adc_base)
{
    u4 wait;

    REG32(adc_base + ADC_CR2_OFFSET) |= ADC_CR2_ADON;

    REG32(adc_base + ADC_CR2_OFFSET) |= ADC_CR2_RSTCAL;
    wait = ADC_WAIT_TIMEOUT;
    while (((REG32(adc_base + ADC_CR2_OFFSET) & ADC_CR2_RSTCAL) != 0u) && (wait > 0u))
    {
        wait--;
    }

    REG32(adc_base + ADC_CR2_OFFSET) |= ADC_CR2_CAL;
    wait = ADC_WAIT_TIMEOUT;
    while (((REG32(adc_base + ADC_CR2_OFFSET) & ADC_CR2_CAL) != 0u) && (wait > 0u))
    {
        wait--;
    }
}

static void adc_config_sequence(u4 adc_base, const ST_INVERTER_CONFIG *cfg)
{
    u4 sqr3;
    u1 i;

    sqr3 = 0u;
    for (i = 0u; i < (u1)EN_CONFIG_ADC_COUNT; ++i)
    {
        g_adc_seq_channel[i] = cfg->stt_adc[i].u1t_adc_pin;
        sqr3 |= ((u4)g_adc_seq_channel[i] & 0x1Fu) << (5u * i);
    }

    REG32(adc_base + ADC_SQR1_OFFSET) &= ~(0xFu << 20);
    REG32(adc_base + ADC_SQR1_OFFSET) |= ((u4)EN_CONFIG_ADC_COUNT - 1u) << 20;
    REG32(adc_base + ADC_SQR3_OFFSET) = sqr3;
}

static void adc_copy_latest_frame(void)
{
    u1 i;

    for (i = 0u; i < (u1)EN_CONFIG_ADC_COUNT; ++i)
    {
        g_adc_raw_latest[i] = g_adc_raw_work[i];
    }
}

static EN_COM_STS_T adc_read_single_channel(u4 adc_base, u1 channel, u2 *value)
{
    u4 wait;

    REG32(adc_base + ADC_SQR1_OFFSET) &= ~(0xFu << 20);
    REG32(adc_base + ADC_SQR3_OFFSET) = (u4)channel & 0x1Fu;

    REG32(adc_base + ADC_SR_OFFSET) &= ~ADC_SR_EOC;
    REG32(adc_base + ADC_CR2_OFFSET) |= ADC_CR2_ADON;
    REG32(adc_base + ADC_CR2_OFFSET) |= ADC_CR2_SWSTART;

    wait = ADC_WAIT_TIMEOUT;
    while (((REG32(adc_base + ADC_SR_OFFSET) & ADC_SR_EOC) == 0u) && (wait > 0u))
    {
        wait--;
    }

    if (wait == 0u)
    {
        return EN_COM_STS_ERR;
    }

    REG32(adc_base + ADC_SR_OFFSET) &= ~ADC_SR_EOC;
    *value = (u2)(REG32(adc_base + ADC_DR_OFFSET) & 0x0FFFu);
    return EN_COM_STS_OK;
}

void ADC_Init(void)
{
    const ST_INVERTER_CONFIG *cfg;
    u1 i;
    u1 channel;
    u1 group;
    u4 adc_base;

    RCC_APB2ENR |= (RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPBEN | RCC_APB2ENR_ADC1EN | RCC_APB2ENR_ADC2EN);

    cfg = Config_Get();
    g_adc_active_group = cfg->stt_adc[0].u1t_adc_group;
    g_adc_busy = 0u;
    g_adc_frame_ready = 0u;
    g_adc_valid = 1u;
    g_adc_sample_index = 0u;

    for (i = 0u; i < (u1)EN_CONFIG_ADC_COUNT; ++i)
    {
        channel = cfg->stt_adc[i].u1t_adc_pin;
        group = cfg->stt_adc[i].u1t_adc_group;

        if (group != g_adc_active_group)
        {
            g_adc_valid = 0u;
        }

        adc_config_gpio_analog_from_channel(channel);
        adc_set_channel_sample_time(adc_base_from_group(group), channel, ADC_SMP_55CYCLES);
        g_adc_raw_work[i] = 0u;
        g_adc_raw_latest[i] = 0u;
    }

    if (g_adc_active_group != 0u)
    {
        g_adc_valid = 0u;
    }

    if (g_adc_valid == 0u)
    {
        return;
    }

    adc_base = adc_base_from_group(g_adc_active_group);
    adc_config_sequence(adc_base, cfg);

    REG32(adc_base + ADC_CR1_OFFSET) = ADC_CR1_SCAN;
    REG32(adc_base + ADC_CR2_OFFSET) &= ~ADC_CR2_EXTSEL_MASK;
    REG32(adc_base + ADC_CR2_OFFSET) |= ADC_CR2_EXTSEL_SWSTART;
    REG32(adc_base + ADC_CR2_OFFSET) &= ~ADC_CR2_DMA;
    REG32(adc_base + ADC_CR2_OFFSET) |= ADC_CR2_EXTTRIG;

    adc_calibrate(adc_base);

}

EN_COM_STS_T ADC_TriggerFromPwmCycle(void)
{
    u4 adc_base;
    u1 i;

    if (g_adc_valid == 0u)
    {
        return EN_COM_STS_ERR;
    }

    if (g_adc_busy != 0u)
    {
        return EN_COM_STS_RUNNING;
    }

    adc_base = adc_base_from_group(g_adc_active_group);

    g_adc_busy = 1u;
    g_adc_frame_ready = 0u;
    g_adc_sample_index = 0u;

    for (i = 0u; i < (u1)EN_CONFIG_ADC_COUNT; ++i)
    {
        {
            u2 sample;

            if (adc_read_single_channel(adc_base, g_adc_seq_channel[i], &sample) != EN_COM_STS_OK)
            {
                g_adc_busy = 0u;
                return EN_COM_STS_ERR;
            }

            g_adc_raw_work[i] = sample;
        }
    }

    adc_copy_latest_frame();
    g_adc_busy = 0u;
    g_adc_frame_ready = 1u;

    return EN_COM_STS_OK;
}

EN_COM_STS_T ADC_ReadAll12bit(u2 raw_array[EN_CONFIG_ADC_COUNT])
{
    u1 i;

    if (raw_array == 0)
    {
        return EN_COM_STS_ERR;
    }

    if (g_adc_valid == 0u)
    {
        return EN_COM_STS_ERR;
    }

    if (g_adc_frame_ready == 0u)
    {
        return EN_COM_STS_RUNNING;
    }

    for (i = 0u; i < (u1)EN_CONFIG_ADC_COUNT; ++i)
    {
        raw_array[i] = g_adc_raw_latest[i];
    }

    g_adc_frame_ready = 0u;
    return EN_COM_STS_OK;
}

EN_COM_STS_T ADC_GetValue(EN_CONFIG_ADC_T adc_id, u2 *value)
{
    if (value == 0)
    {
        return EN_COM_STS_ERR;
    }

    if (adc_id >= EN_CONFIG_ADC_COUNT)
    {
        return EN_COM_STS_ERR;
    }

    if (g_adc_valid == 0u)
    {
        return EN_COM_STS_ERR;
    }

    if (g_adc_frame_ready == 0u)
    {
        *value = g_adc_raw_latest[adc_id];
        return EN_COM_STS_RUNNING;
    }

    *value = g_adc_raw_latest[adc_id];
    return EN_COM_STS_OK;
}

void ADC1_2_IRQHandler(void)
{
    u4 adc_base;

    if ((g_adc_valid == 0u) || (g_adc_busy == 0u))
    {
        return;
    }

    adc_base = adc_base_from_group(g_adc_active_group);

    if ((REG32(adc_base + ADC_SR_OFFSET) & ADC_SR_EOC) == 0u)
    {
        return;
    }

    REG32(adc_base + ADC_SR_OFFSET) &= ~ADC_SR_EOC;
    g_adc_raw_work[g_adc_sample_index] = (u2)(REG32(adc_base + ADC_DR_OFFSET) & 0x0FFFu);

    if (g_adc_sample_index + 1u < (u1)EN_CONFIG_ADC_COUNT)
    {
        g_adc_sample_index++;
        return;
    }

    adc_copy_latest_frame();
    g_adc_busy = 0u;
    g_adc_frame_ready = 1u;
}
