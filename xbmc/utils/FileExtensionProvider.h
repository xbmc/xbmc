/*
 *  Copyright (C) 2012-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "threads/CriticalSection.h"

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace ADDON
{
enum class AddonType;
class CAddonMgr;
}

class CAdvancedSettings;

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
   * @brief Returns a list of archive extensions with a single dot (eg. .zip)
   */
  std::string GetArchiveExtensions() const;

  /*!
   * @brief Returns a list of archive extensions with a multiple dots (eg. .tar.gz)
   */
  std::string GetCompoundArchiveExtensions() const;

  /*!
   * @brief Returns a file folder extensions
   */
  std::string GetFileFolderExtensions() const;

  /*!
   * @brief Returns whether a url protocol from add-ons use encoded hostnames
   */
  bool EncodedHostName(const std::string& protocol) const;

  /*!
   * @brief Returns true if related provider can operate related file
   *
   * @note Thought for cases e.g. by ISO, where can be a video or also a SACD.
   */
  static bool CanOperateExtension(const std::string& path);

private:
  std::string GetAddonExtensions(ADDON::AddonType type) const;
  std::string GetAddonFileFolderExtensions(ADDON::AddonType type) const;
  void SetAddonExtensions();
  void SetAddonExtensions(ADDON::AddonType type);

  void OnAdvancedSettingsLoaded();

  // Construction properties
  std::shared_ptr<CAdvancedSettings> m_advancedSettings;
  ADDON::CAddonMgr &m_addonManager;
  std::optional<int> m_callbackId;

  mutable CCriticalSection m_critSection;

  // File extension properties
  std::map<ADDON::AddonType, std::string> m_addonExtensions;
  std::map<ADDON::AddonType, std::string> m_addonFileFolderExtensions;

  // Protocols from add-ons with encoded host names
  std::vector<std::string> m_encoded;

  // Cached extensions lists - use through atomic operations only
  //
  // @todo: use the safer C++20 std::atomic<std::shared_ptr<std::string>> partial specialization
  // once available for all platform builders
  // std::atomic_load/store of std::shared_ptr<T> is deprecated in C++20 and removed in C++26
  mutable std::shared_ptr<const std::string> m_discStubExtensions;
  mutable std::shared_ptr<const std::string> m_musicExtensions;
  mutable std::shared_ptr<const std::string> m_pictureExtensions;
  mutable std::shared_ptr<const std::string> m_subtitlesExtensions;
  mutable std::shared_ptr<const std::string> m_videoExtensions;
  mutable std::shared_ptr<const std::string> m_archiveExtensions;
  mutable std::shared_ptr<const std::string> m_compoundArchiveExtensions;
  mutable std::shared_ptr<const std::string> m_fileFolderExtensions;
};
