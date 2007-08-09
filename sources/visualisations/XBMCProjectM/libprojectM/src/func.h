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

#ifndef _FUNC_H
#define _FUNC_H

/* Public Prototypes */
func_t * create_func (char * name, float (*func_ptr)(float*), int num_args);
int remove_func(func_t * func);
func_t * find_func(char * name);
int init_builtin_func_db();
int destroy_builtin_func_db();
int load_all_builtin_func();
int load_builtin_func(char * name, float (*func_ptr)(float*), int num_args);
void free_func(func_t * func);

#endif /** !_FUNC_H */
