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
// as_scriptcode.h
//
// A container class for the script code to be compiled
//



#ifndef AS_SCRIPTCODE_H
#define AS_SCRIPTCODE_H

#include "as_array.h"
#include "as_string.h"

BEGIN_AS_NAMESPACE

class asCScriptCode
{
public:
	asCScriptCode();
	~asCScriptCode();

	int SetCode(const char *name, const char *code, bool makeCopy);
	int SetCode(const char *name, const char *code, int length, bool makeCopy);

	void ConvertPosToRowCol(int pos, int *row, int *col);

	asCString name;
	const char *code;
	int   codeLength;
	bool  sharedCode;
	int idx;
	int lineOffset;
	asCArray<int> linePositions;
};

END_AS_NAMESPACE

#endif
