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

/* eval.h: evaluation functions of expressions */
#ifndef _EVAL_H
#define _EVAL_H

#include "projectM.h"
#include "func_types.h"
#include "param_types.h"

//#define EVAL_DEBUG 0
//#define EVAL_DEBUG_DOUBLE 2

#define VAL_T 1
#define PREFUN_T 3
#define TREE_T 4
#define NONE_T 0


#define CONSTANT_TERM_T 0
#define PARAM_TERM_T 1

#define INFIX_ADD 0
#define INFIX_MINUS 1
#define INFIX_MOD 2
#define INFIX_DIV 3
#define INFIX_MULT 4
#define INFIX_OR 5
#define INFIX_AND 6

float eval_gen_expr(gen_expr_t * gen_expr);
inline gen_expr_t * opt_gen_expr(gen_expr_t * gen_expr, int ** param_list);

gen_expr_t * const_to_expr(float val);
gen_expr_t * param_to_expr(struct PARAM_T * param);
gen_expr_t * prefun_to_expr(float (*func_ptr)(), gen_expr_t ** expr_list, int num_args);

tree_expr_t * new_tree_expr(infix_op_t * infix_op, gen_expr_t * gen_expr, tree_expr_t * left, tree_expr_t * right);
gen_expr_t * new_gen_expr(int type, void * item);
val_expr_t * new_val_expr(int type, term_t term);

int free_gen_expr(gen_expr_t * gen_expr);
int free_prefun_expr(prefun_expr_t * prefun_expr);
int free_tree_expr(tree_expr_t * tree_expr);
int free_val_expr(val_expr_t * val_expr);

infix_op_t * new_infix_op(int type, int precedence);
int init_infix_ops();
int destroy_infix_ops();
void reset_engine_vars();

gen_expr_t * clone_gen_expr(gen_expr_t * gen_expr);
tree_expr_t * clone_tree_expr(tree_expr_t * tree_expr);
val_expr_t * clone_val_expr(val_expr_t * val_expr);
prefun_expr_t * clone_prefun_expr(prefun_expr_t * prefun_expr);



#endif /** !_EVAL_H */
