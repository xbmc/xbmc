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
// as_scriptnode.cpp
//
// A node in the script tree built by the parser for compilation
//



#include "as_scriptnode.h"
#include "as_scriptengine.h"

BEGIN_AS_NAMESPACE

asCScriptNode::asCScriptNode(eScriptNode type)
{
	nodeType    = type;
	tokenType   = ttUnrecognizedToken;
	tokenPos    = 0;
	tokenLength = 0;

	parent      = 0;
	next        = 0;
	prev        = 0;
	firstChild  = 0;
	lastChild   = 0;
}

void asCScriptNode::Destroy(asCScriptEngine *engine)
{
	// Destroy all children
	asCScriptNode *node = firstChild;
	asCScriptNode *next;

	while( node )
	{
		next = node->next;
		node->Destroy(engine);
		node = next;
	}

	// Return the memory to the memory manager
	engine->memoryMgr.FreeScriptNode(this);
}

void asCScriptNode::SetToken(sToken *token)
{
	tokenType   = token->type;
}

void asCScriptNode::UpdateSourcePos(size_t pos, size_t length)
{
	if( pos == 0 && length == 0 ) return;

	if( tokenPos == 0 && tokenLength == 0 )
	{
		tokenPos = pos;
		tokenLength = length;
	}
	else
	{
		if( tokenPos > pos )
		{
			tokenLength = tokenPos + tokenLength - pos;
			tokenPos = pos;
		}

		if( pos + length > tokenPos + tokenLength )
		{
			tokenLength = pos + length - tokenPos;
		}
	}
}

void asCScriptNode::AddChildLast(asCScriptNode *node)
{
	if( lastChild )
	{
		lastChild->next = node;
		node->next      = 0;
		node->prev      = lastChild;
		node->parent    = this;
		lastChild       = node;
	}
	else
	{
		firstChild   = node;
		lastChild    = node;
		node->next   = 0;
		node->prev   = 0;
		node->parent = this;
	}

	UpdateSourcePos(node->tokenPos, node->tokenLength);
}

void asCScriptNode::DisconnectParent()
{
	if( parent )
	{
		if( parent->firstChild == this )
			parent->firstChild = next;
		if( parent->lastChild == this )
			parent->lastChild = prev;
	}

	if( next )
		next->prev = prev;

	if( prev )
		prev->next = next;

	parent = 0;
	next = 0;
	prev = 0;
}

END_AS_NAMESPACE

