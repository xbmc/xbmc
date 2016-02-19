/*
 *      Copyright (C) 2015 Team KODI
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

/*
 * Documents:
 * https://msdn.microsoft.com/en-us/library/ms685129(VS.85).aspx
 * https://msdn.microsoft.com/de-de/library/windows/desktop/aa366551(v=vs.85).aspx
 */

#if (defined TARGET_WINDOWS)

#include "SharedMemoryWindows.h"
#include "tools.h"

#include <p8-platform/threads/threads.h>
#include <windows.h>
#include <conio.h>

CBinaryAddonSharedMemory::CBinaryAddonSharedMemory(int randNumber, CBinaryAddon* addon, size_t size /*= DEFAULT_SHARED_MEM_SIZE*/)
 : CBinaryAddonSharedMemory(randNumber, addon, size),
   m_handle(INVALID_HANDLE_VALUE)
{

}

CBinaryAddonSharedMemory::~CBinaryAddonSharedMemory()
{

}

bool CBinaryAddonSharedMemory::CreateSharedMemory()
{
  /*
   * Interface from Addon to Kodi
   */
  m_handle_AddonToKodi = CreateFileMapping(INVALID_HANDLE_VALUE, nullptr,
                               PAGE_READWRITE, 0, m_sharedMemSize,
                               StringUtils::Format("Global\\Kodi_Binary-AddonToKodi-%X", m_randomConnectionNumber).c_str());
  if (m_handle_AddonToKodi == nullptr)
  {
    CLog::Log(LOGERROR, "Binary AddOn: Request of shared memory failed with error '%i': %s",
                GetLastError(), SystemErrorString(GetLastError()).c_str());
    return false;
  }

  m_sharedMem_AddonToKodi = (KodiAPI_ShareData*)MapViewOfFile(m_handle_AddonToKodi, FILE_MAP_ALL_ACCESS, 0, 0, 0);
  if (m_sharedMem_AddonToKodi == nullptr)
  {
    CLog::Log(LOGERROR, "Binary AddOn: Connect of shared memory failed with error '%i': %s",
                GetLastError(), SystemErrorString(GetLastError()).c_str());
    return false;
  }

  m_semaphore_AddonToKodi_Kodi = CreateSemaphore(nullptr, MAX_SEM_COUNT, MAX_SEM_COUNT,
                                StringUtils::Format("Global\\Kodi_Binary-AddonToKodi-ToKodi-%X", m_randomConnectionNumber).c_str());
  if (m_semaphore_AddonToKodi_Kodi == nullptr)
  {
    CLog::Log(LOGERROR, "Binary AddOn: Creation of Lock from Add-on to Kodi failed with error '%i': %s",
                GetLastError(), SystemErrorString(GetLastError()).c_str());
    return false;
  }

  m_semaphore_AddonToKodi_Addon = CreateSemaphore(nullptr, MAX_SEM_COUNT, MAX_SEM_COUNT,
                                StringUtils::Format("Global\\Kodi_Binary-AddonToKodi-ToAddon-%X", m_randomConnectionNumber).c_str());
  if (m_semaphore_AddonToKodi_Addon == nullptr)
  {
    CLog::Log(LOGERROR, "Binary AddOn: Creation of Lock from Add-on to Addon failed with error '%i': %s",
                GetLastError(), SystemErrorString(GetLastError()).c_str());
    return false;
  }

  /*
   * Interface from Kodi to Addon
   */
  m_handle_KodiToAddon = CreateFileMapping(INVALID_HANDLE_VALUE, nullptr,
                               PAGE_READWRITE, 0, m_sharedMemSize,
                               StringUtils::Format("Global\\Kodi_Binary-KodiToAddon-%X", m_randomConnectionNumber+1).c_str());
  if (m_handle_KodiToAddon == nullptr)
  {
    CLog::Log(LOGERROR, "Binary AddOn: Request of shared memory failed with error '%i': %s",
                GetLastError(), SystemErrorString(GetLastError()).c_str());
    return false;
  }

  m_sharedMem_KodiToAddon = (KodiAPI_ShareData*)MapViewOfFile(m_handle_KodiToAddon, FILE_MAP_ALL_ACCESS, 0, 0, 0);
  if (m_sharedMem_KodiToAddon == nullptr)
  {
    CLog::Log(LOGERROR, "Binary AddOn: Connect of shared memory failed with error '%i': %s",
                GetLastError(), SystemErrorString(GetLastError()).c_str());
    return false;
  }

  m_semaphore_KodiToAddon_Kodi = CreateSemaphore(nullptr, MAX_SEM_COUNT, MAX_SEM_COUNT,
                                StringUtils::Format("Global\\Kodi_Binary-KodiToAddon-%X", m_randomConnectionNumber+1).c_str());
  if (m_semaphore_KodiToAddon_Kodi == nullptr)
  {
    CLog::Log(LOGERROR, "Binary AddOn: Creation of Lock from Add-on to Kodi failed with error '%i': %s",
                GetLastError(), SystemErrorString(GetLastError()).c_str());
    return false;
  }

  m_semaphore_KodiToAddon_Addon = CreateSemaphore(nullptr, MAX_SEM_COUNT, MAX_SEM_COUNT,
                                StringUtils::Format("Global\\Kodi_Binary-KodiToAddon-ToAddon-%X", m_randomConnectionNumber+1).c_str());
  if (m_semaphore_KodiToAddon_Addon == nullptr)
  {
    CLog::Log(LOGERROR, "Binary AddOn: Creation of Lock from Add-on to Addon failed with error '%i': %s",
                GetLastError(), SystemErrorString(GetLastError()).c_str());
    return false;
  }

  return true;
}

void CBinaryAddonSharedMemory::DestroySharedMemory()
{
  m_LoggedIn = false;
  // Do a bit paranoia to make sure nobody is locked
  Unlock_AddonToKodi_Addon();
  Unlock_AddonToKodi_Kodi();
  Unlock_KodiToAddon_Addon();
  Unlock_KodiToAddon_Kodi();

  CloseHandle(m_semaphore_AddonToKodi_Addon);
  CloseHandle(m_semaphore_AddonToKodi_Kodi);
  UnmapViewOfFile(m_sharedMem_AddonToKodi);
  CloseHandle(m_handle_AddonToKodi);
  m_handle_AddonToKodi = INVALID_HANDLE_VALUE;

  CloseHandle(m_semaphore_KodiToAddon_Addon);
  CloseHandle(m_semaphore_KodiToAddon_Kodi);
  UnmapViewOfFile(m_sharedMem_KodiToAddon);
  CloseHandle(m_handle_KodiToAddon);
  m_handle_KodiToAddon = INVALID_HANDLE_VALUE;
}

bool CBinaryAddonSharedMemory::Lock_AddonToKodi_Kodi()
{
  WaitForSingleObject(m_semaphore_AddonToKodi_Kodi, INFINITE);
  return true;
}

void CBinaryAddonSharedMemory::Unlock_AddonToKodi_Addon()
{
  if (!ReleaseSemaphore(m_semaphore_AddonToKodi_Addon, 1, nullptr))
    CLog::Log(LOGERROR, "Binary AddOn: Unlock of Addon to Kodi for Addon failed with error '%i': %s",
                GetLastError(), SystemErrorString(GetLastError()).c_str());
}

void CBinaryAddonSharedMemory::Unlock_AddonToKodi_Kodi()
{
  if (!ReleaseSemaphore(m_semaphore_AddonToKodi_Kodi, 1, nullptr))
    CLog::Log(LOGERROR, "Binary AddOn: Unlock of Addon to Kodi for Kodi failed with error '%i': %s",
                GetLastError(), SystemErrorString(GetLastError()).c_str());
}

bool CBinaryAddonSharedMemory::Lock_KodiToAddon_Addon()
{
  WaitForSingleObject(m_semaphore_KodiToAddon_Addon, INFINITE);
  return true;
}

void CBinaryAddonSharedMemory::Unlock_KodiToAddon_Kodi()
{
  if (!ReleaseSemaphore(m_semaphore_KodiToAddon_Kodi, 1, nullptr))
    KodiAPI_Log(LOGERROR, "Binary AddOn: Unlock of Kodi to Addon for Kodi failed with error '%i': %s",
                  GetLastError(), SystemErrorString(GetLastError()).c_str());
}

void CBinaryAddonSharedMemory::Unlock_KodiToAddon_Addon()
{
  if (!ReleaseSemaphore(m_semaphore_KodiToAddon_Addon, 1, nullptr))
    CLog::Log(LOGERROR, "Binary AddOn: Unlock of Kodi to Addon for Addon failed with error '%i': %s",
                GetLastError(), SystemErrorString(GetLastError()).c_str());
}

#endif /* (defined TARGET_WINDOWS) */
