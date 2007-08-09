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

/* Basic Parameter Functions */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "projectM.h"

#include "fatal.h"
#include "common.h"

#include "splaytree_types.h"
#include "splaytree.h"
#include "tree_types.h"

#include "param_types.h"
#include "param.h"

#include "expr_types.h"
#include "eval.h"
#include "wipemalloc.h"

extern projectM_t *PM;

void reset_param(param_t * param);

int is_valid_param_string(char * string); /* true if string is valid variable or function name */


/* A splay tree of builtin parameters */
splaytree_t * builtin_param_tree = NULL;

int insert_param_alt_name(param_t * param, char * alt_name);

int insert_builtin_param(param_t * param);

/* Private function prototypes */
int compare_param(char * name, char * name2);

int load_builtin_param_float(char * name, void * engine_val, void * matrix, short int flags, 
								float init_val, float upper_bound, float lower_bound, char * alt_name);

int load_builtin_param_int(char * name, void * engine_val, short int flags, 
								int init_val, int upper_bound, int lower_bound, char * alt_name);
								
int load_builtin_param_bool(char * name, void * engine_val, short int flags, 
								int init_val, char * alt_name);
								
								
								
param_t * create_param (char * name, short int type, short int flags, void * engine_val, void * matrix,
							value_t default_init_val, value_t upper_bound, value_t lower_bound) {

  param_t * param = NULL;

  param = (param_t*)wipemalloc(sizeof(param_t));
 
  if (param == NULL) {
	printf("create_param: out of memory!!!\n");
	return NULL;
  }

  /* Clear name space, think the strncpy statement makes this redundant */
  //memset(param->name, 0, MAX_TOKEN_SIZE);

  /* Copy given name into parameter structure */
  strncpy(param->name, name, MAX_TOKEN_SIZE-1); 
  
  /* Assign other entries in a constructor like fashion */
  param->type = type;
  param->flags = flags;
  param->matrix_flag = 0;
  param->matrix = matrix;
  param->engine_val = engine_val;
  param->default_init_val = default_init_val;
  //*param->init_val = default_init_val;
  param->upper_bound = upper_bound;
  param->lower_bound = lower_bound;

  /* Return instantiated parameter */
  return param;

}

/* Creates a user defined parameter */
param_t * create_user_param(char * name) {

  param_t * param;
  value_t iv;
  value_t ub;
  value_t lb;
  float * engine_val;
  
  /* Set initial values to default */
  iv.float_val = DEFAULT_DOUBLE_IV;
  ub.float_val = DEFAULT_DOUBLE_UB;
  lb.float_val = DEFAULT_DOUBLE_LB;

  /* Argument checks */
  if (name == NULL)
    return NULL;

  /* Allocate space for the engine variable */
  if ((engine_val = (float*)wipemalloc(sizeof(float))) == NULL)
    return NULL;

  (*engine_val) = iv.float_val; /* set some default init value */
  
  /* Create the new user parameter */
  if ((param = create_param(name, P_TYPE_DOUBLE, P_FLAG_USERDEF, engine_val, NULL, iv, ub, lb)) == NULL) {
    free(engine_val);
    engine_val = NULL;
    return NULL;
  }
  if (PARAM_DEBUG) printf("create_param: \"%s\" initialized\n", param->name);
  /* Return the instantiated parameter */
  return param;
}

/* Initialize the builtin parameter database.
   Should only be necessary once */
int init_builtin_param_db(projectM_t *PM) {
	
  /* Create the builtin parameter splay tree (go Sleator...) */
  if ((builtin_param_tree = create_splaytree((int (*)(void*,void*))compare_string,(void* (*)(void*)) copy_string, (void (*)(void*))free_string)) == NULL) {
	  if (PARAM_DEBUG) printf("init_builtin_param_db: failed to initialize database (FATAL)\n");  
	  return PROJECTM_OUTOFMEM_ERROR;
  } 

  if (PARAM_DEBUG) {
	  printf("init_builtin_param: loading database...");
	  fflush(stdout);
  }
  
  /* Loads all builtin parameters into the database */
  if (load_all_builtin_param( PM ) < 0) {
	if (PARAM_DEBUG) printf("failed loading builtin parameters (FATAL)\n");
    return PROJECTM_ERROR;
  }
  
  if (PARAM_DEBUG) printf("success!\n");
	  
  /* Finished, no errors */
  return PROJECTM_SUCCESS;
}

/* Destroy the builtin parameter database.
   Generally, do this on projectm exit */
int destroy_builtin_param_db() {
  
  splay_traverse((void (*)(void*))free_param, builtin_param_tree);
  destroy_splaytree(builtin_param_tree);
  builtin_param_tree = NULL;
  return PROJECTM_SUCCESS;	

}


/* Insert a parameter into the database with an alternate name */
int insert_param_alt_name(param_t * param, char * alt_name) {
  
  if (param == NULL)
    return PROJECTM_ERROR;
  if (alt_name == NULL)
	  return PROJECTM_ERROR;
  	
  splay_insert_link(alt_name, param->name, builtin_param_tree);

  return PROJECTM_SUCCESS;
}


param_t * find_builtin_param(char * name) {

  /* Null argument checks */
  if (name == NULL)
	  return NULL;
    
  return  (param_t*)splay_find(name, builtin_param_tree);

}

/* Find a parameter given its name, will create one if not found */
param_t * find_param(char * name, preset_t * preset, int flags) {

  param_t * param = NULL;

  /* Null argument checks */
  if (name == NULL)
	  return NULL;
  if (preset == NULL)
	  return NULL;
  
  /* First look in the builtin database */
  param = (param_t *)splay_find(name, builtin_param_tree);

  /* If the search failed, check the user database */
  if (param == NULL) {
    param = (param_t*)splay_find(name, preset->user_param_tree);
  }
  /* If it doesn't exist in the user (or builtin) database and 
  	  create_flag is set, then make it and insert into the database 
  */
  
  if ((param == NULL) && (flags & P_CREATE)) {
	
	/* Check if string is valid */
    if (!is_valid_param_string(name)) {
      if (PARAM_DEBUG) printf("find_param: invalid parameter name:\"%s\"\n", name);
      return NULL;
    }
    /* Now, create the user defined parameter given the passed name */
    if ((param = create_user_param(name)) == NULL) {
      if (PARAM_DEBUG) printf("find_param: failed to create a new user parameter!\n");
      return NULL;
    }
    /* Finally, insert the new parameter into this preset's proper splaytree */
    if (splay_insert(param, param->name, preset->user_param_tree) < 0) {
      if (PARAM_DEBUG) printf("PARAM \"%s\" already exists in user parameter tree!\n", param->name);
      free_param(param);
      return NULL;
    }	 
    
  }	  
  
  /* Return the found (or created) parameter. Note that if P_CREATE is not set, this could be null */
  return param;
  
}

/* Compare string name with parameter name */
int compare_param(char * name, char * name2) {

  int cmpval;
  printf("am i used\n");
  /* Uses string comparison function */
  cmpval = strncmp(name, name2, MAX_TOKEN_SIZE-1);
  
  return cmpval;
}

/* Loads all builtin parameters, limits are also defined here */
int load_all_builtin_param( projectM_t *pm ) {

  load_builtin_param_float("fRating", (void*)&pm->fRating, NULL, P_FLAG_NONE, 0.0 , 5.0, 0.0, NULL);
  load_builtin_param_float("fWaveScale", (void*)&pm->fWaveScale, NULL, P_FLAG_NONE, 0.0, MAX_DOUBLE_SIZE, -MAX_DOUBLE_SIZE, NULL);
  load_builtin_param_float("gamma", (void*)&pm->fGammaAdj, NULL, P_FLAG_NONE, 0.0, MAX_DOUBLE_SIZE, 0, "fGammaAdj");
  load_builtin_param_float("echo_zoom", (void*)&pm->fVideoEchoZoom, NULL, P_FLAG_NONE, 0.0, MAX_DOUBLE_SIZE, 0, "fVideoEchoZoom");
  load_builtin_param_float("echo_alpha", (void*)&pm->fVideoEchoAlpha, NULL, P_FLAG_NONE, 0.0, MAX_DOUBLE_SIZE, 0, "fVideoEchoAlpha");
  load_builtin_param_float("wave_a", (void*)&pm->fWaveAlpha, NULL, P_FLAG_NONE, 0.0, 1.0, 0, "fWaveAlpha");
  load_builtin_param_float("fWaveSmoothing", (void*)&pm->fWaveSmoothing, NULL, P_FLAG_NONE, 0.0, 1.0, -1.0, NULL);  
  load_builtin_param_float("fModWaveAlphaStart", (void*)&pm->fModWaveAlphaStart, NULL, P_FLAG_NONE, 0.0, 1.0, -1.0, NULL);
  load_builtin_param_float("fModWaveAlphaEnd", (void*)&pm->fModWaveAlphaEnd, NULL, P_FLAG_NONE, 0.0, 1.0, -1.0, NULL);
  load_builtin_param_float("fWarpAnimSpeed",  (void*)&pm->fWarpAnimSpeed, NULL, P_FLAG_NONE, 0.0, 1.0, -1.0, NULL);
  //  load_builtin_param_float("warp", (void*)&pm->warp, pm->warp_mesh, P_FLAG_NONE, 0.0, MAX_DOUBLE_SIZE, 0, NULL);
	
  load_builtin_param_float("fShader", (void*)&pm->fShader, NULL, P_FLAG_NONE, 0.0, 1.0, -1.0, NULL);
  load_builtin_param_float("decay", (void*)&pm->decay, NULL, P_FLAG_NONE, 0.0, 1.0, 0, "fDecay");

  load_builtin_param_int("echo_orient", (void*)&pm->nVideoEchoOrientation, P_FLAG_NONE, 0, 3, 0, "nVideoEchoOrientation");
  load_builtin_param_int("wave_mode", (void*)&pm->nWaveMode, P_FLAG_NONE, 0, 7, 0, "nWaveMode");
  
  load_builtin_param_bool("wave_additive", (void*)&pm->bAdditiveWaves, P_FLAG_NONE, FALSE, "bAdditiveWaves");
  load_builtin_param_bool("bModWaveAlphaByVolume", (void*)&pm->bModWaveAlphaByVolume, P_FLAG_NONE, FALSE, NULL);
  load_builtin_param_bool("wave_brighten", (void*)&pm->bMaximizeWaveColor, P_FLAG_NONE, FALSE, "bMaximizeWaveColor");
  load_builtin_param_bool("wrap", (void*)&pm->bTexWrap, P_FLAG_NONE, FALSE, "bTexWrap");
  load_builtin_param_bool("darken_center", (void*)&pm->bDarkenCenter, P_FLAG_NONE, FALSE, "bDarkenCenter");
  load_builtin_param_bool("bRedBlueStereo", (void*)&pm->bRedBlueStereo, P_FLAG_NONE, FALSE, NULL);
  load_builtin_param_bool("brighten", (void*)&pm->bBrighten, P_FLAG_NONE, FALSE, "bBrighten");
  load_builtin_param_bool("darken", (void*)&pm->bDarken, P_FLAG_NONE, FALSE, "bDarken");
  load_builtin_param_bool("solarize", (void*)&pm->bSolarize, P_FLAG_NONE, FALSE, "bSolarize");
  load_builtin_param_bool("invert", (void*)&pm->bInvert, P_FLAG_NONE, FALSE, "bInvert");
  load_builtin_param_bool("bMotionVectorsOn", (void*)&pm->bMotionVectorsOn, P_FLAG_NONE, FALSE, NULL);
  load_builtin_param_bool("wave_dots", (void*)&pm->bWaveDots, P_FLAG_NONE, FALSE, "bWaveDots");
  load_builtin_param_bool("wave_thick", (void*)&pm->bWaveThick, P_FLAG_NONE, FALSE, "bWaveThick");
 
  
  
  load_builtin_param_float("zoom", (void*)&pm->zoom, pm->zoom_mesh,  P_FLAG_PER_PIXEL |P_FLAG_DONT_FREE_MATRIX, 0.0, MAX_DOUBLE_SIZE, 0, NULL);
  load_builtin_param_float("rot", (void*)&pm->rot, pm->rot_mesh,  P_FLAG_PER_PIXEL |P_FLAG_DONT_FREE_MATRIX, 0.0, MAX_DOUBLE_SIZE, MIN_DOUBLE_SIZE, NULL);
  load_builtin_param_float("zoomexp", (void*)&pm->zoomexp, pm->zoomexp_mesh,  P_FLAG_PER_PIXEL |P_FLAG_NONE, 0.0, MAX_DOUBLE_SIZE, 0, "fZoomExponent");
 
  load_builtin_param_float("cx", (void*)&pm->cx, pm->cx_mesh, P_FLAG_PER_PIXEL | P_FLAG_DONT_FREE_MATRIX, 0.0, 1.0, 0, NULL);
  load_builtin_param_float("cy", (void*)&pm->cy, pm->cy_mesh, P_FLAG_PER_PIXEL | P_FLAG_DONT_FREE_MATRIX, 0.0, 1.0, 0, NULL);
  load_builtin_param_float("dx", (void*)&pm->dx, pm->dx_mesh,  P_FLAG_PER_PIXEL | P_FLAG_DONT_FREE_MATRIX, 0.0, MAX_DOUBLE_SIZE, MIN_DOUBLE_SIZE, NULL);
  load_builtin_param_float("dy", (void*)&pm->dy, pm->dy_mesh,  P_FLAG_PER_PIXEL |P_FLAG_DONT_FREE_MATRIX, 0.0, MAX_DOUBLE_SIZE, MIN_DOUBLE_SIZE, NULL);
  load_builtin_param_float("sx", (void*)&pm->sx, pm->sx_mesh,  P_FLAG_PER_PIXEL |P_FLAG_DONT_FREE_MATRIX, 0.0, MAX_DOUBLE_SIZE, 0, NULL);
  load_builtin_param_float("sy", (void*)&pm->sy, pm->sy_mesh,  P_FLAG_PER_PIXEL |P_FLAG_DONT_FREE_MATRIX, 0.0, MAX_DOUBLE_SIZE, 0, NULL);

  load_builtin_param_float("wave_r", (void*)&pm->wave_r, NULL, P_FLAG_NONE, 0.0, 1.0, 0.0, NULL);
  load_builtin_param_float("wave_g", (void*)&pm->wave_g, NULL, P_FLAG_NONE, 0.0, 1.0, 0.0, NULL);
  load_builtin_param_float("wave_b", (void*)&pm->wave_b, NULL, P_FLAG_NONE, 0.0, 1.0, 0.0, NULL);
  load_builtin_param_float("wave_x", (void*)&pm->wave_x, NULL, P_FLAG_NONE, 0.0, 1.0, 0.0, NULL);
  load_builtin_param_float("wave_y", (void*)&pm->wave_y, NULL, P_FLAG_NONE, 0.0, 1.0, 0.0, NULL);
  load_builtin_param_float("wave_mystery", (void*)&pm->wave_mystery, NULL, P_FLAG_NONE, 0.0, 1.0, -1.0, "fWaveParam");
  
  load_builtin_param_float("ob_size", (void*)&pm->ob_size, NULL, P_FLAG_NONE, 0.0, 0.5, 0, NULL);
  load_builtin_param_float("ob_r", (void*)&pm->ob_r, NULL, P_FLAG_NONE, 0.0, 1.0, 0.0, NULL);
  load_builtin_param_float("ob_g", (void*)&pm->ob_g, NULL, P_FLAG_NONE, 0.0, 1.0, 0.0, NULL);
  load_builtin_param_float("ob_b", (void*)&pm->ob_b, NULL, P_FLAG_NONE, 0.0, 1.0, 0.0, NULL);
  load_builtin_param_float("ob_a", (void*)&pm->ob_a, NULL, P_FLAG_NONE, 0.0, 1.0, 0.0, NULL);

  load_builtin_param_float("ib_size", (void*)&pm->ib_size,  NULL,P_FLAG_NONE, 0.0, .5, 0.0, NULL);
  load_builtin_param_float("ib_r", (void*)&pm->ib_r,  NULL,P_FLAG_NONE, 0.0, 1.0, 0.0, NULL);
  load_builtin_param_float("ib_g", (void*)&pm->ib_g,  NULL,P_FLAG_NONE, 0.0, 1.0, 0.0, NULL);
  load_builtin_param_float("ib_b", (void*)&pm->ib_b,  NULL,P_FLAG_NONE, 0.0, 1.0, 0.0, NULL);
  load_builtin_param_float("ib_a", (void*)&pm->ib_a,  NULL,P_FLAG_NONE, 0.0, 1.0, 0.0, NULL);

  load_builtin_param_float("mv_r", (void*)&pm->mv_r,  NULL,P_FLAG_NONE, 0.0, 1.0, 0.0, NULL);
  load_builtin_param_float("mv_g", (void*)&pm->mv_g,  NULL,P_FLAG_NONE, 0.0, 1.0, 0.0, NULL);
  load_builtin_param_float("mv_b", (void*)&pm->mv_b,  NULL,P_FLAG_NONE, 0.0, 1.0, 0.0, NULL);
  load_builtin_param_float("mv_x", (void*)&pm->mv_x,  NULL,P_FLAG_NONE, 0.0, 64.0, 0.0, "nMotionVectorsX");
  load_builtin_param_float("mv_y", (void*)&pm->mv_y,  NULL,P_FLAG_NONE, 0.0, 48.0, 0.0, "nMotionVectorsY");
  load_builtin_param_float("mv_l", (void*)&pm->mv_l,  NULL,P_FLAG_NONE, 0.0, 5.0, 0.0, NULL);
  load_builtin_param_float("mv_dy", (void*)&pm->mv_dy, NULL, P_FLAG_NONE, 0.0, 1.0, -1.0, NULL);
  load_builtin_param_float("mv_dx", (void*)&pm->mv_dx,  NULL,P_FLAG_NONE, 0.0, 1.0, -1.0, NULL);
  load_builtin_param_float("mv_a", (void*)&pm->mv_a,  NULL,P_FLAG_NONE, 0.0, 1.0, 0.0, NULL);

  load_builtin_param_float("time", (void*)&pm->Time,  NULL,P_FLAG_READONLY, 0.0, MAX_DOUBLE_SIZE, 0.0, NULL);        
  load_builtin_param_float("bass", (void*)&pm->bass,  NULL,P_FLAG_READONLY, 0.0, MAX_DOUBLE_SIZE, 0.0, NULL);
  load_builtin_param_float("mid", (void*)&pm->mid,  NULL,P_FLAG_READONLY, 0.0, MAX_DOUBLE_SIZE, 0, NULL);      
  load_builtin_param_float("bass_att", (void*)&pm->bass_att,  NULL,P_FLAG_READONLY, 0.0, MAX_DOUBLE_SIZE, 0, NULL);
  load_builtin_param_float("mid_att", (void*)&pm->mid_att,  NULL,P_FLAG_READONLY, 0.0, MAX_DOUBLE_SIZE, 0, NULL);
  load_builtin_param_float("treb_att", (void*)&pm->treb_att,  NULL,P_FLAG_READONLY, 0.0, MAX_DOUBLE_SIZE, 0, NULL);
  load_builtin_param_int("frame", (void*)&pm->frame, P_FLAG_READONLY, 0, MAX_INT_SIZE, 0, NULL);
  load_builtin_param_float("progress", (void*)&pm->progress,  NULL,P_FLAG_READONLY, 0.0, 1, 0, NULL);
  load_builtin_param_int("fps", (void*)&pm->fps, P_FLAG_NONE, 15, MAX_INT_SIZE, 0, NULL);



  load_builtin_param_float("x", (void*)&pm->x_per_pixel, pm->x_mesh,  P_FLAG_PER_PIXEL |P_FLAG_ALWAYS_MATRIX | P_FLAG_READONLY | P_FLAG_DONT_FREE_MATRIX, 
			    0, MAX_DOUBLE_SIZE, -MAX_DOUBLE_SIZE, NULL);
  load_builtin_param_float("y", (void*)&pm->y_per_pixel, pm->y_mesh,  P_FLAG_PER_PIXEL |P_FLAG_ALWAYS_MATRIX |P_FLAG_READONLY | P_FLAG_DONT_FREE_MATRIX, 
			    0, MAX_DOUBLE_SIZE, -MAX_DOUBLE_SIZE, NULL);
  load_builtin_param_float("ang", (void*)&pm->ang_per_pixel, pm->theta_mesh,  P_FLAG_PER_PIXEL |P_FLAG_ALWAYS_MATRIX | P_FLAG_READONLY | P_FLAG_DONT_FREE_MATRIX, 
			    0, MAX_DOUBLE_SIZE, -MAX_DOUBLE_SIZE, NULL);      
  load_builtin_param_float("rad", (void*)&pm->rad_per_pixel, pm->rad_mesh,  P_FLAG_PER_PIXEL |P_FLAG_ALWAYS_MATRIX | P_FLAG_READONLY | P_FLAG_DONT_FREE_MATRIX, 
			    0, MAX_DOUBLE_SIZE, -MAX_DOUBLE_SIZE, NULL);


  load_builtin_param_float("q1", (void*)&pm->q1,  NULL, P_FLAG_PER_PIXEL |P_FLAG_QVAR, 0, MAX_DOUBLE_SIZE, -MAX_DOUBLE_SIZE, NULL);
  load_builtin_param_float("q2", (void*)&pm->q2,  NULL, P_FLAG_PER_PIXEL |P_FLAG_QVAR, 0, MAX_DOUBLE_SIZE, -MAX_DOUBLE_SIZE, NULL);
  load_builtin_param_float("q3", (void*)&pm->q3,  NULL, P_FLAG_PER_PIXEL |P_FLAG_QVAR, 0, MAX_DOUBLE_SIZE, -MAX_DOUBLE_SIZE, NULL);
  load_builtin_param_float("q4", (void*)&pm->q4,  NULL, P_FLAG_PER_PIXEL |P_FLAG_QVAR, 0, MAX_DOUBLE_SIZE, -MAX_DOUBLE_SIZE, NULL);
  load_builtin_param_float("q5", (void*)&pm->q5,  NULL, P_FLAG_PER_PIXEL |P_FLAG_QVAR, 0, MAX_DOUBLE_SIZE, -MAX_DOUBLE_SIZE, NULL);
  load_builtin_param_float("q6", (void*)&pm->q6,  NULL, P_FLAG_PER_PIXEL |P_FLAG_QVAR, 0, MAX_DOUBLE_SIZE, -MAX_DOUBLE_SIZE, NULL);
  load_builtin_param_float("q7", (void*)&pm->q7,  NULL, P_FLAG_PER_PIXEL |P_FLAG_QVAR, 0, MAX_DOUBLE_SIZE, -MAX_DOUBLE_SIZE, NULL);
  load_builtin_param_float("q8", (void*)&pm->q8,  NULL, P_FLAG_PER_PIXEL |P_FLAG_QVAR, 0, MAX_DOUBLE_SIZE, -MAX_DOUBLE_SIZE, NULL);



  /* variables added in 1.04 */
  load_builtin_param_int("meshx", (void*)&pm->gx, P_FLAG_READONLY, 32, 96, 8, NULL);
  load_builtin_param_int("meshy", (void*)&pm->gy, P_FLAG_READONLY, 24, 72, 6, NULL);

  return PROJECTM_SUCCESS;  
  
}

/* Free's a parameter type */
void free_param(param_t * param) {
  int x;
  if (param == NULL)
	return;
  
  if (param->flags & P_FLAG_USERDEF) {
    free(param->engine_val);
    param->engine_val = NULL;

  }
  //if (!(param->flags & P_FLAG_DONT_FREE_MATRIX)) {
    if (param->matrix && !(param->flags & P_FLAG_DONT_FREE_MATRIX)) {  

    if (param->flags & P_FLAG_PER_POINT) {
      free(param->matrix);
      param->matrix = NULL;
    }//FIX THIS NOW XMAS05
    else if (param->flags & P_FLAG_PER_PIXEL) {
       for(x = 0; x < PM->gx; x++) 
	   free(((float**)param->matrix)[x]);
       free(param->matrix);
	param->matrix = NULL;
      }
  }

  if (PARAM_DEBUG) printf("free_param: freeing \"%s\".\n", param->name);
  free(param);
  param = NULL;

}

/* Loads a float parameter into the builtin database */
int load_builtin_param_float(char * name, void * engine_val, void * matrix, short int flags, 
						float init_val, float upper_bound, float lower_bound, char * alt_name) {

  param_t * param = NULL;
  value_t iv, ub, lb;

  iv.float_val = init_val;
  ub.float_val = upper_bound;
  lb.float_val = lower_bound;
							
  /* Create new parameter of type float */
  if (PARAM_DEBUG == 2) {
	  printf("load_builtin_param_float: (name \"%s\") (alt_name = \"%s\") ", name, alt_name);
	  fflush(stdout);
  }  
  
 if ((param = create_param(name, P_TYPE_DOUBLE, flags, engine_val, matrix, iv, ub, lb)) == NULL) {
    return PROJECTM_OUTOFMEM_ERROR;
  }
  
  if (PARAM_DEBUG == 2) {
	printf("created...");
	fflush(stdout);
   }	  
   
  /* Insert the paremeter into the database */

  if (insert_builtin_param(param) < 0) {
	free_param(param);
    return PROJECTM_ERROR;
  }

  if (PARAM_DEBUG == 2) {
	  printf("inserted...");
	  fflush(stdout);
  }  
  
  /* If this parameter has an alternate name, insert it into the database as link */
  
  if (alt_name != NULL) {
	insert_param_alt_name(param, alt_name); 

  if (PARAM_DEBUG == 2) {
	  printf("alt_name inserted...");
	  fflush(stdout);
  	}
  
	
  }  	  

  if (PARAM_DEBUG == 2) printf("finished\n");	  
  /* Finished, return success */
  return PROJECTM_SUCCESS;
}



/* Loads a float parameter into the builtin database */
param_t * new_param_float(char * name, short int flags, void * engine_val, void * matrix,
						float upper_bound, float lower_bound, float init_val) {

  param_t * param;
  value_t iv, ub, lb;

  iv.float_val = init_val;
  ub.float_val = upper_bound;
  lb.float_val = lower_bound;
			     			
  if ((param = create_param(name, P_TYPE_DOUBLE, flags, engine_val, matrix,iv, ub, lb)) == NULL) 
    return NULL;
  
  
  /* Finished, return success */
  return param;
}


/* Creates a new parameter of type int */
param_t * new_param_int(char * name, short int flags, void * engine_val,
						int upper_bound, int lower_bound, int init_val) {

  param_t * param;
  value_t iv, ub, lb;

  iv.int_val = init_val;
  ub.int_val = upper_bound;
  lb.int_val = lower_bound;
							
  if ((param = create_param(name, P_TYPE_INT, flags, engine_val, NULL, iv, ub, lb)) == NULL) 
    return NULL;
  
 
  /* Finished, return success */
  return param;
}

/* Creates a new parameter of type bool */
param_t * new_param_bool(char * name, short int flags, void * engine_val,
						int upper_bound, int lower_bound, int init_val) {

  param_t * param;
  value_t iv, ub, lb;

  iv.bool_val = init_val;
  ub.bool_val = upper_bound;
  lb.bool_val = lower_bound;
							
  if ((param = create_param(name, P_TYPE_BOOL, flags, engine_val, NULL, iv, ub, lb)) == NULL)
    return NULL;
  
 
  /* Finished, return success */
  return param;
}


/* Loads a integer parameter into the builtin database */
int load_builtin_param_int(char * name, void * engine_val, short int flags,
						int init_val, int upper_bound, int lower_bound, char * alt_name) {

  param_t * param;
  value_t iv, ub, lb;

  iv.int_val = init_val;
  ub.int_val = upper_bound;
  lb.int_val = lower_bound;	
							
  param = create_param(name, P_TYPE_INT, flags, engine_val, NULL, iv, ub, lb);

  if (param == NULL) {
    return PROJECTM_OUTOFMEM_ERROR;
  }

  if (insert_builtin_param(param) < 0) {
	free_param(param);
    return PROJECTM_ERROR;
  }
  
  if (alt_name != NULL) {
	insert_param_alt_name(param, alt_name);  	 
  }  
  
  return PROJECTM_SUCCESS;

}							
							
/* Loads a boolean parameter */
int load_builtin_param_bool(char * name, void * engine_val, short int flags, 
								int init_val, char * alt_name) {

  param_t * param;
  value_t iv, ub, lb;

  iv.int_val = init_val;
  ub.int_val = TRUE;
  lb.int_val = FALSE;	
																
  param = create_param(name, P_TYPE_BOOL, flags, engine_val, NULL, iv, ub, lb);

  if (param == NULL) {
    return PROJECTM_OUTOFMEM_ERROR;
  }

  if (insert_builtin_param(param) < 0) {
	free_param(param);
    return PROJECTM_ERROR;
  }
  
  if (alt_name != NULL) {
	insert_param_alt_name(param, alt_name);  	 
  }  
  
  return PROJECTM_SUCCESS;

}


	

/* Returns nonzero if the string is valid parameter name */
int is_valid_param_string(char * string) {
  
  if (string == NULL)
    return FALSE;
  
  /* This ensures the first character is non numeric */
  if( ((*string) >= 48) && ((*string) <= 57))
    return FALSE; 

  /* These probably should never happen */
  if (*string == '.')
	return FALSE;
  
  if (*string == '+')
	return FALSE;
  
  if (*string == '-')
	return FALSE;
  
  /* Could also add checks for other symbols. May do later */
  
  return TRUE;
   
}

/* Inserts a parameter into the builtin database */
int insert_builtin_param(param_t * param) {

	if (param == NULL)
		return PROJECTM_FAILURE;
	
	return splay_insert(param, param->name, builtin_param_tree);	
}

/* Inserts a parameter into the builtin database */
int insert_param(param_t * param, splaytree_t * database) {

	if (param == NULL)
	  return PROJECTM_FAILURE;
	if (database == NULL)
	  return PROJECTM_FAILURE;

	return splay_insert(param, param->name, database);	
}


/* Sets the parameter engine value to value val.
	clipping occurs if necessary */
void set_param(param_t * param, float val) {

	switch (param->type) {
		
	case P_TYPE_BOOL:
		if (val < 0)
			*((int*)param->engine_val) = 0;
		else if (val > 0)
			*((int*)param->engine_val) = 1;
		else
			*((int*)param->engine_val) = 0;
		break;
	case P_TYPE_INT:
		/* Make sure value is an integer */
		val = floor(val);
		if (val < param->lower_bound.int_val)
				*((int*)param->engine_val) = param->lower_bound.int_val;
		else if (val > param->upper_bound.int_val)
				*((int*)param->engine_val) = param->upper_bound.int_val;
		else
				*((int*)param->engine_val) = val;
		break;
	case P_TYPE_DOUBLE:
	  /* Make sure value is an integer */	

	 
	  if (val < param->lower_bound.float_val) 
	    *((float*)param->engine_val) = param->lower_bound.float_val;	  
	  else if (val > param->upper_bound.float_val)
	    *((float*)param->engine_val) = param->upper_bound.float_val;
	  else
	    *((float*)param->engine_val) = val;
	  break;
	default:
	  break;

	}
	
	return;
}




/* Search for parameter 'name' in 'database', if create_flag is true, then generate the parameter 
   and insert it into 'database' */
param_t * find_param_db(char * name, splaytree_t * database, int create_flag) {

  param_t * param = NULL;

  /* Null argument checks */
  if (name == NULL)
    return NULL;
  if (database == NULL)
    return NULL;
  
  /* First look in the builtin database */
  param = (param_t *)splay_find(name, database);

  
  if (((param = (param_t *)splay_find(name, database)) == NULL) && (create_flag == TRUE)) {
	
	/* Check if string is valid */
	if (!is_valid_param_string(name))
		return NULL;
	
	/* Now, create the user defined parameter given the passed name */
	if ((param = create_user_param(name)) == NULL)
		return NULL;
	
	/* Finally, insert the new parameter into this preset's proper splaytree */
	if (splay_insert(param, param->name, database) < 0) {
		free_param(param);
		return NULL;
	}	 
	
  }	  
  
  /* Return the found (or created) parameter. Note that this could be null */
  return param;

}

