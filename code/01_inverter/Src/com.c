/**
 ********************************************************************************
 * @file    com.c
 * @author  Atikan
 * @date    2026-07-19
 * @brief   
 ********************************************************************************
 */

/************************************
 * INCLUDES
 ************************************/
#include "com.h"
/************************************
 * EXTERN VARIABLES
 ************************************/

/************************************
 * PRIVATE MACROS AND DEFINES
 ************************************/

/************************************
 * PRIVATE TYPEDEFS
 ************************************/

/************************************
 * STATIC VARIABLES
 ************************************/

/************************************
 * GLOBAL VARIABLES
 ************************************/

/************************************
 * STATIC FUNCTION PROTOTYPES
 ************************************/

/************************************
 * STATIC FUNCTIONS
 ************************************/

/************************************
 * STATIC FUNCTIONS
 ************************************/
/**
 * @fn     ens_com_axis_locate_s2
 * @brief  Find segment index and delta on a monotonic s2 axis
 * @param  ps2t_axis: Axis breakpoints
 * @param  u1t_count: Number of breakpoints
 * @param  s2t_in: Input value
 * @param  pu1t_idx: Lower index (0 .. count-2)
 * @param  ps4t_num: (in - axis[idx])
 * @param  ps4t_den: (axis[idx+1] - axis[idx]), 0 if equal
 * @return EN_COM_STS_OK or EN_COM_STS_ERR
 */
static EN_COM_STS_T ens_com_axis_locate_s2(const s2 *ps2t_axis, u1 u1t_count, s2 s2t_in,
                                           u1 *pu1t_idx, s4 *ps4t_num, s4 *ps4t_den)
{
    u1 u1t_i;
    s2 s2t_a0;
    s2 s2t_a1;

    if ((ps2t_axis == 0) || (pu1t_idx == 0) || (ps4t_num == 0) || (ps4t_den == 0) || (u1t_count == 0u))
    {
        return EN_COM_STS_ERR;
    }

    if (u1t_count == 1u)
    {
        *pu1t_idx = 0u;
        *ps4t_num = 0;
        *ps4t_den = 0;
        return EN_COM_STS_OK;
    }

    if (s2t_in <= ps2t_axis[0])
    {
        *pu1t_idx = 0u;
        *ps4t_num = 0;
        *ps4t_den = (s4)ps2t_axis[1] - (s4)ps2t_axis[0];
        return EN_COM_STS_OK;
    }

    if (s2t_in >= ps2t_axis[u1t_count - 1u])
    {
        *pu1t_idx = (u1)(u1t_count - 2u);
        *ps4t_num = (s4)ps2t_axis[u1t_count - 1u] - (s4)ps2t_axis[u1t_count - 2u];
        *ps4t_den = *ps4t_num;
        return EN_COM_STS_OK;
    }

    for (u1t_i = 0u; u1t_i < (u1)(u1t_count - 1u); u1t_i++)
    {
        s2t_a0 = ps2t_axis[u1t_i];
        s2t_a1 = ps2t_axis[u1t_i + 1u];
        if (s2t_in <= s2t_a1)
        {
            *pu1t_idx = u1t_i;
            *ps4t_num = (s4)s2t_in - (s4)s2t_a0;
            *ps4t_den = (s4)s2t_a1 - (s4)s2t_a0;
            return EN_COM_STS_OK;
        }
    }

    *pu1t_idx = (u1)(u1t_count - 2u);
    *ps4t_num = 0;
    *ps4t_den = 0;
    return EN_COM_STS_OK;
}

static s2 ens_com_lerp_s2(s2 s2t_a, s2 s2t_b, s4 s4t_num, s4 s4t_den)
{
    s4 s4t_out;

    if (s4t_den == 0)
    {
        return s2t_a;
    }

    s4t_out = (s4)s2t_a + ((((s4)s2t_b - (s4)s2t_a) * s4t_num) / s4t_den);
    return (s2)s4t_out;
}

/************************************
 * GLOBAL FUNCTIONS
 ************************************/
/**
 * @fn     eng_com_interp2d_s2
 * @brief  Bilinear interpolate s2 2D map
 * @param  ps2t_map: Map, first word 0xRRCC (hi=rows, lo=cols), then
 *                   X[cols], then rows of (Y, Z[cols])
 *                   e.g. 0x0506, 10,20,30,40,50,100,  1,...,  2,...,  5,...
 * @param  s2t_x: Column-axis input
 * @param  s2t_y: Row-axis input
 * @param  ps2t_out: Interpolated s2 output
 * @return EN_COM_STS_OK or EN_COM_STS_ERR
 */
EN_COM_STS_T eng_com_interp2d_s2(const s2 *ps2t_map, s2 s2t_x, s2 s2t_y, s2 *ps2t_out)
{
    u1 u1t_rows;
    u1 u1t_cols;
    u1 u1t_ix;
    u1 u1t_iy;
    u1 u1t_row_stride;
    u2 u2t_hdr;
    s4 s4t_x_num;
    s4 s4t_x_den;
    s4 s4t_y_num;
    s4 s4t_y_den;
    const s2 *ps2t_x_axis;
    s2 s2t_z00;
    s2 s2t_z01;
    s2 s2t_z10;
    s2 s2t_z11;
    s2 s2t_z0;
    s2 s2t_z1;
    u2 u2t_row0;
    u2 u2t_row1;

    /*==========INPUT==========*/
    if ((ps2t_map == 0) || (ps2t_out == 0))
    {
        return EN_COM_STS_ERR;
    }

    u2t_hdr = (u2)ps2t_map[0];
    u1t_rows = (u1)((u2t_hdr >> 8) & 0xFFu);
    u1t_cols = (u1)(u2t_hdr & 0xFFu);
    if ((u1t_rows == 0u) || (u1t_cols == 0u))
    {
        return EN_COM_STS_ERR;
    }

    ps2t_x_axis = &ps2t_map[1];
    u1t_row_stride = (u1)(u1t_cols + 1u);

    /*========OPERATION========*/
    if (ens_com_axis_locate_s2(ps2t_x_axis, u1t_cols, s2t_x, &u1t_ix, &s4t_x_num, &s4t_x_den) != EN_COM_STS_OK)
    {
        return EN_COM_STS_ERR;
    }

    /* Y breakpoints are the first cell of each data row. */
    {
        u1 u1t_r;
        s2 s2t_y_prev;
        s2 s2t_y_cur;
        u2 u2t_base;

        u2t_base = (u2)(1u + (u2)u1t_cols);
        if (u1t_rows == 1u)
        {
            u1t_iy = 0u;
            s4t_y_num = 0;
            s4t_y_den = 0;
        }
        else
        {
            s2t_y_prev = ps2t_map[u2t_base];
            if (s2t_y <= s2t_y_prev)
            {
                u1t_iy = 0u;
                s4t_y_num = 0;
                s4t_y_den = (s4)ps2t_map[u2t_base + (u2)u1t_row_stride] - (s4)s2t_y_prev;
            }
            else
            {
                u1t_iy = (u1)(u1t_rows - 2u);
                s4t_y_num = 0;
                s4t_y_den = 0;
                for (u1t_r = 1u; u1t_r < u1t_rows; u1t_r++)
                {
                    s2t_y_cur = ps2t_map[u2t_base + ((u2)u1t_r * (u2)u1t_row_stride)];
                    if (s2t_y <= s2t_y_cur)
                    {
                        u1t_iy = (u1)(u1t_r - 1u);
                        s4t_y_num = (s4)s2t_y - (s4)s2t_y_prev;
                        s4t_y_den = (s4)s2t_y_cur - (s4)s2t_y_prev;
                        break;
                    }
                    s2t_y_prev = s2t_y_cur;
                }
                if (s2t_y >= ps2t_map[u2t_base + ((u2)(u1t_rows - 1u) * (u2)u1t_row_stride)])
                {
                    u1t_iy = (u1)(u1t_rows - 2u);
                    s2t_y_prev = ps2t_map[u2t_base + ((u2)u1t_iy * (u2)u1t_row_stride)];
                    s2t_y_cur = ps2t_map[u2t_base + ((u2)(u1t_iy + 1u) * (u2)u1t_row_stride)];
                    s4t_y_den = (s4)s2t_y_cur - (s4)s2t_y_prev;
                    s4t_y_num = s4t_y_den;
                }
            }
        }
    }

    u2t_row0 = (u2)(1u + (u2)u1t_cols) + ((u2)u1t_iy * (u2)u1t_row_stride);
    if ((u1t_rows == 1u) || (u1t_cols == 1u))
    {
        if (u1t_cols == 1u)
        {
            s2t_z00 = ps2t_map[u2t_row0 + 1u];
            if (u1t_rows == 1u)
            {
                *ps2t_out = s2t_z00;
                return EN_COM_STS_OK;
            }
            u2t_row1 = u2t_row0 + (u2)u1t_row_stride;
            s2t_z10 = ps2t_map[u2t_row1 + 1u];
            *ps2t_out = ens_com_lerp_s2(s2t_z00, s2t_z10, s4t_y_num, s4t_y_den);
            return EN_COM_STS_OK;
        }
        s2t_z00 = ps2t_map[u2t_row0 + 1u + (u2)u1t_ix];
        s2t_z01 = ps2t_map[u2t_row0 + 1u + (u2)u1t_ix + 1u];
        *ps2t_out = ens_com_lerp_s2(s2t_z00, s2t_z01, s4t_x_num, s4t_x_den);
        return EN_COM_STS_OK;
    }

    u2t_row1 = u2t_row0 + (u2)u1t_row_stride;
    s2t_z00 = ps2t_map[u2t_row0 + 1u + (u2)u1t_ix];
    s2t_z01 = ps2t_map[u2t_row0 + 1u + (u2)u1t_ix + 1u];
    s2t_z10 = ps2t_map[u2t_row1 + 1u + (u2)u1t_ix];
    s2t_z11 = ps2t_map[u2t_row1 + 1u + (u2)u1t_ix + 1u];
    s2t_z0 = ens_com_lerp_s2(s2t_z00, s2t_z01, s4t_x_num, s4t_x_den);
    s2t_z1 = ens_com_lerp_s2(s2t_z10, s2t_z11, s4t_x_num, s4t_x_den);

    /*=========OUTPUT==========*/
    *ps2t_out = ens_com_lerp_s2(s2t_z0, s2t_z1, s4t_y_num, s4t_y_den);
    return EN_COM_STS_OK;
}
