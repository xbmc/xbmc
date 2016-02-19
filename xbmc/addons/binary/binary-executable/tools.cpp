/*
 *      Copyright (C) 2010 Team XBMC
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

#include "tools.h"

#include "addons/kodi-addon-dev-kit/include/kodi/api2/definitions.hpp"
#include "threads/Thread.h"
#include "utils/log.h"
#include "utils/StringUtils.h"

#include <netinet/in.h>

#ifndef TARGET_WINDOWS
#define TYP_INIT 0
#define TYP_SMLE 1
#define TYP_BIGE 2

// need to check for ntohll definition
// as it was added in iOS SDKs since 8.0
#if !defined(ntohll)
uint64_t ntohll(uint64_t a)
{
  return htonll(a);
}
#endif

#if !defined(htonll)
uint64_t htonll(uint64_t a) {
  static int typ = TYP_INIT;
  unsigned char c;
  union {
    uint64_t ull;
    unsigned char c[8];
  } x;
  if (typ == TYP_INIT) {
    x.ull = 0x01;
    typ = (x.c[7] == 0x01ULL) ? TYP_BIGE : TYP_SMLE;
  }
  if (typ == TYP_BIGE)
    return a;
  x.ull = a;
  c = x.c[0]; x.c[0] = x.c[7]; x.c[7] = c;
  c = x.c[1]; x.c[1] = x.c[6]; x.c[6] = c;
  c = x.c[2]; x.c[2] = x.c[5]; x.c[5] = c;
  c = x.c[3]; x.c[3] = x.c[4]; x.c[4] = c;
  return x.ull;
}
#endif
#endif

namespace ADDON
{

CAddonAPI_Poller::CAddonAPI_Poller(int fileHandle, bool out)
 : m_numFileHandles(0)
{
  Add(fileHandle, out);
}

bool CAddonAPI_Poller::Add(int fileHandle, bool out)
{
  if (fileHandle >= 0)
  {
    for (unsigned int i = 0; i < m_numFileHandles; ++i)
    {
      if (m_pfd[i].fd == fileHandle && m_pfd[i].events == (out ? POLLOUT : POLLIN))
        return true;
    }
    if (m_numFileHandles < MaxPollFiles)
    {
      m_pfd[m_numFileHandles].fd = fileHandle;
      m_pfd[m_numFileHandles].events = out ? POLLOUT : POLLIN;
      m_pfd[m_numFileHandles].revents = 0;
      ++m_numFileHandles;
      return true;
    }
    CLog::Log(LOGERROR, "CAddonAPI_Poller::Add: To many file handles in CAddonAPI_Poller");
  }
  return false;
}

bool CAddonAPI_Poller::Poll(int TimeoutMs)
{
  if (m_numFileHandles)
  {
    if (poll(m_pfd, m_numFileHandles, TimeoutMs) != 0)
      return true; // returns true even in case of an error, to let the caller
                   // access the file and thus see the error code
  }
  return false;
}


CAddonAPI_Socket::CAddonAPI_Socket()
 : m_fd(-1),
   m_pollerRead(NULL),
   m_pollerWrite(NULL)
{
}

CAddonAPI_Socket::~CAddonAPI_Socket()
{
  close();
  delete m_pollerRead;
  delete m_pollerWrite;
}

void CAddonAPI_Socket::close()
{
  if(m_fd >= 0)
  {
    ::close(m_fd);
    m_fd=-1;
  }
}

void CAddonAPI_Socket::Shutdown()
{
  if(m_fd >= 0)
  {
    ::shutdown(m_fd, SHUT_RD);
  }
}

void CAddonAPI_Socket::LockWrite()
{
  m_critSection.lock();
}

void CAddonAPI_Socket::UnlockWrite()
{
  m_critSection.unlock();
}

ssize_t CAddonAPI_Socket::write(const void *buffer, size_t size, int timeout_ms, bool more_data)
{
  CSingleLock lock(m_critSection);

  if(m_fd == -1)
    return -1;

  ssize_t written = (ssize_t)size;
  const unsigned char *ptr = (const unsigned char *)buffer;

  while (size > 0)
  {
    if(!m_pollerWrite->Poll(timeout_ms))
    {
      CLog::Log(LOGERROR, "CAddonAPI_Socket::write: poll() failed");
      return written-size;
    }

    ssize_t p = ::send(m_fd, ptr, size, (more_data ? MSG_MORE : 0));

    if (p <= 0)
    {
      if (errno == EINTR || errno == EAGAIN)
      {
        CLog::Log(LOGDEBUG, "CAddonAPI_Socket::write: EINTR during write(), retrying");
        continue;
      }
      else if (errno != EPIPE)
      {
        CLog::Log(LOGERROR, "CAddonAPI_Socket::write: write() error");
      }
      return p;
    }

    ptr  += p;
    size -= p;
  }

  return written;
}

ssize_t CAddonAPI_Socket::read(void *buffer, size_t size, int timeout_ms)
{
  int retryCounter = 0;

  if(m_fd == -1)
    return -1;

  ssize_t missing = (ssize_t)size;
  unsigned char *ptr = (unsigned char *)buffer;

  while (missing > 0)
  {
    if(!m_pollerRead->Poll(timeout_ms))
    {
      CLog::Log(LOGERROR, "CAddonAPI_Socket::read: poll() failed at %d/%d", (int)(size-missing), (int)size);
      return size-missing;
    }

    ssize_t p = ::read(m_fd, ptr, missing);

    if (p < 0)
    {
      if (retryCounter < 10 && (errno == EINTR || errno == EAGAIN))
      {
        CLog::Log(LOGDEBUG, "CAddonAPI_Socket::read: EINTR/EAGAIN during read(), retrying");
        retryCounter++;
        continue;
      }
      CLog::Log(LOGERROR, "CAddonAPI_Socket::read: read() error at %d/%d", (int)(size-missing), (int)size);
      return 0;
    }
    else if (p == 0)
    {
      //CLog::Log(LOGINFO, "CAddonAPI_Socket::read: eof, connection closed");
      return 0;
    }

    retryCounter = 0;
    ptr  += p;
    missing -= p;
  }

  return size;
}

void CAddonAPI_Socket::SetHandle(int h)
{
  if (h != m_fd)
  {
    close();
    m_fd = h;
    delete m_pollerRead;
    delete m_pollerWrite;
    m_pollerRead = new CAddonAPI_Poller(m_fd);
    m_pollerWrite = new CAddonAPI_Poller(m_fd, true);
  }
}

std::string CAddonAPI_Socket::ip2txt(uint32_t ip, unsigned int port)
{
  std::string str;
  unsigned int iph =(unsigned int)ntohl(ip);
  unsigned int porth =(unsigned int)ntohs(port);
  if(!porth)
    str = StringUtils::Format("%d.%d.%d.%d", ((iph>>24)&0xff), ((iph>>16)&0xff), ((iph>>8)&0xff), ((iph)&0xff));
  else
    str = StringUtils::Format("%u.%u.%u.%u:%u", ((iph>>24)&0xff), ((iph>>16)&0xff), ((iph>>8)&0xff), ((iph)&0xff), porth);
  return str;
}

std::string SystemErrorString(int errnum)
{
  const int MaxErrStrLen = 2000;
  char buffer[MaxErrStrLen];

  buffer[0] = '\0';
  char* str = buffer;

#ifdef HAVE_STRERROR_R
  // strerror_r is thread-safe.
  if (errnum)
# if defined(__GLIBC__) && defined(_GNU_SOURCE)
    // glibc defines its own incompatible version of strerror_r
    // which may not use the buffer supplied.
    str = strerror_r(errnum, buffer, MaxErrStrLen-1);
# else
    strerror_r(errnum, buffer, MaxErrStrLen-1);
# endif
#elif defined(HAVE_STRERROR_S)  // Windows.
  if (errnum)
    strerror_s(buffer, errnum);
#elif defined(HAVE_STRERROR)
  // Copy the thread un-safe result of strerror into
  // the buffer as fast as possible to minimize impact
  // of collision of strerror in multiple threads.
  if (errnum)
    strncpy(buffer, strerror(errnum), MaxErrStrLen-1);
  buffer[MaxErrStrLen-1] = '\0';
#else
  // Strange that this system doesn't even have strerror
  // but, oh well, just use a generic message
  sprintf(buffer, "Error #%d", errnum);
#endif
  return str;
}

} /* namespace ADDON */
