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
// as_tokendef.h
//
// Definitions for tokens identifiable by the tokenizer
//


#ifndef AS_TOKENDEF_H
#define AS_TOKENDEF_H

#include "as_config.h"

enum eTokenType
{
	ttUnrecognizedToken,

	ttEnd,				   // End of file

	// White space and comments
	ttWhiteSpace,          // ' ', '\t', '\r', '\n'
	ttOnelineComment,      // // \n
	ttMultilineComment,    // /* */

	// Atoms
	ttIdentifier,            // abc123
	ttIntConstant,           // 1234
	ttFloatConstant,         // 12.34e56f
	ttDoubleConstant,        // 12.34e56
	ttStringConstant,        // "123"
	ttHeredocStringConstant, // """text"""
	ttNonTerminatedStringConstant, // "123
	ttBitsConstant,          // 0xFFFF

	// Math operators
	ttPlus,                // +
	ttMinus,               // -
	ttStar,                // *
	ttSlash,               // /
	ttPercent,             // %

	ttHandle,              // #

	ttAddAssign,           // +=
	ttSubAssign,           // -=
	ttMulAssign,           // *=
	ttDivAssign,           // /=
	ttModAssign,           // %=

	ttOrAssign,            // |=
	ttAndAssign,           // &=
	ttXorAssign,           // ^=
	ttShiftLeftAssign,     // <<=
	ttShiftRightLAssign,   // >>=
	ttShiftRightAAssign,   // >>>=

	ttInc,                 // ++
	ttDec,                 // --

	ttDot,                 // .

	// Statement tokens
	ttAssignment,          // =
	ttEndStatement,        // ;
	ttListSeparator,       // ,
	ttStartStatementBlock, // {
	ttEndStatementBlock,   // }
	ttOpenParanthesis,     // (
	ttCloseParanthesis,    // )
	ttOpenBracket,         // [
	ttCloseBracket,        // ]
	ttAmp,                 // &

	// Bitwise operators
	ttBitOr,               // |
	ttBitNot,              // ~
	ttBitXor,              // ^
	ttBitShiftLeft,        // <<
	ttBitShiftRight,       // >>
	ttBitShiftRightArith,  // >>>

	// Compare operators
	ttEqual,               // ==
	ttNotEqual,            // !=
	ttLessThan,            // <
	ttGreaterThan,         // >
	ttLessThanOrEqual,     // <=
	ttGreaterThanOrEqual,  // >=

	ttQuestion,            // ?
	ttColon,               // :

	// Reserved keywords
	ttIf,                  // if
	ttElse,                // else
	ttFor,				   // for
	ttWhile,               // while
	ttBool,                // bool
	ttImport,              // import
	ttInt,                 // int
	ttInt8,                // int8
	ttInt16,               // int16
	ttUInt,                // uint
	ttUInt8,               // uint8
	ttUInt16,              // uint16
	ttFloat,               // float
	ttVoid,                // void
	ttTrue,                // true
	ttFalse,               // false
	ttReturn,              // return
	ttNot,                 // not
	ttAnd,				   // and
	ttOr,				   // or
	ttXor,                 // xor
	ttBreak,               // break
	ttContinue,            // continue
	ttConst,			   // const
	ttDo,                  // do
	ttBits,                // bits
	ttBits8,               // bits8
	ttBits16,              // bits16
	ttDouble,              // double
	ttSwitch,              // switch
	ttCase,                // case
	ttDefault,             // default
	ttIn,                  // in
	ttOut,                 // out
	ttInOut,               // inout
	ttNull,                // null
    ttStruct               // struct
};

struct sTokenWord
{
	char *word;
	eTokenType   tokenType;
};

sTokenWord const tokenWords[] =
{
	{"+"       , ttPlus},
	{"-"       , ttMinus},
	{"*"       , ttStar},
	{"/"       , ttSlash},
	{"%"       , ttPercent},
	{"="       , ttAssignment},
	{"."       , ttDot},
	{"+="      , ttAddAssign},
	{"-="      , ttSubAssign},
	{"*="      , ttMulAssign},
	{"/="      , ttDivAssign},
	{"%="      , ttModAssign},
	{"|="      , ttOrAssign},
	{"&="      , ttAndAssign},
	{"^="      , ttXorAssign},
	{"<<="     , ttShiftLeftAssign},
	{">>="     , ttShiftRightLAssign},
	{">>>="    , ttShiftRightAAssign},
	{"|"       , ttBitOr},
	{"~"       , ttBitNot},
	{"^"       , ttBitXor},
	{"<<"      , ttBitShiftLeft},
	{">>"      , ttBitShiftRight},
	{">>>"     , ttBitShiftRightArith},
	{";"       , ttEndStatement},
	{","       , ttListSeparator},
	{"{"       , ttStartStatementBlock},
	{"}"       , ttEndStatementBlock},
	{"("       , ttOpenParanthesis},
	{")"       , ttCloseParanthesis},
	{"["       , ttOpenBracket},
	{"]"       , ttCloseBracket},
	{"?"       , ttQuestion},
	{":"       , ttColon},
	{"=="      , ttEqual},
	{"!="      , ttNotEqual},
	{"<"       , ttLessThan},
	{">"       , ttGreaterThan},
	{"<="      , ttLessThanOrEqual},
	{">="      , ttGreaterThanOrEqual},
	{"++"      , ttInc},
	{"--"      , ttDec},
	{"&"       , ttAmp},
	{"!"       , ttNot},
	{"||"      , ttOr},
	{"&&"      , ttAnd},
	{"^^"      , ttXor},
	{"@"       , ttHandle},
	{"and"     , ttAnd},
	{"bits"    , ttBits},
	{"bits8"   , ttBits8},
	{"bits16"  , ttBits16},
	{"bits32"  , ttBits},
	{"bool"    , ttBool},
	{"break"   , ttBreak},
	{"const"   , ttConst},
	{"continue", ttContinue},
	{"do"      , ttDo},
#ifdef  AS_USE_DOUBLE_AS_FLOAT
	{"double"  , ttFloat},
#else
	{"double"  , ttDouble},
#endif
	{"else"    , ttElse},
	{"false"   , ttFalse},
	{"float"   , ttFloat},
	{"for"     , ttFor},
	{"if"      , ttIf},
	{"in"      , ttIn},
	{"inout"   , ttInOut},
	{"import"  , ttImport},
	{"int"     , ttInt},
	{"int8"    , ttInt8},
	{"int16"   , ttInt16},
	{"int32"   , ttInt},  
	{"not"     , ttNot},
	{"null"    , ttNull},
	{"or"      , ttOr},
	{"out"     , ttOut},
	{"return"  , ttReturn},
	{"true"    , ttTrue},
	{"void"    , ttVoid},
	{"while"   , ttWhile},
	{"uint"    , ttUInt},
	{"uint8"   , ttUInt8},
	{"uint16"  , ttUInt16},
	{"uint32"  , ttUInt},
	{"switch"  , ttSwitch},
	{"struct"  , ttStruct},
	{"case"    , ttCase}, 
	{"default" , ttDefault},
	{"xor"     , ttXor},
};

int const numTokenWords = sizeof(tokenWords)/sizeof(sTokenWord);

char * const whiteSpace = " \t\r\n";

#endif
