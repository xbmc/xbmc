/*
    Copyright (C) 2006 Burkhard Plaum <plaum@ipf.uni-stuttgart.de>

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
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301 USA.
*/
/* UTF-8 support */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#ifdef HAVE_JOLIET
#ifdef HAVE_STRING_H
# include <string.h>
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_ICONV
# include <iconv.h>
#endif

#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif

#include <cdio/utf8.h>

#include <stdio.h>


struct cdio_charset_coverter_s
  {
  iconv_t ic;
  };

cdio_charset_coverter_t *
cdio_charset_converter_create(const char * src_charset,
                              const char * dst_charset)
  {
  cdio_charset_coverter_t * ret;
  ret = calloc(1, sizeof(*ret));
  ret->ic = iconv_open(dst_charset, src_charset);
  return ret;
  }

#if 0
static void bgav_hexdump(uint8_t * data, int len, int linebreak)
  {
  int i;
  int bytes_written = 0;
  int imax;
  
  while(bytes_written < len)
    {
    imax = (bytes_written + linebreak > len) ? len - bytes_written : linebreak;
    for(i = 0; i < imax; i++)
      fprintf(stderr, "%02x ", data[bytes_written + i]);
    for(i = imax; i < linebreak; i++)
      fprintf(stderr, "   ");
    for(i = 0; i < imax; i++)
      {
      if(!(data[bytes_written + i] & 0x80) && (data[bytes_written + i] >= 32))
        fprintf(stderr, "%c", data[bytes_written + i]);
      else
        fprintf(stderr, ".");
      }
    bytes_written += imax;
    fprintf(stderr, "\n");
    }
  }
#endif

void cdio_charset_converter_destroy(cdio_charset_coverter_t*cnv)
  {
  iconv_close(cnv->ic);
  free(cnv);
  }

#define BYTES_INCREMENT 16

static bool
do_convert(iconv_t cd, char * src, int src_len,
           char ** dst, int *dst_len)
  {
  char * ret;

  char *inbuf;
  char *outbuf;
  int alloc_size;
  int output_pos;
  size_t inbytesleft;
  size_t outbytesleft;

  if(src_len < 0)
    src_len = strlen(src);
#if 0
  fprintf(stderr, "Converting:\n");
  bgav_hexdump(src, src_len, 16);
#endif    
  alloc_size = src_len + BYTES_INCREMENT;

  inbytesleft  = src_len;
  
  /* We reserve space here to add a final '\0' */
  outbytesleft = alloc_size-1;

  ret    = malloc(alloc_size);

  inbuf  = src;
  outbuf = ret;
  
  while(1)
    {
    
    if(iconv(cd, &inbuf, &inbytesleft,
             &outbuf, &outbytesleft) == (size_t)-1)
      {
      switch(errno)
        {
        case E2BIG:
          output_pos = (int)(outbuf - ret);

          alloc_size   += BYTES_INCREMENT;
          outbytesleft += BYTES_INCREMENT;

          ret = realloc(ret, alloc_size);
          if (ret == NULL)
            {
            fprintf(stderr, "Can't realloc(%d).\n", alloc_size);
            return false;
            }
          outbuf = ret + output_pos;
          break;
        default:
          fprintf(stderr, "Iconv failed: %s\n", strerror(errno));
          if (ret != NULL)
            free(ret);
          return false;
          break;
        }
      }
    if(!inbytesleft)
      break;
    }
  /* Zero terminate */
  *outbuf = '\0';

  /* Set return values */
  *dst = ret;
  if(dst_len)
    *dst_len = (int)(outbuf - ret);
#if 0
  fprintf(stderr, "Conversion done, src:\n");
  bgav_hexdump(src, src_len, 16);
  fprintf(stderr, "dst:\n");
  bgav_hexdump((uint8_t*)(ret), (int)(outbuf - ret), 16);
#endif  
  return true;
  }

bool cdio_charset_convert(cdio_charset_coverter_t*cnv,
                          char * src, int src_len,
                          char ** dst, int * dst_len)
  {
  return do_convert(cnv->ic, src, src_len, dst, dst_len);
  }



bool cdio_charset_from_utf8(cdio_utf8_t * src, char ** dst,
                            int * dst_len, const char * dst_charset)
  {
  iconv_t ic;
  bool result;
  ic = iconv_open(dst_charset, "UTF-8");
  result = do_convert(ic, src, -1, dst, dst_len);
  iconv_close(ic);
  return result;
  }




bool cdio_charset_to_utf8(char *src, size_t src_len, cdio_utf8_t **dst,
                          const char * src_charset)
  {
  iconv_t ic;
  bool result;
  ic = iconv_open("UTF-8", src_charset);
  result = do_convert(ic, src, src_len, dst, NULL);
  iconv_close(ic);
  return result;
  }
#endif /* HAVE_JOLIET */
