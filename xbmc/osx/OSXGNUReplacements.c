/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <pthread.h>
#include <errno.h>

#include "OSXGNUReplacements.h"

size_t strnlen(const char *s, size_t n)
{
  size_t i;
  for (i=0; i<n && s[i] != '\0'; i++)
    /* noop */ ;
  return i;
}

char* strndup(char const *s, size_t n)
{
  size_t len = strnlen(s, n);
  char *new_str = (char*)malloc(len + 1);
  if (new_str == NULL)
    return NULL;
  new_str[len] = '\0';
  
  return (char*)memcpy(new_str, s, len);
} 

/*
From http://svn.digium.com/view/asterisk/trunk/main/utils.c
GNU General Public License Version 2
Brief Reentrant replacement for gethostbyname for BSD-based systems. Note This
routine is derived from code originally written and placed in the public 
domain by Enzo Michelangeli <em@em.no-ip.com> 
*/
static pthread_mutex_t gethostbyname_r_mutex = PTHREAD_MUTEX_INITIALIZER;
//
int gethostbyname_r(const char *name, struct hostent *ret, char *buf,
                size_t buflen, struct hostent **result, int *h_errnop) 
{
  int hsave;
  struct hostent *ph;
  pthread_mutex_lock(&gethostbyname_r_mutex); /* begin critical area */
  hsave = h_errno;

  ph = gethostbyname(name);
  *h_errnop = h_errno; /* copy h_errno to *h_herrnop */
  if (ph == NULL) {
      *result = NULL;
  } else {
    char **p, **q;
    char *pbuf;
    int nbytes = 0;
    int naddr = 0, naliases = 0;
    /* determine if we have enough space in buf */

    /* count how many addresses */
    for (p = ph->h_addr_list; *p != 0; p++) {
      nbytes += ph->h_length; /* addresses */
      nbytes += sizeof(*p); /* pointers */
      naddr++;
    }
    nbytes += sizeof(*p); /* one more for the terminating NULL */

    /* count how many aliases, and total length of strings */
    for (p = ph->h_aliases; *p != 0; p++) {
      nbytes += (strlen(*p)+1); /* aliases */
      nbytes += sizeof(*p);  /* pointers */
      naliases++;
    }
    nbytes += sizeof(*p); /* one more for the terminating NULL */

    /* here nbytes is the number of bytes required in buffer */
    /* as a terminator must be there, the minimum value is ph->h_length */
    if (nbytes > (int)buflen) {
      *result = NULL;
      pthread_mutex_unlock(&gethostbyname_r_mutex); /* end critical area */
      return ERANGE; /* not enough space in buf!! */
    }

    /* There is enough space. Now we need to do a deep copy! */
    /* Allocation in buffer:
        from [0] to [(naddr-1) * sizeof(*p)]:
        pointers to addresses
        at [naddr * sizeof(*p)]:
        NULL
        from [(naddr+1) * sizeof(*p)] to [(naddr+naliases) * sizeof(*p)] :
        pointers to aliases
        at [(naddr+naliases+1) * sizeof(*p)]:
        NULL
        then naddr addresses (fixed length), and naliases aliases (asciiz).
    */

    *ret = *ph;   /* copy whole structure (not its address!) */

    /* copy addresses */
    q = (char **)buf; /* pointer to pointers area (type: char **) */
    ret->h_addr_list = q; /* update pointer to address list */
    pbuf = buf + ((naddr + naliases + 2) * sizeof(*p)); /* skip that area */
    for (p = ph->h_addr_list; *p != 0; p++) {
      memcpy(pbuf, *p, ph->h_length); /* copy address bytes */
      *q++ = pbuf; /* the pointer is the one inside buf... */
      pbuf += ph->h_length; /* advance pbuf */
    }
    *q++ = NULL; /* address list terminator */

    /* copy aliases */
    ret->h_aliases = q; /* update pointer to aliases list */
    for (p = ph->h_aliases; *p != 0; p++) {
      strcpy(pbuf, *p); /* copy alias strings */
      *q++ = pbuf; /* the pointer is the one inside buf... */
      pbuf += strlen(*p); /* advance pbuf */
      *pbuf++ = 0; /* string terminator */
    }
    *q++ = NULL; /* terminator */

    strcpy(pbuf, ph->h_name); /* copy alias strings */
    ret->h_name = pbuf;
    pbuf += strlen(ph->h_name); /* advance pbuf */
    *pbuf++ = 0; /* string terminator */

    *result = ret;  /* and let *result point to structure */

  }
  h_errno = hsave;  /* restore h_errno */
  pthread_mutex_unlock(&gethostbyname_r_mutex); /* end critical area */

  return (*result == NULL); /* return 0 on success, non-zero on error */
}

/* Copyright (C) 1991, 1992, 1995, 1996, 1997 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the GNU C Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <errno.h>

/* Read up to (and including) a TERMINATOR from STREAM into *LINEPTR
   (and null-terminate it). *LINEPTR is a pointer returned from malloc (or
   NULL), pointing to *N characters of space.  It is realloc'd as
   necessary.  Returns the number of characters read (not including the
   null terminator), or -1 on error or EOF.  */

ssize_t
getdelim (lineptr, n, terminator, stream)
     char **lineptr;
     size_t *n;
     int terminator;
     FILE *stream;
{
  char *line, *p;
  size_t size, copy;

  if (stream == NULL || lineptr == NULL || n == NULL)
    {
      errno = EINVAL;
      return -1;
    }

  if (ferror (stream))
    return -1;

  /* Make sure we have a line buffer to start with.  */
  if (*lineptr == NULL || *n < 2) /* !seen and no buf yet need 2 chars.  */
    {
#ifndef	MAX_CANON
#define	MAX_CANON	256
#endif
      line = realloc (*lineptr, MAX_CANON);
      if (line == NULL)
	return -1;
      *lineptr = line;
      *n = MAX_CANON;
    }

  line = *lineptr;
  size = *n;

  copy = size;
  p = line;

      while (1)
	{
	  size_t len;

	  while (--copy > 0)
	    {
	      register int c = getc (stream);
	      if (c == EOF)
		goto lose;
	      else if ((*p++ = c) == terminator)
		goto win;
	    }

	  /* Need to enlarge the line buffer.  */
	  len = p - line;
	  size *= 2;
	  line = realloc (line, size);
	  if (line == NULL)
	    goto lose;
	  *lineptr = line;
	  *n = size;
	  p = line + len;
	  copy = size - len;
	}

 lose:
  if (p == *lineptr)
    return -1;
  /* Return a partial line since we got an error in the middle.  */
 win:
  *p = '\0';
  return p - *lineptr;
}

ssize_t
getline (char **lineptr, size_t *n, FILE *stream)
{
  return getdelim (lineptr, n, '\n', stream);
}

/* Compare strings while treating digits characters numerically.
   Copyright (C) 1997, 2000 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Jean-Franois Bignolles <bignolle@ecoledoc.ibp.fr>, 1997.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the GNU C Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <string.h>
#include <ctype.h>

/* states: S_N: normal, S_I: comparing integral part, S_F: comparing
           fractional parts, S_Z: idem but with leading Zeroes only */
#define S_N    0x0
#define S_I    0x4
#define S_F    0x8
#define S_Z    0xC

/* result_type: CMP: return diff; LEN: compare using len_diff/diff */
#define CMP    2
#define LEN    3


/* ISDIGIT differs from isdigit, as follows:
   - Its arg may be any int or unsigned int; it need not be an unsigned char.
   - It's guaranteed to evaluate its argument exactly once.
   - It's typically faster.
   Posix 1003.2-1992 section 2.5.2.1 page 50 lines 1556-1558 says that
   only '0' through '9' are digits.  Prefer ISDIGIT to isdigit unless
   it's important to use the locale's definition of `digit' even when the
   host does not conform to Posix.  */
#define ISDIGIT(c) ((unsigned) (c) - '0' <= 9)

/*#undef __strverscmp
#undef strverscmp

#define __strverscmp strverscmp
*/
/* Compare S1 and S2 as strings holding indices/version numbers,
   returning less than, equal to or greater than zero if S1 is less than,
   equal to or greater than S2 (for more info, see the texinfo doc).
*/

int
strverscmp (const char *s1, const char *s2)
{
  const unsigned char *p1 = (const unsigned char *) s1;
  const unsigned char *p2 = (const unsigned char *) s2;
  unsigned char c1, c2;
  int state;
  int diff;

  /* Symbol(s)    0       [1-9]   others  (padding)
     Transition   (10) 0  (01) d  (00) x  (11) -   */
  static const unsigned int next_state[] =
  {
      /* state    x    d    0    - */
      /* S_N */  S_N, S_I, S_Z, S_N,
      /* S_I */  S_N, S_I, S_I, S_I,
      /* S_F */  S_N, S_F, S_F, S_F,
      /* S_Z */  S_N, S_F, S_Z, S_Z
  };

  static const int result_type[] =
  {
      /* state   x/x  x/d  x/0  x/-  d/x  d/d  d/0  d/-
                 0/x  0/d  0/0  0/-  -/x  -/d  -/0  -/- */

      /* S_N */  CMP, CMP, CMP, CMP, CMP, LEN, CMP, CMP,
                 CMP, CMP, CMP, CMP, CMP, CMP, CMP, CMP,
      /* S_I */  CMP, -1,  -1,  CMP,  1,  LEN, LEN, CMP,
                  1,  LEN, LEN, CMP, CMP, CMP, CMP, CMP,
      /* S_F */  CMP, CMP, CMP, CMP, CMP, LEN, CMP, CMP,
                 CMP, CMP, CMP, CMP, CMP, CMP, CMP, CMP,
      /* S_Z */  CMP,  1,   1,  CMP, -1,  CMP, CMP, CMP,
                 -1,  CMP, CMP, CMP
  };

  if (p1 == p2)
    return 0;

  c1 = *p1++;
  c2 = *p2++;
  /* Hint: '0' is a digit too.  */
  state = S_N | ((c1 == '0') + (ISDIGIT (c1) != 0));

  while ((diff = c1 - c2) == 0 && c1 != '\0')
    {
      state = next_state[state];
      c1 = *p1++;
      c2 = *p2++;
      state |= (c1 == '0') + (ISDIGIT (c1) != 0);
    }

  state = result_type[state << 2 | ((c2 == '0') + (ISDIGIT (c2) != 0))];

  switch (state)
    {
    case CMP:
      return diff;

    case LEN:
      while (ISDIGIT (*p1++))
        if (!ISDIGIT (*p2++))
          return 1;

      return ISDIGIT (*p2) ? -1 : diff;

    default:
      return state;
    }
}
