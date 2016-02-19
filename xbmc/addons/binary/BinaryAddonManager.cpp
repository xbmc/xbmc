/*
 *      Copyright (C) 2015-2016 Team KODI
 *      http://kodi.tv
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
 *  along with KODI; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "BinaryAddonManager.h"
#include "binary-executable/BinaryAddon.h"
#include "binary-executable/tools.h"

#include "Application.h"
#include "utils/log.h"

#include <netinet/in.h>

#define LOCK_AND_COPY(type, dest, src) \
  if (!m_bInitialized) return; \
  CSingleLock lock(src); \
  src.hadSomethingRemoved = false; \
  type dest; \
  dest = src

#define CHECK_FOR_ENTRY(l,v) \
  (l.hadSomethingRemoved ? (std::find(l.begin(),l.end(),v) != l.end()) : true)

namespace ADDON
{

unsigned int CBinaryAddonManager::m_IdCnt = 0;

CBinaryAddonManager::CBinaryAddonManager()
 : CThread("Binary add-on manager"),
   m_bInitialized(false)
{
}

CBinaryAddonManager::~CBinaryAddonManager()
{
  m_Status.Shutdown();
}

CBinaryAddonManager &CBinaryAddonManager::GetInstance()
{
  static CBinaryAddonManager managerInstance;
  return managerInstance;
}

bool CBinaryAddonManager::StartManager()
{
  Create();
  m_bInitialized = true;
  return true;
}

void CBinaryAddonManager::StopManager()
{
  m_bInitialized = false;
  m_Status.Shutdown();
  StopThread();
}

void CBinaryAddonManager::NewAddonConnected(int fd)
{
  struct sockaddr_in sin;
  socklen_t len = sizeof(sin);

  if (getpeername(fd, (struct sockaddr *)&sin, &len))
  {
    CLog::Log(LOGERROR, "getpeername() failed, dropping new incoming connection %d", m_IdCnt);
    close(fd);
    return;
  }

  if (sin.sin_addr.s_addr == INADDR_LOOPBACK)
  {
    CLog::Log(LOGERROR, "Address not allowed to connect (%s) %i", CAddonAPI_Socket::ip2txt(sin.sin_addr.s_addr, sin.sin_port).c_str(), sin.sin_port);
    close(fd);
    return;
  }

  if (fcntl(fd, F_SETFL, fcntl (fd, F_GETFL) | O_NONBLOCK) == -1)
  {
    CLog::Log(LOGERROR, "Error setting control socket to nonblocking mode");
    close(fd);
    return;
  }

  int val = 1;
  setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &val, sizeof(val));

#ifdef SOL_TCP
  val = 30;
  setsockopt(fd, SOL_TCP, TCP_KEEPIDLE, &val, sizeof(val));

  val = 15;
  setsockopt(fd, SOL_TCP, TCP_KEEPINTVL, &val, sizeof(val));

  val = 5;
  setsockopt(fd, SOL_TCP, TCP_KEEPCNT, &val, sizeof(val));

  val = 1;
  setsockopt(fd, SOL_TCP, TCP_NODELAY, &val, sizeof(val));
#endif

  CLog::Log(LOGINFO, "Addon with ID %d connected: %s",
                m_IdCnt, CAddonAPI_Socket::ip2txt(sin.sin_addr.s_addr, sin.sin_port).c_str());

  bool isLocal = (sin.sin_addr.s_addr == 0x100007F) ? true : false;
  CBinaryAddon *connection = new CBinaryAddon(isLocal, fd, m_IdCnt, true);
  m_Status.AddAddon(connection);
  m_IdCnt++;
}

void CBinaryAddonManager::Process(void)
{
  fd_set fds;
  struct timeval tv;

  m_Status.Create();

  m_ServerFD = socket(AF_INET, SOCK_STREAM, 0);
  if(m_ServerFD == -1)
    return;

  fcntl(m_ServerFD, F_SETFD, fcntl(m_ServerFD, F_GETFD) | FD_CLOEXEC);

  int one = 1;
  setsockopt(m_ServerFD, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(int));

  struct sockaddr_in s;
  memset(&s, 0, sizeof(s));
  s.sin_family = AF_INET;
  s.sin_port = htons(KODI_API_ConnectionPort);

  int x = bind(m_ServerFD, (struct sockaddr *)&s, sizeof(s));
  if (x < 0)
  {
    close(m_ServerFD);
    CLog::Log(LOGERROR, "Unable to start Kodi's add-on Server, port already in use ?");
    m_ServerFD = -1;
    return;
  }

  listen(m_ServerFD, 10);

  while (!m_bStop && !g_application.m_bStop)
  {
    FD_ZERO(&fds);
    FD_SET(m_ServerFD, &fds);

    tv.tv_sec = 0;
    tv.tv_usec = 250*1000;

    int r = select(m_ServerFD + 1, &fds, NULL, NULL, &tv);
    if (r == -1)
    {
      CLog::Log(LOGERROR, "failed during select");
      continue;
    }
    if (r == 0)
    {
      continue;
    }

    int fd = accept(m_ServerFD, 0, 0);
    if (fd >= 0)
    {
      NewAddonConnected(fd);
    }
    else
    {
      CLog::Log(LOGERROR, "accept failed");
    }
  }
}
  
void CBinaryAddonManager::RegisterPlayerCallBack(IPlayerCallback* pCallback)
{
  CSingleLock lock(m_vecPlayerCallbackList);
  m_vecPlayerCallbackList.push_back(pCallback);
}

void CBinaryAddonManager::UnregisterPlayerCallBack(IPlayerCallback* pCallback)
{
  CSingleLock lock(m_vecPlayerCallbackList);
  PlayerCallbackList::iterator it = m_vecPlayerCallbackList.begin();
  while (it != m_vecPlayerCallbackList.end())
  {
    if (*it == pCallback)
    {
      it = m_vecPlayerCallbackList.erase(it);
      m_vecPlayerCallbackList.hadSomethingRemoved = true;
    }
    else
      ++it;
  }
}

void CBinaryAddonManager::OnPlayBackStarted()
{
  LOCK_AND_COPY(std::vector<PVOID>,tmp,m_vecPlayerCallbackList);
  for (PlayerCallbackList::iterator it = tmp.begin(); (it != tmp.end()); ++it)
  {
    if (CHECK_FOR_ENTRY(m_vecPlayerCallbackList,(*it)))
      ((IPlayerCallback*)(*it))->OnPlayBackStarted();
  }
}

void CBinaryAddonManager::OnPlayBackPaused()
{
  LOCK_AND_COPY(std::vector<PVOID>,tmp,m_vecPlayerCallbackList);
  for (PlayerCallbackList::iterator it = tmp.begin(); (it != tmp.end()); ++it)
  {
    if (CHECK_FOR_ENTRY(m_vecPlayerCallbackList,(*it)))
      ((IPlayerCallback*)(*it))->OnPlayBackPaused();
  }
}

void CBinaryAddonManager::OnPlayBackResumed()
{
  LOCK_AND_COPY(std::vector<PVOID>,tmp,m_vecPlayerCallbackList);
  for (PlayerCallbackList::iterator it = tmp.begin(); (it != tmp.end()); ++it)
  {
    if (CHECK_FOR_ENTRY(m_vecPlayerCallbackList,(*it)))
      ((IPlayerCallback*)(*it))->OnPlayBackResumed();
  }
}

void CBinaryAddonManager::OnPlayBackEnded()
{
  LOCK_AND_COPY(std::vector<PVOID>,tmp,m_vecPlayerCallbackList);
  for (PlayerCallbackList::iterator it = tmp.begin(); (it != tmp.end()); ++it)
  {
    if (CHECK_FOR_ENTRY(m_vecPlayerCallbackList,(*it)))
      ((IPlayerCallback*)(*it))->OnPlayBackEnded();
  }
}

void CBinaryAddonManager::OnPlayBackStopped()
{
  LOCK_AND_COPY(std::vector<PVOID>,tmp,m_vecPlayerCallbackList);
  for (PlayerCallbackList::iterator it = tmp.begin(); (it != tmp.end()); ++it)
  {
    if (CHECK_FOR_ENTRY(m_vecPlayerCallbackList,(*it)))
      ((IPlayerCallback*)(*it))->OnPlayBackStopped();
  }
}

void CBinaryAddonManager::OnPlayBackSpeedChanged(int iSpeed)
{
  LOCK_AND_COPY(std::vector<PVOID>,tmp,m_vecPlayerCallbackList);
  for (PlayerCallbackList::iterator it = tmp.begin(); (it != tmp.end()); ++it)
  {
    if (CHECK_FOR_ENTRY(m_vecPlayerCallbackList,(*it)))
      ((IPlayerCallback*)(*it))->OnPlayBackSpeedChanged(iSpeed);
  }
}

void CBinaryAddonManager::OnPlayBackSeek(int iTime, int seekOffset)
{
  LOCK_AND_COPY(std::vector<PVOID>,tmp,m_vecPlayerCallbackList);
  for (PlayerCallbackList::iterator it = tmp.begin(); (it != tmp.end()); ++it)
  {
    if (CHECK_FOR_ENTRY(m_vecPlayerCallbackList,(*it)))
      ((IPlayerCallback*)(*it))->OnPlayBackSeek(iTime, seekOffset);
  }
}

void CBinaryAddonManager::OnPlayBackSeekChapter(int iChapter)
{
  LOCK_AND_COPY(std::vector<PVOID>,tmp,m_vecPlayerCallbackList);
  for (PlayerCallbackList::iterator it = tmp.begin(); (it != tmp.end()); ++it)
  {
    if (CHECK_FOR_ENTRY(m_vecPlayerCallbackList,(*it)))
      ((IPlayerCallback*)(*it))->OnPlayBackSeekChapter(iChapter);
  }
}

void CBinaryAddonManager::OnQueueNextItem()
{
  LOCK_AND_COPY(std::vector<PVOID>,tmp,m_vecPlayerCallbackList);
  for (PlayerCallbackList::iterator it = tmp.begin(); (it != tmp.end()); ++it)
  {
    if (CHECK_FOR_ENTRY(m_vecPlayerCallbackList,(*it)))
      ((IPlayerCallback*)(*it))->OnQueueNextItem();
  }
}

}; /* namespace ADDON */
