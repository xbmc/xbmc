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
/* Library functions to manipulate initial condition values */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "fatal.h"

#include "param_types.h"
#include "expr_types.h"
#include "init_cond_types.h"
#include "init_cond.h"
#include "wipemalloc.h"
#include "splaytree_types.h"
#include "splaytree.h"
char init_cond_string_buffer[STRING_BUFFER_SIZE];
int init_cond_string_buffer_index = 0;


void init_cond_to_string(init_cond_t * init_cond);

/* Frees initial condition structure */
void free_init_cond(init_cond_t * init_cond) {
  free(init_cond);
  init_cond = NULL;
}

/* Evaluate an initial conditon */
void eval_init_cond(init_cond_t * init_cond) {

  if (init_cond == NULL)
    return;
 
  /* Parameter is of boolean type, either a 1 or 0 value integer */

  /* Set matrix flag to zero. This ensures
     its constant value will be used rather than a matrix value 
  */
  init_cond->param->matrix_flag = 0;
  if (init_cond->param->type == P_TYPE_BOOL) {
	 if (INIT_COND_DEBUG) printf("init_cond: %s = %d (TYPE BOOL)\n", init_cond->param->name, init_cond->init_val.bool_val); 
	 *((int*)init_cond->param->engine_val) = init_cond->init_val.bool_val;
     return;
  }
  
  /* Parameter is an integer type, just like C */
  
  if (init_cond->param->type == P_TYPE_INT) {
	 if (INIT_COND_DEBUG) printf("init_cond: %s = %d (TYPE INT)\n", init_cond->param->name, init_cond->init_val.int_val);
	 *((int*)init_cond->param->engine_val) = init_cond->init_val.int_val;
     return;
  }

  /* Parameter is of a float type, just like C */

  if (init_cond->param->type == P_TYPE_DOUBLE) {
	if (INIT_COND_DEBUG) {
	    printf( "init_cond: %s = %f (TYPE DOUBLE) -> %f -> %X -> %X\n", init_cond->param->name, 
	            init_cond->init_val.float_val, *((float *)init_cond->param->engine_val),
	            init_cond->param, init_cond->param->engine_val );
	  }
	*((float*)init_cond->param->engine_val) = init_cond->init_val.float_val;
    return;
  }

  /* Unknown type of parameter */
  return;
}

/* Creates a new initial condition */
init_cond_t * new_init_cond(param_t * param, value_t init_val) {

  init_cond_t * init_cond;

  init_cond = (init_cond_t*)wipemalloc(sizeof(init_cond_t));
   
  if (init_cond == NULL)
    return NULL;
 
  init_cond->param = param;
  init_cond->init_val = init_val;

  if ( INIT_COND_DEBUG ) {
    printf( "new_init_cond: %s -> %X -> %X\n", init_cond->param->name, init_cond->param, init_cond->param->engine_val );
  }

  return init_cond;
}

/* WIP */
void init_cond_to_string(init_cond_t * init_cond) {
	
	int string_length;
	char string[MAX_TOKEN_SIZE];
	
	if (init_cond == NULL)
		return;

	/* Create a string "param_name=val" */
	switch (init_cond->param->type) {
		
		case P_TYPE_BOOL:
			sprintf(string, "%s=%d\n", init_cond->param->name, init_cond->init_val.bool_val);
			break; 
		case P_TYPE_INT:
			sprintf(string, "%s=%d\n", init_cond->param->name, init_cond->init_val.int_val);
			break;
		case P_TYPE_DOUBLE:
			sprintf(string, "%s=%f\n", init_cond->param->name, init_cond->init_val.float_val);
			break;
		default:
			return;
	}		
		
	/* Compute the length of the string */
	string_length = strlen(string);
	
	/* Buffer overflow check */
	if ((init_cond_string_buffer_index + string_length + 1)  > (STRING_BUFFER_SIZE - 1))
		return;
	
	/* Copy the string into the initial condition string buffer */
	
	strncpy(init_cond_string_buffer + init_cond_string_buffer_index, string, string_length);
	
	/* Increment the string buffer, offset by one for the null terminator, which will be
	   overwritten by the next call to this function */
	init_cond_string_buffer_index+= string_length + 1;
		
}


char * create_init_cond_string_buffer(splaytree_t * init_cond_tree) {

	if (init_cond_tree == NULL)
		return NULL;
	
	init_cond_string_buffer_index = 0;
	
	splay_traverse((void (*)(void*))init_cond_to_string, init_cond_tree);
	
	return init_cond_string_buffer;
		
}
