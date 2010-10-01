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
// as_parser.cpp
//
// This class parses the script code and builds a tree for compilation
//



#include "as_config.h"
#include "as_parser.h"
#include "as_tokendef.h"
#include "as_texts.h"

BEGIN_AS_NAMESPACE

asCParser::asCParser(asCBuilder *builder)
{
	this->builder    = builder;

	script			= 0;
	scriptNode		= 0;
}

asCParser::~asCParser()
{
	Reset();
}

void asCParser::Reset()
{
	errorWhileParsing = false;
	isSyntaxError     = false;

	sourcePos = 0;

	if( scriptNode )
		delete scriptNode;

	scriptNode = 0;

	script = 0;
}

asCScriptNode *asCParser::GetScriptNode()
{
	return scriptNode;
}

int asCParser::ParseScript(asCScriptCode *script)
{
	Reset();

	this->script = script;

	scriptNode = ParseScript();

	if( errorWhileParsing )
		return -1;

	return 0;
}

int asCParser::ParseFunctionDefinition(asCScriptCode *script)
{
	Reset();

	this->script = script;

	scriptNode = ParseFunctionDefinition();

	if( errorWhileParsing )
		return -1;

	return 0;
}

int asCParser::ParseDataType(asCScriptCode *script)
{
	Reset();

	this->script = script;

	scriptNode = new asCScriptNode(snDataType);
		
	scriptNode->AddChildLast(ParseType(false));
	if( isSyntaxError ) return -1;

	if( errorWhileParsing )
		return -1;

	return 0;
}

int asCParser::ParsePropertyDeclaration(asCScriptCode *script)
{
	Reset();

	this->script = script;

	scriptNode = new asCScriptNode(snDeclaration);

	scriptNode->AddChildLast(ParseType(true));
	if( isSyntaxError ) return -1;

	scriptNode->AddChildLast(ParseIdentifier());
	if( isSyntaxError ) return -1;

	return 0;
}

asCScriptNode *asCParser::ParseImport()
{
	asCScriptNode *node = new asCScriptNode(snImport);

	sToken t;
	GetToken(&t);
	if( t.type != ttImport )
	{
		Error(ExpectedToken(asGetTokenDefinition(ttImport)).AddressOf(), &t);
		return node;
	}

	node->SetToken(&t);
	node->UpdateSourcePos(t.pos, t.length);

	node->AddChildLast(ParseFunctionDefinition());
	if( isSyntaxError ) return node;

	GetToken(&t);
	if( t.type != ttIdentifier )
	{
		Error(ExpectedToken("from").AddressOf(), &t);
		return node;
	}

	asCString str;
	str.Assign(&script->code[t.pos], t.length);
	if( str != "from" )
	{
		Error(ExpectedToken("from").AddressOf(), &t);
		return node;
	}

	node->UpdateSourcePos(t.pos, t.length);

	GetToken(&t);
	if( t.type != ttStringConstant )
	{
		Error(TXT_EXPECTED_STRING, &t);
		return node;
	}

	asCScriptNode *mod = new asCScriptNode(snConstant);
	node->AddChildLast(mod);

	mod->SetToken(&t);
	mod->UpdateSourcePos(t.pos, t.length);

	GetToken(&t);
	if( t.type != ttEndStatement )
	{
		Error(ExpectedToken(asGetTokenDefinition(ttEndStatement)).AddressOf(), &t);
		return node;
	}

	node->UpdateSourcePos(t.pos, t.length);

	return node;
}

asCScriptNode *asCParser::ParseFunctionDefinition()
{
	asCScriptNode *node = new asCScriptNode(snFunction);

	node->AddChildLast(ParseType(true));
	if( isSyntaxError ) return node;

	node->AddChildLast(ParseTypeMod(false));
	if( isSyntaxError ) return node;

	node->AddChildLast(ParseIdentifier());
	if( isSyntaxError ) return node;

	node->AddChildLast(ParseParameterList());
	if( isSyntaxError ) return node;

	// Parse an optional const after the function definition (used for object methods)
	sToken t1;
	GetToken(&t1);
	RewindTo(&t1);
	if( t1.type == ttConst )
		node->AddChildLast(ParseToken(ttConst));

	return node;
}

asCScriptNode *asCParser::ParseScript()
{
	asCScriptNode *node = new asCScriptNode(snScript);

	// Determine type of node
	sToken t1;

	for(;;)
	{
		while( !isSyntaxError )
		{
			GetToken(&t1);
			RewindTo(&t1);

			if( t1.type == ttImport )
				node->AddChildLast(ParseImport());
			else if( t1.type == ttStruct )
				node->AddChildLast(ParseStruct());
			else if( t1.type == ttConst )
				node->AddChildLast(ParseGlobalVar());
			else if( IsDataType(t1.type) )
			{
				if( IsGlobalVar() )
					node->AddChildLast(ParseGlobalVar());
				else
					node->AddChildLast(ParseFunction());
			}
			else if( t1.type == ttEnd )
				return node;
			else
			{
				asCString str;
				const char *t = asGetTokenDefinition(t1.type);
				if( t == 0 ) t = "<unknown token>";

				str.Format(TXT_UNEXPECTED_TOKEN_s, t);

				Error(str.AddressOf(), &t1);
			}
		}

		if( isSyntaxError )
		{
			// Search for either ';' or '{' or end
			GetToken(&t1);
			while( t1.type != ttEndStatement && t1.type != ttEnd &&
				   t1.type != ttStartStatementBlock )
				GetToken(&t1);

			if( t1.type == ttStartStatementBlock )
			{
				// Find the end of the block and skip nested blocks
				int level = 1;
				while( level > 0 )
				{
					GetToken(&t1);
					if( t1.type == ttStartStatementBlock ) level++;
					if( t1.type == ttEndStatementBlock ) level--;
					if( t1.type == ttEnd ) break;
				}
			}

			isSyntaxError = false;
		}
	}
	return 0;
}

bool asCParser::IsGlobalVar()
{
	// Set start point so that we can rewind
	sToken t;
	GetToken(&t);
	RewindTo(&t);
	
	// A global variable can start with a const
	sToken t1;
	GetToken(&t1);
	if( t1.type == ttConst )
		GetToken(&t1);

	if( !IsDataType(t1.type) )
	{
		RewindTo(&t);
		return false;
	}

	// TODO: Object handles can be interleaved with the array brackets

	sToken t2;
	GetToken(&t2);
	while( t2.type == ttOpenBracket )
	{
		GetToken(&t2);
		if( t2.type != ttCloseBracket )
			return false;
		GetToken(&t2);
	}

	if( t2.type == ttHandle )
		GetToken(&t2);

	if( t2.type != ttIdentifier )
	{
		RewindTo(&t);
		return false;
	}

	GetToken(&t2);
	if( t2.type == ttEndStatement || t2.type == ttAssignment || t2.type == ttListSeparator )
	{
		RewindTo(&t);
		return true;
	}
	if( t2.type == ttOpenParanthesis ) 
	{	
		// If the closing paranthesis is followed by a statement 
		// block or end-of-file, then treat it as a function. 
		while( t2.type != ttCloseParanthesis && t2.type != ttEnd )
			GetToken(&t2);

		if( t2.type == ttEnd ) 
			return false;
		else
		{
			GetToken(&t1);
			RewindTo(&t);
			if( t1.type == ttStartStatementBlock || t1.type == ttEnd )
				return false;
		}

		RewindTo(&t);

		return true;
	}

	RewindTo(&t);
	return false;
}

asCScriptNode *asCParser::ParseFunction()
{
	asCScriptNode *node = new asCScriptNode(snFunction);

	node->AddChildLast(ParseType(false));
	if( isSyntaxError ) return node;

	node->AddChildLast(ParseTypeMod(false));
	if( isSyntaxError ) return node;

	node->AddChildLast(ParseIdentifier());
	if( isSyntaxError ) return node;

	node->AddChildLast(ParseParameterList());
	if( isSyntaxError ) return node;

	node->AddChildLast(ParseStatementBlock());

	return node;
}

asCScriptNode *asCParser::ParseStruct()
{
	asCScriptNode *node = new asCScriptNode(snStruct);

	sToken t;
	GetToken(&t);
	if( t.type != ttStruct )
	{
		Error(ExpectedToken("struct").AddressOf(), &t);
		return node;
	}

	node->SetToken(&t);

	node->AddChildLast(ParseIdentifier());

	GetToken(&t);
	if( t.type != ttStartStatementBlock )
	{
		Error(ExpectedToken("{").AddressOf(), &t);
		return node;
	}

	// Parse properties
	GetToken(&t);
	RewindTo(&t);
	while( t.type != ttEndStatementBlock )
	{
		// Parse a property declaration
		asCScriptNode *prop = new asCScriptNode(snDeclaration);
		node->AddChildLast(prop);

		prop->AddChildLast(ParseType(true));
		if( isSyntaxError ) return node;

		prop->AddChildLast(ParseIdentifier());
		if( isSyntaxError ) return node;

		GetToken(&t);
		if( t.type != ttEndStatement )
		{
			Error(ExpectedToken(";").AddressOf(), &t);
			return node;
		}
		prop->UpdateSourcePos(t.pos, t.length);

		GetToken(&t);
		RewindTo(&t);
	}

	GetToken(&t);
	if( t.type != ttEndStatementBlock )
	{
		Error(ExpectedToken("}").AddressOf(), &t);
		return node;
	}

	GetToken(&t);
	if( t.type != ttEndStatement )
	{
		Error(ExpectedToken(";").AddressOf(), &t);
		return node;
	}

	node->UpdateSourcePos(t.pos, t.length);

	return node;
}

asCScriptNode *asCParser::ParseGlobalVar()
{
	asCScriptNode *node = new asCScriptNode(snGlobalVar);

	// Parse data type
	node->AddChildLast(ParseType(true));
	if( isSyntaxError ) return node;

	sToken t;

	for(;;)
	{
		// Parse identifier
		node->AddChildLast(ParseIdentifier());
		if( isSyntaxError ) return node;

		// If next token is assignment, parse expression
		GetToken(&t);
		if( t.type == ttAssignment )
		{
			GetToken(&t);
			RewindTo(&t);
			if( t.type == ttStartStatementBlock )
			{
				node->AddChildLast(ParseInitList());
				if( isSyntaxError ) return node;
			}
			else
			{
				node->AddChildLast(ParseAssignment());
				if( isSyntaxError ) return node;
			}
		}
		else if( t.type == ttOpenParanthesis ) 
		{
			RewindTo(&t);
			node->AddChildLast(ParseArgList());
			if( isSyntaxError ) return node;
		}
		else
			RewindTo(&t);

		// continue if list separator, else terminate with end statement
		GetToken(&t);
		if( t.type == ttListSeparator )
			continue;
		else if( t.type == ttEndStatement )
		{
			node->UpdateSourcePos(t.pos, t.length);

			return node;
		}
		else
		{
			Error(ExpectedTokens(",", ";").AddressOf(), &t);
			return node;
		}
	}
	return 0;
}

asCScriptNode *asCParser::ParseTypeMod(bool isParam)
{
	asCScriptNode *node = new asCScriptNode(snDataType);

	sToken t;

	// Parse possible & token
	GetToken(&t);
	RewindTo(&t);
	if( t.type == ttAmp )
	{
		node->AddChildLast(ParseToken(ttAmp));
		if( isSyntaxError ) return node;

		if( isParam )
		{
#ifdef AS_ALLOW_UNSAFE_REFERENCES
			GetToken(&t);
			RewindTo(&t);

			if( t.type == ttIn || t.type == ttOut || t.type == ttInOut )
#endif
			{
				int tokens[3] = {ttIn, ttOut, ttInOut};
				node->AddChildLast(ParseOneOf(tokens, 3));
			}
		}
	}

	// Parse possible + token 
	GetToken(&t);
	RewindTo(&t);
	if( t.type == ttPlus )
	{
		node->AddChildLast(ParseToken(ttPlus));
		if( isSyntaxError ) return node;
	}

	return node;
}

asCScriptNode *asCParser::ParseType(bool allowConst)
{
	asCScriptNode *node = new asCScriptNode(snDataType);

	sToken t;

	if( allowConst )
	{
		GetToken(&t);
		RewindTo(&t);
		if( t.type == ttConst )
		{
			node->AddChildLast(ParseToken(ttConst));
			if( isSyntaxError ) return node;
		}
	}

	node->AddChildLast(ParseDataType());

	// Parse [] and @
	GetToken(&t);
	RewindTo(&t);
	while( t.type == ttOpenBracket || t.type == ttHandle)
	{
		if( t.type == ttOpenBracket )
		{
			node->AddChildLast(ParseToken(ttOpenBracket));
			if( isSyntaxError ) return node;

			GetToken(&t);
			if( t.type != ttCloseBracket )
			{
				Error(ExpectedToken("]").AddressOf(), &t);
				return node;
			}
		}
		else
		{
			node->AddChildLast(ParseToken(ttHandle));
			if( isSyntaxError ) return node;
		}

		GetToken(&t);
		RewindTo(&t);
	}

	return node;
}

asCScriptNode *asCParser::ParseToken(int token)
{
	asCScriptNode *node = new asCScriptNode(snUndefined);

	sToken t1;

	GetToken(&t1);
	if( t1.type != token )
	{
		Error(ExpectedToken(asGetTokenDefinition(token)).AddressOf(), &t1);
		return node;
	}

	node->SetToken(&t1);
	node->UpdateSourcePos(t1.pos, t1.length);

	return node;
}

asCScriptNode *asCParser::ParseOneOf(int *tokens, int count)
{
	asCScriptNode *node = new asCScriptNode(snUndefined);

	sToken t1;

	GetToken(&t1);
	int n;
	for( n = 0; n < count; n++ )
	{
		if( tokens[n] == t1.type )
			break;
	}
	if( n == count )
	{
		Error(ExpectedOneOf(tokens, count).AddressOf(), &t1);
		return node;
	}

	node->SetToken(&t1);
	node->UpdateSourcePos(t1.pos, t1.length);

	return node;
}


asCScriptNode *asCParser::ParseDataType()
{
	asCScriptNode *node = new asCScriptNode(snDataType);

	sToken t1;

	GetToken(&t1);
	if( !IsDataType(t1.type) )
	{
		Error(TXT_EXPECTED_DATA_TYPE, &t1);
		return node;
	}

	node->SetToken(&t1);
	node->UpdateSourcePos(t1.pos, t1.length);

	return node;
}

asCScriptNode *asCParser::ParseRealType()
{
	asCScriptNode *node = new asCScriptNode(snDataType);

	sToken t1;

	GetToken(&t1);
	if( !IsRealType(t1.type) )
	{
		Error(TXT_EXPECTED_DATA_TYPE, &t1);
		return node;
	}

	node->SetToken(&t1);
	node->UpdateSourcePos(t1.pos, t1.length);

	return node;
}

asCScriptNode *asCParser::ParseIdentifier()
{
	asCScriptNode *node = new asCScriptNode(snIdentifier);

	sToken t1;

	GetToken(&t1);
	if( t1.type != ttIdentifier )
	{
		Error(TXT_EXPECTED_IDENTIFIER, &t1);
		return node;
	}

	node->SetToken(&t1);
	node->UpdateSourcePos(t1.pos, t1.length);

	return node;
}

asCScriptNode *asCParser::ParseParameterList()
{
	asCScriptNode *node = new asCScriptNode(snParameterList);

	sToken t1;
	GetToken(&t1);
	if( t1.type != ttOpenParanthesis )
	{
		Error(ExpectedToken("(").AddressOf(), &t1);
		return node;
	}

	node->UpdateSourcePos(t1.pos, t1.length);

	GetToken(&t1);
	if( t1.type == ttCloseParanthesis )
	{
		node->UpdateSourcePos(t1.pos, t1.length);

		// Statement block is finished
		return node;
	}
	else
	{
		RewindTo(&t1);

		for(;;)
		{
			// Parse data type
			node->AddChildLast(ParseType(true));
			if( isSyntaxError ) return node;

			node->AddChildLast(ParseTypeMod(true));
			if( isSyntaxError ) return node;

			// Parse identifier
			GetToken(&t1);
			if( t1.type == ttIdentifier )
			{
				RewindTo(&t1);

				node->AddChildLast(ParseIdentifier());
				if( isSyntaxError ) return node;

				GetToken(&t1);
			}

			// Check if list continues
			if( t1.type == ttCloseParanthesis )
			{
				node->UpdateSourcePos(t1.pos, t1.length);

				return node;
			}
			else if( t1.type == ttListSeparator )
				continue;
			else
			{
				Error(ExpectedTokens(")", ",").AddressOf(), &t1);
				return node;
			}
		}
	}
	return 0;
}

asCScriptNode *asCParser::ParseExprValue()
{
	asCScriptNode *node = new asCScriptNode(snExprValue);

	sToken t1;
	GetToken(&t1);
	RewindTo(&t1);

	if( t1.type == ttIdentifier || IsRealType(t1.type) )
	{
		if( IsFunctionCall() )
			node->AddChildLast(ParseFunctionCall());
		else
			node->AddChildLast(ParseIdentifier());
	}
	else if( IsConstant(t1.type) )
		node->AddChildLast(ParseConstant());
	else if( t1.type == ttOpenParanthesis )
	{
		GetToken(&t1);
		node->UpdateSourcePos(t1.pos, t1.length);

		node->AddChildLast(ParseAssignment());
		if( isSyntaxError ) return node;

		GetToken(&t1);
		if( t1.type != ttCloseParanthesis )
			Error(ExpectedToken(")").AddressOf(), &t1);

		node->UpdateSourcePos(t1.pos, t1.length);
	}
	else
		Error(TXT_EXPECTED_EXPRESSION_VALUE, &t1);

	return node;
}

asCScriptNode *asCParser::ParseConstant()
{
	asCScriptNode *node = new asCScriptNode(snConstant);

	sToken t;
	GetToken(&t);
	if( !IsConstant(t.type) )
	{
		Error(TXT_EXPECTED_CONSTANT, &t);
		return node;
	}

	node->SetToken(&t);
	node->UpdateSourcePos(t.pos, t.length);

	// We want to gather a list of string constants to concatenate as children
	if( t.type == ttStringConstant || t.type == ttHeredocStringConstant )
		RewindTo(&t);

	while( t.type == ttStringConstant || t.type == ttHeredocStringConstant )
	{
		node->AddChildLast(ParseStringConstant());

		GetToken(&t);
		RewindTo(&t);
	}

	return node;
}

asCScriptNode *asCParser::ParseStringConstant()
{
	asCScriptNode *node = new asCScriptNode(snConstant);

	sToken t;
	GetToken(&t);
	if( t.type != ttStringConstant && t.type != ttHeredocStringConstant )
	{
		Error(TXT_EXPECTED_STRING, &t);
		return node;
	}

	node->SetToken(&t);
	node->UpdateSourcePos(t.pos, t.length);

	return node;
}

asCScriptNode *asCParser::ParseFunctionCall()
{
	asCScriptNode *node = new asCScriptNode(snFunctionCall);

	node->AddChildLast(ParseType(false));
	if( isSyntaxError ) return node;

	node->AddChildLast(ParseArgList());

	return node;
}

asCScriptNode *asCParser::ParseArgList()
{
	asCScriptNode *node = new asCScriptNode(snArgList);

	sToken t1;
	GetToken(&t1);
	if( t1.type != ttOpenParanthesis )
	{
		Error(ExpectedToken("(").AddressOf(), &t1);
		return node;
	}

	node->UpdateSourcePos(t1.pos, t1.length);

	GetToken(&t1);
	if( t1.type == ttCloseParanthesis )
	{
		node->UpdateSourcePos(t1.pos, t1.length);

		// Statement block is finished
		return node;
	}
	else
	{
		RewindTo(&t1);

		for(;;)
		{
			node->AddChildLast(ParseAssignment());
			if( isSyntaxError ) return node;

			// Check if list continues
			GetToken(&t1);
			if( t1.type == ttCloseParanthesis )
			{
				node->UpdateSourcePos(t1.pos, t1.length);

				return node;
			}
			else if( t1.type == ttListSeparator )
				continue;
			else
			{
				Error(ExpectedTokens(")", ",").AddressOf(), &t1);
				return node;
			}
		}
	}
	return 0;
}

asCScriptNode *asCParser::ParseStatementBlock()
{
	asCScriptNode *node = new asCScriptNode(snStatementBlock);

	sToken t1;

	GetToken(&t1);
	if( t1.type != ttStartStatementBlock )
	{
		Error(ExpectedToken("{").AddressOf(), &t1);
		return node;
	}

	node->UpdateSourcePos(t1.pos, t1.length);

	for(;;)
	{
		while( !isSyntaxError )
		{
			GetToken(&t1);
			if( t1.type == ttEndStatementBlock )
			{
				node->UpdateSourcePos(t1.pos, t1.length);

				// Statement block is finished
				return node;
			}
			else
			{
				RewindTo(&t1);

				if( IsDeclaration() )
					node->AddChildLast(ParseDeclaration());
				else
					node->AddChildLast(ParseStatement());
			}
		}

		if( isSyntaxError )
		{
			// Search for either ';', '{', '}', or end
			GetToken(&t1);
			while( t1.type != ttEndStatement && t1.type != ttEnd &&
				   t1.type != ttStartStatementBlock && t1.type != ttEndStatementBlock )
			{
				GetToken(&t1);
			}

			// Skip this statement block
			if( t1.type == ttStartStatementBlock )
			{
				// Find the end of the block and skip nested blocks
				int level = 1;
				while( level > 0 )
				{
					GetToken(&t1);
					if( t1.type == ttStartStatementBlock ) level++;
					if( t1.type == ttEndStatementBlock ) level--;
					if( t1.type == ttEnd ) break;
				}
			}
			else if( t1.type == ttEndStatementBlock )
			{
				RewindTo(&t1);
			}
			else if( t1.type == ttEnd )
			{
				Error(TXT_UNEXPECTED_END_OF_FILE, &t1);
				return node;
			}

			isSyntaxError = false;
		}
	}
	return 0;
}

asCScriptNode *asCParser::ParseInitList()
{
	asCScriptNode *node = new asCScriptNode(snInitList);

	sToken t1;

	GetToken(&t1);
	if( t1.type != ttStartStatementBlock )
	{
		Error(ExpectedToken("{").AddressOf(), &t1);
		return node;
	}

	node->UpdateSourcePos(t1.pos, t1.length);

	GetToken(&t1);
	if( t1.type == ttEndStatementBlock )
	{
		node->UpdateSourcePos(t1.pos, t1.length);

		// Statement block is finished
		return node;
	}
	else
	{
		RewindTo(&t1);
		for(;;)
		{
			GetToken(&t1);
			if( t1.type == ttListSeparator )
			{
				// No expression 
				node->AddChildLast(new asCScriptNode(snUndefined));

				GetToken(&t1);
				if( t1.type == ttEndStatementBlock )
				{
					// No expression
					node->AddChildLast(new asCScriptNode(snUndefined));
					node->UpdateSourcePos(t1.pos, t1.length);
					return node;
				}
				RewindTo(&t1);
			}
			else if( t1.type == ttEndStatementBlock )
			{
				// No expression 
				node->AddChildLast(new asCScriptNode(snUndefined));

				node->UpdateSourcePos(t1.pos, t1.length);

				// Statement block is finished
				return node;
			}
			else if( t1.type == ttStartStatementBlock )
			{
				RewindTo(&t1);
				node->AddChildLast(ParseInitList());
				if( isSyntaxError ) return node;

				GetToken(&t1);
				if( t1.type == ttListSeparator )
					continue;
				else if( t1.type == ttEndStatementBlock )
				{
					node->UpdateSourcePos(t1.pos, t1.length);

					// Statement block is finished
					return node;
				}
				else
				{
					Error(ExpectedTokens("}", ",").AddressOf(), &t1);
					return node;
				}
			}
			else
			{
				RewindTo(&t1);
				node->AddChildLast(ParseAssignment());
				if( isSyntaxError ) return node;


				GetToken(&t1);
				if( t1.type == ttListSeparator )
					continue;
				else if( t1.type == ttEndStatementBlock )
				{
					node->UpdateSourcePos(t1.pos, t1.length);

					// Statement block is finished
					return node;
				}
				else
				{
					Error(ExpectedTokens("}", ",").AddressOf(), &t1);
					return node;
				}
			}
		}
	}
	return 0;
}

bool asCParser::IsDeclaration()
{
	sToken t1, t2;

	GetToken(&t1);

	if( t1.type == ttConst )
	{
		RewindTo(&t1);
		return true;
	}

	if( !IsDataType(t1.type) )
	{
		RewindTo(&t1);
		return false;
	}

	GetToken(&t2);
	if( t2.type == ttIdentifier )
	{
		RewindTo(&t1);
		return true;
	}

	// The data type can be followed by handle and array brackets
	while( t2.type == ttHandle || t2.type == ttOpenBracket )
	{
		if( t2.type == ttOpenBracket )
		{
			GetToken(&t2);
			if( t2.type != ttCloseBracket )
			{
				RewindTo(&t1);
				return false;
			}
		}

		GetToken(&t2);
	}

	if( t2.type == ttIdentifier )
	{
		RewindTo(&t1);
		return true;
	}

	RewindTo(&t1);
	return false;
}

bool asCParser::IsFunctionCall()
{
	sToken t1, t2;

	GetToken(&t1);

	if( t1.type != ttIdentifier && !IsRealType(t1.type) )
	{
		RewindTo(&t1);
		return false;
	}

	// The name can be followed by handle and closed array brackets
	GetToken(&t2);
	while( t2.type == ttHandle || t2.type == ttOpenBracket )
	{
		if( t2.type == ttOpenBracket )
		{
			GetToken(&t2);
			if( t2.type != ttCloseBracket )
			{
				RewindTo(&t1);
				return false;
			}
		}

		GetToken(&t2);
	}

	if( t2.type == ttOpenParanthesis )
	{
		RewindTo(&t1);
		return true;
	}

	RewindTo(&t1);
	return false;
}

asCScriptNode *asCParser::ParseDeclaration()
{
	asCScriptNode *node = new asCScriptNode(snDeclaration);

	// Parse data type
	node->AddChildLast(ParseType(true));
	if( isSyntaxError ) return node;

	sToken t;

	for(;;)
	{
		// Parse identifier
		node->AddChildLast(ParseIdentifier());
		if( isSyntaxError ) return node;

		// If next token is assignment, parse expression
		GetToken(&t);
		if( t.type == ttOpenParanthesis )
		{
			RewindTo(&t);
			node->AddChildLast(ParseArgList());
			if( isSyntaxError ) return node;
		}
		else if( t.type == ttAssignment )
		{
			GetToken(&t);
			RewindTo(&t);
			if( t.type == ttStartStatementBlock )
			{
				node->AddChildLast(ParseInitList());
				if( isSyntaxError ) return node;
			}
			else
			{
				node->AddChildLast(ParseAssignment());
				if( isSyntaxError ) return node;
			}
		}
		else
			RewindTo(&t);

		// continue if list separator, else terminate with end statement
		GetToken(&t);
		if( t.type == ttListSeparator )
			continue;
		else if( t.type == ttEndStatement )
		{
			node->UpdateSourcePos(t.pos, t.length);

			return node;
		}
		else
		{
			Error(ExpectedTokens(",", ";").AddressOf(), &t);
			return node;
		}
	}
	return 0;
}

asCScriptNode *asCParser::ParseStatement()
{
	sToken t1;

	GetToken(&t1);
	RewindTo(&t1);

	if( t1.type == ttIf )
		return ParseIf();
	else if( t1.type == ttFor )
		return ParseFor();
	else if( t1.type == ttWhile )
		return ParseWhile();
	else if( t1.type == ttReturn )
		return ParseReturn();
	else if( t1.type == ttStartStatementBlock )
		return ParseStatementBlock();
	else if( t1.type == ttBreak )
		return ParseBreak();
	else if( t1.type == ttContinue )
		return ParseContinue();
	else if( t1.type == ttDo )
		return ParseDoWhile();
	else if( t1.type == ttSwitch )
		return ParseSwitch();
	else
		return ParseExpressionStatement();
}

asCScriptNode *asCParser::ParseExpressionStatement()
{
	asCScriptNode *node = new asCScriptNode(snExpressionStatement);

	sToken t;
	GetToken(&t);
	if( t.type == ttEndStatement )
	{
		node->UpdateSourcePos(t.pos, t.length);

		return node;
	}

	RewindTo(&t);

	node->AddChildLast(ParseAssignment());
	if( isSyntaxError ) return node;

	GetToken(&t);
	if( t.type != ttEndStatement )
	{
		Error(ExpectedToken(";").AddressOf(), &t);
		return node;
	}

	node->UpdateSourcePos(t.pos, t.length);

	return node;
}

asCScriptNode *asCParser::ParseSwitch()
{
	asCScriptNode *node = new asCScriptNode(snSwitch);

	sToken t;
	GetToken(&t);
	if( t.type != ttSwitch )
	{
		Error(ExpectedToken("switch").AddressOf(), &t);
		return node;
	}

	node->UpdateSourcePos(t.pos, t.length);

	GetToken(&t);
	if( t.type != ttOpenParanthesis )
	{
		Error(ExpectedToken("(").AddressOf(), &t);
		return node;
	}

	node->AddChildLast(ParseAssignment());
	if( isSyntaxError ) return node;

	GetToken(&t);
	if( t.type != ttCloseParanthesis )
	{
		Error(ExpectedToken(")").AddressOf(), &t);
		return node;
	}

	GetToken(&t);
	if( t.type != ttStartStatementBlock )
	{
		Error(ExpectedToken("{").AddressOf(), &t);
		return node;
	}
	
	while( !isSyntaxError )
	{
		GetToken(&t);
		
		if( t.type == ttEndStatementBlock || t.type == ttDefault)
			break;

		RewindTo(&t);

		if( t.type != ttCase )
		{
			Error(ExpectedToken("case").AddressOf(), &t);
			return node;
		}

		node->AddChildLast(ParseCase());
		if( isSyntaxError ) return node;
	}

	if( t.type == ttDefault)
	{
		RewindTo(&t);

		node->AddChildLast(ParseCase());
		if( isSyntaxError ) return node;

		GetToken(&t);
	}

	if( t.type != ttEndStatementBlock )
	{
		Error(ExpectedToken("}").AddressOf(), &t);
		return node;
	}

	return node;
}

asCScriptNode *asCParser::ParseCase()
{
	asCScriptNode *node = new asCScriptNode(snCase);

	sToken t;
	GetToken(&t);
	if( t.type != ttCase && t.type != ttDefault )
	{
		Error(ExpectedTokens("case", "default").AddressOf(), &t);
		return node;
	}

	node->UpdateSourcePos(t.pos, t.length);

	if(t.type == ttCase)
	{
		node->AddChildLast(ParseExpression());
	}

	GetToken(&t);
	if( t.type != ttColon )
	{
		Error(ExpectedToken(":").AddressOf(), &t);
		return node;
	}

	// Parse statements until we find either of }, case, default, and break
	GetToken(&t);
	RewindTo(&t);
	while( t.type != ttCase && 
		   t.type != ttDefault && 
		   t.type != ttEndStatementBlock && 
		   t.type != ttBreak )
	{

		node->AddChildLast(ParseStatement());
		if( isSyntaxError ) return node;

		GetToken(&t);
		RewindTo(&t);
	}

	// If the case was ended with a break statement, add it to the node
	if( t.type == ttBreak )
		node->AddChildLast(ParseBreak());

	return node;
}

asCScriptNode *asCParser::ParseIf()
{
	asCScriptNode *node = new asCScriptNode(snIf);

	sToken t;
	GetToken(&t);
	if( t.type != ttIf )
	{
		Error(ExpectedToken("if").AddressOf(), &t);
		return node;
	}

	node->UpdateSourcePos(t.pos, t.length);

	GetToken(&t);
	if( t.type != ttOpenParanthesis )
	{
		Error(ExpectedToken("(").AddressOf(), &t);
		return node;
	}

	node->AddChildLast(ParseAssignment());
	if( isSyntaxError ) return node;

	GetToken(&t);
	if( t.type != ttCloseParanthesis )
	{
		Error(ExpectedToken(")").AddressOf(), &t);
		return node;
	}

	node->AddChildLast(ParseStatement());
	if( isSyntaxError ) return node;

	GetToken(&t);
	if( t.type != ttElse )
	{
		// No else statement return already
		RewindTo(&t);
		return node;
	}

	node->AddChildLast(ParseStatement());

	return node;
}

asCScriptNode *asCParser::ParseFor()
{
	asCScriptNode *node = new asCScriptNode(snFor);

	sToken t;
	GetToken(&t);
	if( t.type != ttFor )
	{
		Error(ExpectedToken("for").AddressOf(), &t);
		return node;
	}

	node->UpdateSourcePos(t.pos, t.length);

	GetToken(&t);
	if( t.type != ttOpenParanthesis )
	{
		Error(ExpectedToken("(").AddressOf(), &t);
		return node;
	}

	if( IsDeclaration() )
		node->AddChildLast(ParseDeclaration());
	else
		node->AddChildLast(ParseExpressionStatement());
	if( isSyntaxError ) return node;

	node->AddChildLast(ParseExpressionStatement());
	if( isSyntaxError ) return node;

	GetToken(&t);
	if( t.type != ttCloseParanthesis )
	{
		RewindTo(&t);

		node->AddChildLast(ParseAssignment());
		if( isSyntaxError ) return node;

		GetToken(&t);
		if( t.type != ttCloseParanthesis )
		{
			Error(ExpectedToken(")").AddressOf(), &t);
			return node;
		}
	}

	node->AddChildLast(ParseStatement());
	
	return node;
}




	
asCScriptNode *asCParser::ParseWhile()
{
	asCScriptNode *node = new asCScriptNode(snWhile);

	sToken t;
	GetToken(&t);
	if( t.type != ttWhile )
	{
		Error(ExpectedToken("while").AddressOf(), &t);
		return node;
	}

	node->UpdateSourcePos(t.pos, t.length);

	GetToken(&t);
	if( t.type != ttOpenParanthesis )
	{
		Error(ExpectedToken("(").AddressOf(), &t);
		return node;
	}

	node->AddChildLast(ParseAssignment());
	if( isSyntaxError ) return node;

	GetToken(&t);
	if( t.type != ttCloseParanthesis )
	{
		Error(ExpectedToken(")").AddressOf(), &t);
		return node;
	}

	node->AddChildLast(ParseStatement());

	return node;
}

asCScriptNode *asCParser::ParseDoWhile()
{
	asCScriptNode *node = new asCScriptNode(snDoWhile);

	sToken t;
	GetToken(&t);
	if( t.type != ttDo )
	{
		Error(ExpectedToken("do").AddressOf(), &t);
		return node;
	}

	node->UpdateSourcePos(t.pos, t.length);

	node->AddChildLast(ParseStatement());
	if( isSyntaxError ) return node;

	GetToken(&t);
	if( t.type != ttWhile )
	{
		Error(ExpectedToken("while").AddressOf(), &t);
		return node;
	}

	GetToken(&t);
	if( t.type != ttOpenParanthesis )
	{
		Error(ExpectedToken("(").AddressOf(), &t);
		return node;
	}

	node->AddChildLast(ParseAssignment());
	if( isSyntaxError ) return node;

	GetToken(&t);
	if( t.type != ttCloseParanthesis )
	{
		Error(ExpectedToken(")").AddressOf(), &t);
		return node;
	}

	GetToken(&t);
	if( t.type != ttEndStatement )
	{
		Error(ExpectedToken(";").AddressOf(), &t);
		return node;
	}
	node->UpdateSourcePos(t.pos, t.length);

	return node;
}

asCScriptNode *asCParser::ParseReturn()
{
	asCScriptNode *node = new asCScriptNode(snReturn);

	sToken t;
	GetToken(&t);
	if( t.type != ttReturn )
	{
		Error(ExpectedToken("return").AddressOf(), &t);
		return node;
	}

	node->UpdateSourcePos(t.pos, t.length);

	GetToken(&t);
	if( t.type == ttEndStatement )
	{
		node->UpdateSourcePos(t.pos, t.length);
		return node;
	}

	RewindTo(&t);

	node->AddChildLast(ParseAssignment());
	if( isSyntaxError ) return node;

	GetToken(&t);
	if( t.type != ttEndStatement )
	{
		Error(ExpectedToken(";").AddressOf(), &t);
		return node;
	}

	node->UpdateSourcePos(t.pos, t.length);

	return node;
}

asCScriptNode *asCParser::ParseBreak()
{
	asCScriptNode *node = new asCScriptNode(snBreak);

	sToken t;
	GetToken(&t);
	if( t.type != ttBreak )
	{
		Error(ExpectedToken("break").AddressOf(), &t);
		return node;
	}

	node->UpdateSourcePos(t.pos, t.length);

	GetToken(&t);
	if( t.type != ttEndStatement )
		Error(ExpectedToken(";").AddressOf(), &t);

	node->UpdateSourcePos(t.pos, t.length);

	return node;
}

asCScriptNode *asCParser::ParseContinue()
{
	asCScriptNode *node = new asCScriptNode(snContinue);

	sToken t;
	GetToken(&t);
	if( t.type != ttContinue )
	{
		Error(ExpectedToken("continue").AddressOf(), &t);
		return node;
	}

	node->UpdateSourcePos(t.pos, t.length);

	GetToken(&t);
	if( t.type != ttEndStatement )
		Error(ExpectedToken(";").AddressOf(), &t);

	node->UpdateSourcePos(t.pos, t.length);

	return node;
}

asCScriptNode *asCParser::ParseAssignment()
{
	asCScriptNode *node = new asCScriptNode(snAssignment);

	node->AddChildLast(ParseCondition());
	if( isSyntaxError ) return node;

	sToken t;
	GetToken(&t);
	RewindTo(&t);

	if( IsAssignOperator(t.type) )
	{
		node->AddChildLast(ParseAssignOperator());
		if( isSyntaxError ) return node;

		node->AddChildLast(ParseAssignment());
		if( isSyntaxError ) return node;
	}

	return node;
}

asCScriptNode *asCParser::ParseCondition()
{
	asCScriptNode *node = new asCScriptNode(snCondition);

	node->AddChildLast(ParseExpression());
	if( isSyntaxError ) return node;

	sToken t;
	GetToken(&t);
	if( t.type == ttQuestion )
	{
		node->AddChildLast(ParseAssignment());
		if( isSyntaxError ) return node;

		GetToken(&t);
		if( t.type != ttColon )
		{
			Error(ExpectedToken(":").AddressOf(), &t);
			return node;
		}

		node->AddChildLast(ParseAssignment());
		if( isSyntaxError ) return node;
	}
	else
		RewindTo(&t);

	return node;
}

asCScriptNode *asCParser::ParseExpression()
{
	asCScriptNode *node = new asCScriptNode(snExpression);

	node->AddChildLast(ParseExprTerm());
	if( isSyntaxError ) return node;

	for(;;)
	{
		sToken t;
		GetToken(&t);
		RewindTo(&t);

		if( !IsOperator(t.type) )
			return node;

		node->AddChildLast(ParseExprOperator());
		if( isSyntaxError ) return node;

		node->AddChildLast(ParseExprTerm());
		if( isSyntaxError ) return node;
	}
	return 0;
}

asCScriptNode *asCParser::ParseExprTerm()
{
	asCScriptNode *node = new asCScriptNode(snExprTerm);

	for(;;)
	{
		sToken t;
		GetToken(&t);
		RewindTo(&t);
		if( !IsPreOperator(t.type) )
			break;

		node->AddChildLast(ParseExprPreOp());
		if( isSyntaxError ) return node;
	}

	node->AddChildLast(ParseExprValue());
	if( isSyntaxError ) return node;

	
	for(;;)
	{
		sToken t;
		GetToken(&t);
		RewindTo(&t);
		if( !IsPostOperator(t.type) )
			return node;

		node->AddChildLast(ParseExprPostOp());
		if( isSyntaxError ) return node;
	}
	return 0;
}

asCScriptNode *asCParser::ParseExprPreOp()
{
	asCScriptNode *node = new asCScriptNode(snExprPreOp);

	sToken t;
	GetToken(&t);
	if( !IsPreOperator(t.type) )
	{
		Error(TXT_EXPECTED_PRE_OPERATOR, &t);
		return node;
	}

	node->SetToken(&t);
	node->UpdateSourcePos(t.pos, t.length);

	return node;
}

asCScriptNode *asCParser::ParseExprPostOp()
{
	asCScriptNode *node = new asCScriptNode(snExprPostOp);

	sToken t;
	GetToken(&t);
	if( !IsPostOperator(t.type) )
	{
		Error(TXT_EXPECTED_POST_OPERATOR, &t);
		return node;
	}

	node->SetToken(&t);
	node->UpdateSourcePos(t.pos, t.length);

	if( t.type == ttDot )
	{
		sToken t1, t2;
		GetToken(&t1);
		GetToken(&t2);
		RewindTo(&t1);
		if( t2.type == ttOpenParanthesis )
			node->AddChildLast(ParseFunctionCall());
		else
			node->AddChildLast(ParseIdentifier());
	}
	else if( t.type == ttOpenBracket )
	{
		node->AddChildLast(ParseAssignment());

		GetToken(&t);
		if( t.type != ttCloseBracket )
		{
			ExpectedToken("]");
			return node;
		}

		node->UpdateSourcePos(t.pos, t.length);
	}

	return node;
}

asCScriptNode *asCParser::ParseExprOperator()
{
	asCScriptNode *node = new asCScriptNode(snExprOperator);

	sToken t;
	GetToken(&t);
	if( !IsOperator(t.type) )
	{
		Error(TXT_EXPECTED_OPERATOR, &t);
		return node;
	}

	node->SetToken(&t);
	node->UpdateSourcePos(t.pos, t.length);

	return node;
}

asCScriptNode *asCParser::ParseAssignOperator()
{
	asCScriptNode *node = new asCScriptNode(snExprOperator);

	sToken t;
	GetToken(&t);
	if( !IsAssignOperator(t.type) )
	{
		Error(TXT_EXPECTED_OPERATOR, &t);
		return node;
	}

	node->SetToken(&t);
	node->UpdateSourcePos(t.pos, t.length);

	return node;
}

void asCParser::GetToken(sToken *token)
{
	int sourceLength = script->codeLength;

	do
	{
		if( sourcePos >= sourceLength )
		{
			token->type = ttEnd;
			token->length = 0;
		}
		else
			token->type = tokenizer.GetToken(&script->code[sourcePos], sourceLength - sourcePos, &token->length);

		token->pos = sourcePos;

		// Update state
		sourcePos += token->length;
	}
	// Filter out whitespace and comments
	while( token->type == ttWhiteSpace || 
	       token->type == ttOnelineComment ||
		   token->type == ttMultilineComment );
}

void asCParser::RewindTo(const sToken *token)
{
	sourcePos = token->pos;
}

void asCParser::Error(const char *text, sToken *token)
{
	RewindTo(token);

	isSyntaxError     = true;
	errorWhileParsing = true;

	int row, col;
	script->ConvertPosToRowCol(token->pos, &row, &col);

	if( builder )
		builder->WriteError(script->name.AddressOf(), text, row, col);
}

bool asCParser::IsRealType(int tokenType)
{
	if( tokenType == ttVoid ||
		tokenType == ttInt ||
		tokenType == ttInt8 ||
		tokenType == ttInt16 ||
		tokenType == ttUInt ||
		tokenType == ttUInt8 ||
		tokenType == ttUInt16 ||
		tokenType == ttFloat ||
		tokenType == ttBool ||
		tokenType == ttBits ||
		tokenType == ttBits8 ||
		tokenType == ttBits16 ||
		tokenType == ttDouble )
		return true;

	return false;
}


bool asCParser::IsDataType(int tokenType)
{
	if( tokenType == ttIdentifier ||
		IsRealType(tokenType) )
		return true;

	return false;
}

bool asCParser::IsOperator(int tokenType)
{
	if( tokenType == ttPlus ||
		tokenType == ttMinus ||
		tokenType == ttStar ||
		tokenType == ttSlash ||
		tokenType == ttPercent ||
		tokenType == ttAnd ||
		tokenType == ttOr ||
		tokenType == ttXor ||
		tokenType == ttEqual ||
		tokenType == ttNotEqual ||
		tokenType == ttLessThan ||
		tokenType == ttLessThanOrEqual ||
		tokenType == ttGreaterThan ||
		tokenType == ttGreaterThanOrEqual ||
		tokenType == ttAmp ||
		tokenType == ttBitOr ||
		tokenType == ttBitXor ||
		tokenType == ttBitShiftLeft ||
		tokenType == ttBitShiftRight ||
		tokenType == ttBitShiftRightArith )
		return true;

	return false;
}

bool asCParser::IsAssignOperator(int tokenType)
{
	if( tokenType == ttAssignment ||
		tokenType == ttAddAssign ||
		tokenType == ttSubAssign ||
		tokenType == ttMulAssign ||
		tokenType == ttDivAssign ||
		tokenType == ttModAssign ||
		tokenType == ttAndAssign ||
		tokenType == ttOrAssign ||
		tokenType == ttXorAssign ||
		tokenType == ttShiftLeftAssign ||
		tokenType == ttShiftRightLAssign ||
		tokenType == ttShiftRightAAssign )
		return true;

	return false;
}

bool asCParser::IsPreOperator(int tokenType)
{
	if( tokenType == ttMinus ||
		tokenType == ttPlus ||
		tokenType == ttNot ||
		tokenType == ttInc ||
		tokenType == ttDec ||
		tokenType == ttBitNot ||
		tokenType == ttHandle )
		return true;
	return false;
}

bool asCParser::IsPostOperator(int tokenType)
{
	if( tokenType == ttInc ||
		tokenType == ttDec ||
		tokenType == ttDot ||
		tokenType == ttOpenBracket )
		return true;
	return false;
}

bool asCParser::IsConstant(int tokenType)
{
	if( tokenType == ttIntConstant ||
		tokenType == ttFloatConstant ||
		tokenType == ttDoubleConstant ||
		tokenType == ttStringConstant ||
		tokenType == ttHeredocStringConstant ||
		tokenType == ttTrue ||
		tokenType == ttFalse ||
		tokenType == ttBitsConstant ||
		tokenType == ttNull )
		return true;

	return false;
}

asCString asCParser::ExpectedToken(const char *token)
{
	asCString str;

	str.Format(TXT_EXPECTED_s, token);

	return str;
}

asCString asCParser::ExpectedTokens(const char *t1, const char *t2)
{
	asCString str;

	str.Format(TXT_EXPECTED_s_OR_s, t1, t2);

	return str;
}

asCString asCParser::ExpectedOneOf(int *tokens, int count)
{
	asCString str;

	str = TXT_EXPECTED_ONE_OF;
	for( int n = 0; n < count; n++ )
	{
		str += asGetTokenDefinition(tokens[n]);
		if( n < count-1 )
			str += ", ";
	}

	return str;
}

END_AS_NAMESPACE

