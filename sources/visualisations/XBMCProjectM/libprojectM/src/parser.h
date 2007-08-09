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

#ifndef _PARSER_H
#define _PARSER_H

#define PARSE_DEBUG 0
#include "expr_types.h"
#include "per_frame_eqn_types.h"
#include "init_cond_types.h"
#include "preset_types.h"

per_frame_eqn_t * parse_per_frame_eqn(FILE * fs, int index, struct PRESET_T * preset);
int parse_per_pixel_eqn(FILE * fs, preset_t * preset, char * init_string);
init_cond_t * parse_init_cond(FILE * fs, char * name, struct PRESET_T * preset);
int parse_preset_name(FILE * fs, char * name);
int parse_top_comment(FILE * fs);
int parse_line(FILE * fs, struct PRESET_T * preset);

typedef enum {
  NORMAL_LINE_MODE,
  PER_FRAME_LINE_MODE,
  PER_PIXEL_LINE_MODE,
  PER_FRAME_INIT_LINE_MODE,
  INIT_COND_LINE_MODE,
  CUSTOM_WAVE_PER_POINT_LINE_MODE,
  CUSTOM_WAVE_PER_FRAME_LINE_MODE,
  CUSTOM_WAVE_WAVECODE_LINE_MODE,
  CUSTOM_SHAPE_SHAPECODE_LINE_MODE,
  CUSTOM_SHAPE_PER_FRAME_LINE_MODE,
  CUSTOM_SHAPE_PER_FRAME_INIT_LINE_MODE,
  CUSTOM_WAVE_PER_FRAME_INIT_LINE_MODE
} line_mode_t;

#endif /** !_PARSER_H */
