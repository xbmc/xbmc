/*
 * uemf.c -- GoAhead Micro Embedded Management Framework
 *
 * Copyright (c) GoAhead Software Inc., 1995-2000. All Rights Reserved.
 *
 * See the file "license.txt" for usage and redistribution license requirements
 *
 * $Id: uemf.c,v 1.4 2002/10/24 14:44:50 bporter Exp $
 */

/********************************** Description *******************************/

/*
 *	This module provides compatibility with the full GoAhead EMF.
 *	It is a collection of routines which permits the GoAhead WebServer to
 *	run stand-alone and to also load as a solution pack under the GoAhead EMF.
 */

/*********************************** Includes *********************************/

#include	"uemf.h"

/********************************** Local Data ********************************/

int emfInst;							/* Application instance handle */

/****************************** Forward Declarations **************************/

extern void defaultErrorHandler(int etype, char_t *buf);
static void (*errorHandler)(int etype, char_t *msg) = defaultErrorHandler;

extern void	defaultTraceHandler(int level, char_t *buf);
static void (*traceHandler)(int level, char_t *buf) = defaultTraceHandler;

/************************************* Code ***********************************/
/*
 *	Error message that doesn't need user attention. Customize this code
 *	to direct error messages to wherever the developer wishes
 */

void error(E_ARGS_DEC, int etype, char_t *fmt, ...)
{
	va_list 	args;
	char_t		*fmtBuf, *buf;

	va_start(args, fmt);
	fmtValloc(&fmtBuf, E_MAX_ERROR, fmt, args);

	if (etype == E_LOG) {
		fmtAlloc(&buf, E_MAX_ERROR, T("%s\n"), fmtBuf);
/*#ifdef DEV*/
	} else if (etype == E_ASSERT) {
		fmtAlloc(&buf, E_MAX_ERROR, 
			T("Assertion %s, failed at %s %d\n"), fmtBuf, E_ARGS); 
/*#endif*/
	} else if (etype == E_USER) {
		fmtAlloc(&buf, E_MAX_ERROR, T("%s\n"), fmtBuf);
	}
   /*
    * bugfix -- if etype is not E_LOG, E_ASSERT, or E_USER, the call to
    * bfreeSafe(B_L, buf) below will fail, because 'buf' is randomly
    * initialized. To be nice, we format a message saying that this is an
    * unknown message type, and in doing so give buf a valid value. Thanks 
    * to Simon Byholm.
    */
   else {
      fmtAlloc(&buf, E_MAX_ERROR, T("Unknown error"));
   }
	va_end(args);

	bfree(B_L, fmtBuf);

	if (errorHandler) {
		errorHandler(etype, buf);
	}

	bfreeSafe(B_L, buf);
}

/******************************************************************************/
/*
 *	Replace the default error handler. Return pointer to old error handler.
 */

void (*errorSetHandler(void (*function)(int etype, char_t *msg))) \
	(int etype, char_t *msg)
{
	void (*oldHandler)(int etype, char_t *buf);

	oldHandler = errorHandler;
	errorHandler = function;
	return oldHandler;
}

/******************************************************************************/
/*
 *	Trace log. Customize this function to log trace output
 */

void trace(int level, char_t *fmt, ...)
{
	va_list 	args;
	char_t		*buf;

	va_start(args, fmt);
	fmtValloc(&buf, VALUE_MAX_STRING, fmt, args);

	if (traceHandler) {
		traceHandler(level, buf);
	}
	bfreeSafe(B_L, buf);
	va_end(args);
}

/******************************************************************************/
/*
 *	Trace log. Customize this function to log trace output
 */

void traceRaw(char_t *buf)
{
	if (traceHandler) {
		traceHandler(0, buf);
	}
}

/******************************************************************************/
/*
 *	Replace the default trace handler. Return a pointer to the old handler.
 */

void (*traceSetHandler(void (*function)(int level, char_t *buf))) 
	(int level, char *buf)
{
	void (*oldHandler)(int level, char_t *buf);

	oldHandler = traceHandler;
	if (function) {
		traceHandler = function;
	}
	return oldHandler;
}

/******************************************************************************/
/*
 *	Save the instance handle
 */

void emfInstSet(int inst)
{
	emfInst = inst;
}

/******************************************************************************/
/*
 *	Get the instance handle
 */

int emfInstGet()
{
	return emfInst;
}

/******************************************************************************/
/*
 *	Convert a string to lower case
 */

char_t *webs_strlower(char_t *string)
{
	char_t	*s;

	a_assert(string);

	if (string == NULL) {
		return NULL;
	}

	s = string;
	while (*s) {
		if (gisupper(*s)) {
			*s = (char_t) gtolower(*s);
		}
		s++;
	}
	*s = '\0';
	return string;
}

/******************************************************************************/
/* 
 *	Convert a string to upper case
 */

char_t *webs_strupper(char_t *string)
{
	char_t	*s;

	a_assert(string);
	if (string == NULL) {
		return NULL;
	}

	s = string;
	while (*s) {
		if (gislower(*s)) {
			*s = (char_t) gtoupper(*s);
		}
		s++;
	}
	*s = '\0';
	return string;
}

/******************************************************************************/
/*
 *	Convert integer to ascii string. Allow a NULL string in which case we
 *	allocate a dynamic buffer. 
 */

char_t *webs_stritoa(int n, char_t *string, int width)
{
	char_t	*cp, *lim, *s;
	char_t	buf[16];						/* Just temp to hold number */
	int		next, minus;

	a_assert(string && width > 0);

	if (string == NULL) {
		if (width == 0) {
			width = 10;
		}
		if ((string = balloc(B_L, width + 1)) == NULL) {
			return NULL;
		}
	}
	if (n < 0) {
		minus = 1;
		n = -n;
		width--;
	} else {
		minus = 0;
	}

	cp = buf;
	lim = &buf[width - 1];
	while (n > 9 && cp < lim) {
		next = n;
		n /= 10;
		*cp++ = (char_t) (next - n * 10 + '0');
	}
	if (cp < lim) {
		*cp++ = (char_t) (n + '0');
	}

	s = string;
	if (minus) {
		*s++ = '-';
	}

	while (cp > buf) {
		*s++ = *--cp;
	}

	*s++ = '\0';
	return string;
}

/******************************************************************************/
/*
 *	Stubs
 */

char_t *basicGetProduct()
{
	return T("uemf");
}

char_t *basicGetAddress()
{
	return T("localhost");
}

int errorOpen(char_t *pname)
{
	return 0;
}

void errorClose()
{
}

/******************************************************************************/

