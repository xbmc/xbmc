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

#ifndef _PER_PIXEL_EQN_H
#define _PER_PIXEL_EQN_H

#include "expr_types.h"
#include "preset_types.h"

#define PER_PIXEL_EQN_DEBUG 0

 void evalPerPixelEqns(preset_t *preset);
 int isPerPixelEqn(int index);
int add_per_pixel_eqn(char * name, gen_expr_t * gen_expr, struct PRESET_T * preset);
void free_per_pixel_eqn(per_pixel_eqn_t * per_pixel_eqn);
per_pixel_eqn_t * new_per_pixel_eqn(int index, param_t  * param, gen_expr_t * gen_expr);
 int resetPerPixelEqnFlags();

#endif /** !_PER_PIXEL_EQN_H */
