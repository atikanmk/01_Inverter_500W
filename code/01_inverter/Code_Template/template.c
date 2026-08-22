#include "template.h"

/* Define: module constants use lowercase names. */
#define template_tick_per_1ms 10u

/* Type: use u8, u16, u32, s8, s16, s32, stc, or enum. */
typedef enum
{
	template_state_idle = 0,
	template_state_run
} enum_template_state;

/* Variable: [TYPE][X1][X2]_[MODULE]_[NAME]. */
static volatile u8 sv_template_initialized;
static volatile u32 sc_template_tick_100us;
static enum_template_state sv_template_state;

/* Function declaration. */
static void vs_template_clear_state(void);

/* Function body. */
void vg_template_init(void)
{
	vs_template_clear_state();
	sv_template_initialized = 1u;
}

void vg_template_reset(void)
{
	vs_template_clear_state();
}

void vg_template_on100us(void)
{
	if (sv_template_initialized == 0u)
	{
		return;
	}

	sc_template_tick_100us++;
}

void vg_template_on1ms(void)
{
	if (sv_template_initialized == 0u)
	{
		return;
	}

	/* Add periodic 1 ms processing here. */
}

u8 vg_template_is_initialized(void)
{
	return sv_template_initialized;
}

static void vs_template_clear_state(void)
{
	sc_template_tick_100us = 0u;
	sv_template_state = template_state_idle;
}
