#pragma once
/*
 *      Copyright (C) 2005-2017 Team Kodi
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
 *  along with Kodi; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "addons/AddonManager.h"
#include "threads/CriticalSection.h"

#include <map>

namespace ADDON
{

  class IAddonInstanceHandler;

  class CAddonDll;
  typedef std::shared_ptr<CAddonDll> AddonDllPtr;

  class CBinaryAddonBase;
  typedef std::shared_ptr<CBinaryAddonBase> BinaryAddonBasePtr;
  typedef std::vector<BinaryAddonBasePtr> BinaryAddonBaseList;

  class CBinaryAddonManager
  {
  public:
    CBinaryAddonManager() = default;
    ~CBinaryAddonManager();

    bool Init();

    /*!
     * @brief Checks system about given type to know related add-on's are
     * installed.
     *
     * @param[in] type Add-on type to check installed
     * @return true if given type is installed
     */
    bool HasInstalledAddons(const TYPE &type) const;

    /*!
     * @brief Checks system about given type to know related add-on's are
     * installed and also minimum one enabled.
     *
     * @param[in] type Add-on type to check enabled
     * @return true if given type is enabled
     */
    bool HasEnabledAddons(const TYPE &type) const;

    /*!
     * @brief Checks whether an addon is installed.
     *
     * @param[in] addonId id of the addon
     * @param[in] type Add-on type to check installed
     * @return true if installed
     */
    bool IsAddonInstalled(const std::string& addonId, const TYPE &type = ADDON_UNKNOWN);

    /*!
     * @brief Check whether an addon has been enabled.
     *
     * @param[in] addonId id of the addon
     * @param[in] type Add-on type to check installed and enabled
     * @return true if enabled
     */
    bool IsAddonEnabled(const std::string& addonId, const TYPE &type = ADDON_UNKNOWN);

    /*!
     * @brief Get a list of add-on's with info's for the on system available
     * ones.
     *
     * @param[out] addonInfos list where finded addon information becomes stored
     * @param[in] enabledOnly If true are only enabled ones given back,
     *                        if false all on system available. Default is true.
     * @param[in] type        The requested type, with "ADDON_UNKNOWN"
     *                        are all add-on types given back who match the case
     *                        with value before.
     *                        If a type id becomes added are only add-ons
     *                        returned who match them. Default is for all types.
     */
    void GetAddonInfos(BinaryAddonBaseList& addonInfos, bool enabledOnly, const TYPE &type) const;

    /*!
     * @brief Get a list of disabled add-on's with info's for the on system
     * available ones.
     *
     * @param[out] addonInfos list where finded addon information becomes stored
     * @param[in] type        The requested type, with "ADDON_UNKNOWN"
     *                        are all add-on types given back who match the case
     *                        with value before.
     *                        If a type id becomes added are only add-ons
     *                        returned who match them. Default is for all types.
     */
    void GetDisabledAddonInfos(BinaryAddonBaseList& addonInfos, const TYPE& type);

    /*!
     * @brief To get information from a installed add-on
     *
     * @param[in] addonId the add-on id to get the info for
     * @param[in] type if used becomes used type confirmed and is supported, if
     *                 not a nullptr is returned
     * @return add-on information pointer of installed add-on
     */
    const BinaryAddonBasePtr GetInstalledAddonInfo(const std::string& addonId, const TYPE &type = ADDON_UNKNOWN) const;

    /*!
     * @brief Used from other addon manager to get active addon over a from him
     * created CAddonDll.
     *
     * @param[in] addonId related addon id string
     * @return if present the pointer to active one or nullptr if not present
     *
     */
    AddonPtr GetRunningAddon(const std::string& addonId) const;

  private:
    bool AddAddonBaseEntry(BINARY_ADDON_LIST_ENTRY& entry);

    void OnEvent(const AddonEvent& event);
    void EnableEvent(const std::string& addonId);
    void DisableEvent(const std::string& addonId);
    void InstalledChangeEvent();

    CCriticalSection m_critSection;

    typedef std::map<std::string, BinaryAddonBasePtr> BinaryAddonMgrBaseList;
    BinaryAddonMgrBaseList m_installedAddons;
    BinaryAddonMgrBaseList m_enabledAddons;
  };

} /* namespace ADDON */
