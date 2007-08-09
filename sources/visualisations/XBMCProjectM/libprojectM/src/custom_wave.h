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

#ifndef _CUSTOM_WAVE_H
#define _CUSTOM_WAVE_H

#define CUSTOM_WAVE_DEBUG 0

#include "expr_types.h"
#include "custom_wave_types.h"
#include "preset_types.h"

void free_custom_wave(custom_wave_t * custom_wave);
custom_wave_t * new_custom_wave(int id);

void free_per_point_eqn(per_point_eqn_t * per_point_eqn);
per_point_eqn_t * new_per_point_eqn(int index, param_t * param,gen_expr_t * gen_expr);
void reset_per_point_eqn_array(custom_wave_t * custom_wave);
custom_wave_t * find_custom_wave(int id, preset_t * preset, int create_flag);

int add_per_point_eqn(char * name, gen_expr_t * gen_expr, custom_wave_t * custom_wave);
void evalCustomWaveInitConditions(preset_t *preset);
void evalPerPointEqns(void*);
custom_wave_t * nextCustomWave(preset_t *preset);
void load_unspecified_init_conds(custom_wave_t * custom_wave);

#endif /** !_CUSTOM_WAVE_H */
