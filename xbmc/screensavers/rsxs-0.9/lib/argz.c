/* Functions for dealing with '\0' separated arg vectors.
   Copyright (C) 1995-1998, 2000-2002, 2006, 2008-2011 Free Software
   Foundation, Inc.
   This file is part of the GNU C Library.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA. */

#include <config.h>

#include <argz.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>



/* Add BUF, of length BUF_LEN to the argz vector in ARGZ & ARGZ_LEN.  */
error_t
argz_append (char **argz, size_t *argz_len, const char *buf, size_t buf_len)
{
  size_t new_argz_len = *argz_len + buf_len;
  char *new_argz = realloc (*argz, new_argz_len);
  if (new_argz)
    {
      memcpy (new_argz + *argz_len, buf, buf_len);
      *argz = new_argz;
      *argz_len = new_argz_len;
      return 0;
    }
  else
    return ENOMEM;
}

/* Add STR to the argz vector in ARGZ & ARGZ_LEN.  This should be moved into
   argz.c in libshouldbelibc.  */
error_t
argz_add (char **argz, size_t *argz_len, const char *str)
{
  return argz_append (argz, argz_len, str, strlen (str) + 1);
}



error_t
argz_add_sep (char **argz, size_t *argz_len, const char *string, int delim)
{
  size_t nlen = strlen (string) + 1;

  if (nlen > 1)
    {
      const char *rp;
      char *wp;

      *argz = (char *) realloc (*argz, *argz_len + nlen);
      if (*argz == NULL)
        return ENOMEM;

      wp = *argz + *argz_len;
      rp = string;
      do
        if (*rp == delim)
          {
            if (wp > *argz && wp[-1] != '\0')
              *wp++ = '\0';
            else
              --nlen;
          }
        else
          *wp++ = *rp;
      while (*rp++ != '\0');

      *argz_len += nlen;
    }

  return 0;
}



error_t
argz_create_sep (const char *string, int delim, char **argz, size_t *len)
{
  size_t nlen = strlen (string) + 1;

  if (nlen > 1)
    {
      const char *rp;
      char *wp;

      *argz = (char *) malloc (nlen);
      if (*argz == NULL)
        return ENOMEM;

      rp = string;
      wp = *argz;
      do
        if (*rp == delim)
          {
            if (wp > *argz && wp[-1] != '\0')
              *wp++ = '\0';
            else
              --nlen;
          }
        else
          *wp++ = *rp;
      while (*rp++ != '\0');

      if (nlen == 0)
        {
          free (*argz);
          *argz = NULL;
          *len = 0;
        }

      *len = nlen;
    }
  else
    {
      *argz = NULL;
      *len = 0;
    }

  return 0;
}


/* Insert ENTRY into ARGZ & ARGZ_LEN before BEFORE, which should be an
   existing entry in ARGZ; if BEFORE is NULL, ENTRY is appended to the end.
   Since ARGZ's first entry is the same as ARGZ, argz_insert (ARGZ, ARGZ_LEN,
   ARGZ, ENTRY) will insert ENTRY at the beginning of ARGZ.  If BEFORE is not
   in ARGZ, EINVAL is returned, else if memory can't be allocated for the new
   ARGZ, ENOMEM is returned, else 0.  */
error_t
argz_insert (char **argz, size_t *argz_len, char *before, const char *entry)
{
  if (! before)
    return argz_add (argz, argz_len, entry);

  if (before < *argz || before >= *argz + *argz_len)
    return EINVAL;

  if (before > *argz)
    /* Make sure before is actually the beginning of an entry.  */
    while (before[-1])
      before--;

  {
    size_t after_before = *argz_len - (before - *argz);
    size_t entry_len = strlen  (entry) + 1;
    size_t new_argz_len = *argz_len + entry_len;
    char *new_argz = realloc (*argz, new_argz_len);

    if (new_argz)
      {
        before = new_argz + (before - *argz);
        memmove (before + entry_len, before, after_before);
        memmove (before, entry, entry_len);
        *argz = new_argz;
        *argz_len = new_argz_len;
        return 0;
      }
    else
      return ENOMEM;
  }
}


char *
argz_next (const char *argz, size_t argz_len, const char *entry)
{
  if (entry)
    {
      if (entry < argz + argz_len)
        entry = strchr (entry, '\0') + 1;

      return entry >= argz + argz_len ? NULL : (char *) entry;
    }
  else
    if (argz_len > 0)
      return (char *) argz;
    else
      return NULL;
}


/* Make '\0' separated arg vector ARGZ printable by converting all the '\0's
   except the last into the character SEP.  */
void
argz_stringify (char *argz, size_t len, int sep)
{
  if (len > 0)
    while (1)
      {
        size_t part_len = strnlen (argz, len);
        argz += part_len;
        len -= part_len;
        if (len-- <= 1)         /* includes final '\0' we want to stop at */
          break;
        *argz++ = sep;
      }
}


/* Returns the number of strings in ARGZ.  */
size_t
argz_count (const char *argz, size_t len)
{
  size_t count = 0;
  while (len > 0)
    {
      size_t part_len = strlen (argz);
      argz += part_len + 1;
      len -= part_len + 1;
      count++;
    }
  return count;
}


/* Puts pointers to each string in ARGZ, plus a terminating 0 element, into
   ARGV, which must be large enough to hold them all.  */
void
argz_extract (const char *argz, size_t len, char **argv)
{
  while (len > 0)
    {
      size_t part_len = strlen (argz);
      *argv++ = (char *) argz;
      argz += part_len + 1;
      len -= part_len + 1;
    }
  *argv = 0;
}


/* Make a '\0' separated arg vector from a unix argv vector, returning it in
   ARGZ, and the total length in LEN.  If a memory allocation error occurs,
   ENOMEM is returned, otherwise 0.  */
error_t
argz_create (char *const argv[], char **argz, size_t *len)
{
  int argc;
  size_t tlen = 0;
  char *const *ap;
  char *p;

  for (argc = 0; argv[argc] != NULL; ++argc)
    tlen += strlen (argv[argc]) + 1;

  if (tlen == 0)
    *argz = NULL;
  else
    {
      *argz = malloc (tlen);
      if (*argz == NULL)
        return ENOMEM;

      for (p = *argz, ap = argv; *ap; ++ap, ++p)
        p = stpcpy (p, *ap);
    }
  *len = tlen;

  return 0;
}


/* Delete ENTRY from ARGZ & ARGZ_LEN, if any.  */
void
argz_delete (char **argz, size_t *argz_len, char *entry)
{
  if (entry)
    /* Get rid of the old value for NAME.  */
    {
      size_t entry_len = strlen (entry) + 1;
      *argz_len -= entry_len;
      memmove (entry, entry + entry_len, *argz_len - (entry - *argz));
      if (*argz_len == 0)
        {
          free (*argz);
          *argz = 0;
        }
    }
}


/* Append BUF, of length BUF_LEN to *TO, of length *TO_LEN, reallocating and
   updating *TO & *TO_LEN appropriately.  If an allocation error occurs,
   *TO's old value is freed, and *TO is set to 0.  */
static void
str_append (char **to, size_t *to_len, const char *buf, const size_t buf_len)
{
  size_t new_len = *to_len + buf_len;
  char *new_to = realloc (*to, new_len + 1);

  if (new_to)
    {
      *((char *) mempcpy (new_to + *to_len, buf, buf_len)) = '\0';
      *to = new_to;
      *to_len = new_len;
    }
  else
    {
      free (*to);
      *to = 0;
    }
}

/* Replace any occurrences of the string STR in ARGZ with WITH, reallocating
   ARGZ as necessary.  If REPLACE_COUNT is non-zero, *REPLACE_COUNT will be
   incremented by number of replacements performed.  */
error_t
argz_replace (char **argz, size_t *argz_len, const char *str, const char *with,
                unsigned *replace_count)
{
  error_t err = 0;

  if (str && *str)
    {
      char *arg = 0;
      char *src = *argz;
      size_t src_len = *argz_len;
      char *dst = 0;
      size_t dst_len = 0;
      int delayed_copy = 1;     /* True while we've avoided copying anything.  */
      size_t str_len = strlen (str), with_len = strlen (with);

      while (!err && (arg = argz_next (src, src_len, arg)))
        {
          char *match = strstr (arg, str);
          if (match)
            {
              char *from = match + str_len;
              size_t to_len = match - arg;
              char *to = strndup (arg, to_len);

              while (to && from)
                {
                  str_append (&to, &to_len, with, with_len);
                  if (to)
                    {
                      match = strstr (from, str);
                      if (match)
                        {
                          str_append (&to, &to_len, from, match - from);
                          from = match + str_len;
                        }
                      else
                        {
                          str_append (&to, &to_len, from, strlen (from));
                          from = 0;
                        }
                    }
                }

              if (to)
                {
                  if (delayed_copy)
                    /* We avoided copying SRC to DST until we found a match;
                       now that we've done so, copy everything from the start
                       of SRC.  */
                    {
                      if (arg > src)
                        err = argz_append (&dst, &dst_len, src, (arg - src));
                      delayed_copy = 0;
                    }
                  if (! err)
                    err = argz_add (&dst, &dst_len, to);
                  free (to);
                }
              else
                err = ENOMEM;

              if (replace_count)
                (*replace_count)++;
            }
          else if (! delayed_copy)
            err = argz_add (&dst, &dst_len, arg);
        }

      if (! err)
        {
          if (! delayed_copy)
            /* We never found any instances of str.  */
            {
              free (src);
              *argz = dst;
              *argz_len = dst_len;
            }
        }
      else if (dst_len > 0)
        free (dst);
    }

  return err;
}
