/**
 * projectM -- Milkdrop-esque visualisation SDK
 * Copyright (C)2003-2007 projectM Team
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
/**
 * $Id$
 *
 * Function
 *
 * $Log$
 */

#ifndef _FUNC_H
#define _FUNC_H

#include "Common.hpp"
#include <string>

/* Function Type */
class Func {
public:

	/// Create a new function wrapper object
    /// \param name a name to uniquely identify the function. 
	/// \param func_ptr a pointer to a function of floating point arguments
	/// \param num_args the number of floating point arguments this function requires
    Func(const std::string & name, float (*func_ptr)(float*), int num_args );

    /* Public Prototypes */
    DLLEXPORT ~Func();

	inline const std::string & getName() const {
		return name;
	}

	inline int getNumArgs() const {
		return num_args;
	}

    float (*func_ptr)(float*);
private:	
    std::string name;
    int num_args;

};

#endif /** !_FUNC_H */
