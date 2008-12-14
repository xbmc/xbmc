/*
 *	@file 	ejsInternal.h
 * 	@brief 	Private header for Embedded Javascript (ECMAScript)
 *	@overview This Embedded Javascript header defines the private Embedded 
 *		Javascript internal structures.
 */
/********************************* Copyright **********************************/
/*
 *	@copy	default.g
 *	
 *	Copyright (c) Mbedthis Software LLC, 2003-2007. All Rights Reserved.
 *	Portions Copyright (c) GoAhead Software, 1995-2000. All Rights Reserved.
 *	
 *	This software is distributed under commercial and open source licenses.
 *	You may use the GPL open source license described below or you may acquire 
 *	a commercial license from Mbedthis Software. You agree to be fully bound 
 *	by the terms of either license. Consult the LICENSE.TXT distributed with 
 *	this software for full details.
 *	
 *	This software is open source; you can redistribute it and/or modify it 
 *	under the terms of the GNU General Public License as published by the 
 *	Free Software Foundation; either version 2 of the License, or (at your 
 *	option) any later version. See the GNU General Public License for more 
 *	details at: http://www.mbedthis.com/downloads/gplLicense.html
 *	
 *	This program is distributed WITHOUT ANY WARRANTY; without even the 
 *	implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
 *	
 *	This GPL license does NOT permit incorporating this software into 
 *	proprietary programs. If you are unable to comply with the GPL, you must
 *	acquire a commercial license to use this software. Commercial licenses 
 *	for this software and support services are available from Mbedthis 
 *	Software at http://www.mbedthis.com 
 *	
 *	@end
 */
/********************************* Includes ***********************************/

#ifndef _h_EJS_INTERNAL
#define _h_EJS_INTERNAL 1

#include		"ejs.h"

/********************************** Defines ***********************************/

#ifdef __cplusplus
extern "C" {
#endif

/*
 *	Constants
 */

#if BLD_FEATURE_SQUEEZE
	#define EJS_PARSE_INCR			256		/* Growth factor */
	#define EJS_MAX_RECURSE			25		/* Sanity for maximum recursion */
	#define EJS_MAX_ID				128		/* Maximum ID length */
	#define EJS_OBJ_HASH_SIZE		13		/* Object hash table size */
	#define EJS_SMALL_OBJ_HASH_SIZE	11		/* Small object hash size */
	#define EJS_LIST_INCR			8		/* Growth increment for lists */
#else
	#define EJS_PARSE_INCR			1024	/* Growth factor */
	#define EJS_MAX_RECURSE			100		/* Sanity for maximum recursion */
	#define EJS_MAX_ID				256		/* Maximum ID length */
	#define EJS_OBJ_HASH_SIZE		29		/* Object hash table size */
	#define EJS_SMALL_OBJ_HASH_SIZE	11		/* Small object hash size */
	#define EJS_LIST_INCR			16		/* Growth increment for lists */
#endif
#define EJS_TOKEN_STACK				4		/* Put back token stack */

/*
 *	Lexical analyser tokens
 */
#define EJS_TOK_ERR					-1		/* Any error */
#define EJS_TOK_LPAREN				1		/* ( */
#define EJS_TOK_RPAREN				2		/* ) */
#define EJS_TOK_IF					3		/* if */
#define EJS_TOK_ELSE				4		/* else */
#define EJS_TOK_LBRACE				5		/* { */
#define EJS_TOK_RBRACE				6		/* } */
#define EJS_TOK_LOGICAL				7		/* ||, &&, ! */
#define EJS_TOK_EXPR				8		/* +, -, /, % */
#define EJS_TOK_SEMI				9		/* ; */
#define EJS_TOK_LITERAL				10		/* literal string */
#define EJS_TOK_FUNCTION_NAME		11		/* functionName */
#define EJS_TOK_NEWLINE				12		/* newline white space */
#define EJS_TOK_ID					13		/* Identifier */
#define EJS_TOK_EOF					14		/* End of script */
#define EJS_TOK_COMMA				15		/* Comma */
#define EJS_TOK_VAR					16		/* var */
#define EJS_TOK_ASSIGNMENT			17		/* = */
#define EJS_TOK_FOR					18		/* for */
#define EJS_TOK_INC_DEC				19		/* ++, -- */
#define EJS_TOK_RETURN				20		/* return */
#define EJS_TOK_PERIOD				21		/* . */
#define EJS_TOK_LBRACKET			22		/* [ */
#define EJS_TOK_RBRACKET			23		/* ] */
#define EJS_TOK_NEW					24		/* new */
#define EJS_TOK_DELETE				25		/* delete */
#define EJS_TOK_IN					26		/* in */
#define EJS_TOK_FUNCTION			27		/* function */
#define EJS_TOK_NUMBER				28		/* Number */

/*
 *	Expression operators
 */
#define EJS_EXPR_LESS				1		/* < */
#define EJS_EXPR_LESSEQ				2		/* <= */
#define EJS_EXPR_GREATER			3		/* > */
#define EJS_EXPR_GREATEREQ			4		/* >= */
#define EJS_EXPR_EQ					5		/* == */
#define EJS_EXPR_NOTEQ				6		/* != */
#define EJS_EXPR_PLUS				7		/* + */
#define EJS_EXPR_MINUS				8		/* - */
#define EJS_EXPR_DIV				9		/* / */
#define EJS_EXPR_MOD				10		/* % */
#define EJS_EXPR_LSHIFT				11		/* << */
#define EJS_EXPR_RSHIFT				12		/* >> */
#define EJS_EXPR_MUL				13		/* * */
#define EJS_EXPR_ASSIGNMENT			14		/* = */
#define EJS_EXPR_INC				15		/* ++ */
#define EJS_EXPR_DEC				16		/* -- */
#define EJS_EXPR_BOOL_COMP			17		/* ! */

/*
 *	Conditional operators
 */
#define EJS_COND_AND				1		/* && */
#define EJS_COND_OR					2		/* || */
#define EJS_COND_NOT				3		/* ! */

/*
 *	States
 */
#define EJS_STATE_ERR				-1		/* Error state */
#define EJS_STATE_EOF				1		/* End of file */
#define EJS_STATE_COND				2		/* Parsing a "(conditional)" stmt */
#define EJS_STATE_COND_DONE			3
#define EJS_STATE_RELEXP			4		/* Parsing a relational expr */
#define EJS_STATE_RELEXP_DONE		5
#define EJS_STATE_EXPR				6		/* Parsing an expression */
#define EJS_STATE_EXPR_DONE			7
#define EJS_STATE_STMT				8		/* Parsing General statement */
#define EJS_STATE_STMT_DONE			9
#define EJS_STATE_STMT_BLOCK_DONE	10		/* End of block "}" */
#define EJS_STATE_ARG_LIST			11		/* Function arg list */
#define EJS_STATE_ARG_LIST_DONE		12
#define EJS_STATE_DEC_LIST			16		/* Declaration list */
#define EJS_STATE_DEC_LIST_DONE		17
#define EJS_STATE_DEC				18		/* Declaration statement */
#define EJS_STATE_DEC_DONE			19
#define EJS_STATE_RET				20		/* Return statement */

#define EJS_STATE_BEGIN				EJS_STATE_STMT

/*
 *	General parsing flags.
 */
#define EJS_FLAGS_EXE				0x1		/* Execute statements */
#define EJS_FLAGS_LOCAL				0x2		/* Get local vars only */
#define EJS_FLAGS_GLOBAL			0x4		/* Get global vars only */
#define EJS_FLAGS_CREATE			0x8		/* Create var */
#define EJS_FLAGS_ASSIGNMENT		0x10	/* In assignment stmt */
#define EJS_FLAGS_DELETE			0x20	/* Deleting a variable */
#define EJS_FLAGS_FOREACH			0x40	/* In foreach */
#define EJS_FLAGS_NEW				0x80	/* In a new stmt() */
#define EJS_FLAGS_EXIT				0x100	/* Must exit */

/*
 *	Putback token 
 */

typedef struct EjsToken {
	char		*token;						/* Token string */
	int			id;							/* Token ID */
} EjsToken;

/*
 *	EJ evaluation block structure
 */
typedef struct ejEval {
	EjsToken	putBack[EJS_TOKEN_STACK]; 	/* Put back token stack */
	int			putBackIndex;				/* Top of stack index */
	MprStr		line;						/* Current line */
	int			lineLength;					/* Current line length */
	int			lineNumber;					/* Parse line number */
	int			lineColumn;					/* Column in line */
	MprStr		script;						/* Input script for parsing */
	char		*scriptServp;				/* Next token in the script */
	int			scriptSize;					/* Length of script */
	MprStr		tokbuf;						/* Current token */
	char		*tokEndp;					/* Pointer past end of token */
	char		*tokServp;					/* Pointer to next token char */
	int			tokSize;					/* Size of token buffer */
} EjsInput;

/*
 *	Function call structure
 */
typedef struct {
	MprArray	*args;						/* Args for function */
	MprVar		*fn;						/* Function definition */
	char		*procName;					/* Function name */
} EjsProc;

/*
 *	Per EJS structure
 */
typedef struct ej {
	EjsHandle	altHandle;					/* alternate callback handle */
	MprVar		*currentObj;				/* Ptr to current object */
	MprVar		*currentProperty;			/* Ptr to current property */
	EjsId		eid;						/* Halloc handle */
	char		*error;						/* Error message */
	int			exitStatus;					/* Status to exit() */
	int			flags;						/* Flags */
	MprArray	*frames;					/* List of variable frames */
	MprVar		*global;					/* Global object */
	EjsInput	*input;						/* Input evaluation block */
	MprVar		*local;						/* Local object */
	EjsHandle	primaryHandle;				/* primary callback handle */
	EjsProc		*proc;						/* Current function */
	MprVar		result;						/* Variable result */
	void		*thisPtr;					/* C++ ptr for functions */
	int			tid;						/* Current token id */
	char		*token;						/* Pointer to token string */
	MprVar		tokenNumber;				/* Parsed number */
} Ejs;

typedef int		EjsBlock;					/* Scope block id */

/*
 *	Function callback when using Alternate handles.
 */
typedef int (*EjsAltStringCFunction)(EjsHandle userHandle, EjsHandle altHandle,
		int argc, char **argv);
typedef int (*EjsAltCFunction)(EjsHandle userHandle, EjsHandle altHandle,
		int argc, MprVar **argv);

/******************************** Prototypes **********************************/
/*
 *	Ejs Lex
 */
extern int	 	ejsLexOpenScript(Ejs* ep, char *script);
extern void 	ejsLexCloseScript(Ejs* ep);
extern int 		ejsInitInputState(EjsInput *ip);
extern void 	ejsLexSaveInputState(Ejs* ep, EjsInput* state);
extern void 	ejsLexFreeInputState(Ejs* ep, EjsInput* state);
extern void 	ejsLexRestoreInputState(Ejs* ep, EjsInput* state);
extern int		ejsLexGetToken(Ejs* ep, int state);
extern void		ejsLexPutbackToken(Ejs* ep, int tid, char *string);

/*
 *	Parsing
 */
extern MprVar	*ejsFindObj(Ejs *ep, int state, char *property, int flags);
extern MprVar	*ejsFindProperty(Ejs *ep, int state, MprVar *obj,
					char *property, int flags);
extern int 		ejsGetVarCore(Ejs *ep, char *var, MprVar **obj, 
					MprVar **varValue, int flags);
extern int		ejsParse(Ejs *ep, int state, int flags);
extern Ejs 		*ejsPtr(EjsId eid);
extern void 	ejsSetFlags(int orFlags, int andFlags);

/*
 *	Create variable scope blocks
 */
extern EjsBlock	ejsOpenBlock(EjsId eid);
extern int		ejsCloseBlock(EjsId eid, EjsBlock vid);
extern int		ejsEvalBlock(EjsId eid, char *script, MprVar *v, char **err);
extern int		ejsDefineStandardProperties(MprVar *objVar);

/*
 *	Error handling
 */
extern void		ejsError(Ejs *ep, char *fmt, ...);

#ifdef __cplusplus
}
#endif
#endif /* _h_EJS_INTERNAL */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim: sw=4 ts=4 
 */
