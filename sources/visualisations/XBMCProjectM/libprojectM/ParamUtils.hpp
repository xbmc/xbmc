/** ParamUtils.hpp:
 *    A collection of utility functions to make using parameter types easier.
 *    In reality, this stuff belongs elsewhere, but one step at a time
 */
#ifndef _PARAM_UTILS_HPP
#define _PARAM_UTILS_HPP

#include "Param.hpp"
#include <map>
#include <cassert>


class BuiltinParams;

class ParamUtils
{
public:
  static bool insert(Param * param, std::map<std::string,Param*> * paramTree)
  {

    assert(param);
    assert(paramTree);

    
    return ((paramTree->insert(std::make_pair(param->name,param))).second);

  }

  static const int AUTO_CREATE = 1;
  static const int NO_CREATE = 0;

  template <int FLAGS>
  static Param * find(std::string name, std::map<std::string,Param*> * paramTree)
  {

    assert(paramTree);

    Param * param;

    /* First look in the suggested database */
    std::map<std::string,Param*>::iterator pos = paramTree->find(name);
		

    if ((FLAGS == AUTO_CREATE) && ((pos == paramTree->end())))
    {
      /* Check if string is valid */
      if (!Param::is_valid_param_string(name.c_str()))
        return NULL;

      /* Now, create the user defined parameter given the passed name */
      if ((param = new Param(name)) == NULL)
        return NULL;

      /* Finally, insert the new parameter into this preset's parameter tree */
      std::pair<std::map<std::string,Param*>::iterator, bool>  insertRetPair = 
		paramTree->insert(std::make_pair(param->name, param));

      assert(insertRetPair.second);
	
    } else if (pos != paramTree->end())
	param = pos->second;
    else
	param = NULL;	

    /* Return the found (or created) parameter. Note that this could be null */
    return param;


  }


  static Param * find(const std::string & name, BuiltinParams * builtinParams, std::map<std::string,Param*> * insertionTree)
  {

    Param * param;

    // Check first db
    if ((param = builtinParams->find_builtin_param(name)) != 0)
      return param;

    // Check second db, create if necessary
    return find<AUTO_CREATE>(name, insertionTree);

  }

};

#endif
