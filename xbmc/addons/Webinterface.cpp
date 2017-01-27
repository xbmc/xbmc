/*
 *      Copyright (C) 2015 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "Webinterface.h"
#include "addons/AddonManager.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"

using namespace ADDON;

CWebinterface::CWebinterface(ADDON::CAddonInfo addonInfo)
  : CAddon(std::move(addonInfo)),
    m_type(WebinterfaceTypeStatic),
    m_entryPoint(WEBINTERFACE_DEFAULT_ENTRY_POINT)
{
  // determine the type of the webinterface
  std::string webinterfaceType = AddonInfo()->GetValue("@type").asString();
  if (StringUtils::EqualsNoCase(webinterfaceType, "wsgi"))
    m_type = WebinterfaceTypeWsgi;
  else if (!webinterfaceType.empty() && !StringUtils::EqualsNoCase(webinterfaceType, "static") && !StringUtils::EqualsNoCase(webinterfaceType, "html"))
    CLog::Log(LOGWARNING, "Webinterface addon \"%s\" has specified an unsupported type \"%s\"", addonInfo.m_id.c_str(), webinterfaceType.c_str());

  // determine the entry point of the webinterface
  std::string entry = AddonInfo()->GetValue("@entry").asString();
  if (!entry.empty())
    m_entryPoint = entry;
}

std::string CWebinterface::GetEntryPoint(const std::string &path) const
{
  if (m_type == WebinterfaceTypeWsgi)
    return AddonInfo()->LibPath();

  return URIUtils::AddFileToFolder(path, m_entryPoint);
}

std::string CWebinterface::GetBaseLocation() const
{
  if (m_type == WebinterfaceTypeWsgi)
    return "/addons/" + AddonInfo()->ID();

  return "";
}
