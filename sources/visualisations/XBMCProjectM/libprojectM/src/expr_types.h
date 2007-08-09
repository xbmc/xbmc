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

#ifndef _EXPR_TYPES_H
#define _EXPR_TYPES_H

#include "param_types.h"

#define CONST_STACK_ELEMENT 0
#define EXPR_STACK_ELEMENT 1

/* General Expression Type */
typedef struct GEN_EXPR_T {
  int type;
  void * item;
} gen_expr_t;

typedef union TERM_T {
  float constant; /* static variable */
  struct PARAM_T * param; /* pointer to a changing variable */
} term_t;

/* Value expression, contains a term union */
typedef struct VAL_EXPR_T {
  int type;
  term_t term;
} val_expr_t;

/* Infix Operator Function */
typedef struct INFIX_OP_T {
  int type;
  int precedence;  
} infix_op_t;

/* A binary expression tree ordered by operator precedence */
typedef struct TREE_EXPR_T {
  infix_op_t * infix_op; /* null if leaf */
  gen_expr_t * gen_expr;
  struct TREE_EXPR_T * left, * right;
} tree_expr_t;

/* A function expression in prefix form */
typedef struct PREFUN_EXPR_T {
  float (*func_ptr)(void*);
  int num_args;
  gen_expr_t ** expr_list;
} prefun_expr_t;




#endif /** _EXPR_TYPES_H */
