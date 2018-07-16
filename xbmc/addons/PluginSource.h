/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "Addon.h"

namespace ADDON
{

typedef std::map<std::string, std::vector<std::string>> ContentPathMap;

class CPluginSource : public CAddon
{
public:

  enum Content { UNKNOWN, AUDIO, IMAGE, EXECUTABLE, VIDEO, GAME };

  static std::unique_ptr<CPluginSource> FromExtension(CAddonInfo addonInfo, const cp_extension_t* ext);

  explicit CPluginSource(CAddonInfo addonInfo);
  CPluginSource(CAddonInfo addonInfo, const std::string& provides);

  TYPE FullType() const override;
  bool IsType(TYPE type) const override;
  bool Provides(const Content& content) const
  {
    return content == UNKNOWN ? false : m_providedContent.count(content) > 0;
  }

  bool ProvidesSeveral() const
  {
    return m_providedContent.size() > 1;
  }

  const ContentPathMap& MediaLibraryScanPaths() const
  {
    return m_mediaLibraryScanPaths;
  }

  static Content Translate(const std::string &content);
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
