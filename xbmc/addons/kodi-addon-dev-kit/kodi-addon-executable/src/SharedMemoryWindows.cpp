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

#if (defined TARGET_WINDOWS)

#include "SharedMemoryWindows.h"
#include "tools.h"
#include "kodi/api2/AddonLib.h"

#include <p8-platform/threads/threads.h>
#include <windows.h>
#include <conio.h>

CBinaryAddonSharedMemory::CBinaryAddonSharedMemory(size_t size /*= DEFAULT_SHARED_MEM_SIZE*/)
 : CBinaryAddonSharedMemory(size),
   m_handle_AddonToKodi(INVALID_HANDLE_VALUE),
   m_handle_KodiToAddon(INVALID_HANDLE_VALUE)
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
  m_handle_AddonToKodi = CreateFileMapping(FILE_MAP_ALL_ACCESS, FALSE,
                               StringUtils::Format("Global\\Kodi_Binary-%X", m_randomConnectionNumber).c_str());
  if (m_handle_AddonToKodi == NULL)
  {
    KodiAPI_Log(LOGERROR, "Binary AddOn: Request of Add-on to Kodi shared memory failed with error '%i': %s",
                  GetLastError(), ErrorString(GetLastError()).c_str());
    return false;
  }

  m_sharedMem_AddonToKodi = (KodiAPI_ShareData*)MapViewOfFile(m_handle_AddonToKodi, FILE_MAP_ALL_ACCESS, 0, 0, 0);
  if (m_sharedMem_AddonToKodi == NULL)
  {
    KodiAPI_Log(LOGERROR, "Binary AddOn: Connect of Add-on to Kodi shared memory failed with error '%i': %s",
                  GetLastError(), ErrorString(GetLastError()).c_str());
    return false;
  }

  m_semaphore_AddonToKodi_Kodi = OpenSemaphore(SEMAPHORE_ALL_ACCESS, FALSE,
                                StringUtils::Format("Global\\Kodi_Binary-AddonToKodi-ToKodi-%X", m_randomConnectionNumber).c_str());
  if (m_semaphore_AddonToKodi_Kodi == NULL)
  {
    KodiAPI_Log(LOGERROR, "Binary AddOn: Creation of Add-on to Kodi lock for Add-on failed with error '%i': %s",
                  GetLastError(), ErrorString(GetLastError()).c_str());
    return false;
  }

  m_semaphore_AddonToKodi_Addon = OpenSemaphore(SEMAPHORE_ALL_ACCESS, FALSE,
                                StringUtils::Format("Global\\Kodi_Binary-AddonToKodi-ToAddon-%X", m_randomConnectionNumber).c_str());
  if (m_semaphore_AddonToKodi_Addon == NULL)
  {
    KodiAPI_Log(LOGERROR, "Binary AddOn: Creation of Add-on to Kodi lock for Kodi failed with error '%i': %s",
                  GetLastError(), ErrorString(GetLastError()).c_str());
    return false;
  }

  /*
   * Interface from Kodi to Addon
   */
  m_handle_KodiToAddon = CreateFileMapping(FILE_MAP_ALL_ACCESS, FALSE,
                               StringUtils::Format("Global\\Kodi_Binary-%X", m_randomConnectionNumber+1).c_str());
  if (m_handle_KodiToAddon == NULL)
  {
    KodiAPI_Log(LOGERROR, "Binary AddOn: Request of Add-on to Kodi shared memory failed with error '%i': %s",
                  GetLastError(), ErrorString(GetLastError()).c_str());
    return false;
  }

  m_sharedMem_KodiToAddon = (KodiAPI_ShareData*)MapViewOfFile(m_handle_KodiToAddon, FILE_MAP_ALL_ACCESS, 0, 0, 0);
  if (m_sharedMem_KodiToAddon == NULL)
  {
    KodiAPI_Log(LOGERROR, "Binary AddOn: Connect of Add-on to Kodi shared memory failed with error '%i': %s",
                  GetLastError(), ErrorString(GetLastError()).c_str());
    return false;
  }

  m_semaphore_KodiToAddon_Kodi = OpenSemaphore(SEMAPHORE_ALL_ACCESS, FALSE,
                                StringUtils::Format("Global\\Kodi_Binary-KodiToAddon-ToKodi-%X", m_randomConnectionNumber+1).c_str());
  if (m_semaphore_KodiToAddon_Kodi == NULL)
  {
    KodiAPI_Log(LOGERROR, "Binary AddOn: Creation of Add-on to Kodi lock for Add-on failed with error '%i': %s",
                  GetLastError(), ErrorString(GetLastError()).c_str());
    return false;
  }

  m_semaphore_KodiToAddon_Addon = OpenSemaphore(SEMAPHORE_ALL_ACCESS, FALSE,
                                StringUtils::Format("Global\\Kodi_Binary-KodiToAddon-ToAddon-%X", m_randomConnectionNumber+1).c_str());
  if (m_semaphore_KodiToAddon_Addon == NULL)
  {
    KodiAPI_Log(LOGERROR, "Binary AddOn: Creation of Add-on to Kodi lock for Kodi failed with error '%i': %s",
                  GetLastError(), ErrorString(GetLastError()).c_str());
    return false;
  }

  return true;
}

void CBinaryAddonSharedMemory::DestroySharedMemory()
{
  CloseHandle(m_semaphore_AddonToKodi_Kodi);
  CloseHandle(m_semaphore_AddonToKodi_Addon);
  UnmapViewOfFile(m_sharedMem_AddonToKodi);
  CloseHandle(m_handle_AddonToKodi);
  m_handle_AddonToKodi = INVALID_HANDLE_VALUE;

  CloseHandle(m_semaphore_KodiToAddon_Kodi);
  CloseHandle(m_semaphore_KodiToAddon_Addon);
  UnmapViewOfFile(m_sharedMem_KodiToAddon);
  CloseHandle(m_handle_KodiToAddon);
  m_handle_KodiToAddon = INVALID_HANDLE_VALUE;
}

bool CBinaryAddonSharedMemory::Lock_AddonToKodi_Addon()
{
  WaitForSingleObject(m_semaphore_AddonToKodi_Addon, INFINITE);
  return true;
}

void CBinaryAddonSharedMemory::Unlock_AddonToKodi_Kodi()
{
  if (!ReleaseSemaphore(m_semaphore_AddonToKodi_Kodi, 1, NULL))
    KodiAPI_Log(LOGERROR, "Binary AddOn: Unlock of Kodi on Addon to Kodi failed with error '%i': %s",
                  GetLastError(), ErrorString(GetLastError()).c_str());
}

bool CBinaryAddonSharedMemory::Lock_KodiToAddon_Addon()
{
  WaitForSingleObject(m_semaphore_KodiToAddon_Addon, INFINITE);
  return true;
}

void CBinaryAddonSharedMemoryPosix::Unlock_KodiToAddon_Addon()
{
  if (!ReleaseSemaphore(m_semaphore_KodiToAddon_Addon, 1, NULL))
    KodiAPI_Log(LOGERROR, "Binary AddOn: Unlock of Addon on Kodi to Addon failed with error '%i': %s",
                  GetLastError(), ErrorString(GetLastError()).c_str());
}

void CBinaryAddonSharedMemory::Unlock_KodiToAddon_Kodi()
{
  if (!ReleaseSemaphore(m_semaphore_KodiToAddon_Kodi, 1, NULL))
    KodiAPI_Log(LOGERROR, "Binary AddOn: Unlock of Kodi on Kodi to Addon failed with error '%i': %s",
                  GetLastError(), ErrorString(GetLastError()).c_str());
}

#endif /* (defined TARGET_WINDOWS) */
