/*
    $Id: util.c,v 1.5 2006/11/28 12:14:16 gmerlin Exp $

    Copyright (C) 2000 Herbert Valerio Riedel <hvr@gnu.org>
    Copyright (C) 2003, 2004, 2005 Rocky Bernstein <rocky@panix.com>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

#ifdef HAVE_UNISTD_H // readlink
#include <unistd.h>
#endif

#ifdef HAVE_INTTYPES_H
#include "inttypes.h"
#endif

#include "cdio_assert.h"
#include <cdio/types.h>
#include <cdio/util.h>

static const char _rcsid[] = "$Id: util.c,v 1.5 2006/11/28 12:14:16 gmerlin Exp $";

size_t
_cdio_strlenv(char **str_array)
{
  size_t n = 0;

  cdio_assert (str_array != NULL);

  while(str_array[n])
    n++;

  return n;
}

void
_cdio_strfreev(char **strv)
{
  int n;
  
  cdio_assert (strv != NULL);

  for(n = 0; strv[n]; n++)
    free(strv[n]);

  free(strv);
}

char **
_cdio_strsplit(const char str[], char delim) /* fixme -- non-reentrant */
{
  int n;
  char **strv = NULL;
  char *_str, *p;
  char _delim[2] = { 0, 0 };

  cdio_assert (str != NULL);

  _str = strdup(str);
  _delim[0] = delim;

  cdio_assert (_str != NULL);

  n = 1;
  p = _str;
  while(*p) 
    if (*(p++) == delim)
      n++;

  strv = calloc (1, sizeof (char *) * (n+1));
  
  n = 0;
  while((p = strtok(n ? NULL : _str, _delim)) != NULL) 
    strv[n++] = strdup(p);

  free(_str);

  return strv;
}

void *
_cdio_memdup (const void *mem, size_t count)
{
  void *new_mem = NULL;

  if (mem)
    {
      new_mem = calloc (1, count);
      memcpy (new_mem, mem, count);
    }
  
  return new_mem;
}

char *
_cdio_strdup_upper (const char str[])
{
  char *new_str = NULL;

  if (str)
    {
      char *p;

      p = new_str = strdup (str);

      while (*p)
        {
          *p = toupper (*p);
          p++;
        }
    }

  return new_str;
}

uint8_t
cdio_to_bcd8 (uint8_t n)
{
  /*cdio_assert (n < 100);*/

  return ((n/10)<<4) | (n%10);
}

uint8_t
cdio_from_bcd8(uint8_t p)
{
  return (0xf & p)+(10*(p >> 4));
}

/*!
  Follow symlinks until we have the real device file
  (idea taken from libunieject). 
*/

void cdio_follow_symlink (const char * src, char * dst) {
#ifdef HAVE_READLINK
  char tmp_src[PATH_MAX+1];
  char tmp_dst[PATH_MAX+1];
  
  int len;

  strcpy(tmp_src, src);
  while(1) {
    len = readlink(tmp_src, tmp_dst, PATH_MAX);
    if(len < 0) {
      strncpy(dst, tmp_src, PATH_MAX);
      return;
    }
    else {
      tmp_dst[len] = '\0';
      strncpy(tmp_src, tmp_dst, PATH_MAX);
    }
  }
#else
  strncpy(dst, src, PATH_MAX);
#endif
  
}


/* 
 * Local variables:
 *  c-file-style: "gnu"
 *  tab-width: 8
 *  indent-tabs-mode: nil
 * End:
 */
