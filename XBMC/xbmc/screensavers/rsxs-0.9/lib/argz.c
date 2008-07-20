/* argz.c -- argz implementation for non-glibc systems
   Copyright (C) 2004 Free Software Foundation, Inc.
   Originally by Gary V. Vaughan  <gary@gnu.org>

   NOTE: The canonical source of this file is maintained with the
   GNU Libtool package.  Report bugs to bug-libtool@gnu.org.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA. */

#if defined(HAVE_CONFIG_H)
#  if defined(LTDL) && defined LT_CONFIG_H
#    include LT_CONFIG_H
#  else
#    include <config.h>
#  endif
#endif

#include <argz.h>

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <sys/types.h>
#include <errno.h>

#if defined(HAVE_STRING_H)
#  include <string.h>
#elif defined(HAVE_STRINGS_H)
#  include <strings.h>
#endif
#if defined(HAVE_MEMORY_H)
#  include <memory.h>
#endif

#define EOS_CHAR '\0'

error_t
argz_append (char **pargz, size_t *pargz_len, const char *buf, size_t buf_len)
{
  size_t argz_len;
  char  *argz;

  assert (pargz);
  assert (pargz_len);
  assert ((*pargz && *pargz_len) || (!*pargz && !*pargz_len));

  /* If nothing needs to be appended, no more work is required.  */
  if (buf_len == 0)
    return 0;

  /* Ensure there is enough room to append BUF_LEN.  */
  argz_len = *pargz_len + buf_len;
  argz = (char *) realloc (*pargz, argz_len);
  if (!argz)
    return ENOMEM;

  /* Copy characters from BUF after terminating '\0' in ARGZ.  */
  memcpy (argz + *pargz_len, buf, buf_len);

  /* Assign new values.  */
  *pargz = argz;
  *pargz_len = argz_len;

  return 0;
}


error_t
argz_create_sep (const char *str, int delim, char **pargz, size_t *pargz_len)
{
  size_t argz_len;
  char *argz = 0;

  assert (str);
  assert (pargz);
  assert (pargz_len);

  /* Make a copy of STR, but replacing each occurrence of
     DELIM with '\0'.  */
  argz_len = 1+ strlen (str);
  if (argz_len)
    {
      const char *p;
      char *q;

      argz = (char *) malloc (argz_len);
      if (!argz)
	return ENOMEM;

      for (p = str, q = argz; *p != EOS_CHAR; ++p)
	{
	  if (*p == delim)
	    {
	      /* Ignore leading delimiters, and fold consecutive
		 delimiters in STR into a single '\0' in ARGZ.  */
	      if ((q > argz) && (q[-1] != EOS_CHAR))
		*q++ = EOS_CHAR;
	      else
		--argz_len;
	    }
	  else
	    *q++ = *p;
	}
      /* Copy terminating EOS_CHAR.  */
      *q = *p;
    }

  /* If ARGZ_LEN has shrunk to nothing, release ARGZ's memory.  */
  if (!argz_len)
    argz = (free (argz), (char *) 0);

  /* Assign new values.  */
  *pargz = argz;
  *pargz_len = argz_len;

  return 0;
}


error_t
argz_insert (char **pargz, size_t *pargz_len, char *before, const char *entry)
{
  assert (pargz);
  assert (pargz_len);
  assert (entry && *entry);

  /* No BEFORE address indicates ENTRY should be inserted after the
     current last element.  */
  if (!before)
    return argz_append (pargz, pargz_len, entry, 1+ strlen (entry));

  /* This probably indicates a programmer error, but to preserve
     semantics, scan back to the start of an entry if BEFORE points
     into the middle of it.  */
  while ((before > *pargz) && (before[-1] != EOS_CHAR))
    --before;

  {
    size_t entry_len	= 1+ strlen (entry);
    size_t argz_len	= *pargz_len + entry_len;
    size_t offset	= before - *pargz;
    char   *argz	= (char *) realloc (*pargz, argz_len);

    if (!argz)
      return ENOMEM;

    /* Make BEFORE point to the equivalent offset in ARGZ that it
       used to have in *PARGZ incase realloc() moved the block.  */
    before = argz + offset;

    /* Move the ARGZ entries starting at BEFORE up into the new
       space at the end -- making room to copy ENTRY into the
       resulting gap.  */
    memmove (before + entry_len, before, *pargz_len - offset);
    memcpy  (before, entry, entry_len);

    /* Assign new values.  */
    *pargz = argz;
    *pargz_len = argz_len;
  }

  return 0;
}


char *
argz_next (char *argz, size_t argz_len, const char *entry)
{
  assert ((argz && argz_len) || (!argz && !argz_len));

  if (entry)
    {
      /* Either ARGZ/ARGZ_LEN is empty, or ENTRY points into an address
	 within the ARGZ vector.  */
      assert ((!argz && !argz_len)
	      || ((argz <= entry) && (entry < (argz + argz_len))));

      /* Move to the char immediately after the terminating
	 '\0' of ENTRY.  */
      entry = 1+ strchr (entry, EOS_CHAR);

      /* Return either the new ENTRY, or else NULL if ARGZ is
	 exhausted.  */
      return (entry >= argz + argz_len) ? 0 : (char *) entry;
    }
  else
    {
      /* This should probably be flagged as a programmer error,
	 since starting an argz_next loop with the iterator set
	 to ARGZ is safer.  To preserve semantics, handle the NULL
	 case by returning the start of ARGZ (if any).  */
      if (argz_len > 0)
	return argz;
      else
	return 0;
    }
}


void
argz_stringify (char *argz, size_t argz_len, int sep)
{
  assert ((argz && argz_len) || (!argz && !argz_len));

  if (sep)
    {
      --argz_len;		/* don't stringify the terminating EOS */
      while (--argz_len > 0)
	{
	  if (argz[argz_len] == EOS_CHAR)
	    argz[argz_len] = sep;
	}
    }
}
