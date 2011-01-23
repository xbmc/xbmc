/*
 *      Copyright (C) 2010 Alwin Esch (Team XBMC)
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
#ifndef __APPLE__
# include <sys/sendfile.h>
#endif
#include <netinet/tcp.h>

#include <vdr/config.h>
#include <vdr/tools.h>

#include "config.h"
#include "cxsocket.h"

cxSocket::~cxSocket()
{
  CLOSESOCKET(m_fd);
  delete m_pollerRead;
  delete m_pollerWrite;
}

bool cxSocket::connect(struct sockaddr *addr, socklen_t len)
{
  return ::connect(m_fd, addr, len) == 0;
}

bool cxSocket::connect(const char *ip, int port)
{
  struct sockaddr_in sin;
  sin.sin_family = AF_INET;
  sin.sin_port = htons(port);
  sin.sin_addr.s_addr = inet_addr(ip);
  return connect((struct sockaddr *)&sin, sizeof(sin));
}

bool cxSocket::set_blocking(bool state)
{
  int flags = fcntl (m_fd, F_GETFL);

  if(flags == -1)
  {
    esyslog("VNSI-Error: cxSocket::SetBlocking: fcntl(F_GETFL) failed");
    return false;
  }

  flags = state ? (flags&(~O_NONBLOCK)) : (flags|O_NONBLOCK);

  if(fcntl (m_fd, F_SETFL, flags) == -1)
  {
    esyslog("VNSI-Error: cxSocket::SetBlocking: fcntl(F_SETFL) failed");
    return false;
  }

  return true;
}

bool cxSocket::set_buffers(int Tx, int Rx)
{
  int max_buf = Tx;
  /*while(max_buf) {*/
    errno = 0;
    if(setsockopt(m_fd, SOL_SOCKET, SO_SNDBUF, &max_buf, sizeof(int)))
    {
      esyslog("VNSI-Error: cxSocket: setsockopt(SO_SNDBUF,%d) failed", max_buf);
      /*max_buf >>= 1;*/
    }
    /*else {*/
      int tmp = 0;
      int len = sizeof(int);
      errno = 0;
      if(getsockopt(m_fd, SOL_SOCKET, SO_SNDBUF, &tmp, (socklen_t*)&len))
      {
        esyslog("VNSI-Error: cxSocket: getsockopt(SO_SNDBUF,%d) failed", max_buf);
        /*break;*/
      }
      else if(tmp != max_buf)
      {
        dsyslog("VNSI: cxSocket: setsockopt(SO_SNDBUF): got %d bytes", tmp);
        /*max_buf >>= 1;*/
        /*continue;*/
      }
    /*}*/
  /*}*/

  max_buf = Rx;
  setsockopt(m_fd, SOL_SOCKET, SO_RCVBUF, &max_buf, sizeof(int));

  return true;
}

bool cxSocket::set_multicast(int ttl)
{
  int iReuse = 1, iLoop = 1, iTtl = ttl;

  errno = 0;

  if(setsockopt(m_fd, SOL_SOCKET, SO_REUSEADDR, &iReuse, sizeof(int)) < 0)
  {
    esyslog("VNSI-Error: cxSocket: setsockopt(SO_REUSEADDR) failed");
    return false;
  }

  if(setsockopt(m_fd, IPPROTO_IP, IP_MULTICAST_TTL, &iTtl, sizeof(int)))
  {
    esyslog("VNSI-Error: cxSocket: setsockopt(IP_MULTICAST_TTL, %d) failed", iTtl);
    return false;
  }

  if(setsockopt(m_fd, IPPROTO_IP, IP_MULTICAST_LOOP, &iLoop, sizeof(int)))
  {
    esyslog("VNSI-Error: cxSocket: setsockopt(IP_MULTICAST_LOOP) failed");
    return false;
  }

  return true;
}

/*ssize_t cxSocket::sendfile(int fd_file, off_t *offset, size_t count)
{
  int r;
#ifndef __APPLE__
  r = ::sendfile(m_fd, fd_file, offset, count);
  if(r<0 && (errno == ENOSYS || errno == EINVAL))
  {
    // fall back to read/write
    esyslog("VNSI-Error: sendfile failed - using simple read/write");
#endif
    cxPoller p(*this, true);
    char buf[0x10000];
    int todor = count, todow, done = 0;
    if(offset)
    {
      if((r=::lseek(fd_file, *offset, SEEK_SET)) < 0)
        return r;
    }
    todow = ::read(fd_file, buf, count>sizeof(buf) ? sizeof(buf) : count);
    if(todow <= 0)
      return todow;
    todor -= todow;
    while(todow > 0)
    {
      if(p.Poll(100))
      {
        r = write(buf+done, todow);
        if(r <= 0)
          return r;
        todow -= r;
        done += r;
      }
    }
    return done;
#ifndef __APPLE__
  }
  return r;
#endif
}*/

bool cxSocket::set_cork(bool state)
{
#ifdef __APPLE__
  return false;
#else
  int iCork = state ? 1 : 0;
  if(setsockopt(m_fd, IPPROTO_TCP, TCP_CORK, &iCork, sizeof(int)))
  {
    esyslog("VNSI-Error: cxSocket: setsockopt(TCP_CORK) failed");
    return false;
  }
  return true;
#endif
}

bool cxSocket::set_nodelay(bool state)
{
  int i = state ? 1 : 0;
  if(setsockopt(m_fd, IPPROTO_TCP, TCP_NODELAY, &i, sizeof(int)))
  {
    esyslog("VNSI-Error: cxSocket: setsockopt(TCP_NODELAY) failed");
    return false;
  }
  return true;
}

ssize_t cxSocket::tx_buffer_size(void)
{
  socklen_t l = sizeof(int);
  int wmem = -1;
  if(getsockopt(m_fd, SOL_SOCKET, SO_SNDBUF, &wmem, &l))
  {
    esyslog("VNSI-Error: getsockopt(SO_SNDBUF) failed");
    return (ssize_t)-1;
  }
  return (ssize_t)wmem;
}

ssize_t cxSocket::tx_buffer_free(void)
{
  int wmem = tx_buffer_size();
  int size = -1;
  if(ioctl(m_fd, TIOCOUTQ, &size))
  {
    esyslog("VNSI-Error: ioctl(TIOCOUTQ) failed");
    return (ssize_t)-1;
  }

  return (ssize_t)(wmem - size);
}

int cxSocket::getsockname(struct sockaddr *name, socklen_t *namelen)
{
  return ::getsockname(m_fd, name, namelen);
}

int cxSocket::getpeername(struct sockaddr *name, socklen_t *namelen)
{
  return ::getpeername(m_fd, name, namelen);
}

ssize_t cxSocket::send(const void *buf, size_t size, int flags, const struct sockaddr *to, socklen_t tolen)
{
  return ::sendto(m_fd, buf, size, flags, to, tolen);
}

ssize_t cxSocket::recv(void *buf, size_t size, int flags, struct sockaddr *from, socklen_t *fromlen)
{
  return ::recvfrom(m_fd, buf, size, flags, from, fromlen);
}

ssize_t cxSocket::write(const void *buffer, size_t size, int timeout_ms)
{
  cMutexLock CmdLock((cMutex*)&m_MutexWrite);

  if(m_fd == -1) return -1;

  ssize_t written = (ssize_t)size;
  const unsigned char *ptr = (const unsigned char *)buffer;

  while (size > 0)
  {
    errno = 0;
    if(!m_pollerWrite->Poll(timeout_ms))
    {
      esyslog("VNSI-Error: cxSocket::write: poll() failed");
      return written-size;
    }

    errno = 0;
    ssize_t p = ::write(m_fd, ptr, size);

    if (p <= 0)
    {
      if (errno == EINTR || errno == EAGAIN)
      {
        dsyslog("VNSI: cxSocket::write: EINTR during write(), retrying");
        continue;
      }
      esyslog("VNSI-Error: cxSocket::write: write() error");
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

  if(m_fd == -1) return -1;

  ssize_t missing = (ssize_t)size;
  unsigned char *ptr = (unsigned char *)buffer;

  while (missing > 0)
  {
    if(!m_pollerRead->Poll(timeout_ms))
    {
      esyslog("VNSI-Error: cxSocket::read: poll() failed at %d/%d", (int)(size-missing), (int)size);
      return size-missing;
    }

    errno = 0;
    ssize_t p = ::read(m_fd, ptr, missing);

    if (p <= 0)
    {
      if (errno == EINTR || errno == EAGAIN)
      {
        dsyslog("VNSI: cxSocket::read: EINTR/EAGAIN during read(), retrying");
        continue;
      }
      esyslog("VNSI-Error: cxSocket::read: read() error at %d/%d", (int)(size-missing), (int)size);
      return size-missing;
    }

    ptr  += p;
    missing -= p;
  }

  return size;
}

void cxSocket::set_handle(int h)
{
  if(h != m_fd)
  {
    close();
    m_fd = h;
    delete m_pollerRead;
    delete m_pollerWrite;
    m_pollerRead = new cPoller(m_fd);
    m_pollerWrite = new cPoller(m_fd, true);
  }
}

ssize_t cxSocket::printf(const char *fmt, ...)
{
  va_list argp;
  char buf[1024];
  int r;

  va_start(argp, fmt);
  r = vsnprintf(buf, sizeof(buf), fmt, argp);
  if (r<0)
  {
    esyslog("VNSI-Error: cxSocket::printf: vsnprintf failed");
  }
  else if(r >= (int)sizeof(buf))
  {
    dsyslog("VNSI: cxSocket::printf: vsnprintf overflow (%20s)", buf);
  }
  else
    return write(buf, r);

  return (ssize_t)-1;
}

/* readline return value:
 *   <0        : failed
 *   >=maxsize : buffer overflow
 *   >=0       : if errno = EAGAIN -> line is not complete (there was timeout)
 *               if errno = 0      -> succeed
 *               (return value 0 indicates empty line "\r\n")
 */
ssize_t cxSocket::readline(char *buf, int bufsize, int timeout, int bufpos)
{
  int n = -1, cnt = bufpos;

  do
  {
    if(timeout>0 && !m_pollerRead->Poll(timeout))
    {
      errno = EAGAIN;
      return cnt;
    }

    while((n = ::read(m_fd, buf+cnt, 1)) == 1)
    {
      buf[++cnt] = 0;

      if( cnt > 1 && buf[cnt - 2] == '\r' && buf[cnt - 1] == '\n')
      {
        cnt -= 2;
        buf[cnt] = 0;
        errno = 0;
        return cnt;
      }

      if( cnt >= bufsize)
      {
        dsyslog("VNSI: cxSocket::readline: too long control message (%d bytes): %20s", cnt, buf);
        errno = 0;
        return bufsize;
      }
    }

    /* connection closed ? */
    if (n == 0)
    {
      dsyslog("VNSI: cxSocket::readline: disconnected");
      if(errno == EAGAIN)
        errno = ENOTCONN;
      return -1;
    }

  } while (timeout>0 && n<0 && errno == EAGAIN);

  if(errno == EAGAIN)
    return cnt;

  esyslog("VNSI-Error: cxSocket::readline: read failed");
  return n;
}

#include <sys/ioctl.h>
#include <net/if.h>

uint32_t cxSocket::get_local_address(char *ip_address)
{
  uint32_t local_addr = 0;
  struct ifconf conf;
  struct ifreq buf[3];
  unsigned int n;

  struct sockaddr_in sin;
  socklen_t len = sizeof(sin);

  if(!getsockname((struct sockaddr *)&sin, &len))
  {
    local_addr = sin.sin_addr.s_addr;
  }
  else
  {
    //esyslog("VNSI-Error: getsockname failed");

    // scan network interfaces
    conf.ifc_len = sizeof(buf);
    conf.ifc_req = buf;
    memset(buf, 0, sizeof(buf));

    errno = 0;
    if(ioctl(m_fd, SIOCGIFCONF, &conf) < 0)
    {
      esyslog("VNSI-Error: cxSocket: can't obtain socket local address");
    }
    else
    {
      for(n=0; n<conf.ifc_len/sizeof(struct ifreq); n++)
      {
        struct sockaddr_in *in = (struct sockaddr_in *) &buf[n].ifr_addr;
# if 0
        uint32_t tmp = ntohl(in->sin_addr.s_addr);
        dsyslog("VNSI: Local address %6s %d.%d.%d.%d",
                conf.ifc_req[n].ifr_name,
                ((tmp>>24)&0xff), ((tmp>>16)&0xff),
                ((tmp>>8)&0xff), ((tmp)&0xff));
# endif
        if(n==0 || local_addr == htonl(INADDR_LOOPBACK))
          local_addr = in->sin_addr.s_addr;
        else
          break;
      }
    }
  }

  if(!local_addr)
    esyslog("VNSI-Error: No local address found");

  if(ip_address)
    cxSocket::ip2txt(local_addr, 0, ip_address);

  return local_addr;
}

char *cxSocket::ip2txt(uint32_t ip, unsigned int port, char *str)
{
  // inet_ntoa is not thread-safe (?)
  if(str)
  {
    unsigned int iph =(unsigned int)ntohl(ip);
    unsigned int porth =(unsigned int)ntohs(port);
    if(!porth)
      sprintf(str, "%d.%d.%d.%d", ((iph>>24)&0xff), ((iph>>16)&0xff), ((iph>>8)&0xff), ((iph)&0xff));
    else
      sprintf(str, "%u.%u.%u.%u:%u", ((iph>>24)&0xff), ((iph>>16)&0xff), ((iph>>8)&0xff), ((iph)&0xff), porth);
  }
  return str;
}
