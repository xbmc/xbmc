/*
 *  Networking under WINDOWS
 *  Copyright (C) 2007-2008 Andreas Öman
 *  Copyright (C) 2007-2008 Joakim Plate
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "../../lib/libTcpSocket/os-dependent_socket.h"
#include <winsock2.h>
#include <Ws2tcpip.h>
#include "msvc.h"
#include "net.h"

//#define EINPROGRESS WSAEINPROGRESS
//#define ECONNRESET  WSAECONNRESET
//#define ETIMEDOUT   WSAETIMEDOUT
//#define EAGAIN      WSAEWOULDBLOCK

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
