/** \ingroup popt
 * \file popt/findme.h
 */

/* (C) 1998-2000 Red Hat, Inc. -- Licensing details are in the COPYING
   file accompanying popt source distributions, available from 
   ftp://ftp.rpm.org/pub/rpm/dist. */

#ifndef H_FINDME
#define H_FINDME

/**
 * Return absolute path to executable by searching PATH.
 * @param argv0		name of executable
 * @return		(malloc'd) absolute path to executable (or NULL)
 */
/*@null@*/ const char * findProgramPath(/*@null@*/ const char * argv0)
	/*@*/;

#endif
