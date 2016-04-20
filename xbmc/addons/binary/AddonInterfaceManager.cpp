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

#include "AddonInterfaceManager.h"

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

CAddonInterfaceManager::CAddonInterfaceManager()
 : m_bInitialized(false)
{
}

CAddonInterfaceManager::~CAddonInterfaceManager()
{
}

bool CAddonInterfaceManager::StartManager()
{
  m_bInitialized = true;
  return true;
}

void CAddonInterfaceManager::StopManager()
{
  m_bInitialized = false;
}

void CAddonInterfaceManager::RegisterPlayerCallBack(IPlayerCallback* pCallback)
{
  CSingleLock lock(m_vecPlayerCallbackList);
  m_vecPlayerCallbackList.push_back(pCallback);
}

void CAddonInterfaceManager::UnregisterPlayerCallBack(IPlayerCallback* pCallback)
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

void CAddonInterfaceManager::OnPlayBackStarted()
{
  LOCK_AND_COPY(std::vector<PVOID>,tmp,m_vecPlayerCallbackList);
  for (PlayerCallbackList::iterator it = tmp.begin(); (it != tmp.end()); ++it)
  {
    if (CHECK_FOR_ENTRY(m_vecPlayerCallbackList,(*it)))
      ((IPlayerCallback*)(*it))->OnPlayBackStarted();
  }
}

void CAddonInterfaceManager::OnPlayBackPaused()
{
  LOCK_AND_COPY(std::vector<PVOID>,tmp,m_vecPlayerCallbackList);
  for (PlayerCallbackList::iterator it = tmp.begin(); (it != tmp.end()); ++it)
  {
    if (CHECK_FOR_ENTRY(m_vecPlayerCallbackList,(*it)))
      ((IPlayerCallback*)(*it))->OnPlayBackPaused();
  }
}

void CAddonInterfaceManager::OnPlayBackResumed()
{
  LOCK_AND_COPY(std::vector<PVOID>,tmp,m_vecPlayerCallbackList);
  for (PlayerCallbackList::iterator it = tmp.begin(); (it != tmp.end()); ++it)
  {
    if (CHECK_FOR_ENTRY(m_vecPlayerCallbackList,(*it)))
      ((IPlayerCallback*)(*it))->OnPlayBackResumed();
  }
}

void CAddonInterfaceManager::OnPlayBackEnded()
{
  LOCK_AND_COPY(std::vector<PVOID>,tmp,m_vecPlayerCallbackList);
  for (PlayerCallbackList::iterator it = tmp.begin(); (it != tmp.end()); ++it)
  {
    if (CHECK_FOR_ENTRY(m_vecPlayerCallbackList,(*it)))
      ((IPlayerCallback*)(*it))->OnPlayBackEnded();
  }
}

void CAddonInterfaceManager::OnPlayBackStopped()
{
  LOCK_AND_COPY(std::vector<PVOID>,tmp,m_vecPlayerCallbackList);
  for (PlayerCallbackList::iterator it = tmp.begin(); (it != tmp.end()); ++it)
  {
    if (CHECK_FOR_ENTRY(m_vecPlayerCallbackList,(*it)))
      ((IPlayerCallback*)(*it))->OnPlayBackStopped();
  }
}

void CAddonInterfaceManager::OnPlayBackSpeedChanged(int iSpeed)
{
  LOCK_AND_COPY(std::vector<PVOID>,tmp,m_vecPlayerCallbackList);
  for (PlayerCallbackList::iterator it = tmp.begin(); (it != tmp.end()); ++it)
  {
    if (CHECK_FOR_ENTRY(m_vecPlayerCallbackList,(*it)))
      ((IPlayerCallback*)(*it))->OnPlayBackSpeedChanged(iSpeed);
  }
}

void CAddonInterfaceManager::OnPlayBackSeek(int iTime, int seekOffset)
{
  LOCK_AND_COPY(std::vector<PVOID>,tmp,m_vecPlayerCallbackList);
  for (PlayerCallbackList::iterator it = tmp.begin(); (it != tmp.end()); ++it)
  {
    if (CHECK_FOR_ENTRY(m_vecPlayerCallbackList,(*it)))
      ((IPlayerCallback*)(*it))->OnPlayBackSeek(iTime, seekOffset);
  }
}

void CAddonInterfaceManager::OnPlayBackSeekChapter(int iChapter)
{
  LOCK_AND_COPY(std::vector<PVOID>,tmp,m_vecPlayerCallbackList);
  for (PlayerCallbackList::iterator it = tmp.begin(); (it != tmp.end()); ++it)
  {
    if (CHECK_FOR_ENTRY(m_vecPlayerCallbackList,(*it)))
      ((IPlayerCallback*)(*it))->OnPlayBackSeekChapter(iChapter);
  }
}

void CAddonInterfaceManager::OnQueueNextItem()
{
  LOCK_AND_COPY(std::vector<PVOID>,tmp,m_vecPlayerCallbackList);
  for (PlayerCallbackList::iterator it = tmp.begin(); (it != tmp.end()); ++it)
  {
    if (CHECK_FOR_ENTRY(m_vecPlayerCallbackList,(*it)))
      ((IPlayerCallback*)(*it))->OnQueueNextItem();
  }
}

} /* namespace ADDON */
