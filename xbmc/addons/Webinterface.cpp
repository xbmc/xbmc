/*
 *  Copyright (C) 2015-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Webinterface.h"

#include "addons/addoninfo/AddonType.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

using namespace ADDON;

namespace
{
constexpr const char* WEBINTERFACE_DEFAULT_ENTRY_POINT = "index.html";

} // unnamed namespace

CWebinterface::CWebinterface(const AddonInfoPtr& addonInfo)
  : CAddon(addonInfo, AddonType::WEB_INTERFACE), m_entryPoint(WEBINTERFACE_DEFAULT_ENTRY_POINT)

{
  // determine the type of the webinterface
  std::string webinterfaceType = Type(AddonType::WEB_INTERFACE)->GetValue("@type").asString();
  if (StringUtils::EqualsNoCase(webinterfaceType, "wsgi"))
    m_type = WebinterfaceType::TYPE_WSGI;
  else if (!webinterfaceType.empty() && !StringUtils::EqualsNoCase(webinterfaceType, "static") && !StringUtils::EqualsNoCase(webinterfaceType, "html"))
    CLog::LogF(LOGWARNING, R"(Addon "{}" has specified an unsupported type "{}")", ID(),
               webinterfaceType);

  // determine the entry point of the webinterface
  std::string entry = Type(AddonType::WEB_INTERFACE)->GetValue("@entry").asString();
  if (!entry.empty())
    m_entryPoint = entry;
}

std::string CWebinterface::GetEntryPoint(const std::string &path) const
{
  if (m_type == WebinterfaceType::TYPE_WSGI)
    return LibPath();

  return URIUtils::AddFileToFolder(path, m_entryPoint);
}

std::string CWebinterface::GetBaseLocation() const
{
  if (m_type == WebinterfaceType::TYPE_WSGI)
    return "/addons/" + ID();

  return "";
}
