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
// as_parser.h
//
// This class parses the script code and builds a tree for compilation
//



/*

SCRIPT        = (FUNCTION | GLOBVAR | IMPORT | STRUCT)*
TYPE          = 'const'? DATATYPE
TYPEMOD       = ('&' ('in' | 'out' | 'inout')?)?
FUNCTION      = TYPE TYPEMOD IDENTIFIER PARAMLIST BLOCK
IMPORT        = 'import' TYPE TYPEMOD IDENTIFIER PARAMLIST 'from' STRING ';'
GLOBVAR       = TYPE IDENTIFIER ('=' (INITLIST | ASSIGNMENT))? (',' IDENTIFIER ('=' (INITLIST | ASSIGNMENT))?)* ';'
DATATYPE      = REALTYPE | IDENTIFIER
REALTYPE      = 'void' | 'bool' | 'float' | 'int' | 'uint' | 'bits'
PARAMLIST     = '(' (TYPE TYPEMOD IDENTIFIER? (',' TYPE TYPEMOD IDENTIFIER?)*)? ')'
BLOCK         = '{' (DECLARATION | STATEMENT)* '}'
DECLARATION   = TYPE IDENTIFIER ('=' (INITLIST | ASSIGNMENT))? (',' IDENTIFIER ('=' (INITLIST | ASSIGNMENT))?)* ';'
STATEMENT     = BLOCK | IF | WHILE | DOWHILE | RETURN | EXPRSTATEMENT | BREAK | CONTINUE
BREAK         = 'break' ';'
CONTINUE      = 'continue' ';'
EXPRSTATEMENT = ASSIGNMENT? ';'
FOR           = 'for' '(' (DECLARATION | EXPRSTATEMENT) EXPRSTATEMENT ASSIGNMENT? ')' STATEMENT
IF            = 'if' '(' ASSIGNMENT ')' STATEMENT ('else' STATEMENT)?
WHILE         = 'while' '(' ASSIGNMENT ')' STATEMENT
DOWHILE       = 'do' STATEMENT 'while' '(' ASSIGNMENT ')' ';'
RETURN        = 'return' ASSIGNMENT? ';'
ASSIGNMENT    = CONDITION (ASSIGNOP ASSIGNMENT)?
CONDITION     = EXPRESSION ('?' ASSIGNMENT ':' ASSIGNMENT)?
EXPRESSION    = TERM (OP TERM)*
TERM          = PRE* VALUE POST*
VALUE         = '(' ASSIGNMENT ')' | CONSTANT | IDENTIFIER | FUNCTIONCALL | CONVERSION
PRE           = '-' | '+' | 'not' | '++' | '--' | '~'
POST          = '++' | '--' | ('.' | '->') (IDENTIFIER | FUNCTIONCALL) | '[' ASSIGNMENT ']'
FUNCTIONCALL  = IDENTIFIER ARGLIST
ARGLIST       = '(' (ASSIGNMENT (',' ASSIGNMENT)*)? ')'
CONSTANT      = "abc" | 123 | 123.1 | 'true' | 'false' | 0xFFFF
OP            = 'and' | 'or' |
                '==' | '!=' | '<' | '<=' | '>=' | '>' |
			    '+' | '-' | '*' | '/' | '%' | '|' | '&' | '^' | '<<' | '>>' | '>>>'
ASSIGNOP      = '=' | '+=' | '-=' | '*=' | '/=' | '%=' | '|=' | '&=' | '^=' | '<<=' | '>>=' | '>>>='
CONVERSION    = TYPE '(' ASSIGNMENT ')'
INITLIST      = '{' ((INITLIST | ASSIGNMENT)? (',' (INITLIST | ASSIGNMENT)?)*)? '}'

*/

#ifndef AS_PARSER_H
#define AS_PARSER_H

#include "as_scriptnode.h"
#include "as_scriptcode.h"
#include "as_builder.h"
#include "as_tokenizer.h"

BEGIN_AS_NAMESPACE

class asCParser
{
public:
	asCParser(asCBuilder *builder);
	~asCParser();

	int ParseScript(asCScriptCode *script);
	int ParseFunctionDefinition(asCScriptCode *script);
	int ParsePropertyDeclaration(asCScriptCode *script);
	int ParseDataType(asCScriptCode *script);

	asCScriptNode *GetScriptNode();

protected:
	void Reset();

	void GetToken(sToken *token);
	void RewindTo(const sToken *token);
	void Error(const char *text, sToken *token);

	asCScriptNode *ParseImport();
	asCScriptNode *ParseFunctionDefinition();

	asCScriptNode *ParseScript();
	asCScriptNode *ParseType(bool allowConst);
	asCScriptNode *ParseTypeMod(bool isParam);
	asCScriptNode *ParseFunction();
	asCScriptNode *ParseGlobalVar();
	asCScriptNode *ParseParameterList();
	asCScriptNode *ParseStatementBlock();
	asCScriptNode *ParseDeclaration();
	asCScriptNode *ParseStatement();
	asCScriptNode *ParseExpressionStatement();
	asCScriptNode *ParseSwitch();
	asCScriptNode *ParseCase();
	asCScriptNode *ParseIf();
	asCScriptNode *ParseFor();
	asCScriptNode *ParseWhile();
	asCScriptNode *ParseDoWhile();
	asCScriptNode *ParseReturn();
	asCScriptNode *ParseBreak();
	asCScriptNode *ParseContinue();
	asCScriptNode *ParseAssignment();
	asCScriptNode *ParseAssignOperator();
	asCScriptNode *ParseCondition();
	asCScriptNode *ParseExpression();
	asCScriptNode *ParseExprTerm();
	asCScriptNode *ParseExprOperator();
	asCScriptNode *ParseExprPreOp();
	asCScriptNode *ParseExprPostOp();
	asCScriptNode *ParseExprValue();
	asCScriptNode *ParseArgList();
	asCScriptNode *ParseDataType();
	asCScriptNode *ParseRealType();
	asCScriptNode *ParseIdentifier();
	asCScriptNode *ParseConstant();
	asCScriptNode *ParseStringConstant();
	asCScriptNode *ParseFunctionCall();
	asCScriptNode *ParseToken(int token);
	asCScriptNode *ParseOneOf(int *tokens, int num);
	asCScriptNode *ParseStruct();
	asCScriptNode *ParseInitList();

	bool IsGlobalVar();
	bool IsDeclaration();
	bool IsRealType(int tokenType);
	bool IsDataType(int tokenType);
	bool IsOperator(int tokenType);
	bool IsPreOperator(int tokenType);
	bool IsPostOperator(int tokenType);
	bool IsConstant(int tokenType);
	bool IsAssignOperator(int tokenType);
	bool IsFunctionCall();

	asCString ExpectedToken(const char *token);
	asCString ExpectedTokens(const char *token1, const char *token2);
	asCString ExpectedOneOf(int *tokens, int count);

	bool errorWhileParsing;
	bool isSyntaxError;

	asCBuilder    *builder;
	asCScriptCode *script;
	asCScriptNode *scriptNode;

	asCTokenizer tokenizer;
	int          sourcePos;
};

END_AS_NAMESPACE

#endif
