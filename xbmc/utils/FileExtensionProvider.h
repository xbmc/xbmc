/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "addons/AddonInfo.h"
#include "addons/AddonEvents.h"
#include "settings/AdvancedSettings.h"

namespace ADDON
{
  class CAddonMgr;
  class CBinaryAddonManager;
}

class CFileExtensionProvider
{
public:
  CFileExtensionProvider(ADDON::CAddonMgr &addonManager,
                         ADDON::CBinaryAddonManager &binaryAddonManager);
  ~CFileExtensionProvider();

  /*!
   * @brief Returns a list of picture extensions
   */
  std::string GetPictureExtensions() const;

  /*!
   * @brief Returns a list of music extensions
   */
  std::string GetMusicExtensions() const;

  /*!
   * @brief Returns a list of video extensions
   */
  std::string GetVideoExtensions() const;

  /*!
   * @brief Returns a list of subtitle extensions
   */
  std::string GetSubtitleExtensions() const;

  /*!
   * @brief Returns a list of disc stub extensions
   */
  std::string GetDiscStubExtensions() const;

  /*!
   * @brief Returns a file folder extensions
   */
  std::string GetFileFolderExtensions() const;

private:
  std::string GetAddonExtensions(const ADDON::TYPE &type) const;
  std::string GetAddonFileFolderExtensions(const ADDON::TYPE &type) const;
  void SetAddonExtensions();
  void SetAddonExtensions(const ADDON::TYPE &type);

  void OnAddonEvent(const ADDON::AddonEvent& event);

  // Construction properties
  ADDON::CAddonMgr &m_addonManager;
  ADDON::CBinaryAddonManager &m_binaryAddonManager;

  // File extension properties
  std::map<ADDON::TYPE, std::string> m_addonExtensions;
  std::shared_ptr<CAdvancedSettings> m_advancedSettings;
  std::map<ADDON::TYPE, std::string> m_addonFileFolderExtensions;
};
