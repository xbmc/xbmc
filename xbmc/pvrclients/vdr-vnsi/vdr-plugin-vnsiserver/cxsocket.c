/*
 *      vdr-plugin-vnsi - XBMC server plugin for VDR
 *
 *      Copyright (C) 2003-2006 Petri Hintukainen
 *      Copyright (C) 2010 Alwin Esch (Team XBMC)
 *      Copyright (C) 2011 Alexander Pipelka
 *
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

/*
 * Socket wrapper classes
 *
 * Code is taken from xineliboutput plugin.
 *
 */

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/tcp.h>

#include <vdr/config.h>
#include <vdr/tools.h>

#include "config.h"
#include "cxsocket.h"

cxSocket::~cxSocket()
{
  close();
  delete m_pollerRead;
  delete m_pollerWrite;
}

void cxSocket::close() {
  if(m_fd >= 0) { 
    ::close(m_fd);
    m_fd=-1; 
  }
}

ssize_t cxSocket::write(const void *buffer, size_t size, int timeout_ms, bool more_data)
{
  cMutexLock CmdLock((cMutex*)&m_MutexWrite);

  if(m_fd == -1) return -1;

  ssize_t written = (ssize_t)size;
  const unsigned char *ptr = (const unsigned char *)buffer;

  while (size > 0)
  {
    if(!m_pollerWrite->Poll(timeout_ms))
    {
      ERRORLOG("cxSocket::write: poll() failed");
      return written-size;
    }

    ssize_t p = ::send(m_fd, ptr, size, MSG_NOSIGNAL | (more_data ? MSG_MORE : 0));

    if (p <= 0)
    {
      if (errno == EINTR || errno == EAGAIN)
      {
        DEBUGLOG("cxSocket::write: EINTR during write(), retrying");
        continue;
      }
      ERRORLOG("cxSocket::write: write() error");
      return p;
    }

    ptr  += p;
    size -= p;
  }

  return written;
}

ssize_t cxSocket::read(void *buffer, size_t size, int timeout_ms)
{
  cMutexLock CmdLock((cMutex*)&m_MutexRead);

  int retryCounter = 0;

  if(m_fd == -1) return -1;

  ssize_t missing = (ssize_t)size;
  unsigned char *ptr = (unsigned char *)buffer;

  while (missing > 0)
  {
    if(!m_pollerRead->Poll(timeout_ms))
    {
      ERRORLOG("cxSocket::read: poll() failed at %d/%d", (int)(size-missing), (int)size);
      return size-missing;
    }

    ssize_t p = ::read(m_fd, ptr, missing);

    if (p <= 0)
    {
      if (retryCounter < 10 && (errno == EINTR || errno == EAGAIN))
      {
        DEBUGLOG("cxSocket::read: EINTR/EAGAIN during read(), retrying");
        retryCounter++;
        continue;
      }
      ERRORLOG("cxSocket::read: read() error at %d/%d", (int)(size-missing), (int)size);
      return size-missing;
    }

    retryCounter = 0;
    ptr  += p;
    missing -= p;
  }

  return size;
}

void cxSocket::set_handle(int h) {
  if(h != m_fd) {
    close();
    m_fd = h;
    delete m_pollerRead;
    delete m_pollerWrite;
    m_pollerRead = new cPoller(m_fd);
    m_pollerWrite = new cPoller(m_fd, true);
  }
}

#include <sys/ioctl.h>
#include <net/if.h>

char *cxSocket::ip2txt(uint32_t ip, unsigned int port, char *str)
{
  // inet_ntoa is not thread-safe (?)
  if(str) {
    unsigned int iph =(unsigned int)ntohl(ip);
    unsigned int porth =(unsigned int)ntohs(port);
    if(!porth)
      sprintf(str, "%d.%d.%d.%d",
	      ((iph>>24)&0xff), ((iph>>16)&0xff),
	      ((iph>>8)&0xff), ((iph)&0xff));
    else
      sprintf(str, "%u.%u.%u.%u:%u",
	      ((iph>>24)&0xff), ((iph>>16)&0xff),
	      ((iph>>8)&0xff), ((iph)&0xff),
	      porth);
  }
  return str;
}
