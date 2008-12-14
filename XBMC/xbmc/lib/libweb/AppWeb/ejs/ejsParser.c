/*
 *	@file 	ejsParser.c
 *	@brief 	EJS Parser and Execution 
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

/********************************** Includes **********************************/

#include	"ejsInternal.h"

#if BLD_FEATURE_EJS

/****************************** Forward Declarations **************************/

static void 	appendValue(MprVar *v1, MprVar *v2);
static int		evalCond(Ejs *ep, MprVar *lhs, int rel, MprVar *rhs);
static int		evalExpr(Ejs *ep, MprVar *lhs, int rel, MprVar *rhs);
#if BLD_FEATURE_FLOATING_POINT
static int 		evalFloatExpr(Ejs *ep, double l, int rel, double r);
#endif 
static int 		evalBoolExpr(Ejs *ep, bool l, int rel, bool r);
static int 		evalNumericExpr(Ejs *ep, MprNum l, int rel, MprNum r);
static int 		evalStringExpr(Ejs *ep, MprVar *lhs, int rel, MprVar *rhs);
static int		evalFunction(Ejs *ep, MprVar *obj, int flags);
static void		freeProc(EjsProc *proc);
static int		parseArgs(Ejs *ep, int state, int flags);
static int		parseAssignment(Ejs *ep, int state, int flags, char *id, 
					char *fullName);
static int		parseCond(Ejs *ep, int state, int flags);
static int		parseDeclaration(Ejs *ep, int state, int flags);
static int		parseExpr(Ejs *ep, int state, int flags);
static int 		parseFor(Ejs *ep, int state, int flags);
static int 		parseForIn(Ejs *ep, int state, int flags);
static int		parseFunctionDec(Ejs *ep, int state, int flags);
static int		parseFunction(Ejs *ep, int state, int flags, char *id);
static int 		parseId(Ejs *ep, int state, int flags, char **id, 
					char **fullName, int *fullNameLen, int *done);
static int 		parseInc(Ejs *ep, int state, int flags);
static int 		parseIf(Ejs *ep, int state, int flags, int *done);
static int		parseStmt(Ejs *ep, int state, int flags);
static void 	removeNewlines(Ejs *ep, int state);
static void 	updateResult(Ejs *ep, int state, int flags, MprVar *vp);

/************************************* Code ***********************************/
/*
 *	Recursive descent parser for EJS
 */

int ejsParse(Ejs *ep, int state, int flags)
{
	mprAssert(ep);

	switch (state) {
	/*
	 *	Any statement, function arguments or conditional expressions
	 */
	case EJS_STATE_STMT:
		if ((state = parseStmt(ep, state, flags)) != EJS_STATE_STMT_DONE &&
			state != EJS_STATE_EOF && state != EJS_STATE_STMT_BLOCK_DONE &&
			state != EJS_STATE_RET) {
			state = EJS_STATE_ERR;
		}
		break;

	case EJS_STATE_DEC:
		if ((state = parseStmt(ep, state, flags)) != EJS_STATE_DEC_DONE &&
			state != EJS_STATE_EOF) {
			state = EJS_STATE_ERR;
		}
		break;

	case EJS_STATE_EXPR:
		if ((state = parseStmt(ep, state, flags)) != EJS_STATE_EXPR_DONE &&
			state != EJS_STATE_EOF) {
			state = EJS_STATE_ERR;
		}
		break;

	/*
	 *	Variable declaration list
	 */
	case EJS_STATE_DEC_LIST:
		state = parseDeclaration(ep, state, flags);
		break;

	/*
	 *	Function argument string
	 */
	case EJS_STATE_ARG_LIST:
		state = parseArgs(ep, state, flags);
		break;

	/*
	 *	Logical condition list (relational operations separated by &&, ||)
	 */
	case EJS_STATE_COND:
		state = parseCond(ep, state, flags);
		break;

	/*
	 *	Expression list
	 */
	case EJS_STATE_RELEXP:
		state = parseExpr(ep, state, flags);
		break;
	}

	if (state == EJS_STATE_ERR && ep->error == NULL) {
		ejsError(ep, "Syntax error");
	}
	return state;
}

/******************************************************************************/
/*
 *	Parse any statement including functions and simple relational operations
 */

static int parseStmt(Ejs *ep, int state, int flags)
{
	EjsProc		*saveProc;
	MprVar		*vp, *saveObj;
	char		*id, *fullName, *initToken;
	int 		done, expectSemi, tid, fullNameLen, rel;
	int 		initId;

	mprAssert(ep);

	expectSemi = 0;
	saveProc = NULL;
	id = 0;
	fullName = 0;
	fullNameLen = 0;

	ep->currentObj = 0;
	ep->currentProperty = 0;

	for (done = 0; !done && state != EJS_STATE_ERR; ) {
		tid = ejsLexGetToken(ep, state);

		switch (tid) {
		default:
			ejsLexPutbackToken(ep, EJS_TOK_EXPR, ep->token);
			done++;
			break;

		case EJS_TOK_EXPR:
			rel = (int) *ep->token;
			if (state == EJS_STATE_EXPR) {
				ejsLexPutbackToken(ep, EJS_TOK_EXPR, ep->token);
			}
			done++;
			break;

		case EJS_TOK_LOGICAL:
			ejsLexPutbackToken(ep, tid, ep->token);
			done++;
			break;

		case EJS_TOK_ERR:
			state = EJS_STATE_ERR;
			done++;
			break;

		case EJS_TOK_EOF:
			state = EJS_STATE_EOF;
			done++;
			break;

		case EJS_TOK_NEWLINE:
			break;

		case EJS_TOK_SEMI:
			/*
			 *	This case is when we discover no statement and just a lone ';'
			 */
			if (state != EJS_STATE_STMT) {
				ejsLexPutbackToken(ep, tid, ep->token);
			}
			done++;
			break;

		case EJS_TOK_PERIOD:
			if (flags & EJS_FLAGS_EXE) {
				if (ep->currentProperty == 0) {
					ejsError(ep, "Undefined object \"%s\"\n", id);
					goto error;
				}
			}
			ep->currentObj = ep->currentProperty;

			if ((tid = ejsLexGetToken(ep, state)) != EJS_TOK_ID) {
				ejsError(ep, "Bad property after '.': %s\n", ep->token);
				goto error;
			}
			mprFree(id);
			id = mprStrdup(ep->token);

			vp = ejsFindProperty(ep, state, ep->currentObj, id, flags);
			updateResult(ep, state, flags, vp);

#if BLD_DEBUG
			fullNameLen = mprReallocStrcat(&fullName, MPR_MAX_VAR, fullNameLen,
				0, ".", (void*) 0);
#endif

			ep->currentProperty = vp;
			ejsLexPutbackToken(ep, tid, ep->token);
			break;

		case EJS_TOK_LBRACKET:
			ep->currentObj = ep->currentProperty;
			saveObj = ep->currentObj;
			if (ejsParse(ep, EJS_STATE_RELEXP, flags) != EJS_STATE_RELEXP_DONE){
				goto error;
			}
			ep->currentObj = saveObj;

			mprFree(id);
			mprVarToString(&id, MPR_MAX_STRING, 0, &ep->result);

			if (id[0] == '\0') {
				if (flags & EJS_FLAGS_EXE) {
					ejsError(ep, 
						"[] expression evaluates to the empty string\n");
					goto error;
				}
			} else {
				vp = ejsFindProperty(ep, state, ep->currentObj, id, flags);
				ep->currentProperty = vp;
				updateResult(ep, state, flags, vp);
			}

#if BLD_DEBUG
			if (id[0] && strlen(id) < (MPR_MAX_VAR / 2)) {
				/*
				 *	If not executing yet, id may not be known
				 */
				fullNameLen = mprReallocStrcat(&fullName, MPR_MAX_VAR, 
					fullNameLen, 0, "[", id, "]", (void*) 0);
			}
#endif

			if ((tid = ejsLexGetToken(ep, state)) != EJS_TOK_RBRACKET) {
				ejsError(ep, "Missing ']'\n");
				goto error;
			}
			break;

		case EJS_TOK_ID:
			state = parseId(ep, state, flags, &id, &fullName, &fullNameLen, 
				&done);
			if (done && state == EJS_STATE_STMT) {
				expectSemi++;
			}
			break;

		case EJS_TOK_ASSIGNMENT:
			state = parseAssignment(ep, state, flags, id, fullName);
			if (state == EJS_STATE_STMT) {
				expectSemi++;
				done++;
			}
			break;

		case EJS_TOK_INC_DEC:
			state = parseInc(ep, state, flags);
			if (state == EJS_STATE_STMT) {
				expectSemi++;
			}
			break;

		case EJS_TOK_NEW:
			if (ejsParse(ep, EJS_STATE_EXPR, flags | EJS_FLAGS_NEW) 
					!= EJS_STATE_EXPR_DONE) {
				goto error;
			}
			break;

		case EJS_TOK_DELETE:
			if (ejsParse(ep, EJS_STATE_EXPR, 
					flags | EJS_FLAGS_DELETE) != EJS_STATE_EXPR_DONE) {
				goto error;
			}
			if (ep->currentObj && ep->currentProperty) {
				mprDeleteProperty(ep->currentObj, ep->currentProperty->name);
			}
			done++;
			break;

		case EJS_TOK_FUNCTION:
			state = parseFunctionDec(ep, state, flags);
			done++;
			break;

		case EJS_TOK_LITERAL:
			/*
			 *	Set the result to the string literal 
			 */
			mprCopyVarValue(&ep->result, mprCreateStringVar(ep->token, 0), 
				MPR_SHALLOW_COPY);
			if (state == EJS_STATE_STMT) {
				expectSemi++;
			}
			done++;
			break;

		case EJS_TOK_NUMBER:
			/*
			 *	Set the result to the parsed number
			 */
			mprCopyVar(&ep->result, &ep->tokenNumber, 0);
			if (state == EJS_STATE_STMT) {
				expectSemi++;
			}
			done++;
			break;

		case EJS_TOK_FUNCTION_NAME:
			state = parseFunction(ep, state, flags, id);
			if (state == EJS_STATE_STMT) {
				expectSemi++;
			}
			if (ep->flags & EJS_FLAGS_EXIT) {
				state = EJS_STATE_RET;
			}
			done++;
			break;

		case EJS_TOK_IF:
			state = parseIf(ep, state, flags, &done);
			if (state == EJS_STATE_RET) {
				goto doneParse;
			}
			break;

		case EJS_TOK_FOR:
			if (state != EJS_STATE_STMT) {
				goto error;
			}
			if (ejsLexGetToken(ep, state) != EJS_TOK_LPAREN) {
				goto error;
			}
			/*
			 *	Need to peek 2-3 tokens ahead and see if this is a 
			 *		for ([var] x in set) 
			 *	or
			 *		for (init ; whileCond; incr)
			 */
			initId = ejsLexGetToken(ep, EJS_STATE_EXPR);
			if (initId == EJS_TOK_ID && strcmp(ep->token, "var") == 0) {
				/*	Simply eat var tokens */
				initId = ejsLexGetToken(ep, EJS_STATE_EXPR);
			}
			initToken = mprStrdup(ep->token);

			tid = ejsLexGetToken(ep, EJS_STATE_EXPR);

			ejsLexPutbackToken(ep, tid, ep->token);
			ejsLexPutbackToken(ep, initId, initToken);
			mprFree(initToken);

			if (tid == EJS_TOK_IN) {
				if ((state = parseForIn(ep, state, flags)) < 0) {
					goto error;
				}
			} else {
				if ((state = parseFor(ep, state, flags)) < 0) {
					goto error;
				}
			}
			done++;
			break;

		case EJS_TOK_VAR:
			if (ejsParse(ep, EJS_STATE_DEC_LIST, flags) 
					!= EJS_STATE_DEC_LIST_DONE) {
				goto error;
			}
			done++;
			break;

		case EJS_TOK_COMMA:
			ejsLexPutbackToken(ep, tid, ep->token);
			done++;
			break;

		case EJS_TOK_LPAREN:
			if (state == EJS_STATE_EXPR) {
				if (ejsParse(ep, EJS_STATE_RELEXP, flags) 
						!= EJS_STATE_RELEXP_DONE) {
					goto error;
				}
				if (ejsLexGetToken(ep, state) != EJS_TOK_RPAREN) {
					goto error;
				}
			}
			done++;
			break;

		case EJS_TOK_RPAREN:
			ejsLexPutbackToken(ep, tid, ep->token);
			done++;
			break;

		case EJS_TOK_LBRACE:
			/*
			 *	This handles any code in braces except "if () {} else {}"
			 */
			if (state != EJS_STATE_STMT) {
				goto error;
			}

			/*
			 *	Parse will return EJS_STATE_STMT_BLOCK_DONE when the RBRACE 
			 *	is seen.
			 */
			do {
				state = ejsParse(ep, EJS_STATE_STMT, flags);
			} while (state == EJS_STATE_STMT_DONE);

			if (state != EJS_STATE_RET) {
				if (ejsLexGetToken(ep, state) != EJS_TOK_RBRACE) {
					goto error;
				}
				state = EJS_STATE_STMT_DONE;
			}
			done++;
			break;

		case EJS_TOK_RBRACE:
			if (state == EJS_STATE_STMT) {
				ejsLexPutbackToken(ep, tid, ep->token);
				state = EJS_STATE_STMT_BLOCK_DONE;
				done++;
				break;
			}
			goto error;

		case EJS_TOK_RETURN:
			if (ejsParse(ep, EJS_STATE_RELEXP, flags) 
					!= EJS_STATE_RELEXP_DONE) {
				goto error;
			}
			if (flags & EJS_FLAGS_EXE) {
				while (ejsLexGetToken(ep, state) != EJS_TOK_EOF) {
					;
				}
				state = EJS_STATE_RET;
				done++;
			}
			break;
		}
	}

	if (expectSemi) {
		tid = ejsLexGetToken(ep, state);
		if (tid != EJS_TOK_SEMI && tid != EJS_TOK_NEWLINE && 
				tid != EJS_TOK_EOF) {
			goto error;
		}

		/*
		 *	Skip newline after semi-colon
		 */
		removeNewlines(ep, state);
	}

/*
 *	Free resources and return the correct status
 */
doneParse:
	mprFree(id);
	mprFree(fullName);

	/*
	 *	Advance the state
	 */
	switch (state) {
	case EJS_STATE_STMT:
		return EJS_STATE_STMT_DONE;

	case EJS_STATE_DEC:
		return EJS_STATE_DEC_DONE;

	case EJS_STATE_EXPR:
		return EJS_STATE_EXPR_DONE;

	case EJS_STATE_STMT_DONE:
	case EJS_STATE_STMT_BLOCK_DONE:
	case EJS_STATE_EOF:
	case EJS_STATE_RET:
		return state;

	default:
		return EJS_STATE_ERR;
	}

/*
 *	Common error exit
 */
error:
	state = EJS_STATE_ERR;
	goto doneParse;
}

/******************************************************************************/
/*
 *	Parse function arguments
 */

static int parseArgs(Ejs *ep, int state, int flags)
{
	int		tid;

	mprAssert(ep);

	do {
		/*
		 *	Peek and see if there are no args
		 */
		tid = ejsLexGetToken(ep, state);
		ejsLexPutbackToken(ep, tid, ep->token);
		if (tid == EJS_TOK_RPAREN) {
			break;
		}

		state = ejsParse(ep, EJS_STATE_RELEXP, flags);
		if (state == EJS_STATE_EOF || state == EJS_STATE_ERR) {
			return state;
		}
		if (state == EJS_STATE_RELEXP_DONE) {
			if (flags & EJS_FLAGS_EXE) {
				mprAssert(ep->proc->args);
				mprAddToArray(ep->proc->args, 
					mprDupVar(&ep->result, MPR_SHALLOW_COPY));
			}
		}
		/*
		 *	Peek at the next token, continue if more args (ie. comma seen)
		 */
		tid = ejsLexGetToken(ep, state);
		if (tid != EJS_TOK_COMMA) {
			ejsLexPutbackToken(ep, tid, ep->token);
		}
	} while (tid == EJS_TOK_COMMA);

	if (tid != EJS_TOK_RPAREN && state != EJS_STATE_RELEXP_DONE) {
		return EJS_STATE_ERR;
	}
	return EJS_STATE_ARG_LIST_DONE;
}

/******************************************************************************/
/*
 *	Parse an assignment statement
 */

static int parseAssignment(Ejs *ep, int state, int flags, char *id, 
	char *fullName)
{
	MprVar		*vp, *saveProperty, *saveObj;

	if (id == 0) {
		return -1;
	}

	saveObj = ep->currentObj;
	saveProperty = ep->currentProperty;
	if (ejsParse(ep, EJS_STATE_RELEXP, flags | EJS_FLAGS_ASSIGNMENT) 
			!= EJS_STATE_RELEXP_DONE) {
		return -1;
	}
	ep->currentObj = saveObj;
	ep->currentProperty = saveProperty;

	if (! (flags & EJS_FLAGS_EXE)) {
		return state;
	}

	if (ep->currentProperty) {
		/*
		 *	Update the variable. Update the property name if not
		 *	yet defined.
		 */
		if (ep->currentProperty->name == 0 || 
				ep->currentProperty->name[0] == '\0') {
			mprSetVarName(ep->currentProperty, id);
		}
		if (mprWriteProperty(ep->currentProperty, &ep->result) < 0){
			ejsError(ep, "Can't write to variable\n");
			return -1;
		}

	} else {
		/*
		 *	Create the variable
		 */
		if (ep->currentObj) {
			if (ep->currentObj->type != MPR_TYPE_OBJECT) {
				if (strcmp(ep->currentObj->name, "session") == 0) {
					ejsError(ep, "Variable \"%s\" is not an array or object." 
						"If using ESP, you need useSession(); in your page.",
						ep->currentObj->name);
				} else {
					ejsError(ep, "Variable \"%s\" is not an array or object", 
						ep->currentObj->name);
				}
				return -1;
			}
			vp = mprCreateProperty(ep->currentObj, id, &ep->result);

		} else {
			/*
			 *	Standard says: "var x" means declare locally.
			 *	"x = 2" means declare globally if x is undefined.
			 */
			if (state == EJS_STATE_DEC) {
				vp = mprCreateProperty(ep->local, id, &ep->result);
			} else {
				vp = mprCreateProperty(ep->global, id, &ep->result);
			}
		}
#if BLD_DEBUG
		mprSetVarFullName(vp, fullName);
#endif
	}
	return state;
}

/******************************************************************************/
/*
 *	Parse conditional expression (relational ops separated by ||, &&)
 */

static int parseCond(Ejs *ep, int state, int flags)
{
	MprVar		lhs, rhs;
	int			tid, oper;

	mprAssert(ep);

	mprDestroyVar(&ep->result);
	rhs = lhs = mprCreateUndefinedVar();
	oper = 0;

	do {
		/*
		 *	Recurse to handle one side of a conditional. Accumulate the
		 *	left hand side and the final result in ep->result.
		 */
		state = ejsParse(ep, EJS_STATE_RELEXP, flags);
		if (state != EJS_STATE_RELEXP_DONE) {
			state = EJS_STATE_ERR;
			break;
		}

		if (oper > 0) {
			mprCopyVar(&rhs, &ep->result, MPR_SHALLOW_COPY);
			if (evalCond(ep, &lhs, oper, &rhs) < 0) {
				state = EJS_STATE_ERR;
				break;
			}
		}
		mprCopyVar(&lhs, &ep->result, MPR_SHALLOW_COPY);

		tid = ejsLexGetToken(ep, state);
		if (tid == EJS_TOK_LOGICAL) {
			oper = (int) *ep->token;

		} else if (tid == EJS_TOK_RPAREN || tid == EJS_TOK_SEMI) {
			ejsLexPutbackToken(ep, tid, ep->token);
			state = EJS_STATE_COND_DONE;
			break;

		} else {
			ejsLexPutbackToken(ep, tid, ep->token);
		}
		tid = (state == EJS_STATE_RELEXP_DONE);

	} while (state == EJS_STATE_RELEXP_DONE);

	mprDestroyVar(&lhs);
	mprDestroyVar(&rhs);
	return state;
}

/******************************************************************************/
/*
 *	Parse variable declaration list. Declarations can be of the following forms:
 *		var x;
 *		var x, y, z;
 *		var x = 1 + 2 / 3, y = 2 + 4;
 *
 *	We set the variable to NULL if there is no associated assignment.
 */

static int parseDeclaration(Ejs *ep, int state, int flags)
{
	int		tid;

	mprAssert(ep);

	do {
		if ((tid = ejsLexGetToken(ep, state)) != EJS_TOK_ID) {
			return EJS_STATE_ERR;
		}
		ejsLexPutbackToken(ep, tid, ep->token);

		/*
		 *	Parse the entire assignment or simple identifier declaration
		 */
		if (ejsParse(ep, EJS_STATE_DEC, flags) != EJS_STATE_DEC_DONE) {
			return EJS_STATE_ERR;
		}

		/*
		 *	Peek at the next token, continue if comma seen
		 */
		tid = ejsLexGetToken(ep, state);
		if (tid == EJS_TOK_SEMI) {
			return EJS_STATE_DEC_LIST_DONE;
		} else if (tid != EJS_TOK_COMMA) {
			return EJS_STATE_ERR;
		}
	} while (tid == EJS_TOK_COMMA);

	if (tid != EJS_TOK_SEMI) {
		return EJS_STATE_ERR;
	}
	return EJS_STATE_DEC_LIST_DONE;
}

/******************************************************************************/
/*
 *	Parse expression (leftHandSide operator rightHandSide)
 */

static int parseExpr(Ejs *ep, int state, int flags)
{
	MprVar		lhs, rhs;
	int			rel, tid;

	mprAssert(ep);

	mprDestroyVar(&ep->result);
	rhs = lhs = mprCreateUndefinedVar();
	rel = 0;
	tid = 0;

	do {
		/*
		 *	This loop will handle an entire expression list. We call parse
		 *	to evalutate each term which returns the result in ep->result.
		 */
		if (tid == EJS_TOK_LOGICAL) {
			state = ejsParse(ep, EJS_STATE_RELEXP, flags);
			if (state != EJS_STATE_RELEXP_DONE) {
				state = EJS_STATE_ERR;
				break;
			}
		} else {
			tid = ejsLexGetToken(ep, state);
			if (tid == EJS_TOK_EXPR && (int) *ep->token == EJS_EXPR_MINUS) {
				lhs = mprCreateIntegerVar(0);
				rel = (int) *ep->token;
			} else {
				ejsLexPutbackToken(ep, tid, ep->token);
			}

			state = ejsParse(ep, EJS_STATE_EXPR, flags);
			if (state != EJS_STATE_EXPR_DONE) {
				state = EJS_STATE_ERR;
				break;
			}
		}

		if (rel > 0) {
			mprCopyVar(&rhs, &ep->result, MPR_SHALLOW_COPY);
			if (tid == EJS_TOK_LOGICAL) {
				if (evalCond(ep, &lhs, rel, &rhs) < 0) {
					state = EJS_STATE_ERR;
					break;
				}
			} else {
				if (evalExpr(ep, &lhs, rel, &rhs) < 0) {
					state = EJS_STATE_ERR;
					break;
				}
			}
		}
		mprCopyVar(&lhs, &ep->result, MPR_SHALLOW_COPY);

		if ((tid = ejsLexGetToken(ep, state)) == EJS_TOK_EXPR ||
			 tid == EJS_TOK_INC_DEC || tid == EJS_TOK_LOGICAL) {
			rel = (int) *ep->token;

		} else {
			ejsLexPutbackToken(ep, tid, ep->token);
			state = EJS_STATE_RELEXP_DONE;
		}

	} while (state == EJS_STATE_EXPR_DONE);

	mprDestroyVar(&lhs);
	mprDestroyVar(&rhs);

	return state;
}

/******************************************************************************/
/*
 *	Parse the "for ... in" statement. Format for the statement is:
 *
 *		for (var in expr) {
 *			body;
 *		}
 */

static int parseForIn(Ejs *ep, int state, int flags)
{
	EjsInput	endScript, bodyScript;
	MprVar		*iteratorVar, *setVar, *vp, v;
	int			forFlags, tid;

	mprAssert(ep);

	tid = ejsLexGetToken(ep, state);
	if (tid != EJS_TOK_ID) {
		return -1;
	}
	ejsLexPutbackToken(ep, tid, ep->token);

	if (ejsParse(ep, EJS_STATE_EXPR, EJS_FLAGS_FOREACH | EJS_FLAGS_EXE)
			!= EJS_STATE_EXPR_DONE) {
		return -1;
	}
	if (ep->currentProperty == 0) {
		return -1;
	}
	iteratorVar = ep->currentProperty;
	
	if (ejsLexGetToken(ep, state) != EJS_TOK_IN) {
		return -1;
	}

	/*
	 *	Get the set
	 */
	tid = ejsLexGetToken(ep, state);
	if (tid != EJS_TOK_ID) {
		return -1;
	}
	ejsLexPutbackToken(ep, tid, ep->token);

	if (ejsParse(ep, EJS_STATE_EXPR, flags) != EJS_STATE_EXPR_DONE) {
		return -1;
	}
	if (ep->currentProperty == 0 && flags & EJS_FLAGS_EXE) {
		return -1;
	}
	setVar = ep->currentProperty;
	
	if (ejsLexGetToken(ep, state) != EJS_TOK_RPAREN) {
		return -1;
	}

	/*
	 *	Parse the body and remember the end of the body script
	 */
	forFlags = flags & ~EJS_FLAGS_EXE;
	ejsLexSaveInputState(ep, &bodyScript);
	if (ejsParse(ep, EJS_STATE_STMT, forFlags) != EJS_STATE_STMT_DONE) {
		ejsLexFreeInputState(ep, &bodyScript);
		return -1;
	}
	ejsInitInputState(&endScript);
	ejsLexSaveInputState(ep, &endScript);

	/*
	 *	Now actually do the for loop.
	 */
	if (flags & EJS_FLAGS_EXE) {
		if (setVar->type == MPR_TYPE_OBJECT) {
			vp = mprGetFirstProperty(setVar, MPR_ENUM_DATA);
			while (vp) {
				if (strcmp(vp->name, "length") != 0) {
					v = mprCreateStringVar(vp->name, 0);
					if (mprWriteProperty(iteratorVar, &v) < 0) {
						ejsError(ep, "Can't write to variable\n");
						ejsLexFreeInputState(ep, &bodyScript);
						ejsLexFreeInputState(ep, &endScript);
						return -1;
					}

					ejsLexRestoreInputState(ep, &bodyScript);
					switch (ejsParse(ep, EJS_STATE_STMT, flags)) {
					case EJS_STATE_RET:
						return EJS_STATE_RET;
					case EJS_STATE_STMT_DONE:
						break;
					default:
						ejsLexFreeInputState(ep, &endScript);
						ejsLexFreeInputState(ep, &bodyScript);
						return -1;
					}
				}
				vp = mprGetNextProperty(setVar, vp, MPR_ENUM_DATA);
			}
		} else {
			ejsError(ep, "Variable \"%s\" is not an array or object", 
				setVar->name);
			ejsLexFreeInputState(ep, &endScript);
			ejsLexFreeInputState(ep, &bodyScript);
			return -1;
		}
	}
	ejsLexRestoreInputState(ep, &endScript);

	ejsLexFreeInputState(ep, &endScript);
	ejsLexFreeInputState(ep, &bodyScript);

	return state;
}

/******************************************************************************/
/*
 *	Parse the for statement. Format for the expression is:
 *
 *		for (initial; condition; incr) {
 *			body;
 *		}
 */

static int parseFor(Ejs *ep, int state, int flags)
{
	EjsInput	condScript, endScript, bodyScript, incrScript;
	int			forFlags, cond;

	ejsInitInputState(&endScript);
	ejsInitInputState(&bodyScript);
	ejsInitInputState(&incrScript);
	ejsInitInputState(&condScript);

	mprAssert(ep);

	/*
	 *	Evaluate the for loop initialization statement
	 */
	if (ejsParse(ep, EJS_STATE_EXPR, flags) != EJS_STATE_EXPR_DONE) {
		return -1;
	}
	if (ejsLexGetToken(ep, state) != EJS_TOK_SEMI) {
		return -1;
	}

	/*
	 *	The first time through, we save the current input context just prior
	 *	to each step: prior to the conditional, the loop increment and 
 	 *	the loop body.
	 */
	ejsLexSaveInputState(ep, &condScript);
	if (ejsParse(ep, EJS_STATE_COND, flags) != EJS_STATE_COND_DONE) {
		goto error;
	}
	cond = (ep->result.boolean != 0);

	if (ejsLexGetToken(ep, state) != EJS_TOK_SEMI) {
		goto error;
	}

	/*
	 *	Don't execute the loop increment statement or the body 
	 *	first time.
	 */
	forFlags = flags & ~EJS_FLAGS_EXE;
	ejsLexSaveInputState(ep, &incrScript);
	if (ejsParse(ep, EJS_STATE_EXPR, forFlags) != EJS_STATE_EXPR_DONE) {
		goto error;
	}
	if (ejsLexGetToken(ep, state) != EJS_TOK_RPAREN) {
		goto error;
	}

	/*
	 *	Parse the body and remember the end of the body script
	 */
	ejsLexSaveInputState(ep, &bodyScript);
	if (ejsParse(ep, EJS_STATE_STMT, forFlags) != EJS_STATE_STMT_DONE) {
		goto error;
	}
	ejsLexSaveInputState(ep, &endScript);

	/*
	 *	Now actually do the for loop. Note loop has been rotated
	 */
	while (cond && (flags & EJS_FLAGS_EXE)) {
		/*
		 *	Evaluate the body
		 */
		ejsLexRestoreInputState(ep, &bodyScript);

		switch (ejsParse(ep, EJS_STATE_STMT, flags)) {
		case EJS_STATE_RET:
			return EJS_STATE_RET;
		case EJS_STATE_STMT_DONE:
			break;
		default:
			goto error;
		}
		/*
		 *	Evaluate the increment script
		 */
		ejsLexRestoreInputState(ep, &incrScript);
		if (ejsParse(ep, EJS_STATE_EXPR, flags) != EJS_STATE_EXPR_DONE){
			goto error;
		}
		/*
		 *	Evaluate the condition
		 */
		ejsLexRestoreInputState(ep, &condScript);
		if (ejsParse(ep, EJS_STATE_COND, flags) != EJS_STATE_COND_DONE) {
			goto error;
		}
		mprAssert(ep->result.type == MPR_TYPE_BOOL);
		cond = (ep->result.boolean != 0);
	}

	ejsLexRestoreInputState(ep, &endScript);

done:
	ejsLexFreeInputState(ep, &condScript);
	ejsLexFreeInputState(ep, &incrScript);
	ejsLexFreeInputState(ep, &endScript);
	ejsLexFreeInputState(ep, &bodyScript);
	return state;

error:
	state = EJS_STATE_ERR;
	goto done;
}

/******************************************************************************/
/*
 *	Parse a function declaration
 */

static int parseFunctionDec(Ejs *ep, int state, int flags)
{
	EjsInput	endScript, bodyScript;
	MprVar		v, *currentObj, *vp;
	char		*procName;
	int			len, tid, bodyFlags;

	mprAssert(ep);
	mprAssert(ejsPtr(ep->eid));

	/*	
	 *	function <name>(arg, arg, arg) { body };
	 *	function name(arg, arg, arg) { body };
	 */

	tid = ejsLexGetToken(ep, state);
	if (tid == EJS_TOK_ID) {
		procName = mprStrdup(ep->token);
		tid = ejsLexGetToken(ep, state);
	}  else {
		procName = 0;
	}
	if (tid != EJS_TOK_LPAREN) {
		mprFree(procName);
		return EJS_STATE_ERR;
	}

	/*
 	 *	Hand craft the function value structure.
	 */
	v = mprCreateFunctionVar(0, 0, 0);
	tid = ejsLexGetToken(ep, state);
	while (tid == EJS_TOK_ID) {
		mprAddToArray(v.function.args, mprStrdup(ep->token));
		tid = ejsLexGetToken(ep, state);
		if (tid == EJS_TOK_RPAREN || tid != EJS_TOK_COMMA) {
			break;
		}
		tid = ejsLexGetToken(ep, state);
	}
	if (tid != EJS_TOK_RPAREN) {
		mprFree(procName);
		mprDestroyVar(&v);
		return EJS_STATE_ERR;
	}

	/* Allow new lines before opening brace */
	do {
		tid = ejsLexGetToken(ep, state);
	} while (tid == EJS_TOK_NEWLINE);

	if (tid != EJS_TOK_LBRACE) {
		mprFree(procName);
		mprDestroyVar(&v);
		return EJS_STATE_ERR;
	}
	
	/*
	 *	Register before parsing the body to support recursion.
	 */
	if (! (flags & EJS_FLAGS_ASSIGNMENT)) {
		currentObj = ejsFindObj(ep, 0, procName, flags);
		vp = mprSetProperty(currentObj, procName, &v);
	}

	/*
	 *	Parse the function body. Turn execute off.
	 */
	bodyFlags = flags & ~EJS_FLAGS_EXE;
	ejsLexSaveInputState(ep, &bodyScript);

	do {
		state = ejsParse(ep, EJS_STATE_STMT, bodyFlags);
	} while (state == EJS_STATE_STMT_DONE);

	tid = ejsLexGetToken(ep, state);
	if (state != EJS_STATE_STMT_BLOCK_DONE || tid != EJS_TOK_RBRACE) {
		mprFree(procName);
		mprDestroyVar(&v);
		ejsLexFreeInputState(ep, &bodyScript);
		return EJS_STATE_ERR;
	}
	ejsLexSaveInputState(ep, &endScript);

	/*
	 *	Save the function body between the starting and ending parse positions.
	 *	Overwrite the trailing '}' with a null.
	 */
	len = (int) (endScript.scriptServp - bodyScript.scriptServp);
	v.function.body = (char*) mprMalloc(len + 1);
	memcpy(v.function.body, bodyScript.scriptServp, len);

	if (len <= 0) {
		v.function.body[0] = '\0';
	} else {
		v.function.body[len - 1] = '\0';
	}
	ejsLexFreeInputState(ep, &bodyScript);
	ejsLexFreeInputState(ep, &endScript);

	/*
	 *	If we are in an assignment, don't register the function name, rather
	 *	return the function structure in the parser result.
	 */
	if (flags & EJS_FLAGS_ASSIGNMENT) {
		mprCopyVar(&ep->result, &v, MPR_SHALLOW_COPY);
	} else {
		currentObj = ejsFindObj(ep, 0, procName, flags);
		vp = mprSetProperty(currentObj, procName, &v);
	}

	mprFree(procName);
	mprDestroyVar(&v);

	return EJS_STATE_STMT;
}

/******************************************************************************/
/*
 *	Parse a function name and invoke the function
 */

static int parseFunction(Ejs *ep, int state, int flags, char *id)
{
	EjsProc		proc, *saveProc;
	MprVar		*saveObj;

	/*
	 *	Must save any current ep->proc value for the current stack frame
	 *	to allow for recursive function calls.
	 */
	saveProc = (ep->proc) ? ep->proc: 0;

	memset(&proc, 0, sizeof(EjsProc));
	proc.procName = mprStrdup(id);
	proc.fn = ep->currentProperty;
	proc.args = mprCreateArray();
	ep->proc = &proc;

	mprDestroyVar(&ep->result);

	saveObj = ep->currentObj;
	if (ejsParse(ep, EJS_STATE_ARG_LIST, flags) != EJS_STATE_ARG_LIST_DONE) {
		freeProc(&proc);
		ep->proc = saveProc;
		return -1;
	}
	ep->currentObj = saveObj;

	/*
	 *	Evaluate the function if required
	 */
	if (flags & EJS_FLAGS_EXE) {
		if (evalFunction(ep, ep->currentObj, flags) < 0) {
			freeProc(&proc);
			ep->proc = saveProc;
			return -1;
		}
	}

	freeProc(&proc);
	ep->proc = saveProc;

	if (ejsLexGetToken(ep, state) != EJS_TOK_RPAREN) {
		return -1;
	}
	return state;
}

/******************************************************************************/
/*
 *	Parse an identifier. This is a segment of a fully qualified variable.
 *	May come here for an initial identifier or for property names
 *	after a "." or "[...]".
 */

static int parseId(Ejs *ep, int state, int flags, char **id, char **fullName, 
	int *fullNameLen, int *done)
{
	int		tid;

	mprFree(*id);
	*id = mprStrdup(ep->token);
#if BLD_DEBUG
	*fullNameLen = mprReallocStrcat(fullName, MPR_MAX_VAR, *fullNameLen,
		0, *id, (void*) 0);
#endif
	if (ep->currentObj == 0) {
		ep->currentObj = ejsFindObj(ep, state, *id, flags);
	}

	/*
	 *	Find the referenced variable and store it in currentProperty.
	  */
	ep->currentProperty = ejsFindProperty(ep, state, ep->currentObj, 
		*id, flags);
	updateResult(ep, state, flags, ep->currentProperty);

#if BLD_DEBUG
	if (ep->currentProperty && (ep->currentProperty->name == 0 || 
			ep->currentProperty->name[0] == '\0')) {
		mprSetVarName(ep->currentProperty, *id);
	}
#endif

	tid = ejsLexGetToken(ep, state);
	if (tid == EJS_TOK_LPAREN) {
		if (ep->currentProperty == 0) {
			ejsError(ep, "Function name not defined \"%s\"\n", *id);
			return -1;
		}
		ejsLexPutbackToken(ep, EJS_TOK_FUNCTION_NAME, ep->token);
		return state;
	}

	if (tid == EJS_TOK_PERIOD || tid == EJS_TOK_LBRACKET || 
			tid == EJS_TOK_ASSIGNMENT || tid == EJS_TOK_INC_DEC) {
		ejsLexPutbackToken(ep, tid, ep->token);
		return state;
	}

	/*
	 *	Only come here for variable access and declarations.
	 *	Assignment handled elsewhere.
	 */
	if (flags & EJS_FLAGS_EXE) {
		if (state == EJS_STATE_DEC) {
			/*
 			 *	Declare a variable. Standard allows: var x ; var x ;
			 */
#if DISABLED
			if (ep->currentProperty != 0) {
				ejsError(ep, "Variable already defined \"%s\"\n", *id);
				return -1;
			}
#endif
			/*
			 *	Create or overwrite if it already exists
			 */
			mprSetPropertyValue(ep->currentObj, *id, 
				mprCreateUndefinedVar());
			ep->currentProperty = 0;
			mprDestroyVar(&ep->result);

		} else if (flags & EJS_FLAGS_FOREACH) {
			if (ep->currentProperty == 0) {
				ep->currentProperty = 
					mprCreatePropertyValue(ep->currentObj, *id, 
						mprCreateUndefinedVar());
			}

		} else {
			if (ep->currentProperty == 0) {
				if (ep->currentObj == ep->global || 
						ep->currentObj == ep->local) {
					ejsError(ep, "Undefined variable \"%s\"\n", *id);
					return -1;
				}
				ep->currentProperty = mprCreatePropertyValue(ep->currentObj, 
					*id, mprCreateUndefinedVar());
			}
		}
	}
	ejsLexPutbackToken(ep, tid, ep->token);
	if (tid == EJS_TOK_RBRACKET || tid == EJS_TOK_COMMA || 
			tid == EJS_TOK_IN) {
		*done = 1;
	}
	return state;
}

/******************************************************************************/
/*
 *	Parse an "if" statement
 */

static int parseIf(Ejs *ep, int state, int flags, int *done)
{
	bool	ifResult;
	int		thenFlags, elseFlags, tid;

	if (state != EJS_STATE_STMT) {
		return -1;
	}
	if (ejsLexGetToken(ep, state) != EJS_TOK_LPAREN) {
		return -1;
	}

	/*
	 *	Evaluate the entire condition list "(condition)"
	 */
	if (ejsParse(ep, EJS_STATE_COND, flags) != EJS_STATE_COND_DONE) {
		return -1;
	}
	if (ejsLexGetToken(ep, state) != EJS_TOK_RPAREN) {
		return -1;
	}

	/*
	 *	This is the "then" case. We need to always parse both cases and
	 *	execute only the relevant case.
	 */
	ifResult = mprVarToBool(&ep->result);
	if (ifResult) {
		thenFlags = flags;
		elseFlags = flags & ~EJS_FLAGS_EXE;
	} else {
		thenFlags = flags & ~EJS_FLAGS_EXE;
		elseFlags = flags;
	}

	/*
	 *	Process the "then" case.
	 */
	switch (ejsParse(ep, EJS_STATE_STMT, thenFlags)) {
	case EJS_STATE_RET:
		state = EJS_STATE_RET;
		return state;
	case EJS_STATE_STMT_DONE:
		break;
	default:
		return -1;
	}

	/*
	 *	Check to see if there is an "else" case
	 */
	removeNewlines(ep, state);
	tid = ejsLexGetToken(ep, state);
	if (tid != EJS_TOK_ELSE) {
		ejsLexPutbackToken(ep, tid, ep->token);
		*done = 1;
		return state;
	}

	/*
	 *	Process the "else" case.
	 */
	switch (ejsParse(ep, EJS_STATE_STMT, elseFlags)) {
	case EJS_STATE_RET:
		state = EJS_STATE_RET;
		return state;
	case EJS_STATE_STMT_DONE:
		break;
	default:
		return -1;
	}
	*done = 1;
	return state;
}

/******************************************************************************/
/*
 *	Parse an "++" or "--" statement
 */

static int parseInc(Ejs *ep, int state, int flags)
{
	MprVar	one;

	if (! (flags & EJS_FLAGS_EXE)) {
		return state;
	}

	if (ep->currentProperty == 0) {
		ejsError(ep, "Undefined variable \"%s\"\n", ep->token);
		return -1;
	}
	one = mprCreateIntegerVar(1);
	if (evalExpr(ep, ep->currentProperty, (int) *ep->token, 
			&one) < 0) {
		return -1;
	}
	if (mprWriteProperty(ep->currentProperty, &ep->result) < 0) {
		ejsError(ep, "Can't write to variable\n");
		return -1;
	}
	return state;
}

/******************************************************************************/
/*
 *	Evaluate a condition. Implements &&, ||, !. Returns with a boolean result
 *	in ep->result. Returns -1 on errors, zero if successful.
 */

static int evalCond(Ejs *ep, MprVar *lhs, int rel, MprVar *rhs)
{
	bool	l, r, lval;

	mprAssert(rel > 0);

	l = mprVarToBool(lhs);
	r = mprVarToBool(rhs);

	switch (rel) {
	case EJS_COND_AND:
		lval = l && r;
		break;
	case EJS_COND_OR:
		lval = l || r;
		break;
	default:
		ejsError(ep, "Bad operator %d", rel);
		return -1;
	}

	mprCopyVarValue(&ep->result, mprCreateBoolVar(lval), 0);
	return 0;
}

/******************************************************************************/
/*
 *	Evaluate an operation. Returns with the result in ep->result. Returns -1
 *	on errors, otherwise zero is returned.
 */

static int evalExpr(Ejs *ep, MprVar *lhs, int rel, MprVar *rhs)
{
	char	*str;
	MprNum	lval, num;
	int		rc;

	mprAssert(rel > 0);
	str = 0;
	lval = 0;

	/*
 	 *	Type conversion. This is tricky and must be according to the standard.
	 *	Only numbers (including floats) and strings can be compared. All other
	 *	types are first converted to numbers by preference and if that fails,
	 *	to strings.
	 *
	 *	First convert objects to comparable types. The "===" operator will
	 *	test the sameness of object references. Here, we coerce to comparable
	 *	types first.
	 */
	if (lhs->type == MPR_TYPE_OBJECT) {
		if (ejsRunFunction(ep->eid, lhs, "toValue", 0) == 0) {
			mprCopyVar(lhs, &ep->result, MPR_SHALLOW_COPY);
		} else {
			if (ejsRunFunction(ep->eid, lhs, "toString", 0) == 0) {
				mprCopyVar(lhs, &ep->result, MPR_SHALLOW_COPY);
			}
		}
		/* Nothing more can be done */
	}

	if (rhs->type == MPR_TYPE_OBJECT) {
		if (ejsRunFunction(ep->eid, rhs, "toValue", 0) == 0) {
			mprCopyVar(rhs, &ep->result, MPR_SHALLOW_COPY);
		} else {
			if (ejsRunFunction(ep->eid, rhs, "toString", 0) == 0) {
				mprCopyVar(rhs, &ep->result, MPR_SHALLOW_COPY);
			}
		}
		/* Nothing more can be done */
	}

	/*
 	 *	From here on, lhs and rhs may contain allocated data (strings), so 
	 *	we must always destroy before overwriting.
	 */
	
	/*
	 *	Only allow a few bool operations. Otherwise convert to number.
 	 */
	if (lhs->type == MPR_TYPE_BOOL && rhs->type == MPR_TYPE_BOOL &&
			(rel != EJS_EXPR_EQ && rel != EJS_EXPR_NOTEQ &&
			rel != EJS_EXPR_BOOL_COMP)) {
		num = mprVarToNumber(lhs);
		mprDestroyVar(lhs);
		*lhs = mprCreateNumberVar(num);
	}

	/*
 	 *	Types do not match, so try to coerce the right operand to match the left
 	 *	But first, try to convert a left operand that is a numeric stored as a
	 *	string, into a numeric.
	 */
	if (lhs->type != rhs->type) {
		if (lhs->type == MPR_TYPE_STRING) {
			if (isdigit((int) lhs->string[0])) {
				num = mprVarToNumber(lhs);
				mprDestroyVar(lhs);
				*lhs = mprCreateNumberVar(num);
				/* Examine further below */

			} else {
				/*
				 *	Convert the RHS to a string
				 */
				mprVarToString(&str, MPR_MAX_STRING, 0, rhs);
				mprDestroyVar(rhs);
				*rhs = mprCreateStringVar(str, 1);
				mprFree(str);
			}

#if BLD_FEATURE_FLOATING_POINT
		} else if (lhs->type == MPR_TYPE_FLOAT) {
			/*
			 *	Convert rhs to floating
			 */
			double f = mprVarToFloat(rhs);
			mprDestroyVar(rhs);
			*rhs = mprCreateFloatVar(f);

#endif
		} else if (lhs->type == MPR_TYPE_INT64) {
			/*
			 *	Convert the rhs to 64 bit
			 */
			int64 n = mprVarToInteger64(rhs);
			mprDestroyVar(rhs);
			*rhs = mprCreateInteger64Var(n);

		} else if (lhs->type == MPR_TYPE_BOOL || lhs->type == MPR_TYPE_INT) {

			if (rhs->type == MPR_TYPE_STRING) {
				/*
				 *	Convert to lhs to a string
				 */
				mprVarToString(&str, MPR_MAX_STRING, 0, lhs);
				mprDestroyVar(lhs);
				*lhs = mprCreateStringVar(str, 1);
				mprFree(str);

#if BLD_FEATURE_FLOATING_POINT
			} else if (rhs->type == MPR_TYPE_FLOAT) {
				/*
				 *	Convert lhs to floating
				 */
				double f = mprVarToFloat(lhs);
				mprDestroyVar(lhs);
				*lhs = mprCreateFloatVar(f);
#endif

			} else {
				/*
				 *	Convert both operands to numbers
				 */
				num = mprVarToNumber(lhs);
				mprDestroyVar(lhs);
				*lhs = mprCreateNumberVar(num);

				num = mprVarToNumber(rhs);
				mprDestroyVar(rhs);
				*rhs = mprCreateNumberVar(num);
			}
		}
	}

	/*
 	 *	Special case here for undefined and null. We need to allow comparisions against these
	 *	special values.
	 */
	if (lhs->type == MPR_TYPE_UNDEFINED || lhs->type == MPR_TYPE_NULL ||
			rhs->type == MPR_TYPE_UNDEFINED || rhs->type == MPR_TYPE_NULL) {
		switch (rel) {
		case EJS_EXPR_EQ:
			lval = lhs->type == rhs->type;
			break;
		case EJS_EXPR_NOTEQ:
			lval = lhs->type != rhs->type;
			break;
		default:
			lval = 0;
		}
		mprCopyVarValue(&ep->result, mprCreateBoolVar((bool) lval), 0);
		return 0;
	}

	/*
	 *	Types are the same here
 	 */
	switch (lhs->type) {
	default:
	case MPR_TYPE_UNDEFINED:
	case MPR_TYPE_NULL:
		/* Should be handled above */
		mprAssert(0);
		return 0;

	case MPR_TYPE_STRING_CFUNCTION:
	case MPR_TYPE_CFUNCTION:
	case MPR_TYPE_FUNCTION:
	case MPR_TYPE_OBJECT:
		mprCopyVarValue(&ep->result, mprCreateBoolVar(0), 0);
		return 0;

	case MPR_TYPE_BOOL:
		rc = evalBoolExpr(ep, lhs->boolean, rel, rhs->boolean);
		break;

#if BLD_FEATURE_FLOATING_POINT
	case MPR_TYPE_FLOAT:
		rc = evalFloatExpr(ep, lhs->floating, rel, rhs->floating);
		break;
#endif

	case MPR_TYPE_INT:
		rc = evalNumericExpr(ep, (MprNum) lhs->integer, rel, 
			(MprNum) rhs->integer);
		break;

	case MPR_TYPE_INT64:
		rc = evalNumericExpr(ep, (MprNum) lhs->integer64, rel, 
			(MprNum) rhs->integer64);
		break;

	case MPR_TYPE_STRING:
		rc = evalStringExpr(ep, lhs, rel, rhs);
	}
	return rc;
}

/******************************************************************************/
#if BLD_FEATURE_FLOATING_POINT
/*
 *	Expressions with floating operands
 */

static int evalFloatExpr(Ejs *ep, double l, int rel, double r) 
{
	double	lval;
	bool	logical;

	lval = 0;
	logical = 0;

	switch (rel) {
	case EJS_EXPR_PLUS:
		lval = l + r;
		break;
	case EJS_EXPR_INC:
		lval = l + 1;
		break;
	case EJS_EXPR_MINUS:
		lval = l - r;
		break;
	case EJS_EXPR_DEC:
		lval = l - 1;
		break;
	case EJS_EXPR_MUL:
		lval = l * r;
		break;
	case EJS_EXPR_DIV:
		lval = l / r;
		break;
	default:
		logical++;
		break;
	}

	/*
	 *	Logical operators
	 */
	if (logical) {

		switch (rel) {
		case EJS_EXPR_EQ:
			lval = l == r;
			break;
		case EJS_EXPR_NOTEQ:
			lval = l != r;
			break;
		case EJS_EXPR_LESS:
			lval = (l < r) ? 1 : 0;
			break;
		case EJS_EXPR_LESSEQ:
			lval = (l <= r) ? 1 : 0;
			break;
		case EJS_EXPR_GREATER:
			lval = (l > r) ? 1 : 0;
			break;
		case EJS_EXPR_GREATEREQ:
			lval = (l >= r) ? 1 : 0;
			break;
		case EJS_EXPR_BOOL_COMP:
			lval = (r == 0) ? 1 : 0;
			break;
		default:
			ejsError(ep, "Bad operator %d", rel);
			return -1;
		}
		mprCopyVarValue(&ep->result, mprCreateBoolVar(lval != 0), 0);

	} else {
		mprCopyVarValue(&ep->result, mprCreateFloatVar(lval), 0);
	}
	return 0;
}

#endif /* BLD_FEATURE_FLOATING_POINT */
/******************************************************************************/
/*
 *	Expressions with boolean operands
 */

static int evalBoolExpr(Ejs *ep, bool l, int rel, bool r) 
{
	bool	lval;

	switch (rel) {
	case EJS_EXPR_EQ:
		lval = l == r;
		break;
	case EJS_EXPR_NOTEQ:
		lval = l != r;
		break;
	case EJS_EXPR_BOOL_COMP:
		lval = (r == 0) ? 1 : 0;
		break;
	default:
		ejsError(ep, "Bad operator %d", rel);
		return -1;
	}
	mprCopyVarValue(&ep->result, mprCreateBoolVar(lval), 0);
	return 0;
}

/******************************************************************************/
/*
 *	Expressions with numeric operands
 */

static int evalNumericExpr(Ejs *ep, MprNum l, int rel, MprNum r) 
{
	MprNum	lval;
	bool	logical;

	lval = 0;
	logical = 0;

	switch (rel) {
	case EJS_EXPR_PLUS:
		lval = l + r;
		break;
	case EJS_EXPR_INC:
		lval = l + 1;
		break;
	case EJS_EXPR_MINUS:
		lval = l - r;
		break;
	case EJS_EXPR_DEC:
		lval = l - 1;
		break;
	case EJS_EXPR_MUL:
		lval = l * r;
		break;
	case EJS_EXPR_DIV:
		if (r != 0) {
			lval = l / r;
		} else {
			ejsError(ep, "Divide by zero");
			return -1;
		}
		break;
	case EJS_EXPR_MOD:
		if (r != 0) {
			lval = l % r;
		} else {
			ejsError(ep, "Modulo zero");
			return -1;
		}
		break;
	case EJS_EXPR_LSHIFT:
		lval = l << r;
		break;
	case EJS_EXPR_RSHIFT:
		lval = l >> r;
		break;

	default:
		logical++;
		break;
	}

	/*
	 *	Logical operators
	 */
	if (logical) {

		switch (rel) {
		case EJS_EXPR_EQ:
			lval = l == r;
			break;
		case EJS_EXPR_NOTEQ:
			lval = l != r;
			break;
		case EJS_EXPR_LESS:
			lval = (l < r) ? 1 : 0;
			break;
		case EJS_EXPR_LESSEQ:
			lval = (l <= r) ? 1 : 0;
			break;
		case EJS_EXPR_GREATER:
			lval = (l > r) ? 1 : 0;
			break;
		case EJS_EXPR_GREATEREQ:
			lval = (l >= r) ? 1 : 0;
			break;
		case EJS_EXPR_BOOL_COMP:
			lval = (r == 0) ? 1 : 0;
			break;
		default:
			ejsError(ep, "Bad operator %d", rel);
			return -1;
		}
		mprCopyVarValue(&ep->result, mprCreateBoolVar(lval != 0), 0);

	} else {
		mprCopyVarValue(&ep->result, mprCreateNumberVar(lval), 0);
	}
	return 0;
}

/******************************************************************************/
/*
 *	Expressions with string operands
 */

static int evalStringExpr(Ejs *ep, MprVar *lhs, int rel, MprVar *rhs)
{
	int		lval;

	mprAssert(ep);
	mprAssert(lhs);
	mprAssert(rhs);

	switch (rel) {
	case EJS_EXPR_LESS:
		lval = strcmp(lhs->string, rhs->string) < 0;
		break;
	case EJS_EXPR_LESSEQ:
		lval = strcmp(lhs->string, rhs->string) <= 0;
		break;
	case EJS_EXPR_GREATER:
		lval = strcmp(lhs->string, rhs->string) > 0;
		break;
	case EJS_EXPR_GREATEREQ:
		lval = strcmp(lhs->string, rhs->string) >= 0;
		break;
	case EJS_EXPR_EQ:
		lval = strcmp(lhs->string, rhs->string) == 0;
		break;
	case EJS_EXPR_NOTEQ:
		lval = strcmp(lhs->string, rhs->string) != 0;
		break;
	case EJS_EXPR_PLUS:
		/*
 		 *	This differs from all the above operations. We append rhs to lhs.
		 */
		mprDestroyVar(&ep->result);
		appendValue(&ep->result, lhs);
		appendValue(&ep->result, rhs);
		return 0;

	case EJS_EXPR_INC:
	case EJS_EXPR_DEC:
	case EJS_EXPR_MINUS:
	case EJS_EXPR_DIV:
	case EJS_EXPR_MOD:
	case EJS_EXPR_LSHIFT:
	case EJS_EXPR_RSHIFT:
	default:
		ejsError(ep, "Bad operator");
		return -1;
	}

	mprCopyVarValue(&ep->result, mprCreateBoolVar(lval), 0);
	return 0;
}

/******************************************************************************/
/*
 *	Evaluate a function. obj is set to the current object if a function is being
 *	run.
 */

static int evalFunction(Ejs *ep, MprVar *obj, int flags)
{
	EjsProc		*proc;
	MprVar		arguments, callee, thisObject, *prototype, **argValues;
	MprArray	*formalArgs, *actualArgs;
	char		buf[16], **argNames, **argBuf;
	int			i, rc, fid;

	mprAssert(ep); 
	mprAssert(ejsPtr(ep->eid));

	rc = -1;
	proc = ep->proc;
	prototype = proc->fn;
	actualArgs = proc->args;
	argValues = (MprVar**) actualArgs->handles;

	/*
	 *	Create a new variable stack frame. ie. new local variables.
 	 */
	fid = ejsOpenBlock(ep->eid);

	if (flags & EJS_FLAGS_NEW) {
		/*
 		 *	Create a new bare object and pass it into the constructor as the 
		 *	"this" local variable. 
		 */
		thisObject = ejsCreateObj("this", EJS_OBJ_HASH_SIZE);
		mprCreatePropertyValue(ep->local, "this", thisObject);

	} else if (obj) {
		mprCreateProperty(ep->local, "this", obj);
	}

	switch (prototype->type) {
	default:
		mprAssert(0);
		break;

	case MPR_TYPE_STRING_CFUNCTION:
		if (actualArgs->used > 0) {
			argBuf = (char**) mprMalloc(actualArgs->used * sizeof(char*));
			for (i = 0; i < actualArgs->used; i++) {
				mprVarToString(&argBuf[i], MPR_MAX_STRING, 0, argValues[i]);
			}
		} else {
			argBuf = 0;
		}

		/*
		 *	Call the function depending on the various handle flags
		 */
		ep->thisPtr = prototype->cFunctionWithStrings.thisPtr;
		if (prototype->flags & MPR_VAR_ALT_HANDLE) {
			rc = ((EjsAltStringCFunction) prototype->cFunctionWithStrings.fn)
				((EjsHandle) ep->eid, ep->altHandle, actualArgs->used, argBuf);
		} else if (prototype->flags & MPR_VAR_SCRIPT_HANDLE) {
			rc = (prototype->cFunctionWithStrings.fn)((EjsHandle) ep->eid, 
				actualArgs->used, argBuf);
		} else {
			rc = (prototype->cFunctionWithStrings.fn)(ep->primaryHandle, 
				actualArgs->used, argBuf);
		}

		if (actualArgs->used > 0) {
			for (i = 0; i < actualArgs->used; i++) {
				mprFree(argBuf[i]);
			}
			mprFree(argBuf);
		}
		ep->thisPtr = 0;
		break;

	case MPR_TYPE_CFUNCTION:
		/*
		 *	Call the function depending on the various handle flags
		 */
		ep->thisPtr = prototype->cFunction.thisPtr;
		if (prototype->flags & MPR_VAR_ALT_HANDLE) {
			rc = ((EjsAltCFunction) prototype->cFunction.fn)
				((EjsHandle) ep->eid, ep->altHandle, actualArgs->used, 
				argValues);
		} else if (prototype->flags & MPR_VAR_SCRIPT_HANDLE) {
			rc = (prototype->cFunction.fn)((EjsHandle) ep->eid, 
				actualArgs->used, argValues);
		} else {
			rc = (prototype->cFunction.fn)(ep->primaryHandle, 
				actualArgs->used, argValues);
		}
		ep->thisPtr = 0;
		break;

	case MPR_TYPE_FUNCTION:

		formalArgs = prototype->function.args;
		argNames = (char**) formalArgs->handles;

#if FUTURE
		if (formalArgs->used != actualArgs->used) {
			ejsError(ep, "Bad number of args. Should be %d", formalArgs->used);
			return -1;
		}
#endif

		/*
		 *	Create the arguments and callee variables
		 */
		arguments = ejsCreateObj("arguments", EJS_SMALL_OBJ_HASH_SIZE);
		callee = ejsCreateObj("callee", EJS_SMALL_OBJ_HASH_SIZE);

		/*
		 *	Overwrite the length property
		 */
		mprCreatePropertyValue(&arguments, "length", 
			mprCreateIntegerVar(actualArgs->used));
		mprCreatePropertyValue(&callee, "length", 
			mprCreateIntegerVar(formalArgs->used));

		/*
 		 *	Define all the agruments to be set to the actual parameters
		 */
		for (i = 0; i < formalArgs->used; i++) {
            if (i >= actualArgs->used) {
                MprVar undefined = mprCreateUndefinedVar();
                mprCreateProperty(ep->local, argNames[i], &undefined);
            } else {
    			mprCreateProperty(ep->local, argNames[i], argValues[i]);
            }
		}
		for (i = 0; i < actualArgs->used; i++) {
			mprItoa(i, buf, sizeof(buf));
			mprCreateProperty(&arguments, buf, argValues[i]);
		}

		mprCreateProperty(&arguments, "callee", &callee);
		mprCreateProperty(ep->local, "arguments", &arguments);

		/*
		 *	Can destroy our variables here as they are now referenced via
		 *	"local"
		 */
		mprDestroyVar(&callee);
		mprDestroyVar(&arguments);

		/*
		 *	Actually run the function
	 	 */
		rc = ejsEvalScript(ep->eid, prototype->function.body, 0, 0);
		break;
	}

	ejsCloseBlock(ep->eid, fid);

	/*
	 *	New statements return the newly created object as the result of the
	 *	command
	 */
	if (flags & EJS_FLAGS_NEW) {
		mprDestroyVar(&ep->result);
		/*
		 *	Don't copy, we want to assign the actual object into result.
		 *	(mprCopyVar would inc the refCount to 2).
		 */
		ep->result = thisObject;
	}
	return rc;
}

/******************************************************************************/
/*
 *	Run a function
 */

int ejsRunFunction(int eid, MprVar *obj, char *functionName, MprArray *args)
{
	EjsProc		proc, *saveProc;
	Ejs			*ep;
	int			rc;

	mprAssert(obj);
	mprAssert(functionName && *functionName);

	if ((ep = ejsPtr(eid)) == NULL) {
		mprAssert(ep);
		return MPR_ERR_NOT_FOUND;
	}
	saveProc = ep->proc;
	ep->proc = &proc;

	memset(&proc, 0, sizeof(EjsProc));
	mprDestroyVar(&ep->result);

	proc.fn = mprGetProperty(obj, functionName, 0);
	if (proc.fn == 0 || proc.fn->type == MPR_TYPE_UNDEFINED) {
		ep->proc = saveProc;
		return MPR_ERR_NOT_FOUND;
	}
	proc.procName = mprStrdup(functionName);
	if (args == 0) {
		proc.args = mprCreateArray();
		rc = evalFunction(ep, obj, 0);
	} else {
		proc.args = args;
		rc = evalFunction(ep, obj, 0);
		proc.args = 0;
	}

	freeProc(&proc);
	ep->proc = saveProc;

	return rc;
}

/******************************************************************************/
/*
 *	Find which object contains the property given the current context.
 *	Only used for top level properties. 
 */

MprVar *ejsFindObj(Ejs *ep, int state, char *property, int flags)
{
	MprVar		*vp;
	MprVar		*obj;

	mprAssert(ep);
	mprAssert(property && *property);

	if (flags & EJS_FLAGS_GLOBAL) {
		obj = ep->global;

	} else if (state == EJS_STATE_DEC || flags & EJS_FLAGS_LOCAL) {
		obj = ep->local;

	} else {
		/* First look local, then look global */
		vp = mprGetProperty(ep->local, property, 0);
		if (vp) {
			obj = ep->local;
		} else if (mprGetProperty(ep->local, property, 0)) {
			obj = ep->local;
		} else {
			obj = ep->global;
		}
	}
	return obj;
}

/******************************************************************************/
/*
 *	Find an object property given a object and a property name. We
 *	intelligently look in the local and global namespaces depending on
 *	our state. If not found in local or global, try base classes for function
 *	names only. Returns the property or NULL.
 */

MprVar *ejsFindProperty(Ejs *ep, int state, MprVar *obj, char *property, 
	int flags)
{
	MprVar	*vp;

	mprAssert(ep);
	if (flags & EJS_FLAGS_EXE) {
		mprAssert(property && *property);
	}

	if (obj != 0) {
#if FUTURE && MB
		op = obj;
		do {
			vp = mprGetProperty(op, property, 0);
			if (vp != 0) {
				if (op != obj && mprVarIsFunction(vp->type)) {
				}
				break;
			}
			op = op->baseObj;
		} while (op);
#endif
		vp = mprGetProperty(obj, property, 0);

	} else {
		if (state == EJS_STATE_DEC) {
			vp = mprGetProperty(ep->local, property, 0);

		} else {
			/* Look local first, then global */
			vp = mprGetProperty(ep->local, property, 0);
			if (vp == NULL) {
				vp = mprGetProperty(ep->global, property, 0);
			}
		}
	}
	return vp;
}

/******************************************************************************/
/*
 *	Update result
 */

static void updateResult(Ejs *ep, int state, int flags, MprVar *vp)
{
	if (flags & EJS_FLAGS_EXE && state != EJS_STATE_DEC) {
		mprDestroyVar(&ep->result);
		if (vp) {
			mprCopyProperty(&ep->result, vp, MPR_SHALLOW_COPY);
		}
	}
}

/******************************************************************************/
/*
 *	Append to the pointer value
 */

static void appendValue(MprVar *dest, MprVar *src)
{
	char	*value, *oldBuf, *buf;
	int		len, oldLen;

	mprAssert(dest);

	mprVarToString(&value, MPR_MAX_STRING, 0, src);

	if (mprVarIsValid(dest)) {
		len = strlen(value);
		oldBuf = dest->string;
		oldLen = strlen(oldBuf);
		buf = (char*) mprRealloc(oldBuf, (len + oldLen + 1) * sizeof(char));
		dest->string = buf;
		strcpy(&buf[oldLen], value);

	} else {
		*dest = mprCreateStringVar(value, 1);
	}
	mprFree(value);
}

/******************************************************************************/
/*
 *	Exit with status
 */

void ejsSetExitStatus(int eid, int status)
{
	Ejs		*ep;

	if ((ep = ejsPtr(eid)) == NULL) {
		mprAssert(ep);
		return;
	}
	ep->exitStatus = status;
	ep->flags |= EJS_FLAGS_EXIT;
}

/******************************************************************************/
/*
 *	Free an argument list
 */

static void freeProc(EjsProc *proc)
{
	MprVar	**argValues;
	int		i;

	if (proc->args) {
		argValues = (MprVar**) proc->args->handles;

		for (i = 0; i < proc->args->max; i++) {
			if (argValues[i]) {
				mprDestroyVar(argValues[i]);
				mprFree(argValues[i]);
				mprRemoveFromArray(proc->args, i);
			}
		}

		mprDestroyArray(proc->args);
	}

	if (proc->procName) {
		mprFree(proc->procName);
		proc->procName = NULL;
	}
}

/******************************************************************************/
/*
 *	This function removes any new lines.  Used for else	cases, etc.
 */

static void removeNewlines(Ejs *ep, int state)
{
	int tid;

	do {
		tid = ejsLexGetToken(ep, state);
	} while (tid == EJS_TOK_NEWLINE);

	ejsLexPutbackToken(ep, tid, ep->token);
}

/******************************************************************************/

#else
void ejsParserDummy() {}

/******************************************************************************/
#endif /* BLD_FEATURE_EJS */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim: sw=4 ts=4 
 */
