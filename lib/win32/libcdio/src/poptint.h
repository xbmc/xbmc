/* (C) 1998 Red Hat Software, Inc. -- Licensing details are in the COPYING
   file accompanying popt source distributions, available from 
   ftp://ftp.redhat.com/pub/code/popt */

#ifndef H_POPTINT
#define H_POPTINT

struct optionStackEntry {
    int argc;
    char ** argv;
    int next;
    char * nextArg;
    char * nextCharArg;
    struct poptAlias * currAlias;
    int stuffed;
};

struct execEntry {
    char * longName;
    char shortName;
    char * script;
};

struct poptContext_s {
    struct optionStackEntry optionStack[POPT_OPTION_DEPTH], * os;
    char ** leftovers;
    int numLeftovers;
    int nextLeftover;
    const struct poptOption * options;
    int restLeftover;
    char * appName;
    struct poptAlias * aliases;
    int numAliases;
    int flags;
    struct execEntry * execs;
    int numExecs;
    char ** finalArgv;
    int finalArgvCount;
    int finalArgvAlloced;
    struct execEntry * doExec;
    char * execPath;
    int execAbsolute;
    char * otherHelp;
};

#ifdef HAVE_LIBINTL_H
#include <libintl.h>
#endif

#ifdef HAVE_GETTEXT
#define _(foo) gettext(foo)
#else
#define _(foo) (foo)
#endif

#ifdef HAVE_DGETTEXT
#define POPT_(foo) dgettext("popt", foo)
#else
#define POPT_(foo) (foo)
#endif

#define N_(foo) (foo)

#endif
