/*
    $Id$

    Copyright (C) 2000 Herbert Valerio Riedel <hvr@gnu.org>

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

#ifndef __CDIO_UTIL_H__
#define __CDIO_UTIL_H__

#include <stdlib.h>

#include "types.h"

#undef  MAX
#define MAX(a, b)  (((a) > (b)) ? (a) : (b))

#undef  MIN
#define MIN(a, b)  (((a) < (b)) ? (a) : (b))

#undef  IN
#define IN(x, low, high) ((x) >= (low) && (x) <= (high))

#undef  CLAMP
#define CLAMP(x, low, high)  (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))

static inline unsigned
_cdio_len2blocks (unsigned len, int blocksize)
{
  unsigned blocks;

  blocks = len / blocksize;
  if (len % blocksize)
    blocks++;

  return blocks;
}

/* round up to next block boundary */
static inline unsigned 
_cdio_ceil2block (unsigned offset, int blocksize)
{
  return _cdio_len2blocks (offset, blocksize) * blocksize;
}

static inline unsigned 
_cdio_ofs_add (unsigned offset, unsigned length, int blocksize)
{
  if (blocksize - (offset % blocksize) < length)
    offset = _cdio_ceil2block (offset, blocksize);

  offset += length;

  return offset;
}

void *
_cdio_malloc (size_t size);

void *
_cdio_memdup (const void *mem, size_t count);

char *
_cdio_strdup_upper (const char str[]);

void
_cdio_strfreev(char **strv);

char *
_cdio_strjoin (char *strv[], unsigned count, const char delim[]);

size_t
_cdio_strlenv(char **str_array);

char **
_cdio_strsplit(const char str[], char delim);

static inline const char *
_cdio_bool_str (bool b)
{
  return b ? "yes" : "no";
}

/* BCD */
#ifdef __cplusplus
extern "C" {
#endif

uint8_t  to_bcd8(uint8_t n);
uint8_t  from_bcd8(uint8_t p);

#ifdef __cplusplus
}
#endif

#endif /* __CDIO_UTIL_H__ */


/* 
 * Local variables:
 *  c-file-style: "gnu"
 *  tab-width: 8
 *  indent-tabs-mode: nil
 * End:
 */
