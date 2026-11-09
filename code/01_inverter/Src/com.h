/**
 ********************************************************************************
 * @file    com.h
 * @author  Atikan
 * @date    2026-07-19
 * @brief   
 ********************************************************************************
 */

#ifndef COM_H
#define COM_H

#ifdef __cplusplus
extern "C" {
#endif

/************************************
 * INCLUDES
 ************************************/
#include <stdint.h>
/************************************
 * MACROS AND DEFINES
 ************************************/
#define NULL ((void *)0)

/************************************
 * TYPEDEFS
 ************************************/
typedef uint8_t  u1;
typedef uint16_t u2;
typedef uint32_t u4;
typedef int8_t   s1;
typedef int16_t  s2;
typedef int32_t  s4;
typedef float    f4;

typedef enum
{
    EN_COM_STS_OK = 0,
    EN_COM_STS_ERR  = 1,
    EN_COM_STS_RUNNING = 2
} EN_COM_STS_T;
/************************************
 * EXPORTED VARIABLES
 ************************************/

/************************************
 * GLOBAL FUNCTION PROTOTYPES
 ************************************/
extern EN_COM_STS_T eng_com_interp2d_s2(const s2 *ps2t_map, s2 s2t_x, s2 s2t_y, s2 *ps2t_out);

#ifdef __cplusplus
}
#endif

#endif /* COM_H */
