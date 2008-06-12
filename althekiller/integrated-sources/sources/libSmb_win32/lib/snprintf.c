/*
 * NOTE: If you change this file, please merge it into rsync, samba, etc.
 */

/*
 * Copyright Patrick Powell 1995
 * This code is based on code written by Patrick Powell (papowell@astart.com)
 * It may be used for any purpose as long as this notice remains intact
 * on all source code distributions
 */

/**************************************************************
 * Original:
 * Patrick Powell Tue Apr 11 09:48:21 PDT 1995
 * A bombproof version of doprnt (dopr) included.
 * Sigh.  This sort of thing is always nasty do deal with.  Note that
 * the version here does not include floating point...
 *
 * snprintf() is used instead of sprintf() as it does limit checks
 * for string length.  This covers a nasty loophole.
 *
 * The other functions are there to prevent NULL pointers from
 * causing nast effects.
 *
 * More Recently:
 *  Brandon Long <blong@fiction.net> 9/15/96 for mutt 0.43
 *  This was ugly.  It is still ugly.  I opted out of floating point
 *  numbers, but the formatter understands just about everything
 *  from the normal C string format, at least as far as I can tell from
 *  the Solaris 2.5 printf(3S) man page.
 *
 *  Brandon Long <blong@fiction.net> 10/22/97 for mutt 0.87.1
 *    Ok, added some minimal floating point support, which means this
 *    probably requires libm on most operating systems.  Don't yet
 *    support the exponent (e,E) and sigfig (g,G).  Also, fmtint()
 *    was pretty badly broken, it just wasn't being exercised in ways
 *    which showed it, so that's been fixed.  Also, formated the code
 *    to mutt conventions, and removed dead code left over from the
 *    original.  Also, there is now a builtin-test, just compile with:
 *           gcc -DTEST_SNPRINTF -o snprintf snprintf.c -lm
 *    and run snprintf for results.
 * 
 *  Thomas Roessler <roessler@guug.de> 01/27/98 for mutt 0.89i
 *    The PGP code was using unsigned hexadecimal formats. 
 *    Unfortunately, unsigned formats simply didn't work.
 *
 *  Michael Elkins <me@cs.hmc.edu> 03/05/98 for mutt 0.90.8
 *    The original code assumed that both snprintf() and vsnprintf() were
 *    missing.  Some systems only have snprintf() but not vsnprintf(), so
 *    the code is now broken down under HAVE_SNPRINTF and HAVE_VSNPRINTF.
 *
 *  Andrew Tridgell (tridge@samba.org) Oct 1998
 *    fixed handling of %.0f
 *    added test for HAVE_LONG_DOUBLE
 *
 * tridge@samba.org, idra@samba.org, April 2001
 *    got rid of fcvt code (twas buggy and made testing harder)
 *    added C99 semantics
 *
 * date: 2002/12/19 19:56:31;  author: herb;  state: Exp;  lines: +2 -0
 * actually print args for %g and %e
 * 
 * date: 2002/06/03 13:37:52;  author: jmcd;  state: Exp;  lines: +8 -0
 * Since includes.h isn't included here, VA_COPY has to be defined here.  I don't
 * see any include file that is guaranteed to be here, so I'm defining it
 * locally.  Fixes AIX and Solaris builds.
 * 
 * date: 2002/06/03 03:07:24;  author: tridge;  state: Exp;  lines: +5 -13
 * put the ifdef for HAVE_VA_COPY in one place rather than in lots of
 * functions
 * 
 * date: 2002/05/17 14:51:22;  author: jmcd;  state: Exp;  lines: +21 -4
 * Fix usage of va_list passed as an arg.  Use __va_copy before using it
 * when it exists.
 * 
 * date: 2002/04/16 22:38:04;  author: idra;  state: Exp;  lines: +20 -14
 * Fix incorrect zpadlen handling in fmtfp.
 * Thanks to Ollie Oldham <ollie.oldham@metro-optix.com> for spotting it.
 * few mods to make it easier to compile the tests.
 * addedd the "Ollie" test to the floating point ones.
 *
 * Martin Pool (mbp@samba.org) April 2003
 *    Remove NO_CONFIG_H so that the test case can be built within a source
 *    tree with less trouble.
 *    Remove unnecessary SAFE_FREE() definition.
 *
 * Martin Pool (mbp@samba.org) May 2003
 *    Put in a prototype for dummy_snprintf() to quiet compiler warnings.
 *
 *    Move #endif to make sure VA_COPY, LDOUBLE, etc are defined even
 *    if the C library has some snprintf functions already.
 *
 * Darren Tucker (dtucker@zip.com.au) 2005
 *    Fix bug allowing read overruns of the source string with "%.*s"
 *    Usually harmless unless the read runs outside the process' allocation
 *    (eg if your malloc does guard pages) in which case it will segfault.
 *    From OpenSSH.  Also added test for same.
 *
 * Simo Sorce (idra@samba.org) Jan 2006
 * 
 *    Add support for position independent parameters 
 *    fix fmtstr now it conforms to sprintf wrt min.max
 *
 **************************************************************/

#ifndef NO_CONFIG_H
#include "config.h"
#else
#define NULL 0
#endif 

#ifdef TEST_SNPRINTF /* need math library headers for testing */

/* In test mode, we pretend that this system doesn't have any snprintf
 * functions, regardless of what config.h says. */
#  undef HAVE_SNPRINTF
#  undef HAVE_VSNPRINTF
#  undef HAVE_C99_VSNPRINTF
#  undef HAVE_ASPRINTF
#  undef HAVE_VASPRINTF
#  include <math.h>
#endif /* TEST_SNPRINTF */

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif
#ifdef HAVE_CTYPE_H
#include <ctype.h>
#endif
#include <sys/types.h>
#include <stdarg.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#if defined(HAVE_SNPRINTF) && defined(HAVE_VSNPRINTF) && defined(HAVE_C99_VSNPRINTF)
/* only include stdio.h if we are not re-defining snprintf or vsnprintf */
#include <stdio.h>
 /* make the compiler happy with an empty file */
 void dummy_snprintf(void);
 void dummy_snprintf(void) {} 
#endif /* HAVE_SNPRINTF, etc */

#ifdef HAVE_LONG_DOUBLE
#define LDOUBLE long double
#else
#define LDOUBLE double
#endif

#ifdef HAVE_LONG_LONG
#define LLONG long long
#else
#define LLONG long
#endif

#ifndef VA_COPY
#ifdef HAVE_VA_COPY
#define VA_COPY(dest, src) va_copy(dest, src)
#else
#ifdef HAVE___VA_COPY
#define VA_COPY(dest, src) __va_copy(dest, src)
#else
#define VA_COPY(dest, src) (dest) = (src)
#endif
#endif

/*
 * dopr(): poor man's version of doprintf
 */

/* format read states */
#define DP_S_DEFAULT 0
#define DP_S_FLAGS   1
#define DP_S_MIN     2
#define DP_S_DOT     3
#define DP_S_MAX     4
#define DP_S_MOD     5
#define DP_S_CONV    6
#define DP_S_DONE    7

/* format flags - Bits */
#define DP_F_MINUS 	(1 << 0)
#define DP_F_PLUS  	(1 << 1)
#define DP_F_SPACE 	(1 << 2)
#define DP_F_NUM   	(1 << 3)
#define DP_F_ZERO  	(1 << 4)
#define DP_F_UP    	(1 << 5)
#define DP_F_UNSIGNED 	(1 << 6)

/* Conversion Flags */
#define DP_C_CHAR    1
#define DP_C_SHORT   2
#define DP_C_LONG    3
#define DP_C_LDOUBLE 4
#define DP_C_LLONG   5

/* Chunk types */
#define CNK_FMT_STR 0
#define CNK_INT     1
#define CNK_OCTAL   2
#define CNK_UINT    3
#define CNK_HEX     4
#define CNK_FLOAT   5
#define CNK_CHAR    6
#define CNK_STRING  7
#define CNK_PTR     8
#define CNK_NUM     9
#define CNK_PRCNT   10

#define char_to_int(p) ((p)- '0')
#ifndef MAX
#define MAX(p,q) (((p) >= (q)) ? (p) : (q))
#endif

/* yes this really must be a ||. Don't muck with this (tridge) */
#if !defined(HAVE_VSNPRINTF) || !defined(HAVE_C99_VSNPRINTF)

struct pr_chunk {
	int type; /* chunk type */
	int num; /* parameter number */
	int min; 
	int max;
	int flags;
	int cflags;
	int start;
	int len;
	LLONG value;
	LDOUBLE fvalue;
	char *strvalue;
	void *pnum;
	struct pr_chunk *min_star;
	struct pr_chunk *max_star;
	struct pr_chunk *next;
};

struct pr_chunk_x {
	struct pr_chunk **chunks;
	int num;
};

static size_t dopr(char *buffer, size_t maxlen, const char *format, 
		   va_list args_in);
static void fmtstr(char *buffer, size_t *currlen, size_t maxlen,
		    char *value, int flags, int min, int max);
static void fmtint(char *buffer, size_t *currlen, size_t maxlen,
		    LLONG value, int base, int min, int max, int flags);
static void fmtfp(char *buffer, size_t *currlen, size_t maxlen,
		   LDOUBLE fvalue, int min, int max, int flags);
static void dopr_outch(char *buffer, size_t *currlen, size_t maxlen, char c);
static struct pr_chunk *new_chunk(void);
static int add_cnk_list_entry(struct pr_chunk_x **list,
				int max_num, struct pr_chunk *chunk);

static size_t dopr(char *buffer, size_t maxlen, const char *format, va_list args_in)
{
	char ch;
	int state;
	int pflag;
	int pnum;
	int pfirst;
	size_t currlen;
	va_list args;
	const char *base;
	struct pr_chunk *chunks = NULL;
	struct pr_chunk *cnk = NULL;
	struct pr_chunk_x *clist = NULL;
	int max_pos;
	size_t ret = -1;

	VA_COPY(args, args_in);

	state = DP_S_DEFAULT;
	pfirst = 1;
	pflag = 0;
	pnum = 0;

	max_pos = 0;
	base = format;
	ch = *format++;
	
	/* retrieve the string structure as chunks */
	while (state != DP_S_DONE) {
		if (ch == '\0') 
			state = DP_S_DONE;

		switch(state) {
		case DP_S_DEFAULT:
			
			if (cnk) {
				cnk->next = new_chunk();
				cnk = cnk->next;
			} else {
				cnk = new_chunk();
			}
			if (!cnk) goto done;
			if (!chunks) chunks = cnk;
			
			if (ch == '%') {
				state = DP_S_FLAGS;
				ch = *format++;
			} else {
				cnk->type = CNK_FMT_STR;
				cnk->start = format - base -1;
				while ((ch != '\0') && (ch != '%')) ch = *format++;
				cnk->len = format - base - cnk->start -1;
			}
			break;
		case DP_S_FLAGS:
			switch (ch) {
			case '-':
				cnk->flags |= DP_F_MINUS;
				ch = *format++;
				break;
			case '+':
				cnk->flags |= DP_F_PLUS;
				ch = *format++;
				break;
			case ' ':
				cnk->flags |= DP_F_SPACE;
				ch = *format++;
				break;
			case '#':
				cnk->flags |= DP_F_NUM;
				ch = *format++;
				break;
			case '0':
				cnk->flags |= DP_F_ZERO;
				ch = *format++;
				break;
			case 'I':
				/* internationalization not supported yet */
				ch = *format++;
				break;
			default:
				state = DP_S_MIN;
				break;
			}
			break;
		case DP_S_MIN:
			if (isdigit((unsigned char)ch)) {
				cnk->min = 10 * cnk->min + char_to_int (ch);
				ch = *format++;
			} else if (ch == '$') {
				if (!pfirst && !pflag) {
					/* parameters must be all positioned or none */
					goto done;
				}
				if (pfirst) {
					pfirst = 0;
					pflag = 1;
				}
				if (cnk->min == 0) /* what ?? */
					goto done;
				cnk->num = cnk->min;
				cnk->min = 0;
				ch = *format++;
			} else if (ch == '*') {
				if (pfirst) pfirst = 0;
				cnk->min_star = new_chunk();
				if (!cnk->min_star) /* out of memory :-( */
					goto done;
				cnk->min_star->type = CNK_INT;
				if (pflag) {
					int num;
					ch = *format++;
					if (!isdigit((unsigned char)ch)) {
						/* parameters must be all positioned or none */
						goto done;
					}
					for (num = 0; isdigit((unsigned char)ch); ch = *format++) {
						num = 10 * num + char_to_int(ch);
					}
					cnk->min_star->num = num;
					if (ch != '$') /* what ?? */
						goto done;
				} else {
					cnk->min_star->num = ++pnum;
				}
				max_pos = add_cnk_list_entry(&clist, max_pos, cnk->min_star);
				if (max_pos == 0) /* out of memory :-( */
					goto done;
				ch = *format++;
				state = DP_S_DOT;
			} else {
				if (pfirst) pfirst = 0;
				state = DP_S_DOT;
			}
			break;
		case DP_S_DOT:
			if (ch == '.') {
				state = DP_S_MAX;
				ch = *format++;
			} else { 
				state = DP_S_MOD;
			}
			break;
		case DP_S_MAX:
			if (isdigit((unsigned char)ch)) {
				if (cnk->max < 0)
					cnk->max = 0;
				cnk->max = 10 * cnk->max + char_to_int (ch);
				ch = *format++;
			} else if (ch == '$') {
				if (!pfirst && !pflag) {
					/* parameters must be all positioned or none */
					goto done;
				}
				if (cnk->max <= 0) /* what ?? */
					goto done;
				cnk->num = cnk->max;
				cnk->max = -1;
				ch = *format++;
			} else if (ch == '*') {
				cnk->max_star = new_chunk();
				if (!cnk->max_star) /* out of memory :-( */
					goto done;
				cnk->max_star->type = CNK_INT;
				if (pflag) {
					int num;
					ch = *format++;
					if (!isdigit((unsigned char)ch)) {
						/* parameters must be all positioned or none */
						goto done;
					}
					for (num = 0; isdigit((unsigned char)ch); ch = *format++) {
						num = 10 * num + char_to_int(ch);
					}
					cnk->max_star->num = num;
					if (ch != '$') /* what ?? */
						goto done;
				} else {
					cnk->max_star->num = ++pnum;
				}
				max_pos = add_cnk_list_entry(&clist, max_pos, cnk->max_star);
				if (max_pos == 0) /* out of memory :-( */
					goto done;

				ch = *format++;
				state = DP_S_MOD;
			} else {
				state = DP_S_MOD;
			}
			break;
		case DP_S_MOD:
			switch (ch) {
			case 'h':
				cnk->cflags = DP_C_SHORT;
				ch = *format++;
				if (ch == 'h') {
					cnk->cflags = DP_C_CHAR;
					ch = *format++;
				}
				break;
			case 'l':
				cnk->cflags = DP_C_LONG;
				ch = *format++;
				if (ch == 'l') {	/* It's a long long */
					cnk->cflags = DP_C_LLONG;
					ch = *format++;
				}
				break;
			case 'L':
				cnk->cflags = DP_C_LDOUBLE;
				ch = *format++;
				break;
			default:
				break;
			}
			state = DP_S_CONV;
			break;
		case DP_S_CONV:
			if (cnk->num == 0) cnk->num = ++pnum;
			max_pos = add_cnk_list_entry(&clist, max_pos, cnk);
			if (max_pos == 0) /* out of memory :-( */
				goto done;
			
			switch (ch) {
			case 'd':
			case 'i':
				cnk->type = CNK_INT;
				break;
			case 'o':
				cnk->type = CNK_OCTAL;
				cnk->flags |= DP_F_UNSIGNED;
				break;
			case 'u':
				cnk->type = CNK_UINT;
				cnk->flags |= DP_F_UNSIGNED;
				break;
			case 'X':
				cnk->flags |= DP_F_UP;
			case 'x':
				cnk->type = CNK_HEX;
				cnk->flags |= DP_F_UNSIGNED;
				break;
			case 'A':
				/* hex float not supported yet */
			case 'E':
			case 'G':
			case 'F':
				cnk->flags |= DP_F_UP;
			case 'a':
				/* hex float not supported yet */
			case 'e':
			case 'f':
			case 'g':
				cnk->type = CNK_FLOAT;
				break;
			case 'c':
				cnk->type = CNK_CHAR;
				break;
			case 's':
				cnk->type = CNK_STRING;
				break;
			case 'p':
				cnk->type = CNK_PTR;
				break;
			case 'n':
				cnk->type = CNK_NUM;
				break;
			case '%':
				cnk->type = CNK_PRCNT;
				break;
			default:
				/* Unknown, bail out*/
				goto done;
			}
			ch = *format++;
			state = DP_S_DEFAULT;
			break;
		case DP_S_DONE:
			break;
		default:
			/* hmm? */
			break; /* some picky compilers need this */
		}
	}

	/* retieve the format arguments */
	for (pnum = 0; pnum < max_pos; pnum++) {
		int i;

		if (clist[pnum].num == 0) {
			/* ignoring a parameter should not be permitted
			 * all parameters must be matched at least once
			 * BUT seem some system ignore this rule ...
			 * at least my glibc based system does --SSS
			 */
#ifdef DEBUG_SNPRINTF
			printf("parameter at position %d not used\n", pnum+1);
#endif
			/* eat the parameter */
			va_arg (args, int);
			continue;
		}
		for (i = 1; i < clist[pnum].num; i++) {
			if (clist[pnum].chunks[0]->type != clist[pnum].chunks[i]->type) {
				/* nooo noo no!
				 * all the references to a parameter
				 * must be of the same type
				 */
				goto done;
			}
		}
		cnk = clist[pnum].chunks[0];
		switch (cnk->type) {
		case CNK_INT:
			if (cnk->cflags == DP_C_SHORT) 
				cnk->value = va_arg (args, int);
			else if (cnk->cflags == DP_C_LONG)
				cnk->value = va_arg (args, long int);
			else if (cnk->cflags == DP_C_LLONG)
				cnk->value = va_arg (args, LLONG);
			else
				cnk->value = va_arg (args, int);

			for (i = 1; i < clist[pnum].num; i++) {
				clist[pnum].chunks[i]->value = cnk->value;
			}
			break;

		case CNK_OCTAL:
		case CNK_UINT:
		case CNK_HEX:
			if (cnk->cflags == DP_C_SHORT)
				cnk->value = va_arg (args, unsigned int);
			else if (cnk->cflags == DP_C_LONG)
				cnk->value = (unsigned long int)va_arg (args, unsigned long int);
			else if (cnk->cflags == DP_C_LLONG)
				cnk->value = (LLONG)va_arg (args, unsigned LLONG);
			else
				cnk->value = (unsigned int)va_arg (args, unsigned int);

			for (i = 1; i < clist[pnum].num; i++) {
				clist[pnum].chunks[i]->value = cnk->value;
			}
			break;

		case CNK_FLOAT:
			if (cnk->cflags == DP_C_LDOUBLE)
				cnk->fvalue = va_arg (args, LDOUBLE);
			else
				cnk->fvalue = va_arg (args, double);

			for (i = 1; i < clist[pnum].num; i++) {
				clist[pnum].chunks[i]->fvalue = cnk->fvalue;
			}
			break;

		case CNK_CHAR:
			cnk->value = va_arg (args, int);

			for (i = 1; i < clist[pnum].num; i++) {
				clist[pnum].chunks[i]->value = cnk->value;
			}
			break;

		case CNK_STRING:
			cnk->strvalue = va_arg (args, char *);
			if (!cnk->strvalue) cnk->strvalue = "(NULL)";

			for (i = 1; i < clist[pnum].num; i++) {
				clist[pnum].chunks[i]->strvalue = cnk->strvalue;
			}
			break;

		case CNK_PTR:
			cnk->strvalue = va_arg (args, void *);
			for (i = 1; i < clist[pnum].num; i++) {
				clist[pnum].chunks[i]->strvalue = cnk->strvalue;
			}
			break;

		case CNK_NUM:
			if (cnk->cflags == DP_C_CHAR)
				cnk->pnum = va_arg (args, char *);
			else if (cnk->cflags == DP_C_SHORT)
				cnk->pnum = va_arg (args, short int *);
			else if (cnk->cflags == DP_C_LONG)
				cnk->pnum = va_arg (args, long int *);
			else if (cnk->cflags == DP_C_LLONG)
				cnk->pnum = va_arg (args, LLONG *);
			else
				cnk->pnum = va_arg (args, int *);

			for (i = 1; i < clist[pnum].num; i++) {
				clist[pnum].chunks[i]->pnum = cnk->pnum;
			}
			break;

		case CNK_PRCNT:
			break;

		default:
			/* what ?? */
			goto done;
		}
	}
	/* print out the actual string from chunks */
	currlen = 0;
	cnk = chunks;
	while (cnk) {
		int len, min, max;

		if (cnk->min_star) min = cnk->min_star->value;
		else min = cnk->min;
		if (cnk->max_star) max = cnk->max_star->value;
		else max = cnk->max;

		switch (cnk->type) {

		case CNK_FMT_STR:
			if (maxlen != 0 && maxlen > currlen) {
				if (maxlen > (currlen + cnk->len)) len = cnk->len;
				else len = maxlen - currlen;

				memcpy(&(buffer[currlen]), &(base[cnk->start]), len);
			}
			currlen += cnk->len;
				
			break;

		case CNK_INT:
		case CNK_UINT:
			fmtint (buffer, &currlen, maxlen, cnk->value, 10, min, max, cnk->flags);
			break;

		case CNK_OCTAL:
			fmtint (buffer, &currlen, maxlen, cnk->value, 8, min, max, cnk->flags);
			break;

		case CNK_HEX:
			fmtint (buffer, &currlen, maxlen, cnk->value, 16, min, max, cnk->flags);
			break;

		case CNK_FLOAT:
			fmtfp (buffer, &currlen, maxlen, cnk->fvalue, min, max, cnk->flags);
			break;

		case CNK_CHAR:
			dopr_outch (buffer, &currlen, maxlen, cnk->value);
			break;

		case CNK_STRING:
			if (max == -1) {
				max = strlen(cnk->strvalue);
			}
			fmtstr (buffer, &currlen, maxlen, cnk->strvalue, cnk->flags, min, max);
			break;

		case CNK_PTR:
			fmtint (buffer, &currlen, maxlen, (long)(cnk->strvalue), 16, min, max, cnk->flags);
			break;

		case CNK_NUM:
			if (cnk->cflags == DP_C_CHAR)
				*((char *)(cnk->pnum)) = (char)currlen;
			else if (cnk->cflags == DP_C_SHORT)
				*((short int *)(cnk->pnum)) = (short int)currlen;
			else if (cnk->cflags == DP_C_LONG)
				*((long int *)(cnk->pnum)) = (long int)currlen;
			else if (cnk->cflags == DP_C_LLONG)
				*((LLONG *)(cnk->pnum)) = (LLONG)currlen;
			else
				*((int *)(cnk->pnum)) = (int)currlen;
			break;

		case CNK_PRCNT:
			dopr_outch (buffer, &currlen, maxlen, '%');
			break;

		default:
			/* what ?? */
			goto done;
		}
		cnk = cnk->next;
	}
	if (maxlen != 0) {
		if (currlen < maxlen - 1) 
			buffer[currlen] = '\0';
		else if (maxlen > 0) 
			buffer[maxlen - 1] = '\0';
	}
	ret = currlen;

done:
	while (chunks) {
		cnk = chunks->next;
		free(chunks);
		chunks = cnk;
	}
	if (clist) {
		for (pnum = 0; pnum < max_pos; pnum++) {
			if (clist[pnum].chunks) free(clist[pnum].chunks);
		}
		free(clist);
	}
	return ret;
}

static void fmtstr(char *buffer, size_t *currlen, size_t maxlen,
		    char *value, int flags, int min, int max)
{
	int padlen, strln;     /* amount to pad */
	int cnt = 0;

#ifdef DEBUG_SNPRINTF
	printf("fmtstr min=%d max=%d s=[%s]\n", min, max, value);
#endif
	if (value == 0) {
		value = "<NULL>";
	}

	for (strln = 0; strln < max && value[strln]; ++strln); /* strlen */
	padlen = min - strln;
	if (padlen < 0) 
		padlen = 0;
	if (flags & DP_F_MINUS) 
		padlen = -padlen; /* Left Justify */
	
	while (padlen > 0) {
		dopr_outch (buffer, currlen, maxlen, ' ');
		--padlen;
	}
	while (*value && (cnt < max)) {
		dopr_outch (buffer, currlen, maxlen, *value++);
		++cnt;
	}
	while (padlen < 0) {
		dopr_outch (buffer, currlen, maxlen, ' ');
		++padlen;
	}
}

/* Have to handle DP_F_NUM (ie 0x and 0 alternates) */

static void fmtint(char *buffer, size_t *currlen, size_t maxlen,
		    LLONG value, int base, int min, int max, int flags)
{
	int signvalue = 0;
	unsigned LLONG uvalue;
	char convert[20];
	int place = 0;
	int spadlen = 0; /* amount to space pad */
	int zpadlen = 0; /* amount to zero pad */
	int caps = 0;
	
	if (max < 0)
		max = 0;
	
	uvalue = value;
	
	if(!(flags & DP_F_UNSIGNED)) {
		if( value < 0 ) {
			signvalue = '-';
			uvalue = -value;
		} else {
			if (flags & DP_F_PLUS)  /* Do a sign (+/i) */
				signvalue = '+';
			else if (flags & DP_F_SPACE)
				signvalue = ' ';
		}
	}
  
	if (flags & DP_F_UP) caps = 1; /* Should characters be upper case? */

	do {
		convert[place++] =
			(caps? "0123456789ABCDEF":"0123456789abcdef")
			[uvalue % (unsigned)base  ];
		uvalue = (uvalue / (unsigned)base );
	} while(uvalue && (place < 20));
	if (place == 20) place--;
	convert[place] = 0;

	zpadlen = max - place;
	spadlen = min - MAX (max, place) - (signvalue ? 1 : 0);
	if (zpadlen < 0) zpadlen = 0;
	if (spadlen < 0) spadlen = 0;
	if (flags & DP_F_ZERO) {
		zpadlen = MAX(zpadlen, spadlen);
		spadlen = 0;
	}
	if (flags & DP_F_MINUS) 
		spadlen = -spadlen; /* Left Justifty */

#ifdef DEBUG_SNPRINTF
	printf("zpad: %d, spad: %d, min: %d, max: %d, place: %d\n",
	       zpadlen, spadlen, min, max, place);
#endif

	/* Spaces */
	while (spadlen > 0) {
		dopr_outch (buffer, currlen, maxlen, ' ');
		--spadlen;
	}

	/* Sign */
	if (signvalue) 
		dopr_outch (buffer, currlen, maxlen, signvalue);

	/* Zeros */
	if (zpadlen > 0) {
		while (zpadlen > 0) {
			dopr_outch (buffer, currlen, maxlen, '0');
			--zpadlen;
		}
	}

	/* Digits */
	while (place > 0) 
		dopr_outch (buffer, currlen, maxlen, convert[--place]);
  
	/* Left Justified spaces */
	while (spadlen < 0) {
		dopr_outch (buffer, currlen, maxlen, ' ');
		++spadlen;
	}
}

static LDOUBLE abs_val(LDOUBLE value)
{
	LDOUBLE result = value;

	if (value < 0)
		result = -value;
	
	return result;
}

static LDOUBLE POW10(int exp)
{
	LDOUBLE result = 1;
	
	while (exp) {
		result *= 10;
		exp--;
	}
  
	return result;
}

static LLONG ROUND(LDOUBLE value)
{
	LLONG intpart;

	intpart = (LLONG)value;
	value = value - intpart;
	if (value >= 0.5) intpart++;
	
	return intpart;
}

/* a replacement for modf that doesn't need the math library. Should
   be portable, but slow */
static double my_modf(double x0, double *iptr)
{
	int i;
	LLONG l;
	double x = x0;
	double f = 1.0;

	for (i=0;i<100;i++) {
		l = (long)x;
		if (l <= (x+1) && l >= (x-1)) break;
		x *= 0.1;
		f *= 10.0;
	}

	if (i == 100) {
		/* yikes! the number is beyond what we can handle. What do we do? */
		(*iptr) = 0;
		return 0;
	}

	if (i != 0) {
		double i2;
		double ret;

		ret = my_modf(x0-l*f, &i2);
		(*iptr) = l*f + i2;
		return ret;
	} 

	(*iptr) = l;
	return x - (*iptr);
}


static void fmtfp (char *buffer, size_t *currlen, size_t maxlen,
		   LDOUBLE fvalue, int min, int max, int flags)
{
	int signvalue = 0;
	double ufvalue;
	char iconvert[311];
	char fconvert[311];
	int iplace = 0;
	int fplace = 0;
	int padlen = 0; /* amount to pad */
	int zpadlen = 0; 
	int caps = 0;
	int idx;
	double intpart;
	double fracpart;
	double temp;
  
	/* 
	 * AIX manpage says the default is 0, but Solaris says the default
	 * is 6, and sprintf on AIX defaults to 6
	 */
	if (max < 0)
		max = 6;

	ufvalue = abs_val (fvalue);

	if (fvalue < 0) {
		signvalue = '-';
	} else {
		if (flags & DP_F_PLUS) { /* Do a sign (+/i) */
			signvalue = '+';
		} else {
			if (flags & DP_F_SPACE)
				signvalue = ' ';
		}
	}

#if 0
	if (flags & DP_F_UP) caps = 1; /* Should characters be upper case? */
#endif

#if 0
	 if (max == 0) ufvalue += 0.5; /* if max = 0 we must round */
#endif

	/* 
	 * Sorry, we only support 9 digits past the decimal because of our 
	 * conversion method
	 */
	if (max > 9)
		max = 9;

	/* We "cheat" by converting the fractional part to integer by
	 * multiplying by a factor of 10
	 */

	temp = ufvalue;
	my_modf(temp, &intpart);

	fracpart = ROUND((POW10(max)) * (ufvalue - intpart));
	
	if (fracpart >= POW10(max)) {
		intpart++;
		fracpart -= POW10(max);
	}


	/* Convert integer part */
	do {
		temp = intpart*0.1;
		my_modf(temp, &intpart);
		idx = (int) ((temp -intpart +0.05)* 10.0);
		/* idx = (int) (((double)(temp*0.1) -intpart +0.05) *10.0); */
		/* printf ("%llf, %f, %x\n", temp, intpart, idx); */
		iconvert[iplace++] =
			(caps? "0123456789ABCDEF":"0123456789abcdef")[idx];
	} while (intpart && (iplace < 311));
	if (iplace == 311) iplace--;
	iconvert[iplace] = 0;

	/* Convert fractional part */
	if (fracpart)
	{
		do {
			temp = fracpart*0.1;
			my_modf(temp, &fracpart);
			idx = (int) ((temp -fracpart +0.05)* 10.0);
			/* idx = (int) ((((temp/10) -fracpart) +0.05) *10); */
			/* printf ("%lf, %lf, %ld\n", temp, fracpart, idx ); */
			fconvert[fplace++] =
			(caps? "0123456789ABCDEF":"0123456789abcdef")[idx];
		} while(fracpart && (fplace < 311));
		if (fplace == 311) fplace--;
	}
	fconvert[fplace] = 0;
  
	/* -1 for decimal point, another -1 if we are printing a sign */
	padlen = min - iplace - max - 1 - ((signvalue) ? 1 : 0); 
	zpadlen = max - fplace;
	if (zpadlen < 0) zpadlen = 0;
	if (padlen < 0) 
		padlen = 0;
	if (flags & DP_F_MINUS) 
		padlen = -padlen; /* Left Justifty */
	
	if ((flags & DP_F_ZERO) && (padlen > 0)) {
		if (signvalue) {
			dopr_outch (buffer, currlen, maxlen, signvalue);
			--padlen;
			signvalue = 0;
		}
		while (padlen > 0) {
			dopr_outch (buffer, currlen, maxlen, '0');
			--padlen;
		}
	}
	while (padlen > 0) {
		dopr_outch (buffer, currlen, maxlen, ' ');
		--padlen;
	}
	if (signvalue) 
		dopr_outch (buffer, currlen, maxlen, signvalue);
	
	while (iplace > 0) 
		dopr_outch (buffer, currlen, maxlen, iconvert[--iplace]);

#ifdef DEBUG_SNPRINTF
	printf("fmtfp: fplace=%d zpadlen=%d\n", fplace, zpadlen);
#endif

	/*
	 * Decimal point.  This should probably use locale to find the correct
	 * char to print out.
	 */
	if (max > 0) {
		dopr_outch (buffer, currlen, maxlen, '.');
		
		while (zpadlen > 0) {
			dopr_outch (buffer, currlen, maxlen, '0');
			--zpadlen;
		}

		while (fplace > 0) 
			dopr_outch (buffer, currlen, maxlen, fconvert[--fplace]);
	}

	while (padlen < 0) {
		dopr_outch (buffer, currlen, maxlen, ' ');
		++padlen;
	}
}

static void dopr_outch(char *buffer, size_t *currlen, size_t maxlen, char c)
{
	if (*currlen < maxlen) {
		buffer[(*currlen)] = c;
	}
	(*currlen)++;
}

static struct pr_chunk *new_chunk(void) {
	struct pr_chunk *new_c = (struct pr_chunk *)malloc(sizeof(struct pr_chunk));

	if (!new_c)
		return NULL;

	new_c->type = 0;
	new_c->num = 0;
	new_c->min = 0;
	new_c->min_star = NULL;
	new_c->max = -1;
	new_c->max_star = NULL;
	new_c->flags = 0;
	new_c->cflags = 0;
	new_c->start = 0;
	new_c->len = 0;
	new_c->value = 0;
	new_c->fvalue = 0;
	new_c->strvalue = NULL;
	new_c->pnum = NULL;
	new_c->next = NULL;

	return new_c;
}

static int add_cnk_list_entry(struct pr_chunk_x **list,
				int max_num, struct pr_chunk *chunk) {
	struct pr_chunk_x *l;
	struct pr_chunk **c;
	int max;
	int cnum;
	int i, pos;

	if (chunk->num > max_num) {
		max = chunk->num;
	
		if (*list == NULL) {
			l = (struct pr_chunk_x *)malloc(sizeof(struct pr_chunk_x) * max);
			pos = 0;
		} else {
			l = (struct pr_chunk_x *)realloc(*list, sizeof(struct pr_chunk_x) * max);
			pos = max_num;
		}
		if (l == NULL) {
			for (i = 0; i < max; i++) {
				if ((*list)[i].chunks) free((*list)[i].chunks);
			}
			return 0;
		}
		for (i = pos; i < max; i++) {
			l[i].chunks = NULL;
			l[i].num = 0;
		}
	} else {
		l = *list;
		max = max_num;
	}

	i = chunk->num - 1;
	cnum = l[i].num + 1;
	if (l[i].chunks == NULL) {
		c = (struct pr_chunk **)malloc(sizeof(struct pr_chunk *) * cnum); 
	} else {
		c = (struct pr_chunk **)realloc(l[i].chunks, sizeof(struct pr_chunk *) * cnum);
	}
	if (c == NULL) {
		for (i = 0; i < max; i++) {
			if (l[i].chunks) free(l[i].chunks);
		}
		return 0;
	}
	c[l[i].num] = chunk;
	l[i].chunks = c;
	l[i].num = cnum;

	*list = l;
	return max;
}

 int smb_vsnprintf (char *str, size_t count, const char *fmt, va_list args)
{
	return dopr(str, count, fmt, args);
}
#define vsnprintf smb_vsnprintf
#endif

/* yes this really must be a ||. Don't muck with this (tridge)
 *
 * The logic for these two is that we need our own definition if the
 * OS *either* has no definition of *sprintf, or if it does have one
 * that doesn't work properly according to the autoconf test.
 */
#if !defined(HAVE_SNPRINTF) || !defined(HAVE_C99_VSNPRINTF)
int smb_snprintf(char *str,size_t count,const char *fmt,...)
{
	size_t ret;
	va_list ap;
    
	va_start(ap, fmt);
	ret = vsnprintf(str, count, fmt, ap);
	va_end(ap);
	return ret;
}
#define snprintf smb_snprintf
#endif

#endif 

#ifndef HAVE_VASPRINTF
 int vasprintf(char **ptr, const char *format, va_list ap)
{
	int ret;
	va_list ap2;

	VA_COPY(ap2, ap);
	
	ret = vsnprintf(NULL, 0, format, ap2);
	if (ret <= 0) return ret;

	(*ptr) = (char *)malloc(ret+1);
	if (!*ptr) return -1;

	VA_COPY(ap2, ap);

	ret = vsnprintf(*ptr, ret+1, format, ap2);

	return ret;
}
#endif


#ifndef HAVE_ASPRINTF
 int asprintf(char **ptr, const char *format, ...)
{
	va_list ap;
	int ret;
	
	*ptr = NULL;
	va_start(ap, format);
	ret = vasprintf(ptr, format, ap);
	va_end(ap);

	return ret;
}
#endif

#ifdef TEST_SNPRINTF

 int sprintf(char *str,const char *fmt,...);

 int main (void)
{
	char buf1[1024];
	char buf2[1024];
	char *buf3;
	char *fp_fmt[] = {
		"%1.1f",
		"%-1.5f",
		"%1.5f",
		"%123.9f",
		"%10.5f",
		"% 10.5f",
		"%+22.9f",
		"%+4.9f",
		"%01.3f",
		"%4f",
		"%3.1f",
		"%3.2f",
		"%.0f",
		"%f",
		"%-8.8f",
		"%-9.9f",
		NULL
	};
	double fp_nums[] = { 6442452944.1234, -1.5, 134.21, 91340.2, 341.1234, 203.9, 0.96, 0.996, 
			     0.9996, 1.996, 4.136, 5.030201, 0.00205,
			     /* END LIST */ 0};
	char *int_fmt[] = {
		"%-1.5d",
		"%1.5d",
		"%123.9d",
		"%5.5d",
		"%10.5d",
		"% 10.5d",
		"%+22.33d",
		"%01.3d",
		"%4d",
		"%d",
		NULL
	};
	long int_nums[] = { -1, 134, 91340, 341, 0203, 1234567890, 0};
	char *str_fmt[] = {
		"%10.5s",
		"%-10.5s",
		"%5.10s",
		"%-5.10s",
		"%10.1s",
		"%0.10s",
		"%10.0s",
		"%1.10s",
		"%s",
		"%.1s",
		"%.10s",
		"%10s",
		NULL
	};
	char *str_vals[] = {"hello", "a", "", "a longer string", NULL};
#ifdef HAVE_LONG_LONG
	char *ll_fmt[] = {
		"%llu",
		NULL
	};
	LLONG ll_nums[] = { 134, 91340, 341, 0203, 1234567890, 128006186140000000LL, 0};
#endif
	int x, y;
	int fail = 0;
	int num = 0;
	int l1, l2;

	printf ("Testing snprintf format codes against system sprintf...\n");

	for (x = 0; fp_fmt[x] ; x++) {
		for (y = 0; fp_nums[y] != 0 ; y++) {
			buf1[0] = buf2[0] = '\0';
			l1 = snprintf(NULL, 0, fp_fmt[x], fp_nums[y]);
			l2 = sprintf(buf1, fp_fmt[x], fp_nums[y]);
			sprintf (buf2, fp_fmt[x], fp_nums[y]);
			buf1[1023] = buf2[1023] = '\0';
			if (strcmp (buf1, buf2) || (l1 != l2)) {
				printf("snprintf doesn't match Format: %s\n\tsnprintf(%d) = [%s]\n\t sprintf(%d) = [%s]\n", 
				       fp_fmt[x], l1, buf1, l2, buf2);
				fail++;
			}
			num++;
		}
	}

	for (x = 0; int_fmt[x] ; x++) {
		for (y = 0; int_nums[y] != 0 ; y++) {
			buf1[0] = buf2[0] = '\0';
			l1 = snprintf(NULL, 0, int_fmt[x], int_nums[y]);
			l2 = sprintf(buf1, int_fmt[x], int_nums[y]);
			sprintf (buf2, int_fmt[x], int_nums[y]);
			buf1[1023] = buf2[1023] = '\0';
			if (strcmp (buf1, buf2) || (l1 != l2)) {
				printf("snprintf doesn't match Format: %s\n\tsnprintf(%d) = [%s]\n\t sprintf(%d) = [%s]\n", 
				       int_fmt[x], l1, buf1, l2, buf2);
				fail++;
			}
			num++;
		}
	}

	for (x = 0; str_fmt[x] ; x++) {
		for (y = 0; str_vals[y] != 0 ; y++) {
			buf1[0] = buf2[0] = '\0';
			l1 = snprintf(NULL, 0, str_fmt[x], str_vals[y]);
			l2 = sprintf(buf1, str_fmt[x], str_vals[y]);
			sprintf (buf2, str_fmt[x], str_vals[y]);
			buf1[1023] = buf2[1023] = '\0';
			if (strcmp (buf1, buf2) || (l1 != l2)) {
				printf("snprintf doesn't match Format: %s\n\tsnprintf(%d) = [%s]\n\t sprintf(%d) = [%s]\n", 
				       str_fmt[x], l1, buf1, l2, buf2);
				fail++;
			}
			num++;
		}
	}

#ifdef HAVE_LONG_LONG
	for (x = 0; ll_fmt[x] ; x++) {
		for (y = 0; ll_nums[y] != 0 ; y++) {
			buf1[0] = buf2[0] = '\0';
			l1 = snprintf(NULL, 0, ll_fmt[x], ll_nums[y]);
			l2 = sprintf(buf1, ll_fmt[x], ll_nums[y]);
			sprintf (buf2, ll_fmt[x], ll_nums[y]);
			buf1[1023] = buf2[1023] = '\0';
			if (strcmp (buf1, buf2) || (l1 != l2)) {
				printf("snprintf doesn't match Format: %s\n\tsnprintf(%d) = [%s]\n\t sprintf(%d) = [%s]\n", 
				       ll_fmt[x], l1, buf1, l2, buf2);
				fail++;
			}
			num++;
		}
	}
#endif

#define BUFSZ 2048

	buf1[0] = buf2[0] = '\0';
	if ((buf3 = malloc(BUFSZ)) == NULL) {
		fail++;
	} else {
		num++;
		memset(buf3, 'a', BUFSZ);
		snprintf(buf1, sizeof(buf1), "%.*s", 1, buf3);
		buf1[1023] = '\0';
		if (strcmp(buf1, "a") != 0) {
			printf("length limit buf1 '%s' expected 'a'\n", buf1);
			fail++;
		}
        }

	buf1[0] = buf2[0] = '\0';
	l1 = snprintf(buf1, sizeof(buf1), "%4$*1$d %2$s %3$*1$.*1$f", 3, "pos test", 12.3456, 9);
	l2 = sprintf(buf2, "%4$*1$d %2$s %3$*1$.*1$f", 3, "pos test", 12.3456, 9);
	buf1[1023] = buf2[1023] = '\0';
	if (strcmp(buf1, buf2) || (l1 != l2)) {
		printf("snprintf doesn't match Format: %s\n\tsnprintf(%d) = [%s]\n\t sprintf(%d) = [%s]\n",
				"%4$*1$d %2$s %3$*1$.*1$f", l1, buf1, l2, buf2);
		fail++;
	}

	buf1[0] = buf2[0] = '\0';
	l1 = snprintf(buf1, sizeof(buf1), "%4$*4$d %2$s %3$*4$.*4$f", 3, "pos test", 12.3456, 9);
	l2 = sprintf(buf2, "%4$*4$d %2$s %3$*4$.*4$f", 3, "pos test", 12.3456, 9);
	buf1[1023] = buf2[1023] = '\0';
	if (strcmp(buf1, buf2)) {
		printf("snprintf doesn't match Format: %s\n\tsnprintf(%d) = [%s]\n\t sprintf(%d) = [%s]\n",
				"%4$*1$d %2$s %3$*1$.*1$f", l1, buf1, l2, buf2);
		fail++;
	}
#if 0
	buf1[0] = buf2[0] = '\0';
	l1 = snprintf(buf1, sizeof(buf1), "%lld", (LLONG)1234567890);
	l2 = sprintf(buf2, "%lld", (LLONG)1234567890);
	buf1[1023] = buf2[1023] = '\0';
	if (strcmp(buf1, buf2)) {
		printf("snprintf doesn't match Format: %s\n\tsnprintf(%d) = [%s]\n\t sprintf(%d) = [%s]\n",
				"%lld", l1, buf1, l2, buf2);
		fail++;
	}

	buf1[0] = buf2[0] = '\0';
	l1 = snprintf(buf1, sizeof(buf1), "%Lf", (LDOUBLE)890.1234567890123);
	l2 = sprintf(buf2, "%Lf", (LDOUBLE)890.1234567890123);
	buf1[1023] = buf2[1023] = '\0';
	if (strcmp(buf1, buf2)) {
		printf("snprintf doesn't match Format: %s\n\tsnprintf(%d) = [%s]\n\t sprintf(%d) = [%s]\n",
				"%Lf", l1, buf1, l2, buf2);
		fail++;
	}
#endif
	printf ("%d tests failed out of %d.\n", fail, num);

	printf("seeing how many digits we support\n");
	{
		double v0 = 0.12345678901234567890123456789012345678901;
		for (x=0; x<100; x++) {
			double p = pow(10, x); 
			double r = v0*p;
			snprintf(buf1, sizeof(buf1), "%1.1f", r);
			sprintf(buf2,                "%1.1f", r);
			if (strcmp(buf1, buf2)) {
				printf("we seem to support %d digits\n", x-1);
				break;
			}
		}
	}

	return 0;
}
#endif /* TEST_SNPRINTF */
