/** \ingroup popt
 * \file popt/poptparse.c
 */

/* (C) 1998-2002 Red Hat, Inc. -- Licensing details are in the COPYING
   file accompanying popt source distributions, available from 
   ftp://ftp.rpm.org/pub/rpm/dist. */

#include "system.h"

#define POPT_ARGV_ARRAY_GROW_DELTA 5

/*@-boundswrite@*/
int poptDupArgv(int argc, const char **argv,
		int * argcPtr, const char *** argvPtr)
{
    size_t nb = (argc + 1) * sizeof(*argv);
    const char ** argv2;
    char * dst;
    int i;

    if (argc <= 0 || argv == NULL)	/* XXX can't happen */
	return POPT_ERROR_NOARG;
    for (i = 0; i < argc; i++) {
	if (argv[i] == NULL)
	    return POPT_ERROR_NOARG;
	nb += strlen(argv[i]) + 1;
    }
	
    dst = malloc(nb);
    if (dst == NULL)			/* XXX can't happen */
	return POPT_ERROR_MALLOC;
    argv2 = (void *) dst;
    dst += (argc + 1) * sizeof(*argv);

    /*@-branchstate@*/
    for (i = 0; i < argc; i++) {
	argv2[i] = dst;
	dst += strlen(strcpy(dst, argv[i])) + 1;
    }
    /*@=branchstate@*/
    argv2[argc] = NULL;

    if (argvPtr) {
	*argvPtr = argv2;
    } else {
	free(argv2);
	argv2 = NULL;
    }
    if (argcPtr)
	*argcPtr = argc;
    return 0;
}
/*@=boundswrite@*/

/*@-bounds@*/
int poptParseArgvString(const char * s, int * argcPtr, const char *** argvPtr)
{
    const char * src;
    char quote = '\0';
    int argvAlloced = POPT_ARGV_ARRAY_GROW_DELTA;
    const char ** argv = malloc(sizeof(*argv) * argvAlloced);
    int argc = 0;
    int buflen = strlen(s) + 1;
    char * buf = memset(alloca(buflen), 0, buflen);
    int rc = POPT_ERROR_MALLOC;

    if (argv == NULL) return rc;
    argv[argc] = buf;

    for (src = s; *src != '\0'; src++) {
	if (quote == *src) {
	    quote = '\0';
	} else if (quote != '\0') {
	    if (*src == '\\') {
		src++;
		if (!*src) {
		    rc = POPT_ERROR_BADQUOTE;
		    goto exit;
		}
		if (*src != quote) *buf++ = '\\';
	    }
	    *buf++ = *src;
	} else if (isspace(*src)) {
	    if (*argv[argc] != '\0') {
		buf++, argc++;
		if (argc == argvAlloced) {
		    argvAlloced += POPT_ARGV_ARRAY_GROW_DELTA;
		    argv = realloc(argv, sizeof(*argv) * argvAlloced);
		    if (argv == NULL) goto exit;
		}
		argv[argc] = buf;
	    }
	} else switch (*src) {
	  case '"':
	  case '\'':
	    quote = *src;
	    /*@switchbreak@*/ break;
	  case '\\':
	    src++;
	    if (!*src) {
		rc = POPT_ERROR_BADQUOTE;
		goto exit;
	    }
	    /*@fallthrough@*/
	  default:
	    *buf++ = *src;
	    /*@switchbreak@*/ break;
	}
    }

    if (strlen(argv[argc])) {
	argc++, buf++;
    }

    rc = poptDupArgv(argc, argv, argcPtr, argvPtr);

exit:
    if (argv) free(argv);
    return rc;
}
/*@=bounds@*/

/* still in the dev stage.
 * return values, perhaps 1== file erro
 * 2== line to long
 * 3== umm.... more?
 */
int poptConfigFileToString(FILE *fp, char ** argstrp, /*@unused@*/ int flags)
{
    char line[999];
    char * argstr;
    char * p;
    char * q;
    char * x;
    int t;
    int argvlen = 0;
    size_t maxlinelen = sizeof(line);
    size_t linelen;
    int maxargvlen = 480;
    int linenum = 0;

    *argstrp = NULL;

    /*   |   this_is   =   our_line
     *	     p             q      x
     */

    if (fp == NULL)
	return POPT_ERROR_NULLARG;

    argstr = calloc(maxargvlen, sizeof(*argstr));
    if (argstr == NULL) return POPT_ERROR_MALLOC;

    while (fgets(line, (int)maxlinelen, fp) != NULL) {
	linenum++;
	p = line;

	/* loop until first non-space char or EOL */
	while( *p != '\0' && isspace(*p) )
	    p++;

	linelen = strlen(p);
	if (linelen >= maxlinelen-1)
	    return POPT_ERROR_OVERFLOW;	/* XXX line too long */

	if (*p == '\0' || *p == '\n') continue;	/* line is empty */
	if (*p == '#') continue;		/* comment line */

	q = p;

	while (*q != '\0' && (!isspace(*q)) && *q != '=')
	    q++;

	if (isspace(*q)) {
	    /* a space after the name, find next non space */
	    *q++='\0';
	    while( *q != '\0' && isspace((int)*q) ) q++;
	}
	if (*q == '\0') {
	    /* single command line option (ie, no name=val, just name) */
	    q[-1] = '\0';		/* kill off newline from fgets() call */
	    argvlen += (t = q - p) + (sizeof(" --")-1);
	    if (argvlen >= maxargvlen) {
		maxargvlen = (t > maxargvlen) ? t*2 : maxargvlen*2;
		argstr = realloc(argstr, maxargvlen);
		if (argstr == NULL) return POPT_ERROR_MALLOC;
	    }
	    strcat(argstr, " --");
	    strcat(argstr, p);
	    continue;
	}
	if (*q != '=')
	    continue;	/* XXX for now, silently ignore bogus line */

	/* *q is an equal sign. */
	*q++ = '\0';

	/* find next non-space letter of value */
	while (*q != '\0' && isspace(*q))
	    q++;
	if (*q == '\0')
	    continue;	/* XXX silently ignore missing value */

	/* now, loop and strip all ending whitespace */
	x = p + linelen;
	while (isspace(*--x))
	    *x = 0;	/* null out last char if space (including fgets() NL) */

	/* rest of line accept */
	t = x - p;
	argvlen += t + (sizeof("' --='")-1);
	if (argvlen >= maxargvlen) {
	    maxargvlen = (t > maxargvlen) ? t*2 : maxargvlen*2;
	    argstr = realloc(argstr, maxargvlen);
	    if (argstr == NULL) return POPT_ERROR_MALLOC;
	}
	strcat(argstr, " --");
	strcat(argstr, p);
	strcat(argstr, "=\"");
	strcat(argstr, q);
	strcat(argstr, "\"");
    }

    *argstrp = argstr;
    return 0;
}
