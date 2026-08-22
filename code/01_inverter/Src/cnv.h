#ifndef CNV_H
#define CNV_H

#include <stdint.h>
#include "com.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CNV_OFFSET_SAMPLE_COUNT 100u

void CNV_Init(void);
void CNV_On100us(void);

uint8_t CNV_IsOffsetReady(void);
uint16_t CNV_GetPhaseVOffsetMv(void);
uint16_t CNV_GetPhaseWOffsetMv(void);
int32_t CNV_GetPhaseUCurrentCa(void);
int32_t CNV_GetPhaseVCurrentCa(void);
int32_t CNV_GetPhaseWCurrentCa(void);

EN_COM_STS_T eng_cnv_get_phase_currents(int32_t *phase_u_ca, int32_t *phase_v_ca, int32_t *phase_w_ca);

#ifdef __cplusplus
}
#endif

#endif /* CNV_H */
