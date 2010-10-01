/*
   std::vector binding library for AngelScript
   Copyright (c) 2004 Anthony "JM" Casteel

   This software is provided 'as-is', without any express or implied 
   warranty. In no event will the authors be held liable for any 
   damages arising from the use of this software.

   Permission is granted to anyone to use this software for any 
   purpose, including commercial applications, and to alter it and 
   redistribute it freely, subject to the following restrictions:

   1. The origin of this software must not be misrepresented; you 
      must not claim that you wrote the original software. If you use
	  this software in a product, an acknowledgment in the product 
	  documentation would be appreciated but is not required.

   2. Altered source versions must be plainly marked as such, and 
      must not be misrepresented as being the original software.

   3. This notice may not be removed or altered from any source 
      distribution.

   The original version of this library can be located at:
   http://www.angelcode.com/angelscript/

   AngelScript Copyright (c) 2003-2004 Andreas Jönsson (andreas@angelcode.com)

   Anthony "JM" Casteel
   DeyjaL@AOL.com
*/

// 2004-10-26 : Minor changes by Andreas Jönsson
// 2005-01-08 : Made compileable for dreamcast by Fredrik Ehnbom

#ifndef JM_AS_VECTOR_H
#define JM_AS_VECTOR_H

#define AS_VECTOR_CHECKBOUNDS

#include <vector>
#include <string>
#include <assert.h>

#include "angelscript.h"

template <typename T>
class vectorRegisterHelper
{
public:
	static void Construct(std::vector<T>* in)
	{
		new (in) std::vector<T>();
	}
			
	static void Destruct(std::vector<T>* in)
	{
		using namespace std;
		in->~vector<T>();
	}
	
	static void CopyConstruct(const std::vector<T>& rhs, std::vector<T>* in)
	{
		new (in) std::vector<T>(rhs);
	}

	static void NumConstruct(int size, std::vector<T>* in)
	{
		new (in) std::vector<T>(size);
	}

	static std::vector<T>& Assign(const std::vector<T>& rhs, std::vector<T>* lhs)
	{
		*lhs = rhs;
		return *lhs;
	}

	static T* Index(int i, std::vector<T>* lhs)
	{

#ifdef AS_VECTOR_ASSERTBOUNDS
		assert(i >= 0 && i < lhs->size() && "Array index out of bounds.");
#endif

#ifdef AS_VECTOR_CHECKBOUNDS
		if (i < 0 || i >= lhs->size())
		{
			asIScriptContext* context = asGetActiveContext();
			if( context )
				context->SetException("Array Index Out of Bounds.");
			return 0;
		}
#endif

		return &(*lhs)[i];
	}

	static int Size(std::vector<T>* lhs)
	{
		return lhs->size();
	}

	static void Resize(int size, std::vector<T>* lhs)
	{
		lhs->resize(size);
	}

	static void PushBack(const T& in, std::vector<T> *lhs)
	{
		lhs->push_back(in);
	}

	static void PopBack(std::vector<T>* lhs)
	{
		lhs->pop_back();
	}

/*	static void Erase(int i, std::vector<T>* lhs)
	{
		lhs->erase(Index(i,lhs));
	}

	static void Insert(int i, const T& e, std::vector<T>* lhs)
	{
		lhs->insert(Index(i,lhs), e);
	}
*/
};

template <typename T>
void RegisterVector(const std::string V_AS,  //The typename of the vector inside AS
	   		        const std::string T_AS,  //Template parameter typename in AS - must already be
			        asIScriptEngine* engine) //registered (or be primitive type)!!
{
	assert(engine && "Passed NULL engine pointer to registerVector");

	int error_code = 0;
	error_code = engine->RegisterObjectType(V_AS.c_str(), sizeof(std::vector<T>), asOBJ_CLASS_CDA);
	assert(error_code == 0 && "Failed to register object type");
	
	error_code = engine->RegisterObjectBehaviour(V_AS.c_str(), 
		asBEHAVE_CONSTRUCT, 
		"void f()", 
		asFUNCTION(vectorRegisterHelper<T>::Construct), 
		asCALL_CDECL_OBJLAST);
	assert(error_code == 0 && "Failed to register constructor");
	
	error_code = engine->RegisterObjectBehaviour(V_AS.c_str(),
		asBEHAVE_DESTRUCT,
		"void f()",
		asFUNCTION(vectorRegisterHelper<T>::Destruct),
		asCALL_CDECL_OBJLAST);
	assert(error_code == 0 && "Failed to register destructor");
	
	error_code = engine->RegisterObjectBehaviour(V_AS.c_str(),
		asBEHAVE_CONSTRUCT,
		(std::string("void f(")+V_AS+"&in)").c_str(),
		asFUNCTION(vectorRegisterHelper<T>::CopyConstruct),
		asCALL_CDECL_OBJLAST);
	assert(error_code == 0 && "Failed to register copy constructor");
	
	error_code = engine->RegisterObjectBehaviour(V_AS.c_str(),
		asBEHAVE_CONSTRUCT,
		"void f(int)",
		asFUNCTION(vectorRegisterHelper<T>::NumConstruct),
		asCALL_CDECL_OBJLAST);
	assert(error_code == 0 && "Failed to register construct(size)");

	error_code = engine->RegisterObjectBehaviour(V_AS.c_str(),
		asBEHAVE_INDEX,
		(T_AS+"& f(int)").c_str(),
		asFUNCTION(vectorRegisterHelper<T>::Index),
		asCALL_CDECL_OBJLAST);
	assert(error_code == 0 && "Failed to register operator[]");

	error_code = engine->RegisterObjectBehaviour(V_AS.c_str(),
		asBEHAVE_INDEX,
		("const "+T_AS+"& f(int) const").c_str(),
		asFUNCTION(vectorRegisterHelper<T>::Index),
		asCALL_CDECL_OBJLAST);
	assert(error_code == 0 && "Failed to register operator[]");
	
	error_code = engine->RegisterObjectBehaviour(V_AS.c_str(),
		asBEHAVE_ASSIGNMENT,
		(V_AS+"& f(const "+V_AS+"&in)").c_str(),
		asFUNCTION(vectorRegisterHelper<T>::Assign),
		asCALL_CDECL_OBJLAST);
	assert(error_code == 0 && "Failed to register operator=");
	
	error_code = engine->RegisterObjectMethod(V_AS.c_str(),
		"int size() const",
		asFUNCTION(vectorRegisterHelper<T>::Size),
		asCALL_CDECL_OBJLAST);
	assert(error_code == 0 && "Failed to register size");
	
	error_code = engine->RegisterObjectMethod(V_AS.c_str(),
		"void resize(int)",
		asFUNCTION(vectorRegisterHelper<T>::Resize),
		asCALL_CDECL_OBJLAST);
	assert(error_code == 0 && "Failed to register resize");
	
	error_code = engine->RegisterObjectMethod(V_AS.c_str(),
		(std::string("void push_back(")+T_AS+"&in)").c_str(),
		asFUNCTION(vectorRegisterHelper<T>::PushBack),
		asCALL_CDECL_OBJLAST);
	assert(error_code == 0 && "Failed to register push_back");
	
	error_code = engine->RegisterObjectMethod(V_AS.c_str(),
		"void pop_back()",
		asFUNCTION(vectorRegisterHelper<T>::PopBack),
		asCALL_CDECL_OBJLAST);
	assert(error_code == 0 && "Failed to register pop_back");

/*	error_code = engine->RegisterObjectMethod(V_AS.c_str(),
		"void erase(int)",
		asFUNCTION(vectorRegisterHelper<T>::Erase),
		asCALL_CDECL_OBJLAST);
	assert(error_code == 0 && "Failed to register erase");

	error_code = engine->RegisterObjectMethod(V_AS.c_str(),
		(std::string("void insert(int, const ")+T_AS+"&)").c_str(),
		asFUNCTION(vectorRegisterHelper<T>::Insert),
		asCALL_CDECL_OBJLAST);
	assert(error_code == 0 && "Failed to register insert");
*/
}

#endif

