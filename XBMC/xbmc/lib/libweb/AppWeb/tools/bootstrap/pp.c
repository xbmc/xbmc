/**
 *	@file 	pp.c
 *	@brief	Preprocessor for C, C++ and Java files
 *	@overview 
 *	@remarks Will not work correctly if input contains null characters.
 */

/*
 *	@copy	default
 *	
 *	Copyright (c) Mbedthis Software LLC, 2003-2007. All Rights Reserved.
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

/*
 * Copyright (c) 1985, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Dave Yost.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#if WIN
#include <direct.h>
#include <io.h>
#else
#include <dirent.h>
#endif

#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>

#define MPR_MAX_PATH 1024
FILE *input;
#ifndef YES
#define YES 1
#define NO  0
#endif/*YES */
typedef int Bool;

char *progname;
char *filename;
char text;				/* -t option in effect: this is a text file */
char lnblank;			/* -l option in effect: blank deleted lines */
char complement;		/* -c option in effect: complement the operation */
char java;				/* -j option in effect: java source */
char *outfile;			/* -o option in effect: output file */
FILE *out;

#define MAXSYMS 1000
char *symname[MAXSYMS]; /* symbol name */
char trueValue[MAXSYMS];/* -Dsym */
char ignore[MAXSYMS];   /* -iDsym or -iUsym */
char insym[MAXSYMS];	/* state: false, inactive, true */
#define SYM_INACTIVE 0	/* symbol is currently inactive */
#define SYM_FALSE	 1	/* symbol is currently false */
#define SYM_TRUE	 2	/* symbol is currently true  */

int nsyms;
char incomment;			/* inside C comment */
char inCPPcomment;		/* inside C++ comment */

#define QUOTE_NONE   0
#define QUOTE_SINGLE 1
#define QUOTE_DOUBLE 2
char inquote;		/* inside single or double quotes */

int exitstat;
char *skipcomment(char *cp);
char *skipquote (char *cp, int type);

/* MOB -- forward declarations */
int findsym(char *str);
void prname();
void pfile();
void flushline(Bool keep);
int error(int err, int line, int depth);
int getlin(char *line, int maxline, FILE *inp, int expandtabs);
char *mprGetDirName(char *buf, int bufsize, const char *path);
int mprMakeDir(char *dir, int perms);
int mprMakeDirPath(const char *path);

int main(int argc, char **argv)
{
	char dir[MPR_MAX_PATH];
	char **curarg;
	char *cp;
	char *cp1;
	char ignorethis;

	progname = "pp";
	out = stdout;

	for (curarg = &argv[1]; --argc > 0; curarg++) {
		if (*(cp1 = cp = *curarg) != '-') {
			break;
		}
		if (*++cp1 == 'i') {
			ignorethis = YES;
			cp1++;
		} else {
			ignorethis = NO;
		}
		if ((*cp1 == 'D' || *cp1 == 'U') && cp1[1] != '\0') {
			int symind;

			if ((symind = findsym(&cp1[1])) < 0) {
			if (nsyms >= MAXSYMS) {
				prname();
				fprintf(stderr, "too many symbols.\n");
				exit(2);
			}
			symind = nsyms++;
			symname[symind] = &cp1[1];
			insym[symind] = SYM_INACTIVE;
			}
			ignore[symind] = ignorethis;
			trueValue[symind] = *cp1 == 'D' ? YES : NO;
		} else if (ignorethis) {
			goto unrec;
		} else if (strcmp (&cp[1], "t") == 0) {
			text = YES;
		} else if (strcmp (&cp[1], "l") == 0) {
			lnblank = YES;
		} else if (strcmp (&cp[1], "c") == 0) {
			complement = YES;
		} else if (strcmp (&cp[1], "j") == 0) {
			java = YES;
		} else if (strcmp (&cp[1], "o") == 0) {
			if (--argc <= 0) {
				prname();
				fprintf(stderr, "missing output filename\n");
				exit(2);
			}
			outfile = *++curarg;

			mprGetDirName(dir, sizeof(dir), outfile);
			mprMakeDirPath(dir);
			out = fopen(outfile, "wt");
			if (out == NULL) {
				prname();
				fprintf(stderr, "Can't open %s\n", outfile);
				exit(2);
			}

		} else {
	 unrec:
			prname();
			fprintf(stderr, "unrecognized option: %s\n", cp);
			goto usage;
		}
	}

	if (argc > 1) {
		prname();
		fprintf(stderr, "can only do one file.\n");

	} else if (argc == 1) {
		filename = *curarg;
		if ((input = fopen(filename, "r")) != NULL) {
			pfile();
			fclose(input);
		} else {
			prname();
			fprintf(stderr, "can't open ");
			perror(*curarg);
		}
	} else {
		filename = "[stdin]";
		input = stdin;
		pfile();
	}

	fflush(out);
	exit(exitstat);

usage:
	fprintf (stderr, "\
Usage: pp [-l] [-t] [-c] [[-Dsym] [-Usym] [-iDsym] [-iUsym]]... [file]\n");
	exit (2);
}


/* types of input lines: */
typedef int Linetype;
#define LT_PLAIN       0   /* ordinary line */
#define LT_TRUE        1   /* a true  #ifdef of a symbol known to us */
#define LT_FALSE       2   /* a false #ifdef of a symbol known to us */
#define LT_OTHER       3   /* an #ifdef of a symbol not known to us */
#define LT_IF          4   /* an #ifdef of a symbol not known to us */
#define LT_ELSE        5   /* #else */
#define LT_ENDIF       6   /* #endif */
#define LT_LEOF        7   /* end of file */
extern Linetype checkline(int *cursym, int unknown);

typedef int Reject_level;
Reject_level reject;    	/* 0 or 1: pass thru; 1 or 2: ignore comments */
#define REJ_NO          0
#define REJ_IGNORE      1
#define REJ_YES         2

int doif(int thissym, int inif, int unknown, Reject_level prereject, int depth);

int linenum;    			/* current line number */
int stqcline;   			/* start of current coment or quote */
char *errs[] = {
#define NO_ERR      0
			"",
#define END_ERR     1
			"",
#define ELSE_ERR    2
			"Inappropriate else",
#define ENDIF_ERR   3
			"Inappropriate endif",
#define IEOF_ERR    4
			"Premature EOF in ifdef",
#define CEOF_ERR    5
			"Premature EOF in comment",
#define Q1EOF_ERR   6
			"Premature EOF in quoted character",
#define Q2EOF_ERR   7
			"Premature EOF in quoted string"
};

/* States for inif arg to doif */
#define IN_NONE 0
#define IN_IF   1
#define IN_ELSE 2

void pfile()
{
	reject = REJ_NO;
	doif(-1, IN_NONE, 0, reject, 0);
	return;
}

#if OLD_COMMENT
int thissym;   /* index of the symbol who was last ifdef'ed */
int inif;               /* YES or NO we are inside an ifdef */
Reject_level prevreject;/* previous value of reject */
int depth;              /* depth of ifdef's */
#endif

int doif(int thissym, int inif, int unknown, Reject_level prevreject, int depth)
{
	Linetype lineval;
	Reject_level thisreject;
	int doret;          /* tmp return value of doif */
	int cursym;         /* index of the symbol returned by checkline */
	int stline;         /* line number when called this time */

	stline = linenum;
	for (;;) {
		switch (lineval = checkline(&cursym, unknown)) {
			case LT_PLAIN:
				flushline(YES);
				break;

			case LT_TRUE:
			case LT_FALSE:
				thisreject = reject;
				if (lineval == LT_TRUE) {
					insym[cursym] = SYM_TRUE;
				} else {
					if (reject != REJ_YES) {
						reject = ignore[cursym] ? REJ_IGNORE : REJ_YES;
					}
					insym[cursym] = SYM_FALSE;
				}
				if (ignore[cursym]) {
					flushline(YES);
				} else {
//MOB				exitstat = 1;
					flushline(NO);
				}
				doret = doif(cursym, IN_IF, 1, thisreject, depth + 1);
				if (doret != NO_ERR) {
					return error(doret, stline, depth);
				}
				break;

			case LT_IF:
			case LT_OTHER:
				flushline(YES);
				if ((doret = doif(-1, IN_IF, 0, reject, depth + 1)) != NO_ERR) {
					return error(doret, stline, depth);
				}
				break;

			case LT_ELSE:
				if (inif != IN_IF) {
					return error(ELSE_ERR, linenum, depth);
				}
				inif = IN_ELSE;
				if (thissym >= 0) {
					if (insym[thissym] == SYM_TRUE) {
						reject = ignore[thissym] ? REJ_IGNORE : REJ_YES;
						insym[thissym] = SYM_FALSE;
					} else { /* (insym[thissym] == SYM_FALSE) */
						reject = prevreject;
						insym[thissym] = SYM_TRUE;
					}
					if (!ignore[thissym]) {
						flushline(NO);
						break;
					}
				}
				flushline(YES);
				break;

			case LT_ENDIF:
				if (inif == IN_NONE) {
					return error (ENDIF_ERR, linenum, depth);
				}
				if (thissym >= 0) {
					insym[thissym] = SYM_INACTIVE;
					reject = prevreject;
					if (!ignore[thissym]) {
						flushline(NO);
						return NO_ERR;
					}
				}
				flushline(YES);
				return NO_ERR;

			case LT_LEOF: {
				int err;
				err = incomment 
					? CEOF_ERR : inquote == QUOTE_SINGLE
					? Q1EOF_ERR : inquote == QUOTE_DOUBLE
					? Q2EOF_ERR : NO_ERR;
				if (inif != IN_NONE) {
					if (err != NO_ERR)
						error (err, stqcline, depth);
					return error (IEOF_ERR, stline, depth);
				} else if (err != NO_ERR) {
					return error (err, stqcline, depth);
				} else {
					return NO_ERR;
				}
			}
		}
	}
}

#define endsym(c) (!isalpha (c) && !isdigit (c) && c != '_')

#define KWSIZE 8
#define MAXLINE 256
char tline[MAXLINE];

Linetype checkline(int *cursym, int unknown)
{
	char *cp, *symp, *scp, *np;
	char keyword[KWSIZE];
	Linetype retval;
	int symind, isDirective;

	linenum++;
	if (getlin(tline, sizeof tline, input, NO) == EOF) {
		return LT_LEOF;
	}

	retval = LT_PLAIN;
	np = tline;

	if (java) {
		for (np = tline; isspace(*np); np++) {
			;
		}
		isDirective = (np[0] == '/' && np[1] == '/' && np[2] == '#');
	} else {
		isDirective = (*np == '#');
	}

	if (! isDirective || incomment || inquote == QUOTE_SINGLE
			|| inquote == QUOTE_DOUBLE) {
		cp = np;
		goto eol;
	}

	if (java) {
		cp = &np[3];
	} else {
		cp = np;
		cp = skipcomment(++cp);
	}

	symp = keyword;
	while (!endsym (*cp)) {
	*symp = *cp++;
	if (++symp >= &keyword[KWSIZE])
		goto eol;
	}
	*symp = '\0';

	if (strcmp (keyword, "if") == 0) {
		retval = YES;
		goto ifdef;
	} else if (strcmp (keyword, "ifdef") == 0) {
		retval = YES;
		goto ifdef;
	} else if (strcmp (keyword, "ifndef") == 0) {
		retval = NO;
ifdef:
		scp = cp = skipcomment (++cp);
		if (incomment) {
			retval = LT_PLAIN;
			goto eol;
		}
		if ((symind = findsym (scp)) >= 0) {
			retval = (retval ^ trueValue[*cursym = symind])
				 ? LT_FALSE : LT_TRUE;
		} else {
			retval = LT_OTHER;
		}
	} else if (strcmp (keyword, "if") == 0) {
		retval = LT_IF;
	} else if (strcmp (keyword, "else") == 0) {
		retval = LT_ELSE;
	} else if (strcmp (keyword, "endif") == 0) {
		retval = LT_ENDIF;
	} else {
		if (java) {
			if (unknown || reject == REJ_YES) {
				for (np = &tline[3]; *np; np++) {
					np[-3] = *np;
				}
				np[-3] = '\0';
			}
			retval = LT_PLAIN;
		}
	}

 eol:
	if (!text && reject != REJ_IGNORE) {
		for (; *cp;) {
			if (incomment)
			cp = skipcomment (cp);
			else if (inquote == QUOTE_SINGLE)
			cp = skipquote (cp, QUOTE_SINGLE);
			else if (inquote == QUOTE_DOUBLE)
			cp = skipquote (cp, QUOTE_DOUBLE);
			else if (*cp == '/' && (cp[1] == '*' || cp[1] == '/'))
			cp = skipcomment (cp);
			else if (*cp == '\'')
			cp = skipquote (cp, QUOTE_SINGLE);
			else if (*cp == '"')
			cp = skipquote (cp, QUOTE_DOUBLE);
			else
			cp++;
		}
	}
	return retval;
}

/*
 *  Skip over comments and stop at the next charaacter
 *  position that is not whitespace.
 */
char * skipcomment(char *cp)
{
	if (incomment) {
		goto inside;
	}

	for (;; cp++) {
		while (*cp == ' ' || *cp == '\t') {
			cp++;
		}
		if (text) {
			return cp;
		}
		if (cp[0] != '/' || (cp[1] != '*' && cp[1] != '/')) {
			return cp;
		}
		if (!incomment) {
			incomment = YES;
			inCPPcomment = (cp[1] == '/');
			stqcline = linenum;
		}
		cp += 2;
	 inside:
		for (;;) {
			for (; *cp != '*'; cp++)
			if (*cp == '\0') {
				if (inCPPcomment) {
					incomment = NO;
				}
				return cp;
			}
			if (*++cp == '/') {
				incomment = NO;
				break;
			}
		}
	}
}

/*
 *  Skip over a quoted string or character and stop at the next charaacter
 *  position that is not whitespace.
 */
char *skipquote (char *cp, int type)
{
	char qchar;

	qchar = type == QUOTE_SINGLE ? '\'' : '"';

	if (inquote == type) {
		goto inside;
	}

	for (;; cp++) {
		if (*cp != qchar) {
			return cp;
		}
		cp++;
		inquote = type;
		stqcline = linenum;
	 inside:
		for (; ; cp++) {
			if (*cp == qchar) {
				break;
			}
			if (*cp == '\0' || (*cp == '\\' && *++cp == '\0')) {
				return cp;
			}
		}
		inquote = QUOTE_NONE;
	}
}

/*
 *  findsym - look for the symbol in the symbol table.
 *            if found, return symbol table index,
 *            else return -1.
 */
int findsym(char *str)
{
	char *cp, *symp;
	char chr;
	int symind;

	for (symind = 0; symind < nsyms; ++symind) {
		if (insym[symind] == SYM_INACTIVE) {
			symp = symname[symind];
			for (cp = str; *symp && *cp == *symp; cp++, symp++) {
				continue;
			}
			chr = *cp;
			if (*symp == '\0' && endsym (chr)) {
				return symind;
			}
		}
	}
	return -1;
}

/*
 *   getlin - expands tabs if asked for
 *            and (if compiled in) treats form-feed as an end-of-line
 */
int getlin(char *line, int maxline, FILE *inp, int expandtabs)
{
	int tmp, num, chr;
#ifdef  FFSPECIAL
	static char havechar = NO;  /* have leftover char from last time */
	static char svchar;
#endif/*FFSPECIAL */

	num = 0;
#ifdef  FFSPECIAL
	if (havechar) {
		havechar = NO;
		chr = svchar;
		goto ent;
	}
#endif/*FFSPECIAL */
	while (num + 8 < maxline) {   /* leave room for tab */
		chr = getc (inp);
		if (isprint (chr)) {
#ifdef  FFSPECIAL
 ent:
#endif/*FFSPECIAL */
			*line++ = chr;
			num++;
		} else
			switch (chr) {
			case EOF:
				return EOF;

			case '\t':
				if (expandtabs) {
					num += tmp = 8 - (num & 7);
					do
					*line++ = ' ';
					while (--tmp);
					break;
				}
				default:
				*line++ = chr;
				num++;
				break;

			case '\n':
				*line = '\n';
				num++;
				goto end;

#ifdef  FFSPECIAL
			case '\f':
				if (++num == 1)
					*line = '\f';
				else {
					*line = '\n';
					havechar = YES;
					svchar = chr;
				}
				goto end;
#endif/*FFSPECIAL */
			}
		}
 end:
	*++line = '\0';
	return num;
}


void flushline(Bool keep)
{
	if ((keep && reject != REJ_YES) ^ complement) {
		char *line = tline;
		char chr;

		while ((chr = *line++) != 0) {
			putc(chr, out);
		}

	} else if (lnblank) {
		putc('\n', out);
	}
	return;
}


void prname()
{
	fprintf(stderr, "%s: ", progname);
	return;
}

int error(int err, int line, int depth)
{
	if (err == END_ERR) {
		return err;
	}

	prname();

#ifndef TESTING
	fprintf(stderr, "Error in %s line %d: %s.\n", filename, line, errs[err]);
#else/* TESTING */
	fprintf(stderr, "Error in %s line %d: %s. ", filename, line, errs[err]);
	fprintf(stderr, "ifdef depth: %d\n", depth);
#endif/*TESTING */

	exitstat = 2;

	return depth > 1 ? IEOF_ERR : END_ERR;
}


/*
 *	Return the directory portion of a pathname into the users buffer.
 */
char *mprGetDirName(char *buf, int bufsize, const char *path)
{
	char	*cp;
	int		dlen;

	cp = strrchr(path, '/');
	if (cp == 0) {
#if WIN
		cp = strrchr(path, '\\');
		if (cp == 0)
#endif
		{
			buf[0] = '\0';
			return buf;
		}
	}

	if (cp == path && cp[1] == '\0') {
		strcpy(buf, ".");
		return buf;
	}

	dlen = cp - path;
	if (dlen < bufsize) {
		if (dlen == 0) {
			dlen++;
		}
		memcpy(buf, path, dlen);
		buf[dlen] = '\0';
		return buf;
	}
	return 0;
}


/*
 *	Thread-safe wrapping of strtok. Note "str" is modifed as per strtok()
 */
char *mprStrTok(char *str, const char *delim, char **last)
{
	char	*start, *end;
	int		i;

	start = str ? str : *last;

	if (start == 0) {
		return 0;
	}
	
	i = strspn(start, delim);
	start += i;
	if (*start == '\0') {
		*last = 0;
		return 0;
	}
	end = strpbrk(start, delim);
	if (end) {
		*end++ = '\0';
		i = strspn(end, delim);
		end += i;
	}
	*last = end;
	return start;
}




/*
 *	Make intervening directories
 */
int mprMakeDirPath(const char *path)
{
	struct stat	sbuf;
	char		dir[MPR_MAX_PATH], buf[MPR_MAX_PATH];
	char		*dirSep;
	char		*next, *tok;

	dir[0] = '\0';
	dirSep = "/\\";

	if (path == 0 || *path == '\0') {
		return -1;
	}

	strcpy(buf, path);
	next = mprStrTok(buf, dirSep, &tok);
	if (*buf == '/') {
		dir[0] = '/';
	}
	while (next != NULL) {
		if (strcmp(next, ".") == 0 ) {
			next = mprStrTok(NULL, dirSep, &tok);
			continue;
		}
		strcat(dir, next);
		if (stat(dir, &sbuf) != 0) {
			if (mprMakeDir(dir, 0755) < 0) {
				return -1;
			}
		}
		strcat(dir, "/");
		next = mprStrTok(NULL, dirSep, &tok);
	}
	return 0;
}



int mprMakeDir(char *dir, int perms)
{
	int		rc;

#if WIN
	rc = _mkdir(dir);
#elif  VXWORKS
	rc = mkdir(dir);
#else
	rc = mkdir(dir, 0775);
#endif
	return rc;
}
