/*
 *  Copyright (C) 1991, 1992, 1995, 1996, 1997 Free Software Foundation, Inc.
 *  This file is part of the GNU C Library.
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include <errno.h>
#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

