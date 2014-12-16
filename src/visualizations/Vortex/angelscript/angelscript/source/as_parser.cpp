/*
   AngelCode Scripting Library
   Copyright (c) 2003-2009 Andreas Jonsson

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
// as_parser.cpp
//
// This class parses the script code and builds a tree for compilation
//



#include "as_config.h"
#include "as_parser.h"
#include "as_tokendef.h"
#include "as_texts.h"

#ifdef _MSC_VER
#pragma warning(disable:4702) // unreachable code
#endif

BEGIN_AS_NAMESPACE

asCParser::asCParser(asCBuilder *builder)
{
	this->builder    = builder;
	this->engine     = builder->engine;

	script			      = 0;
	scriptNode		      = 0;
	checkValidTypes       = false;
	isParsingAppInterface = false;
}

asCParser::~asCParser()
{
	Reset();
}

void asCParser::Reset()
{
	errorWhileParsing     = false;
	isSyntaxError         = false;
	checkValidTypes       = false;
	isParsingAppInterface = false;

	sourcePos = 0;

	if( scriptNode )
	{
		scriptNode->Destroy(engine);
	}

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

	// Set flag that permits ? as datatype for parameters
	isParsingAppInterface = true;

	this->script = script;

	scriptNode = ParseFunctionDefinition();

	// The declaration should end after the definition
	if( !isSyntaxError )
	{
		sToken t;
		GetToken(&t);
		if( t.type != ttEnd )
		{
			Error(ExpectedToken(asGetTokenDefinition(ttEnd)).AddressOf(), &t);
			return -1;
		}
	}

	if( errorWhileParsing )
		return -1;

	return 0;
}

int asCParser::ParseDataType(asCScriptCode *script)
{
	Reset();

	this->script = script;

	scriptNode = new(engine->memoryMgr.AllocScriptNode()) asCScriptNode(snDataType);
		
	scriptNode->AddChildLast(ParseType(true));
	if( isSyntaxError ) return -1;

	// The declaration should end after the type
	sToken t;
	GetToken(&t);
	if( t.type != ttEnd )
	{
		Error(ExpectedToken(asGetTokenDefinition(ttEnd)).AddressOf(), &t);
		return -1;
	}

	if( errorWhileParsing )
		return -1;

	return 0;
}


// Parse a template declaration: IDENTIFIER < class IDENTIFIER >
int asCParser::ParseTemplateDecl(asCScriptCode *script)
{
	Reset();

	this->script = script;
	scriptNode = new(engine->memoryMgr.AllocScriptNode()) asCScriptNode(snUndefined);

	scriptNode->AddChildLast(ParseIdentifier());
	if( isSyntaxError ) return -1;

	sToken t;
	GetToken(&t);
	if( t.type != ttLessThan )
	{
		Error(ExpectedToken(asGetTokenDefinition(ttLessThan)).AddressOf(), &t);
		return -1;
	}

	GetToken(&t);
	if( t.type != ttClass )
	{
		Error(ExpectedToken(asGetTokenDefinition(ttClass)).AddressOf(), &t);
		return -1;
	}

	scriptNode->AddChildLast(ParseIdentifier());
	if( isSyntaxError ) return -1;

	GetToken(&t);
	if( t.type != ttGreaterThan )
	{
		Error(ExpectedToken(asGetTokenDefinition(ttGreaterThan)).AddressOf(), &t);
		return -1;
	}

	GetToken(&t);
	if( t.type != ttEnd )
	{
		Error(ExpectedToken(asGetTokenDefinition(ttEnd)).AddressOf(), &t);
		return -1;
	}

	if( errorWhileParsing )
		return -1;

	return 0;
}

int asCParser::ParsePropertyDeclaration(asCScriptCode *script)
{
	Reset();

	this->script = script;

	scriptNode = new(engine->memoryMgr.AllocScriptNode()) asCScriptNode(snDeclaration);

	scriptNode->AddChildLast(ParseType(true));
	if( isSyntaxError ) return -1;

	scriptNode->AddChildLast(ParseIdentifier());
	if( isSyntaxError ) return -1;

	// The declaration should end after the identifier
	sToken t;
	GetToken(&t);
	if( t.type != ttEnd )
	{
		Error(ExpectedToken(asGetTokenDefinition(ttEnd)).AddressOf(), &t);
		return -1;
	}

	return 0;
}

asCScriptNode *asCParser::ParseImport()
{
	asCScriptNode *node = new(engine->memoryMgr.AllocScriptNode()) asCScriptNode(snImport);

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
		Error(ExpectedToken(FROM_TOKEN).AddressOf(), &t);
		return node;
	}

	asCString str;
	str.Assign(&script->code[t.pos], t.length);
	if( str != FROM_TOKEN )
	{
		Error(ExpectedToken(FROM_TOKEN).AddressOf(), &t);
		return node;
	}

	node->UpdateSourcePos(t.pos, t.length);

	GetToken(&t);
	if( t.type != ttStringConstant )
	{
		Error(TXT_EXPECTED_STRING, &t);
		return node;
	}

	asCScriptNode *mod = new(engine->memoryMgr.AllocScriptNode()) asCScriptNode(snConstant);
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
	asCScriptNode *node = new(engine->memoryMgr.AllocScriptNode()) asCScriptNode(snFunction);

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
	asCScriptNode *node = new(engine->memoryMgr.AllocScriptNode()) asCScriptNode(snScript);

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
			else if( t1.type == ttEnum )
				node->AddChildLast(ParseEnumeration());	//	Handle enumerations
			else if( t1.type == ttTypedef )
				node->AddChildLast(ParseTypedef());		//	Handle primitive typedefs
			else if( t1.type == ttClass )
				node->AddChildLast(ParseClass());
			else if( t1.type == ttInterface )
				node->AddChildLast(ParseInterface());
			else if( t1.type == ttConst || IsDataType(t1) )
			{
				if( IsVarDecl() )
					node->AddChildLast(ParseGlobalVar());
				else
					node->AddChildLast(ParseFunction());
			}
			else if( t1.type == ttEndStatement )
			{
				// Ignore a semicolon by itself
				GetToken(&t1);
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
	UNREACHABLE_RETURN;
}

int asCParser::ParseStatementBlock(asCScriptCode *script, asCScriptNode *block)
{
	Reset();

	// Tell the parser to validate the identifiers as valid types
	checkValidTypes = true;

	this->script = script;
	sourcePos = block->tokenPos;

	scriptNode = ParseStatementBlock();	

	if( isSyntaxError || errorWhileParsing )
		return -1;

	return 0;
}

asCScriptNode *asCParser::ParseEnumeration()
{
	asCScriptNode *ident;
	asCScriptNode *dataType;

	asCScriptNode *node = new(engine->memoryMgr.AllocScriptNode()) asCScriptNode(snEnum);

	sToken	token;

	// Check for enum
	GetToken(&token);
	if( token.type != ttEnum )
	{
		Error(ExpectedToken(asGetTokenDefinition(ttEnum)).AddressOf(), &token);
		return node;
	}

	node->SetToken(&token);
	node->UpdateSourcePos(token.pos, token.length);

	// Get the identifier
	GetToken(&token);
	if(ttIdentifier != token.type) 
	{
		Error(TXT_EXPECTED_IDENTIFIER, &token);
		return node;
	}

	dataType = new(engine->memoryMgr.AllocScriptNode()) asCScriptNode(snDataType);
	node->AddChildLast(dataType);

	ident = new(engine->memoryMgr.AllocScriptNode()) asCScriptNode(snIdentifier);
	ident->SetToken(&token);
	ident->UpdateSourcePos(token.pos, token.length);
	dataType->AddChildLast(ident);

	// check for the start of the declaration block
	GetToken(&token);
	if( token.type != ttStartStatementBlock ) 
	{
		RewindTo(&token);
		Error(ExpectedToken(asGetTokenDefinition(token.type)).AddressOf(), &token);
		return node;
	}

	while(ttEnd != token.type) 
	{
		GetToken(&token);

		if( ttEndStatementBlock == token.type ) 
		{
			RewindTo(&token);
			break;
		}

		if(ttIdentifier != token.type) 
		{
			Error(TXT_EXPECTED_IDENTIFIER, &token);
			return node;
		}

		//	Add the enum element
		ident = new(engine->memoryMgr.AllocScriptNode()) asCScriptNode(snIdentifier);
		ident->SetToken(&token);
		ident->UpdateSourcePos(token.pos, token.length);
		node->AddChildLast(ident);

		GetToken(&token);

		if( token.type == ttAssignment )
		{
			asCScriptNode	*tmp;

			RewindTo(&token);

			tmp = SuperficiallyParseGlobalVarInit();

			node->AddChildLast(tmp);
			if( isSyntaxError ) return node;
			GetToken(&token);
		}

		if(ttListSeparator != token.type) 
		{
			RewindTo(&token);
			break;
		}
	}

	//	check for the end of the declaration block
	GetToken(&token);
	if( token.type != ttEndStatementBlock ) 
	{
		RewindTo(&token);
		Error(ExpectedToken(asGetTokenDefinition(token.type)).AddressOf(), &token);
		return node;
	}

	//	Parse the declarations
	return node;
}

bool asCParser::CheckTemplateType(sToken &t)
{
	// Is this a template type?
	asCString typeName;
	typeName.Assign(&script->code[t.pos], t.length);
	if( engine->IsTemplateType(typeName.AddressOf()) )
	{
		// Expect the sub type within < >
		GetToken(&t);
		if( t.type != ttLessThan )
			return false;

		// Now there must be a data type
		GetToken(&t);
		if( !IsDataType(t) )
			return false;

		if( !CheckTemplateType(t) )
			return false;

		GetToken(&t);

		// Is it a handle or array?
		while( t.type == ttHandle || t.type == ttOpenBracket )
		{
			if( t.type == ttOpenBracket )
			{
				GetToken(&t);
				if( t.type != ttCloseBracket )
					return false;
			}

			GetToken(&t);
		}

		// Accept >> and >>> tokens too. But then force the tokenizer to move 
		// only 1 character ahead (thus splitting the token in two).
		if( script->code[t.pos] != '>' )
			return false;
		else if( t.length != 1 )
		{
			// We need to break the token, so that only the first character is parsed
			sToken t2 = t;
			t2.pos = t.pos + 1;
			RewindTo(&t2);
		}
	}

	return true;
}

bool asCParser::IsVarDecl()
{
	// Set start point so that we can rewind
	sToken t;
	GetToken(&t);
	RewindTo(&t);
	
	// A variable decl can start with a const
	sToken t1;
	GetToken(&t1);
	if( t1.type == ttConst )
		GetToken(&t1);

	if( !IsDataType(t1) )
	{
		RewindTo(&t);
		return false;
	}

	if( !CheckTemplateType(t1) )
	{
		RewindTo(&t);
		return false;
	}

	// Object handles can be interleaved with the array brackets
	sToken t2;
	GetToken(&t2);
	while( t2.type == ttHandle || t2.type == ttOpenBracket )
	{
		if( t2.type == ttOpenBracket )
		{
			GetToken(&t2);
			if( t2.type != ttCloseBracket )
			{
				RewindTo(&t);
				return false;
			}
		}

		GetToken(&t2);
	}

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

bool asCParser::IsFuncDecl(bool isMethod)
{
	// Set start point so that we can rewind
	sToken t;
	GetToken(&t);
	RewindTo(&t);

	// A class constructor starts with identifier followed by parenthesis
	// A class destructor starts with the ~ token
	if( isMethod )
	{
		sToken t1, t2;
		GetToken(&t1);
		GetToken(&t2);
		RewindTo(&t);
		if( (t1.type == ttIdentifier && t2.type == ttOpenParanthesis) || t1.type == ttBitNot )
			return true;
	}

	// A function decl can start with a const
	sToken t1;
	GetToken(&t1);
	if( t1.type == ttConst )
		GetToken(&t1);

	if( !IsDataType(t1) )
	{
		RewindTo(&t);
		return false;
	}

	// Object handles can be interleaved with the array brackets
	sToken t2;
	GetToken(&t2);
	while( t2.type == ttHandle || t2.type == ttOpenBracket )
	{
		if( t2.type == ttOpenBracket )
		{
			GetToken(&t2);
			if( t2.type != ttCloseBracket )
			{
				RewindTo(&t);
				return false;
			}
		}

		GetToken(&t2);
	}

	if( t2.type != ttIdentifier )
	{
		RewindTo(&t);
		return false;
	}

	GetToken(&t2);
	if( t2.type == ttOpenParanthesis ) 
	{	
		// If the closing paranthesis is not followed by a  
		// statement block then it is not a function. 
		while( t2.type != ttCloseParanthesis && t2.type != ttEnd )
			GetToken(&t2);

		if( t2.type == ttEnd ) 
			return false;
		else
		{
			// A class method can have a 'const' token after the parameter list
			if( isMethod )
			{
				GetToken(&t1);
				if( t1.type != ttConst )
					RewindTo(&t1);
			}

			GetToken(&t1);
			RewindTo(&t);
			if( t1.type == ttStartStatementBlock )
				return true;
		}

		RewindTo(&t);
		return false;
	}

	RewindTo(&t);
	return false;
}

asCScriptNode *asCParser::ParseFunction(bool isMethod)
{
	asCScriptNode *node = new(engine->memoryMgr.AllocScriptNode()) asCScriptNode(snFunction);

	sToken t1,t2;
	GetToken(&t1);
	GetToken(&t2);
	RewindTo(&t1);

	// If it is a global function, or a method, except constructor and destructor, then the return type is parsed
	if( !isMethod || (t1.type != ttBitNot && t2.type != ttOpenParanthesis) )
	{
		node->AddChildLast(ParseType(true));
		if( isSyntaxError ) return node;

		node->AddChildLast(ParseTypeMod(false));
		if( isSyntaxError ) return node;
	}

	// If this is a class destructor then it starts with ~, and no return type is declared
	if( isMethod && t1.type == ttBitNot )
	{
		node->AddChildLast(ParseToken(ttBitNot));
		if( isSyntaxError ) return node;
	}

	node->AddChildLast(ParseIdentifier());
	if( isSyntaxError ) return node;

	node->AddChildLast(ParseParameterList());
	if( isSyntaxError ) return node;

	if( isMethod )
	{
		// Is the method a const?
		GetToken(&t1);
		RewindTo(&t1);
		if( t1.type == ttConst )
			node->AddChildLast(ParseToken(ttConst));
	}

	// We should just find the end of the statement block here. The statements 
	// will be parsed on request by the compiler once it starts the compilation.
	node->AddChildLast(SuperficiallyParseStatementBlock());

	return node;
}

asCScriptNode *asCParser::ParseInterfaceMethod()
{
	asCScriptNode *node = new(engine->memoryMgr.AllocScriptNode()) asCScriptNode(snFunction);

	node->AddChildLast(ParseType(true));
	if( isSyntaxError ) return node;

	node->AddChildLast(ParseTypeMod(false));
	if( isSyntaxError ) return node;

	node->AddChildLast(ParseIdentifier());
	if( isSyntaxError ) return node;

	node->AddChildLast(ParseParameterList());
	if( isSyntaxError ) return node;

	// Parse an optional const after the method definition
	sToken t1;
	GetToken(&t1);
	RewindTo(&t1);
	if( t1.type == ttConst )
		node->AddChildLast(ParseToken(ttConst));

	GetToken(&t1);
	if( t1.type != ttEndStatement )
	{
		Error(ExpectedToken(";").AddressOf(), &t1);
		return node;
	}

	node->UpdateSourcePos(t1.pos, t1.length);

	return node;
}

asCScriptNode *asCParser::ParseInterface()
{
	asCScriptNode *node = new(engine->memoryMgr.AllocScriptNode()) asCScriptNode(snInterface);

	sToken t;
	GetToken(&t);
	if( t.type != ttInterface )
	{
		Error(ExpectedToken("interface").AddressOf(), &t);
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

	// Parse interface methods
	GetToken(&t);
	RewindTo(&t);
	while( t.type != ttEndStatementBlock && t.type != ttEnd )
	{
		// Parse the method signature
		node->AddChildLast(ParseInterfaceMethod());
		if( isSyntaxError ) return node;
		
		GetToken(&t);
		RewindTo(&t);
	}

	GetToken(&t);
	if( t.type != ttEndStatementBlock )
	{
		Error(ExpectedToken("}").AddressOf(), &t);
		return node;
	}

	node->UpdateSourcePos(t.pos, t.length);

	return node;
}

asCScriptNode *asCParser::ParseClass()
{
	asCScriptNode *node = new(engine->memoryMgr.AllocScriptNode()) asCScriptNode(snClass);

	sToken t;
	GetToken(&t);
	if( t.type != ttClass )
	{
		Error(ExpectedToken("class").AddressOf(), &t);
		return node;
	}

	node->SetToken(&t);

	if( engine->ep.allowImplicitHandleTypes )
	{
		// Parse 'implicit handle class' construct
		GetToken(&t);
		
		if ( t.type == ttHandle )
			node->SetToken(&t);
		else
			RewindTo(&t);
	}

	node->AddChildLast(ParseIdentifier());

	GetToken(&t);

	// Optional list of interfaces that are being implemented and classes that are being inherited
	if( t.type == ttColon )
	{
		node->AddChildLast(ParseIdentifier());
		GetToken(&t);
		while( t.type == ttListSeparator )
		{
			node->AddChildLast(ParseIdentifier());
			GetToken(&t);
		}
	}

	if( t.type != ttStartStatementBlock )
	{
		Error(ExpectedToken("{").AddressOf(), &t);
		return node;
	}

	// Parse properties
	GetToken(&t);
	RewindTo(&t);
	while( t.type != ttEndStatementBlock && t.type != ttEnd )
	{
		// Is it a property or a method?
		if( IsFuncDecl(true) )
		{
			// Parse the method
			node->AddChildLast(ParseFunction(true));
		}
		else if( IsVarDecl() )
		{
			// Parse a property declaration
			asCScriptNode *prop = new(engine->memoryMgr.AllocScriptNode()) asCScriptNode(snDeclaration);
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
		}
		else
		{
			Error(TXT_EXPECTED_METHOD_OR_PROPERTY, &t);
			return node;
		}

		GetToken(&t);
		RewindTo(&t);
	}

	GetToken(&t);
	if( t.type != ttEndStatementBlock )
	{
		Error(ExpectedToken("}").AddressOf(), &t);
		return node;
	}
	node->UpdateSourcePos(t.pos, t.length);

	return node;
}

asCScriptNode *asCParser::ParseGlobalVar()
{
	asCScriptNode *node = new(engine->memoryMgr.AllocScriptNode()) asCScriptNode(snGlobalVar);

	// Parse data type
	node->AddChildLast(ParseType(true));
	if( isSyntaxError ) return node;

	sToken t;

	for(;;)
	{
		// Parse identifier
		node->AddChildLast(ParseIdentifier());
		if( isSyntaxError ) return node;

		// Only superficially parse the initialization info for the variable
		GetToken(&t);
		RewindTo(&t);
		if( t.type == ttAssignment || t.type == ttOpenParanthesis )
		{
			node->AddChildLast(SuperficiallyParseGlobalVarInit());
			if( isSyntaxError ) return node;
		}

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
	UNREACHABLE_RETURN;
}

int asCParser::ParseGlobalVarInit(asCScriptCode *script, asCScriptNode *init)
{
	Reset();

	// Tell the parser to validate the identifiers as valid types
	checkValidTypes = true;

	this->script = script;
	sourcePos = init->tokenPos;

	// If next token is assignment, parse expression
	sToken t;
	GetToken(&t);
	if( t.type == ttAssignment )
	{
		GetToken(&t);
		RewindTo(&t);
		if( t.type == ttStartStatementBlock )
			scriptNode = ParseInitList();
		else
			scriptNode = ParseAssignment();
	}
	else if( t.type == ttOpenParanthesis ) 
	{
		RewindTo(&t);
		scriptNode = ParseArgList();
	}
	else
	{
		int tokens[] = {ttAssignment, ttOpenParanthesis};
		Error(ExpectedOneOf(tokens, 2).AddressOf(), &t);
	}

	if( isSyntaxError || errorWhileParsing )
		return -1;

	return 0;
}

asCScriptNode *asCParser::SuperficiallyParseGlobalVarInit()
{
	asCScriptNode *node = new(engine->memoryMgr.AllocScriptNode()) asCScriptNode(snAssignment);

	sToken t;
	GetToken(&t);
	node->UpdateSourcePos(t.pos, t.length);

	if( t.type == ttAssignment )
	{
		GetToken(&t);
		if( t.type == ttStartStatementBlock )
		{
			// Find the end of the initialization list
			int indent = 1;
			while( indent )
			{
				GetToken(&t);
				if( t.type == ttStartStatementBlock )
					indent++;
				else if( t.type == ttEndStatementBlock )
					indent--;
				else if( t.type == ttEnd )
				{
					Error(TXT_UNEXPECTED_END_OF_FILE, &t);
					break;
				}
			}
		}
		else
		{
			// Find the end of the expression
			int indent = 0;
			while( indent || (t.type != ttListSeparator && t.type != ttEndStatement && t.type != ttEndStatementBlock) )
			{
				if( t.type == ttOpenParanthesis )
					indent++;
				else if( t.type == ttCloseParanthesis )
					indent--;
				else if( t.type == ttEnd )
				{
					Error(TXT_UNEXPECTED_END_OF_FILE, &t);
					break;
				}
				GetToken(&t);
			}

			// Rewind so that the next token read is the list separator, end statement, or end statement block
			RewindTo(&t);
		}
	}
	else if( t.type == ttOpenParanthesis )
	{
		// Find the end of the argument list
		int indent = 1;
		while( indent )
		{
			GetToken(&t);
			if( t.type == ttOpenParanthesis )
				indent++;
			else if( t.type == ttCloseParanthesis )
				indent--;
			else if( t.type == ttEnd )
			{
				Error(TXT_UNEXPECTED_END_OF_FILE, &t);
				break;
			}
		}
	}
	else
	{
		int tokens[] = {ttAssignment, ttOpenParanthesis};
		Error(ExpectedOneOf(tokens, 2).AddressOf(), &t);
	}

	return node;
}

asCScriptNode *asCParser::ParseTypeMod(bool isParam)
{
	asCScriptNode *node = new(engine->memoryMgr.AllocScriptNode()) asCScriptNode(snDataType);

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
			GetToken(&t);
			RewindTo(&t);

			if( t.type == ttIn || t.type == ttOut || t.type == ttInOut )
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

asCScriptNode *asCParser::ParseType(bool allowConst, bool allowVariableType)
{
	asCScriptNode *node = new(engine->memoryMgr.AllocScriptNode()) asCScriptNode(snDataType);

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

	node->AddChildLast(ParseDataType(allowVariableType));

	// If the datatype is a template type, then parse the subtype within the < >
	asCScriptNode *type = node->lastChild;
	asCString typeName;
	typeName.Assign(&script->code[type->tokenPos], type->tokenLength);
	if( engine->IsTemplateType(typeName.AddressOf()) )
	{
		GetToken(&t);
		if( t.type != ttLessThan )
		{
			Error(ExpectedToken(asGetTokenDefinition(ttLessThan)).AddressOf(), &t);
			return node;
		}

		node->AddChildLast(ParseType(true, false));
		if( isSyntaxError ) return node;

		// Accept >> and >>> tokens too. But then force the tokenizer to move 
		// only 1 character ahead (thus splitting the token in two).
		GetToken(&t);
		if( script->code[t.pos] != '>' )
		{
			Error(ExpectedToken(asGetTokenDefinition(ttGreaterThan)).AddressOf(), &t);
			return node;
		}
		else
		{
			// Break the token so that only the first > is parsed
			sToken t2 = t;
			t2.pos = t.pos + 1;
			RewindTo(&t2);
		}
	}

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
	asCScriptNode *node = new(engine->memoryMgr.AllocScriptNode()) asCScriptNode(snUndefined);

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
	asCScriptNode *node = new(engine->memoryMgr.AllocScriptNode()) asCScriptNode(snUndefined);

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


asCScriptNode *asCParser::ParseDataType(bool allowVariableType)
{
	asCScriptNode *node = new(engine->memoryMgr.AllocScriptNode()) asCScriptNode(snDataType);

	sToken t1;

	GetToken(&t1);
	if( !IsDataType(t1) && !(allowVariableType && t1.type == ttQuestion) )
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
	asCScriptNode *node = new(engine->memoryMgr.AllocScriptNode()) asCScriptNode(snDataType);

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
	asCScriptNode *node = new(engine->memoryMgr.AllocScriptNode()) asCScriptNode(snIdentifier);

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

asCScriptNode *asCParser::ParseCast()
{
	asCScriptNode *node = new(engine->memoryMgr.AllocScriptNode()) asCScriptNode(snCast);

	sToken t1;
	GetToken(&t1);
	if( t1.type != ttCast )
	{
		Error(ExpectedToken("cast").AddressOf(), &t1);
		return node;
	}

	node->UpdateSourcePos(t1.pos, t1.length);

	GetToken(&t1);
	if( t1.type != ttLessThan )
	{
		Error(ExpectedToken("<").AddressOf(), &t1);
		return node;
	}

	// Parse the data type
	node->AddChildLast(ParseType(true));
	if( isSyntaxError ) return node;

	node->AddChildLast(ParseTypeMod(false));
	if( isSyntaxError ) return node;

	GetToken(&t1);
	if( t1.type != ttGreaterThan )
	{
		Error(ExpectedToken(">").AddressOf(), &t1);
		return node;
	}

	GetToken(&t1);
	if( t1.type != ttOpenParanthesis )
	{
		Error(ExpectedToken("(").AddressOf(), &t1);
		return node;
	}

	node->AddChildLast(ParseAssignment());
	if( isSyntaxError ) return node;

	GetToken(&t1);
	if( t1.type != ttCloseParanthesis )
	{
		Error(ExpectedToken(")").AddressOf(), &t1);
		return node;
	}

	node->UpdateSourcePos(t1.pos, t1.length);

	return node;
}

asCScriptNode *asCParser::ParseParameterList()
{
	asCScriptNode *node = new(engine->memoryMgr.AllocScriptNode()) asCScriptNode(snParameterList);

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
			node->AddChildLast(ParseType(true, isParsingAppInterface));
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
	UNREACHABLE_RETURN;
}

asCScriptNode *asCParser::ParseExprValue()
{
	asCScriptNode *node = new(engine->memoryMgr.AllocScriptNode()) asCScriptNode(snExprValue);

	sToken t1, t2;
	GetToken(&t1);
	GetToken(&t2);
	RewindTo(&t1);

	// TODO: namespace: Datatypes can be defined in namespaces, thus types too must allow scope prefix
	if( IsDataType(t1) && t2.type != ttScope )
		node->AddChildLast(ParseConstructCall());
	else if( t1.type == ttIdentifier || t1.type == ttScope )
	{
		if( IsFunctionCall() )
			node->AddChildLast(ParseFunctionCall());
		else
			node->AddChildLast(ParseVariableAccess());
	}
	else if( t1.type == ttCast )
		node->AddChildLast(ParseCast());
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
	asCScriptNode *node = new(engine->memoryMgr.AllocScriptNode()) asCScriptNode(snConstant);

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
	if( t.type == ttStringConstant || t.type == ttMultilineStringConstant || t.type == ttHeredocStringConstant )
		RewindTo(&t);

	while( t.type == ttStringConstant || t.type == ttMultilineStringConstant || t.type == ttHeredocStringConstant )
	{
		node->AddChildLast(ParseStringConstant());

		GetToken(&t);
		RewindTo(&t);
	}

	return node;
}

asCScriptNode *asCParser::ParseStringConstant()
{
	asCScriptNode *node = new(engine->memoryMgr.AllocScriptNode()) asCScriptNode(snConstant);

	sToken t;
	GetToken(&t);
	if( t.type != ttStringConstant && t.type != ttMultilineStringConstant && t.type != ttHeredocStringConstant )
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
	asCScriptNode *node = new(engine->memoryMgr.AllocScriptNode()) asCScriptNode(snFunctionCall);

	// Parse scope prefix
	sToken t1, t2;
	GetToken(&t1);
	if( t1.type == ttScope )
	{
		RewindTo(&t1);
		node->AddChildLast(ParseToken(ttScope));
		GetToken(&t1);
	}
	GetToken(&t2);
	while( t1.type == ttIdentifier && t2.type == ttScope )
	{
		RewindTo(&t1);
		node->AddChildLast(ParseIdentifier());
		node->AddChildLast(ParseToken(ttScope));
		GetToken(&t1);
		GetToken(&t2);
	}

	RewindTo(&t1);

	// Parse the function name followed by the argument list
	node->AddChildLast(ParseIdentifier());
	if( isSyntaxError ) return node;

	node->AddChildLast(ParseArgList());

	return node;
}

asCScriptNode *asCParser::ParseVariableAccess()
{
	asCScriptNode *node = new(engine->memoryMgr.AllocScriptNode()) asCScriptNode(snVariableAccess);

	// Parse scope prefix
	sToken t1, t2;
	GetToken(&t1);
	if( t1.type == ttScope )
	{
		RewindTo(&t1);
		node->AddChildLast(ParseToken(ttScope));
		GetToken(&t1);
	}
	GetToken(&t2);
	while( t1.type == ttIdentifier && t2.type == ttScope )
	{
		RewindTo(&t1);
		node->AddChildLast(ParseIdentifier());
		node->AddChildLast(ParseToken(ttScope));
		GetToken(&t1);
		GetToken(&t2);
	}

	RewindTo(&t1);

	// Parse the variable name
	node->AddChildLast(ParseIdentifier());

	return node;
}

asCScriptNode *asCParser::ParseConstructCall()
{
	asCScriptNode *node = new(engine->memoryMgr.AllocScriptNode()) asCScriptNode(snConstructCall);

	node->AddChildLast(ParseType(false));
	if( isSyntaxError ) return node;

	node->AddChildLast(ParseArgList());

	return node;
}

asCScriptNode *asCParser::ParseArgList()
{
	asCScriptNode *node = new(engine->memoryMgr.AllocScriptNode()) asCScriptNode(snArgList);

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

asCScriptNode *asCParser::SuperficiallyParseStatementBlock()
{
	asCScriptNode *node = new(engine->memoryMgr.AllocScriptNode()) asCScriptNode(snStatementBlock);

	// This function will only superficially parse the statement block in order to find the end of it
	sToken t1;

	GetToken(&t1);
	if( t1.type != ttStartStatementBlock )
	{
		Error(ExpectedToken("{").AddressOf(), &t1);
		return node;
	}

	node->UpdateSourcePos(t1.pos, t1.length);

	int level = 1;
	while( level > 0 && !isSyntaxError )
	{
		GetToken(&t1);
		if( t1.type == ttEndStatementBlock )
			level--;
		else if( t1.type == ttStartStatementBlock )
			level++;
		else if( t1.type == ttEnd )
			Error(TXT_UNEXPECTED_END_OF_FILE, &t1);
	}

	node->UpdateSourcePos(t1.pos, t1.length);

	return node;
}

asCScriptNode *asCParser::ParseStatementBlock()
{
	asCScriptNode *node = new(engine->memoryMgr.AllocScriptNode()) asCScriptNode(snStatementBlock);

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

				if( IsVarDecl() )
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
	UNREACHABLE_RETURN;
}

asCScriptNode *asCParser::ParseInitList()
{
	asCScriptNode *node = new(engine->memoryMgr.AllocScriptNode()) asCScriptNode(snInitList);

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
				node->AddChildLast(new(engine->memoryMgr.AllocScriptNode()) asCScriptNode(snUndefined));

				GetToken(&t1);
				if( t1.type == ttEndStatementBlock )
				{
					// No expression
					node->AddChildLast(new(engine->memoryMgr.AllocScriptNode()) asCScriptNode(snUndefined));
					node->UpdateSourcePos(t1.pos, t1.length);
					return node;
				}
				RewindTo(&t1);
			}
			else if( t1.type == ttEndStatementBlock )
			{
				// No expression 
				node->AddChildLast(new(engine->memoryMgr.AllocScriptNode()) asCScriptNode(snUndefined));

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
	UNREACHABLE_RETURN;
}

bool asCParser::IsFunctionCall()
{
	sToken s;
	sToken t1, t2;

	GetToken(&s);
	t1 = s;

	// A function call may be prefixed with scope resolution
	if( t1.type == ttScope )
		GetToken(&t1);
	GetToken(&t2);

	while( t1.type == ttIdentifier && t2.type == ttScope )
	{
		GetToken(&t1);
		GetToken(&t2);
	}

	// A function call starts with an identifier followed by an argument list
	if( t1.type != ttIdentifier || IsDataType(t1) )
	{
		RewindTo(&s);
		return false;
	}

	if( t2.type == ttOpenParanthesis )
	{
		RewindTo(&s);
		return true;
	}

	RewindTo(&s);
	return false;
}

asCScriptNode *asCParser::ParseDeclaration()
{
	asCScriptNode *node = new(engine->memoryMgr.AllocScriptNode()) asCScriptNode(snDeclaration);

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
	UNREACHABLE_RETURN;
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
	asCScriptNode *node = new(engine->memoryMgr.AllocScriptNode()) asCScriptNode(snExpressionStatement);

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
	asCScriptNode *node = new(engine->memoryMgr.AllocScriptNode()) asCScriptNode(snSwitch);

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
	asCScriptNode *node = new(engine->memoryMgr.AllocScriptNode()) asCScriptNode(snCase);

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
	asCScriptNode *node = new(engine->memoryMgr.AllocScriptNode()) asCScriptNode(snIf);

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
	asCScriptNode *node = new(engine->memoryMgr.AllocScriptNode()) asCScriptNode(snFor);

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

	if( IsVarDecl() )
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

		asCScriptNode *n = new(engine->memoryMgr.AllocScriptNode()) asCScriptNode(snExpressionStatement);
		node->AddChildLast(n);
		n->AddChildLast(ParseAssignment());
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
	asCScriptNode *node = new(engine->memoryMgr.AllocScriptNode()) asCScriptNode(snWhile);

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
	asCScriptNode *node = new(engine->memoryMgr.AllocScriptNode()) asCScriptNode(snDoWhile);

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
	asCScriptNode *node = new(engine->memoryMgr.AllocScriptNode()) asCScriptNode(snReturn);

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
	asCScriptNode *node = new(engine->memoryMgr.AllocScriptNode()) asCScriptNode(snBreak);

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
	asCScriptNode *node = new(engine->memoryMgr.AllocScriptNode()) asCScriptNode(snContinue);

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
	asCScriptNode *node = new(engine->memoryMgr.AllocScriptNode()) asCScriptNode(snAssignment);

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
	asCScriptNode *node = new(engine->memoryMgr.AllocScriptNode()) asCScriptNode(snCondition);

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
	asCScriptNode *node = new(engine->memoryMgr.AllocScriptNode()) asCScriptNode(snExpression);

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
	UNREACHABLE_RETURN;
}

asCScriptNode *asCParser::ParseExprTerm()
{
	asCScriptNode *node = new(engine->memoryMgr.AllocScriptNode()) asCScriptNode(snExprTerm);

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
	UNREACHABLE_RETURN;
}

asCScriptNode *asCParser::ParseExprPreOp()
{
	asCScriptNode *node = new(engine->memoryMgr.AllocScriptNode()) asCScriptNode(snExprPreOp);

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
	asCScriptNode *node = new(engine->memoryMgr.AllocScriptNode()) asCScriptNode(snExprPostOp);

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
	asCScriptNode *node = new(engine->memoryMgr.AllocScriptNode()) asCScriptNode(snExprOperator);

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
	asCScriptNode *node = new(engine->memoryMgr.AllocScriptNode()) asCScriptNode(snExprOperator);

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
	size_t sourceLength = script->codeLength;

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
		tokenType == ttInt64 ||
		tokenType == ttUInt ||
		tokenType == ttUInt8 ||
		tokenType == ttUInt16 ||
		tokenType == ttUInt64 ||
		tokenType == ttFloat ||
		tokenType == ttBool ||
		tokenType == ttDouble )
		return true;

	return false;
}


bool asCParser::IsDataType(const sToken &token)
{
	if( token.type == ttIdentifier )
	{
		if( checkValidTypes )
		{
			// Check if this is a registered type
			asCString str;
			str.Assign(&script->code[token.pos], token.length);
			if( !builder->GetObjectType(str.AddressOf()) )
				return false;
		}
		return true;
	}

	if( IsRealType(token.type) )
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
		tokenType == ttBitShiftRightArith ||
		tokenType == ttIs ||
		tokenType == ttNotIs )
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
		tokenType == ttMultilineStringConstant ||
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

// TODO: typedef: Typedefs should accept complex types as well
asCScriptNode *asCParser::ParseTypedef()
{
	// Create the typedef node
	asCScriptNode *node = new(engine->memoryMgr.AllocScriptNode()) asCScriptNode(snTypedef);

	sToken	token;

	GetToken(&token);
	if( token.type != ttTypedef)
	{
		Error(ExpectedToken(asGetTokenDefinition(token.type)).AddressOf(), &token);
		return node;
	}
	
	node->SetToken(&token);
	node->UpdateSourcePos(token.pos, token.length);

	// Parse the base type
	GetToken(&token);
	RewindTo(&token);

	// Make sure it is a primitive type (except ttVoid)
	if( !IsRealType(token.type) || token.type == ttVoid )
	{
		asCString str;
		str.Format(TXT_UNEXPECTED_TOKEN_s, asGetTokenDefinition(token.type));
		Error(str.AddressOf(), &token);
		return node;
	}

	node->AddChildLast(ParseRealType());
	node->AddChildLast(ParseIdentifier());

	// Check for the end of the typedef
	GetToken(&token);
	if( token.type != ttEndStatement ) 
	{
		RewindTo(&token);
		Error(ExpectedToken(asGetTokenDefinition(token.type)).AddressOf(), &token);
	}

	return node;
}

END_AS_NAMESPACE

