#pragma once
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

#include "SharedMemory.h"

class CBinaryAddonSharedMemoryWindows
  : public CBinaryAddonSharedMemory
{
public:
  CBinaryAddonSharedMemoryWindows(size_t size = DEFAULT_SHARED_MEM_SIZE);
  virtual ~CBinaryAddonSharedMemoryWindows();

  bool CreateSharedMemory() override;
  void DestroySharedMemory() override;

  bool Lock_AddonToKodi_Addon() override;
  void Unlock_AddonToKodi_Kodi() override;

  bool Lock_KodiToAddon_Addon() override;
  void Unlock_KodiToAddon_Kodi() override;
  void Unlock_KodiToAddon_Addon() override;

private:
  HANDLE m_handle_AddonToKodi;
  HANDLE m_semaphore_AddonToKodi_Kodi;
  HANDLE m_semaphore_AddonToKodi_Addon;

  HANDLE m_handle_KodiToAddon;
  HANDLE m_semaphore_KodiToAddon_Kodi;
  HANDLE m_semaphore_KodiToAddon_Addon;
};

#endif /* (defined TARGET_WINDOWS) */
