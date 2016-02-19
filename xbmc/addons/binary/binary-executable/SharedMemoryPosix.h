#pragma once
/*
 *      Copyright (C) 2010-2015 Team KODI
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

#if (defined TARGET_POSIX)

#include "SharedMemory.h"

#include <semaphore.h>

namespace ADDON
{

  class CBinaryAddonSharedMemoryPosix
    : public CBinaryAddonSharedMemory
  {
  public:
    CBinaryAddonSharedMemoryPosix(int randNumber, CBinaryAddon* addon, size_t size = DEFAULT_SHARED_MEM_SIZE);
    virtual ~CBinaryAddonSharedMemoryPosix();

    bool CreateSharedMemory() override;
    void DestroySharedMemory() override;

    bool Lock_AddonToKodi_Kodi() override;
    void Unlock_AddonToKodi_Kodi() override;
    void Unlock_AddonToKodi_Addon() override;
    
    bool Lock_KodiToAddon_Kodi() override;
    void Unlock_KodiToAddon_Kodi() override;
    void Unlock_KodiToAddon_Addon() override;

  private:
    int     m_shmId_FromAddonToKodi;
    int     m_shmId_FromKodiToAddon;
  };

}; /* namespace ADDON */

#endif /* (defined TARGET_POSIX) */
