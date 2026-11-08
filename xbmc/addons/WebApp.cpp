/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "WebApp.h"

#include "addons/AddonVersion.h"
#include "addons/addoninfo/AddonInfo.h"
#include "addons/addoninfo/AddonType.h"
#include "filesystem/File.h"
#include "utils/XBMCTinyXML.h"
#include "utils/XMLUtils.h"
#include "utils/log.h"

#include <algorithm>
#include <set>

using namespace ADDON;

namespace
{
constexpr const char* KODI_SETTINGS_FILE = "special://xbmc/system/settings/webapp.xml";
constexpr const char* KODI_SETTINGS_VERSION = "1.3.0";

TiXmlElement* SectionOf(CXBMCTinyXML& doc)
{
  TiXmlElement* root = doc.RootElement();
  return root ? root->FirstChildElement("section") : nullptr;
}

void AddSettingIds(const TiXmlElement* category, std::set<std::string>& ids)
{
  for (const TiXmlElement* group = category->FirstChildElement("group"); group;
       group = group->NextSiblingElement("group"))
  {
    for (const TiXmlElement* setting = group->FirstChildElement("setting"); setting;
         setting = setting->NextSiblingElement("setting"))
      ids.insert("setting " + XMLUtils::GetAttribute(setting, "id"));
  }
}

std::set<std::string> IdsOf(const TiXmlElement* section)
{
  std::set<std::string> ids;
  for (const TiXmlElement* category = section->FirstChildElement("category"); category;
       category = category->NextSiblingElement("category"))
  {
    ids.insert("category " + XMLUtils::GetAttribute(category, "id"));
    AddSettingIds(category, ids);
  }
  return ids;
}
} // namespace

CWebApp::CWebApp(const AddonInfoPtr& addonInfo) : CAddon(addonInfo, AddonType::WEBAPP)
{
}

bool CWebApp::LeavesKodiSettingsToKodi() const
{
  const CAddonVersion version{KODI_SETTINGS_VERSION};
  const auto& dependencies = GetDependencies();
  return std::any_of(dependencies.begin(), dependencies.end(),
                     [&version](const DependencyInfo& dependency)
                     { return dependency.id == "kodi.webapp" && dependency.version >= version; });
}

bool CWebApp::LoadSettingsDefinition(const std::string& path, CXBMCTinyXML& doc, AddonInstanceId id)
{
  if (id != ADDON_SETTINGS_ID || !LeavesKodiSettingsToKodi())
    return CAddon::LoadSettingsDefinition(path, doc, id);

  CXBMCTinyXML kodiDoc;
  const TiXmlElement* kodiSection =
      kodiDoc.LoadFile(KODI_SETTINGS_FILE) ? SectionOf(kodiDoc) : nullptr;
  if (!kodiSection)
  {
    CLog::Log(LOGERROR, "CWebApp[{}]: unable to load {}", ID(), KODI_SETTINGS_FILE);
    return false;
  }

  if (!XFILE::CFile::Exists(path))
  {
    doc.Clear();
    doc.InsertEndChild(*kodiDoc.RootElement());
    SectionOf(doc)->SetAttribute("id", ID());
    return true;
  }

  if (!CAddon::LoadSettingsDefinition(path, doc, id))
    return false;

  TiXmlElement* section = SectionOf(doc);
  if (!section)
  {
    CLog::Log(LOGERROR, "CWebApp[{}]: {} has no settings section", ID(), path);
    return false;
  }

  const std::set<std::string> ids = IdsOf(section);
  for (const std::string& kodiId : IdsOf(kodiSection))
  {
    if (ids.count(kodiId))
    {
      CLog::Log(LOGERROR, "CWebApp[{}]: {} defines the {}, which Kodi defines for every web app",
                ID(), path, kodiId);
      return false;
    }
  }

  for (const TiXmlElement* category = kodiSection->FirstChildElement("category"); category;
       category = category->NextSiblingElement("category"))
    section->InsertEndChild(*category);
  return true;
}
