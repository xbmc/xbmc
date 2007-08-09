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

#include "common.h"
#include "fatal.h"

#include "param_types.h"
#include "param.h"

#include "expr_types.h"
#include "eval.h"

#include "splaytree_types.h"
#include "splaytree.h"
#include "tree_types.h"

#include "per_frame_eqn_types.h"
#include "per_frame_eqn.h"

#include "init_cond_types.h"
#include "init_cond.h"

#include "preset_types.h"

#include "custom_shape_types.h"
#include "custom_shape.h"

#include "init_cond_types.h"
#include "init_cond.h"

#include "wipemalloc.h"

custom_shape_t * interface_shape = NULL;
int cwave_interface_id = 0;
void eval_custom_shape_init_conds(custom_shape_t * custom_shape);
void load_unspec_init_cond_shape(param_t * param);

void destroy_param_db_tree_shape(splaytree_t * tree);
void destroy_per_frame_eqn_tree_shape(splaytree_t * tree);
void destroy_per_frame_init_eqn_tree_shape(splaytree_t * tree);
void destroy_init_cond_tree_shape(splaytree_t * tree);

custom_shape_t * new_custom_shape(int id) {

  custom_shape_t * custom_shape;
  param_t * param;

  if ((custom_shape = (custom_shape_t*)wipemalloc(sizeof(custom_shape_t))) == NULL)
    return NULL;

  custom_shape->id = id;
  custom_shape->per_frame_count = 0;
  custom_shape->per_frame_eqn_string_index = 0;
  custom_shape->per_frame_init_eqn_string_index = 0;

  /* Initialize tree data structures */

  if ((custom_shape->param_tree = 
       create_splaytree( (int (*)(void*,void*))compare_string, (void* (*)(void*)) copy_string,(void (*)(void*)) free_string)) == NULL) {
    free_custom_shape(custom_shape);
    return NULL;
  }
  
  if ((custom_shape->per_frame_eqn_tree = 
       create_splaytree((int (*)(void*, void*))compare_int, (void* (*)(void*))copy_int,(void (*)(void*)) free_int)) == NULL) {
    free_custom_shape(custom_shape);
    return NULL;
  }

  if ((custom_shape->init_cond_tree = 
       create_splaytree((int (*)(void*, void*))compare_string, (void* (*)(void*)) copy_string,(void (*)(void*)) free_string)) == NULL) {
    free_custom_shape(custom_shape);
    return NULL;
  }
  
  if ((custom_shape->per_frame_init_eqn_tree = 
       create_splaytree((int (*)(void*, void*)) compare_string, (void* (*)(void*))copy_string, (void (*)(void*))free_string)) == NULL) {
    free_custom_shape(custom_shape);
    return NULL;
  }

  /* Start: Load custom shape parameters */

  if ((param = new_param_float("r", P_FLAG_NONE, &custom_shape->r, NULL, 1.0, 0.0, .5)) == NULL) {
    free_custom_shape(custom_shape);
    return NULL;
  }

  if (insert_param(param, custom_shape->param_tree) < 0) {
    free_custom_shape(custom_shape);
    return NULL;
  }
 
  if ((param = new_param_float("g", P_FLAG_NONE, &custom_shape->g, NULL, 1.0, 0.0, .5)) == NULL){
    free_custom_shape(custom_shape);
    return NULL;
  }

  if (insert_param(param, custom_shape->param_tree) < 0) {
    free_custom_shape(custom_shape);
    return NULL;
  }

  if ((param = new_param_float("b", P_FLAG_NONE, &custom_shape->b, NULL, 1.0, 0.0, .5)) == NULL){
    free_custom_shape(custom_shape);
    return NULL;				       
  }

  if (insert_param(param, custom_shape->param_tree) < 0) {
    free_custom_shape(custom_shape);
    return NULL;
  }

  if ((param = new_param_float("a", P_FLAG_NONE, &custom_shape->a, NULL, 1.0, 0.0, .5)) == NULL){
    free_custom_shape(custom_shape);
    return NULL;
  }

  if (insert_param(param, custom_shape->param_tree) < 0) {
    free_custom_shape(custom_shape);
    return NULL;
  }

  if ((param = new_param_float("border_r", P_FLAG_NONE, &custom_shape->border_r, NULL, 1.0, 0.0, .5)) == NULL) {
    free_custom_shape(custom_shape);
    return NULL;
  }

  if (insert_param(param, custom_shape->param_tree) < 0) {
    free_custom_shape(custom_shape);
    return NULL;
  }
 
  if ((param = new_param_float("border_g", P_FLAG_NONE, &custom_shape->border_g, NULL, 1.0, 0.0, .5)) == NULL){
    free_custom_shape(custom_shape);
    return NULL;
  }

  if (insert_param(param, custom_shape->param_tree) < 0) {
    free_custom_shape(custom_shape);
    return NULL;
  }

  if ((param = new_param_float("border_b", P_FLAG_NONE, &custom_shape->border_b, NULL, 1.0, 0.0, .5)) == NULL){
    free_custom_shape(custom_shape);
    return NULL;				       
  }

  if (insert_param(param, custom_shape->param_tree) < 0) {
    free_custom_shape(custom_shape);
    return NULL;
  }

  if ((param = new_param_float("border_a", P_FLAG_NONE, &custom_shape->border_a, NULL, 1.0, 0.0, .5)) == NULL){
    free_custom_shape(custom_shape);
    return NULL;
  }

  if (insert_param(param, custom_shape->param_tree) < 0) {
    free_custom_shape(custom_shape);
    return NULL;
  }

  if ((param = new_param_float("r2", P_FLAG_NONE, &custom_shape->r2, NULL, 1.0, 0.0, .5)) == NULL) {
    free_custom_shape(custom_shape);
    return NULL;
  }

  if (insert_param(param, custom_shape->param_tree) < 0) {
    free_custom_shape(custom_shape);
    return NULL;
  }
 
  if ((param = new_param_float("g2", P_FLAG_NONE, &custom_shape->g2, NULL, 1.0, 0.0, .5)) == NULL){
    free_custom_shape(custom_shape);
    return NULL;
  }

  if (insert_param(param, custom_shape->param_tree) < 0) {
    free_custom_shape(custom_shape);
    return NULL;
  }

  if ((param = new_param_float("b2", P_FLAG_NONE, &custom_shape->b2, NULL, 1.0, 0.0, .5)) == NULL){
    free_custom_shape(custom_shape);
    return NULL;				       
  }

  if (insert_param(param, custom_shape->param_tree) < 0) {
    free_custom_shape(custom_shape);
    return NULL;
  }

  if ((param = new_param_float("a2", P_FLAG_NONE, &custom_shape->a2, NULL, 1.0, 0.0, .5)) == NULL){
    free_custom_shape(custom_shape);
    return NULL;
  }
  
  if (insert_param(param, custom_shape->param_tree) < 0) {
    free_custom_shape(custom_shape);
    return NULL;
  }

  if ((param = new_param_float("x", P_FLAG_NONE, &custom_shape->x, NULL, 1.0, 0.0, .5)) == NULL) {
    free_custom_shape(custom_shape);
    return NULL;
  }

  if (insert_param(param, custom_shape->param_tree) < 0) {
    free_custom_shape(custom_shape);
    return NULL;
  }

  if ((param = new_param_float("y", P_FLAG_NONE, &custom_shape->y, NULL, 1.0, 0.0, .5)) == NULL) {
    free_custom_shape(custom_shape);
    return NULL;
  }

  if (insert_param(param, custom_shape->param_tree) < 0) {
    free_custom_shape(custom_shape);
    return NULL;
  }

  if ((param = new_param_bool("thickOutline", P_FLAG_NONE, &custom_shape->thickOutline, 1, 0, 0)) == NULL) {
    free_custom_shape(custom_shape);
    return NULL;
  }

  if (insert_param(param, custom_shape->param_tree) < 0) {
    free_custom_shape(custom_shape);
    return NULL;
  }

  if ((param = new_param_bool("enabled", P_FLAG_NONE, &custom_shape->enabled, 1, 0, 0)) == NULL) {
    free_custom_shape(custom_shape);
    return NULL;
  }

  if (insert_param(param, custom_shape->param_tree) < 0) {
    free_custom_shape(custom_shape);
    return NULL;
  }

  if ((param = new_param_int("sides", P_FLAG_NONE, &custom_shape->sides, 100, 3, 3)) == NULL) {
    free_custom_shape(custom_shape);
    return NULL;
  }

  if (insert_param(param, custom_shape->param_tree) < 0) {
    free_custom_shape(custom_shape);
    return NULL;
  }

  if ((param = new_param_bool("additive", P_FLAG_NONE, &custom_shape->additive, 1, 0, 0)) == NULL) {
    free_custom_shape(custom_shape);
    return NULL;
  }

  if (insert_param(param, custom_shape->param_tree) < 0) {
    free_custom_shape(custom_shape);
    return NULL;
  }

  if ((param = new_param_bool("textured", P_FLAG_NONE, &custom_shape->textured, 1, 0, 0)) == NULL) {
    free_custom_shape(custom_shape);
    return NULL;
  }

  if (insert_param(param, custom_shape->param_tree) < 0) {
    free_custom_shape(custom_shape);
    return NULL;
  }

   if ((param = new_param_float("rad", P_FLAG_NONE, &custom_shape->radius, NULL, MAX_DOUBLE_SIZE, 0, 0.0)) == NULL) {
    free_custom_shape(custom_shape);
    return NULL;
  }

   if (insert_param(param, custom_shape->param_tree) < 0) {
    free_custom_shape(custom_shape);
    return NULL;
  }

   if ((param = new_param_float("ang", P_FLAG_NONE, &custom_shape->ang, NULL, MAX_DOUBLE_SIZE, -MAX_DOUBLE_SIZE, 0.0)) == NULL) {
    free_custom_shape(custom_shape);
    return NULL;
  }

   if (insert_param(param, custom_shape->param_tree) < 0) {
    free_custom_shape(custom_shape);
    return NULL;
  }

   if ((param = new_param_float("tex_zoom", P_FLAG_NONE, &custom_shape->tex_zoom, NULL, MAX_DOUBLE_SIZE, .00000000001, 0.0)) == NULL) {
    free_custom_shape(custom_shape);
    return NULL;
  }

   if (insert_param(param, custom_shape->param_tree) < 0) {
    free_custom_shape(custom_shape);
    return NULL;
  }
   
   if ((param = new_param_float("tex_ang", P_FLAG_NONE, &custom_shape->tex_ang, NULL, MAX_DOUBLE_SIZE, -MAX_DOUBLE_SIZE, 0.0)) == NULL) {
    free_custom_shape(custom_shape);
    return NULL;
  }

   if (insert_param(param, custom_shape->param_tree) < 0) {
    free_custom_shape(custom_shape);
    return NULL;
  }
   if ((param = new_param_float("t1", P_FLAG_TVAR, &custom_shape->t1, NULL, MAX_DOUBLE_SIZE, -MAX_DOUBLE_SIZE, 0.0)) == NULL) {
    free_custom_shape(custom_shape);
    return NULL;
  }

  if (insert_param(param, custom_shape->param_tree) < 0) {
    free_custom_shape(custom_shape);
    return NULL;
  }

  if ((param = new_param_float("t2", P_FLAG_TVAR, &custom_shape->t2, NULL, MAX_DOUBLE_SIZE, -MAX_DOUBLE_SIZE, 0.0)) == NULL) {
    free_custom_shape(custom_shape);
    return NULL;
  }

  if (insert_param(param, custom_shape->param_tree) < 0) {
    free_custom_shape(custom_shape);
    return NULL;
  }

  if ((param = new_param_float("t3", P_FLAG_TVAR, &custom_shape->t3, NULL, MAX_DOUBLE_SIZE, -MAX_DOUBLE_SIZE, 0.0)) == NULL) {
    free_custom_shape(custom_shape);
    return NULL;
  }

  if (insert_param(param, custom_shape->param_tree) < 0) {
    free_custom_shape(custom_shape);
    return NULL;
  }
  if ((param = new_param_float("t4", P_FLAG_TVAR, &custom_shape->t4, NULL, MAX_DOUBLE_SIZE, -MAX_DOUBLE_SIZE, 0.0)) == NULL) {
    free_custom_shape(custom_shape);
    return NULL;
  }

  if (insert_param(param, custom_shape->param_tree) < 0) {
    free_custom_shape(custom_shape);
    return NULL;
  }
  if ((param = new_param_float("t5", P_FLAG_TVAR, &custom_shape->t5, NULL, MAX_DOUBLE_SIZE, -MAX_DOUBLE_SIZE, 0.0)) == NULL) {
    free_custom_shape(custom_shape);
    return NULL;
  }
 
  if (insert_param(param, custom_shape->param_tree) < 0) {
    free_custom_shape(custom_shape);
    return NULL;
  }
  if ((param = new_param_float("t6", P_FLAG_TVAR, &custom_shape->t6, NULL, MAX_DOUBLE_SIZE, -MAX_DOUBLE_SIZE, 0.0)) == NULL) {
    free_custom_shape(custom_shape);
    return NULL;
  }

  if (insert_param(param, custom_shape->param_tree) < 0) {
    free_custom_shape(custom_shape);
    return NULL;
  }
  if ((param = new_param_float("t7", P_FLAG_TVAR, &custom_shape->t7, NULL, MAX_DOUBLE_SIZE, -MAX_DOUBLE_SIZE, 0.0)) == NULL) {
    free_custom_shape(custom_shape);
    return NULL;
  }

  if (insert_param(param, custom_shape->param_tree) < 0) {
    free_custom_shape(custom_shape);
    return NULL;
  }

  if ((param = new_param_float("t8", P_FLAG_TVAR, &custom_shape->t8, NULL, MAX_DOUBLE_SIZE, -MAX_DOUBLE_SIZE, 0.0)) == NULL) {
    free_custom_shape(custom_shape);
    return NULL;
  }

  if (insert_param(param, custom_shape->param_tree) < 0) {
    free_custom_shape(custom_shape);
    return NULL;
  }
 
  /* End of parameter loading. Note that the read only parameters associated
     with custom shapes (ie, sample) are global variables, and not specific to 
     the custom shape datastructure. */



  return custom_shape;

}

void destroy_per_frame_init_eqn_tree_shape(splaytree_t * tree) {

  if (!tree)
    return;

  splay_traverse((void (*)(void*))free_init_cond, tree);
  destroy_splaytree(tree);

}



void destroy_init_cond_tree_shape(splaytree_t * tree) {

  if (!tree)
    return;

  splay_traverse((void (*)(void*))free_init_cond, tree);
  destroy_splaytree(tree);

}

void destroy_per_frame_eqn_tree_shape(splaytree_t * tree) {


  if (!tree)
    return;

  splay_traverse((void (*)(void*))free_per_frame_eqn, tree);
  destroy_splaytree(tree);

}


void destroy_param_db_tree_shape(splaytree_t * tree) {

  if (!tree)
    return;

  splay_traverse((void (*)(void*))free_param, tree);
  destroy_splaytree(tree);

}

/* Frees a custom shape form object */
void free_custom_shape(custom_shape_t * custom_shape) {

  if (custom_shape == NULL)
    return;

  if (custom_shape->param_tree == NULL)
    return;

  destroy_per_frame_eqn_tree_shape(custom_shape->per_frame_eqn_tree);
  destroy_init_cond_tree_shape(custom_shape->init_cond_tree);
  destroy_param_db_tree_shape(custom_shape->param_tree);
  destroy_per_frame_init_eqn_tree_shape(custom_shape->per_frame_init_eqn_tree);
  
  free(custom_shape);
  custom_shape = NULL;

  return;

}


custom_shape_t * find_custom_shape(int id, preset_t * preset, int create_flag) {

  custom_shape_t * custom_shape = NULL;

  if (preset == NULL)
    return NULL;
  
  if ((custom_shape = (custom_shape_t*)splay_find(&id, preset->custom_shape_tree)) == NULL) {
    
    if (CUSTOM_SHAPE_DEBUG) { printf("find_custom_shape: creating custom shape (id = %d)...", id);fflush(stdout);}
    
    if (create_flag == FALSE) {
      if (CUSTOM_SHAPE_DEBUG) printf("you specified not to (create flag = false), returning null\n");
      return NULL;
    }
    
    if ((custom_shape = new_custom_shape(id)) == NULL) {
      if (CUSTOM_SHAPE_DEBUG) printf("failed...out of memory?\n");
      return NULL;
    }
    
    if (CUSTOM_SHAPE_DEBUG) { printf("success.Inserting..."); fflush(stdout);}
    
    if (splay_insert(custom_shape, &custom_shape->id, preset->custom_shape_tree) < 0) {
      if (CUSTOM_SHAPE_DEBUG) printf("failed, probably a duplicated!!\n");
      free_custom_shape(custom_shape);
      return NULL;
    }
    
    if (CUSTOM_SHAPE_DEBUG) printf("done.\n");
  }
  
  return custom_shape;
}

void evalCustomShapeInitConditions(preset_t *preset) {
  splay_traverse((void (*)(void*))eval_custom_shape_init_conds, preset->custom_shape_tree);

}

 void eval_custom_shape_init_conds(custom_shape_t * custom_shape) {
 
  splay_traverse((void (*)(void*))eval_init_cond, custom_shape->per_frame_init_eqn_tree);
}


void load_unspecified_init_conds_shape(custom_shape_t * custom_shape) {

  interface_shape = custom_shape;
  splay_traverse((void (*)(void*))load_unspec_init_cond_shape, interface_shape->param_tree);
  interface_shape = NULL;
 
}

void load_unspec_init_cond_shape(param_t * param) {

  init_cond_t * init_cond;
  value_t init_val;

  /* Don't count read only parameters as initial conditions */
  if (param->flags & P_FLAG_READONLY)
    return;
 if (param->flags & P_FLAG_QVAR)
    return;
 if (param->flags & P_FLAG_TVAR)
    return;
 if (param->flags & P_FLAG_USERDEF)
    return;

  /* If initial condition was not defined by the preset file, force a default one
     with the following code */
  if ((init_cond =(init_cond_t*) splay_find(param->name, interface_shape->init_cond_tree)) == NULL) {
    
    /* Make sure initial condition does not exist in the set of per frame initial equations */
    if ((init_cond = (init_cond_t*)splay_find(param->name, interface_shape->per_frame_init_eqn_tree)) != NULL)
      return;
    
    if (param->type == P_TYPE_BOOL)
      init_val.bool_val = 0;
    
    else if (param->type == P_TYPE_INT)
      init_val.int_val = *(int*)param->engine_val;

    else if (param->type == P_TYPE_DOUBLE)
      init_val.float_val = *(float*)param->engine_val;

    //printf("%s\n", param->name);
    /* Create new initial condition */
    if ((init_cond = new_init_cond(param, init_val)) == NULL)
      return;
    
    /* Insert the initial condition into this presets tree */
    if (splay_insert(init_cond, init_cond->param->name, interface_shape->init_cond_tree) < 0) {
      free_init_cond(init_cond);
      return;
    }
    
  }
 
}


/* Interface function. Makes another custom shape the current
   concern for per frame / point equations */
custom_shape_t * nextCustomShape(preset_t *preset) {

  if ((interface_shape = (custom_shape_t*) splay_find(&cwave_interface_id, preset->custom_shape_tree)) == NULL) {
    cwave_interface_id = 0;
    return NULL;
  }

  cwave_interface_id++;

  /* Evaluate all per frame equations associated with this shape */
 splay_traverse((void (*)(void*))eval_init_cond, interface_shape->init_cond_tree);
  splay_traverse((void (*)(void*))eval_per_frame_eqn, interface_shape->per_frame_eqn_tree);

  return interface_shape;
}
