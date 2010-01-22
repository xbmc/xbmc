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
 *  CcXstream Client Library for XBMC Media Center
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
#include "ccxclient.h"
#include "ccxencode.h"

#ifndef __GNUC__
#pragma code_seg()
#pragma data_seg()
#pragma bss_seg()
#pragma const_seg()
#endif

static unsigned long lNextId = 0;
unsigned long next_id()
{
  
  lNextId = lNextId + 1 % 0xfffff; /* 20 bits in maximum */
  if (lNextId == 0)
    lNextId++;
  return (lNextId << 12); /* Low 12 bits are zero */
}

#ifndef __GNUC__
#pragma code_seg( "LIBXBMS" )
#pragma data_seg( "LIBXBMS_RW" )
#pragma bss_seg( "LIBXBMS_RW" )
#pragma const_seg( "LIBXBMS_RD" )
#pragma comment(linker, "/merge:LIBXBMS_RW=LIBXBMS")
#pragma comment(linker, "/merge:LIBXBMS_RD=LIBXBMS")
#endif

CcXstreamClientError cc_xstream_client_version_handshake(CcXstreamServerConnection s)
{
  unsigned char *c, x;
  CcBufferRec buf[1];
  char *hlp1, *hlp2, *hlp3;
  int i;

  cc_buffer_init(buf);
  
  for (i = 0; 1; i++)
    {
      if (i > 2048)
	{
	  cc_buffer_uninit(buf);
	  return CC_XSTREAM_CLIENT_FATAL_ERROR;
	}
      c = cc_xstream_client_read_data(s, 1, CCXSTREAM_CLIENT_TIMEOUT_SECONDS * 1000U);
      if (c == NULL)
	{
	  cc_buffer_uninit(buf);
	  return CC_XSTREAM_CLIENT_FATAL_ERROR;
	}
      x = c[0];
      cc_xfree(c);
      if (x == '\n')
	break;
      else if (x != '\r')
	cc_buffer_append(buf, &x, 1);
    }
  hlp1 = cc_xmemdup(cc_buffer_ptr(buf), cc_buffer_len(buf));
  cc_buffer_uninit(buf);
  /* OK, we parse the version string.  Supported versions are between
     the first and the second space characters in the string.
     Versions are in the comma separated list.  This code is somewhat
     spaghetti, but it does the trick and ensures that supported
     versions list contain string CC_XSTREAM_CLIENT_VERSION.  The
     reason for complexity is that it can be the first or last item in
     the list.  It can also be the only item in the list.  And of
     course string we got from the server may be nonstandard and we
     can't crash because of it. */
  hlp2 = strchr(hlp1, ' ');
  if (hlp2 == NULL)
    {
      cc_xfree(hlp1);
      return CC_XSTREAM_CLIENT_FATAL_ERROR;
    }
  hlp2++;
  hlp3 = strchr(hlp2, ' ');
  if (hlp3 == NULL)
    {
      cc_xfree(hlp1);
      return CC_XSTREAM_CLIENT_FATAL_ERROR;
    }
  *hlp3 = '\0';
  while ((hlp3 = strrchr(hlp2, ',')) != NULL)
    {
      *hlp3 = '\0';
      hlp3++;
      if (strcmp(hlp3, CC_XSTREAM_CLIENT_VERSION) == 0)
      	break;
    }
  if (hlp3 == NULL)
    {
      if (strcmp(hlp2, CC_XSTREAM_CLIENT_VERSION) != 0)
	{
	  cc_xfree(hlp1);
	  return CC_XSTREAM_CLIENT_FATAL_ERROR;
	}
    }
  cc_xfree(hlp1);
  if (cc_xstream_client_write_data(s, (unsigned char *)CC_XSTREAM_CLIENT_VERSION_STR "\n", strlen(CC_XSTREAM_CLIENT_VERSION_STR  "\n"), 0) == 0)
    return CC_XSTREAM_CLIENT_FATAL_ERROR;
  return CC_XSTREAM_CLIENT_OK;
}

unsigned long cc_xstream_client_mkpacket_setcwd(const char *path, 
						unsigned char **p, size_t *p_len)
{
  CcBufferRec buf[1];
  unsigned int r;

  /* Initialize the buffer. */
  cc_buffer_init(buf);
  /* Encode packet */
  cc_xstream_buffer_encode_byte(buf, (unsigned char)CC_XSTREAM_XBMSP_PACKET_SETCWD);
  r = next_id();
  cc_xstream_buffer_encode_int(buf, r);
  cc_xstream_buffer_encode_string(buf, path);
  cc_xstream_buffer_encode_packet_length(buf);
  /* Return payload */
  *p = cc_xmemdup(cc_buffer_ptr(buf), cc_buffer_len(buf));
  *p_len = cc_buffer_len(buf);
  /* Free the buffer */
  cc_buffer_uninit(buf);
  /* Return the command id. */
  return r;
}

unsigned long cc_xstream_client_mkpacket_upcwd(unsigned long levels,
					       unsigned char **p, size_t *p_len)
{
  CcBufferRec buf[1];
  unsigned int r;

  /* Initialize the buffer. */
  cc_buffer_init(buf);
  /* Encode packet */
  cc_xstream_buffer_encode_byte(buf, (unsigned char)CC_XSTREAM_XBMSP_PACKET_UPCWD);
  r = next_id();
  cc_xstream_buffer_encode_int(buf, r);
  cc_xstream_buffer_encode_int(buf, levels);
  cc_xstream_buffer_encode_packet_length(buf);
  /* Return payload */
  *p = cc_xmemdup(cc_buffer_ptr(buf), cc_buffer_len(buf));
  *p_len = cc_buffer_len(buf);
  /* Free the buffer */
  cc_buffer_uninit(buf);
  /* Return the command id. */
  return r;
}

unsigned long cc_xstream_client_mkpacket_filelist_open(unsigned char **p, size_t *p_len)
{
  CcBufferRec buf[1];
  unsigned int r;

  /* Initialize the buffer. */
  cc_buffer_init(buf);
  /* Encode packet */
  cc_xstream_buffer_encode_byte(buf, (unsigned char)CC_XSTREAM_XBMSP_PACKET_FILELIST_OPEN);
  r = next_id();
  cc_xstream_buffer_encode_int(buf, r);
  cc_xstream_buffer_encode_packet_length(buf);
  /* Return payload */
  *p = cc_xmemdup(cc_buffer_ptr(buf), cc_buffer_len(buf));
  *p_len = cc_buffer_len(buf);
  /* Free the buffer */
  cc_buffer_uninit(buf);
  /* Return the command id. */
  return r;
}

unsigned long cc_xstream_client_mkpacket_filelist_read(unsigned long handle,
						       unsigned char **p, size_t *p_len)
{
  CcBufferRec buf[1];
  unsigned int r;

  /* Initialize the buffer. */
  cc_buffer_init(buf);
  /* Encode packet */
  cc_xstream_buffer_encode_byte(buf, (unsigned char)CC_XSTREAM_XBMSP_PACKET_FILELIST_READ);
  r = next_id();
  cc_xstream_buffer_encode_int(buf, r);
  cc_xstream_buffer_encode_int(buf, handle);
  cc_xstream_buffer_encode_packet_length(buf);
  /* Return payload */
  *p = cc_xmemdup(cc_buffer_ptr(buf), cc_buffer_len(buf));
  *p_len = cc_buffer_len(buf);
  /* Free the buffer */
  cc_buffer_uninit(buf);
  /* Return the command id. */
  return r;
}

unsigned long cc_xstream_client_mkpacket_file_info(const char *path,
						   unsigned char **p, size_t *p_len)
{
  CcBufferRec buf[1];
  unsigned int r;

  /* Initialize the buffer. */
  cc_buffer_init(buf);
  /* Encode packet */
  cc_xstream_buffer_encode_byte(buf, (unsigned char)CC_XSTREAM_XBMSP_PACKET_FILE_INFO);
  r = next_id();
  cc_xstream_buffer_encode_int(buf, r);
  cc_xstream_buffer_encode_string(buf, path);
  cc_xstream_buffer_encode_packet_length(buf);
  /* Return payload */
  *p = cc_xmemdup(cc_buffer_ptr(buf), cc_buffer_len(buf));
  *p_len = cc_buffer_len(buf);
  /* Free the buffer */
  cc_buffer_uninit(buf);
  /* Return the command id. */
  return r;
}

unsigned long cc_xstream_client_mkpacket_file_open(const char *path,
						   unsigned char **p, size_t *p_len)
{
  CcBufferRec buf[1];
  unsigned int r;

  /* Initialize the buffer. */
  cc_buffer_init(buf);
  /* Encode packet */
  cc_xstream_buffer_encode_byte(buf, (unsigned char)CC_XSTREAM_XBMSP_PACKET_FILE_OPEN);
  r = next_id();
  cc_xstream_buffer_encode_int(buf, r);
  cc_xstream_buffer_encode_string(buf, path);
  cc_xstream_buffer_encode_packet_length(buf);
  /* Return payload */
  *p = cc_xmemdup(cc_buffer_ptr(buf), cc_buffer_len(buf));
  *p_len = cc_buffer_len(buf);
  /* Free the buffer */
  cc_buffer_uninit(buf);
  /* Return the command id. */
  return r;
}

unsigned long cc_xstream_client_mkpacket_file_read(unsigned long handle, size_t len,
						   unsigned char **p, size_t *p_len)
{
  CcBufferRec buf[1];
  unsigned int r;

  /* Initialize the buffer. */
  cc_buffer_init(buf);
  /* Encode packet */
  cc_xstream_buffer_encode_byte(buf, (unsigned char)CC_XSTREAM_XBMSP_PACKET_FILE_READ);
  r = next_id();
  cc_xstream_buffer_encode_int(buf, r);
  cc_xstream_buffer_encode_int(buf, handle);
  cc_xstream_buffer_encode_int(buf, len);
  cc_xstream_buffer_encode_packet_length(buf);
  /* Return payload */
  *p = cc_xmemdup(cc_buffer_ptr(buf), cc_buffer_len(buf));
  *p_len = cc_buffer_len(buf);
  /* Free the buffer */
  cc_buffer_uninit(buf);
  /* Return the command id. */
  return r;
}

unsigned long cc_xstream_client_mkpacket_seek(unsigned long handle, 
					      int seek_type, CC_UINT_64_TYPE_NAME bytes,
					      unsigned char **p, size_t *p_len)
{
  CcBufferRec buf[1];
  unsigned int r;

  /* Initialize the buffer. */
  cc_buffer_init(buf);
  /* Encode packet */
  cc_xstream_buffer_encode_byte(buf, (unsigned char)CC_XSTREAM_XBMSP_PACKET_FILE_SEEK);
  r = next_id();
  cc_xstream_buffer_encode_int(buf, r);
  cc_xstream_buffer_encode_int(buf, handle);
  cc_xstream_buffer_encode_byte(buf, (unsigned char)seek_type);
  cc_xstream_buffer_encode_int64(buf, bytes);
  cc_xstream_buffer_encode_packet_length(buf);
  /* Return payload */
  *p = cc_xmemdup(cc_buffer_ptr(buf), cc_buffer_len(buf));
  *p_len = cc_buffer_len(buf);
  /* Free the buffer */
  cc_buffer_uninit(buf);
  /* Return the command id. */
  return r;
}

unsigned long cc_xstream_client_mkpacket_close(unsigned long handle,
					       unsigned char **p, size_t *p_len)
{
  CcBufferRec buf[1];
  unsigned int r;

  /* Initialize the buffer. */
  cc_buffer_init(buf);
  /* Encode packet */
  cc_xstream_buffer_encode_byte(buf, (unsigned char)CC_XSTREAM_XBMSP_PACKET_CLOSE);
  r = next_id();
  cc_xstream_buffer_encode_int(buf, r);
  cc_xstream_buffer_encode_int(buf, handle);
  cc_xstream_buffer_encode_packet_length(buf);
  /* Return payload */
  *p = cc_xmemdup(cc_buffer_ptr(buf), cc_buffer_len(buf));
  *p_len = cc_buffer_len(buf);
  /* Free the buffer */
  cc_buffer_uninit(buf);
  /* Return the command id. */
  return r;
}

unsigned long cc_xstream_client_mkpacket_close_all(unsigned char **p, size_t *p_len)
{
  CcBufferRec buf[1];
  unsigned int r;

  /* Initialize the buffer. */
  cc_buffer_init(buf);
  /* Encode packet */
  cc_xstream_buffer_encode_byte(buf, (unsigned char)CC_XSTREAM_XBMSP_PACKET_CLOSE_ALL);
  r = next_id();
  cc_xstream_buffer_encode_int(buf, r);
  cc_xstream_buffer_encode_packet_length(buf);
  /* Return payload */
  *p = cc_xmemdup(cc_buffer_ptr(buf), cc_buffer_len(buf));
  *p_len = cc_buffer_len(buf);
  /* Free the buffer */
  cc_buffer_uninit(buf);
  /* Return the command id. */
  return r;
}

unsigned long cc_xstream_client_mkpacket_setconfoption(const char *option, 
						       const char *value,
						       unsigned char **p, size_t *p_len)
{
  CcBufferRec buf[1];
  unsigned int r;

  /* Initialize the buffer. */
  cc_buffer_init(buf);
  /* Encode packet */
  cc_xstream_buffer_encode_byte(buf, (unsigned char)CC_XSTREAM_XBMSP_PACKET_SET_CONFIGURATION_OPTION);
  r = next_id();
  cc_xstream_buffer_encode_int(buf, r);
  cc_xstream_buffer_encode_string(buf, option);
  cc_xstream_buffer_encode_string(buf, value);
  cc_xstream_buffer_encode_packet_length(buf);
  /* Return payload */
  *p = cc_xmemdup(cc_buffer_ptr(buf), cc_buffer_len(buf));
  *p_len = cc_buffer_len(buf);
  /* Free the buffer */
  cc_buffer_uninit(buf);
  /* Return the command id. */
  return r;
}

unsigned long cc_xstream_client_mkpacket_authentication_init(const char *method,
	       						     unsigned char **p, size_t *p_len)
{
  CcBufferRec buf[1];
  unsigned int r;

  /* Initialize the buffer. */
  cc_buffer_init(buf);
  /* Encode packet */
  cc_xstream_buffer_encode_byte(buf, (unsigned char)CC_XSTREAM_XBMSP_PACKET_AUTHENTICATION_INIT);
  r = next_id();
  cc_xstream_buffer_encode_int(buf, r);
  cc_xstream_buffer_encode_string(buf, method);
  cc_xstream_buffer_encode_packet_length(buf);
  /* Return payload */
  *p = cc_xmemdup(cc_buffer_ptr(buf), cc_buffer_len(buf));
  *p_len = cc_buffer_len(buf);
  /* Free the buffer */
  cc_buffer_uninit(buf);
  /* Return the command id. */
  return r;
}

unsigned long cc_xstream_client_mkpacket_authenticate_password(unsigned long handle,
							       const char *user_id,
							       const char *password,
							       unsigned char **p, size_t *p_len)
{
  CcBufferRec buf[1];
  unsigned int r;

  /* Initialize the buffer. */
  cc_buffer_init(buf);
  /* Encode packet */
  cc_xstream_buffer_encode_byte(buf, (unsigned char)CC_XSTREAM_XBMSP_PACKET_AUTHENTICATE);
  r = next_id();
  cc_xstream_buffer_encode_int(buf, r);
  cc_xstream_buffer_encode_int(buf, handle);
  cc_xstream_buffer_encode_string(buf, user_id);
  cc_xstream_buffer_encode_string(buf, password);
  cc_xstream_buffer_encode_packet_length(buf);
  /* Return payload */
  *p = cc_xmemdup(cc_buffer_ptr(buf), cc_buffer_len(buf));
  *p_len = cc_buffer_len(buf);
  /* Free the buffer */
  cc_buffer_uninit(buf);
  /* Return the command id. */
  return r;
}

unsigned long cc_xstream_client_mkpacket_server_discovery(unsigned char **p, size_t *p_len)
{
  CcBufferRec buf[1];
  unsigned int r;

  /* Initialize the buffer. */
  cc_buffer_init(buf);
  /* Encode packet. */
  cc_xstream_buffer_encode_byte(buf, (unsigned char)CC_XSTREAM_XBMSP_PACKET_SERVER_DISCOVERY_QUERY);
  r = next_id();
  cc_xstream_buffer_encode_int(buf, r);
  cc_xstream_buffer_encode_string(buf, CC_XSTREAM_CLIENT_VERSION_STR);
  cc_xstream_buffer_encode_packet_length(buf);
  /* Return payload */
  *p = cc_xmemdup(cc_buffer_ptr(buf), cc_buffer_len(buf));
  *p_len = cc_buffer_len(buf);
  /* Free the buffer */
  cc_buffer_uninit(buf);
  /* Return the command id. */
  return r;
}

void cc_xstream_client_reply_packet_free(CcXstreamReplyPacket packet)
{
  if (packet != NULL)
    {
      cc_xfree(packet->string1);
      cc_xfree(packet->string2);
    }
  cc_xfree(packet);
}

CcXstreamReplyPacket cc_xstream_client_reply_packet_parse(const unsigned char *packet,
							  size_t packet_len)
{
  CcXstreamPacket pt;
  unsigned long id, handle, err;
  unsigned char *s1, *s2;
  size_t sl1, sl2;
  CcXstreamReplyPacket r;

  if (packet_len < 5)
    return NULL;

  id = cc_xstream_decode_int(packet + 1);

  switch (packet[0])
    {
    case CC_XSTREAM_XBMSP_PACKET_OK:
      if (packet_len != 5)
	return NULL;
      pt = CC_XSTREAM_XBMSP_PACKET_OK;
      s1 = s2 = NULL;
      sl1 = sl2 = 0;
      err = handle = 0;
      break;

    case CC_XSTREAM_XBMSP_PACKET_ERROR:
      if (packet_len < 10)
	return NULL;
      pt = CC_XSTREAM_XBMSP_PACKET_ERROR;
      err = packet[5];
      s1 = s2 = NULL;
      sl1 = sl2 = 0;
      handle = 0;
      break;

    case CC_XSTREAM_XBMSP_PACKET_HANDLE:
      if (packet_len != 9)
	return NULL;
      pt = CC_XSTREAM_XBMSP_PACKET_HANDLE;
      handle = cc_xstream_decode_int(packet + 5);
      s1 = s2 = NULL;
      sl1 = sl2 = 0;
      err = 0;
      break;

    case CC_XSTREAM_XBMSP_PACKET_FILE_DATA:
      pt = CC_XSTREAM_XBMSP_PACKET_FILE_DATA;
      sl1 = cc_xstream_decode_int(packet + 5);
      if (packet_len < (sl1 + 13))
	return NULL;
      sl2 = cc_xstream_decode_int(packet + 9 + sl1);
      if (packet_len != (13 + sl1 + sl2))
	return NULL;
      s1 = cc_xmemdup(packet + 9, sl1);
      s2 = cc_xmemdup(packet + 13 + sl1, sl2);
      err = handle = 0;
      break;

    case CC_XSTREAM_XBMSP_PACKET_FILE_CONTENTS:
      if (packet_len < 5)
	return NULL;
      pt = CC_XSTREAM_XBMSP_PACKET_FILE_CONTENTS;
      sl1 = cc_xstream_decode_int(packet + 5);
      if (packet_len != (sl1 + 9))
	return NULL;
      s1 = cc_xmemdup(packet + 9, sl1);
      s2 = NULL;
      sl2 = 0;
      err = handle = 0;
      break;

    default:
      return NULL;
    }
  r = cc_xcalloc(1, sizeof (*r));
  r->type = pt;
  r->id = id;
  r->handle = handle;
  r->error = err;
  r->string1 = s1;
  r->string2 = s2;
  r->string1_len = sl1;
  r->string2_len = sl2;
  return r;
}

CcXstreamClientError cc_xstream_client_reply_packet_read(CcXstreamServerConnection s,
							 CcXstreamReplyPacket *packet)
{
  unsigned char *lb, *pb;
  size_t len;
  CcXstreamReplyPacket p;

  if ((lb = cc_xstream_client_read_data(s, 4, CCXSTREAM_CLIENT_TIMEOUT_SECONDS * 1000U)) == NULL)
    return CC_XSTREAM_CLIENT_FATAL_ERROR;
  len = cc_xstream_decode_int(lb);
  cc_xfree(lb);
  if (len > 0x20000)
    {
      fprintf(stderr, "Too long packet (len=%u)\n", (unsigned int)len);
      return CC_XSTREAM_CLIENT_FATAL_ERROR;
    }
  if ((pb = cc_xstream_client_read_data(s, len, CCXSTREAM_CLIENT_TIMEOUT_SECONDS * 1000U)) == NULL)
    {
      free(pb);
      return CC_XSTREAM_CLIENT_FATAL_ERROR;
    }
  p = cc_xstream_client_reply_packet_parse(pb, len);
  cc_xfree(pb);
  if (p == NULL)
    return CC_XSTREAM_CLIENT_FATAL_ERROR;
  *packet = p;
  return CC_XSTREAM_CLIENT_OK;
}

CcXstreamClientError cc_xstream_client_setcwd(CcXstreamServerConnection s,
					      const char *path)
{
  unsigned char *pb;
  size_t pb_len;
  unsigned long id;
  CcXstreamReplyPacket rp;

  id = cc_xstream_client_mkpacket_setcwd(path, &pb, &pb_len);
  if (cc_xstream_client_write_data(s, pb, pb_len, 0) == 0)
    {
      cc_xfree(pb);
      return CC_XSTREAM_CLIENT_FATAL_ERROR;
    }
  cc_xfree(pb);
  if (cc_xstream_client_reply_packet_read(s, &rp) != CC_XSTREAM_CLIENT_OK)
    return CC_XSTREAM_CLIENT_FATAL_ERROR;
  if (rp->id != id)
    {
      cc_xstream_client_reply_packet_free(rp);
      return CC_XSTREAM_CLIENT_FATAL_ERROR;
    }
  else if (rp->type == CC_XSTREAM_XBMSP_PACKET_OK)
    {
      cc_xstream_client_reply_packet_free(rp);
      return CC_XSTREAM_CLIENT_OK;
    }
  else if (rp->type == CC_XSTREAM_XBMSP_PACKET_ERROR)
    {
      cc_xstream_client_reply_packet_free(rp);
      return CC_XSTREAM_CLIENT_COMMAND_FAILED;
    }
  else
    {
      cc_xstream_client_reply_packet_free(rp);
      return CC_XSTREAM_CLIENT_FATAL_ERROR;
    }
  /*NOTREACHED*/
}

CcXstreamClientError cc_xstream_client_upcwd(CcXstreamServerConnection s,
					     unsigned long levels)
{
  unsigned char *pb;
  size_t pb_len;
  unsigned long id;
  CcXstreamReplyPacket rp;

  id = cc_xstream_client_mkpacket_upcwd(levels, &pb, &pb_len);
  if (cc_xstream_client_write_data(s, pb, pb_len, 0) == 0)
    {
      cc_xfree(pb);
      return CC_XSTREAM_CLIENT_FATAL_ERROR;
    }
  cc_xfree(pb);
  if (cc_xstream_client_reply_packet_read(s, &rp) != CC_XSTREAM_CLIENT_OK)
    return CC_XSTREAM_CLIENT_FATAL_ERROR;
  if (rp->id != id)
    {
      cc_xstream_client_reply_packet_free(rp);
      return CC_XSTREAM_CLIENT_FATAL_ERROR;
    }
  else if (rp->type == CC_XSTREAM_XBMSP_PACKET_OK)
    {
      cc_xstream_client_reply_packet_free(rp);
      return CC_XSTREAM_CLIENT_OK;
    }
  else if (rp->type == CC_XSTREAM_XBMSP_PACKET_ERROR)
    {
      cc_xstream_client_reply_packet_free(rp);
      return CC_XSTREAM_CLIENT_COMMAND_FAILED;
    }
  else
    {
      cc_xstream_client_reply_packet_free(rp);
      return CC_XSTREAM_CLIENT_FATAL_ERROR;
    }
  /*NOTREACHED*/
}


CcXstreamClientError cc_xstream_client_close(CcXstreamServerConnection s,
					     unsigned long handle)
{
  unsigned char *pb;
  size_t pb_len;
  unsigned long id;
  CcXstreamReplyPacket rp;

  id = cc_xstream_client_mkpacket_close(handle, &pb, &pb_len);
  if (cc_xstream_client_write_data(s, pb, pb_len, 0) == 0)
    {
      cc_xfree(pb);
      return CC_XSTREAM_CLIENT_FATAL_ERROR;
    }
  cc_xfree(pb);
  if (cc_xstream_client_reply_packet_read(s, &rp) != CC_XSTREAM_CLIENT_OK)
    return CC_XSTREAM_CLIENT_FATAL_ERROR;
  if (rp->id != id)
    {
      cc_xstream_client_reply_packet_free(rp);
      return CC_XSTREAM_CLIENT_FATAL_ERROR;
    }
  else if (rp->type == CC_XSTREAM_XBMSP_PACKET_OK)
    {
      cc_xstream_client_reply_packet_free(rp);
      return CC_XSTREAM_CLIENT_OK;
    }
  else if (rp->type == CC_XSTREAM_XBMSP_PACKET_ERROR)
    {
      cc_xstream_client_reply_packet_free(rp);
      return CC_XSTREAM_CLIENT_COMMAND_FAILED;
    }
  else
    {
      cc_xstream_client_reply_packet_free(rp);
      return CC_XSTREAM_CLIENT_FATAL_ERROR;
    }
  /*NOTREACHED*/
}

CcXstreamClientError cc_xstream_client_close_all(CcXstreamServerConnection s)
{
  unsigned char *pb;
  size_t pb_len;
  unsigned long id;
  CcXstreamReplyPacket rp;

  id = cc_xstream_client_mkpacket_close_all(&pb, &pb_len);
  if (cc_xstream_client_write_data(s, pb, pb_len, 0) == 0)
    {
      cc_xfree(pb);
      return CC_XSTREAM_CLIENT_FATAL_ERROR;
    }
  cc_xfree(pb);
  if (cc_xstream_client_reply_packet_read(s, &rp) != CC_XSTREAM_CLIENT_OK)
    return CC_XSTREAM_CLIENT_FATAL_ERROR;
  if (rp->id != id)
    {
      cc_xstream_client_reply_packet_free(rp);
      return CC_XSTREAM_CLIENT_FATAL_ERROR;
    }
  else if (rp->type == CC_XSTREAM_XBMSP_PACKET_OK)
    {
      cc_xstream_client_reply_packet_free(rp);
      return CC_XSTREAM_CLIENT_OK;
    }
  else if (rp->type == CC_XSTREAM_XBMSP_PACKET_ERROR)
    {
      cc_xstream_client_reply_packet_free(rp);
      return CC_XSTREAM_CLIENT_COMMAND_FAILED;
    }
  else
    {
      cc_xstream_client_reply_packet_free(rp);
      return CC_XSTREAM_CLIENT_FATAL_ERROR;
    }
  /*NOTREACHED*/
}

CcXstreamClientError cc_xstream_client_file_open(CcXstreamServerConnection s,
						 const char *path,
						 unsigned long *handle)
{
  unsigned char *pb;
  size_t pb_len;
  unsigned long id;
  CcXstreamReplyPacket rp;

  id = cc_xstream_client_mkpacket_file_open(path, &pb, &pb_len);
  if (cc_xstream_client_write_data(s, pb, pb_len, 0) == 0)
    {
      cc_xfree(pb);
      return CC_XSTREAM_CLIENT_FATAL_ERROR;
    }
  cc_xfree(pb);
  if (cc_xstream_client_reply_packet_read(s, &rp) != CC_XSTREAM_CLIENT_OK)
    return CC_XSTREAM_CLIENT_FATAL_ERROR;
  if (rp->id != id)
    {
      cc_xstream_client_reply_packet_free(rp);
      return CC_XSTREAM_CLIENT_FATAL_ERROR;
    }
  else if (rp->type == CC_XSTREAM_XBMSP_PACKET_HANDLE)
    {
      *handle = rp->handle;
      cc_xstream_client_reply_packet_free(rp);
      return CC_XSTREAM_CLIENT_OK;
    }
  else if (rp->type == CC_XSTREAM_XBMSP_PACKET_ERROR)
    {
      cc_xstream_client_reply_packet_free(rp);
      return CC_XSTREAM_CLIENT_COMMAND_FAILED;
    }
  else
    {
      cc_xstream_client_reply_packet_free(rp);
      return CC_XSTREAM_CLIENT_FATAL_ERROR;
    }
  /*NOTREACHED*/
}

CcXstreamClientError cc_xstream_client_file_read(CcXstreamServerConnection s,
						 unsigned long handle,
						 size_t len,
						 unsigned char **data,
						 size_t *data_len)
{
  unsigned char *pb;
  size_t pb_len;
  unsigned long id;
  CcXstreamReplyPacket rp;

  id = cc_xstream_client_mkpacket_file_read(handle, len, &pb, &pb_len);
  if (cc_xstream_client_write_data(s, pb, pb_len, 0) == 0)
    {
      cc_xfree(pb);
      return CC_XSTREAM_CLIENT_FATAL_ERROR;
    }
  cc_xfree(pb);
  if (cc_xstream_client_reply_packet_read(s, &rp) != CC_XSTREAM_CLIENT_OK)
    {
      return CC_XSTREAM_CLIENT_FATAL_ERROR;
    }
  if (rp->id != id)
    {
      cc_xstream_client_reply_packet_free(rp);
      return CC_XSTREAM_CLIENT_FATAL_ERROR;
    }
  else if (rp->type == CC_XSTREAM_XBMSP_PACKET_FILE_CONTENTS)
    {
      *data = rp->string1;
      *data_len = rp->string1_len;
      rp->string1 = NULL;
      rp->string1_len = 0;
      cc_xstream_client_reply_packet_free(rp);
      return CC_XSTREAM_CLIENT_OK;
    }
  else if (rp->type == CC_XSTREAM_XBMSP_PACKET_ERROR)
    {
      cc_xstream_client_reply_packet_free(rp);
      return CC_XSTREAM_CLIENT_COMMAND_FAILED;
    }
  else
    {
      cc_xstream_client_reply_packet_free(rp);
      return CC_XSTREAM_CLIENT_FATAL_ERROR;
    }
  /*NOTREACHED*/
}

CcXstreamClientError cc_xstream_client_dir_open(CcXstreamServerConnection s,
						unsigned long *handle)
{
  unsigned char *pb;
  size_t pb_len;
  unsigned long id;
  CcXstreamReplyPacket rp;

  id = cc_xstream_client_mkpacket_filelist_open(&pb, &pb_len);
  if (cc_xstream_client_write_data(s, pb, pb_len, 0) == 0)
    {
      cc_xfree(pb);
      return CC_XSTREAM_CLIENT_FATAL_ERROR;
    }
  cc_xfree(pb);
  if (cc_xstream_client_reply_packet_read(s, &rp) != CC_XSTREAM_CLIENT_OK)
    return CC_XSTREAM_CLIENT_FATAL_ERROR;
  if (rp->id != id)
    {
      cc_xstream_client_reply_packet_free(rp);
      return CC_XSTREAM_CLIENT_FATAL_ERROR;
    }
  else if (rp->type == CC_XSTREAM_XBMSP_PACKET_HANDLE)
    {
      *handle = rp->handle;
      cc_xstream_client_reply_packet_free(rp);
      return CC_XSTREAM_CLIENT_OK;
    }
  else if (rp->type == CC_XSTREAM_XBMSP_PACKET_ERROR)
    {
      cc_xstream_client_reply_packet_free(rp);
      return CC_XSTREAM_CLIENT_COMMAND_FAILED;
    }
  else
    {
      cc_xstream_client_reply_packet_free(rp);
      return CC_XSTREAM_CLIENT_FATAL_ERROR;
    }
  /*NOTREACHED*/
}

CcXstreamClientError cc_xstream_client_dir_read(CcXstreamServerConnection s,
						unsigned long handle,
						char **name,
						char **info)
{
  unsigned char *pb;
  size_t pb_len;
  unsigned long id;
  CcXstreamReplyPacket rp;

  id = cc_xstream_client_mkpacket_filelist_read(handle, &pb, &pb_len);
  if (cc_xstream_client_write_data(s, pb, pb_len, 0) == 0)
    {
      cc_xfree(pb);
      return CC_XSTREAM_CLIENT_FATAL_ERROR;
    }
  cc_xfree(pb);
  if (cc_xstream_client_reply_packet_read(s, &rp) != CC_XSTREAM_CLIENT_OK)
    return CC_XSTREAM_CLIENT_FATAL_ERROR;
  if (rp->id != id)
    {
      cc_xstream_client_reply_packet_free(rp);
      return CC_XSTREAM_CLIENT_FATAL_ERROR;
    }
  else if (rp->type == CC_XSTREAM_XBMSP_PACKET_FILE_DATA)
    {
      *name = (char *)rp->string1;
      *info = (char *)rp->string2;
      rp->string1 = NULL;
      rp->string1_len = 0;
      rp->string2 = NULL;
      rp->string2_len = 0;
      cc_xstream_client_reply_packet_free(rp);
      return CC_XSTREAM_CLIENT_OK;
    }
  else if (rp->type == CC_XSTREAM_XBMSP_PACKET_ERROR)
    {
      cc_xstream_client_reply_packet_free(rp);
      return CC_XSTREAM_CLIENT_COMMAND_FAILED;
    }
  else
    {
      cc_xstream_client_reply_packet_free(rp);
      return CC_XSTREAM_CLIENT_FATAL_ERROR;
    }
  /*NOTREACHED*/
}

CcXstreamClientError cc_xstream_client_file_forward(CcXstreamServerConnection s,
					            unsigned long handle,
					            CC_UINT_64_TYPE_NAME bytes,
					            int seek_eof_if_fails)
{
  unsigned char *pb;
  size_t pb_len;
  unsigned long id;
  CcXstreamReplyPacket rp;

  id = cc_xstream_client_mkpacket_seek(handle, 2, bytes, &pb, &pb_len);
  if (cc_xstream_client_write_data(s, pb, pb_len, 0) == 0)
    {
      cc_xfree(pb);
      return CC_XSTREAM_CLIENT_FATAL_ERROR;
    }
  cc_xfree(pb);
  if (cc_xstream_client_reply_packet_read(s, &rp) != CC_XSTREAM_CLIENT_OK)
    return CC_XSTREAM_CLIENT_FATAL_ERROR;
  if (rp->id != id)
    {
      cc_xstream_client_reply_packet_free(rp);
      return CC_XSTREAM_CLIENT_FATAL_ERROR;
    }
  else if (rp->type == CC_XSTREAM_XBMSP_PACKET_OK)
    {
      cc_xstream_client_reply_packet_free(rp);
      return CC_XSTREAM_CLIENT_OK;
    }
  else if (rp->type == CC_XSTREAM_XBMSP_PACKET_ERROR)
    {
      cc_xstream_client_reply_packet_free(rp);
      if (seek_eof_if_fails)
	return cc_xstream_client_file_end(s, handle);
      else
	return CC_XSTREAM_CLIENT_COMMAND_FAILED;
    }
  else
    {
      cc_xstream_client_reply_packet_free(rp);
      return CC_XSTREAM_CLIENT_FATAL_ERROR;
    }
  /*NOTREACHED*/
}

CcXstreamClientError cc_xstream_client_file_backwards(CcXstreamServerConnection s,
					       	      unsigned long handle,
					              CC_UINT_64_TYPE_NAME bytes,
					              int rewind_if_fails)
{
  unsigned char *pb;
  size_t pb_len;
  unsigned long id;
  CcXstreamReplyPacket rp;

  id = cc_xstream_client_mkpacket_seek(handle, 3, bytes, &pb, &pb_len);
  if (cc_xstream_client_write_data(s, pb, pb_len, 0) == 0)
    {
      cc_xfree(pb);
      return CC_XSTREAM_CLIENT_FATAL_ERROR;
    }
  cc_xfree(pb);
  if (cc_xstream_client_reply_packet_read(s, &rp) != CC_XSTREAM_CLIENT_OK)
    return CC_XSTREAM_CLIENT_FATAL_ERROR;
  if (rp->id != id)
    {
      cc_xstream_client_reply_packet_free(rp);
      return CC_XSTREAM_CLIENT_FATAL_ERROR;
    }
  else if (rp->type == CC_XSTREAM_XBMSP_PACKET_OK)
    {
      cc_xstream_client_reply_packet_free(rp);
      return CC_XSTREAM_CLIENT_OK;
    }
  else if (rp->type == CC_XSTREAM_XBMSP_PACKET_ERROR)
    {
      cc_xstream_client_reply_packet_free(rp);
      if (rewind_if_fails)
	return cc_xstream_client_file_rewind(s, handle);
      else
	return CC_XSTREAM_CLIENT_COMMAND_FAILED;
    }
  else
    {
      cc_xstream_client_reply_packet_free(rp);
      return CC_XSTREAM_CLIENT_FATAL_ERROR;
    }
  /*NOTREACHED*/
}

CcXstreamClientError cc_xstream_client_file_rewind(CcXstreamServerConnection s,
					       	   unsigned long handle)
{
  unsigned char *pb;
  size_t pb_len;
  unsigned long id;
  CcXstreamReplyPacket rp;

  id = cc_xstream_client_mkpacket_seek(handle, 0, 0, &pb, &pb_len);
  if (cc_xstream_client_write_data(s, pb, pb_len, 0) == 0)
    {
      cc_xfree(pb);
      return CC_XSTREAM_CLIENT_FATAL_ERROR;
    }
  cc_xfree(pb);
  if (cc_xstream_client_reply_packet_read(s, &rp) != CC_XSTREAM_CLIENT_OK)
    return CC_XSTREAM_CLIENT_FATAL_ERROR;
  if (rp->id != id)
    {
      cc_xstream_client_reply_packet_free(rp);
      return CC_XSTREAM_CLIENT_FATAL_ERROR;
    }
  else if (rp->type == CC_XSTREAM_XBMSP_PACKET_OK)
    {
      cc_xstream_client_reply_packet_free(rp);
      return CC_XSTREAM_CLIENT_OK;
    }
  else if (rp->type == CC_XSTREAM_XBMSP_PACKET_ERROR)
    {
      cc_xstream_client_reply_packet_free(rp);
      return CC_XSTREAM_CLIENT_COMMAND_FAILED;
    }
  else
    {
      cc_xstream_client_reply_packet_free(rp);
      return CC_XSTREAM_CLIENT_FATAL_ERROR;
    }
  /*NOTREACHED*/
}

CcXstreamClientError cc_xstream_client_file_end(CcXstreamServerConnection s,
					       	   unsigned long handle)
{
  unsigned char *pb;
  size_t pb_len;
  unsigned long id;
  CcXstreamReplyPacket rp;

  id = cc_xstream_client_mkpacket_seek(handle, 1, 0, &pb, &pb_len);
  if (cc_xstream_client_write_data(s, pb, pb_len, 0) == 0)
    {
      cc_xfree(pb);
      return CC_XSTREAM_CLIENT_FATAL_ERROR;
    }
  cc_xfree(pb);
  if (cc_xstream_client_reply_packet_read(s, &rp) != CC_XSTREAM_CLIENT_OK)
    return CC_XSTREAM_CLIENT_FATAL_ERROR;
  if (rp->id != id)
    {
      cc_xstream_client_reply_packet_free(rp);
      return CC_XSTREAM_CLIENT_FATAL_ERROR;
    }
  else if (rp->type == CC_XSTREAM_XBMSP_PACKET_OK)
    {
      cc_xstream_client_reply_packet_free(rp);
      return CC_XSTREAM_CLIENT_OK;
    }
  else if (rp->type == CC_XSTREAM_XBMSP_PACKET_ERROR)
    {
      cc_xstream_client_reply_packet_free(rp);
      return CC_XSTREAM_CLIENT_COMMAND_FAILED;
    }
  else
    {
      cc_xstream_client_reply_packet_free(rp);
      return CC_XSTREAM_CLIENT_FATAL_ERROR;
    }
  /*NOTREACHED*/
}

CcXstreamClientError cc_xstream_client_file_info(CcXstreamServerConnection s,
						 const char *path,
       						 char **info)
{
  unsigned char *pb;
  size_t pb_len;
  unsigned long id;
  CcXstreamReplyPacket rp;

  id = cc_xstream_client_mkpacket_file_info(path, &pb, &pb_len);
  if (cc_xstream_client_write_data(s, pb, pb_len, 0) == 0)
    {
      cc_xfree(pb);
      return CC_XSTREAM_CLIENT_FATAL_ERROR;
    }
  cc_xfree(pb);
  if (cc_xstream_client_reply_packet_read(s, &rp) != CC_XSTREAM_CLIENT_OK)
    return CC_XSTREAM_CLIENT_FATAL_ERROR;
  if (rp->id != id)
    {
      cc_xstream_client_reply_packet_free(rp);
      return CC_XSTREAM_CLIENT_FATAL_ERROR;
    }
  else if (rp->type == CC_XSTREAM_XBMSP_PACKET_FILE_DATA)
    {
      *info = (char *)rp->string2;
      rp->string2 = NULL;
      rp->string2_len = 0;
      cc_xstream_client_reply_packet_free(rp);
      return CC_XSTREAM_CLIENT_OK;
    }
  else if (rp->type == CC_XSTREAM_XBMSP_PACKET_ERROR)
    {
      cc_xstream_client_reply_packet_free(rp);
      return CC_XSTREAM_CLIENT_COMMAND_FAILED;
    }
  else
    {
      cc_xstream_client_reply_packet_free(rp);
      return CC_XSTREAM_CLIENT_FATAL_ERROR;
    }
  /*NOTREACHED*/
}

CcXstreamClientError cc_xstream_client_set_configuration_option(CcXstreamServerConnection s,
								const char *option,
								const char *value)
{
  unsigned char *pb;
  size_t pb_len;
  unsigned long id;
  CcXstreamReplyPacket rp;

  id = cc_xstream_client_mkpacket_setconfoption(option, value, &pb, &pb_len);
  if (cc_xstream_client_write_data(s, pb, pb_len, 0) == 0)
    {
      cc_xfree(pb);
      return CC_XSTREAM_CLIENT_FATAL_ERROR;
    }
  cc_xfree(pb);
  if (cc_xstream_client_reply_packet_read(s, &rp) != CC_XSTREAM_CLIENT_OK)
    return CC_XSTREAM_CLIENT_FATAL_ERROR;
  if (rp->id != id)
    {
      cc_xstream_client_reply_packet_free(rp);
      return CC_XSTREAM_CLIENT_FATAL_ERROR;
    }
  else if (rp->type == CC_XSTREAM_XBMSP_PACKET_OK)
    {
      cc_xstream_client_reply_packet_free(rp);
      return CC_XSTREAM_CLIENT_OK;
    }
  else if (rp->type == CC_XSTREAM_XBMSP_PACKET_ERROR)
    {
      cc_xstream_client_reply_packet_free(rp);
      return CC_XSTREAM_CLIENT_COMMAND_FAILED;
    }
  else
    {
      cc_xstream_client_reply_packet_free(rp);
      return CC_XSTREAM_CLIENT_FATAL_ERROR;
    }
  /*NOTREACHED*/
}

CcXstreamClientError cc_xstream_client_password_authenticate(CcXstreamServerConnection s,
							     const char *user_id,
							     const char *password)
{
  unsigned char *pb;
  size_t pb_len;
  unsigned long id;
  CcXstreamReplyPacket rp;
  unsigned int handle;

  id = cc_xstream_client_mkpacket_authentication_init("password", &pb, &pb_len);
  if (cc_xstream_client_write_data(s, pb, pb_len, 0) == 0)
    {
      cc_xfree(pb);
      return CC_XSTREAM_CLIENT_FATAL_ERROR;
    }
  cc_xfree(pb);
  if (cc_xstream_client_reply_packet_read(s, &rp) != CC_XSTREAM_CLIENT_OK)
    {
      return CC_XSTREAM_CLIENT_FATAL_ERROR;
    }
  if (rp->id != id)
    {
      cc_xstream_client_reply_packet_free(rp);
      return CC_XSTREAM_CLIENT_FATAL_ERROR;
    }
  else if (rp->type == CC_XSTREAM_XBMSP_PACKET_HANDLE)
    {
      handle = rp->handle;
      cc_xstream_client_reply_packet_free(rp);
    }
  else
    {
      cc_xstream_client_reply_packet_free(rp);
      return CC_XSTREAM_CLIENT_FATAL_ERROR;
    }
  id = cc_xstream_client_mkpacket_authenticate_password(handle, user_id, password, &pb, &pb_len);
  if (cc_xstream_client_write_data(s, pb, pb_len, 0) == 0)
    {
      cc_xfree(pb);
      return CC_XSTREAM_CLIENT_FATAL_ERROR;
    }
  cc_xfree(pb);
  if (cc_xstream_client_reply_packet_read(s, &rp) != CC_XSTREAM_CLIENT_OK)
    {
      return CC_XSTREAM_CLIENT_FATAL_ERROR;
    }
  if (rp->id != id)
    {
      cc_xstream_client_reply_packet_free(rp);
      return CC_XSTREAM_CLIENT_FATAL_ERROR;
    }
  else if (rp->type == CC_XSTREAM_XBMSP_PACKET_OK)
    {
      cc_xstream_client_reply_packet_free(rp);
      return CC_XSTREAM_CLIENT_OK;
    }
  else if (rp->type == CC_XSTREAM_XBMSP_PACKET_ERROR)
    {
      cc_xstream_client_reply_packet_free(rp);
      return CC_XSTREAM_CLIENT_COMMAND_FAILED;
    }
  else
    {
      cc_xstream_client_reply_packet_free(rp);
      return CC_XSTREAM_CLIENT_FATAL_ERROR;
    }
  /*NOTREACHED*/
}

/* eof (ccxclient.c) */
