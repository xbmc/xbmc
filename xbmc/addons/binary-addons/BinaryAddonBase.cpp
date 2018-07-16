/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "BinaryAddonBase.h"
#include "AddonDll.h"

#include "filesystem/SpecialProtocol.h"
#include "threads/SingleLock.h"
#include "utils/log.h"
#include "utils/URIUtils.h"
#include "utils/XMLUtils.h"

using namespace ADDON;

bool CBinaryAddonBase::Create()
{
  std::string path = CSpecialProtocol::TranslatePath(m_addonInfo.Path());

  StringUtils::TrimRight(path, "/\\");

  auto addonXmlPath = URIUtils::AddFileToFolder(path, "addon.xml");

  CXBMCTinyXML xmlDoc;
  if (!xmlDoc.LoadFile(addonXmlPath))
  {
    CLog::Log(LOGERROR, "CBinaryAddonBase::%s: Unable to load '%s', Line %d\n%s",
                                               __FUNCTION__,
                                               path.c_str(),
                                               xmlDoc.ErrorRow(),
                                               xmlDoc.ErrorDesc());
    return false;
  }

  return LoadAddonXML(xmlDoc.RootElement(), path);
}

bool CBinaryAddonBase::IsType(TYPE type) const
{
  return (m_addonInfo.MainType() == type || ProvidesSubContent(type));
}

bool CBinaryAddonBase::ProvidesSubContent(const TYPE& content, const TYPE& mainType/* = ADDON_UNKNOWN*/) const
{
  if (content == ADDON_UNKNOWN)
    return false;

  for (auto addonType : m_types)
  {
    if ((mainType == ADDON_UNKNOWN || addonType.Type() == mainType) && addonType.ProvidesSubContent(content))
      return true;
  }

  return false;
}

bool CBinaryAddonBase::ProvidesSeveralSubContents() const
{
  int contents = 0;
  for (auto addonType : m_types)
    contents += addonType.ProvidedSubContents();
  return (contents > 0);
}

bool CBinaryAddonBase::MeetsVersion(const AddonVersion &version) const
{
  return m_addonInfo.MinVersion() <= version && version <= m_addonInfo.Version();
}

AddonDllPtr CBinaryAddonBase::GetAddon(const IAddonInstanceHandler* handler)
{
  if (handler == nullptr)
  {
    CLog::Log(LOGERROR, "CBinaryAddonBase::%s: for Id '%s' called with empty instance handler", __FUNCTION__, ID().c_str());
    return nullptr;
  }

  CSingleLock lock(m_critSection);

  // If no 'm_activeAddon' is defined create it new.
  if (m_activeAddon == nullptr)
    m_activeAddon = std::make_shared<CAddonDll>(m_addonInfo, shared_from_this());

  // add the instance handler to the info to know used amount on addon
  m_activeAddonHandlers.insert(handler);

  return m_activeAddon;
}

void CBinaryAddonBase::ReleaseAddon(const IAddonInstanceHandler* handler)
{
  if (handler == nullptr)
  {
    CLog::Log(LOGERROR, "CBinaryAddonBase::%s: for Id '%s' called with empty instance handler", __FUNCTION__, ID().c_str());
    return;
  }

  CSingleLock lock(m_critSection);

  auto presentHandler = m_activeAddonHandlers.find(handler);
  if (presentHandler == m_activeAddonHandlers.end())
    return;

  m_activeAddonHandlers.erase(presentHandler);

  // if no handler is present anymore reset and delete the add-on class on informations
  if (m_activeAddonHandlers.empty())
  {
    m_activeAddon.reset();
  }
}

AddonDllPtr CBinaryAddonBase::GetActiveAddon()
{
  CSingleLock lock(m_critSection);
  return m_activeAddon;
}

const CBinaryAddonType* CBinaryAddonBase::Type(TYPE type) const
{
  if (type == ADDON_UNKNOWN)
    return &m_types[0];

  for (auto& addonType : m_types)
  {
    if (addonType.Type() == type)
      return &addonType;
  }
  return nullptr;
}

bool CBinaryAddonBase::LoadAddonXML(const TiXmlElement* baseElement, const std::string& addonPath)
{
  if (!StringUtils::EqualsNoCase(baseElement->Value(), "addon"))
  {
    CLog::Log(LOGERROR, "CBinaryAddonBase::%s: file from '%s' doesnt contain <addon>", __FUNCTION__, addonPath.c_str());
    return false;
  }

  /*
   * Parse addon.xml:
   * <extension>
   *   ...
   * </extension>
   */
  for (const TiXmlElement* child = baseElement->FirstChildElement("extension"); child != nullptr; child = child->NextSiblingElement("extension"))
  {
    const char* cstring = child->Attribute("point");
    std::string point = cstring ? cstring : "";

    if (point != "kodi.addon.metadata" && point != "xbmc.addon.metadata")
    {
      TYPE type = CAddonInfo::TranslateType(point);
      if (type == ADDON_UNKNOWN || type >= ADDON_MAX)
      {
        CLog::Log(LOGERROR, "CBinaryAddonBase::%s: file '%s' doesn't contain a valid add-on type name (%s)", __FUNCTION__, addonPath.c_str(), point.c_str());
        return false;
      }

      m_types.push_back(CBinaryAddonType(type, this, child));
    }
  }

  /*
   * If nothing is defined in addon.xml set this as unknown to have minimum one
   * instance type present.
   */
  if (m_types.empty())
    m_types.push_back(CBinaryAddonType(ADDON_UNKNOWN, this, nullptr));

  m_addonInfo.SetMainType(m_types[0].Type());
  m_addonInfo.SetLibName(m_types[0].LibName());

  return true;
}
