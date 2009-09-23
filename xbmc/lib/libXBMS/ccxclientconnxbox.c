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

#ifndef _XBOX
#define CC_XSTREAM_SOCKET_FD_TYPE          int
#define CC_XSTREAM_SOCKET_CLOSE            close
#define CC_XSTREAM_SOCKET_WRITE(s, b, l)   write((s), (b), (l))
#define CC_XSTREAM_SOCKET_READ(s, b, l)    read((s), (b), (l))
#else /* ! _XBOX */
#define CC_XSTREAM_SOCKET_FD_TYPE          SOCKET
#define CC_XSTREAM_SOCKET_CLOSE            closesocket
#define CC_XSTREAM_SOCKET_WRITE(s, b, l)   send((s), (b), (l), 0)
#define CC_XSTREAM_SOCKET_READ(s, b, l)    recv((s), (b), (l), 0)
#endif /* ! _XBOX */

static void cc_xstream_client_socket_setup(CC_XSTREAM_SOCKET_FD_TYPE sock)
{
#ifdef TCP_NODELAY
  int i;

  i = 1;
  setsockopt(sock, IPPROTO_TCP,TCP_NODELAY, (char *)(&i), sizeof (i));
#endif /* TCP_NODELAY */
}

CcXstreamClientError cc_xstream_client_connect(const char *host,
					       int port,
					       CcXstreamServerConnection *s)
{
  CC_XSTREAM_SOCKET_FD_TYPE sock;
  struct sockaddr_in sa;
#ifndef _XBOX
  struct hostent *he;
#endif /* ! _XBOX */

  memset(&sa, 0, sizeof (sa));
  sa.sin_port = htons(port);
  sa.sin_family = AF_INET;
  if (inet_addr(host) == INADDR_NONE)
    {
#ifndef _XBOX
      he = gethostbyname(host);
      if ((he == NULL) || (he->h_addrtype != AF_INET) || (he->h_length != 4))
	return CC_XSTREAM_CLIENT_SERVER_NOT_FOUND;
      memcpy(&(sa.sin_addr), he->h_addr, 4);
#else  /* ! _XBOX */
      return CC_XSTREAM_CLIENT_SERVER_NOT_FOUND;
#endif  /* ! _XBOX */
    }
  else
    {
      sa.sin_addr.s_addr = inet_addr(host);
    }
  sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (sock < 0)
    return CC_XSTREAM_CLIENT_FATAL_ERROR;
  if (connect(sock, (struct sockaddr *)&sa, sizeof (sa)) != 0)
    {
      CC_XSTREAM_SOCKET_CLOSE(sock);
      return CC_XSTREAM_CLIENT_SERVER_CONNECTION_FAILED;
    }
  cc_xstream_client_socket_setup(sock);
  *s = (CcXstreamServerConnection)sock;
  return CC_XSTREAM_CLIENT_OK;
}

CcXstreamClientError cc_xstream_client_disconnect(CcXstreamServerConnection s)
{
  CC_XSTREAM_SOCKET_FD_TYPE sock;

  sock = (CC_XSTREAM_SOCKET_FD_TYPE)s;
  CC_XSTREAM_SOCKET_CLOSE(s);
  return CC_XSTREAM_CLIENT_OK;
}

unsigned char *cc_xstream_client_read_data(CcXstreamServerConnection s, 
					   size_t len, 
					   unsigned long timeout_ms)
{
  unsigned char *buf;
  size_t done;
  CC_XSTREAM_SOCKET_FD_TYPE sock;
  int rv;
  struct timeval tv;
  fd_set fds;

  /* Convert the conenction handle to simple file descriptor. */
  sock = (CC_XSTREAM_SOCKET_FD_TYPE)s;

  /* We terminate incoming buffer just to make code safer. 
     Caller should not count on it anyway. */
  buf = cc_xmalloc(len + 1);
  buf[len] = '\0'; 

  for (done = 0; done < len; /*NOTHING*/)
    {
      if (timeout_ms > 0)
	{
	  tv.tv_sec = timeout_ms / 1000U;
	  tv.tv_usec = (timeout_ms % 1000U) * 1000U;
	  FD_ZERO(&fds);
	  FD_SET(sock, &fds);
	  if (select(sock + 1, &fds, NULL, NULL, &tv) != 1)
	    {
	      /* Timeout or error, we don't really care. */
	      cc_xfree(buf);
	      return NULL;
	    }
	}
      rv = CC_XSTREAM_SOCKET_READ(sock, buf + done, len - done);
      if (rv < 1)
	{
	  cc_xfree(buf);
	  return NULL;
	}
      done += rv;
    }
  return buf;
}

int cc_xstream_client_write_data(CcXstreamServerConnection s,
				 unsigned char *buf,
				 size_t len,
				 unsigned long timeout_ms)
{
  size_t done;
  CC_XSTREAM_SOCKET_FD_TYPE sock;
  int rv;
  struct timeval tv;
  fd_set fds;

  /* Convert the conenction handle to simple file descriptor. */
  sock = (CC_XSTREAM_SOCKET_FD_TYPE)s;

  for (done = 0; done < len; /*NOTHING*/)
    {
      if (timeout_ms > 0)
	{
	  tv.tv_sec = timeout_ms / 1000U;
	  tv.tv_usec = (timeout_ms % 1000U) * 1000U;
	  FD_ZERO(&fds);
	  FD_SET(sock, &fds);
	  if (select(sock + 1, NULL, &fds, NULL, &tv) != 1)
	    {
	      /* Timeout or error, we don't really care. */
	      cc_xfree(buf);
	      return 0;
	    }
	}
      rv = CC_XSTREAM_SOCKET_WRITE(sock, buf + done, len - done);
      if (rv < 1)
	return 0;
      done += rv;
    }
  return 1;
}

/* eof (ccxclientconn.c) */
