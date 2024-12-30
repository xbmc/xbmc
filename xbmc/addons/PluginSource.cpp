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
      if (content != Content::UNKNOWN)
        m_providedContent.insert(content);
    }
  }
  if (Type() == AddonType::SCRIPT && m_providedContent.empty())
    m_providedContent.insert(Content::EXECUTABLE);
}

CPluginSource::Content CPluginSource::Translate(const std::string &content)
{
  if (content == "audio")
    return CPluginSource::Content::AUDIO;
  else if (content == "image")
    return CPluginSource::Content::IMAGE;
  else if (content == "executable")
    return CPluginSource::Content::EXECUTABLE;
  else if (content == "video")
    return CPluginSource::Content::VIDEO;
  else if (content == "game")
    return CPluginSource::Content::GAME;
  else
    return CPluginSource::Content::UNKNOWN;
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

