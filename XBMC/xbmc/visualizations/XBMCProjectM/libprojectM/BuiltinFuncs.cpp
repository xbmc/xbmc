//
// C++ Implementation: BuiltinFuncs
//
// Description: 
//  
//
// Author: Carmelo Piccione <carmelo.piccione@gmail.com>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//

/* Loads all builtin functions */


/* Loads a builtin function */
#include "BuiltinFuncs.hpp"
#include <string>
#include "Algorithms.hpp"
#include <iostream>
#include "fatal.h"
using namespace Algorithms;

std::map<std::string, Func*> BuiltinFuncs::builtin_func_tree;

int BuiltinFuncs::load_builtin_func(const std::string & name, float (*func_ptr)(float*), int num_args) {
	
  Func * func; 
  int retval; 

  /* Create new function */
  func = new Func(name, func_ptr, num_args);

  if (func == NULL)
    return PROJECTM_OUTOFMEM_ERROR;

  retval = insert_func( func );

  return retval;

}

Func * BuiltinFuncs::find_func(const std::string & name) {

  std::map<std::string, Func*>::iterator pos = builtin_func_tree.find(name);

  // Case: function not found, return null
  if (pos == builtin_func_tree.end())
	return 0;

  // Case: function found, return a pointer to it
  return pos->second;

}

int BuiltinFuncs::load_all_builtin_func() {

  if (load_builtin_func("int", FuncWrappers::int_wrapper, 1) < 0)
    return PROJECTM_ERROR;
  if (load_builtin_func("abs", FuncWrappers::abs_wrapper, 1) < 0)
    return PROJECTM_ERROR;
  if (load_builtin_func("sin", FuncWrappers::sin_wrapper, 1) < 0)
    return PROJECTM_ERROR;
  if (load_builtin_func("cos", FuncWrappers::cos_wrapper, 1) < 0)
    return PROJECTM_ERROR;
  if (load_builtin_func("tan", FuncWrappers::tan_wrapper, 1) < 0)
    return PROJECTM_ERROR;
  if (load_builtin_func("asin", FuncWrappers::asin_wrapper, 1) < 0)
    return PROJECTM_ERROR;
  if (load_builtin_func("acos", FuncWrappers::acos_wrapper, 1) < 0)
    return PROJECTM_ERROR;
  if (load_builtin_func("atan", FuncWrappers::atan_wrapper, 1) < 0)
    return PROJECTM_ERROR;
  if (load_builtin_func("sqr", FuncWrappers::sqr_wrapper, 1) < 0)
    return PROJECTM_ERROR;
  if (load_builtin_func("sqrt", FuncWrappers::sqrt_wrapper, 1) < 0)
    return PROJECTM_ERROR;
  if (load_builtin_func("pow", FuncWrappers::pow_wrapper, 2) < 0)
    return PROJECTM_ERROR;
  if (load_builtin_func("exp", FuncWrappers::exp_wrapper, 1) < 0)
    return PROJECTM_ERROR;
  if (load_builtin_func("log", FuncWrappers::log_wrapper, 1) < 0)
    return PROJECTM_ERROR;
  if (load_builtin_func("log10", FuncWrappers::log10_wrapper, 1) < 0)
    return PROJECTM_ERROR;
  if (load_builtin_func("sign", FuncWrappers::sign_wrapper, 1) < 0)
    return PROJECTM_ERROR;
  if (load_builtin_func("min", FuncWrappers::min_wrapper, 2) < 0)
    return PROJECTM_ERROR;
  if (load_builtin_func("max", FuncWrappers::max_wrapper, 2) < 0)
    return PROJECTM_ERROR;
  if (load_builtin_func("sigmoid", FuncWrappers::sigmoid_wrapper, 2) < 0)
    return PROJECTM_ERROR;
  if (load_builtin_func("atan2", FuncWrappers::atan2_wrapper, 2) < 0)
    return PROJECTM_ERROR;
  if (load_builtin_func("rand", FuncWrappers::rand_wrapper, 1) < 0)
    return PROJECTM_ERROR;
  if (load_builtin_func("band", FuncWrappers::band_wrapper, 2) < 0)
    return PROJECTM_ERROR;
  if (load_builtin_func("bor", FuncWrappers::bor_wrapper, 2) < 0)
    return PROJECTM_ERROR;
  if (load_builtin_func("bnot", FuncWrappers::bnot_wrapper, 1) < 0)
    return PROJECTM_ERROR;
  if (load_builtin_func("if", FuncWrappers::if_wrapper, 3) < 0)
    return PROJECTM_ERROR;
  if (load_builtin_func("equal", FuncWrappers::equal_wrapper, 2) < 0)
    return PROJECTM_ERROR;
  if (load_builtin_func("above", FuncWrappers::above_wrapper, 2) < 0)
    return PROJECTM_ERROR;
  if (load_builtin_func("below", FuncWrappers::below_wrapper, 2) < 0)
    return PROJECTM_ERROR;
  if (load_builtin_func("nchoosek", FuncWrappers::nchoosek_wrapper, 2) < 0)
    return PROJECTM_ERROR;
  if (load_builtin_func("fact", FuncWrappers::fact_wrapper, 1) < 0)
    return PROJECTM_ERROR;

  return PROJECTM_SUCCESS;
}


/* Initialize the builtin function database.
   Should only be necessary once */
int BuiltinFuncs::init_builtin_func_db() {
  int retval;

  retval = load_all_builtin_func();
  return retval;
}



/* Destroy the builtin function database.
   Generally, do this on projectm exit */
int BuiltinFuncs::destroy_builtin_func_db() {

traverse<TraverseFunctors::DeleteFunctor<Func> >(builtin_func_tree);

builtin_func_tree.clear();

return PROJECTM_SUCCESS;
}

/* Insert a function into the database */
int BuiltinFuncs::insert_func( Func *func ) {

  assert(func);
  std::pair<std::map<std::string, Func*>::iterator, bool> inserteePair =
  	builtin_func_tree.insert(std::make_pair(std::string(func->getName()), func));
  
  if (!inserteePair.second) {
	std::cerr << "Failed to insert builtin function \"" << func->getName() << "\" into collection! Bailing..." << std::endl;
	abort();

  }

  return PROJECTM_SUCCESS;
}


