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


#ifndef CCUTIL_H_INCLUDED
#define CCUTIL_H_INCLUDED 1

void cc_fatal(const char *m);
void *cc_xmalloc(size_t n);
void *cc_xcalloc(size_t n, size_t s);
void *cc_xrealloc(void *o, size_t n);
void *cc_xstrdup(const char *s);
void *cc_xmemdup(const void *s, size_t len);
void cc_xfree(void *p);

struct CcStringListRec {
  char *s;
  struct CcStringListRec *next;
};

typedef struct CcStringListRec *CcStringList;

#endif /* CCUTIL_H_INCLUDED */
/* eof (ccutil.h) */
