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

#ifndef _INIT_COND_H
#define _INIT_COND_H

#define INIT_COND_DEBUG 0

#include "param_types.h"
#include "splaytree_types.h"

void eval_init_cond(init_cond_t * init_cond);
init_cond_t * new_init_cond(param_t * param, value_t init_val);
void free_init_cond(init_cond_t * init_cond);
char * create_init_cond_string_buffer(splaytree_t * init_cond_tree);

#endif /** !_INIT_COND_H */
