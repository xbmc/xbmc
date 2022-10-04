/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "threads/CriticalSection.h"

#include <map>
#include <memory>

namespace ADDON
{

  class IAddonInstanceHandler;

  class IAddon;
  using AddonPtr = std::shared_ptr<IAddon>;

  class CAddonInfo;
  using AddonInfoPtr = std::shared_ptr<CAddonInfo>;

  class CAddonDll;
  typedef std::shared_ptr<CAddonDll> AddonDllPtr;

  class CBinaryAddonBase;
  typedef std::shared_ptr<CBinaryAddonBase> BinaryAddonBasePtr;

  class CBinaryAddonManager
  {
  public:
    CBinaryAddonManager() = default;
    CBinaryAddonManager(const CBinaryAddonManager&) = delete;
    ~CBinaryAddonManager() = default;

    /*!
     * @brief Create or get available addon instance handle base.
     *
     * On first call the binary addon base class becomes created, on every next
     * call of addon id, this becomes given again and a counter about in
     * @ref CBinaryAddonBase increased.
     *
     * @param[in] addonBase related addon base to release
     * @param[in] handler related instance handle class
     *
     * @warning This and @ref ReleaseAddonBase are only be called from
     * @ref IAddonInstanceHandler, use nowhere else allowed!
     *
     */
    BinaryAddonBasePtr GetAddonBase(const AddonInfoPtr& addonInfo,
                                    IAddonInstanceHandler* handler,
                                    AddonDllPtr& addon);

    /*!
     * @brief Release a running addon instance handle base.
     *
     * On last release call the here on map stored entry becomes
     * removed and the dll unloaded.
     *
     * @param[in] addonBase related addon base to release
     * @param[in] handler related instance handle class
     *
     */
    void ReleaseAddonBase(const BinaryAddonBasePtr& addonBase, IAddonInstanceHandler* handler);

    /*!
     * @brief Get running addon base class for a given addon id.
     *
     * @param[in] addonId the addon id
     * @return running addon base class if found, nullptr otherwise.
     *
     */
    BinaryAddonBasePtr GetRunningAddonBase(const std::string& addonId) const;

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
    mutable CCriticalSection m_critSection;

    std::map<std::string, BinaryAddonBasePtr> m_runningAddons;
  };

} /* namespace ADDON */
