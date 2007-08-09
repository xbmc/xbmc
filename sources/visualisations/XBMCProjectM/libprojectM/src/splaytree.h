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

#ifndef _SPLAYTREE_H
#define _SPLAYTREE_H

#define REGULAR_NODE_TYPE 0
#define SYMBOLIC_NODE_TYPE 1

#define PERFECT_MATCH 0
#define CLOSEST_MATCH 1

#include "projectM.h"

 void * splay_find(void * key, splaytree_t * t);
 int splay_insert(void * data, void * key, splaytree_t * t);
 int splay_insert_link(void * alias_key, void * orig_key, splaytree_t * splaytree);
 int splay_delete(void * key, splaytree_t * splaytree);
int splay_size(splaytree_t * t);
 splaytree_t * create_splaytree(int (*compare)(void*,void*), void * (*copy_key)(void*), void (*free_key)(void*));
 int destroy_splaytree(splaytree_t * splaytree);
 void splay_traverse(void (*func_ptr)(void*), splaytree_t * splaytree);
void splay_traverse_helper (void (*func_ptr)(void*), splaynode_t * splaynode);
 splaynode_t  * get_splaynode_of(void * key, splaytree_t * splaytree);
 void * splay_find_above_min(void * key, splaytree_t * root);
 void * splay_find_below_max(void * key, splaytree_t * root);
 void * splay_find_min(splaytree_t * t);
 void * splay_find_max(splaytree_t * t);

#endif /** !_SPLAYTREE_H */
