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
// as_outputbuffer.h
//
// This class appends strings to one large buffer that can later
// be sent to the real output stream
//


#ifndef AS_OUTPUTBUFFER_H
#define AS_OUTPUTBUFFER_H

#include "as_config.h"
#include "as_string.h"
#include "as_array.h"

BEGIN_AS_NAMESPACE

struct asSSystemFunctionInterface;
class asCScriptEngine;

class asCOutputBuffer
{
public:
	~asCOutputBuffer ();
	void Clear();
	void Callback(asSMessageInfo *msg);
	void Append(asCOutputBuffer &in);
	void SendToCallback(asCScriptEngine *engine, asSSystemFunctionInterface *func, void *obj);

	struct message_t
	{
		asCString section;
		int row;
		int col;
		asEMsgType type;
		asCString msg;
	};

	asCArray<message_t*> messages;
};

END_AS_NAMESPACE

#endif
