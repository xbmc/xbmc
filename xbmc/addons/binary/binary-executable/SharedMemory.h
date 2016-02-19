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

/**
 * @brief Global header of add-on dev kit itself
 */
#include "addons/binary/callbacks/AddonCallbacksAddonBase.h"
#include "addons/kodi-addon-dev-kit/include/kodi/api2/definitions.hpp"
#include "addons/kodi-addon-dev-kit/include/kodi/api2/.internal/AddonLib_internal.hpp"

/**
 * @brief Headers from Kodi side
 * @{
 */
#include "threads/CriticalSection.h"
#include "threads/Thread.h"
/** @} */

#include <string>

namespace ADDON
{
  class CBinaryAddonSharedMemoryPosix;
  class CBinaryAddonSharedMemoryWindows;
  class CBinaryAddon;

  /**
   * @class CBinaryAddonSharedMemory
   * @brief Parent shared memory control class.
   *
   * Class separated as parent to have structure for multiplatform support of
   * Kodi.
   *
   * Is used to create shared memory structure between Kodi and a add-on thread.
   * On every new thread on add-on becomes this class created for communciation,
   * is done to prevent stuck Communications if Several add-on threads want to
   * talk together with Kodi.
   *
   * Further are for communication over shared memory two separated places
   * present, one for talk from Add-on to here and one from here to there.
   */
  class CBinaryAddonSharedMemory
   : private CThread
  {
  public:
    CBinaryAddonSharedMemory(int randNumber, CBinaryAddon* addon, size_t size = DEFAULT_SHARED_MEM_SIZE);
    virtual ~CBinaryAddonSharedMemory();

    /**
     * @brief Functions used for multi platform support of Kodi.
     * @{
     */
    virtual bool CreateSharedMemory() = 0;
    virtual void DestroySharedMemory() = 0;

    virtual bool Lock_AddonToKodi_Kodi() = 0;
    virtual void Unlock_AddonToKodi_Kodi() = 0;
    virtual void Unlock_AddonToKodi_Addon() = 0;

    virtual bool Lock_KodiToAddon_Kodi() = 0;
    virtual void Unlock_KodiToAddon_Kodi() = 0;
    virtual void Unlock_KodiToAddon_Addon() = 0;
    /** @} */

    /**
     * @brief Connection number used here and on add-on to identify
     * shared memory.
     *
     * Is pointed public with 'const' condition to have Id available
     * for all places where it is needed in Kodi.
     */
    const int     m_randomConnectionNumber;

    /**
     * @brief Size for global shared memory between Kodi and Add-on.
     */
    const size_t  m_sharedMemSize;

  protected:
    virtual void Process(void);

  private:
    /**
     * @brief This class is direct used here and with already
     * present thread on CBinaryAddon is it not possible to use this
     * as normal child.
     */
    friend class CBinaryAddon;

    /**
     * @brief Use the both classes as friend.
     *
     * These are separated from here to hold a better overview
     * for cross platform support on Kodi.
     * @{
     */
    friend class CBinaryAddonSharedMemoryPosix;
    friend class CBinaryAddonSharedMemoryWindows;
    /** @} */

    /** @brief Shared memory pointer used for Communication from Add-on to Kodi */
    KodiAPI_ShareData*  m_sharedMem_AddonToKodi;
    /** @brief Shared memory pointer used for Communication from Kodi to Add-on */
    KodiAPI_ShareData*  m_sharedMem_KodiToAddon;
    /** @brief The binary addon handler from connection itself */
    CBinaryAddon*       m_addon;
    /** @brief LockedIn value, used to prevent not wanted locks on operaion thread */
    bool                m_LoggedIn;
  };

}; /* namespace ADDON */
