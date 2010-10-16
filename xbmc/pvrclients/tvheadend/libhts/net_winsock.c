/*
 *  Networking under POSIX
 *  Copyright (C) 2007-2008 Andreas Öman
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include <winsock2.h>
#include <Ws2tcpip.h>
#include "msvc.h"
#include "net.h"

#define EINPROGRESS WSAEINPROGRESS
#define ECONNRESET  WSAECONNRESET
#define ETIMEDOUT   WSAETIMEDOUT
#define EAGAIN      WSAEWOULDBLOCK

#define ERANGE 34

static int gethostbyname_r(const char *name,
                           struct hostent *ret, char *buf, size_t buflen,
                           struct hostent **result, int *h_errnop)
{
  memset(buf, 0, buflen);

  *result = gethostbyname(name);
  if(*result)
  {
    memcpy(ret, *result, sizeof(struct hostent));
    *result = ret;
    *h_errnop = 0;
  }
  else
    *h_errnop = HOST_NOT_FOUND;

  return 0;
}

#ifndef MSG_WAITALL
#define MSG_WAITALL 0x8
#endif

static int recv_fixed (SOCKET s, char * buf, int len, int flags)
{
  char* org = buf;
  int   res = 1;

  if((flags & MSG_WAITALL) == 0)
    return recv(s, buf, len, flags);

  flags &= ~MSG_WAITALL;
  while(len > 0 && res > 0)
  {
    res = recv(s, buf, len, flags);
    if(res < 0)
      return res;

    buf += res;
    len -= res;
  }
  return buf - org;
}
#define recv(s, buf, len, flags) recv_fixed(s, buf, len, flags)



/**
 *
 */
socket_t
htsp_tcp_connect(const char *hostname, int port, char *errbuf, size_t errbufsize,
	    int timeout)
{
  const char *errtxt;
  struct hostent hostbuf, *hp;
  char *tmphstbuf;
  size_t hstbuflen;
  socket_t fd;
  int herr, r, res, err, val;
  struct sockaddr_in6 in6;
  struct sockaddr_in in;
  socklen_t errlen = sizeof(int);

  hstbuflen = 1024;
  tmphstbuf = malloc(hstbuflen);

  while((res = gethostbyname_r(hostname, &hostbuf, tmphstbuf, hstbuflen,
			       &hp, &herr)) == ERANGE) {
    hstbuflen *= 2;
    tmphstbuf = realloc(tmphstbuf, hstbuflen);
  }

  if(res != 0) {
    snprintf(errbuf, errbufsize, "Resolver internal error");
    free(tmphstbuf);
    return -1;
  } else if(herr != 0) {
    switch(herr) {
    case HOST_NOT_FOUND:
      errtxt = "The specified host is unknown";
      break;
    case NO_ADDRESS:
      errtxt = "The requested name is valid but does not have an IP address";
      break;

    case NO_RECOVERY:
      errtxt = "A non-recoverable name server error occurred";
      break;

    case TRY_AGAIN:
      errtxt = "A temporary error occurred on an authoritative name server";
      break;

    default:
      errtxt = "Unknown error";
      break;
    }

    snprintf(errbuf, errbufsize, "%s", errtxt);
    free(tmphstbuf);
    return -1;
  } else if(hp == NULL) {
    snprintf(errbuf, errbufsize, "Resolver internal error");
    free(tmphstbuf);
    return -1;
  }
  fd = socket(hp->h_addrtype, SOCK_STREAM, 0);
  if(fd == -1) {
    snprintf(errbuf, errbufsize, "Unable to create socket: %s",
	     strerror(WSAGetLastError()));
    free(tmphstbuf);
    return -1;
  }

  /**
   * Switch to nonblocking
   */
  val = 1;
  ioctlsocket(fd, FIONBIO, &val);

  switch(hp->h_addrtype) {
  case AF_INET:
    memset(&in, 0, sizeof(in));
    in.sin_family = AF_INET;
    in.sin_port = htons(port);
    memcpy(&in.sin_addr, hp->h_addr_list[0], sizeof(struct in_addr));
    r = connect(fd, (struct sockaddr *)&in, sizeof(struct sockaddr_in));
    break;

  case AF_INET6:
    memset(&in6, 0, sizeof(in6));
    in6.sin6_family = AF_INET6;
    in6.sin6_port = htons(port);
    memcpy(&in6.sin6_addr, hp->h_addr_list[0], sizeof(struct in6_addr));
    r = connect(fd, (struct sockaddr *)&in, sizeof(struct sockaddr_in6));
    break;

  default:
    snprintf(errbuf, errbufsize, "Invalid protocol family");
    free(tmphstbuf);
    return -1;
  }

  free(tmphstbuf);

  if(r == -1) {
    if(WSAGetLastError() == EINPROGRESS ||
       WSAGetLastError() == EAGAIN) {
      fd_set fd_write, fd_except;
      struct timeval tv;

      tv.tv_sec  =         timeout / 1000;
      tv.tv_usec = 1000 * (timeout % 1000);

      FD_ZERO(&fd_write);
      FD_ZERO(&fd_except);

      FD_SET(fd, &fd_write);
      FD_SET(fd, &fd_except);

      r = select((int)fd+1, NULL, &fd_write, &fd_except, &tv);

      if(r == 0) {
        /* Timeout */
        snprintf(errbuf, errbufsize, "Connection attempt timed out");
        closesocket(fd);
        return -1;
      }

      if(r == -1) {
        snprintf(errbuf, errbufsize, "select() error: %s", strerror(WSAGetLastError()));
        closesocket(fd);
        return -1;
      }

      getsockopt(fd, SOL_SOCKET, SO_ERROR, (void *)&err, &errlen);
    } else {
      err = WSAGetLastError();
    }
  } else {
    err = 0;
  }

  if(err != 0) {
    snprintf(errbuf, errbufsize, "%s", strerror(err));
    closesocket(fd);
    return -1;
  }

  val = 0;
  ioctlsocket(fd, FIONBIO, &val);

  val = 1;
  setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (const char*)&val, sizeof(val));

  return fd;
}


/**
 *
 */
int
htsp_tcp_write_queue(socket_t fd, htsbuf_queue_t *q)
{
  htsbuf_data_t *hd;
  int l, r;

  while((hd = TAILQ_FIRST(&q->hq_q)) != NULL) {
    TAILQ_REMOVE(&q->hq_q, hd, hd_link);

    l = hd->hd_data_len - hd->hd_data_off;
    r = send(fd, hd->hd_data + hd->hd_data_off, l, 0);
    free(hd->hd_data);
    free(hd);
  }
  q->hq_size = 0;
  return 0;
}


/**
 *
 */
static int
tcp_fill_htsbuf_from_fd(socket_t fd, htsbuf_queue_t *hq)
{
  htsbuf_data_t *hd = TAILQ_LAST(&hq->hq_q, htsbuf_data_queue);
  int c;

  if(hd != NULL) {
    /* Fill out any previous buffer */
    c = hd->hd_data_size - hd->hd_data_len;

    if(c > 0) {

      c = recv(fd, hd->hd_data + hd->hd_data_len, c, MSG_WAITALL);
      if(c < 1)
	return -1;

      hd->hd_data_len += c;
      hq->hq_size += c;
      return 0;
    }
  }

  hd = malloc(sizeof(htsbuf_data_t));

  hd->hd_data_size = 1000;
  hd->hd_data = malloc(hd->hd_data_size);

  c = recv(fd, hd->hd_data, hd->hd_data_size, MSG_WAITALL);
  if(c < 1) {
    free(hd->hd_data);
    free(hd);
    return -1;
  }
  hd->hd_data_len = c;
  hd->hd_data_off = 0;
  TAILQ_INSERT_TAIL(&hq->hq_q, hd, hd_link);
  hq->hq_size += c;
  return 0;
}


/**
 *
 */
int
htsp_tcp_read_line(socket_t fd, char *buf, const size_t bufsize, htsbuf_queue_t *spill)
{
  int len;

  while(1) {
    len = htsbuf_find(spill, 0xa);

    if(len == -1) {
      if(tcp_fill_htsbuf_from_fd(fd, spill) < 0)
	return -1;
      continue;
    }

    if(len >= (int)bufsize - 1)
      return -1;

    htsbuf_read(spill, buf, len);
    buf[len] = 0;
    while(len > 0 && buf[len - 1] < 32)
      buf[--len] = 0;
    htsbuf_drop(spill, 1); /* Drop the \n */
    return 0;
  }
}


/**
 *
 */
int
htsp_tcp_read_data(socket_t fd, char *buf, const size_t bufsize, htsbuf_queue_t *spill)
{
  int x, tot = htsbuf_read(spill, buf, bufsize);

  if(tot == bufsize)
    return 0;

  x = recv(fd, buf + tot, bufsize - tot, MSG_WAITALL);
  if(x != bufsize - tot)
    return -1;

  return 0;
}

/**
 *
 */
int
htsp_tcp_read(socket_t fd, void *buf, size_t len)
{
  int x = recv(fd, buf, len, MSG_WAITALL);

  if(x == -1)
    return WSAGetLastError();
  if(x != len)
    return ECONNRESET;
  return 0;

}

/**
 *
 */
int
htsp_tcp_read_timeout(socket_t fd, char *buf, size_t len, int timeout)
{
  int x, tot = 0, val, err;
  fd_set fd_read;
  struct timeval tv;

  assert(timeout > 0);

  while(tot != len) {

    tv.tv_sec  =         timeout / 1000;
    tv.tv_usec = 1000 * (timeout % 1000);

    FD_ZERO(&fd_read);
    FD_SET(fd, &fd_read);

    x = select((int)fd+1, &fd_read, NULL, NULL, &tv);

    if(x == 0)
      return ETIMEDOUT;

    val = 1;
    ioctlsocket(fd, FIONBIO, &val);

    x   = recv(fd, buf + tot, len - tot, 0);
    err = WSAGetLastError();

    val = 0;
    ioctlsocket(fd, FIONBIO, &val);

    if(x == 0)
      return ECONNRESET;
    else if(x == -1)
    {
      if(err == EAGAIN)
        continue;
      return err;
    }

    tot += x;
  }
  return 0;
}

/**
 *
 */
void
htsp_tcp_close(socket_t fd)
{
  closesocket(fd);
}
