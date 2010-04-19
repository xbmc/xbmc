/*
  @(#) $Id: common.c,v 1.4 2003/11/17 12:27:39 yeti Exp $
  memory and string operations and some more common stuff

  Copyright (C) 2000-2002 David Necas (Yeti) <yeti@physics.muni.cz>

  This program is free software; you can redistribute it and/or modify it
  under the terms of version 2 of the GNU General Public License as published
  by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
  more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
*/
#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdarg.h>
#include <string.h>

#include "enca.h"
#include "internal.h"

/**
 * enca_malloc:
 * @size: The number of bytes to allocate.
 *
 * Allocates memory, always successfully (when fails, aborts program).
 *
 * Returns: Pointer to the newly allocated memory.
 **/
void*
enca_malloc(size_t size)
{
  void *ptr;

  if (size == 0)
    size = 1;
  ptr = malloc(size);
  assert(ptr != NULL);

  return ptr;
}

/**
 * enca_realloc:
 * @ptr: Pointer to block of previously allocated memory.
 * @size: The number of bytes to resize the block.
 *
 * Reallocates memory, always successfully (when fails, aborts program).
 *
 * Returns: Pointer to the newly allocated memory, #NULL when @size is zero.
 **/
void*
enca_realloc(void *ptr, size_t size)
{
  if (size == 0) {
    free(ptr);
    return NULL;
  }

  ptr = realloc(ptr, size);
  assert(ptr != NULL);

  return ptr;
}

/**
 * enca_strdup:
 * @s: A string.
 *
 * Duplicates string.
 *
 * Will be defined as strdup() when system provides it.
 *
 * Returns: The newly allocated string copy.
 **/
char*
enca_strdup(const char *s) {
  if (s == NULL)
    return NULL;
  else
    return strcpy(enca_malloc(strlen(s) + 1), s);
}

#ifndef HAVE_STRSTR
/**
 * enca_strstr:
 * @haystack: A string where to search.
 * @needle: A string to find.
 *
 * Finds occurence of a substring in a string.
 *
 * Will be defined as strstr() when system provides it.
 *
 * Returns: Pointer to the first occurence of @needle in @haystack; #NULL if
 *          not found.
 **/
const char*
enca_strstr(const char *haystack,
            const char *needle)
{
  char c;
  size_t n;

  /* handle singularities */
  if (needle == NULL)
    return haystack;
  if ((n = strlen(needle)) == 0)
    return haystack;

  /* search */
  c = needle[0];
  while ((haystack = strchr(haystack, c)) != NULL) {
    if (strncmp(haystack, needle, n) == 0)
      return haystack;
  }

  return NULL;
}
#endif

#ifndef HAVE_STPCPY
/**
 * enca_stpcpy:
 * @dest: A string.
 * @src: A string to append.
 *
 * Appends a string to the end of another strings, returning pointer to
 * the terminating zero byte.
 *
 * Will be defined as stpcpy() when system provides it.
 *
 * Caller is responisble for providing @dest long enough to hold the result.
 *
 * Returns: Pointer to the terminating zero byte of resulting string.
 **/
char*
enca_stpcpy(char *dest,
            const char *src)
{
  const char *p = src;

  if (src == NULL)
    return dest;

  while (*p != '\0')
    *dest++ = *p++;

  *dest = '\0';
  return dest;
}
#endif

/**
 * enca_strconcat:
 * @str: A string.
 * @...: A #NULL-terminated list of string to append.
 *
 * Concatenates arbitrary (but at least one) number of strings.
 *
 * Returns: All the strings concatenated together.
 **/
char*
enca_strconcat(const char *str,
               ...)
{
  va_list ap;
  char *result = NULL;
  size_t n;
  const char *s;
  char *r;

  /* compute size of resulting string */
  n = 1;
  va_start(ap, str);
  for (s = str; s != NULL; s = va_arg(ap, const char*))
    n += strlen(s);
  va_end(ap);

  /* and construct it using the smart stpcpy() function */
  r = result = (char*)enca_malloc(n);
  va_start(ap, str);
  for (s = str; s != NULL; s = va_arg(ap, const char*))
    r = enca_stpcpy(r, s);
  va_end(ap);

  return result;
}

/**
 * enca_strappend:
 * @str: A string.
 * @...: A #NULL-terminated list of string to append.
 *
 * Appends arbitrary number of strings to a string.
 *
 * The string @str is destroyed (reallocated), the others are kept.
 *
 * Returns: All the strings concatenated together.
 **/
char*
enca_strappend(char *str,
               ...)
{
  va_list ap;
  size_t n, n1;
  const char *s;
  char *r;

  /* compute size of resulting string */
  n1 = strlen(str);
  n = 1 + n1;
  va_start(ap, str);
  for (s = va_arg(ap, const char*); s != NULL; s = va_arg(ap, const char*))
    n += strlen(s);
  va_end(ap);

  /* and construct it using the smart stpcpy() function */
  str = (char*)enca_realloc(str, n);
  r = str + n1;
  va_start(ap, str);
  for (s = va_arg(ap, const char*); s != NULL; s = va_arg(ap, const char*))
    r = enca_stpcpy(r, s);
  va_end(ap);

  return str;
}

/* vim: ts=2
 */
