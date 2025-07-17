/*
 *  Copyright (C) 2013 Arne Morten Kvarving
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ExtsMimeSupportList.h"

#include "ServiceBroker.h"
#include "addons/AddonEvents.h"
#include "addons/AddonManager.h"
#include "addons/addoninfo/AddonInfo.h"
#include "addons/addoninfo/AddonType.h"
#include "addons/kodi-dev-kit/include/kodi/addon-instance/AudioDecoder.h"
#include "guilib/LocalizeStrings.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

#include <mutex>

using namespace ADDON;
using namespace KODI::ADDONS;

CExtsMimeSupportList::CExtsMimeSupportList(CAddonMgr& addonMgr) : m_addonMgr(addonMgr)
{
  m_addonMgr.Events().Subscribe(
      this,
      [this](const AddonEvent& event)
      {
        if (typeid(event) == typeid(AddonEvents::Enabled) ||
            typeid(event) == typeid(AddonEvents::Disabled) ||
            typeid(event) == typeid(AddonEvents::ReInstalled))
        {
          if (m_addonMgr.HasType(event.addonId, AddonType::AUDIODECODER) ||
              m_addonMgr.HasType(event.addonId, AddonType::IMAGEDECODER))
            Update(event.addonId);
        }
        else if (typeid(event) == typeid(AddonEvents::UnInstalled))
        {
          Update(event.addonId);
        }
      });

  // Load all available audio decoder addons during Kodi start
  const std::vector<AddonType> types = {AddonType::AUDIODECODER, AddonType::IMAGEDECODER};
  const auto addonInfos = m_addonMgr.GetAddonInfos(true, types);
  for (const auto& addonInfo : addonInfos)
    m_supportedList.emplace_back(ScanAddonProperties(addonInfo->MainType(), addonInfo));
}

CExtsMimeSupportList::~CExtsMimeSupportList()
{
  m_addonMgr.Events().Unsubscribe(this);
}

void CExtsMimeSupportList::Update(const std::string& id)
{
  // Stop used instance if present, otherwise the new becomes created on already created addon base one.
  {
    std::unique_lock lock(m_critSection);

    const auto itAddon = std::ranges::find_if(m_supportedList, [&id](const SupportValues& addon)
                                              { return addon.m_addonInfo->ID() == id; });

    if (itAddon != m_supportedList.end())
    {
      m_supportedList.erase(itAddon);
    }
  }

  // Create and init the new addon instance
  std::shared_ptr<CAddonInfo> addonInfo = m_addonMgr.GetAddonInfo(id, AddonType::UNKNOWN);
  if (addonInfo && !m_addonMgr.IsAddonDisabled(id) &&
      (addonInfo->HasType(AddonType::AUDIODECODER) || addonInfo->HasType(AddonType::IMAGEDECODER)))
  {
    SupportValues values = ScanAddonProperties(addonInfo->MainType(), addonInfo);
    {
      std::unique_lock lock(m_critSection);
      m_supportedList.emplace_back(std::move(values));
    }
  }
}

CExtsMimeSupportList::SupportValues CExtsMimeSupportList::ScanAddonProperties(
    AddonType type, const std::shared_ptr<CAddonInfo>& addonInfo)
{
  SupportValues values;

  values.m_addonType = type;
  values.m_addonInfo = addonInfo;
  if (type == AddonType::AUDIODECODER)
  {
    values.m_codecName = addonInfo->Type(type)->GetValue("@name").asString();
    values.m_hasTags = addonInfo->Type(type)->GetValue("@tags").asBoolean();
    values.m_hasTracks = addonInfo->Type(type)->GetValue("@tracks").asBoolean();
  }

  const CAddonExtensions* support = addonInfo->Type(type)->GetElement("support");
  if (support)
  {
    // Scan here about complete defined xml groups with description and maybe icon
    // e.g.
    // `<extension name=".zwdsp">`
    // `  <description>30246</description>`
    // `</extension>`
    for (const auto& [extensionType, addonExtensions] : support->GetElements())
    {
      std::string name = addonExtensions.GetValue("@name").asString();
      if (name.empty())
        continue;

      const int description = addonExtensions.GetValue("description").asInteger();
      const std::string icon =
          !addonExtensions.GetValue("icon").empty()
              ? URIUtils::AddFileToFolder(addonInfo->Path(),
                                          addonExtensions.GetValue("icon").asString())
              : "";

      if (extensionType == "extension")
      {
        if (name[0] != '.')
          name.insert(name.begin(), '.');
        values.m_supportedExtensions.try_emplace(name, description, icon);
      }
      else if (extensionType == "mimetype")
      {
        values.m_supportedMimetypes.try_emplace(name, description, icon);
        const std::string extension = addonExtensions.GetValue("extension").asString();
        if (!extension.empty())
          values.m_supportedExtensions.try_emplace(extension, description, icon);
      }
    }

    // Scan here about small defined xml groups without anything
    // e.g. `<extension name=".adp"/>`
    for (const auto& [_, extValues] : support->GetValues())
    {
      for (const auto& [extName, extValue] : extValues)
      {
        std::string name = extValue.asString();
        if (extName == "extension@name")
        {
          if (name[0] != '.')
            name.insert(name.begin(), '.');
          values.m_supportedExtensions.try_emplace(name, -1, "");
        }
        else if (extName == "mimetype@name")
          values.m_supportedMimetypes.try_emplace(name, -1, "");
      }
    }
  }

  // Check addons support tracks, if yes add extension about related entry names
  // By them addon no more need to add itself on his addon.xml
  if (values.m_hasTracks)
    values.m_supportedExtensions.try_emplace(
        "." + values.m_codecName + KODI_ADDON_AUDIODECODER_TRACK_EXT, -1, "");

  return values;
}

std::vector<CExtsMimeSupportList::SupportValues> CExtsMimeSupportList::GetSupportedAddonInfos(
    FilterSelect select)
{
  std::vector<SupportValues> addonInfos;

  std::unique_lock lock(m_critSection);

  for (const auto& entry : m_supportedList)
  {
    if (select == FilterSelect::all || (select == FilterSelect::hasTags && entry.m_hasTags) ||
        (select == FilterSelect::hasTracks && entry.m_hasTracks))
      addonInfos.emplace_back(entry);
  }

  return addonInfos;
}

bool CExtsMimeSupportList::IsExtensionSupported(const std::string& ext)
{
  std::unique_lock lock(m_critSection);

  for (const auto& entry : m_supportedList)
  {
    const auto it = std::ranges::find_if(entry.m_supportedExtensions,
                                         [&ext](const std::pair<std::string, SupportValue>& v)
                                         { return v.first == ext; });
    if (it != entry.m_supportedExtensions.end())
      return true;
  }

  return false;
}

std::vector<std::pair<AddonType, std::shared_ptr<ADDON::CAddonInfo>>> CExtsMimeSupportList::
    GetExtensionSupportedAddonInfos(const std::string& ext, FilterSelect select)
{
  std::vector<std::pair<AddonType, std::shared_ptr<CAddonInfo>>> addonInfos;

  std::unique_lock lock(m_critSection);

  for (const auto& entry : m_supportedList)
  {
    const auto it = std::ranges::find_if(entry.m_supportedExtensions,
                                         [&ext](const std::pair<std::string, SupportValue>& v)
                                         { return v.first == ext; });
    if (it != entry.m_supportedExtensions.end() &&
        (select == FilterSelect::all || (select == FilterSelect::hasTags && entry.m_hasTags) ||
         (select == FilterSelect::hasTracks && entry.m_hasTracks)))
      addonInfos.emplace_back(entry.m_addonType, entry.m_addonInfo);
  }

  return addonInfos;
}

bool CExtsMimeSupportList::IsMimetypeSupported(const std::string& mimetype)
{
  std::unique_lock lock(m_critSection);

  for (const auto& entry : m_supportedList)
  {

    const auto it = std::ranges::find_if(entry.m_supportedMimetypes,
                                         [&mimetype](const std::pair<std::string, SupportValue>& v)
                                         { return v.first == mimetype; });
    if (it != entry.m_supportedMimetypes.end())
      return true;
  }

  return false;
}

std::vector<std::pair<AddonType, std::shared_ptr<CAddonInfo>>> CExtsMimeSupportList::
    GetMimetypeSupportedAddonInfos(const std::string& mimetype, FilterSelect select)
{
  std::vector<std::pair<AddonType, std::shared_ptr<CAddonInfo>>> addonInfos;

  std::unique_lock lock(m_critSection);

  for (const auto& entry : m_supportedList)
  {
    const auto it = std::ranges::find_if(entry.m_supportedMimetypes,
                                         [&mimetype](const std::pair<std::string, SupportValue>& v)
                                         { return v.first == mimetype; });
    if (it != entry.m_supportedMimetypes.end() &&
        (select == FilterSelect::all || (select == FilterSelect::hasTags && entry.m_hasTags) ||
         (select == FilterSelect::hasTracks && entry.m_hasTracks)))
      addonInfos.emplace_back(entry.m_addonType, entry.m_addonInfo);
  }

  return addonInfos;
}

std::vector<AddonSupportEntry> CExtsMimeSupportList::GetSupportedExtsAndMimeTypes(
    const std::string& addonId)
{
  std::vector<AddonSupportEntry> list;

  const auto it = std::ranges::find_if(m_supportedList, [&addonId](const SupportValues& v)
                                       { return v.m_addonInfo->ID() == addonId; });
  if (it == m_supportedList.end())
    return list;

  for (const auto& [name, supportValue] : it->m_supportedExtensions)
  {
    AddonSupportEntry supportEntry;
    supportEntry.m_type = AddonSupportType::Extension;
    supportEntry.m_name = name;
    supportEntry.m_description =
        g_localizeStrings.GetAddonString(addonId, supportValue.m_description);
    supportEntry.m_icon = supportValue.m_icon;
    list.emplace_back(std::move(supportEntry));
  }
  for (const auto& [name, supportValue] : it->m_supportedMimetypes)
  {
    AddonSupportEntry supportEntry;
    supportEntry.m_type = AddonSupportType::Mimetype;
    supportEntry.m_name = name;
    supportEntry.m_description =
        g_localizeStrings.GetAddonString(addonId, supportValue.m_description);
    supportEntry.m_icon = supportValue.m_icon;
    list.emplace_back(std::move(supportEntry));
  }

  return list;
}
