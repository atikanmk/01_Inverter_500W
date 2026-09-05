#ifndef ADC_H
#define ADC_H

#include "com.h"
#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ADC_RAW_MAX_12BIT 4095u

void ADC_Init(void);
EN_COM_STS_T ADC_TriggerFromPwmCycle(void);
EN_COM_STS_T ADC_ReadAll12bit(u2 raw_array[EN_CONFIG_ADC_COUNT]);
EN_COM_STS_T ADC_GetValue(EN_CONFIG_ADC_T adc_id, u2 *value);

#ifdef __cplusplus
}
#endif

#endif /* ADC_H */
