/*
 * asp.c -- Active Server Page Support
 *
 * Copyright (c) GoAhead Software Inc., 1995-2000. All Rights Reserved.
 *
 * See the file "license.txt" for usage and redistribution license requirements
 *
 * $Id: asp.c,v 1.3 2002/10/24 14:44:50 bporter Exp $
 */

/******************************** Description *********************************/

/*
 *	The ASP module processes ASP pages and executes embedded scripts. It 
 *	support an open scripting architecture with in-built support for 
 *	Ejscript(TM).
 */

/********************************* Includes ***********************************/

#include	"wsIntrn.h"

/********************************** Locals ************************************/

static sym_fd_t	websAspFunctions = -1;	/* Symbol table of functions */
static int		aspOpenCount = 0;		/* count of apps using this module */

/***************************** Forward Declarations ***************************/

static char_t	*strtokcmp(char_t *s1, char_t *s2);
static char_t	*skipWhite(char_t *s);

/************************************* Code ***********************************/
/*
 *	Create script spaces and commands
 */

int websAspOpen()
{
	if (++aspOpenCount == 1) {
/*
 *	Create the table for ASP functions
 */
		websAspFunctions = symOpen(WEBS_SYM_INIT * 2);

/*
 *	Create standard ASP commands
 */
		websAspDefine(T("write"), websAspWrite);
		websAspDefine(T("isset"), websAspIsSet);
	}
	return 0;
}

/************************************* Code ***********************************/
/*
 *	Close Asp symbol table.
 */

void websAspClose()
{
	if (--aspOpenCount <= 0) {
		if (websAspFunctions != -1) {
			symClose(websAspFunctions);
			websAspFunctions = -1;
		}
	}
}

/******************************************************************************/
/*
 *	Process ASP requests and expand all scripting commands. We read the
 *	entire ASP page into memory and then process. If you have really big 
 *	documents, it is better to make them plain HTML files rather than ASPs.
 */

int websAspRequest(webs_t wp, char_t *lpath)
{
	websStatType	sbuf;
	char			*rbuf;
	char_t			*token, *lang, *result, *path, *ep, *cp, *buf, *nextp;
	char_t			*last;
	int				rc, engine, len, ejid;

	a_assert(websValid(wp));
	a_assert(lpath && *lpath);

	rc = -1;
	buf = NULL;
	rbuf = NULL;
	engine = EMF_SCRIPT_EJSCRIPT;
	wp->flags |= WEBS_HEADER_DONE;
	path = websGetRequestPath(wp);

/*
 *	Create Ejscript instance in case it is needed
 */
	ejid = ejOpenEngine(wp->cgiVars, websAspFunctions);
	if (ejid < 0) {
		websError(wp, 200, T("Can't create Ejscript engine"));
		goto done;
	}
	ejSetUserHandle(ejid, wp);

	if (websPageStat(wp, lpath, path, &sbuf) < 0) {
		websError(wp, 200, T("Can't stat %s"), lpath);
		goto done;
	}

/*
 *	Create a buffer to hold the ASP file in-memory
 */
	len = sbuf.size * sizeof(char);
	if ((rbuf = balloc(B_L, len + 1)) == NULL) {
		websError(wp, 200, T("Can't get memory"));
		goto done;
	}
	rbuf[len] = '\0';

	if (websPageReadData(wp, rbuf, len) != len) {
		websError(wp, 200, T("Cant read %s"), lpath);
		goto done;
	}
	websPageClose(wp);

/*
 *	Convert to UNICODE if necessary.
 */
	if ((buf = ballocAscToUni(rbuf, len)) == NULL) {
		websError(wp, 200, T("Can't get memory"));
		goto done;
	}

/*
 *	Scan for the next "<%"
 */
	last = buf;
	rc = 0;
	while (rc == 0 && *last && ((nextp = gstrstr(last, T("<%"))) != NULL)) {
		websWriteBlock(wp, last, (nextp - last));
		nextp = skipWhite(nextp + 2);

/*
 *		Decode the language
 */
		token = T("language");

		if ((lang = strtokcmp(nextp, token)) != NULL) {
			if ((cp = strtokcmp(lang, T("=javascript"))) != NULL) {
				engine = EMF_SCRIPT_EJSCRIPT;
			} else {
				cp = nextp;
			}
			nextp = cp;
		}

/*
 *		Find tailing bracket and then evaluate the script
 */
		if ((ep = gstrstr(nextp, T("%>"))) != NULL) {

			*ep = '\0';
			last = ep + 2;
			nextp = skipWhite(nextp);
/*
 *			Handle backquoted newlines
 */
			for (cp = nextp; *cp; ) {
				if (*cp == '\\' && (cp[1] == '\r' || cp[1] == '\n')) {
					*cp++ = ' ';
					while (*cp == '\r' || *cp == '\n') {
						*cp++ = ' ';
					}
				} else {
					cp++;
				}
			}

/*
 *			Now call the relevant script engine. Output is done directly
 *			by the ASP script procedure by calling websWrite()
 */
			if (*nextp) {
				result = NULL;
				if (engine == EMF_SCRIPT_EJSCRIPT) {
					rc = scriptEval(engine, nextp, &result, (void*) ejid);
				} else {
					rc = scriptEval(engine, nextp, &result, wp);
				}
				if (rc < 0) {
/*
 *					On an error, discard all output accumulated so far
 *					and store the error in the result buffer. Be careful if the
 *					user has called websError() already.
 */
					if (websValid(wp)) {
						if (result) {
							websWrite(wp, T("<h2><b>ASP Error: %s</b></h2>\n"), 
								result);
							websWrite(wp, T("<pre>%s</pre>"), nextp);
							bfree(B_L, result);
						} else {
							websWrite(wp, T("<h2><b>ASP Error</b></h2>\n%s\n"),
								nextp);
						}
						websWrite(wp, T("</body></html>\n"));
						rc = 0;
					}
					goto done;
				}
			}

		} else {
			websError(wp, 200, T("Unterminated script in %s: \n"), lpath);
			rc = -1;
			goto done;
		}
	}
/*
 *	Output any trailing HTML page text
 */
	if (last && *last && rc == 0) {
		websWriteBlock(wp, last, gstrlen(last));
	}
	rc = 0;

/*
 *	Common exit and cleanup
 */
done:
	if (websValid(wp)) {
		websPageClose(wp);
		if (ejid >= 0) {
			ejCloseEngine(ejid);
		}
	}
	bfreeSafe(B_L, buf);
	bfreeSafe(B_L, rbuf);
	return rc;
}

/******************************************************************************/
/*
 *	Define an ASP Ejscript function. Bind an ASP name to a C procedure.
 */

int websAspDefine(char_t *name, 
	int (*fn)(int ejid, webs_t wp, int argc, char_t **argv))
{
	return ejSetGlobalFunctionDirect(websAspFunctions, name, 
		(int (*)(int, void*, int, char_t**)) fn);
}

/******************************************************************************/
/*
 *	Asp write command. This implemements <% write("text"); %> command
 */

int websAspWrite(int ejid, webs_t wp, int argc, char_t **argv)
{
	int		i;

	a_assert(websValid(wp));
	
	for (i = 0; i < argc; ) {
		a_assert(argv);
		if (websWriteBlock(wp, argv[i], gstrlen(argv[i])) < 0) {
			return -1;
		}
		if (++i < argc) {
			if (websWriteBlock(wp, T(" "), 2) < 0) {
				return -1;
			}
		}
	}
	return 0;
}

/******************************************************************************/
/*
 *	Asp isset command. This implemements <% isset("text"); %> command
 */

int websAspIsSet(int ejid, webs_t wp, int argc, char_t **argv)
{
	if (argc != 1) return -1;
	ejSetResult( ejid, websGetVar(wp, argv[0], NULL) ? "1" : "0");
	return 0;
}

/******************************************************************************/
/*
 *	strtokcmp -- Find s2 in s1. We skip leading white space in s1.
 *	Return a pointer to the location in s1 after s2 ends.
 */

static char_t *strtokcmp(char_t *s1, char_t *s2)
{
	int		len;

	s1 = skipWhite(s1);
	len = gstrlen(s2);
	for (len = gstrlen(s2); len > 0 && (tolower(*s1) == tolower(*s2)); len--) {
		if (*s2 == '\0') {
			return s1;
		}
		s1++;
		s2++;
	}
	if (len == 0) {
		return s1;
	}
	return NULL;
}

/******************************************************************************/
/*
 *	Skip white space
 */

static char_t *skipWhite(char_t *s) 
{
	a_assert(s);

	if (s == NULL) {
		return s;
	}
	while (*s && gisspace(*s)) {
		s++;
	}
	return s;
}

/******************************************************************************/

