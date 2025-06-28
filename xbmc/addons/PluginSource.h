/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "addons/Addon.h"

#include <set>
#include <string_view>

namespace ADDON
{

using ContentPathMap = std::map<std::string, std::vector<std::string>, std::less<>>;

class CPluginSource : public CAddon
{
public:
  enum class Content
  {
    UNKNOWN,
    AUDIO,
    IMAGE,
    EXECUTABLE,
    VIDEO,
    GAME
  };

  explicit CPluginSource(const AddonInfoPtr& addonInfo, AddonType addonType);

  bool HasType(AddonType type) const override;
  bool Provides(const Content& content) const
  {
    return content == Content::UNKNOWN ? false : m_providedContent.contains(content);
  }

  bool ProvidesSeveral() const
  {
    return m_providedContent.size() > 1;
  }

  const ContentPathMap& MediaLibraryScanPaths() const
  {
    return m_mediaLibraryScanPaths;
  }

  static Content Translate(std::string_view content);

private:
  /*! \brief Set the provided content for this plugin
   If no valid content types are passed in, we set the EXECUTABLE type
   \param content a space-separated list of content types
   */
  void SetProvides(const std::string &content);
  std::set<Content> m_providedContent;
  ContentPathMap m_mediaLibraryScanPaths;
};

} /*namespace ADDON*/
