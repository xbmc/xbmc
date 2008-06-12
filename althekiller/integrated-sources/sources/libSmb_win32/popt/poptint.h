/** \ingroup popt
 * \file popt/poptint.h
 */

/* (C) 1998-2000 Red Hat, Inc. -- Licensing details are in the COPYING
   file accompanying popt source distributions, available from 
   ftp://ftp.rpm.org/pub/rpm/dist. */

#ifndef H_POPTINT
#define H_POPTINT

/**
 * Wrapper to free(3), hides const compilation noise, permit NULL, return NULL.
 * @param p		memory to free
 * @retval		NULL always
 */
/*@unused@*/ static inline /*@null@*/ void *
_free(/*@only@*/ /*@null@*/ const void * p)
	/*@modifies p @*/
{
    if (p != NULL)	free((void *)p);
    return NULL;
}

/* Bit mask macros. */
/*@-exporttype -redef @*/
typedef	unsigned int __pbm_bits;
/*@=exporttype =redef @*/
#define	__PBM_NBITS		(8 * sizeof (__pbm_bits))
#define	__PBM_IX(d)		((d) / __PBM_NBITS)
#define __PBM_MASK(d)		((__pbm_bits) 1 << (((unsigned)(d)) % __PBM_NBITS))
/*@-exporttype -redef @*/
typedef struct {
    __pbm_bits bits[1];
} pbm_set;
/*@=exporttype =redef @*/
#define	__PBM_BITS(set)	((set)->bits)

#define	PBM_ALLOC(d)	calloc(__PBM_IX (d) + 1, sizeof(__pbm_bits))
#define	PBM_FREE(s)	_free(s);
#define PBM_SET(d, s)   (__PBM_BITS (s)[__PBM_IX (d)] |= __PBM_MASK (d))
#define PBM_CLR(d, s)   (__PBM_BITS (s)[__PBM_IX (d)] &= ~__PBM_MASK (d))
#define PBM_ISSET(d, s) ((__PBM_BITS (s)[__PBM_IX (d)] & __PBM_MASK (d)) != 0)

struct optionStackEntry {
    int argc;
/*@only@*/ /*@null@*/
    const char ** argv;
/*@only@*/ /*@null@*/
    pbm_set * argb;
    int next;
/*@only@*/ /*@null@*/
    const char * nextArg;
/*@observer@*/ /*@null@*/
    const char * nextCharArg;
/*@dependent@*/ /*@null@*/
    poptItem currAlias;
    int stuffed;
};

struct poptContext_s {
    struct optionStackEntry optionStack[POPT_OPTION_DEPTH];
/*@dependent@*/
    struct optionStackEntry * os;
/*@owned@*/ /*@null@*/
    const char ** leftovers;
    int numLeftovers;
    int nextLeftover;
/*@keep@*/
    const struct poptOption * options;
    int restLeftover;
/*@only@*/ /*@null@*/
    const char * appName;
/*@only@*/ /*@null@*/
    poptItem aliases;
    int numAliases;
    int flags;
/*@owned@*/ /*@null@*/
    poptItem execs;
    int numExecs;
/*@only@*/ /*@null@*/
    const char ** finalArgv;
    int finalArgvCount;
    int finalArgvAlloced;
/*@dependent@*/ /*@null@*/
    poptItem doExec;
/*@only@*/
    const char * execPath;
    int execAbsolute;
/*@only@*/
    const char * otherHelp;
/*@null@*/
    pbm_set * arg_strip;
};

#ifdef HAVE_LIBINTL_H
#include <libintl.h>
#endif

#if defined(HAVE_GETTEXT) && !defined(__LCLINT__)
#define _(foo) gettext(foo)
#else
#define _(foo) foo
#endif

#if defined(HAVE_DCGETTEXT) && !defined(__LCLINT__)
#define D_(dom, str) dgettext(dom, str)
#define POPT_(foo) D_("popt", foo)
#else
#define D_(dom, str) str
#define POPT_(foo) foo
#endif

#define N_(foo) foo

#endif
