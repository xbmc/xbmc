/* (C) 1998 Red Hat Software, Inc. -- Licensing details are in the COPYING
   file accompanying popt source distributions, available from 
   ftp://ftp.redhat.com/pub/code/popt */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "popt.h"
#define alloca malloc
int poptParseArgvString(char * s, int * argcPtr, char *** argvPtr) {
    char * buf = strcpy(alloca(strlen(s) + 1), s);
    char * bufStart = buf;
    char * src, * dst;
    char quote = '\0';
    int argvAlloced = 5;
    char ** argv = malloc(sizeof(*argv) * argvAlloced);
    char ** argv2;
    int argc = 0;
    int i;

    src = s;
    dst = buf;
    argv[argc] = buf;

    memset(buf, '\0', strlen(s) + 1);

    while (*src) {
	if (quote == *src) {
	    quote = '\0';
	} else if (quote) {
	    if (*src == '\\') {
		src++;
		if (!*src) {
		    free(argv);
		    return POPT_ERROR_BADQUOTE;
		}
		if (*src != quote) *buf++ = '\\';
	    }
	    *buf++ = *src;
	} else if (isspace(*src)) {
	    if (*argv[argc]) {
		buf++, argc++;
		if (argc == argvAlloced) {
		    argvAlloced += 5;
		    argv = realloc(argv, sizeof(*argv) * argvAlloced);
		}
		argv[argc] = buf;
	    }
	} else switch (*src) {
	  case '"':
	  case '\'':
	    quote = *src;
	    break;
	  case '\\':
	    src++;
	    if (!*src) {
		free(argv);
		return POPT_ERROR_BADQUOTE;
	    }
	    /* fallthrough */
	  default:
	    *buf++ = *src;
	}

	src++;
    }

    if (strlen(argv[argc])) {
	argc++, buf++;
    }

    dst = malloc(argc * sizeof(*argv) + (buf - bufStart));
    argv2 = (void *) dst;
    dst += argc * sizeof(*argv);
    memcpy(argv2, argv, argc * sizeof(*argv));
    memcpy(dst, bufStart, buf - bufStart);

    for (i = 0; i < argc; i++) {
	argv2[i] = dst + (argv[i] - bufStart);
    }

    free(argv);

    *argvPtr = argv2;
    *argcPtr = argc;

    return 0;
}
