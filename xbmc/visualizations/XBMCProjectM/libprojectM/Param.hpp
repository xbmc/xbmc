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
 * Parameter used within a preset
 *
 * $Log$
 */

#ifndef _PARAM_H
#define _PARAM_H

/* Debug level, zero for none */
#define PARAM_DEBUG 0

#define P_CREATE 1
#define P_NONE 0

#define P_TYPE_BOOL 0
#define P_TYPE_INT 1
#define P_TYPE_DOUBLE 2
#define P_TYPE_STRING 3

#define P_FLAG_NONE 0
#define P_FLAG_READONLY 1
#define P_FLAG_USERDEF (1 << 1)
#define P_FLAG_QVAR (1 << 2)
#define P_FLAG_TVAR (1 << 3)
#define P_FLAG_ALWAYS_MATRIX (1 << 4)
#define P_FLAG_PER_PIXEL (1 << 6)
#define P_FLAG_PER_POINT (1 << 7)


#include "Expr.hpp"
#include "Common.hpp"
#include <cmath>
#include <string>
class InitCond;
class Param;
class Preset;
//#include <map>

/* Parameter Type */
class Param {
public:
    std::string name; /* name of the parameter, not necessary but useful neverthless */
    short int type; /* parameter number type (int, bool, or float) */	
    short int flags; /* read, write, user defined, etc */	
    short int matrix_flag; /* for optimization purposes */
    void * engine_val; /* pointer to the engine variable */
    void * matrix; /* per pixel / per point matrix for this variable */
    CValue default_init_val; /* a default initial condition value */
    CValue upper_bound; /* this parameter's upper bound */
    CValue lower_bound; /* this parameter's lower bound */

    /// Create a new parameter
    Param(std::string name, short int type, short int flags, 
           void * eqn_val, void *matrix,
           CValue default_init_val, CValue upper_bound, 
           CValue lower_bound);

    ~Param();

    /// Create a user defined floating point parameter
    Param( std::string name );

    static bool is_valid_param_string( const char *string );
    void set_param( float val );

    static Param *new_param_float( const char *name, short int flags, void *engine_val,
                             void *matrix, float upper_bound, 
                             float lower_bound,
                             float init_val );
    static Param *new_param_double(const char *name, short int flags, void *engine_val,
                             void *matrix, double upper_bound, 
                             double lower_bound,
                             double init_val );
    static Param * new_param_int(const char * name, short int flags, void * engine_val,
                           int upper_bound, int lower_bound, int init_val );
    static Param * new_param_bool(const char * name, short int flags, void * engine_val,
                            bool upper_bound, bool lower_bound, bool init_val );
    static Param * new_param_string(const char * name, short int flags, void * engine_val);

};


/* Sets the parameter engine value to value val.
	clipping occurs if necessary */
inline void Param::set_param( float val) {

    switch (type) {

    case P_TYPE_BOOL:
        if (val < 0)
            *((bool*)engine_val) = false;
        else if (val > 0)
            *((bool*)engine_val) = true;
        else
            *((bool*)engine_val) = false;
        break;
    case P_TYPE_INT:
        /* Make sure value is an integer */
        val = floor(val);
        if (val < lower_bound.int_val)
            *((int*)engine_val) = lower_bound.int_val;
        else if (val > upper_bound.int_val)
            *((int*)engine_val) = upper_bound.int_val;
        else
            *((int*)engine_val) = (int)val;
        break;
    case P_TYPE_DOUBLE:
        /* Make sure value is an integer */


        if (val < lower_bound.float_val)
            *((float*)engine_val) = lower_bound.float_val;
        else if (val > upper_bound.float_val)
            *((float*)engine_val) = upper_bound.float_val;
        else
            *((float*)engine_val) = val;
        break;
    default:
	abort();
        break;

    }

    return;
}

#endif /** !_PARAM_TYPES_H */

