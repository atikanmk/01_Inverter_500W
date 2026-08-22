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

/************************************
 * TYPEDEFS
 ************************************/
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int8_t   s8;
typedef int16_t  s16;
typedef int32_t  s32;

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


#ifdef __cplusplus
}
#endif

#endif /* COM_H */