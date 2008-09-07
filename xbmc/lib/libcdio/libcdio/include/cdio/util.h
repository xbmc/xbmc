/*
    $Id: util.h,v 1.11 2006/11/27 19:31:37 gmerlin Exp $

    Copyright (C) 2000 Herbert Valerio Riedel <hvr@gnu.org>
    Copyright (C) 2004, 2005, 2006 Rocky Bernstein <rocky@panix.com>

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

/*!
   \file util.h 
   \brief Miscellaneous utility functions. 

   Warning: this will probably get removed/replaced by using glib.h
*/
#include <stdlib.h>

#ifdef WIN32
#define snprintf _snprintf
#endif

#undef  MAX
#define MAX(a, b)  (((a) > (b)) ? (a) : (b))

#undef  MIN
#define MIN(a, b)  (((a) < (b)) ? (a) : (b))

#undef  IN
#define IN(x, low, high) ((x) >= (low) && (x) <= (high))

#undef  CLAMP
#define CLAMP(x, low, high)  (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))

static inline uint32_t
_cdio_len2blocks (uint32_t i_len, uint16_t i_blocksize)
{
  uint32_t i_blocks;

  i_blocks = i_len / (uint32_t) i_blocksize;
  if (i_len % i_blocksize)
    i_blocks++;

  return i_blocks;
}

/* round up to next block boundary */
static inline unsigned 
_cdio_ceil2block (unsigned offset, uint16_t i_blocksize)
{
  return _cdio_len2blocks (offset, i_blocksize) * i_blocksize;
}

static inline unsigned int
_cdio_ofs_add (unsigned offset, unsigned length, uint16_t i_blocksize)
{
  if (i_blocksize - (offset % i_blocksize) < length)
    offset = _cdio_ceil2block (offset, i_blocksize);

  offset += length;

  return offset;
}

static inline const char *
_cdio_bool_str (bool b)
{
  return b ? "yes" : "no";
}

#ifdef __cplusplus
extern "C" {
#endif

void *
_cdio_memdup (const void *mem, size_t count);

char *
_cdio_strdup_upper (const char str[]);

void
_cdio_strfreev(char **strv);

size_t
_cdio_strlenv(char **str_array);

char **
_cdio_strsplit(const char str[], char delim);

uint8_t cdio_to_bcd8(uint8_t n);
uint8_t cdio_from_bcd8(uint8_t p);

void cdio_follow_symlink (const char * src, char * dst);
  
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
