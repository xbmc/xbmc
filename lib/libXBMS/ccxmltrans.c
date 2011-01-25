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
 *  Translate raw strings so that they can be inserted into xml.
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
#include "ccxmltrans.h"

typedef struct {
   char *raw;
   char *xml;
} CcXstreamXmlTranslationRec, *CcXstreamXmlTranslation;

#ifndef __GNUC__
#pragma code_seg()
#pragma data_seg()
#pragma bss_seg()
#pragma const_seg()
#endif

static CcXstreamXmlTranslationRec cc_xml_translation[] = {
  { ">", "&gt;" },
  { "<", "&lt;" },
  { "/", "&slash;" },
  { "\\", "&backslash;" },
  { "\"", "&doublequote;" },
  { "&", "&amp;" },
  { NULL, NULL }
};

#ifndef __GNUC__
#pragma code_seg( "LIBXBMS" )
#pragma data_seg( "LIBXBMS_RW" )
#pragma bss_seg( "LIBXBMS_RW" )
#pragma const_seg( "LIBXBMS_RD" )
#pragma comment(linker, "/merge:LIBXBMS_RW=LIBXBMS")
#pragma comment(linker, "/merge:LIBXBMS_RD=LIBXBMS")
#endif

char *cc_xstream_xml_encode(const char *raw)
{
  CcBufferRec buf[1];
  const char *tmp;
  char *r, cb[16];
  int i;
  unsigned int x;

  cc_buffer_init(buf);
  for (tmp = raw; *tmp != '\0'; /*NOTHING*/)
    {
      for (i = 0; cc_xml_translation[i].raw != NULL; i++)
	{
	  if (strncmp(tmp, cc_xml_translation[i].raw, strlen(cc_xml_translation[i].raw)) == 0)
	    {
	      cc_buffer_append_string(buf, cc_xml_translation[i].xml);
	      tmp += strlen(cc_xml_translation[i].raw);
	      break;
	    }
	}
      if (cc_xml_translation[i].raw == NULL)
	{
	  if (isprint(*tmp) && ((! isspace(*tmp)) || (*tmp == ' ')))
	    {
	      cc_buffer_append(buf, (unsigned char *)tmp, 1);
	    }
	  else
	    {
	      x = (unsigned int)(*tmp);
	      sprintf(cb, "&%04x;", x);
	      cc_buffer_append_string(buf, cb);
	    }
	  tmp++;
	}
    }
  r = cc_xmemdup(cc_buffer_ptr(buf), cc_buffer_len(buf));
  cc_buffer_uninit(buf);
  return r;
}

char *cc_xstream_xml_decode(const char *xml)
{
  CcBufferRec buf[1];
  const char *tmp;
  char *r;
  unsigned long l;
  unsigned char c;
  int i;

  cc_buffer_init(buf);
  for (tmp = xml; *tmp != '\0'; /*NOTHING*/)
    {
      for (i = 0; cc_xml_translation[i].xml != NULL; i++)
	{
	  if (strncmp(tmp, cc_xml_translation[i].xml, strlen(cc_xml_translation[i].xml)) == 0)
	    {
	      cc_buffer_append_string(buf, cc_xml_translation[i].raw);
	      tmp += strlen(cc_xml_translation[i].xml);
	      break;
	    }
	}
      if (cc_xml_translation[i].raw == NULL)
	{
	  if ((tmp[0] == '&') &&
	      (tmp[1] == '0') &&
	      (tmp[2] == '0') &&
	      (isxdigit(tmp[3])) &&
	      (isxdigit(tmp[4])) &&
	      (tmp[5] == ';'))
	    {
	      l = strtoul(tmp + 1, NULL, 16);
	      c = (unsigned char)l;
	      cc_buffer_append(buf, &c, 1);
	      tmp += 6;
	    }
	  else
	    {
	      cc_buffer_append(buf, (unsigned char *)tmp, 1);
	      tmp++;
	    }
	}
    }
  r = cc_xmemdup(cc_buffer_ptr(buf), cc_buffer_len(buf));
  cc_buffer_uninit(buf);
  return r;
}

/* eof (ccxmltrans.c) */
