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
// as_typeinfo.cpp
//
// This class holds extra type info for the compiler
//

#include "as_config.h"
#include "as_typeinfo.h"

BEGIN_AS_NAMESPACE

asCTypeInfo::asCTypeInfo()
{
	isTemporary           = false;
	stackOffset           = 0;
	isConstant            = false;
	isVariable            = false;
	isExplicitHandle      = false;
	qwordValue            = 0;
}

void asCTypeInfo::Set(const asCDataType &dt)
{
	dataType         = dt;

	isTemporary      = false;
	stackOffset      = 0;
	isConstant       = false;
	isVariable       = false;
	isExplicitHandle = false;
	qwordValue       = 0;
}

void asCTypeInfo::SetVariable(const asCDataType &dt, int stackOffset, bool isTemporary)
{
	Set(dt);

	this->isVariable  = true;
	this->isTemporary = isTemporary;
	this->stackOffset = (short)stackOffset;
}

void asCTypeInfo::SetConstantQW(const asCDataType &dt, asQWORD value)
{
	Set(dt);

	isConstant = true;
	qwordValue = value;
}

void asCTypeInfo::SetConstantDW(const asCDataType &dt, asDWORD value)
{
	Set(dt);

	isConstant = true;
	dwordValue = value;
}

void asCTypeInfo::SetConstantB(const asCDataType &dt, asBYTE value)
{
	Set(dt);

	isConstant = true;
	byteValue = value;
}

void asCTypeInfo::SetConstantF(const asCDataType &dt, float value)
{
	Set(dt);

	isConstant = true;
	floatValue = value;
}

void asCTypeInfo::SetConstantD(const asCDataType &dt, double value)
{
	Set(dt);

	isConstant = true;
	doubleValue = value;
}

void asCTypeInfo::SetNullConstant()
{
	Set(asCDataType::CreateNullHandle());
	isConstant       = true;
	isExplicitHandle = true;
	qwordValue       = 0;
}

void asCTypeInfo::SetDummy()
{
	SetConstantQW(asCDataType::CreatePrimitive(ttInt, true), 0);
}

bool asCTypeInfo::IsNullConstant()
{
	if( isConstant && dataType.IsObjectHandle() )
		return true;

	return false;
}

END_AS_NAMESPACE
