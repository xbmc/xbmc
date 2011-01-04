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
 *  Buffer stuff.
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

CcBuffer cc_buffer_allocate(void)
{
  CcBuffer buffer;

  buffer = cc_xcalloc(1, sizeof (*buffer));
  return buffer;
}

void cc_buffer_free(CcBuffer buffer)
{
  cc_buffer_uninit(buffer);
  cc_xfree(buffer);
}

void cc_buffer_init(CcBuffer buffer)
{
  memset(buffer, 0, sizeof (*buffer));
}

void cc_buffer_uninit(CcBuffer buffer)
{
  if (buffer != NULL)
    cc_xfree(buffer->data);
  memset(buffer, 0, sizeof (*buffer));
}

size_t cc_buffer_len(CcBuffer buffer)
{
  return buffer->len;
}

unsigned char *cc_buffer_ptr(CcBuffer buffer)
{
  return buffer->data;
}

void cc_buffer_append(CcBuffer buffer, const unsigned char *data, size_t len)
{
  size_t ol;

  ol = cc_buffer_len(buffer);
  cc_buffer_append_space(buffer, len);
  memcpy(buffer->data + ol, data, len);
}

void cc_buffer_prepend(CcBuffer buffer, const unsigned char *data, size_t len)
{
  size_t ol;

  ol = cc_buffer_len(buffer);
  cc_buffer_append_space(buffer, len);
  memmove(buffer->data + len, buffer->data, ol);
  memcpy(buffer->data, data, len);
}

void cc_buffer_append_space(CcBuffer buffer, size_t len)
{
  if (len > 0)
    {
      if (buffer->data == NULL)
	{
	  buffer->data = cc_xmalloc(len);
	  buffer->len = 0;
	}
      else
	{
	  buffer->data = cc_xrealloc(buffer->data, buffer->len + len);
	}
      buffer->len += len;
    }
}

void cc_buffer_append_string(CcBuffer buffer, const char *string)
{
  if (*string != '\0')
    cc_buffer_append(buffer, (const unsigned char *)string, strlen(string));
}

void cc_buffer_prepend_string(CcBuffer buffer, const char *string)
{
  if (*string != '\0')
    cc_buffer_prepend(buffer, (const unsigned char *)string, strlen(string));
}

void cc_buffer_consume(CcBuffer buffer, size_t len)
{
  if (len > buffer->len)
    {
      cc_fatal("Buffer underflow.");
    }
  else if (len == 0)
    {
      /*NOTHING*/;
    }
  else if (len == buffer->len)
    {
      buffer->len = 0;
    }
  else
    {
      memmove(buffer->data, buffer->data + len, buffer->len - len);
      buffer->len -= len;
    }
}

void cc_buffer_consume_end(CcBuffer buffer, size_t len)
{
  if (len > buffer->len)
    {
      cc_fatal("Buffer underflow.");
    }
  else if (len == 0)
    {
      /*NOTHING*/;
    }
  else if (len == buffer->len)
    {
      buffer->len = 0;
    }
  else
    {
      buffer->len -= len;
    }
}

void cc_buffer_clear(CcBuffer buffer)
{
  buffer->len = 0;
}

void cc_buffer_steal(CcBuffer buffer, unsigned char **data, size_t *len)
{
  buffer->data = cc_xrealloc(buffer->data, buffer->len + 1);
  buffer->data[buffer->len] = '\0';
  *data = buffer->data;
  if (len != NULL)
    *len = buffer->len;
  buffer->data = NULL;
  buffer->len = 0;
}

/* eof (ccbuffer.c) */
