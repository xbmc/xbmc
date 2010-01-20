// Place the code and data below here into the LIBXBMS section.
#ifndef __GNUC__
#pragma code_seg( "LIBXBMS" )
#pragma data_seg( "LIBXBMS_RW" )
#pragma bss_seg( "LIBXBMS_RW" )
#pragma const_seg( "LIBXBMS_RD" )
#pragma comment(linker, "/merge:LIBXBMS_RW=LIBXBMS")
#pragma comment(linker, "/merge:LIBXBMS_RD=LIBXBMS")
#endif
/*   -*- c -*-
 * 
 *  ----------------------------------------------------------------------
 *  Protocol packet encode/decode for CcXstream Server for XBMC Media Center
 *  ----------------------------------------------------------------------
 *
 *  Copyright (c) 2002-2003 by PuhPuh
 *  
 *  This code is copyrighted property of the author.  It can still
 *  be used for any non-commercial purpose following conditions:
 *  
 *      1) This copyright notice is not removed.
 *      2) Source code follows any distribution of the software
 *         if possible.
 *      3) Copyright notice above is found in the documentation
 *         of the distributed software.
 *  
 *  Any express or implied warranties are disclaimed.  Author is
 *  not liable for any direct or indirect damages caused by the use
 *  of this software.
 *
 *  ----------------------------------------------------------------------
 *
 *  This code has been integrated into XBMC Media Center.  
 *  As such it can me copied, redistributed and modified under
 *  the same conditions as the XBMC itself.
 *
 */

#include "ccincludes.h"
#include "ccbuffer.h"
#include "ccxencode.h"

void cc_xstream_encode_int(unsigned char *buf, unsigned long x)
{
  buf[3] = (unsigned char)(x & 0xff);
  x >>= 8;
  buf[2] = (unsigned char)(x & 0xff);
  x >>= 8;
  buf[1] = (unsigned char)(x & 0xff);
  x >>= 8;
  buf[0] = (unsigned char)(x & 0xff);
}

void cc_xstream_buffer_encode_int(CcBuffer buf, unsigned long x)
{
  cc_buffer_append_space(buf, 4);
  cc_xstream_encode_int(cc_buffer_ptr(buf) + cc_buffer_len(buf) - 4, x);
}

void cc_xstream_buffer_encode_int64(CcBuffer buf, CC_UINT_64_TYPE_NAME x)
{
  unsigned char nb[8];

  nb[7] = (unsigned char)(x & 0xff);
  x >>= 8;
  nb[6] = (unsigned char)(x & 0xff);
  x >>= 8;
  nb[5] = (unsigned char)(x & 0xff);
  x >>= 8;
  nb[4] = (unsigned char)(x & 0xff);
  x >>= 8;
  nb[3] = (unsigned char)(x & 0xff);
  x >>= 8;
  nb[2] = (unsigned char)(x & 0xff);
  x >>= 8;
  nb[1] = (unsigned char)(x & 0xff);
  x >>= 8;
  nb[0] = (unsigned char)(x & 0xff);
  cc_buffer_append(buf, nb, 8);
}

void cc_xstream_buffer_encode_byte(CcBuffer buf, unsigned char b)
{
  cc_buffer_append(buf, &b, 1);
}

void cc_xstream_buffer_encode_data_string(CcBuffer buf, const unsigned char *str, size_t str_len)
{
  cc_xstream_buffer_encode_int(buf, (unsigned long)str_len);
  cc_buffer_append(buf, str, str_len);
}

void cc_xstream_buffer_encode_string(CcBuffer buf, const char *str)
{
  size_t str_len;

  str_len = strlen(str);
  cc_xstream_buffer_encode_int(buf, (unsigned long)str_len);
  cc_buffer_append(buf, (unsigned char *)str, str_len);
}

void cc_xstream_buffer_encode_packet_length(CcBuffer buf)
{
  unsigned long len;
  unsigned char hlp[4];

  len = (unsigned long)cc_buffer_len(buf);
  cc_xstream_encode_int(hlp, len);
  cc_buffer_prepend(buf, hlp, 4);
}


unsigned long cc_xstream_decode_int(const unsigned char *buf)
{
  unsigned long r;

  r = (unsigned long)(buf[0]);
  r <<= 8;
  r |= (unsigned long)(buf[1]);
  r <<= 8;
  r |= (unsigned long)(buf[2]);
  r <<= 8;
  r |= (unsigned long)(buf[3]);
  return r;
}

int cc_xstream_buffer_decode_int(CcBuffer buf, unsigned long *x)
{
  if (cc_buffer_len(buf) < 4)
    return 0;
  *x = cc_xstream_decode_int(cc_buffer_ptr(buf));
  cc_buffer_consume(buf, 4);
  return 1;
}

int cc_xstream_buffer_decode_int64(CcBuffer buf, CC_UINT_64_TYPE_NAME *x)
{
  CC_UINT_64_TYPE_NAME r;
  unsigned char *b;
  int i;

  if (cc_buffer_len(buf) < 8)
    return 0;
  r = 0;
  for (i = 0; i < 8; i++)
    {
      b = cc_buffer_ptr(buf) + i;
      r = (r << 8) | (CC_UINT_64_TYPE_NAME)(*b);
    }
  *x = r;
  cc_buffer_consume(buf, 8);
  return 1;
}

int cc_xstream_buffer_decode_byte(CcBuffer buf, unsigned char *b)
{
  if (cc_buffer_len(buf) < 1)
    return 0;
  *b = *(cc_buffer_ptr(buf));
  cc_buffer_consume(buf, 1);
  return 1;
}

int cc_xstream_buffer_decode_string(CcBuffer buf, unsigned char **str, size_t *str_len)
{
  unsigned long len;

  if (cc_buffer_len(buf) < 4)
    return 0;
  len = cc_xstream_decode_int(cc_buffer_ptr(buf));
  if ((len + 4) > cc_buffer_len(buf))
    return 0;
  cc_buffer_consume(buf, 4);
  if (str_len != NULL)
    *str_len = (size_t)len;
  *str = cc_xmemdup(cc_buffer_ptr(buf), len);
  cc_buffer_consume(buf, len);
  return 1;
}

/* eof (ccxencode.c) */
