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

#ifndef _PARAM_TYPES_H
#define _PARAM_TYPES_H

#include "expr_types.h"
#include "common.h"

#define P_CREATE 1
#define P_NONE 0

#define P_TYPE_BOOL 0
#define P_TYPE_INT 1
#define P_TYPE_DOUBLE 2

#define P_FLAG_NONE 0
#define P_FLAG_READONLY 1
#define P_FLAG_USERDEF (1 << 1)
#define P_FLAG_QVAR (1 << 2)
#define P_FLAG_TVAR (1 << 3)
#define P_FLAG_ALWAYS_MATRIX (1 << 4)
#define P_FLAG_DONT_FREE_MATRIX (1 << 5)
#define P_FLAG_PER_PIXEL (1 << 6)
#define P_FLAG_PER_POINT (1 << 7)

typedef union VALUE_T {
  int bool_val;
  int int_val;
  float float_val;	
} value_t;

/* Parameter Type */
typedef struct PARAM_T {
  char name[MAX_TOKEN_SIZE]; /* name of the parameter, not necessary but useful neverthless */
  short int type; /* parameter number type (int, bool, or float) */	
  short int flags; /* read, write, user defined, etc */	
  short int matrix_flag; /* for optimization purposes */
  void * engine_val; /* pointer to the engine variable */
  void * matrix; /* per pixel / per point matrix for this variable */
  value_t default_init_val; /* a default initial condition value */
  value_t upper_bound; /* this parameter's upper bound */
  value_t lower_bound; /* this parameter's lower bound */
} param_t;

#endif /** !_PARAM_TYPES_H */
