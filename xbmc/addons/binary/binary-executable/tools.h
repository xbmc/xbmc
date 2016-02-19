#pragma once
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

#include "addons/IAddon.h"
#include "threads/CriticalSection.h"

#include <string>
#include <inttypes.h>
#include <stdlib.h>
#include <poll.h>

#ifndef TARGET_WINDOWS
#  ifndef __FreeBSD__
#    include <asm/byteorder.h>
#  else
#    include <sys/endian.h>
#  endif
#  if !defined(ntohll)
#    if defined(be64toh)
#      define ntohll be64toh
#    else
uint64_t ntohll(uint64_t a);
#    endif
#  endif
#  if !defined(htonll)
#    if defined(htobe64)
#      define htonll htobe64
#    else
uint64_t htonll(uint64_t a);
#    endif
#  endif
#endif

namespace ADDON
{

std::string SystemErrorString(int errnum);

class CAddonAPI_Poller
{
public:
  CAddonAPI_Poller(int fileHandle = -1, bool out = false);

  bool Add(int fileHandle, bool out);
  bool Poll(int TimeoutMs = 0);

private:
  enum { MaxPollFiles = 16 };

  pollfd        m_pfd[MaxPollFiles];
  unsigned int  m_numFileHandles;
};

class CAddonAPI_Socket
{
private:
  int m_fd;
  CCriticalSection m_critSection;
  CAddonAPI_Poller *m_pollerRead;
  CAddonAPI_Poller *m_pollerWrite;

public:
  CAddonAPI_Socket();
  ~CAddonAPI_Socket();

  void SetHandle(int h);
  void close(void);
  void Shutdown(void);
  void LockWrite();
  void UnlockWrite();
  ssize_t read(void *buffer, size_t size, int timeout_ms = -1);
  ssize_t write(const void *buffer, size_t size, int timeout_ms = -1, bool more_data = false);

  static std::string ip2txt(uint32_t ip, unsigned int port);
};

}; /* namespace ADDON */
