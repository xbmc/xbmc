/*
   AngelCode Scripting Library
   Copyright (c) 2003-2006 Andreas Jönsson

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

   Andreas Jönsson
   andreas@angelcode.com
*/


//
// as_generic.h
//
// This class handles the call to a function registered with asCALL_GENERIC
//


#ifndef AS_GENERIC_H
#define AS_GENERIC_H

#include "as_config.h"

BEGIN_AS_NAMESPACE

class asCScriptEngine;
class asCScriptFunction;

class asCGeneric : public asIScriptGeneric
{
public:
	asCGeneric(asCScriptEngine *engine, asCScriptFunction *sysFunction, void *currentObject, asDWORD *stackPointer);
	virtual ~asCGeneric();

// interface - begin
	asIScriptEngine *GetEngine();

	void   *GetObject();
	asDWORD GetArgDWord(asUINT arg);
	asQWORD GetArgQWord(asUINT arg);
	float   GetArgFloat(asUINT arg);
	double  GetArgDouble(asUINT arg);
	void   *GetArgObject(asUINT arg);

	int     SetReturnDWord(asDWORD val);
	int     SetReturnQWord(asQWORD val);
	int     SetReturnFloat(float val);
	int     SetReturnDouble(double val);
	int     SetReturnObject(void *obj);
// interface - end

	asCScriptEngine *engine;
	asCScriptFunction *sysFunction;
	void *currentObject;
	asDWORD *stackPointer;
	void *objectRegister;

	asQWORD returnVal;
};

END_AS_NAMESPACE

#endif
