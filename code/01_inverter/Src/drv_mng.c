/**
 ********************************************************************************
 * @file    drv_mng.c
 * @author  Atikan
 * @date    2026-09-07
 * @brief
 ********************************************************************************
 */

/************************************
 * INCLUDES
 ************************************/
#include "cnv.h"
#include "drv_mng.h"
#include "pwm.h"
#include "task.h"

/************************************
 * EXTERN VARIABLES
 ************************************/

/************************************
 * PRIVATE MACROS AND DEFINES
 ************************************/
#define DRV_MNG_THROTTLE_ENABLE_PERC  5u
#define DRV_MNG_THROTTLE_DISABLE_PERC 2u

/************************************
 * PRIVATE TYPEDEFS
 ************************************/

/************************************
 * STATIC VARIABLES
 ************************************/
static u1 u1sg_drv_mng_pwm_enabled;

/************************************
 * GLOBAL VARIABLES
 ************************************/

/************************************
 * FUNCTION PROTOTYPES
 ************************************/

/************************************
 * FUNCTIONS
 ************************************/
/**
 * @fn     eng_drv_mng_init
 * @id     DRV_MNG-001
 * @brief  Initialize driver manager states
 * @return Driver manager initialization status
 */
EN_COM_STS_T eng_drv_mng_init(void)
{
    u1sg_drv_mng_pwm_enabled = 0u;
    PWM_DisableOutput();

    return EN_COM_STS_OK;
}

/**
 * @fn     eng_drv_mng_1ms
 * @id     DRV_MNG-002
 * @brief  Control PWM output from throttle position and mechanical speed
 * @return Driver manager cyclic task status
 */
EN_COM_STS_T eng_drv_mng_1ms(void)
{
    u2 u2t_throttle_perc;

    if (eng_cnv_get_throttle_perc(&u2t_throttle_perc) != EN_COM_STS_OK)
    {
        return EN_COM_STS_ERR;
    }

    if ((u2t_throttle_perc > DRV_MNG_THROTTLE_ENABLE_PERC) &&
        (u1sg_drv_mng_pwm_enabled == 0u))
    {
        PWM_EnableOutput();
        u1sg_drv_mng_pwm_enabled = 1u;
    }
    else if ((u2t_throttle_perc < DRV_MNG_THROTTLE_DISABLE_PERC) &&
             (s4g_task_speed_mech_rpm == 0) &&
             (u1sg_drv_mng_pwm_enabled != 0u))
    {
        PWM_DisableOutput();
        u1sg_drv_mng_pwm_enabled = 0u;
    }

    return EN_COM_STS_OK;
}
