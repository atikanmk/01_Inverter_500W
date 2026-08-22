#ifndef TEMPLATE_H
#define TEMPLATE_H

#include <stdint.h>

#define template_h_version 1u

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;

#ifdef __cplusplus
extern "C" {
#endif

/* Function declaration: [RETURN_TYPE][X1]_[MODULE]_[NAME]. */
void vg_template_init(void);
void vg_template_reset(void);
void vg_template_on100us(void);
void vg_template_on1ms(void);
u8 vg_template_is_initialized(void);

#ifdef __cplusplus
}
#endif

#endif /* template_h */
