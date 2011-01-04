/*
   AngelCode Scripting Library
   Copyright (c) 2003-2007 Andreas Jonsson

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

   Andreas Jonsson
   andreas@angelcode.com
*/


//
// as_variablescope.h
//
// A manager class for variable declarations
//


#ifndef AS_VARIABLESCOPE_H
#define AS_VARIABLESCOPE_H

#include "as_array.h"
#include "as_string.h"
#include "as_datatype.h"

BEGIN_AS_NAMESPACE

struct sVariable
{
	asCString name;
	asCDataType type;
	int stackOffset;
	bool isInitialized;
	bool isPureConstant;
	asQWORD constantValue;
};

class asCVariableScope
{
public:
	asCVariableScope(asCVariableScope *parent);
	~asCVariableScope();

	void Reset();

	int DeclareVariable(const char *name, const asCDataType &type, int stackOffset);
	sVariable *GetVariable(const char *name);
	sVariable *GetVariableByOffset(int offset);

	asCVariableScope *parent;

	bool isBreakScope;
	bool isContinueScope;

	asCArray<sVariable *> variables;
};

END_AS_NAMESPACE

#endif
