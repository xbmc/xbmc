/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FileExtensionProvider.h"

#include "ServiceBroker.h"
#include "addons/AddonManager.h"
#include "addons/AudioDecoder.h"
#include "addons/ExtsMimeSupportList.h"
#include "addons/ImageDecoder.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "utils/URIUtils.h"

#include <string>
#include <vector>

using namespace ADDON;
using namespace KODI::ADDONS;

const std::vector<TYPE> ADDON_TYPES = {
  ADDON_VFS,
  ADDON_IMAGEDECODER,
  ADDON_AUDIODECODER
};

CFileExtensionProvider::CFileExtensionProvider(ADDON::CAddonMgr& addonManager)
  : m_advancedSettings(CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()),
    m_addonManager(addonManager)
{
  SetAddonExtensions();

  m_addonManager.Events().Subscribe(this, &CFileExtensionProvider::OnAddonEvent);
}

CFileExtensionProvider::~CFileExtensionProvider()
{
  m_addonManager.Events().Unsubscribe(this);

  m_advancedSettings.reset();
  m_addonExtensions.clear();
}

std::string CFileExtensionProvider::GetDiscStubExtensions() const
{
  return m_advancedSettings->m_discStubExtensions;
}

std::string CFileExtensionProvider::GetMusicExtensions() const
{
  std::string extensions(m_advancedSettings->m_musicExtensions);
  extensions += '|' + GetAddonExtensions(ADDON_VFS);
  extensions += '|' + GetAddonExtensions(ADDON_AUDIODECODER);

  return extensions;
}

std::string CFileExtensionProvider::GetPictureExtensions() const
{
  std::string extensions(m_advancedSettings->m_pictureExtensions);
  extensions += '|' + GetAddonExtensions(ADDON_VFS);
  extensions += '|' + GetAddonExtensions(ADDON_IMAGEDECODER);

  return extensions;
}

std::string CFileExtensionProvider::GetSubtitleExtensions() const
{
  std::string extensions(m_advancedSettings->m_subtitlesExtensions);
  extensions += '|' + GetAddonExtensions(ADDON_VFS);

  return extensions;
}

std::string CFileExtensionProvider::GetVideoExtensions() const
{
  std::string extensions(m_advancedSettings->m_videoExtensions);
  if (!extensions.empty())
    extensions += '|';
  extensions += GetAddonExtensions(ADDON_VFS);

  return extensions;
}

std::string CFileExtensionProvider::GetFileFolderExtensions() const
{
  std::string extensions(GetAddonFileFolderExtensions(ADDON_VFS));
  if (!extensions.empty())
    extensions += '|';
  extensions += GetAddonFileFolderExtensions(ADDON_AUDIODECODER);

  return extensions;
}

bool CFileExtensionProvider::CanOperateExtension(const std::string& path) const
{
  /*!
   * @todo Improve this function to support all cases and not only audio decoder.
   */

  // Get file extensions to find addon related to it.
  std::string strExtension = URIUtils::GetExtension(path);
  StringUtils::ToLower(strExtension);
  if (!strExtension.empty() && CServiceBroker::IsBinaryAddonCacheUp())
  {
    std::vector<std::unique_ptr<KODI::ADDONS::IAddonSupportCheck>> supportList;

    auto addonInfos = CServiceBroker::GetExtsMimeSupportList().GetExtensionSupportedAddonInfos(
        strExtension, CExtsMimeSupportList::FilterSelect::all);
    for (const auto& addonInfo : addonInfos)
    {
      switch (addonInfo.first)
      {
        case ADDON_AUDIODECODER:
          supportList.emplace_back(new CAudioDecoder(addonInfo.second));
          break;
        case ADDON_IMAGEDECODER:
          supportList.emplace_back(new CImageDecoder(addonInfo.second, ""));
          break;
        default:
          break;
      }
    }

    /*!
     * We expect that other addons can support the file, and return true if
     * list empty.
     *
     * @todo Check addons can also be types in conflict with Kodi's
     * supported parts!
     *
     * @warning This part is really big ugly at the moment and as soon as possible
     * add about other addons where works with extensions!!!
     * Due to @ref GetFileFolderExtensions() call from outside place before here, becomes
     * it usable in this way, as there limited to AudioDecoder and VFS addons.
     */
    if (supportList.empty())
    {
      return true;
    }

    /*!
     * Check all found addons about support of asked file.
     */
    for (const auto& addon : supportList)
    {
      if (addon->SupportsFile(path))
        return true;
    }
  }

  /*!
   * If no file extensions present, mark it as not supported.
   */
  return false;
}

std::string CFileExtensionProvider::GetAddonExtensions(const TYPE &type) const
{
  auto it = m_addonExtensions.find(type);
  if (it != m_addonExtensions.end())
    return it->second;

  return "";
}

std::string CFileExtensionProvider::GetAddonFileFolderExtensions(const TYPE &type) const
{
  auto it = m_addonFileFolderExtensions.find(type);
  if (it != m_addonFileFolderExtensions.end())
    return it->second;

  return "";
}

void CFileExtensionProvider::SetAddonExtensions()
{
  for (auto const type : ADDON_TYPES)
  {
    SetAddonExtensions(type);
  }
}

void CFileExtensionProvider::SetAddonExtensions(const TYPE& type)
{
  std::vector<std::string> extensions;
  std::vector<std::string> fileFolderExtensions;

  if (type == ADDON_AUDIODECODER || type == ADDON_IMAGEDECODER)
  {
    auto addonInfos = CServiceBroker::GetExtsMimeSupportList().GetSupportedAddonInfos(
        CExtsMimeSupportList::FilterSelect::all);
    for (const auto& addonInfo : addonInfos)
    {
      if (addonInfo.m_addonType != type)
        continue;

      for (const auto& ext : addonInfo.m_supportedExtensions)
      {
        extensions.push_back(ext.first);
        if (addonInfo.m_hasTracks)
          fileFolderExtensions.push_back(ext.first);
      }
    }
  }
  else if (type == ADDON_VFS)
  {
    std::vector<AddonInfoPtr> addonInfos;
    m_addonManager.GetAddonInfos(addonInfos, true, type);
    for (const auto& addonInfo : addonInfos)
    {
      std::string ext = addonInfo->Type(type)->GetValue("@extensions").asString();
      if (!ext.empty())
      {
        extensions.push_back(ext);
        if (addonInfo->Type(type)->GetValue("@filedirectories").asBoolean())
          fileFolderExtensions.push_back(ext);

        if (addonInfo->Type(type)->GetValue("@encodedhostname").asBoolean())
        {
          std::string prot = addonInfo->Type(type)->GetValue("@protocols").asString();
          auto prots = StringUtils::Split(prot, "|");
          for (const std::string& it : prots)
            m_encoded.push_back(it);
        }
      }
    }
  }

  m_addonExtensions[type] = StringUtils::Join(extensions, "|");
  m_addonFileFolderExtensions[type] = StringUtils::Join(fileFolderExtensions, "|");
}

void CFileExtensionProvider::OnAddonEvent(const AddonEvent& event)
{
  if (typeid(event) == typeid(AddonEvents::Enabled) ||
      typeid(event) == typeid(AddonEvents::Disabled) ||
      typeid(event) == typeid(AddonEvents::ReInstalled))
  {
    for (auto &type : ADDON_TYPES)
    {
      if (m_addonManager.HasType(event.id, type))
      {
        SetAddonExtensions(type);
        break;
      }
    }
  }
  else if (typeid(event) == typeid(AddonEvents::UnInstalled))
  {
    SetAddonExtensions();
  }
}

bool CFileExtensionProvider::EncodedHostName(const std::string& protocol) const
{
  return std::find(m_encoded.begin(),m_encoded.end(),protocol) != m_encoded.end();
}
