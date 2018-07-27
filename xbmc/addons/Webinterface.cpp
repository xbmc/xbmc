/*
 *  Copyright (C) 2015-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Webinterface.h"
#include "ServiceBroker.h"
#include "addons/AddonManager.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"

using namespace ADDON;

std::unique_ptr<CWebinterface> CWebinterface::FromExtension(CAddonInfo addonInfo, const cp_extension_t* ext)
{
  // determine the type of the webinterface
  WebinterfaceType type(WebinterfaceTypeStatic);
  std::string webinterfaceType = CServiceBroker::GetAddonMgr().GetExtValue(ext->configuration, "@type");
  if (StringUtils::EqualsNoCase(webinterfaceType.c_str(), "wsgi"))
    type = WebinterfaceTypeWsgi;
  else if (!webinterfaceType.empty() && !StringUtils::EqualsNoCase(webinterfaceType.c_str(), "static") && !StringUtils::EqualsNoCase(webinterfaceType.c_str(), "html"))
    CLog::Log(LOGWARNING, "Webinterface addon \"%s\" has specified an unsupported type \"%s\"", addonInfo.ID().c_str(), webinterfaceType.c_str());

  // determine the entry point of the webinterface
  std::string entryPoint(WEBINTERFACE_DEFAULT_ENTRY_POINT);
  std::string entry = CServiceBroker::GetAddonMgr().GetExtValue(ext->configuration, "@entry");
  if (!entry.empty())
    entryPoint = entry;

  return std::unique_ptr<CWebinterface>(new CWebinterface(std::move(addonInfo), type, entryPoint));
}

CWebinterface::CWebinterface(ADDON::CAddonInfo addonInfo, WebinterfaceType type,
    const std::string &entryPoint) : CAddon(std::move(addonInfo)), m_type(type), m_entryPoint(entryPoint)
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
