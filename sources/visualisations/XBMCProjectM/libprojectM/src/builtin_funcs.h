/**
 * projectM -- Milkdrop-esque visualisation SDK
 * Copyright (C)2003-2004 projectM Team
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * See 'LICENSE.txt' included within this release
 *
 */
/* Wrappers for all the builtin functions 
   The arg_list pointer is a list of floats. Its
   size is equal to the number of arguments the parameter
   takes */

#ifndef _BUILTIN_FUNCS_H
#define _BUILTIN_FUNCS_H
 
inline float below_wrapper(float * arg_list);
inline float above_wrapper(float * arg_list);
inline float equal_wrapper(float * arg_list);
inline float if_wrapper(float * arg_list);
inline float bnot_wrapper(float * arg_list);
inline float rand_wrapper(float * arg_list);
inline float bor_wrapper(float * arg_list);
inline float band_wrapper(float * arg_list);
inline float sigmoid_wrapper(float * arg_list);
inline float max_wrapper(float * arg_list);
inline float min_wrapper(float * arg_list);
inline float sign_wrapper(float * arg_list);
inline float sqr_wrapper(float * arg_list);
inline float int_wrapper(float * arg_list);
inline float nchoosek_wrapper(float * arg_list);
inline float sin_wrapper(float * arg_list);
inline float cos_wrapper(float * arg_list);
inline float tan_wrapper(float * arg_list);
inline float fact_wrapper(float * arg_list);
inline float asin_wrapper(float * arg_list);
inline float acos_wrapper(float * arg_list);
inline float atan_wrapper(float * arg_list);
inline float atan2_wrapper(float * arg_list);

inline float pow_wrapper(float * arg_list);
inline float exp_wrapper(float * arg_list);
inline float abs_wrapper(float * arg_list);
inline float log_wrapper(float *arg_list);
inline float log10_wrapper(float * arg_list);
inline float sqrt_wrapper(float * arg_list);

#endif /** !_BUILTIN_FUNCS_H */
