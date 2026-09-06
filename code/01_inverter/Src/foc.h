#ifndef FOC_H
#define FOC_H

#include <stdint.h>
#include "com.h"

#ifdef __cplusplus
extern "C" {
#endif

EN_COM_STS_T eng_foc_init(void);
u2 u16g_foc_main(void);
EN_COM_STS_T eng_foc_curr_ref(s4 s4t_trq_cmd_ca, s4 s4t_iq_feb_ca, s4 *s4t_iq_ref_ca, s4 *s4t_id_ref_ca);
EN_COM_STS_T eng_foc_curr_pi_cal(s4 s4t_iq_ref_ca, s4 s4t_id_ref_ca, s4 s4t_iq_feb_ca, s4 s4t_id_feb_ca, s4 *s4t_vq_cpct, s4 *s4t_vd_cpct);

#ifdef __cplusplus
}
#endif

#endif /* FOC_H */
