/*
 * ejlex.c -- Ejscript(TM) Lexical Analyser
 *
 * Copyright (c) GoAhead Software Inc., 1995-2000. All Rights Reserved.
 *
 * See the file "license.txt" for usage and redistribution license requirements
 *
 * $Id: ejlex.c,v 1.4 2002/10/24 14:44:50 bporter Exp $
 */

/******************************** Description *********************************/

/*
 *	Ejscript lexical analyser. This implementes a lexical analyser for a 
 *	a subset of the JavaScript language.
 */

/********************************** Includes **********************************/

#include	"ejIntrn.h"

#ifdef UEMF
	#include "uemf.h"
#else
	#include "basic/basicInternal.h"
#endif

/********************************** Defines ***********************************/
#define		OCTAL	8
#define		HEX		16
/****************************** Forward Declarations **************************/

static int 		getLexicalToken(ej_t* ep, int state);
static int 		tokenAddChar(ej_t *ep, int c);
static int 		inputGetc(ej_t* ep);
static void		inputPutback(ej_t* ep, int c);
static int		charConvert(ej_t* ep, int base, int maxDig);

/************************************* Code ***********************************/
/*
 *	Setup the lexical analyser
 */

int ejLexOpen(ej_t* ep)
{
	return 0;
}

/******************************************************************************/
/*
 *	Close the lexicial analyser
 */

void ejLexClose(ej_t* ep)
{
}

/******************************************************************************/
/*
 *	Open a new input script
 */

int ejLexOpenScript(ej_t* ep, char_t *script)
{
	ejinput_t	*ip;

	a_assert(ep);
	a_assert(script);

	if ((ep->input = balloc(B_L, sizeof(ejinput_t))) == NULL) {
		return -1;
	}
	ip = ep->input;
	memset(ip, 0, sizeof(*ip));

	a_assert(ip);
	a_assert(ip->putBackToken == NULL);
	a_assert(ip->putBackTokenId == 0);

/*
 *	Create the parse token buffer and script buffer
 */
	if (ringqOpen(&ip->tokbuf, EJ_INC, -1) < 0) {
		return -1;
	}
	if (ringqOpen(&ip->script, EJ_SCRIPT_INC, -1) < 0) {
		return -1;
	}
/*
 *	Put the Ejscript into a ring queue for easy parsing
 */
	ringqPutStr(&ip->script, script);

	ip->lineNumber = 1;
	ip->lineLength = 0;
	ip->lineColumn = 0;
	ip->line = NULL;

	return 0;
}

/******************************************************************************/
/*
 *	Close the input script
 */

void ejLexCloseScript(ej_t* ep)
{
	ejinput_t	*ip;

	a_assert(ep);

	ip = ep->input;
	a_assert(ip);

	if (ip->putBackToken) {
		bfree(B_L, ip->putBackToken);
		ip->putBackToken = NULL;
	}
	ip->putBackTokenId = 0;

	if (ip->line) {
		bfree(B_L, ip->line);
		ip->line = NULL;
	}

	ringqClose(&ip->tokbuf);
	ringqClose(&ip->script);

	bfree(B_L, ip);
}

/******************************************************************************/
/*
 *	Save the input state
 */

void ejLexSaveInputState(ej_t* ep, ejinput_t* state)
{
	ejinput_t	*ip;

	a_assert(ep);

	ip = ep->input;
	a_assert(ip);

	*state = *ip;
	if (ip->putBackToken) {
		state->putBackToken = bstrdup(B_L, ip->putBackToken);
	}
}

/******************************************************************************/
/*
 *	Restore the input state
 */

void ejLexRestoreInputState(ej_t* ep, ejinput_t* state)
{
	ejinput_t	*ip;

	a_assert(ep);

	ip = ep->input;
	a_assert(ip);

	ip->tokbuf = state->tokbuf;
	ip->script = state->script;
	ip->putBackTokenId = state->putBackTokenId;
	if (ip->putBackToken) {
		bfree(B_L, ip->putBackToken);
	}
	if (state->putBackToken) {
		ip->putBackToken = bstrdup(B_L, state->putBackToken);
	}
}

/******************************************************************************/
/*
 *	Free a saved input state
 */

void ejLexFreeInputState(ej_t* ep, ejinput_t* state)
{
	if (state->putBackToken) {
		bfree(B_L, state->putBackToken);
		state->putBackToken = NULL;
	}
}

/******************************************************************************/
/*
 *	Get the next Ejscript token
 */

int ejLexGetToken(ej_t* ep, int state)
{
	ep->tid = getLexicalToken(ep, state);
   /*
    * commented out 04 Apr 02 Bg Porter -- we found a case where very long
    * arguments to write() were being corrupted downstream in the trace call
    * (the ep->token pointer was being overwritten with the trace message.
    * restore this if it's useful for your debugging.
	trace(9, T("ejGetToken: %d, \"%s\"\n"), ep->tid, ep->token);
    */
	return ep->tid;
}

/******************************************************************************/
/*
 *	Get the next Ejscript token
 */

static int getLexicalToken(ej_t* ep, int state)
{
	ringq_t		*inq, *tokq;
	ejinput_t*	ip;
	int			done, tid, c, quote, style;

	a_assert(ep);
	ip = ep->input;
	a_assert(ip);

	inq = &ip->script;
	tokq = &ip->tokbuf;

	ep->tid = -1;
	tid = -1;
	ep->token = T("");

	ringqFlush(tokq);

	if (ip->putBackTokenId > 0) {
		ringqPutStr(tokq, ip->putBackToken);
		tid = ip->putBackTokenId;
		ip->putBackTokenId = 0;
		ep->token = (char_t*) tokq->servp;
		return tid;
	}

	if ((c = inputGetc(ep)) < 0) {
		return TOK_EOF;
	}

	for (done = 0; !done; ) {
		switch (c) {
		case -1:
			return TOK_EOF;

		case ' ':
		case '\t':
		case '\r':
			do {
				if ((c = inputGetc(ep)) < 0)
					break;
			} while (c == ' ' || c == '\t' || c == '\r');
			break;

		case '\n':
			return TOK_NEWLINE;

		case '(':
			tokenAddChar(ep, c);
			return TOK_LPAREN;

		case ')':
			tokenAddChar(ep, c);
			return TOK_RPAREN;

		case '{':
			tokenAddChar(ep, c);
			return TOK_LBRACE;

		case '}':
			tokenAddChar(ep, c);
			return TOK_RBRACE;

		case '+':
			if ((c = inputGetc(ep)) < 0) {
				ejError(ep, T("Syntax Error"));
				return TOK_ERR;
			}
			if (c != '+' ) {
				inputPutback(ep, c);
				tokenAddChar(ep, EXPR_PLUS);
				return TOK_EXPR;
			}
			tokenAddChar(ep, EXPR_INC);
			return TOK_INC_DEC;

		case '-':
			if ((c = inputGetc(ep)) < 0) {
				ejError(ep, T("Syntax Error"));
				return TOK_ERR;
			}
			if (c != '-' ) {
				inputPutback(ep, c);
				tokenAddChar(ep, EXPR_MINUS);
				return TOK_EXPR;
			}
			tokenAddChar(ep, EXPR_DEC);
			return TOK_INC_DEC;

		case '*':
			tokenAddChar(ep, EXPR_MUL);
			return TOK_EXPR;

		case '%':
			tokenAddChar(ep, EXPR_MOD);
			return TOK_EXPR;

		case '/':
/*
 *			Handle the division operator and comments
 */
			if ((c = inputGetc(ep)) < 0) {
				ejError(ep, T("Syntax Error"));
				return TOK_ERR;
			}
			if (c != '*' && c != '/') {
				inputPutback(ep, c);
				tokenAddChar(ep, EXPR_DIV);
				return TOK_EXPR;
			}
			style = c;
/*
 *			Eat comments. Both C and C++ comment styles are supported.
 */
			while (1) {
				if ((c = inputGetc(ep)) < 0) {
					ejError(ep, T("Syntax Error"));
					return TOK_ERR;
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
 *			Continue looking for a token, so get the next character
 */
			if ((c = inputGetc(ep)) < 0) {
				return TOK_EOF;
			}
			break;

		case '<':									/* < and <= */
			if ((c = inputGetc(ep)) < 0) {
				ejError(ep, T("Syntax Error"));
				return TOK_ERR;
			}
			if (c == '<') {
				tokenAddChar(ep, EXPR_LSHIFT);
				return TOK_EXPR;
			} else if (c == '=') {
				tokenAddChar(ep, EXPR_LESSEQ);
				return TOK_EXPR;
			}
			tokenAddChar(ep, EXPR_LESS);
			inputPutback(ep, c);
			return TOK_EXPR;

		case '>':									/* > and >= */
			if ((c = inputGetc(ep)) < 0) {
				ejError(ep, T("Syntax Error"));
				return TOK_ERR;
			}
			if (c == '>') {
				tokenAddChar(ep, EXPR_RSHIFT);
				return TOK_EXPR;
			} else if (c == '=') {
				tokenAddChar(ep, EXPR_GREATEREQ);
				return TOK_EXPR;
			}
			tokenAddChar(ep, EXPR_GREATER);
			inputPutback(ep, c);
			return TOK_EXPR;

		case '=':									/* "==" */
			if ((c = inputGetc(ep)) < 0) {
				ejError(ep, T("Syntax Error"));
				return TOK_ERR;
			}
			if (c == '=') {
				tokenAddChar(ep, EXPR_EQ);
				return TOK_EXPR;
			}
			inputPutback(ep, c);
			return TOK_ASSIGNMENT;

		case '!':									/* "!=" or "!"*/
			if ((c = inputGetc(ep)) < 0) {
				ejError(ep, T("Syntax Error"));
				return TOK_ERR;
			}
			if (c == '=') {
				tokenAddChar(ep, EXPR_NOTEQ);
				return TOK_EXPR;
			}
			inputPutback(ep, c);
			tokenAddChar(ep, EXPR_BOOL_COMP);
			return TOK_EXPR;

		case ';':
			tokenAddChar(ep, c);
			return TOK_SEMI;

		case ',':
			tokenAddChar(ep, c);
			return TOK_COMMA;

		case '|':									/* "||" */
			if ((c = inputGetc(ep)) < 0 || c != '|') {
				ejError(ep, T("Syntax Error"));
				return TOK_ERR;
			}
			tokenAddChar(ep, COND_OR);
			return TOK_LOGICAL;

		case '&':									/* "&&" */
			if ((c = inputGetc(ep)) < 0 || c != '&') {
				ejError(ep, T("Syntax Error"));
				return TOK_ERR;
			}
			tokenAddChar(ep, COND_AND);
			return TOK_LOGICAL;

		case '\"':									/* String quote */
		case '\'':
			quote = c;
			if ((c = inputGetc(ep)) < 0) {
				ejError(ep, T("Syntax Error"));
				return TOK_ERR;
			}

			while (c != quote) {
/*
 *				check for escape sequence characters
 */
				if (c == '\\') {
					c = inputGetc(ep);

					if (gisdigit(c)) {
/*
 *						octal support, \101 maps to 65 = 'A'. put first char
 *						back so converter will work properly.
 */
						inputPutback(ep, c);
						c = charConvert(ep, OCTAL, 3);

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
 *							hex support, \x41 maps to 65 = 'A'
 */
							c = charConvert(ep, HEX, 2);
							break;
						case 'u':
/*
 *							unicode support, \x0401 maps to 65 = 'A'
 */
							c = charConvert(ep, HEX, 2);
							c = c*16 + charConvert(ep, HEX, 2);

							break;
						case '\'':
						case '\"':
						case '\\':
							break;
						default:
							ejError(ep, T("Invalid Escape Sequence"));
							return TOK_ERR;
						}
					}
					if (tokenAddChar(ep, c) < 0) {
						return TOK_ERR;
					}
				} else {
					if (tokenAddChar(ep, c) < 0) {
						return TOK_ERR;
					}
				}
				if ((c = inputGetc(ep)) < 0) {
					ejError(ep, T("Unmatched Quote"));
					return TOK_ERR;
				}
			}
			return TOK_LITERAL;

		case '0': case '1': case '2': case '3': case '4': 
		case '5': case '6': case '7': case '8': case '9':
			do {
				if (tokenAddChar(ep, c) < 0) {
					return TOK_ERR;
				}
				if ((c = inputGetc(ep)) < 0)
					break;
			} while (gisdigit(c));
			inputPutback(ep, c);
			return TOK_LITERAL;

		default:
/*
 *			Identifiers or a function names
 */
			while (1) {
				if (c == '\\') {
/*
 *					just ignore any \ characters.
 */
				} else if (tokenAddChar(ep, c) < 0) {
						break;
				}
				if ((c = inputGetc(ep)) < 0) {
					break;
				}
				if (!gisalnum(c) && c != '$' && c != '_' &&
					c != '\\') {
					break;
				}
			}
			if (! gisalpha(*tokq->servp) && *tokq->servp != '$' && 
					*tokq->servp != '_') {
				ejError(ep, T("Invalid identifier %s"), tokq->servp);
				return TOK_ERR;
			}
/*
 *			Check for reserved words (only "if", "else", "var", "for"
 *			and "return" at the moment)
 */
			if (state == STATE_STMT) {
				if (gstrcmp(ep->token, T("if")) == 0) {
					return TOK_IF;
				} else if (gstrcmp(ep->token, T("else")) == 0) {
					return TOK_ELSE;
				} else if (gstrcmp(ep->token, T("var")) == 0) {
					return TOK_VAR;
				} else if (gstrcmp(ep->token, T("for")) == 0) {
					return TOK_FOR;
				} else if (gstrcmp(ep->token, T("return")) == 0) {
					if ((c == ';') || (c == '(')) {
						inputPutback(ep, c);
					}
					return TOK_RETURN;
				}
			}

/* 
 * 			Skip white space after token to find out whether this is
 * 			a function or not.
 */ 
			while (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
				if ((c = inputGetc(ep)) < 0)
					break;
			}

			tid = (c == '(') ? TOK_FUNCTION : TOK_ID;
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
 *	Putback the last token read
 */

void ejLexPutbackToken(ej_t* ep, int tid, char_t *string)
{
	ejinput_t*	ip;

	a_assert(ep);
	ip = ep->input;
	a_assert(ip);

	if (ip->putBackToken) {
		bfree(B_L, ip->putBackToken);
	}
	ip->putBackTokenId = tid;
	ip->putBackToken = bstrdup(B_L, string);
}

/******************************************************************************/
/*
 *	Add a character to the token ringq buffer
 */

static int tokenAddChar(ej_t *ep, int c)
{
	ejinput_t*	ip;

	a_assert(ep);
	ip = ep->input;
	a_assert(ip);

	if (ringqPutc(&ip->tokbuf, (char_t) c) < 0) {
		ejError(ep, T("Token too big"));
		return -1;
	}
	* ((char_t*) ip->tokbuf.endp) = '\0';
	ep->token = (char_t*) ip->tokbuf.servp;

	return 0;
}

/******************************************************************************/
/*
 *	Get another input character
 */

static int inputGetc(ej_t* ep)
{
	ejinput_t	*ip;
	int			c, len;

	a_assert(ep);
	ip = ep->input;

	if ((len = ringqLen(&ip->script)) == 0) {
		return -1;
	}

	c = ringqGetc(&ip->script);

	if (c == '\n') {
		ip->lineNumber++;
		ip->lineColumn = 0;
	} else {
		if ((ip->lineColumn + 2) >= ip->lineLength) {
			ip->lineLength += EJ_INC;
			ip->line = brealloc(B_L, ip->line, ip->lineLength * sizeof(char_t));
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

static void inputPutback(ej_t* ep, int c)
{
	ejinput_t	*ip;

	a_assert(ep);

	ip = ep->input;
	ringqInsertc(&ip->script, (char_t) c);
	ip->lineColumn--;
	ip->line[ip->lineColumn] = '\0';
}

/******************************************************************************/
/*
 *	Convert a hex or octal character back to binary, return original char if 
 *	not a hex digit
 */

static int charConvert(ej_t* ep, int base, int maxDig)
{
	int		i, c, lval, convChar;

	lval = 0;
	for (i = 0; i < maxDig; i++) {
		if ((c = inputGetc(ep)) < 0) {
			break;
		}
/*
 *		Initialize to out of range value
 */
		convChar = base;
		if (gisdigit(c)) {
			convChar = c - '0';
		} else if (c >= 'a' && c <= 'f') {
			convChar = c - 'a' + 10;
		} else if (c >= 'A' && c <= 'F') {
			convChar = c - 'A' + 10;
		}
/*
 *		if unexpected character then return it to buffer.
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

