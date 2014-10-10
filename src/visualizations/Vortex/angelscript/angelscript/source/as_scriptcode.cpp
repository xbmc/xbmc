/*
   AngelCode Scripting Library
   Copyright (c) 2003-2008 Andreas Jonsson

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
// as_scriptcode.cpp
//
// A container class for the script code to be compiled
//



#include "as_config.h"
#include "as_scriptcode.h"

BEGIN_AS_NAMESPACE

asCScriptCode::asCScriptCode()
{
	lineOffset = 0;
	code = 0;
	codeLength = 0;
	sharedCode = false;
}

asCScriptCode::~asCScriptCode()
{
	if( !sharedCode && code ) 
	{
		asDELETEARRAY(code);
	}
}

int asCScriptCode::SetCode(const char *name, const char *code, bool makeCopy)
{
	return SetCode(name, code, strlen(code), makeCopy);
}

int asCScriptCode::SetCode(const char *name, const char *code, size_t length, bool makeCopy)
{
	this->name = name;
	if( !sharedCode && this->code ) 
	{
		asDELETEARRAY(this->code);
	}
	if( length == 0 )
		length = strlen(code);
	if( makeCopy )
	{
		this->code = asNEWARRAY(char,length);
		memcpy((char*)this->code, code, length);
		codeLength = length;
		sharedCode = false;
	}
	else
	{
		codeLength = length;
		this->code = const_cast<char*>(code);
		sharedCode = true;
	}

	// Find the positions of each line
	linePositions.PushLast(0);
	for( size_t n = 0; n < length; n++ )
		if( code[n] == '\n' ) linePositions.PushLast(n+1);
	linePositions.PushLast(length);

	return 0;
}

void asCScriptCode::ConvertPosToRowCol(size_t pos, int *row, int *col)
{
	if( linePositions.GetLength() == 0 ) 
	{
		if( row ) *row = lineOffset;
		if( col ) *col = 1;
		return;
	}

	// Do a binary search in the buffer
	int max = (int)linePositions.GetLength() - 1;
	int min = 0;
	int i = max/2;

	for(;;)
	{
		if( linePositions[i] < pos )
		{
			// Have we found the largest number < programPosition?
			if( min == i ) break;

			min = i;
			i = (max + min)/2;
		}
		else if( linePositions[i] > pos )
		{
			// Have we found the smallest number > programPoisition?
			if( max == i ) break;

			max = i;
			i = (max + min)/2;
		}
		else
		{
			// We found the exact position
			break;
		}
	}

	if( row ) *row = i + 1 + lineOffset;
	if( col ) *col = (int)(pos - linePositions[i]) + 1;
}

END_AS_NAMESPACE
