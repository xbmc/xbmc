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

  for (const auto& values : addonInfo->Type(addonType)->GetValues())
  {
    if (values.first != "medialibraryscanpath")
      continue;

    std::string url = "plugin://" + ID() + '/';
    std::string content = values.second.GetValue("medialibraryscanpath@content").asString();
    std::string path = values.second.GetValue("medialibraryscanpath").asString();
    if (!path.empty() && path.front() == '/')
      path.erase(0, 1);
    if (path.compare(0, url.size(), url))
      path.insert(0, url);
    m_mediaLibraryScanPaths[content].push_back(CURL(path).GetFileName());
  }

  SetProvides(provides);
}

void CPluginSource::SetProvides(const std::string &content)
{
  if (!content.empty())
  {
    std::vector<std::string> provides = StringUtils::Split(content, ' ');
    for (std::vector<std::string>::const_iterator i = provides.begin(); i != provides.end(); ++i)
    {
      Content content = Translate(*i);
      if (content != UNKNOWN)
        m_providedContent.insert(content);
    }
  }
  if (Type() == AddonType::SCRIPT && m_providedContent.empty())
    m_providedContent.insert(EXECUTABLE);
}

CPluginSource::Content CPluginSource::Translate(const std::string &content)
{
  if (content == "audio")
    return CPluginSource::AUDIO;
  else if (content == "image")
    return CPluginSource::IMAGE;
  else if (content == "executable")
    return CPluginSource::EXECUTABLE;
  else if (content == "video")
    return CPluginSource::VIDEO;
  else if (content == "game")
    return CPluginSource::GAME;
  else
    return CPluginSource::UNKNOWN;
}

bool CPluginSource::HasType(AddonType type) const
{
  return ((type == AddonType::VIDEO && Provides(VIDEO)) ||
          (type == AddonType::AUDIO && Provides(AUDIO)) ||
          (type == AddonType::IMAGE && Provides(IMAGE)) ||
          (type == AddonType::GAME && Provides(GAME)) ||
          (type == AddonType::EXECUTABLE && Provides(EXECUTABLE)) || (type == CAddon::Type()));
}

} /*namespace ADDON*/

