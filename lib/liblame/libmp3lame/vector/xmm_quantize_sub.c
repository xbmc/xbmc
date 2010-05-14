/*
 * MP3 quantization, intrinsics functions
 *
 *      Copyright (c) 2005-2006 Gabriel Bouvigne
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.     See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */


#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "lame.h"
#include "machine.h"
#include "encoder.h"
#include "util.h"
#include "lame_intrin.h"



#ifdef HAVE_XMMINTRIN_H

#include <xmmintrin.h>

typedef union {
    __m128  _m128;
    float   _float[4];
} vecfloat_union;



void
init_xrpow_core_sse(gr_info * const cod_info, FLOAT xrpow[576], int upper, FLOAT * sum)
{
    int     i;
    FLOAT   tmp;
    int     upper4 = (upper / 4) * 4;


    const int fabs_mask[4] = { 0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF };
    const __m128 vec_fabs_mask = _mm_loadu_ps((float const*) &fabs_mask[0]);
    vecfloat_union vec_xrpow_max;
    vecfloat_union vec_sum;

    _mm_prefetch((char *) cod_info->xr, _MM_HINT_T0);
    _mm_prefetch((char *) xrpow, _MM_HINT_T0);

    vec_xrpow_max._m128 = _mm_set_ps1(0);
    vec_sum._m128 = _mm_set_ps1(0);

    for (i = 0; i < upper4; i += 4) {
        __m128  vec_tmp;

        vec_tmp = _mm_loadu_ps(&(cod_info->xr[i])); /* load */
        vec_tmp = _mm_and_ps(vec_tmp, vec_fabs_mask); /* fabs */

        vec_sum._m128 = _mm_add_ps(vec_sum._m128, vec_tmp);

        vec_tmp = _mm_sqrt_ps(_mm_mul_ps(vec_tmp, _mm_sqrt_ps(vec_tmp)));

        _mm_storeu_ps(&(xrpow[i]), vec_tmp); /* store into xrpow[] */

        vec_xrpow_max._m128 = _mm_max_ps(vec_xrpow_max._m128, vec_tmp); /* retrieve max */
    }

    *sum = vec_sum._float[0] + vec_sum._float[1] + vec_sum._float[2] + vec_sum._float[3];
    cod_info->xrpow_max = Max(vec_xrpow_max._float[0],
                              Max(vec_xrpow_max._float[1],
                                  Max(vec_xrpow_max._float[2], vec_xrpow_max._float[3])));



    for (i = upper4; i <= upper; ++i) {
        tmp = fabs(cod_info->xr[i]);
        *sum += tmp;
        xrpow[i] = sqrt(tmp * sqrt(tmp));

        if (xrpow[i] > cod_info->xrpow_max)
            cod_info->xrpow_max = xrpow[i];
    }
}

#endif	/* HAVE_XMMINTRIN_H */

