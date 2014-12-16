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
// as_typeinfo.h
//
// This class holds extra type info for the compiler
//



#ifndef AS_TYPEINFO_H
#define AS_TYPEINFO_H

#include "as_config.h"
#include "as_datatype.h"

BEGIN_AS_NAMESPACE

struct asCTypeInfo
{
	asCTypeInfo();
	void Set(const asCDataType &dataType);

	void SetVariable(const asCDataType &dataType, int stackOffset, bool isTemporary);
	void SetConstantB(const asCDataType &dataType, asBYTE value);
	void SetConstantQW(const asCDataType &dataType, asQWORD value);
	void SetConstantDW(const asCDataType &dataType, asDWORD value);
	void SetConstantF(const asCDataType &dataType, float value);
	void SetConstantD(const asCDataType &dataType, double value);
	void SetNullConstant();
	void SetDummy();

	bool IsNullConstant();

	asCDataType dataType;
	bool  isTemporary      :  1;
	bool  isConstant       :  1;
	bool  isVariable       :  1;
	bool  isExplicitHandle :  1;
	short dummy            : 12;
	short stackOffset;
	union 
	{	
		asQWORD qwordValue;
		double  doubleValue;
		asDWORD dwordValue;
		float   floatValue;
		int     intValue;
		asWORD  wordValue;
		asBYTE  byteValue;
	};
};

END_AS_NAMESPACE

#endif
