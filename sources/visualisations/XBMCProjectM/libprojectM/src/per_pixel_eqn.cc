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
#include <string.h>
#include <stdlib.h>

#include "projectM.h"

#include "fatal.h"
#include "common.h"

#include "expr_types.h"
#include "eval.h"

#include "splaytree_types.h"
#include "splaytree.h"

#include "param_types.h"
#include "param.h"

#include "per_pixel_eqn.h"
#include "per_pixel_eqn_types.h"

#include "wipemalloc.h"

extern preset_t *active_preset;
extern projectM_t *PM;
extern int mesh_i;
extern int mesh_j;

/* Evaluates a per pixel equation */
void evalPerPixelEqn( per_pixel_eqn_t * per_pixel_eqn) {

  float ** param_matrix = NULL;
  gen_expr_t * eqn_ptr = NULL;
  int x,y;

  eqn_ptr = per_pixel_eqn->gen_expr; 
 if (per_pixel_eqn->param->matrix == NULL) {
    if (PER_PIXEL_EQN_DEBUG) printf("evalPerPixelEqn: [begin initializing matrix] (index = %d) (name = %s)\n", 
  			  per_pixel_eqn->index, per_pixel_eqn->param->name);
    
    param_matrix = (float**)wipemalloc(PM->gx*sizeof(float*));
    per_pixel_eqn->param->matrix = param_matrix;

    for(x = 0; x < PM->gx; x++)
      param_matrix[x] = (float *)wipemalloc(PM->gy * sizeof(float));

    for (x = 0; x < PM->gx; x++)
      for (y = 0; y < PM->gy; y++)
	    param_matrix[x][y] = 0.0;

    if (per_pixel_eqn->param->name == NULL)
      printf("null parameter?\n");

    //    printf("PARAM MATRIX: \"%s\" initialized.\n", per_pixel_eqn->param->name);
  }
  else 
    param_matrix = (float**)per_pixel_eqn->param->matrix;
 
  if (eqn_ptr == NULL || param_matrix == NULL )
    printf("something is seriously wrong...\n");
  for (mesh_i = 0; mesh_i < PM->gx; mesh_i++) {    
    for (mesh_j = 0; mesh_j < PM->gy; mesh_j++) {     
      param_matrix[mesh_i][mesh_j] = eval_gen_expr(eqn_ptr);
    }
  }
  
  /* Now that this parameter has been referenced with a per
     pixel equation, we let the evaluator know by setting
     this flag */
  per_pixel_eqn->param->matrix_flag = 1; 
   per_pixel_eqn->param->flags |= P_FLAG_PER_PIXEL;
}

 void evalPerPixelEqns(preset_t *preset) {

  /* Evaluate all per pixel equations using splay traversal */
  splay_traverse((void (*)(void*))evalPerPixelEqn, preset->per_pixel_eqn_tree);

  /* Set mesh i / j values to -1 so engine vars are used by default again */
  mesh_i = mesh_j = -1;

}
/* Adds a per pixel equation according to its string name. This
   will be used only by the parser */

int add_per_pixel_eqn(char * name, gen_expr_t * gen_expr, preset_t * preset) {

  per_pixel_eqn_t * per_pixel_eqn;
  int index;
  param_t * param = NULL;

  /* Argument checks */
  if (preset == NULL)
	  return PROJECTM_FAILURE;
  if (gen_expr == NULL)
	  return PROJECTM_FAILURE;
  if (name == NULL)
	  return PROJECTM_FAILURE;
  
 if (PER_PIXEL_EQN_DEBUG) printf("add_per_pixel_eqn: per pixel equation (name = \"%s\")\n", name);
 
 if (!strncmp(name, "dx", strlen("dx"))) 
   preset->per_pixel_flag[DX_OP] = TRUE;
 else if (!strncmp(name, "dy", strlen("dy"))) 
   preset->per_pixel_flag[DY_OP] = TRUE;
 else if (!strncmp(name, "cx", strlen("cx"))) 
   preset->per_pixel_flag[CX_OP] = TRUE;
 else if (!strncmp(name, "cy", strlen("cy"))) 
   preset->per_pixel_flag[CX_OP] = TRUE;
 else if (!strncmp(name, "zoom", strlen("zoom"))) 
   preset->per_pixel_flag[ZOOM_OP] = TRUE;
 else if (!strncmp(name, "zoomexp", strlen("zoomexp"))) 
   preset->per_pixel_flag[ZOOMEXP_OP] = TRUE;
 else if (!strncmp(name, "rot", strlen("rot")))
   preset->per_pixel_flag[ROT_OP] = TRUE;
 else if (!strncmp(name, "sx", strlen("sx")))
   preset->per_pixel_flag[SX_OP] = TRUE;
 else if (!strncmp(name, "sy", strlen("sy")))
   preset->per_pixel_flag[SY_OP] = TRUE;
 

 /* Search for the parameter so we know what matrix the per pixel equation is referencing */

 param = find_param(name, preset, TRUE);
 if ( !param ) {
   if (PER_PIXEL_EQN_DEBUG) printf("add_per_pixel_eqn: failed to allocate a new parameter!\n");
   return PROJECTM_FAILURE;
 } 	 

/**
 if ( !param->matrix ) {
    if (PER_PIXEL_EQN_DEBUG) printf( "add_per_pixel_eqn: failed to locate param matrix\n" );
    return PROJECTM_FAILURE;
  }
*/

 /* Find most largest index in the splaytree */
 // if ((per_pixel_eqn = splay_find_max(PM->active_preset->per_pixel_eqn_tree)) == NULL)
 // index = 0;
 // else
 index = splay_size(preset->per_pixel_eqn_tree);
   
 /* Create the per pixel equation given the index, parameter, and general expression */
 if ((per_pixel_eqn = new_per_pixel_eqn(index, param, gen_expr)) == NULL) {
   if (PER_PIXEL_EQN_DEBUG) printf("add_per_pixel_eqn: failed to create new per pixel equation!\n");
   return PROJECTM_FAILURE;

 }

 if (PER_PIXEL_EQN_DEBUG) printf("add_per_pixel_eqn: new equation (index = %d,matrix=%X) (param = \"%s\")\n", 
				 per_pixel_eqn->index, per_pixel_eqn->param->matrix, per_pixel_eqn->param->name);
 /* Insert the per pixel equation into the preset per pixel database */
 if (splay_insert(per_pixel_eqn, &per_pixel_eqn->index, preset->per_pixel_eqn_tree) < 0) {
   free_per_pixel_eqn(per_pixel_eqn);
   printf("failed to add per pixel eqn!\n");
   return PROJECTM_FAILURE;	 
 }

 /* Done */ 
 return PROJECTM_SUCCESS;
}

per_pixel_eqn_t * new_per_pixel_eqn(int index, param_t * param, gen_expr_t * gen_expr) {

	per_pixel_eqn_t * per_pixel_eqn;
	
	if (index < 0)
	  return NULL;
	if (param == NULL)
	  return NULL;
	if (gen_expr == NULL)
	  return NULL;
	
	if ((per_pixel_eqn = (per_pixel_eqn_t*)wipemalloc(sizeof(per_pixel_eqn_t))) == NULL)
	  return NULL;

	
	per_pixel_eqn->index = index;
	per_pixel_eqn->param = param;
	per_pixel_eqn->gen_expr = gen_expr;
	
	return per_pixel_eqn;	
}


void free_per_pixel_eqn(per_pixel_eqn_t * per_pixel_eqn) {

	if (per_pixel_eqn == NULL)
		return;
	
	free_gen_expr(per_pixel_eqn->gen_expr);
	
	free(per_pixel_eqn);
	per_pixel_eqn = NULL;
	
	return;
}

int isPerPixelEqn(int op) {
    
  return active_preset->per_pixel_flag[op];

}

 int resetPerPixelEqnFlags(preset_t * preset) {
  int i;

  for (i = 0; i < NUM_OPS;i++)
    preset->per_pixel_flag[i] = FALSE;

  return PROJECTM_SUCCESS;
}
