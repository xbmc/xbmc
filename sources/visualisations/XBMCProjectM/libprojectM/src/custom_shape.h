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

#ifndef _CUSTOM_SHAPE_H
#define _CUSTOM_SHAPE_H

#define CUSTOM_SHAPE_DEBUG 0

#include "expr_types.h"
#include "custom_shape_types.h"
#include "preset_types.h"

void free_custom_shape(custom_shape_t * custom_shape);
custom_shape_t * new_custom_shape(int id);
custom_shape_t * find_custom_shape(int id, preset_t * preset, int create_flag);
void load_unspecified_init_conds_shape(custom_shape_t * custom_shape);
void evalCustomShapeInitConditions(preset_t *preset);
custom_shape_t * nextCustomShape(preset_t *preset);

#endif /** !_CUSTOM_SHAPE_H */
