#ifndef _INIT_COND_UTILS_HPP
#define _INIT_COND_UTILS_HPP
#include <map>
#include "InitCond.hpp"
#include <iostream>

namespace InitCondUtils {
class LoadUnspecInitCond {
	public:
	
	LoadUnspecInitCond(std::map<std::string,InitCond*> & initCondTree, std::map<std::string,InitCond*> & perFrameInitEqnTree):
		 m_initCondTree(initCondTree), m_perFrameInitEqnTree(perFrameInitEqnTree) {}

	void operator()(Param * param);

	private:
		std::map<std::string,InitCond*> & m_initCondTree;
		std::map<std::string,InitCond*> & m_perFrameInitEqnTree;
};


inline void LoadUnspecInitCond::operator() (Param * param) {

    InitCond * init_cond = 0;
    CValue init_val;

    assert(param);
    assert(param->engine_val);
    

    /* Don't count these parameters as initial conditions */
    if (param->flags & P_FLAG_READONLY)
        return;
    if (param->flags & P_FLAG_QVAR)
        return;
//    if (param->flags & P_FLAG_TVAR)
 //       return;
    if (param->flags & P_FLAG_USERDEF)
        return;

    /* If initial condition was not defined by the preset file, force a default one
       with the following code */

    if (m_initCondTree.find(param->name) == m_initCondTree.end()) {

        /* Make sure initial condition does not exist in the set of per frame initial equations */
	if (m_perFrameInitEqnTree.find(param->name) != m_perFrameInitEqnTree.end())
		return;

	// Set an initial vialue via correct union member
        if (param->type == P_TYPE_BOOL) 
            init_val.bool_val = param->default_init_val.bool_val;
        else if (param->type == P_TYPE_INT)
            init_val.int_val = param->default_init_val.int_val;
 
        else if (param->type == P_TYPE_DOUBLE) {
           		init_val.float_val = param->default_init_val.float_val;
	}

        //printf("%s\n", param->name);
        /* Create new initial condition */
	//std::cerr << "[InitCondUtils] creating an unspecified initial condition of name " << param->name << std::endl;
        if ((init_cond = new InitCond(param, init_val)) == NULL) {
	    abort();
        }

        /* Insert the initial condition into this presets tree */
	std::pair<std::map<std::string, InitCond*>::iterator, bool> inserteePair =
		m_initCondTree.insert(std::make_pair(init_cond->param->name, init_cond));
	assert(inserteePair.second);
	assert(inserteePair.first->second);
    } else
	assert(m_initCondTree.find(param->name)->second);

    
}
}
#endif
