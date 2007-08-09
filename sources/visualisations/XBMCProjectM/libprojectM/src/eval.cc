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
/* Evaluation Code */

#include <stdio.h>
#include <stdlib.h>
#include "projectM.h"
#include "common.h"
#include "fatal.h"

#include "param_types.h"
#include "func_types.h"
#include "expr_types.h"
#include "eval.h"
#include "builtin_funcs.h"
#include "wipemalloc.h"
#define EVAL_ERROR -1

/* All infix operators (except '=') are prototyped here */
infix_op_t * infix_add, * infix_minus, * infix_div, * infix_mult,
  * infix_or, * infix_and, * infix_mod, * infix_negative, * infix_positive;
int mesh_i=-1, mesh_j=-1;

static inline float eval_tree_expr(tree_expr_t * tree_expr);
static inline float eval_prefun_expr(prefun_expr_t * prefun_expr);
static inline float eval_val_expr(val_expr_t * val_expr);


float eval_gen_expr(gen_expr_t * gen_expr) {
  float l;

  if (gen_expr == NULL) 
    	return 0;
 	
  switch(gen_expr->type) {
  case VAL_T:  
    return eval_val_expr((val_expr_t*)(gen_expr->item));
  case PREFUN_T:
    l = eval_prefun_expr((prefun_expr_t*)(gen_expr->item));
    //if (EVAL_DEBUG) printf("eval_gen_expr: prefix function return value: %f\n", l);
    return l;		
  case TREE_T:
    return eval_tree_expr((tree_expr_t*)(gen_expr->item));
  default:
    #ifdef EVAL_DEBUG
    printf("eval_gen_expr: general expression matched no cases!\n");
    #endif
    return EVAL_ERROR;
  }  
	
}

/* Evaluates functions in prefix form */
static inline float eval_prefun_expr(prefun_expr_t * prefun_expr) {

	int i;
	float rv;
       
	
	/* This is slightly less than safe, since
	   who knows if the passed argument is valid. For 
	   speed purposes we'll go with this */
	float *arg_list = (float *)wipemalloc( sizeof( float ) * prefun_expr->num_args );
	
	#ifdef EVAL_DEBUG_DOUBLE
		printf("fn[");
		fflush(stdout);
	#endif
	/* Evaluate each argument before calling the function itself */
	for (i = 0; i < prefun_expr->num_args; i++) {
		arg_list[i] = eval_gen_expr(prefun_expr->expr_list[i]);
		#ifdef EVAL_DEBUG_DOUBLE 
			if (i < (prefun_expr->num_args - 1))
				printf(", ");
			fflush(stdout);
		#endif
	}
	
	#ifdef EVAL_DEBUG_DOUBLE 
		printf("]");
		fflush(stdout);
	#endif
	
	/* Now we call the function, passing a list of 	
	   floats as its argument */
     
    rv = (prefun_expr->func_ptr)(arg_list);	
    free( arg_list );
    arg_list = NULL;
    return rv;
}	

/* Evaluates a value expression */
static inline float eval_val_expr(val_expr_t * val_expr) {

  /* Shouldn't happen */
  if (val_expr == NULL)
    return EVAL_ERROR;

  /* Value is a constant, return the float value */
  if (val_expr->type == CONSTANT_TERM_T) {
    #ifdef EVAL_DEBUG 
		printf("%.4f", val_expr->term.constant);
		fflush(stdout);
    #endif
    return (val_expr->term.constant);
  }

  /* Value is variable, dereference it */
  if (val_expr->type == PARAM_TERM_T) {
   	switch (val_expr->term.param->type) {
		
	case P_TYPE_BOOL:
		#ifdef EVAL_DEBUG 
			printf("(%s:%.4f)", val_expr->term.param->name, (float)(*((int*)(val_expr->term.param->engine_val))));
			fflush(stdout);
		#endif
	       
		  
		return (float)(*((int*)(val_expr->term.param->engine_val)));
	case P_TYPE_INT:
		#ifdef EVAL_DEBUG 
			printf("(%s:%.4f)", val_expr->term.param->name, (float)(*((int*)(val_expr->term.param->engine_val))));
			fflush(stdout);
		#endif

	     
		return (float)(*((int*)(val_expr->term.param->engine_val)));
	case P_TYPE_DOUBLE:		
		#ifdef EVAL_DEBUG_DOUBLE 
			printf("(%s:%.4f)", val_expr->term.param->name, (*((float*)val_expr->term.param->engine_val)));
			fflush(stdout);
		#endif

		if (val_expr->term.param->matrix_flag | (val_expr->term.param->flags & P_FLAG_ALWAYS_MATRIX)) {
		/** Sanity check the matrix is there... */
		if ( val_expr->term.param->matrix == NULL ) {
		    printf( "param->matrix == NULL! [%s]\tmesh: %d x %d\n", val_expr->term.param->name, mesh_i, mesh_j );
		    return EVAL_ERROR;
		  }
		  if (mesh_i >= 0) {
			  if (mesh_j >= 0) {
			    return (((float**)val_expr->term.param->matrix)[mesh_i][mesh_j]);
			  } else {
			    return (((float*)val_expr->term.param->matrix)[mesh_i]);
			  }
		  }
		}
		return *((float*)(val_expr->term.param->engine_val));
	default:
	  return EVAL_ERROR;	
    }
  }
  /* Unknown type, return failure */
  return PROJECTM_FAILURE;
}

/* Evaluates an expression tree */
static inline float eval_tree_expr(tree_expr_t * tree_expr) {
		
	float left_arg, right_arg;	
	infix_op_t * infix_op;
	
	/* Shouldn't happen */
	if (tree_expr == NULL)
	  return EVAL_ERROR;

	/* A leaf node, evaluate the general expression. If the expression is null as well, return zero */
	if (tree_expr->infix_op == NULL) {
		if (tree_expr->gen_expr == NULL)
			return 0;
		else
	  		return eval_gen_expr(tree_expr->gen_expr);
	}
	
	/* Otherwise, this node is an infix operator. Evaluate
	   accordingly */
	
	infix_op = (infix_op_t*)tree_expr->infix_op;	
	#ifdef EVAL_DEBUG 
		printf("(");
		fflush(stdout);
	#endif
	
	left_arg = eval_tree_expr(tree_expr->left);

	#ifdef EVAL_DEBUG 
		
		switch (infix_op->type) {
		case INFIX_ADD:
			printf("+");
			break;		
		case INFIX_MINUS:
			printf("-");
			break;
		case INFIX_MULT:
			printf("*");
			break;
		case INFIX_MOD:
			printf("%%");
			break;
		case INFIX_OR:
			printf("|");
			break;
		case INFIX_AND:
			printf("&");
			break;
		case INFIX_DIV:
			printf("/");
			break;
		default:
			printf("?");
		}
	
	fflush(stdout);	
	#endif
	
	right_arg = eval_tree_expr(tree_expr->right);
	
	#ifdef EVAL_DEBUG
		printf(")");
		fflush(stdout);
	#endif
	
	switch (infix_op->type) {		
	case INFIX_ADD:
	  return (left_arg + right_arg);		
	case INFIX_MINUS:
		return (left_arg - right_arg);
	case INFIX_MULT:
		return (left_arg * right_arg);
	case INFIX_MOD:
	  if ((int)right_arg == 0) {
	    #ifdef EVAL_DEBUG 
	    printf("eval_tree_expr: modulo zero!\n");
	    #endif
	    return PROJECTM_DIV_BY_ZERO; 
	  }
	  return ((int)left_arg % (int)right_arg);
	case INFIX_OR:
		return ((int)left_arg | (int)right_arg);
	case INFIX_AND:
		return ((int)left_arg & (int)right_arg);
	case INFIX_DIV:
	  if (right_arg == 0) {
	    #ifdef EVAL_DEBUG 
	    printf("eval_tree_expr: division by zero!\n");
	    #endif
	    return MAX_DOUBLE_SIZE;
	  }		
	  return (left_arg / right_arg);
	default:
          #ifdef EVAL_DEBUG 
	    printf("eval_tree_expr: unknown infix operator!\n");
          #endif
		return EVAL_ERROR;
	}
	
	return EVAL_ERROR;
}	

/* Converts a float value to a general expression */
gen_expr_t * const_to_expr(float val) {

  gen_expr_t * gen_expr;
  val_expr_t * val_expr;
  term_t term;
  
  term.constant = val;
    
  if ((val_expr = new_val_expr(CONSTANT_TERM_T, term)) == NULL)
    return NULL;

  gen_expr = new_gen_expr(VAL_T, (void*)val_expr);

  if (gen_expr == NULL) {
	free_val_expr(val_expr);
  }
  
  return gen_expr;
}

/* Converts a regular parameter to an expression */
gen_expr_t * param_to_expr(param_t * param) {

  gen_expr_t * gen_expr = NULL;
  val_expr_t * val_expr = NULL;
  term_t term;

  if (param == NULL)
    return NULL;
 
  /* This code is still a work in progress. We need
     to figure out if the initial condition is used for 
     each per frame equation or not. I am guessing that
     it isn't, and it is thusly implemented this way */
  
  /* Current guess of true behavior (08/01/03) note from carm
     First try to use the per_pixel_expr (with cloning). 
     If it is null however, use the engine variable instead. */
  
  /* 08/20/03 : Presets are now objects, as well as per pixel equations. This ends up
     making the parser handle the case where parameters are essentially per pixel equation
     substitutions */
       
  
  term.param = param;
  if ((val_expr = new_val_expr(PARAM_TERM_T, term)) == NULL)
    return NULL;
  
  if ((gen_expr = new_gen_expr(VAL_T, (void*)val_expr)) == NULL) {
    free_val_expr(val_expr);
	return NULL;	  
  } 
  return gen_expr;
}

/* Converts a prefix function to an expression */
gen_expr_t * prefun_to_expr(float (*func_ptr)(), gen_expr_t ** expr_list, int num_args) {

  gen_expr_t * gen_expr;
  prefun_expr_t * prefun_expr;
  

  /* Malloc a new prefix function expression */
  prefun_expr = (prefun_expr_t*)wipemalloc(sizeof(prefun_expr_t));

  if (prefun_expr == NULL)
	  return NULL;
  
  prefun_expr->num_args = num_args;
  prefun_expr->func_ptr =(float (*)(void*)) func_ptr;
  prefun_expr->expr_list = expr_list;

  gen_expr = new_gen_expr(PREFUN_T, (void*)prefun_expr);

  if (gen_expr == NULL)
	  free_prefun_expr(prefun_expr);
  
  return gen_expr;
}

/* Creates a new tree expression */
tree_expr_t * new_tree_expr(infix_op_t * infix_op, gen_expr_t * gen_expr, tree_expr_t * left, tree_expr_t * right) {

		tree_expr_t * tree_expr;
		tree_expr = (tree_expr_t*)wipemalloc(sizeof(tree_expr_t));
	
		if (tree_expr == NULL)
			return NULL;
		tree_expr->infix_op = infix_op;
		tree_expr->gen_expr = gen_expr;
		tree_expr->left = left;
		tree_expr->right = right;
		return tree_expr;
}


/* Creates a new value expression */
val_expr_t * new_val_expr(int type, term_t term) {

  val_expr_t * val_expr;
  val_expr = (val_expr_t*)wipemalloc(sizeof(val_expr_t));

  if (val_expr == NULL)
    return NULL;

  val_expr->type = type;
  val_expr->term = term;

  return val_expr;
}

/* Creates a new general expression */
gen_expr_t * new_gen_expr(int type, void * item) {

	gen_expr_t * gen_expr;

	gen_expr = (gen_expr_t*)wipemalloc(sizeof(gen_expr_t));
	if (gen_expr == NULL)
		return NULL;
	gen_expr->type = type;
	gen_expr->item = item;	

	return gen_expr;
}

/* Frees a general expression */
int free_gen_expr(gen_expr_t * gen_expr) {

	if (gen_expr == NULL)
	 	return PROJECTM_SUCCESS;
	
	switch (gen_expr->type) {
	case VAL_T:
		free_val_expr((val_expr_t*)(gen_expr->item));
		break;
	case PREFUN_T:
		free_prefun_expr((prefun_expr_t*)(gen_expr->item));
		break;
	case TREE_T:
		free_tree_expr((tree_expr_t*)(gen_expr->item));
		break;
	default:
		return PROJECTM_FAILURE;
	}	

	free(gen_expr);
        gen_expr = NULL;
	return PROJECTM_SUCCESS;

}


/* Frees a function in prefix notation */
int free_prefun_expr(prefun_expr_t * prefun_expr) {

	int i;
	if (prefun_expr == NULL)
		return PROJECTM_SUCCESS;
	
	/* Free every element in expression list */
	for (i = 0 ; i < prefun_expr->num_args; i++) {
		free_gen_expr(prefun_expr->expr_list[i]);
	}
        free(prefun_expr->expr_list);
	free(prefun_expr);
	prefun_expr = NULL;
	return PROJECTM_SUCCESS;
}

/* Frees values of type VARIABLE and CONSTANT */
int free_val_expr(val_expr_t * val_expr) {

	if (val_expr == NULL)
		return PROJECTM_SUCCESS;	
	
	free(val_expr);
	val_expr = NULL;
	return PROJECTM_SUCCESS;
}

/* Frees a tree expression */
int free_tree_expr(tree_expr_t * tree_expr) {

	if (tree_expr == NULL)
		return PROJECTM_SUCCESS;
	
	/* free left tree */
	free_tree_expr(tree_expr->left);
	
	/* free general expression object */
	free_gen_expr(tree_expr->gen_expr);
	
	/* Note that infix operators are always
	   stored in memory unless the program 
	   exits, so we don't remove them here */
	
	/* free right tree */
	free_tree_expr(tree_expr->right);
	
	
	/* finally, free the struct itself */
	free(tree_expr);
	tree_expr = NULL;
	return PROJECTM_SUCCESS;
}



/* Initializes all infix operators */
int init_infix_ops() {

	infix_add = new_infix_op(INFIX_ADD, 4);
	infix_minus = new_infix_op(INFIX_MINUS, 3);
	infix_div = new_infix_op(INFIX_DIV, 2);
	infix_or = new_infix_op(INFIX_OR, 5);
	infix_and = new_infix_op(INFIX_AND,4);
	infix_mod = new_infix_op(INFIX_MOD, 1);
	infix_mult = new_infix_op(INFIX_MULT, 2);
	
	/* Prefix operators */
	infix_positive = new_infix_op(INFIX_ADD, 0);
	infix_negative = new_infix_op(INFIX_MINUS, 0);

	return PROJECTM_SUCCESS;
}

/* Destroys the infix operator list. This should
   be done on program exit */
int destroy_infix_ops()
{

  free(infix_add);
  free(infix_minus);
  free(infix_div);
  free(infix_or);
  free(infix_and);
  free(infix_mod);
  free(infix_mult);
  free(infix_positive);
  free(infix_negative);

  infix_add = NULL;
  infix_minus = NULL;
  infix_div = NULL;
  infix_or = NULL;
  infix_and = NULL;
  infix_mod = NULL;
  infix_mult = NULL;
  infix_positive = NULL;
  infix_negative = NULL;

  return PROJECTM_SUCCESS;
}

/* Initializes an infix operator */
infix_op_t * new_infix_op(int type, int precedence) {

	infix_op_t * infix_op;
	
	infix_op = (infix_op_t*)wipemalloc(sizeof(infix_op_t));
	
	if (infix_op == NULL)
		return NULL;
	
	infix_op->type = type;
	infix_op->precedence = precedence;
	
	return infix_op;
}




/* Clones a general expression */
gen_expr_t * clone_gen_expr(gen_expr_t * gen_expr) {

  gen_expr_t * new_gen_expr;
  val_expr_t * val_expr;
  tree_expr_t * tree_expr;
  prefun_expr_t * prefun_expr;

  /* Null argument check */
  if (gen_expr == NULL)
    return NULL;

  /* Out of memory */
  if ((new_gen_expr = (gen_expr_t*)wipemalloc(sizeof(gen_expr_t))) == NULL)
    return NULL;

  /* Case on the type of general expression */
  switch (new_gen_expr->type = gen_expr->type) {

  case VAL_T: /* val expression */
    if ((val_expr = clone_val_expr((val_expr_t*)gen_expr->item)) == NULL) {
      free(new_gen_expr);
      new_gen_expr = NULL;
      return NULL;
    }
    new_gen_expr->item = (void*)val_expr;
    break;
    
  case PREFUN_T: /* prefix function expression */
    if ((prefun_expr = clone_prefun_expr((prefun_expr_t*)gen_expr->item)) == NULL) {
      free(new_gen_expr);
      new_gen_expr = NULL;
      return NULL;
    }
    new_gen_expr->item = (void*)prefun_expr;
    break;
    
  case TREE_T:  /* tree expression */
    if ((tree_expr = clone_tree_expr((tree_expr_t*)gen_expr->item)) == NULL) {
      free(new_gen_expr);
      new_gen_expr = NULL;
      return NULL;
    }
    new_gen_expr->item = (void*)tree_expr;
    break;
    
  default: /* unknown type, ut oh.. */
    free(new_gen_expr);
      new_gen_expr = NULL;
    return NULL;
  }
  
  return new_gen_expr; /* Return the new (cloned) general expression */
}


/* Clones a tree expression */
tree_expr_t * clone_tree_expr(tree_expr_t * tree_expr) {

  tree_expr_t * new_tree_expr;

  /* Null argument */
  if (tree_expr == NULL)
    return NULL;
  
  /* Out of memory */
  if ((new_tree_expr = (tree_expr_t*)wipemalloc(sizeof(tree_expr_t))) == NULL) 
    return NULL;
  
  /* Set each argument in tree_expr_t struct */
  new_tree_expr->infix_op = tree_expr->infix_op;  /* infix operators are in shared memory */
  new_tree_expr->gen_expr = clone_gen_expr(tree_expr->gen_expr); /* clone the general expression */
  new_tree_expr->left = clone_tree_expr(tree_expr->left); /* clone the left tree expression */
  new_tree_expr->right = clone_tree_expr(tree_expr->right); /* clone the right tree expression */

  return new_tree_expr; /* Return the new (cloned) tree expression */
}

/* Clones a value expression, currently only passes the pointer to 
   the value that this object represents, not a pointer to a copy of the value */
val_expr_t * clone_val_expr(val_expr_t * val_expr) {

  val_expr_t * new_val_expr;

  /* Null argument */
  if (val_expr == NULL)
    return NULL;
  
  /* Allocate space, check for out of memory */
  if ((new_val_expr = (val_expr_t*)wipemalloc(sizeof(val_expr_t))) == NULL) 
    return NULL;

  /* Set the values in the val_expr_t struct */
  new_val_expr->type = val_expr->type;
  new_val_expr->term = val_expr->term;
  
  /* Return the new (cloned) value expression */
  return new_val_expr;
}

/* Clones a prefix function with its arguments */
prefun_expr_t * clone_prefun_expr(prefun_expr_t * prefun_expr) {

  int i;
  prefun_expr_t * new_prefun_expr;
  
  /* Null argument */
  if (prefun_expr == NULL)
    return NULL;
  
  /* Out of memory */
  if ((new_prefun_expr = (prefun_expr_t*)wipemalloc(sizeof(prefun_expr_t))) == NULL) 
    return NULL;
  
  /* Set the function argument paired with its number of arguments */
  new_prefun_expr->num_args = prefun_expr->num_args;
  new_prefun_expr->func_ptr = prefun_expr->func_ptr;

  /* Allocate space for the expression list pointers */
  if ((new_prefun_expr->expr_list = (gen_expr_t**)wipemalloc(sizeof(gen_expr_t*)*new_prefun_expr->num_args)) == NULL) {
    free(new_prefun_expr);
    new_prefun_expr = NULL;
    return NULL;
  }

  /* Now copy each general expression from the argument expression list */
  for (i = 0; i < new_prefun_expr->num_args;i++) 
    new_prefun_expr->expr_list[i] = clone_gen_expr(prefun_expr->expr_list[i]);
  
  /* Finally, return the new (cloned) prefix function expression */
  return new_prefun_expr;
}
