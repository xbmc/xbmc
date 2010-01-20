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

#include "wipemalloc.h"

#include "Expr.hpp"
#include <cassert>

#include <iostream>
#include "Eval.hpp"

float GenExpr::eval_gen_expr ( int mesh_i, int mesh_j )
{
	float l;

	assert ( item );
	switch ( this->type )
	{
		case VAL_T:
			return ( ( ValExpr* ) item )->eval_val_expr ( mesh_i, mesh_j );
		case PREFUN_T:
			l = ( ( PrefunExpr * ) item )->eval_prefun_expr ( mesh_i, mesh_j );
			//if (EVAL_DEBUG) DWRITE( "eval_gen_expr: prefix function return value: %f\n", l);
			return l;
		case TREE_T:
			return ( ( TreeExpr* ) ( item ) )->eval_tree_expr ( mesh_i, mesh_j );
		default:
#ifdef EVAL_DEBUG
			DWRITE ( "eval_gen_expr: general expression matched no cases!\n" );
#endif
			return EVAL_ERROR;
	}

}

/* Evaluates functions in prefix form */
float PrefunExpr::eval_prefun_expr ( int mesh_i, int mesh_j )
{


	assert ( func_ptr );

	float * arg_list = new float[this->num_args];

	assert(arg_list);

#ifdef EVAL_DEBUG_DOUBLE
	DWRITE ( "fn[" );
#endif

	//printf("numargs %d", num_args);

	/* Evaluate each argument before calling the function itself */
	for ( int i = 0; i < num_args; i++ )
	{
		arg_list[i] = expr_list[i]->eval_gen_expr ( mesh_i, mesh_j );
#ifdef EVAL_DEBUG_DOUBLE
		if ( i < ( num_args - 1 ) )
			DWRITE ( ", " );
#endif
		//printf("numargs %x", arg_list[i]);


	}
#ifdef EVAL_DEBUG_DOUBLE
	DWRITE ( "]" );
#endif

	/* Now we call the function, passing a list of
	   floats as its argument */

	const float value = ( func_ptr ) ( arg_list );

	delete[](arg_list);
	return value;
}


/* Evaluates a value expression */
float ValExpr::eval_val_expr ( int mesh_i, int mesh_j )
{


	/* Value is a constant, return the float value */
	if ( type == CONSTANT_TERM_T )
	{
#ifdef EVAL_DEBUG
		DWRITE ( "%.4f", term.constant );
#endif
		return ( term.constant );
	}

	/* Value is variable, dereference it */
	if ( type == PARAM_TERM_T )
	{
		switch ( term.param->type )
		{

			case P_TYPE_BOOL:
#ifdef EVAL_DEBUG
				DWRITE ( "(%s:%.4f)", term.param->name.c_str(), ( float ) ( * ( ( bool* ) ( term.param->engine_val ) ) ) );
#endif


				return ( float ) ( * ( ( bool* ) ( term.param->engine_val ) ) );
			case P_TYPE_INT:
#ifdef EVAL_DEBUG
				DWRITE ( "(%s:%.4f)", term.param->name.c_str(), ( float ) ( * ( ( int* ) ( term.param->engine_val ) ) ) );
#endif


				return ( float ) ( * ( ( int* ) ( term.param->engine_val ) ) );
			case P_TYPE_DOUBLE:

				
				if ( term.param->matrix_flag | ( term.param->flags & P_FLAG_ALWAYS_MATRIX ) )
				{

					/* Sanity check the matrix is there... */
					assert ( term.param->matrix != NULL );

					/// @slow boolean check could be expensive in this critical (and common) step of evaluation
					if ( mesh_i >= 0 )
					{
						if ( mesh_j >= 0 )
						{
							return ( ( ( float** ) term.param->matrix ) [mesh_i][mesh_j] );
						}
						else
						{
							return ( ( ( float* ) term.param->matrix ) [mesh_i] );
						}
					}
					//assert(mesh_i >=0);
				}
				//std::cout << term.param->name << ": " << (*((float*)term.param->engine_val)) << std::endl;
				return * ( ( float* ) ( term.param->engine_val ) );			
			default:
				return EVAL_ERROR;
		}
	}
	/* Unknown type, return failure */
	return PROJECTM_FAILURE;
}

/* Evaluates an expression tree */
float TreeExpr::eval_tree_expr ( int mesh_i, int mesh_j )
{

	float left_arg, right_arg;

	/* A leaf node, evaluate the general expression. If the expression is null as well, return zero */
	if ( infix_op == NULL )
	{
		if ( gen_expr == NULL )
			return 0;
		else
			return gen_expr->eval_gen_expr ( mesh_i, mesh_j );
	}

	/* Otherwise, this node is an infix operator. Evaluate
	   accordingly */

#ifdef EVAL_DEBUG
	DWRITE ( "(" );
#endif

	assert(left);
	left_arg = left->eval_tree_expr ( mesh_i, mesh_j );

#ifdef EVAL_DEBUG

	switch ( infix_op->type )
	{
		case INFIX_ADD:
			DWRITE ( "+" );
			break;
		case INFIX_MINUS:
			DWRITE ( "-" );
			break;
		case INFIX_MULT:
			DWRITE ( "*" );
			break;
		case INFIX_MOD:
			DWRITE ( "%%" );
			break;
		case INFIX_OR:
			DWRITE ( "|" );
			break;
		case INFIX_AND:
			DWRITE ( "&" );
			break;
		case INFIX_DIV:
			DWRITE ( "/" );
			break;
		default:
			DWRITE ( "?" );
	}

#endif

	assert(right);
	right_arg = right->eval_tree_expr ( mesh_i, mesh_j );

#ifdef EVAL_DEBUG
	DWRITE ( ")" );
#endif

#ifdef EVAL_DEBUG
	DWRITE ( "\n" );
#endif

	switch ( infix_op->type )
	{
		case INFIX_ADD:
			return ( left_arg + right_arg );
		case INFIX_MINUS:
			return ( left_arg - right_arg );
		case INFIX_MULT:
			return ( left_arg * right_arg );
		case INFIX_MOD:
			if ( ( int ) right_arg == 0 )
			{
#ifdef EVAL_DEBUG
				DWRITE ( "eval_tree_expr: modulo zero!\n" );
#endif
				return PROJECTM_DIV_BY_ZERO;
			}
			return ( ( int ) left_arg % ( int ) right_arg );
		case INFIX_OR:
			return ( ( int ) left_arg | ( int ) right_arg );
		case INFIX_AND:
			return ( ( int ) left_arg & ( int ) right_arg );
		case INFIX_DIV:
			if ( right_arg == 0 )
			{
#ifdef EVAL_DEBUG
				DWRITE ( "eval_tree_expr: division by zero!\n" );
#endif
				return MAX_DOUBLE_SIZE;
			}
			return ( left_arg / right_arg );
		default:
#ifdef EVAL_DEBUG
			DWRITE ( "eval_tree_expr: unknown infix operator!\n" );
#endif
			return EVAL_ERROR;
	}

	return EVAL_ERROR;
}

/* Converts a float value to a general expression */
GenExpr * GenExpr::const_to_expr ( float val )
{

	GenExpr * gen_expr;
	ValExpr * val_expr;
	Term term;

	term.constant = val;

	if ( ( val_expr = new ValExpr ( CONSTANT_TERM_T, &term ) ) == NULL )
		return NULL;

	gen_expr = new GenExpr ( VAL_T, ( void* ) val_expr );

	if ( gen_expr == NULL )
	{
		delete val_expr;
	}

	return gen_expr;
}

/* Converts a regular parameter to an expression */
GenExpr * GenExpr::param_to_expr ( Param * param )
{

	GenExpr * gen_expr = NULL;
	ValExpr * val_expr = NULL;
	Term term;

	if ( param == NULL )
		return NULL;

	/* This code is still a work in progress. We need
	   to figure out if the initial condition is used for
	   each per frame equation or not. I am guessing that
	   it isn't, and it is thusly implemented this way */

	/* Current guess of true behavior (08/01/03) note from carm
	   First try to use the per_pixel_expr (with cloning)
	   If it is null however, use the engine variable instead. */

	/* 08/20/03 : Presets are now objects, as well as per pixel equations. This ends up
	   making the parser handle the case where parameters are essentially per pixel equation
	   substitutions */


	term.param = param;
	if ( ( val_expr = new ValExpr ( PARAM_TERM_T, &term ) ) == NULL )
		return NULL;

	if ( ( gen_expr = new GenExpr ( VAL_T, ( void* ) val_expr ) ) == NULL )
	{
		delete val_expr;
		return NULL;
	}
	return gen_expr;
}

/* Converts a prefix function to an expression */
GenExpr * GenExpr::prefun_to_expr ( float ( *func_ptr ) ( void * ), GenExpr ** expr_list, int num_args )
{

	GenExpr * gen_expr;
	PrefunExpr * prefun_expr;

	prefun_expr = new PrefunExpr();

	if ( prefun_expr == NULL )
		return NULL;

	prefun_expr->num_args = num_args;
	prefun_expr->func_ptr = ( float ( * ) ( void* ) ) func_ptr;
	prefun_expr->expr_list = expr_list;

	gen_expr = new GenExpr ( PREFUN_T, ( void* ) prefun_expr );

	if ( gen_expr == NULL )
		delete prefun_expr;

	return gen_expr;
}

/* Creates a new tree expression */
TreeExpr::TreeExpr ( InfixOp * _infix_op, GenExpr * _gen_expr, TreeExpr * _left, TreeExpr * _right ) :
		infix_op ( _infix_op ), gen_expr ( _gen_expr ),
	left ( _left ), right ( _right ) {}


/* Creates a new value expression */
ValExpr::ValExpr ( int _type, Term * _term ) :type ( _type )
{


	//val_expr->type = _type;
	term.constant = _term->constant;
	term.param = _term->param;

	//return val_expr;
}

/* Creates a new general expression */

GenExpr::GenExpr ( int _type, void * _item ) :type ( _type ), item ( _item ) {}

/* Frees a general expression */
GenExpr::~GenExpr()
{

	switch ( type )
	{
		case VAL_T:
			delete ( ( ValExpr* ) item );
			break;
		case PREFUN_T:
			delete ( ( PrefunExpr* ) item );
			break;
		case TREE_T:
			delete ( ( TreeExpr* ) item );
			break;
	}
}

/* Frees a function in prefix notation */
PrefunExpr::~PrefunExpr()
{

	int i;

	/* Free every element in expression list */
	for ( i = 0 ; i < num_args; i++ )
	{
		delete expr_list[i];
	}
	free ( expr_list );
}

/* Frees values of type VARIABLE and CONSTANT */
ValExpr::~ValExpr()
{}

/* Frees a tree expression */
TreeExpr::~TreeExpr()
{

	/* free left tree */
	if ( left != NULL )
	{
		delete left;
	}

	/* free general expression object */
	if ( gen_expr != NULL )
	{
		delete gen_expr;
	}

	/* Note that infix operators are always
	   stored in memory unless the program
	   exits, so we don't remove them here */

	/* free right tree */
	if ( right != NULL )
	{
		delete right;
	}
}

/* Initializes an infix operator */
DLLEXPORT InfixOp::InfixOp ( int type, int precedence )
{

	this->type = type;
	this->precedence = precedence;
}



PrefunExpr::PrefunExpr() {}
