/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "addons/AddonEvents.h"
#include "addons/addoninfo/AddonInfo.h"
#include "settings/AdvancedSettings.h"

namespace ADDON
{
  class CAddonMgr;
}

class CFileExtensionProvider
{
public:
  CFileExtensionProvider(ADDON::CAddonMgr& addonManager);
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

  /*!
   * @brief Returns whether a url protocol from add-ons use encoded hostnames
   */
  bool EncodedHostName(const std::string& protocol) const;

private:
  std::string GetAddonExtensions(const ADDON::TYPE &type) const;
  std::string GetAddonFileFolderExtensions(const ADDON::TYPE &type) const;
  void SetAddonExtensions();
  void SetAddonExtensions(const ADDON::TYPE &type);

  void OnAddonEvent(const ADDON::AddonEvent& event);

  // Construction properties
  std::shared_ptr<CAdvancedSettings> m_advancedSettings;
  ADDON::CAddonMgr &m_addonManager;

  // File extension properties
  std::map<ADDON::TYPE, std::string> m_addonExtensions;
  std::map<ADDON::TYPE, std::string> m_addonFileFolderExtensions;

  // Protocols from add-ons with encoded host names
  std::vector<std::string> m_encoded;
};
