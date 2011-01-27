// Place the code and data below here into the LIBXBMS section.
#ifndef __GNUC__
#pragma code_seg( "LIBXBMS" )
#pragma data_seg( "LIBXBMS_RW" )
#pragma bss_seg( "LIBXBMS_RW" )
#pragma const_seg( "LIBXBMS_RD" )
#pragma comment(linker, "/merge:LIBXBMS_RW=LIBXBMS")
#pragma comment(linker, "/merge:LIBXBMS_RD=LIBXBMS")
#endif
/*   -*- c -*-
 * 
 *  ----------------------------------------------------------------------
 *  Misc utilities.
 *  ----------------------------------------------------------------------
 *
 *  Copyright (c) 2002-2003 by PuhPuh
 *  
 *  This code is copyrighted property of the author.  It can still
 *  be used for any non-commercial purpose following conditions:
 *  
 *      1) This copyright notice is not removed.
 *      2) Source code follows any distribution of the software
 *         if possible.
 *      3) Copyright notice above is found in the documentation
 *         of the distributed software.
 *  
 *  Any express or implied warranties are disclaimed.  Author is
 *  not liable for any direct or indirect damages caused by the use
 *  of this software.
 *
 *  ----------------------------------------------------------------------
 *
 *  This code has been integrated into XBMC Media Center.  
 *  As such it can me copied, redistributed and modified under
 *  the same conditions as the XBMC itself.
 *
 */


#include "ccincludes.h"
#include "ccutil.h"

void cc_fatal(const char *m)
{
#if 0
  fprintf(stderr, "FATAL ERROR%s%s\n", 
	  (m != NULL) ? ": " : "",
	  (m != NULL) ? m :  "");
#endif
  exit(-1);
}

void *cc_xmalloc(size_t n)
{
  void *r;

  r = malloc(n);
  if (r == NULL)
    cc_fatal("Out of memory");
  return r;
}

void *cc_xcalloc(size_t n, size_t s)
{
  void *r;

  r = calloc(n, s);
  if (r == NULL)
    cc_fatal("Out of memory");
  return r;
}

void *cc_xrealloc(void *o, size_t n)
{
  void *r;

  r = realloc(o, n);
  if (r == NULL)
    cc_fatal("Out of memory");
  return r;
}

void *cc_xstrdup(const char *s)
{
  char *r;

  r = strdup(s != NULL ? s : "");
  if (r == NULL)
    cc_fatal("Out of memory");
  return r;
}

void *cc_xmemdup(const void *s, size_t len)
{
  unsigned char *r;

  r = cc_xmalloc(len + 1);
  r[len] = '\0';
  if (len > 0)
    memcpy(r, s, len);
  return (void *)r;
}

void cc_xfree(void *p)
{
  if (p != NULL)
    free(p);
}

/* eof (ccutil.c) */
