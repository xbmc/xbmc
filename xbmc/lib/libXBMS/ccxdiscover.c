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
 *  CcXstream Client Library for XBMC Media Center (Server Discovery)
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

static unsigned long timeout_start()
{
#if defined (__linux__) || defined(__APPLE__) || defined (__NetBSD__) || defined (__FreeBSD__) || defined (__CYGWIN__) || defined (sun)
  struct timeval tv;
  unsigned long r;

  gettimeofday(&tv, NULL);
  r = (((unsigned long)tv.tv_sec) % 200000000U) * 10U;
  r += ((unsigned long)tv.tv_usec) / 100000U;
  return r;
#elif defined (_XBOX)
  return ((unsigned long)(GetTickCount()) / 100U);
#else
  time_t t;

  t = time(NULL);
  return (((unsigned long)t) % 200000000U) * 10U;
#endif
}

static int timeout_exceeded(unsigned long start_time)
{
  unsigned long t;

  t = timeout_start();
  return ((t < start_time) || ((start_time + 9) < t));
}

CcXstreamClientError ccx_client_discover_servers(CcXstreamServerDiscoveryCB callback, void *context)
{
  struct sockaddr_in sa, ra;
#if defined (_XBOX) || defined (WIN32)
  size_t sa_len, ra_len;
#else
  socklen_t sa_len, ra_len;
#endif
  unsigned long handle, p_len, p_handle;
  unsigned char *packet, ch;
  size_t packet_len;
  int found = 0, l, c;
#if defined (_XBOX) || defined (WIN32)
  SOCKET sock;
#else
  int sock;
#endif
  unsigned long t0;
  fd_set rs;
  struct timeval tv;
  CcBufferRec buf[1], seen_buf[1];
  char *p_address, *p_port, *p_version, *p_comment;

  memset(&sa, 0, sizeof (sa));
  sa.sin_family = AF_INET;
  sa.sin_addr.s_addr = htonl(INADDR_BROADCAST);
  sa.sin_port = htons(CC_XSTREAM_DEFAULT_PORT);
  sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (sock < 0)
    return CC_XSTREAM_CLIENT_COMMAND_FAILED;
  c = 1;
  setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (char *)(&c), sizeof (c));
  handle = cc_xstream_client_mkpacket_server_discovery(&packet, &packet_len);
  sa_len = sizeof (sa);
  sendto(sock,  packet, packet_len, 0, (struct sockaddr *)(&sa), sa_len);
  t0 = timeout_start();
  cc_buffer_init(seen_buf);
  while (1)
    {
      if (timeout_exceeded(t0))
	{
	  cc_buffer_uninit(seen_buf);
	  break;
	}
      FD_ZERO(&rs);
      FD_SET(sock, &rs);
      tv.tv_sec = 0;
      tv.tv_usec = 350000;
      switch (select(sock + 1, &rs, NULL, NULL, &tv))
	{
	case -1:
#if defined (_XBOX) || defined (WIN32)
	  closesocket(sock);
#else
	  close(sock);
#endif
	  cc_buffer_uninit(seen_buf);
	  cc_xfree(packet);
	  return (found > 0) ? CC_XSTREAM_CLIENT_OK : CC_XSTREAM_CLIENT_COMMAND_FAILED;
	  /*NOTREACHED*/
	  
	case 0:
	  /* Resend packet if we got timeout. */
	  sendto(sock,  packet, packet_len, 0, (struct sockaddr *)(&sa), sa_len);
	  break;

	default:
	  memset(&ra, 0, sizeof (ra));
	  cc_buffer_init(buf);
	  cc_buffer_append_space(buf, 2000);
	  ra_len = sizeof (ra);
	  l = recvfrom(sock, cc_buffer_ptr(buf), cc_buffer_len(buf), 0, (struct sockaddr *)(&ra), &ra_len);
	  if (l > 0)
	    {
	      cc_buffer_consume_end(buf, cc_buffer_len(buf) - l);
	      if (cc_xstream_buffer_decode_int(buf, &p_len) &&
		  (p_len == cc_buffer_len(buf)) &&
		  cc_xstream_buffer_decode_byte(buf, &ch) &&
		  ((CcXstreamPacket)ch == CC_XSTREAM_XBMSP_PACKET_SERVER_DISCOVERY_REPLY) &&
		  cc_xstream_buffer_decode_int(buf, &p_handle) &&
		  (p_handle == handle) &&
		  cc_xstream_buffer_decode_string(buf, (unsigned char **)(&p_address), NULL) &&
		  cc_xstream_buffer_decode_string(buf, (unsigned char **)(&p_port), NULL) &&
		  cc_xstream_buffer_decode_string(buf, (unsigned char **)(&p_version), NULL) &&
		  cc_xstream_buffer_decode_string(buf, (unsigned char **)(&p_comment), NULL) &&
		  (cc_buffer_len(buf) == 0))
		{
		  if (strlen(p_address) == 0)
		    {
		      cc_xfree(p_address);
#ifdef _XBOX
		      {
			unsigned char *b;

			b = (unsigned char *)(&(ra.sin_addr.s_addr));
			p_address = cc_xmalloc(32);
			sprintf(p_address, "%d.%d.%d.%d", (int)(b[0]), (int)(b[1]), (int)(b[2]), (int)(b[3]));
		      }
#else
		      p_address = cc_xstrdup(inet_ntoa(ra.sin_addr));
#endif
		    }
		  if (strlen(p_port) == 0)
		    {
		      cc_xfree(p_port);
		      p_port = cc_xmalloc(16);
#ifdef _XBOX
		      sprintf(p_port, "%d", (int)(ntohs(ra.sin_port)));
#else
		      snprintf(p_port, 16, "%d", (int)(ntohs(ra.sin_port)));
#endif
		    }
		  cc_buffer_append_string(buf, ">");
		  cc_buffer_append_string(buf, p_address);
		  cc_buffer_append_string(buf, ":");
		  cc_buffer_append_string(buf, p_port);
		  cc_buffer_append_string(buf, "<");
		  ch = 0;
		  /* Terminate both buffers with '\0' */
		  cc_buffer_append(buf, &ch, 1);
		  cc_buffer_append(seen_buf, &ch, 1);
		  if (strstr((char *)cc_buffer_ptr(seen_buf), (char *)cc_buffer_ptr(buf)) == NULL)
		    {
		      /* Don't forget to remove the terminating '\0' */
		      cc_buffer_consume_end(seen_buf, 1);
		      cc_buffer_append_string(seen_buf, (char *)cc_buffer_ptr(buf));
		      if (callback != NULL)
			(*callback)(p_address, p_port, p_version, p_comment, context);
		      found++;
		    }
		  else
		    {
		      /* Don't forget to remove the terminating '\0' */
		      cc_buffer_consume_end(seen_buf, 1);
		    }
		  cc_buffer_clear(buf);
		  cc_xfree(p_address);
		  cc_xfree(p_port);
		  cc_xfree(p_version);
		  cc_xfree(p_comment);
		}
	    }
	  cc_buffer_uninit(buf);
	  break;
	}
    }
#if defined (_XBOX) || defined (WIN32)
  closesocket(sock);
#else
  close(sock);
#endif
  cc_xfree(packet);

  return (found > 0) ? CC_XSTREAM_CLIENT_OK : CC_XSTREAM_CLIENT_SERVER_NOT_FOUND;
}

/* eof (ccxdiscover.c) */
