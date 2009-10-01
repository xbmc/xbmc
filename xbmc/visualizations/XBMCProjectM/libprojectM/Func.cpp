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
/* Function management */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "Common.hpp"
#include "fatal.h"

#include "Func.hpp"
#include <map>

Func::Func (const std::string & _name, float (*_func_ptr)(float*), int _num_args):
	name(_name), func_ptr(_func_ptr), num_args(_num_args) {}

/* Frees a function type, real complicated... */
Func::~Func() {}
