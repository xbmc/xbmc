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
// as_objecttype.cpp
//
// A class for storing object type information
//

#include <stdio.h>

#include "as_config.h"
#include "as_objecttype.h"

BEGIN_AS_NAMESPACE

asCObjectType::asCObjectType()
{
	engine   = 0; 
	refCount = 0; 
	subType  = 0;
}

asCObjectType::asCObjectType(asCScriptEngine *engine) 
{
	this->engine = engine; 
	refCount     = 0; 
	subType      = 0;
}


asCObjectType::~asCObjectType()
{
	if( subType )
		subType->refCount--;
		
	for( asUINT n = 0; n < properties.GetLength(); n++ )
		if( properties[n] ) delete properties[n];

	properties.SetLength(0);

	methods.SetLength(0);
}

END_AS_NAMESPACE



