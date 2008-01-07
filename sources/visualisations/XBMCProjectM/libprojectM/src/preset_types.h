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

#ifndef _PRESET_TYPES_H
#define _PRESET_TYPES_H

#include "splaytree_types.h"
#include "expr_types.h"
#include "per_pixel_eqn_types.h"
#include "per_frame_eqn_types.h"
#include "custom_shape_types.h"
#include "custom_wave_types.h"


typedef enum {	
  ALPHA_NEXT,
  ALPHA_PREVIOUS,
  RANDOM_NEXT,
  RESTART_ACTIVE,
} switch_mode_t;

typedef struct PRESET_T {
  
  char name[MAX_TOKEN_SIZE]; /* preset name as parsed in file */
  char file_path[MAX_PATH_SIZE]; /* Points to the preset file name */

  int index; /* preset index */

  int per_pixel_eqn_string_index;
  int per_frame_eqn_string_index;
  int per_frame_init_eqn_string_index;
	
  int per_pixel_flag[NUM_OPS];
  char per_pixel_eqn_string_buffer[STRING_BUFFER_SIZE];
  char per_frame_eqn_string_buffer[STRING_BUFFER_SIZE];
  char per_frame_init_eqn_string_buffer[STRING_BUFFER_SIZE];

  /* Data structures that contain equation and initial condition information */
  splaytree_t * per_frame_eqn_tree;   /* per frame equations */
  splaytree_t * per_pixel_eqn_tree; /* per pixel equation tree */
  gen_expr_t * per_pixel_eqn_array[NUM_OPS]; /* per pixel equation array */
  splaytree_t * per_frame_init_eqn_tree; /* per frame initial equations */
  splaytree_t * init_cond_tree; /* initial conditions */
  splaytree_t * user_param_tree; /* user parameter splay tree */

  splaytree_t * custom_wave_tree; /* custom wave forms for this preset */
  splaytree_t * custom_shape_tree; /* custom shapes for this preset */

} preset_t;

#endif /** !_PRESET_TYPES_H */
