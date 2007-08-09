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

#ifndef _PARAM_H
#define _PARAM_H

#include "projectM.h"
#include "preset_types.h"
#include "splaytree_types.h"
/* Debug level, zero for none */
#define PARAM_DEBUG 0

/* Used to store a number of decidable type */

/* Function prototypes */
param_t * create_param (char * name, short int type, short int flags, void * eqn_val, void * matrix,
							value_t default_init_val, value_t upper_bound, value_t lower_bound);
param_t * create_user_param(char * name);
int init_builtin_param_db(projectM_t *PM);
int init_user_param_db();
int destroy_user_param_db();
int destroy_builtin_param_db();
void set_param(param_t * param, float val);
int remove_param(param_t * param);
param_t * find_param(char * name, struct PRESET_T * preset, int flags);
void free_param(param_t * param);
int load_all_builtin_param( projectM_t *pm );
int insert_param(param_t * param, splaytree_t * database);
param_t * find_builtin_param(char * name);
param_t * new_param_float(char * name, short int flags, void * engine_val, void * matrix,
		        float upper_bound, float lower_bound, float init_val);

param_t * new_param_int(char * name, short int flags, void * engine_val,
			int upper_bound, int lower_bound, int init_val);

param_t * new_param_bool(char * name, short int flags, void * engine_val,
			 int upper_bound, int lower_bound, int init_val);

param_t * find_param_db(char * name, splaytree_t * database, int create_flag);

#endif /** !_PARAM_H */
