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


/**  BuiltinParams.hpp :: a class to encapsulate and centralize
  *   all projectm builtin parameter methods and state. Used primarily
  *   by preset class
**/
#ifndef _BUILTIN_PARAMS_HPP
#define _BUILTIN_PARAMS_HPP

#include <stdio.h>
#include <string>
#include "PresetFrameIO.hpp"
#include "Param.hpp"
#include <map>
#include "Algorithms.hpp"

class BuiltinParams {

public:
   typedef std::map<std::string, std::string> AliasMap;

    /** Default constructor leaves database in an uninitialized state.  */
    BuiltinParams();

    /** Construct a new builtin parameter database with variables references given by
     * the preset input and output structures */
     BuiltinParams(const PresetInputs &  presetInputs, PresetOutputs & presetOutputs);

    ~BuiltinParams();

    /** Param database initalizer / destructor functions */
    int init_builtin_param_db(const PresetInputs & presetInputs, PresetOutputs & presetOutputs);
    int load_all_builtin_param(const PresetInputs & presetInputs, PresetOutputs & presetOutputs);
    int destroy_builtin_param_db();

    int insert_param_alt_name( Param *param, const std::string& salt_name );
    Param *find_builtin_param( const std::string & name );
    int load_builtin_param_float( const std::string & name, void *engine_val, void *matrix,
                                  short int flags,
                                  float init_val, float upper_bound,
                                  float lower_bound, const std::string & alt_name );
    int load_builtin_param_int( const std::string & name, void *engine_val, short int flags,
                                int init_val, int upper_bound,
                                int lower_bound, const std::string & alt_name );
    int load_builtin_param_bool( const std::string & name, void *engine_val, short int flags,
                                int init_val, const std::string & alt_name );
    int insert_builtin_param( Param *param );

    template <class Fun>
    void traverse(Fun & fun) {
	Algorithms::traverse(builtin_param_tree, fun);
    }


private:
    static const bool BUILTIN_PARAMS_DEBUG = false;

    // Used to associate multiple string names to one parameter
    AliasMap aliasMap;

    // Internal datastructure to store the parameters
    std::map<std::string,Param*> builtin_param_tree;
};
#endif
