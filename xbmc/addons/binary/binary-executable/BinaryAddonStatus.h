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

#include "addons/kodi-addon-dev-kit/include/kodi/api2/.internal/AddonLib_internal.hpp"

#include "threads/CriticalSection.h"
#include "threads/Thread.h"

#include <list>

namespace ADDON
{

  class CBinaryAddon;

  /**
   * Value to be used as timeout if a add-on is disabled and related process
   * identifier (PID) is still present. The countdown is the time in seconds
   * given for add-on to exit itself, otherwise becomes it shot down from Kodi.
   *
   * This kill can be inhibited from add-on by set of the properties value
   * on structure addon_properties.is_independent to true.
   */
  #define PROCESS_KILL_COUNTDOWN 10 // seconds

  /**
   * @class CBinaryAddonStatus
   * @brief Class checks for currently opened add-ons
   *
   * If they seems no more present (crash maybe there on add-on?) becomes for
   * this add-on everything stopped.
   */
  class CBinaryAddonStatus : public CThread
  {
  public:
    CBinaryAddonStatus();
    virtual ~CBinaryAddonStatus();

    /**
     * @brief Handle Kodi's system shutdown
     *
     * @todo Need to improve after add-ons are present for testing and to make
     * sure on Kodi's exit all add-ons a also exited!
     */
    void Shutdown();

    /**
     * @brief To add a binary add-on to the status check list from here.
     *
     * @param[in] addon       Pointer of created binary add-on class to store here
     */
    void AddAddon(CBinaryAddon* addon);

    /**
     * @brief Global static function to checks the system about a presence of
     * asked process identifier.
     *
     * @param[in] processId   Process identification code to check
     */
    static bool CheckPresenceOfProcess(uint64_t processId);

    /**
     * @brief Structure where text names about add-on system error codes are
     * stored.
     *
     * Used to translate interger value to a human readable string.
     */
    static const KODI_API_ErrorTranslator errorTranslator[];

  protected:
    /**
     * @brief Process thread to check add-ons on background
     */
    virtual void Process(void);

  private:
    /**
     * @brief Function to kill external application on given process identifier.
     *
     * @param[in] processId   Process identification code to kill
     */
    bool KillOfProcess(uint64_t processId);

    /**
     * @brief List stored the currently used binary add-ons to observe.
     * @{
     */
    typedef std::list<CBinaryAddon*> AddonList;
    AddonList         m_addons;
    /** @} */

    CCriticalSection  m_mutex;
  };

}; /* namespace ADDON */
