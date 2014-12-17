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
// as_outputbuffer.cpp
//
// This class appends strings to one large buffer that can later
// be sent to the real output stream
//

#include "as_config.h"
#include "as_outputbuffer.h"
#include "as_scriptengine.h"

BEGIN_AS_NAMESPACE

asCOutputBuffer::~asCOutputBuffer()
{
	Clear();
}

void asCOutputBuffer::Clear()
{
	for( asUINT n = 0; n < messages.GetLength(); n++ )
	{
		if( messages[n] ) 
		{
			asDELETE(messages[n],message_t);
		}
	}
	messages.SetLength(0);
}

void asCOutputBuffer::Callback(asSMessageInfo *msg)
{
	message_t *msgInfo = asNEW(message_t);
	msgInfo->section = msg->section;
	msgInfo->row = msg->row;
	msgInfo->col = msg->col;
	msgInfo->type = msg->type;
	msgInfo->msg = msg->message;

	messages.PushLast(msgInfo);
}

void asCOutputBuffer::Append(asCOutputBuffer &in)
{
	for( asUINT n = 0; n < in.messages.GetLength(); n++ )
		messages.PushLast(in.messages[n]);
	in.messages.SetLength(0);
}

void asCOutputBuffer::SendToCallback(asCScriptEngine *engine, asSSystemFunctionInterface *func, void *obj)
{
	for( asUINT n = 0; n < messages.GetLength(); n++ )
	{
		asSMessageInfo msg;
		msg.section = messages[n]->section.AddressOf();
		msg.row     = messages[n]->row;
		msg.col     = messages[n]->col;
		msg.type    = messages[n]->type;
		msg.message = messages[n]->msg.AddressOf();

		if( func->callConv < ICC_THISCALL )
			engine->CallGlobalFunction(&msg, obj, func, 0);
		else
			engine->CallObjectMethod(obj, &msg, func, 0);
	}
	Clear();
}

END_AS_NAMESPACE
