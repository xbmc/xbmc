/*
 *	@file 	ejsLex.c
 *	@brief 	EJS Lexical Analyser
 *	@overview EJS lexical analyser. This implementes a lexical analyser 
 *		for a subset of the JavaScript language.
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

static int 		getLexicalToken(Ejs *ep, int state);
static int 		tokenAddChar(Ejs *ep, int c);
static int 		inputGetc(Ejs *ep);
static void		inputPutback(Ejs *ep, int c);
static int		charConvert(Ejs *ep, int base, int maxDig);

/************************************* Code ***********************************/
/*
 *	Open a new input script
 */

int ejsLexOpenScript(Ejs *ep, char *script)
{
	EjsInput	*ip;

	mprAssert(ep);
	mprAssert(script);

	if ((ep->input = (EjsInput*) mprMalloc(sizeof(EjsInput))) == NULL) {
		return -1;
	}
	ip = ep->input;
	memset(ip, 0, sizeof(*ip));

/*
 *	Create the parse token buffer and script buffer
 */
	ip->tokbuf = (char*) mprMalloc(EJS_PARSE_INCR);
	ip->tokSize = EJS_PARSE_INCR; 
	ip->tokServp = ip->tokbuf;
	ip->tokEndp = ip->tokbuf;

	ip->script = mprStrdup(script);
	ip->scriptSize = strlen(script);
	ip->scriptServp = ip->script;

	ip->lineNumber = 1;
	ip->lineLength = 0;
	ip->lineColumn = 0;
	ip->line = NULL;

	ip->putBackIndex = -1;

	return 0;
}

/******************************************************************************/
/*
 *	Close the input script
 */

void ejsLexCloseScript(Ejs *ep)
{
	EjsInput	*ip;
	int			i;

	mprAssert(ep);

	ip = ep->input;
	mprAssert(ip);

	for (i = 0; i < EJS_TOKEN_STACK; i++) {
		mprFree(ip->putBack[i].token);
		ip->putBack[i].token = 0;
	}

	mprFree(ip->line);
	mprFree(ip->tokbuf);
	mprFree(ip->script);

	mprFree(ip);
}

/******************************************************************************/
/*
 *	Initialize an input state structure
 */

int ejsInitInputState(EjsInput *ip)
{
	mprAssert(ip);

	memset(ip, 0, sizeof(*ip));
	ip->putBackIndex = -1;

	return 0;
}
/******************************************************************************/
/*
 *	Save the input state
 */

void ejsLexSaveInputState(Ejs *ep, EjsInput *state)
{
	EjsInput	*ip;
	int			i;

	mprAssert(ep);

	ip = ep->input;
	mprAssert(ip);

	*state = *ip;

	for (i = 0; i < ip->putBackIndex; i++) {
		state->putBack[i].token = mprStrdup(ip->putBack[i].token);
		state->putBack[i].id = ip->putBack[i].id;
	}
	for (; i < EJS_TOKEN_STACK; i++) {
		state->putBack[i].token = 0;
	}

	state->line = (char*) mprMalloc(ip->lineLength);
	mprStrcpy(state->line, ip->lineLength, ip->line);

	state->lineColumn = ip->lineColumn;
	state->lineNumber = ip->lineNumber;
	state->lineLength = ip->lineLength;
}

/******************************************************************************/
/*
 *	Restore the input state
 */

void ejsLexRestoreInputState(Ejs *ep, EjsInput *state)
{
	EjsInput	*ip;
	int			i;

	mprAssert(ep);
	mprAssert(state);

	ip = ep->input;
	mprAssert(ip);

	ip->tokbuf = state->tokbuf;
	ip->tokServp = state->tokServp;
	ip->tokEndp = state->tokEndp;
	ip->tokSize = state->tokSize;

	ip->script = state->script;
	ip->scriptServp = state->scriptServp;
	ip->scriptSize = state->scriptSize;

	ip->putBackIndex = state->putBackIndex;
	for (i = 0; i < ip->putBackIndex; i++) {
		mprFree(ip->putBack[i].token);
		ip->putBack[i].id = state->putBack[i].id;
		ip->putBack[i].token = mprStrdup(state->putBack[i].token);
	}

	mprFree(ip->line);
	ip->line = (char*) mprMalloc(state->lineLength);
	mprStrcpy(ip->line, state->lineLength, state->line);

	ip->lineColumn = state->lineColumn;
	ip->lineNumber = state->lineNumber;
	ip->lineLength = state->lineLength;
}

/******************************************************************************/
/*
 *	Free a saved input state
 */

void ejsLexFreeInputState(Ejs *ep, EjsInput *state)
{
	int			i;

	mprAssert(ep);
	mprAssert(state);

	for (i = 0; i < EJS_TOKEN_STACK; i++) {
		mprFree(state->putBack[i].token);
	}
	state->putBackIndex = -1;
	mprFree(state->line);
	state->lineLength = 0;
	state->lineColumn = 0;
}

/******************************************************************************/
/*
 *	Get the next EJS token
 */

int ejsLexGetToken(Ejs *ep, int state)
{
	mprAssert(ep);

	ep->tid = getLexicalToken(ep, state);
	return ep->tid;
}

/******************************************************************************/

/*
 *	Check for reserved words "if", "else", "var", "for", "foreach",
 *	"delete", "function", and "return". "new", "in" and "function" 
 *	done below. "true", "false", "null", "undefined" are handled
 *	as global objects.
 *
 *	Other reserved words not supported:
 *		"break", "case", "catch", "continue", "default", "do", 
 *		"finally", "instanceof", "switch", "this", "throw", "try",
 *		"typeof", "while", "with"
 *
 *	ECMA extensions reserved words (not supported):
 *		"abstract", "boolean", "byte", "char", "class", "const",
 *		"debugger", "double", "enum", "export", "extends",
 *		"final", "float", "goto", "implements", "import", "int",
 *		"interface", "long", "native", "package", "private",
 *		"protected", "public", "short", "static", "super",
 *		"synchronized", "throws", "transient", "volatile"
 */

static int checkReservedWord(Ejs *ep, int state, int c, int tid)
{
	if (state == EJS_STATE_STMT) {
		if (strcmp(ep->token, "if") == 0) {
			inputPutback(ep, c);
			return EJS_TOK_IF;
		} else if (strcmp(ep->token, "else") == 0) {
			inputPutback(ep, c);
			return EJS_TOK_ELSE;
		} else if (strcmp(ep->token, "var") == 0) {
			inputPutback(ep, c);
			return EJS_TOK_VAR;
		} else if (strcmp(ep->token, "for") == 0) {
			inputPutback(ep, c);
			return EJS_TOK_FOR;
		} else if (strcmp(ep->token, "delete") == 0) {
			inputPutback(ep, c);
			return EJS_TOK_DELETE;
		} else if (strcmp(ep->token, "function") == 0) {
			inputPutback(ep, c);
			return EJS_TOK_FUNCTION;
		} else if (strcmp(ep->token, "return") == 0) {
			if ((c == ';') || (c == '(')) {
				inputPutback(ep, c);
			}
			return EJS_TOK_RETURN;
		}
	} else if (state == EJS_STATE_EXPR) {
		if (strcmp(ep->token, "new") == 0) {
			inputPutback(ep, c);
			return EJS_TOK_NEW;
		} else if (strcmp(ep->token, "in") == 0) {
			inputPutback(ep, c);
			return EJS_TOK_IN;
		} else if (strcmp(ep->token, "function") == 0) {
			inputPutback(ep, c);
			return EJS_TOK_FUNCTION;
		}
	}
	return tid;
}

/******************************************************************************/
/*
 *	Get the next EJS token
 */

static int getLexicalToken(Ejs *ep, int state)
{
	MprType		type;
	EjsInput	*ip;
	int			done, tid, c, quote, style, index;

	mprAssert(ep);
	ip = ep->input;
	mprAssert(ip);

	ep->tid = -1;
	tid = -1;
	type = BLD_FEATURE_NUM_TYPE_ID;

	/*
 	 *	Use a putback tokens first. Don't free strings as caller needs access.
	 */
	if (ip->putBackIndex >= 0) {
		index = ip->putBackIndex;
		tid = ip->putBack[index].id;
		ep->token = (char*) ip->putBack[index].token;
		tid = checkReservedWord(ep, state, 0, tid);
		ip->putBackIndex--;
		return tid;
	}
	ep->token = ip->tokServp = ip->tokEndp = ip->tokbuf;
	*ip->tokServp = '\0';

	if ((c = inputGetc(ep)) < 0) {
		return EJS_TOK_EOF;
	}

	/*
 	 *	Main lexical analyser
	 */
	for (done = 0; !done; ) {
		switch (c) {
		case -1:
			return EJS_TOK_EOF;

		case ' ':
		case '\t':
		case '\r':
			do {
				if ((c = inputGetc(ep)) < 0)
					break;
			} while (c == ' ' || c == '\t' || c == '\r');
			break;

		case '\n':
			return EJS_TOK_NEWLINE;

		case '(':
			tokenAddChar(ep, c);
			return EJS_TOK_LPAREN;

		case ')':
			tokenAddChar(ep, c);
			return EJS_TOK_RPAREN;

		case '[':
			tokenAddChar(ep, c);
			return EJS_TOK_LBRACKET;

		case ']':
			tokenAddChar(ep, c);
			return EJS_TOK_RBRACKET;

		case '.':
			tokenAddChar(ep, c);
			return EJS_TOK_PERIOD;

		case '{':
			tokenAddChar(ep, c);
			return EJS_TOK_LBRACE;

		case '}':
			tokenAddChar(ep, c);
			return EJS_TOK_RBRACE;

		case '+':
			if ((c = inputGetc(ep)) < 0) {
				ejsError(ep, "Syntax Error");
				return EJS_TOK_ERR;
			}
			if (c != '+' ) {
				inputPutback(ep, c);
				tokenAddChar(ep, EJS_EXPR_PLUS);
				return EJS_TOK_EXPR;
			}
			tokenAddChar(ep, EJS_EXPR_INC);
			return EJS_TOK_INC_DEC;

		case '-':
			if ((c = inputGetc(ep)) < 0) {
				ejsError(ep, "Syntax Error");
				return EJS_TOK_ERR;
			}
			if (c != '-' ) {
				inputPutback(ep, c);
				tokenAddChar(ep, EJS_EXPR_MINUS);
				return EJS_TOK_EXPR;
			}
			tokenAddChar(ep, EJS_EXPR_DEC);
			return EJS_TOK_INC_DEC;

		case '*':
			tokenAddChar(ep, EJS_EXPR_MUL);
			return EJS_TOK_EXPR;

		case '%':
			tokenAddChar(ep, EJS_EXPR_MOD);
			return EJS_TOK_EXPR;

		case '/':
			/*
			 *	Handle the division operator and comments
			 */
			if ((c = inputGetc(ep)) < 0) {
				ejsError(ep, "Syntax Error");
				return EJS_TOK_ERR;
			}
			if (c != '*' && c != '/') {
				inputPutback(ep, c);
				tokenAddChar(ep, EJS_EXPR_DIV);
				return EJS_TOK_EXPR;
			}
			style = c;
			/*
			 *	Eat comments. Both C and C++ comment styles are supported.
			 */
			while (1) {
				if ((c = inputGetc(ep)) < 0) {
					ejsError(ep, "Syntax Error");
					return EJS_TOK_ERR;
				}
				if (c == '\n' && style == '/') {
					break;
				} else if (c == '*') {
					c = inputGetc(ep);
					if (style == '/') {
						if (c == '\n') {
							break;
						}
					} else {
						if (c == '/') {
							break;
						}
					}
				}
			}
			/*
			 *	Continue looking for a token, so get the next character
			 */
			if ((c = inputGetc(ep)) < 0) {
				return EJS_TOK_EOF;
			}
			break;

		case '<':									/* < and <= */
			if ((c = inputGetc(ep)) < 0) {
				ejsError(ep, "Syntax Error");
				return EJS_TOK_ERR;
			}
			if (c == '<') {
				tokenAddChar(ep, EJS_EXPR_LSHIFT);
				return EJS_TOK_EXPR;
			} else if (c == '=') {
				tokenAddChar(ep, EJS_EXPR_LESSEQ);
				return EJS_TOK_EXPR;
			}
			tokenAddChar(ep, EJS_EXPR_LESS);
			inputPutback(ep, c);
			return EJS_TOK_EXPR;

		case '>':									/* > and >= */
			if ((c = inputGetc(ep)) < 0) {
				ejsError(ep, "Syntax Error");
				return EJS_TOK_ERR;
			}
			if (c == '>') {
				tokenAddChar(ep, EJS_EXPR_RSHIFT);
				return EJS_TOK_EXPR;
			} else if (c == '=') {
				tokenAddChar(ep, EJS_EXPR_GREATEREQ);
				return EJS_TOK_EXPR;
			}
			tokenAddChar(ep, EJS_EXPR_GREATER);
			inputPutback(ep, c);
			return EJS_TOK_EXPR;

		case '=':									/* "==" */
			if ((c = inputGetc(ep)) < 0) {
				ejsError(ep, "Syntax Error");
				return EJS_TOK_ERR;
			}
			if (c == '=') {
				tokenAddChar(ep, EJS_EXPR_EQ);
				return EJS_TOK_EXPR;
			}
			inputPutback(ep, c);
			return EJS_TOK_ASSIGNMENT;

		case '!':									/* "!=" or "!"*/
			if ((c = inputGetc(ep)) < 0) {
				ejsError(ep, "Syntax Error");
				return EJS_TOK_ERR;
			}
			if (c == '=') {
				tokenAddChar(ep, EJS_EXPR_NOTEQ);
				return EJS_TOK_EXPR;
			}
			inputPutback(ep, c);
			tokenAddChar(ep, EJS_EXPR_BOOL_COMP);
			return EJS_TOK_EXPR;

		case ';':
			tokenAddChar(ep, c);
			return EJS_TOK_SEMI;

		case ',':
			tokenAddChar(ep, c);
			return EJS_TOK_COMMA;

		case '|':									/* "||" */
			if ((c = inputGetc(ep)) < 0 || c != '|') {
				ejsError(ep, "Syntax Error");
				return EJS_TOK_ERR;
			}
			tokenAddChar(ep, EJS_COND_OR);
			return EJS_TOK_LOGICAL;

		case '&':									/* "&&" */
			if ((c = inputGetc(ep)) < 0 || c != '&') {
				ejsError(ep, "Syntax Error");
				return EJS_TOK_ERR;
			}
			tokenAddChar(ep, EJS_COND_AND);
			return EJS_TOK_LOGICAL;

		case '\"':									/* String quote */
		case '\'':
			quote = c;
			if ((c = inputGetc(ep)) < 0) {
				ejsError(ep, "Syntax Error");
				return EJS_TOK_ERR;
			}

			while (c != quote) {
				/*
				 *	Check for escape sequence characters
				 */
				if (c == '\\') {
					c = inputGetc(ep);

					if (isdigit(c)) {
						/*
						 *	Octal support, \101 maps to 65 = 'A'. Put first 
						 *	char back so converter will work properly.
						 */
						inputPutback(ep, c);
						c = charConvert(ep, 8, 3);

					} else {
						switch (c) {
						case 'n':
							c = '\n'; break;
						case 'b':
							c = '\b'; break;
						case 'f':
							c = '\f'; break;
						case 'r':
							c = '\r'; break;
						case 't':
							c = '\t'; break;
						case 'x':
							/*
							 *	Hex support, \x41 maps to 65 = 'A'
							 */
							c = charConvert(ep, 16, 2);
							break;
						case 'u':
							/*
							 *	Unicode support, \x0401 maps to 65 = 'A'
							 */
							c = charConvert(ep, 16, 2);
							c = c*16 + charConvert(ep, 16, 2);

							break;
						case '\'':
						case '\"':
						case '\\':
							break;
						default:
							ejsError(ep, "Invalid Escape Sequence");
							return EJS_TOK_ERR;
						}
					}
					if (tokenAddChar(ep, c) < 0) {
						return EJS_TOK_ERR;
					}
				} else {
					if (tokenAddChar(ep, c) < 0) {
						return EJS_TOK_ERR;
					}
				}
				if ((c = inputGetc(ep)) < 0) {
					ejsError(ep, "Unmatched Quote");
					return EJS_TOK_ERR;
				}
			}
			return EJS_TOK_LITERAL;

		case '0': 
			if (tokenAddChar(ep, c) < 0) {
				return EJS_TOK_ERR;
			}
			if ((c = inputGetc(ep)) < 0) {
				break;
			}
			if (tolower(c) == 'x') {
				if (tokenAddChar(ep, c) < 0) {
					return EJS_TOK_ERR;
				}
				if ((c = inputGetc(ep)) < 0) {
					break;
				}
			}
			if (! isdigit(c)) {
#if BLD_FEATURE_FLOATING_POINT
				if (c == '.' || tolower(c) == 'e' || c == '+' || c == '-') {
					/* Fall through */
					type = MPR_TYPE_FLOAT;
				} else
#endif
				{
					mprDestroyVar(&ep->tokenNumber);
					ep->tokenNumber = mprParseVar(ep->token, type);
					inputPutback(ep, c);
					return EJS_TOK_NUMBER;
				}
			}
			/* Fall through to get more digits */

		case '1': case '2': case '3': case '4': 
		case '5': case '6': case '7': case '8': case '9':
			do {
				if (tokenAddChar(ep, c) < 0) {
					return EJS_TOK_ERR;
				}
				if ((c = inputGetc(ep)) < 0) {
					break;
				}
#if BLD_FEATURE_FLOATING_POINT
				if (c == '.' || tolower(c) == 'e' || c == '+' || c == '-') {
					type = MPR_TYPE_FLOAT;
				}
			} while (isdigit(c) || c == '.' || tolower(c) == 'e' ||
				c == '+' || c == '-');
#else
			} while (isdigit(c));
#endif

			mprDestroyVar(&ep->tokenNumber);
			ep->tokenNumber = mprParseVar(ep->token, type);
			inputPutback(ep, c);
			return EJS_TOK_NUMBER;

		default:
			/*
			 *	Identifiers or a function names
			 */
			while (1) {
				if (c == '\\') {
					if ((c = inputGetc(ep)) < 0) {
						break;
					}
					if (c == '\n' || c == '\r') {
						break;
					}
				} else if (tokenAddChar(ep, c) < 0) {
						break;
				}
				if ((c = inputGetc(ep)) < 0) {
					break;
				}
				if (!isalnum(c) && c != '$' && c != '_' && c != '\\') {
					break;
				}
			}
			if (*ep->token == '\0') {
				c = inputGetc(ep);
				break;
			}
			if (! isalpha((int) *ep->token) && *ep->token != '$' && 
					*ep->token != '_') {
				ejsError(ep, "Invalid identifier %s", ep->token);
				return EJS_TOK_ERR;
			}

			tid = checkReservedWord(ep, state, c, EJS_TOK_ID);
			if (tid != EJS_TOK_ID) {
				return tid;
			}

			/* 
			 *	Skip white space after token to find out whether this is
			 * 	a function or not.
			 */ 
			while (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
				if ((c = inputGetc(ep)) < 0)
					break;
			}

			tid = EJS_TOK_ID;
			done++;
		}
	}

	/*
	 *	Putback the last extra character for next time
	 */
	inputPutback(ep, c);
	return tid;
}

/******************************************************************************/
/*
 *	Convert a hex or octal character back to binary, return original char if 
 *	not a hex digit
 */

static int charConvert(Ejs *ep, int base, int maxDig)
{
	int		i, c, lval, convChar;

	lval = 0;
	for (i = 0; i < maxDig; i++) {
		if ((c = inputGetc(ep)) < 0) {
			break;
		}
		/*
		 *	Initialize to out of range value
		 */
		convChar = base;
		if (isdigit(c)) {
			convChar = c - '0';
		} else if (c >= 'a' && c <= 'f') {
			convChar = c - 'a' + 10;
		} else if (c >= 'A' && c <= 'F') {
			convChar = c - 'A' + 10;
		}
		/*
		 *	If unexpected character then return it to buffer.
		 */
		if (convChar >= base) {
			inputPutback(ep, c);
			break;
		}
		lval = (lval * base) + convChar;
	}
	return lval;
}

/******************************************************************************/
/*
 *	Putback the last token read. Accept at most one push back token.
 */

void ejsLexPutbackToken(Ejs *ep, int tid, char *string)
{
	EjsInput	*ip;
	int			index;

	mprAssert(ep);
	ip = ep->input;
	mprAssert(ip);

	ip->putBackIndex += 1;
	index = ip->putBackIndex;
	ip->putBack[index].id = tid;

	if (ip->putBack[index].token) {
		if (ip->putBack[index].token == string) {
			return;
		}
		mprFree(ip->putBack[index].token);
	}
	ip->putBack[index].token = mprStrdup(string);
}

/******************************************************************************/
/*
 *	Add a character to the token buffer
 */

static int tokenAddChar(Ejs *ep, int c)
{
	EjsInput	*ip;
	uchar		*oldbuf;

	mprAssert(ep);
	ip = ep->input;
	mprAssert(ip);

	if (ip->tokEndp >= &ip->tokbuf[ip->tokSize - 1]) {
		ip->tokSize += EJS_PARSE_INCR;
		oldbuf = (uchar*) ip->tokbuf;
		ip->tokbuf = (char*) mprRealloc(ip->tokbuf, ip->tokSize);
		if (ip->tokbuf == 0) {
			ejsError(ep, "Token too big");
			return -1;
		}
		ip->tokEndp += (int) ((uchar*) ip->tokbuf - oldbuf);
		ip->tokServp += (int) ((uchar*) ip->tokbuf - oldbuf);
		ep->token += (int) ((uchar*) ip->tokbuf - oldbuf);
	}
	*ip->tokEndp++ = c;
	*ip->tokEndp = '\0';

	return 0;
}

/******************************************************************************/
/*
 *	Get another input character
 */

static int inputGetc(Ejs *ep)
{
	EjsInput	*ip;
	int			c;

	mprAssert(ep);
	ip = ep->input;

	if (ip->scriptSize <= 0) {
		return -1;
	}

	c = (uchar) (*ip->scriptServp++);
	ip->scriptSize--;

	/*
	 *	For debugging, accumulate the line number and the currenly parsed line
	 */
	if (c == '\n') {
#if BLD_DEBUG && 0
		if (ip->lineColumn > 0) {
			printf("PARSED: %s\n", ip->line);
		}
#endif
		ip->lineNumber++;
		ip->lineColumn = 0;
	} else {
		if ((ip->lineColumn + 2) >= ip->lineLength) {
			ip->lineLength += 80;
			ip->line = (char*) mprRealloc(ip->line, ip->lineLength * sizeof(char));
		}
		ip->line[ip->lineColumn++] = c;
		ip->line[ip->lineColumn] = '\0';
	}
	return c;
}

/******************************************************************************/
/*
 *	Putback a character onto the input queue
 */

static void inputPutback(Ejs *ep, int c)
{
	EjsInput	*ip;

	mprAssert(ep);

	if (c != 0) {
		ip = ep->input;
		*--ip->scriptServp = c;
		ip->scriptSize++;
		ip->lineColumn--;
		if (ip->lineColumn < 0) {
			ip->lineColumn = 0;
		}
		mprAssert(ip->line);
		mprAssert(ip->lineColumn >= 0);
		mprAssert(ip->lineColumn < ip->lineLength);
		ip->line[ip->lineColumn] = '\0';
	}
}

/******************************************************************************/

#else
void ejsLexDummy() {}

/******************************************************************************/
#endif /* BLD_FEATURE_EJS */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim: sw=4 ts=4 
 */
