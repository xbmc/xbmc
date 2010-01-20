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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fatal.h"
#include "Common.hpp"

#include "Param.hpp"
#include "PerFrameEqn.hpp"

#include "Eval.hpp"
#include "Expr.hpp"

#include "wipemalloc.h"
#include <cassert>

/* Evaluate an equation */
void PerFrameEqn::evaluate() {

     if (PER_FRAME_EQN_DEBUG) { 
		 printf("per_frame_%d=%s= ", index, param->name.c_str());
		 fflush(stdout); 
     }
	 
    //*((float*)per_frame_eqn->param->engine_val) = eval_gen_expr(per_frame_eqn->gen_expr);
	assert(gen_expr);
	assert(param);
	param->set_param(gen_expr->eval_gen_expr(-1,-1));

     if (PER_FRAME_EQN_DEBUG) printf(" = %.4f\n", *((float*)param->engine_val)); 
		 
}


/* Frees perframe equation structure. Warning: assumes gen_expr pointer is not freed by anyone else! */
PerFrameEqn::~PerFrameEqn() {

  delete gen_expr;

  // param is freed in param_tree container of some other class

}

/* Create a new per frame equation */
PerFrameEqn::PerFrameEqn(int _index, Param * _param, GenExpr * _gen_expr) :
	index(_index), param(_param), gen_expr(_gen_expr) {}
