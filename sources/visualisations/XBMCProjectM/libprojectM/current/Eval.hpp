/**
 * projectM -- Milkdrop-esque visualisation SDK
 * Copyright (C)2003-2007 projectM Team
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
/**
 * $Id$
 *
 * Expression evaluators
 *
 * $Log$
 */

/* Eval.hpp: evaluation functions of expressions */

#ifndef __EVAL_H
#define __EVAL_H

#include "fatal.h"
//#include "projectM.hpp"
#include "Func.hpp"
#include "Param.hpp"

//#define EVAL_DEBUG 2
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

class InfixOp;

class Eval {
public:
    static InfixOp *infix_add,
                   *infix_minus,
                   *infix_div,
                   *infix_mult,
                   *infix_or,
                   *infix_and,
                   *infix_mod,
                   *infix_negative,
                   *infix_positive;

    float eval_gen_expr(GenExpr * gen_expr);
    inline GenExpr * opt_gen_expr(GenExpr * gen_expr, int ** param_list);

    GenExpr * const_to_expr(float val);
    GenExpr * param_to_expr(Param * param);
    GenExpr * prefun_to_expr(float (*func_ptr)(), GenExpr ** expr_list, int num_args);

    static TreeExpr * new_tree_expr(InfixOp * infix_op, GenExpr * gen_expr, TreeExpr * left, TreeExpr * right);
    static GenExpr * new_gen_expr(int type, void * item);
    static ValExpr * new_val_expr(int type, Term *term);

    static InfixOp * new_infix_op(int type, int precedence);
    static int init_infix_ops();
    static int destroy_infix_ops();
    void reset_engine_vars();
    
    GenExpr * clone_gen_expr(GenExpr * gen_expr);
    TreeExpr * clone_tree_expr(TreeExpr * tree_expr);
    ValExpr * clone_val_expr(ValExpr * val_expr);
    PrefunExpr * clone_prefun_expr(PrefunExpr * prefun_expr);
  };

#endif /** !_EVAL_H */
