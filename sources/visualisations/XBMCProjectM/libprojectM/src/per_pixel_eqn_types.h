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

#ifndef _PER_PIXEL_EQN_TYPES_H
#define _PER_PIXEL_EQN_TYPES_H

/* This is sort of ugly, but it is also the fastest way to access the per pixel equations */
#include "common.h"
#include "expr_types.h"

typedef struct PER_PIXEL_EQN_T {
  int index; /* used for splay tree ordering. */
  int flags; /* primarily to specify if this variable is user-defined */
  param_t * param; 
  gen_expr_t * gen_expr;	
} per_pixel_eqn_t;


#define ZOOM_OP 0
#define ZOOMEXP_OP 1
#define ROT_OP 2
#define CX_OP 3
#define CY_OP 4
#define SX_OP 5
#define SY_OP  6
#define DX_OP 7
#define DY_OP 8
#define WARP_OP 9
#define NUM_OPS 10 /* obviously, this number is dependent on the number of existing per pixel operations */

#endif /** !_PER_PIXEL_EQN_TYPES_H */
