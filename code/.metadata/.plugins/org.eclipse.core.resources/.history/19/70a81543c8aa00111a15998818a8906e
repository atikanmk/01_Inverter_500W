#include "hall.h"

#include "config.h"

#include <stdint.h>

#define RCC_BASE_ADDR   0x40021000u
#define AFIO_BASE_ADDR  0x40010000u
#define EXTI_BASE_ADDR  0x40010400u
#define GPIOA_BASE_ADDR 0x40010800u
#define NVIC_ISER0_ADDR 0xE000E100u
#define SCB_DEMCR_ADDR  0xE000EDFCu
#define DWT_CTRL_ADDR   0xE0001000u
#define DWT_CYCCNT_ADDR 0xE0001004u

#define REG32(addr) (*(volatile uint32_t *)(addr))

#define RCC_APB2ENR REG32(RCC_BASE_ADDR + 0x18u)

#define GPIOA_CRL REG32(GPIOA_BASE_ADDR + 0x00u)
#define GPIOA_IDR REG32(GPIOA_BASE_ADDR + 0x08u)

#define AFIO_EXTICR1 REG32(AFIO_BASE_ADDR + 0x08u)

#define EXTI_IMR  REG32(EXTI_BASE_ADDR + 0x00u)
#define EXTI_RTSR REG32(EXTI_BASE_ADDR + 0x08u)
#define EXTI_FTSR REG32(EXTI_BASE_ADDR + 0x0Cu)
#define EXTI_PR   REG32(EXTI_BASE_ADDR + 0x14u)

#define NVIC_ISER0 REG32(NVIC_ISER0_ADDR)
#define SCB_DEMCR  REG32(SCB_DEMCR_ADDR)
#define DWT_CTRL   REG32(DWT_CTRL_ADDR)
#define DWT_CYCCNT REG32(DWT_CYCCNT_ADDR)

#define RCC_APB2ENR_AFIOEN (1u << 0)
#define RCC_APB2ENR_IOPAEN (1u << 2)

#define SCB_DEMCR_TRCENA (1u << 24)
#define DWT_CTRL_CYCCNTENA (1u << 0)

#define HALL_EST_SECTOR_DEG 60u
#define HALL_EST_FULL_DEG    360u
#define HALL_EST_HISTORY_LEN 6u
#define HALL_EST_TIMEOUT_CYCLES (HALL_CPU_CLOCK_HZ)

#define HALL_LINE_MASK ((1u << HALL_CH1_PIN) | (1u << HALL_CH2_PIN) | (1u << HALL_CH3_PIN))

static volatile u1 u1g_hall_ch1_state;
static volatile u1 u1g_hall_ch2_state;
static volatile u1 u1g_hall_ch3_state;
static volatile u1 u1g_hall_state;
static volatile u2 u2g_hall_angle_deg;
static volatile u2 u2g_hall_est_curr_ang;
static volatile u2 u2g_hall_est_ang_ref;
static volatile u2 u2g_hall_est_ang_next;

static const u2 u2g_hall_state_angle_deg[8] = {
    0u,   /* 000: invalid */
    120u, /* 001 */
    0u,   /* 010 */
    60u,  /* 011 */
    240u, /* 100 */
    180u, /* 101 */
    300u, /* 110 */
    0u    /* 111: invalid */
};

static volatile uint8_t hall_level[3];
static volatile uint32_t hall_edge_count[3];
static volatile uint32_t hall_last_irq_cycle[3];
static volatile uint32_t hall_est_last_cycle;
static volatile uint32_t hall_est_avg_sector_cycles;
static volatile uint32_t hall_est_sector_hist[HALL_EST_HISTORY_LEN];
static volatile uint32_t hall_est_sector_hist_sum;
static volatile uint8_t hall_est_sector_hist_count;
static volatile uint8_t hall_est_sector_hist_index;
static volatile uint32_t hall_est_sector_hist_start_cycle;
static volatile uint8_t hall_est_has_prev_sector;
static volatile uint8_t hall_est_prev_sector;
static volatile int8_t hall_est_dir;

static volatile s4 s4g_hall_speed_elec_rpm;
static volatile s4 s4g_hall_speed_mech_rpm;

static uint32_t hall_debounce_cycles;

static const uint8_t hall_pin_map[3] = { HALL_CH1_PIN, HALL_CH2_PIN, HALL_CH3_PIN };

static void hall_est_reset_history(void);

static uint16_t hall_norm_angle(uint16_t angle_deg)
{
    if (angle_deg >= HALL_EST_FULL_DEG)
    {
        return 0u;
    }
    return angle_deg;
}

static uint8_t hall_angle_to_sector(uint16_t angle_deg)
{
    return (uint8_t)(hall_norm_angle(angle_deg) / HALL_EST_SECTOR_DEG);
}

static uint16_t hall_sector_to_angle(uint8_t sector)
{
    return (uint16_t)((sector % 6u) * HALL_EST_SECTOR_DEG);
}

static uint8_t hall_est_is_timeout(uint32_t now_cycle)
{
    if (hall_est_last_cycle == 0u)
    {
        return 0u;
    }

    if ((now_cycle - hall_est_last_cycle) > HALL_EST_TIMEOUT_CYCLES)
    {
        return 1u;
    }

    return 0u;
}

static void hall_est_force_reset(uint32_t now_cycle)
{
    uint8_t curr_sector;

    curr_sector = hall_angle_to_sector(u2g_hall_angle_deg);

    hall_est_reset_history();
    hall_est_dir = 0;
    hall_est_has_prev_sector = 0u;
    hall_est_prev_sector = curr_sector;
    hall_est_last_cycle = now_cycle;

    u2g_hall_est_ang_ref = hall_sector_to_angle(curr_sector);
    u2g_hall_est_ang_next = u2g_hall_est_ang_ref;
    u2g_hall_est_curr_ang = u2g_hall_est_ang_ref;
}

static void hall_est_reset_history(void)
{
    uint8_t i;

    hall_est_sector_hist_sum = 0u;
    hall_est_sector_hist_count = 0u;
    hall_est_sector_hist_index = 0u;
    hall_est_sector_hist_start_cycle = 0u;
    hall_est_avg_sector_cycles = 0u;

    for (i = 0u; i < HALL_EST_HISTORY_LEN; ++i)
    {
        hall_est_sector_hist[i] = 0u;
    }
}

static void hall_est_add_sector_sample(uint32_t now_cycle, uint32_t sector_cycles)
{
    if ((hall_est_sector_hist_count > 0u) &&
        ((now_cycle - hall_est_sector_hist_start_cycle) > HALL_EST_TIMEOUT_CYCLES))
    {
        hall_est_reset_history();
    }

    if (hall_est_sector_hist_count == 0u)
    {
        hall_est_sector_hist_start_cycle = now_cycle;
    }

    if (hall_est_sector_hist_count < HALL_EST_HISTORY_LEN)
    {
        hall_est_sector_hist[hall_est_sector_hist_index] = sector_cycles;
        hall_est_sector_hist_sum += sector_cycles;
        hall_est_sector_hist_count++;
    }
    else
    {
        hall_est_sector_hist_sum -= hall_est_sector_hist[hall_est_sector_hist_index];
        hall_est_sector_hist[hall_est_sector_hist_index] = sector_cycles;
        hall_est_sector_hist_sum += sector_cycles;
    }

    hall_est_sector_hist_index++;
    if (hall_est_sector_hist_index >= HALL_EST_HISTORY_LEN)
    {
        hall_est_sector_hist_index = 0u;
    }

    if (hall_est_sector_hist_count == HALL_EST_HISTORY_LEN)
    {
        hall_est_avg_sector_cycles = hall_est_sector_hist_sum / HALL_EST_HISTORY_LEN;
    }
    else
    {
        hall_est_avg_sector_cycles = 0u;
    }
}

static void hall_timebase_init(void)
{
    SCB_DEMCR |= SCB_DEMCR_TRCENA;
    DWT_CYCCNT = 0u;
    DWT_CTRL |= DWT_CTRL_CYCCNTENA;
}

static uint8_t hall_read_pin(uint8_t pin)
{
    return (uint8_t)((GPIOA_IDR >> pin) & 0x1u);
}

static void hall_refresh_all_levels(void)
{
    hall_level[0] = hall_read_pin(HALL_CH1_PIN);
    hall_level[1] = hall_read_pin(HALL_CH2_PIN);
    hall_level[2] = hall_read_pin(HALL_CH3_PIN);

    u1g_hall_ch1_state = hall_level[0];
    u1g_hall_ch2_state = hall_level[1];
    u1g_hall_ch3_state = hall_level[2];

    u1g_hall_state = (u1)((hall_level[2] << 2) | (hall_level[1] << 1) | hall_level[0]);
    u2g_hall_angle_deg = u2g_hall_state_angle_deg[u1g_hall_state];
}

static void hall_est_reset_on_interrupt(void)
{
    uint32_t now_cycle;
    uint32_t sector_cycles;
    uint8_t curr_sector;
    uint8_t sector_step;
    int8_t next_dir;

    now_cycle = DWT_CYCCNT;
    curr_sector = hall_angle_to_sector(u2g_hall_angle_deg);

    if (hall_est_is_timeout(now_cycle) != 0u)
    {
        hall_est_force_reset(now_cycle);
    }

    next_dir = 0;
    if (hall_est_has_prev_sector != 0u)
    {
        sector_step = (uint8_t)((curr_sector + 6u - hall_est_prev_sector) % 6u);
        if (sector_step == 1u)
        {
            next_dir = 1;
        }
        else if (sector_step == 5u)
        {
            next_dir = -1;
        }

        if (next_dir == 0)
        {
            hall_est_dir = 0;
            hall_est_reset_history();
        }
        else
        {
            if (next_dir != hall_est_dir)
            {
                hall_est_dir = next_dir;
                hall_est_reset_history();
            }

            sector_cycles = now_cycle - hall_est_last_cycle;
            hall_est_add_sector_sample(now_cycle, sector_cycles);
        }
    }
    else
    {
        hall_est_dir = 0;
        hall_est_reset_history();
        hall_est_has_prev_sector = 1u;
    }

    hall_est_last_cycle = now_cycle;
    hall_est_prev_sector = curr_sector;
    u2g_hall_est_ang_ref = hall_sector_to_angle(curr_sector);

    if (hall_est_dir > 0)
    {
        u2g_hall_est_ang_next = hall_sector_to_angle((uint8_t)((curr_sector + 1u) % 6u));
    }
    else if (hall_est_dir < 0)
    {
        u2g_hall_est_ang_next = hall_sector_to_angle((uint8_t)((curr_sector + 5u) % 6u));
    }
    else
    {
        u2g_hall_est_ang_next = u2g_hall_est_ang_ref;
    }

    u2g_hall_est_curr_ang = u2g_hall_est_ang_ref;
}

static void hall_handle_exti_line(uint8_t line, uint8_t channel)
{
    uint32_t mask = (1u << line);
    uint32_t now_cycle;
    uint32_t elapsed_cycle;

    if ((EXTI_PR & mask) != 0u)
    {
        EXTI_PR = mask;

        now_cycle = DWT_CYCCNT;
        elapsed_cycle = now_cycle - hall_last_irq_cycle[channel];
        if (elapsed_cycle < hall_debounce_cycles)
        {
            return;
        }

        hall_last_irq_cycle[channel] = now_cycle;
        hall_refresh_all_levels();
        hall_est_reset_on_interrupt();
        hall_edge_count[channel]++;
        Hall_OnEdge(channel, hall_level[channel]);
    }
}

void Hall_Init(void)
{
    uint32_t shift;
    uint8_t i;

    RCC_APB2ENR |= (RCC_APB2ENR_AFIOEN | RCC_APB2ENR_IOPAEN);

    hall_timebase_init();
    hall_debounce_cycles = (HALL_CPU_CLOCK_HZ / 1000000u) * HALL_DEBOUNCE_US;

    /* Configure PA1, PA2, PA3 as floating input (MODE=00, CNF=01). */
    for (i = 0u; i < 3u; ++i)
    {
        shift = hall_pin_map[i] * 4u;
        GPIOA_CRL &= ~(0xFu << shift);
        GPIOA_CRL |= (0x4u << shift);
    }

    /* EXTI1..3 map to Port A (value 0000). */
    AFIO_EXTICR1 &= ~(0xFFF0u);

    EXTI_IMR |= HALL_LINE_MASK;
    EXTI_RTSR |= HALL_LINE_MASK;
    EXTI_FTSR |= HALL_LINE_MASK;
    EXTI_PR = HALL_LINE_MASK;

    for (i = 0u; i < 3u; ++i)
    {
        hall_edge_count[i] = 0u;
        hall_last_irq_cycle[i] = 0u;
    }
    hall_est_last_cycle = 0u;
    hall_est_has_prev_sector = 0u;
    hall_est_prev_sector = 0u;
    hall_est_dir = 0;
    hall_est_reset_history();
    hall_refresh_all_levels();
    hall_est_reset_on_interrupt();

    /* Enable EXTI1, EXTI2, EXTI3 IRQs in NVIC. */
    NVIC_ISER0 = (1u << 7) | (1u << 8) | (1u << 9);
}

uint8_t Hall_GetLevel(uint8_t channel)
{
    if (channel >= 3u)
    {
        return 0u;
    }
    return hall_level[channel];
}

uint8_t Hall_GetPattern(void)
{
    return (uint8_t)((hall_level[0] << 0) | (hall_level[1] << 1) | (hall_level[2] << 2));
}

uint32_t Hall_GetEdgeCount(uint8_t channel)
{
    if (channel >= 3u)
    {
        return 0u;
    }
    return hall_edge_count[channel];
}

EN_COM_STS_T eng_hall_ang_est(u2 *pu2t_angle_deg)
{
    uint32_t now_cycle;
    uint32_t elapsed_cycle;
    uint32_t est_offset;
    uint16_t ref_angle;
    uint16_t next_angle;
    uint16_t estimated;

    now_cycle = DWT_CYCCNT;

    if (hall_est_is_timeout(now_cycle) != 0u)
    {
        hall_est_force_reset(now_cycle);
    }

    if ((hall_est_avg_sector_cycles == 0u) || (hall_est_dir == 0))
    {
        u2g_hall_est_curr_ang = u2g_hall_est_ang_ref;
        *pu2t_angle_deg = (u2)u2g_hall_est_curr_ang;
    }

    elapsed_cycle = now_cycle - hall_est_last_cycle;
    ref_angle = hall_norm_angle(u2g_hall_est_ang_ref);
    next_angle = hall_norm_angle(u2g_hall_est_ang_next);

    if (elapsed_cycle >= hall_est_avg_sector_cycles)
    {
        u2g_hall_est_curr_ang = u2g_hall_est_ang_next;
        *pu2t_angle_deg = (u2)u2g_hall_est_curr_ang;
    }

    est_offset = (elapsed_cycle * HALL_EST_SECTOR_DEG) / hall_est_avg_sector_cycles;
    if (est_offset > HALL_EST_SECTOR_DEG)
    {
        est_offset = HALL_EST_SECTOR_DEG;
    }

    if (hall_est_dir > 0)
    {
        estimated = (u2)(ref_angle + est_offset);
        if (estimated >= HALL_EST_FULL_DEG)
        {
            estimated = (u2)(estimated - HALL_EST_FULL_DEG);
        }

        if (ref_angle <= next_angle)
        {
            if (estimated > next_angle)
            {
                estimated = next_angle;
            }
        }
        else
        {
            if ((estimated > next_angle) && (estimated < ref_angle))
            {
                estimated = (u2)next_angle;
            }
        }

        u2g_hall_est_curr_ang = estimated;
    }
    else
    {
        if (ref_angle >= est_offset)
        {
            estimated = (u2)(ref_angle - est_offset);
        }
        else
        {
            estimated = (u2)(HALL_EST_FULL_DEG + ref_angle - est_offset);
        }

        if (ref_angle >= next_angle)
        {
            if (estimated < next_angle)
            {
                estimated = next_angle;
            }
        }
        else
        {
            if ((estimated < next_angle) && (estimated > ref_angle))
            {
                estimated = next_angle;
            }
        }

        u2g_hall_est_curr_ang = estimated;
    }

    if (hall_est_dir > 0)
    {
        if (ref_angle <= next_angle)
        {
            if (u2g_hall_est_curr_ang > next_angle)
            {
                u2g_hall_est_curr_ang = next_angle;
            }
        }
        else
        {
            if ((u2g_hall_est_curr_ang > next_angle) && (u2g_hall_est_curr_ang < ref_angle))
            {
                u2g_hall_est_curr_ang = next_angle;
            }
        }
    }
    else
    {
        if (ref_angle >= next_angle)
        {
            if (u2g_hall_est_curr_ang < next_angle)
            {
                u2g_hall_est_curr_ang = next_angle;
            }
        }
        else
        {
            if ((u2g_hall_est_curr_ang < next_angle) && (u2g_hall_est_curr_ang > ref_angle))
            {
                u2g_hall_est_curr_ang = next_angle;
            }
        }
    }

    return EN_COM_STS_OK;
}

int32_t Hall_GetElectricalRpm(void)
{
    uint32_t now_cycle;
    uint32_t rpm_abs;

    now_cycle = DWT_CYCCNT;

    if ((hall_est_is_timeout(now_cycle) != 0u) ||
        (hall_est_avg_sector_cycles == 0u) ||
        (hall_est_dir == 0))
    {
        s4g_hall_speed_elec_rpm = 0;
        return 0;
    }

    /* One electrical revolution contains 6 Hall sectors. */
    rpm_abs = (HALL_CPU_CLOCK_HZ * 10u) / hall_est_avg_sector_cycles;

    if (hall_est_dir < 0)
    {
        s4g_hall_speed_elec_rpm = -(s4)rpm_abs;
    }
    else
    {
        s4g_hall_speed_elec_rpm = (s4)rpm_abs;
    }

    return s4g_hall_speed_elec_rpm;
}

int32_t Hall_GetMechanicalRpm(void)
{
    const ST_INVERTER_CONFIG *cfg;
    int32_t elec_rpm;

    cfg = Config_Get();
    elec_rpm = Hall_GetElectricalRpm();

    if ((cfg == 0) || (cfg->u1t_motor_pole_pairs == 0u))
    {
        s4g_hall_speed_mech_rpm = 0;
        return 0;
    }

    s4g_hall_speed_mech_rpm = elec_rpm / (s4)cfg->u1t_motor_pole_pairs;
    return s4g_hall_speed_mech_rpm;
}

void Hall_OnEdge(uint8_t channel, uint8_t level)
{
    (void)channel;
    (void)level;
}

void EXTI1_IRQHandler(void)
{
    hall_handle_exti_line(HALL_CH1_PIN, 0u);
}

void EXTI2_IRQHandler(void)
{
    hall_handle_exti_line(HALL_CH2_PIN, 1u);
}

void EXTI3_IRQHandler(void)
{
    hall_handle_exti_line(HALL_CH3_PIN, 2u);
}
