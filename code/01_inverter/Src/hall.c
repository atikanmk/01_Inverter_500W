#include "hall.h"

#include "config.h"
#include "system_clock.h"

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
#define GPIOA_ODR REG32(GPIOA_BASE_ADDR + 0x0Cu)

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
/* Allow RPM/angle timing once at least this many sector samples exist. */
#define HALL_EST_MIN_HISTORY 2u
#define HALL_EST_TIMEOUT_CYCLES (SystemCoreClock_Get())
#define HALL_EST_MIN_MECH_RPM 30u
/* Fresh measurement window: tolerate missed edges up to this many sectors. */
#define HALL_SPEED_TIMEOUT_SECTORS 6u
/* After measurement timeout, hold last filtered RPM this many extra sectors. */
#define HALL_RPM_HOLD_SECTORS 6u
/* LPF: y += (x - y) >> N  (N=3 => alpha ~= 1/8). */
#define HALL_RPM_LPF_SHIFT 3u
/* Decay toward 0 each 1 ms tick while no fresh measurement: rpm -= rpm >> N. */
#define HALL_RPM_DECAY_SHIFT 3u

#define HALL_LINE_MASK ((1u << HALL_CH1_PIN) | (1u << HALL_CH2_PIN) | (1u << HALL_CH3_PIN))
#define HALL_PATTERN_INVALID_0 0u
#define HALL_PATTERN_INVALID_7 7u

static volatile u1 u1g_hall_ch1_state;
static volatile u1 u1g_hall_ch2_state;
static volatile u1 u1g_hall_ch3_state;
static volatile u2 u2g_hall_ch1_state;
static volatile u2 u2g_hall_ch2_state;
static volatile u2 u2g_hall_ch3_state;
static volatile u1 u1g_hall_state;
static volatile u2 u2g_hall_angle_deg;
static volatile u2 u2g_hall_est_curr_ang;
static volatile u2 u2g_hall_est_ang_ref;
static volatile u2 u2g_hall_est_ang_next;

static volatile uint8_t hall_level[3];
static volatile uint32_t hall_edge_count[3];
static volatile uint32_t hall_last_irq_cycle[3];
static volatile uint32_t hall_invalid_pattern_count;
static volatile uint8_t hall_pattern_valid;
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
static volatile s4 s4g_hall_speed_elec_rpm_raw;
static volatile s4 s4g_hall_speed_mech_rpm;

static uint32_t hall_debounce_cycles;

static const uint8_t hall_pin_map[3] = { HALL_CH1_PIN, HALL_CH2_PIN, HALL_CH3_PIN };

static void hall_est_reset_history(void);
static void hall_rpm_set_filtered(s4 rpm_meas);
static void hall_rpm_decay(void);
static uint8_t hall_pattern_is_valid(uint8_t pattern);

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

static uint8_t hall_pattern_is_valid(uint8_t pattern)
{
    return (uint8_t)((pattern != HALL_PATTERN_INVALID_0) &&
                     (pattern != HALL_PATTERN_INVALID_7));
}

static void hall_rpm_set_filtered(s4 rpm_meas)
{
    s4 rpm_filt;
    s4 delta;

    s4g_hall_speed_elec_rpm_raw = rpm_meas;
    rpm_filt = s4g_hall_speed_elec_rpm;

    /* First valid sample: acquire immediately so startup is not sluggish. */
    if (rpm_filt == 0)
    {
        s4g_hall_speed_elec_rpm = rpm_meas;
        return;
    }

    delta = rpm_meas - rpm_filt;
    rpm_filt += (delta >> HALL_RPM_LPF_SHIFT);
    s4g_hall_speed_elec_rpm = rpm_filt;
}

static void hall_rpm_decay(void)
{
    s4 rpm_filt;

    rpm_filt = s4g_hall_speed_elec_rpm;
    if (rpm_filt > 0)
    {
        rpm_filt -= (rpm_filt >> HALL_RPM_DECAY_SHIFT);
        if (rpm_filt < 0)
        {
            rpm_filt = 0;
        }
    }
    else if (rpm_filt < 0)
    {
        rpm_filt -= (rpm_filt >> HALL_RPM_DECAY_SHIFT);
        if (rpm_filt > 0)
        {
            rpm_filt = 0;
        }
    }

    /* Snap tiny residual to zero so decay finishes cleanly. */
    if ((rpm_filt > -2) && (rpm_filt < 2))
    {
        rpm_filt = 0;
        s4g_hall_speed_elec_rpm_raw = 0;
    }

    s4g_hall_speed_elec_rpm = rpm_filt;
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
    s4g_hall_speed_elec_rpm_raw = 0;
    s4g_hall_speed_elec_rpm = 0;
    s4g_hall_speed_mech_rpm = 0;

    u2g_hall_est_ang_ref = hall_norm_angle(u2g_hall_angle_deg);
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

    if (sector_cycles == 0u)
    {
        return;
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

    /* Average as soon as the minimum number of samples is available. */
    if (hall_est_sector_hist_count >= HALL_EST_MIN_HISTORY)
    {
        hall_est_avg_sector_cycles =
            hall_est_sector_hist_sum / (uint32_t)hall_est_sector_hist_count;
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
    const ST_INVERTER_CONFIG *pst_config;

    hall_level[0] = hall_read_pin(HALL_CH1_PIN);
    hall_level[1] = hall_read_pin(HALL_CH2_PIN);
    hall_level[2] = hall_read_pin(HALL_CH3_PIN);

    u1g_hall_ch1_state = hall_level[0];
    u1g_hall_ch2_state = hall_level[1];
    u1g_hall_ch3_state = hall_level[2];

    u2g_hall_ch1_state = u1g_hall_ch1_state * 10u;
    u2g_hall_ch2_state = u1g_hall_ch2_state * 10u;
    u2g_hall_ch3_state = u1g_hall_ch3_state * 10u;

    u1g_hall_state = (u1)((hall_level[2] << 2) | (hall_level[1] << 1) | hall_level[0]);
    hall_pattern_valid = hall_pattern_is_valid(u1g_hall_state);

    pst_config = config_get();
    if (hall_pattern_valid != 0u)
    {
        u2g_hall_angle_deg = pst_config->u2t_hall_pattern_angle_deg[u1g_hall_state];
    }
    /* Invalid 000/111: keep previous valid angle; do not remap to 0 deg. */
}

static void hall_est_reset_on_interrupt(void)
{
    uint32_t now_cycle;
    uint32_t sector_cycles;
    uint32_t sample_cycles;
    uint8_t curr_sector;
    uint8_t sector_step;
    uint8_t sector_span;
    int8_t next_dir;

    now_cycle = DWT_CYCCNT;
    curr_sector = hall_angle_to_sector(u2g_hall_angle_deg);

    if (hall_est_is_timeout(now_cycle) != 0u)
    {
        hall_est_force_reset(now_cycle);
    }

    next_dir = 0;
    sector_span = 1u;
    if (hall_est_has_prev_sector != 0u)
    {
        sector_step = (uint8_t)((curr_sector + 6u - hall_est_prev_sector) % 6u);

        /* Accept +1/+2 FW and -1/-2 (5/4) REV; step 2/4 = one missed sector. */
        if (sector_step == 1u)
        {
            next_dir = 1;
            sector_span = 1u;
        }
        else if (sector_step == 2u)
        {
            next_dir = 1;
            sector_span = 2u;
        }
        else if (sector_step == 5u)
        {
            next_dir = -1;
            sector_span = 1u;
        }
        else if (sector_step == 4u)
        {
            next_dir = -1;
            sector_span = 2u;
        }
        else if (sector_step == 0u)
        {
            /* Same sector / bounce after debounce: ignore timing update. */
            return;
        }

        if (next_dir == 0)
        {
            /* Ambiguous jump (e.g. 180 deg): keep last speed, drop direction. */
            hall_est_dir = 0;
            hall_est_reset_history();
        }
        else
        {
            if ((hall_est_dir != 0) && (next_dir != hall_est_dir))
            {
                /* True reverse: rebuild history with the new direction. */
                hall_est_dir = next_dir;
                hall_est_reset_history();
            }
            else
            {
                hall_est_dir = next_dir;
            }

            sector_cycles = now_cycle - hall_est_last_cycle;
            if (sector_span > 1u)
            {
                sample_cycles = sector_cycles / (uint32_t)sector_span;
            }
            else
            {
                sample_cycles = sector_cycles;
            }
            hall_est_add_sector_sample(now_cycle, sample_cycles);
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
    u2g_hall_est_ang_ref = hall_norm_angle(u2g_hall_angle_deg);

    if (hall_est_dir > 0)
    {
        u2g_hall_est_ang_next =
            hall_norm_angle((uint16_t)(u2g_hall_est_ang_ref + HALL_EST_SECTOR_DEG));
    }
    else if (hall_est_dir < 0)
    {
        if (u2g_hall_est_ang_ref >= HALL_EST_SECTOR_DEG)
        {
            u2g_hall_est_ang_next =
                (uint16_t)(u2g_hall_est_ang_ref - HALL_EST_SECTOR_DEG);
        }
        else
        {
            u2g_hall_est_ang_next =
                (uint16_t)(HALL_EST_FULL_DEG + u2g_hall_est_ang_ref - HALL_EST_SECTOR_DEG);
        }
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
        hall_edge_count[channel]++;
        Hall_OnEdge(channel, hall_level[channel]);

        if (hall_pattern_valid != 0u)
        {
            hall_est_reset_on_interrupt();
        }
        else
        {
            /* 000/111: noise or open wiring — count and skip estimator update. */
            hall_invalid_pattern_count++;
        }
    }
}

void Hall_Init(void)
{
    uint32_t shift;
    uint8_t i;

    RCC_APB2ENR |= (RCC_APB2ENR_AFIOEN | RCC_APB2ENR_IOPAEN);

    hall_timebase_init();
    hall_debounce_cycles = (SystemCoreClock_Get() / 1000000u) * HALL_DEBOUNCE_US;

    /*
     * Configure PA1, PA2, PA3 as input with pull-up/pull-down (MODE=00, CNF=10).
     * ODR=1 selects pull-up to reject floating/open-wire noise that yields 000.
     * If the Hall sensors are open-collector to GND this is the correct bias.
     * If board already has strong external pull-downs, change ODR bits to 0.
     */
    for (i = 0u; i < 3u; ++i)
    {
        shift = hall_pin_map[i] * 4u;
        GPIOA_CRL &= ~(0xFu << shift);
        GPIOA_CRL |= (0x8u << shift); /* input pull-up/pull-down */
        GPIOA_ODR |= (1u << hall_pin_map[i]); /* pull-up */
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
    hall_invalid_pattern_count = 0u;
    hall_pattern_valid = 0u;
    hall_est_last_cycle = 0u;
    hall_est_has_prev_sector = 0u;
    hall_est_prev_sector = 0u;
    hall_est_dir = 0;
    s4g_hall_speed_elec_rpm = 0;
    s4g_hall_speed_elec_rpm_raw = 0;
    s4g_hall_speed_mech_rpm = 0;
    hall_est_reset_history();
    hall_refresh_all_levels();
    if (hall_pattern_valid != 0u)
    {
        hall_est_reset_on_interrupt();
    }

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

uint32_t Hall_GetInvalidPatternCount(void)
{
    return hall_invalid_pattern_count;
}

EN_COM_STS_T eng_hall_ang_est(u2 *pu2t_angle_deg)
{
    const ST_INVERTER_CONFIG *cfg;
    uint32_t now_cycle;
    uint32_t elapsed_cycle;
    uint32_t est_offset;
    uint32_t sector_cycles;
    uint32_t max_sector_cycles;
    uint16_t ref_angle;
    uint16_t next_angle;
    uint16_t estimated;

    now_cycle = DWT_CYCCNT;

    if (hall_est_is_timeout(now_cycle) != 0u)
    {
        hall_est_force_reset(now_cycle);
    }

    sector_cycles = hall_est_avg_sector_cycles;
    cfg = config_get();
    max_sector_cycles = 0u;
    if ((cfg != 0) && (cfg->u1t_motor_pole_pairs != 0u))
    {
        max_sector_cycles = (SystemCoreClock_Get() * 10u) /
            ((uint32_t)HALL_EST_MIN_MECH_RPM * cfg->u1t_motor_pole_pairs);
    }

    elapsed_cycle = now_cycle - hall_est_last_cycle;

    if ((sector_cycles == 0u) || (hall_est_dir == 0) ||
        ((max_sector_cycles != 0u) &&
         ((sector_cycles > max_sector_cycles) || (elapsed_cycle > max_sector_cycles))))
    {
        u2g_hall_est_curr_ang = hall_norm_angle((u2)(u2g_hall_est_ang_ref + 30u));
        *pu2t_angle_deg = (u2)u2g_hall_est_curr_ang;
        return EN_COM_STS_OK;
    }

    ref_angle = hall_norm_angle(u2g_hall_est_ang_ref);
    next_angle = hall_norm_angle(u2g_hall_est_ang_next);

    if (elapsed_cycle >= sector_cycles)
    {
        u2g_hall_est_curr_ang = u2g_hall_est_ang_next;
        *pu2t_angle_deg = (u2)u2g_hall_est_curr_ang;
    }

    est_offset = (elapsed_cycle * HALL_EST_SECTOR_DEG) / sector_cycles;
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

    *pu2t_angle_deg = u2g_hall_est_curr_ang;

    return EN_COM_STS_OK;
}

int32_t eng_hall_elec_rpm_est(void)
{
    uint32_t now_cycle;
    uint32_t elapsed_cycle;
    uint32_t rpm_abs;
    uint32_t sector_cycles;
    uint32_t fresh_limit_cycles;
    uint32_t hold_limit_cycles;
    s4 rpm_meas;

    now_cycle = DWT_CYCCNT;

    if (hall_est_is_timeout(now_cycle) != 0u)
    {
        /* Long stall (~1 s): clear completely. */
        hall_est_force_reset(now_cycle);
        return 0;
    }

    sector_cycles = hall_est_avg_sector_cycles;
    elapsed_cycle = now_cycle - hall_est_last_cycle;

    if ((sector_cycles != 0u) && (hall_est_dir != 0))
    {
        fresh_limit_cycles = sector_cycles * HALL_SPEED_TIMEOUT_SECTORS;
        hold_limit_cycles =
            sector_cycles * (HALL_SPEED_TIMEOUT_SECTORS + HALL_RPM_HOLD_SECTORS);

        if (elapsed_cycle <= fresh_limit_cycles)
        {
            /* One electrical revolution contains 6 Hall sectors. */
            rpm_abs = (SystemCoreClock_Get() * 10u) / sector_cycles;
            if (hall_est_dir < 0)
            {
                rpm_meas = -(s4)rpm_abs;
            }
            else
            {
                rpm_meas = (s4)rpm_abs;
            }
            hall_rpm_set_filtered(rpm_meas);
        }
        else if (elapsed_cycle <= hold_limit_cycles)
        {
            /* No fresh edges yet: hold last filtered value. */
        }
        else
        {
            /* Still moving window expired: decay slowly toward 0. */
            hall_rpm_decay();
        }
    }
    else
    {
        /* No valid timing yet: decay any residual instead of hard zero. */
        if (s4g_hall_speed_elec_rpm != 0)
        {
            hall_rpm_decay();
        }
        else
        {
            s4g_hall_speed_elec_rpm_raw = 0;
        }
    }

    return s4g_hall_speed_elec_rpm;
}

int32_t eng_hall_mech_rpm_est(void)
{
    const ST_INVERTER_CONFIG *cfg;
    int32_t elec_rpm;

    cfg = config_get();
    elec_rpm = s4g_hall_speed_elec_rpm;

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
