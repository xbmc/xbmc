/* gbase64.c - Base64 encoding/decoding
 *
 *  Copyright (C) 2006 Alexander Larsson <alexl@redhat.com>
 *  Copyright (C) 2000-2003 Ximian Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * This is based on code in camel, written by:
 *    Michael Zucchi <notzed@ximian.com>
 *    Jeffrey Stedfast <fejj@ximian.com>
 */

#include "config.h"

#include <string.h>

#include "gbase64.h"
#include "glib.h"
#include "glibintl.h"

#include "galias.h"

static const char base64_alphabet[] =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/**
 * g_base64_encode_step:
 * @in: the binary data to encode
 * @len: the length of @in
 * @break_lines: whether to break long lines
 * @out: pointer to destination buffer
 * @state: Saved state between steps, initialize to 0
 * @save: Saved state between steps, initialize to 0
 *
 * Incrementally encode a sequence of binary data into its Base-64 stringified
 * representation. By calling this function multiple times you can convert 
 * data in chunks to avoid having to have the full encoded data in memory.
 *
 * When all of the data has been converted you must call 
 * g_base64_encode_close() to flush the saved state.
 *
 * The output buffer must be large enough to fit all the data that will
 * be written to it. Due to the way base64 encodes you will need
 * at least: (@len / 3 + 1) * 4 + 4 bytes (+ 4 may be needed in case of
 * non-zero state). If you enable line-breaking you will need at least:
 * ((@len / 3 + 1) * 4 + 4) / 72 + 1 bytes of extra space.
 *
 * @break_lines is typically used when putting base64-encoded data in emails.
 * It breaks the lines at 72 columns instead of putting all of the text on 
 * the same line. This avoids problems with long lines in the email system.
 *
 * Return value: The number of bytes of output that was written
 *
 * Since: 2.12
 */
gsize
g_base64_encode_step (const guchar *in, 
		      gsize         len, 
		      gboolean      break_lines, 
		      gchar        *out, 
		      gint         *state, 
		      gint         *save)
{
  char *outptr;
  const guchar *inptr;
  
  g_return_val_if_fail (in != NULL, 0);
  g_return_val_if_fail (out != NULL, 0);
  g_return_val_if_fail (state != NULL, 0);
  g_return_val_if_fail (save != NULL, 0);

  if (len <= 0)
    return 0;
  
  inptr = in;
  outptr = out;
  
  if (len + ((char *) save) [0] > 2)
    {
      const guchar *inend = in+len-2;
      int c1, c2, c3;
      int already;
      
      already = *state;
      
      switch (((char *) save) [0])
	{	
	case 1:	
	  c1 = ((unsigned char *) save) [1]; 
          goto skip1;
	case 2:	
          c1 = ((unsigned char *) save) [1];
	  c2 = ((unsigned char *) save) [2]; 
          goto skip2;
	}
      
      /* 
       * yes, we jump into the loop, no i'm not going to change it, 
       * it's beautiful! 
       */
      while (inptr < inend)
	{
	  c1 = *inptr++;
	skip1:
	  c2 = *inptr++;
	skip2:
	  c3 = *inptr++;
	  *outptr++ = base64_alphabet [ c1 >> 2 ];
	  *outptr++ = base64_alphabet [ c2 >> 4 | 
					((c1&0x3) << 4) ];
	  *outptr++ = base64_alphabet [ ((c2 &0x0f) << 2) | 
					(c3 >> 6) ];
	  *outptr++ = base64_alphabet [ c3 & 0x3f ];
	  /* this is a bit ugly ... */
	  if (break_lines && (++already) >= 19)
	    {
	      *outptr++ = '\n';
	      already = 0;
	    }
	}
      
      ((char *)save)[0] = 0;
      len = 2 - (inptr - inend);
      *state = already;
    }
  
  if (len>0)
    {
      char *saveout;
      
      /* points to the slot for the next char to save */
      saveout = & (((char *)save)[1]) + ((char *)save)[0];
      
      /* len can only be 0 1 or 2 */
      switch(len)
	{
	case 2:	*saveout++ = *inptr++;
	case 1:	*saveout++ = *inptr++;
	}
      ((char *)save)[0] += len;
    }
  
  return outptr - out;
}

/**
 * g_base64_encode_close:
 * @break_lines: whether to break long lines
 * @out: pointer to destination buffer
 * @state: Saved state from g_base64_encode_step()
 * @save: Saved state from g_base64_encode_step()
 *
 * Flush the status from a sequence of calls to g_base64_encode_step().
 *
 * Return value: The number of bytes of output that was written
 *
 * Since: 2.12
 */
gsize
g_base64_encode_close (gboolean  break_lines,
		       gchar    *out, 
		       gint     *state, 
		       gint     *save)
{
  int c1, c2;
  char *outptr = out;

  g_return_val_if_fail (out != NULL, 0);
  g_return_val_if_fail (state != NULL, 0);
  g_return_val_if_fail (save != NULL, 0);

  c1 = ((unsigned char *) save) [1];
  c2 = ((unsigned char *) save) [2];
  
  switch (((char *) save) [0])
    {
    case 2:
      outptr [2] = base64_alphabet[ ( (c2 &0x0f) << 2 ) ];
      g_assert (outptr [2] != 0);
      goto skip;
    case 1:
      outptr[2] = '=';
    skip:
      outptr [0] = base64_alphabet [ c1 >> 2 ];
      outptr [1] = base64_alphabet [ c2 >> 4 | ( (c1&0x3) << 4 )];
      outptr [3] = '=';
      outptr += 4;
      break;
    }
  if (break_lines)
    *outptr++ = '\n';
  
  *save = 0;
  *state = 0;
  
  return outptr - out;
}

/**
 * g_base64_encode:
 * @data: the binary data to encode
 * @len: the length of @data
 *
 * Encode a sequence of binary data into its Base-64 stringified
 * representation.
 *
 * Return value: a newly allocated, zero-terminated Base-64 encoded
 *               string representing @data. The returned string must 
 *               be freed with g_free().
 *
 * Since: 2.12
 */
gchar *
g_base64_encode (const guchar *data, 
                 gsize         len)
{
  gchar *out;
  gint state = 0, outlen;
  gint save = 0;

  g_return_val_if_fail (data != NULL, NULL);
  g_return_val_if_fail (len > 0, NULL);

  /* We can use a smaller limit here, since we know the saved state is 0,
     +1 is needed for trailing \0, also check for unlikely integer overflow */
  if (len >= ((G_MAXSIZE - 1) / 4 - 1) * 3)
    g_error("%s: input too large for Base64 encoding (%"G_GSIZE_FORMAT" chars)",
        G_STRLOC, len);

  out = g_malloc ((len / 3 + 1) * 4 + 1);

  outlen = g_base64_encode_step (data, len, FALSE, out, &state, &save);
  outlen += g_base64_encode_close (FALSE, out + outlen, &state, &save);
  out[outlen] = '\0';

  return (gchar *) out;
}

static const unsigned char mime_base64_rank[256] = {
  255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
  255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
  255,255,255,255,255,255,255,255,255,255,255, 62,255,255,255, 63,
   52, 53, 54, 55, 56, 57, 58, 59, 60, 61,255,255,255,  0,255,255,
  255,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
   15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25,255,255,255,255,255,
  255, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
   41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51,255,255,255,255,255,
  255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
  255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
  255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
  255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
  255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
  255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
  255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
  255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
};

/**
 * g_base64_decode_step: 
 * @in: binary input data
 * @len: max length of @in data to decode
 * @out: output buffer
 * @state: Saved state between steps, initialize to 0
 * @save: Saved state between steps, initialize to 0
 *
 * Incrementally decode a sequence of binary data from its Base-64 stringified
 * representation. By calling this function multiple times you can convert 
 * data in chunks to avoid having to have the full encoded data in memory.
 *
 * The output buffer must be large enough to fit all the data that will
 * be written to it. Since base64 encodes 3 bytes in 4 chars you need
 * at least: (@len / 4) * 3 + 3 bytes (+ 3 may be needed in case of non-zero
 * state).
 * 
 * Return value: The number of bytes of output that was written
 *
 * Since: 2.12
 **/
gsize
g_base64_decode_step (const gchar  *in, 
		      gsize         len, 
		      guchar       *out, 
		      gint         *state, 
		      guint        *save)
{
  const guchar *inptr;
  guchar *outptr;
  const guchar *inend;
  guchar c, rank;
  guchar last[2];
  unsigned int v;
  int i;

  g_return_val_if_fail (in != NULL, 0);
  g_return_val_if_fail (out != NULL, 0);
  g_return_val_if_fail (state != NULL, 0);
  g_return_val_if_fail (save != NULL, 0);

  if (len <= 0)
    return 0;
  
  inend = (const guchar *)in+len;
  outptr = out;
  
  /* convert 4 base64 bytes to 3 normal bytes */
  v=*save;
  i=*state;
  inptr = (const guchar *)in;
  last[0] = last[1] = 0;
  while (inptr < inend)
    {
      c = *inptr++;
      rank = mime_base64_rank [c];
      if (rank != 0xff)
	{
	  last[1] = last[0];
	  last[0] = c;
	  v = (v<<6) | rank;
	  i++;
	  if (i==4)
	    {
	      *outptr++ = v>>16;
	      if (last[1] != '=')
		*outptr++ = v>>8;
	      if (last[0] != '=')
		*outptr++ = v;
	      i=0;
	    }
	}
    }
  
  *save = v;
  *state = i;
  
  return outptr - out;
}

/**
 * g_base64_decode:
 * @text: zero-terminated string with base64 text to decode
 * @out_len: The length of the decoded data is written here
 *
 * Decode a sequence of Base-64 encoded text into binary data
 *
 * Return value: a newly allocated buffer containing the binary data
 *               that @text represents. The returned buffer must
 *               be freed with g_free().
 *
 * Since: 2.12
 */
guchar *
g_base64_decode (const gchar *text,
		 gsize       *out_len)
{
  guchar *ret;
  gsize input_length;
  gint state = 0;
  guint save = 0;
  
  g_return_val_if_fail (text != NULL, NULL);
  g_return_val_if_fail (out_len != NULL, NULL);

  input_length = strlen (text);

  g_return_val_if_fail (input_length > 1, NULL);

  /* We can use a smaller limit here, since we know the saved state is 0,
     +1 used to avoid calling g_malloc0(0), and hence retruning NULL */
  ret = g_malloc0 ((input_length / 4) * 3 + 1);
  
  *out_len = g_base64_decode_step (text, input_length, ret, &state, &save);
  
  return ret; 
}
 
/**
 * g_base64_decode_inplace:
 * @text: zero-terminated string with base64 text to decode
 * @out_len: The length of the decoded data is written here
 *
 * Decode a sequence of Base-64 encoded text into binary data
 * by overwriting the input data.
 *
 * Return value: The binary data that @text responds. This pointer
 *               is the same as the input @text.
 *
 * Since: 2.20
 */
guchar *
g_base64_decode_inplace (gchar *text,
                         gsize *out_len)
{
  gint input_length, state = 0;
  guint save = 0;
  
  g_return_val_if_fail (text != NULL, NULL);
  g_return_val_if_fail (out_len != NULL, NULL);

  input_length = strlen (text);

  g_return_val_if_fail (input_length > 1, NULL);

  *out_len = g_base64_decode_step (text, input_length, (guchar *) text, &state, &save);
  
  return text; 
}


#define __G_BASE64_C__
#include "galiasdef.c"
