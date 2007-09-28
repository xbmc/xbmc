/* 
 *	ejIntrn.h -- Ejscript(TM) header
 *
 *	Copyright (c) GoAhead Software, Inc., 1992-2000
 *
 *	See the file "license.txt" for information on usage and redistribution
 *
 * $Id: ejIntrn.h,v 1.3 2002/10/24 14:44:50 bporter Exp $
 */

#ifndef _h_EJINTERNAL
#define _h_EJINTERNAL 1

/******************************** Description *********************************/

/* 
 *	GoAhead Ejscript(TM) header. This defines the Ejscript API and internal
 *	structures.
 */

/********************************* Includes ***********************************/

#include	<ctype.h>
#include	<stdarg.h>
#include	<stdlib.h>

#ifdef CE
#ifndef UEMF
	#include	<io.h>
#endif
#endif

#ifdef LYNX
	#include	<unistd.h>
#endif

#ifdef QNX4
	#include	<dirent.h>
#endif

#ifdef UEMF
	#include	"uemf.h"
#else
	#include	<param.h>
	#include	<stat.h>
	#include	"basic/basicInternal.h"
	#include	"emf/emfInternal.h"
#endif

#include		"ej.h"

/********************************** Defines ***********************************/
/*
 *	Constants
 */
#define EJ_INC				110		/* Growth for tags/tokens */
#define EJ_SCRIPT_INC		1023	/* Growth for ej scripts */
#define EJ_OFFSET			1		/* hAlloc doesn't like 0 entries */
#define EJ_MAX_RECURSE		100		/* Sanity for maximum recursion */

/*
 *	Ejscript Lexical analyser tokens
 */
#define TOK_ERR				-1		/* Any error */
#define TOK_LPAREN			1		/* ( */
#define TOK_RPAREN			2		/* ) */
#define TOK_IF				3		/* if */
#define TOK_ELSE			4		/* else */
#define TOK_LBRACE			5		/* { */
#define TOK_RBRACE			6		/* } */
#define TOK_LOGICAL			7		/* ||, &&, ! */
#define TOK_EXPR			8		/* +, -, /, % */
#define TOK_SEMI			9		/* ; */
#define TOK_LITERAL			10		/* literal string */
#define TOK_FUNCTION		11		/* function name */
#define TOK_NEWLINE			12		/* newline white space */
#define TOK_ID				13		/* function name */
#define TOK_EOF				14		/* End of script */
#define TOK_COMMA			15		/* Comma */
#define TOK_VAR				16		/* var */
#define TOK_ASSIGNMENT		17		/* = */
#define TOK_FOR				18		/* for */
#define TOK_INC_DEC			19		/* ++, -- */
#define TOK_RETURN			20		/* return */

/*
 *	Expression operators
 */
#define EXPR_LESS			1		/* < */
#define EXPR_LESSEQ			2		/* <= */
#define EXPR_GREATER		3		/* > */
#define EXPR_GREATEREQ		4		/* >= */
#define EXPR_EQ				5		/* == */
#define EXPR_NOTEQ			6		/* != */
#define EXPR_PLUS			7		/* + */
#define EXPR_MINUS			8		/* - */
#define EXPR_DIV			9		/* / */
#define EXPR_MOD			10		/* % */
#define EXPR_LSHIFT			11		/* << */
#define EXPR_RSHIFT			12		/* >> */
#define EXPR_MUL			13		/* * */
#define EXPR_ASSIGNMENT		14		/* = */
#define EXPR_INC			15		/* ++ */
#define EXPR_DEC			16		/* -- */
#define EXPR_BOOL_COMP		17		/* ! */
/*
 *	Conditional operators
 */
#define COND_AND			1		/* && */
#define COND_OR				2		/* || */
#define COND_NOT			3		/* ! */

/*
 *	States
 */
#define STATE_ERR				-1			/* Error state */
#define STATE_EOF				1			/* End of file */
#define STATE_COND				2			/* Parsing a "(conditional)" stmt */
#define STATE_COND_DONE			3
#define STATE_RELEXP			4			/* Parsing a relational expr */
#define STATE_RELEXP_DONE		5
#define STATE_EXPR				6			/* Parsing an expression */
#define STATE_EXPR_DONE			7
#define STATE_STMT				8			/* Parsing General statement */
#define STATE_STMT_DONE			9
#define STATE_STMT_BLOCK_DONE	10			/* End of block "}" */
#define STATE_ARG_LIST			11			/* Function arg list */
#define STATE_ARG_LIST_DONE		12
#define STATE_DEC_LIST			16			/* Declaration list */
#define STATE_DEC_LIST_DONE		17
#define STATE_DEC				18
#define STATE_DEC_DONE			19

#define STATE_RET				20			/* Return statement */

#define STATE_BEGIN				STATE_STMT

/*
 *	Flags. Used in ej_t and as parameter to parse()
 */
#define FLAGS_EXE				0x1				/* Execute statements */
#define FLAGS_VARIABLES			0x2				/* Allocated variables store */
#define FLAGS_FUNCTIONS			0x4				/* Allocated function store */

/*
 *	Function call structure
 */
typedef struct {
	char_t		*fname;							/* Function name */
	char_t		**args;							/* Args for function (halloc) */
	int			nArgs;							/* Number of args */
} ejfunc_t;

/*
 *	EJ evaluation block structure
 */
typedef struct ejEval {
	ringq_t		tokbuf;							/* Current token */
	ringq_t		script;							/* Input script for parsing */
	char_t		*putBackToken;					/* Putback token string */
	int			putBackTokenId;					/* Putback token ID */
	char_t		*line;							/* Current line */
	int			lineLength;						/* Current line length */
	int			lineNumber;						/* Parse line number */
	int			lineColumn;						/* Column in line */
} ejinput_t;

/*
 *	Per Ejscript session structure
 */
typedef struct ej {
	ejinput_t	*input;							/* Input evaluation block */
	sym_fd_t	functions;						/* Symbol table for functions */
	sym_fd_t	*variables;						/* hAlloc list of variables */
	int			variableMax;					/* Number of entries */
	ejfunc_t	*func;							/* Current function */
	char_t		*result;						/* Current expression result */
	char_t		*error;							/* Error message */
	char_t		*token;							/* Pointer to token string */
	int			tid;							/* Current token id */
	int			eid;							/* Halloc handle */
	int			flags;							/* Flags */
	int			userHandle;						/* User defined handle */
} ej_t;

/******************************** Prototypes **********************************/

extern int		ejOpenBlock(int eid);
extern int		ejCloseBlock(int eid, int vid);
extern char_t	*ejEvalBlock(int eid, char_t *script, char_t **emsg);
#ifndef __NO_EJ_FILE
extern char_t	*ejEvalFile(int eid, char_t *path, char_t **emsg);
#endif
extern int		ejRemoveGlobalFunction(int eid, char_t *name);
extern void		*ejGetGlobalFunction(int eid, char_t *name);
extern int 		ejSetGlobalFunctionDirect(sym_fd_t functions, char_t *name, 
					int (*fn)(int eid, void *handle, int argc, char_t **argv));
extern void 	ejError(ej_t* ep, char_t* fmt, ...);
extern void		ejSetUserHandle(int eid, int handle);
extern int		ejGetUserHandle(int eid);
extern int		ejGetLineNumber(int eid);
extern char_t	*ejGetResult(int eid);
extern void		ejSetLocalVar(int eid, char_t *var, char_t *value);
extern void		ejSetGlobalVar(int eid, char_t *var, char_t *value);

extern int 		ejLexOpen(ej_t* ep);
extern void 	ejLexClose(ej_t* ep);
extern int	 	ejLexOpenScript(ej_t* ep, char_t *script);
extern void 	ejLexCloseScript(ej_t* ep);
extern void 	ejLexSaveInputState(ej_t* ep, ejinput_t* state);
extern void 	ejLexFreeInputState(ej_t* ep, ejinput_t* state);
extern void 	ejLexRestoreInputState(ej_t* ep, ejinput_t* state);
extern int		ejLexGetToken(ej_t* ep, int state);
extern void		ejLexPutbackToken(ej_t* ep, int tid, char_t *string);

extern sym_fd_t	ejGetVariableTable(int eid);
extern sym_fd_t	ejGetFunctionTable(int eid);

extern int		ejEmfOpen(int eid);
extern void		ejEmfClose(int eid);

extern int ejEmfDbRead(int eid, void *handle, int argc, char_t **argv);
extern int ejEmfDbReadKeyed(int eid, void *handle, int argc, char_t **argv);
extern int ejEmfDbTableGetNrow(int eid, void *handle, int argc, char_t **argv);
extern int ejEmfDbDeleteRow(int eid, void *handle, int argc, char_t **argv);
extern int ejEmfTrace(int eid, void *handle, int argc, char_t **argv);
extern int ejEmfDbWrite(int eid, void *handle, int argc, char_t **argv);
extern int ejEmfDbCollectTable(int eid, void *handle, int argc, char_t **argv);

#endif /* _h_EJINTERNAL */

