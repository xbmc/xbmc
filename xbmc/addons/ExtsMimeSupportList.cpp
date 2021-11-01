/*
 *  Copyright (C) 2013 Arne Morten Kvarving
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ExtsMimeSupportList.h"

#include "ServiceBroker.h"
#include "addons/AddonManager.h"
#include "addons/kodi-dev-kit/include/kodi/addon-instance/AudioDecoder.h"
#include "guilib/LocalizeStrings.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

using namespace ADDON;
using namespace KODI::ADDONS;

CExtsMimeSupportList::CExtsMimeSupportList(CAddonMgr& addonMgr) : m_addonMgr(addonMgr)
{
  m_addonMgr.Events().Subscribe(this, &CExtsMimeSupportList::OnEvent);

  // Load all available audio decoder addons during Kodi start
  std::vector<std::shared_ptr<CAddonInfo>> addonInfos;
  if (m_addonMgr.GetAddonInfos(addonInfos, true, ADDON_AUDIODECODER))
  {
    for (const auto& addonInfo : addonInfos)
      m_supportedList.emplace_back(ScanAddonProperties(ADDON_AUDIODECODER, addonInfo));
  }
}

CExtsMimeSupportList::~CExtsMimeSupportList()
{
  m_addonMgr.Events().Unsubscribe(this);
}

void CExtsMimeSupportList::OnEvent(const AddonEvent& event)
{
  if (typeid(event) == typeid(AddonEvents::Enabled) ||
      typeid(event) == typeid(AddonEvents::Disabled) ||
      typeid(event) == typeid(AddonEvents::ReInstalled))
  {
    if (m_addonMgr.HasType(event.id, ADDON_AUDIODECODER))
      Update(event.id);
  }
  else if (typeid(event) == typeid(AddonEvents::UnInstalled))
  {
    Update(event.id);
  }
}

void CExtsMimeSupportList::Update(const std::string& id)
{
  // Stop used instance if present, otherwise the new becomes created on already created addon base one.
  {
    CSingleLock lock(m_critSection);

    const auto itAddon =
        std::find_if(m_supportedList.begin(), m_supportedList.end(),
                     [&id](const SupportValues& addon) { return addon.m_addonInfo->ID() == id; });

    if (itAddon != m_supportedList.end())
    {
      m_supportedList.erase(itAddon);
    }
  }

  // Create and init the new audio decoder addon instance
  std::shared_ptr<CAddonInfo> addonInfo = m_addonMgr.GetAddonInfo(id, ADDON_AUDIODECODER);
  if (addonInfo && !m_addonMgr.IsAddonDisabled(id))
  {
    SupportValues values = ScanAddonProperties(ADDON_AUDIODECODER, addonInfo);
    {
      CSingleLock lock(m_critSection);
      m_supportedList.emplace_back(values);
    }
  }
}

CExtsMimeSupportList::SupportValues CExtsMimeSupportList::ScanAddonProperties(
    ADDON::TYPE type, const std::shared_ptr<CAddonInfo>& addonInfo)
{
  SupportValues values;

  values.m_addonType = type;
  values.m_addonInfo = addonInfo;
  if (type == ADDON_AUDIODECODER)
  {
    values.m_codecName = addonInfo->Type(type)->GetValue("@name").asString();
    values.m_hasTags = addonInfo->Type(type)->GetValue("@tags").asBoolean();
    values.m_hasTracks = addonInfo->Type(type)->GetValue("@tracks").asBoolean();
  }

  auto exts = StringUtils::Split(
      addonInfo->Type(ADDON_AUDIODECODER)->GetValue("@extension").asString(), "|");
  for (const auto& ext : exts)
  {
    size_t iColon = ext.find('(');
    if (iColon != std::string::npos)
      values.m_supportedExtensions.emplace(ext.substr(0, iColon),
                                           atoi(ext.substr(iColon + 1).c_str()));
    else
      values.m_supportedExtensions.emplace(ext, -1);
  }

  // Check addons support tracks, if yes add extension about related entry names
  // By them addon no more need to add itself on his addon.xml
  if (values.m_hasTracks)
    values.m_supportedExtensions.emplace("." + values.m_codecName + "stream", -1);

  auto mimes = StringUtils::Split(
      addonInfo->Type(ADDON_AUDIODECODER)->GetValue("@mimetype").asString(), "|");
  for (const auto& mime : mimes)
  {
    size_t iColon = mime.find('(');
    if (iColon != std::string::npos)
      values.m_supportedMimetypes.emplace(mime.substr(0, iColon),
                                          atoi(mime.substr(iColon + 1).c_str()));
    else
      values.m_supportedMimetypes.emplace(mime, -1);
  }

  return values;
}

std::vector<CExtsMimeSupportList::SupportValues> CExtsMimeSupportList::GetSupportedAddonInfos(
    FilterSelect select)
{
  std::vector<SupportValues> addonInfos;

  CSingleLock lock(m_critSection);

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
  CSingleLock lock(m_critSection);

  for (const auto& entry : m_supportedList)
  {
    const auto it =
        std::find_if(entry.m_supportedExtensions.begin(), entry.m_supportedExtensions.end(),
                     [ext](const std::pair<std::string, int>& v) { return v.first == ext; });
    if (it != entry.m_supportedExtensions.end())
      return true;
  }

  return false;
}

std::vector<std::pair<ADDON::TYPE, std::shared_ptr<ADDON::CAddonInfo>>> CExtsMimeSupportList::
    GetExtensionSupportedAddonInfos(const std::string& ext, FilterSelect select)
{
  std::vector<std::pair<ADDON::TYPE, std::shared_ptr<CAddonInfo>>> addonInfos;

  CSingleLock lock(m_critSection);

  for (const auto& entry : m_supportedList)
  {
    const auto it =
        std::find_if(entry.m_supportedExtensions.begin(), entry.m_supportedExtensions.end(),
                     [ext](const std::pair<std::string, int>& v) { return v.first == ext; });
    if (it != entry.m_supportedExtensions.end() &&
        (select == FilterSelect::all || (select == FilterSelect::hasTags && entry.m_hasTags) ||
         (select == FilterSelect::hasTracks && entry.m_hasTracks)))
      addonInfos.emplace_back(entry.m_addonType, entry.m_addonInfo);
  }

  return addonInfos;
}

bool CExtsMimeSupportList::IsMimetypeSupported(const std::string& mimetype)
{
  CSingleLock lock(m_critSection);

  for (const auto& entry : m_supportedList)
  {

    const auto it = std::find_if(
        entry.m_supportedMimetypes.begin(), entry.m_supportedMimetypes.end(),
        [mimetype](const std::pair<std::string, int>& v) { return v.first == mimetype; });
    if (it != entry.m_supportedMimetypes.end())
      return true;
  }

  return false;
}

std::vector<std::pair<ADDON::TYPE, std::shared_ptr<CAddonInfo>>> CExtsMimeSupportList::
    GetMimetypeSupportedAddonInfos(const std::string& mimetype, FilterSelect select)
{
  std::vector<std::pair<ADDON::TYPE, std::shared_ptr<CAddonInfo>>> addonInfos;

  CSingleLock lock(m_critSection);

  for (const auto& entry : m_supportedList)
  {
    const auto it = std::find_if(
        entry.m_supportedMimetypes.begin(), entry.m_supportedMimetypes.end(),
        [mimetype](const std::pair<std::string, int>& v) { return v.first == mimetype; });
    if (it != entry.m_supportedMimetypes.end() &&
        (select == FilterSelect::all || (select == FilterSelect::hasTags && entry.m_hasTags) ||
         (select == FilterSelect::hasTracks && entry.m_hasTracks)))
      addonInfos.emplace_back(entry.m_addonType, entry.m_addonInfo);
  }

  return addonInfos;
}
