/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PluginSource.h"

#include <utility>

#include "AddonManager.h"
#include "ServiceBroker.h"
#include "utils/StringUtils.h"
#include "URL.h"

namespace ADDON
{

std::unique_ptr<CPluginSource> CPluginSource::FromExtension(CAddonInfo addonInfo, const cp_extension_t* ext)
{
  std::string provides = CServiceBroker::GetAddonMgr().GetExtValue(ext->configuration, "provides");
  if (!provides.empty())
    addonInfo.AddExtraInfo("provides", provides);
  CPluginSource* p = new CPluginSource(std::move(addonInfo), provides);

  ELEMENTS elements;
  if (CServiceBroker::GetAddonMgr().GetExtElements(ext->configuration, "medialibraryscanpath", elements))
  {
    std::string url = "plugin://" + p->ID() + '/';
    for (const auto& elem : elements)
    {
      std::string content = CServiceBroker::GetAddonMgr().GetExtValue(elem, "@content");
      if (content.empty())
        continue;
      std::string path;
      if (elem->value)
        path.assign(elem->value);
      if (!path.empty() && path.front() == '/')
        path.erase(0, 1);
      if (path.compare(0, url.size(), url))
        path.insert(0, url);
      p->m_mediaLibraryScanPaths[content].push_back(CURL(path).GetFileName());
    }
  }

  return std::unique_ptr<CPluginSource>(p);
}

CPluginSource::CPluginSource(CAddonInfo addonInfo) : CAddon(std::move(addonInfo))
{
  std::string provides;
  InfoMap::const_iterator i = m_addonInfo.ExtraInfo().find("provides");
  if (i != m_addonInfo.ExtraInfo().end())
    provides = i->second;
  SetProvides(provides);
}

CPluginSource::CPluginSource(CAddonInfo addonInfo, const std::string& provides)
  : CAddon(std::move(addonInfo))
{
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
  if (Type() == ADDON_SCRIPT && m_providedContent.empty())
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

TYPE CPluginSource::FullType() const
{
  if (Provides(VIDEO))
    return ADDON_VIDEO;
  if (Provides(AUDIO))
    return ADDON_AUDIO;
  if (Provides(IMAGE))
    return ADDON_IMAGE;
  if (Provides(GAME))
    return ADDON_GAME;
  if (Provides(EXECUTABLE))
    return ADDON_EXECUTABLE;

  return CAddon::FullType();
}

bool CPluginSource::IsType(TYPE type) const
{
  return ((type == ADDON_VIDEO && Provides(VIDEO))
       || (type == ADDON_AUDIO && Provides(AUDIO))
       || (type == ADDON_IMAGE && Provides(IMAGE))
       || (type == ADDON_GAME && Provides(GAME))
       || (type == ADDON_EXECUTABLE && Provides(EXECUTABLE)));
}

} /*namespace ADDON*/

