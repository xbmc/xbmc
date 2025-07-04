/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PluginSource.h"

#include "URL.h"
#include "addons/addoninfo/AddonInfo.h"
#include "addons/addoninfo/AddonType.h"
#include "utils/StringUtils.h"

#include <utility>

namespace ADDON
{

CPluginSource::CPluginSource(const AddonInfoPtr& addonInfo, AddonType addonType)
  : CAddon(addonInfo, addonType)
{
  std::string provides = addonInfo->Type(addonType)->GetValue("provides").asString();

  for (const auto& [name, extensions] : addonInfo->Type(addonType)->GetValues())
  {
    if (name != "medialibraryscanpath")
      continue;

    const std::string url = "plugin://" + ID() + '/';
    const std::string content = extensions.GetValue("medialibraryscanpath@content").asString();
    std::string path = extensions.GetValue("medialibraryscanpath").asString();
    if (!path.empty() && path.front() == '/')
      path.erase(0, 1);
    if (path.compare(0, url.size(), url))
      path.insert(0, url);
    m_mediaLibraryScanPaths[content].emplace_back(CURL(path).GetFileName());
  }

  SetProvides(provides);
}

void CPluginSource::SetProvides(const std::string &content)
{
  if (!content.empty())
  {
    const std::vector<std::string> provides = StringUtils::Split(content, ' ');
    for (const auto& i : provides)
    {
      const Content content = Translate(i);
      if (content != Content::UNKNOWN)
        m_providedContent.insert(content);
    }
  }
  if (Type() == AddonType::SCRIPT && m_providedContent.empty())
    m_providedContent.insert(Content::EXECUTABLE);
}

CPluginSource::Content CPluginSource::Translate(std::string_view content)
{
  using enum ADDON::CPluginSource::Content;
  if (content == "audio")
    return AUDIO;
  else if (content == "image")
    return IMAGE;
  else if (content == "executable")
    return EXECUTABLE;
  else if (content == "video")
    return VIDEO;
  else if (content == "game")
    return GAME;
  else
    return UNKNOWN;
}

bool CPluginSource::HasType(AddonType type) const
{
  return ((type == AddonType::VIDEO && Provides(Content::VIDEO)) ||
          (type == AddonType::AUDIO && Provides(Content::AUDIO)) ||
          (type == AddonType::IMAGE && Provides(Content::IMAGE)) ||
          (type == AddonType::GAME && Provides(Content::GAME)) ||
          (type == AddonType::EXECUTABLE && Provides(Content::EXECUTABLE)) ||
          (type == CAddon::Type()));
}

} /*namespace ADDON*/

