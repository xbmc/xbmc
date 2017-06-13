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

std::unique_ptr<CWebinterface> CWebinterface::FromExtension(const AddonInfoPtr& addonInfo, const cp_extension_t* ext)
{
  // determine the type of the webinterface
  WebinterfaceType type(WebinterfaceTypeStatic);
  std::string webinterfaceType = CAddonMgr::GetInstance().GetExtValue(ext->configuration, "@type");
  if (StringUtils::EqualsNoCase(webinterfaceType.c_str(), "wsgi"))
    type = WebinterfaceTypeWsgi;
  else if (!webinterfaceType.empty() && !StringUtils::EqualsNoCase(webinterfaceType.c_str(), "static") && !StringUtils::EqualsNoCase(webinterfaceType.c_str(), "html"))
    CLog::Log(LOGWARNING, "Webinterface addon \"%s\" has specified an unsupported type \"%s\"", addonInfo->ID().c_str(), webinterfaceType.c_str());

  // determine the entry point of the webinterface
  std::string entryPoint(WEBINTERFACE_DEFAULT_ENTRY_POINT);
  std::string entry = CAddonMgr::GetInstance().GetExtValue(ext->configuration, "@entry");
  if (!entry.empty())
    entryPoint = entry;

  return std::unique_ptr<CWebinterface>(new CWebinterface(addonInfo, type, entryPoint));
}

CWebinterface::CWebinterface(const AddonInfoPtr& addonInfo, WebinterfaceType type,
    const std::string &entryPoint) : CAddon(addonInfo), m_type(type), m_entryPoint(entryPoint)
{ }

std::string CWebinterface::GetEntryPoint(const std::string &path) const
{
  if (m_type == WebinterfaceTypeWsgi)
    return LibPath();

  return URIUtils::AddFileToFolder(path, m_entryPoint);
}

std::string CWebinterface::GetBaseLocation() const
{
  if (m_type == WebinterfaceTypeWsgi)
    return "/addons/" + ID();

  return "";
}
