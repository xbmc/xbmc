/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FileExtensionProvider.h"

#include <string>
#include <vector>

#include "ServiceBroker.h"
#include "addons/AddonManager.h"
#include "addons/binary-addons/BinaryAddonBase.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"

using namespace ADDON;

const std::vector<TYPE> ADDON_TYPES = {
  ADDON_VFS,
  ADDON_IMAGEDECODER,
  ADDON_AUDIODECODER
};

CFileExtensionProvider::CFileExtensionProvider(ADDON::CAddonMgr &addonManager,
                                               ADDON::CBinaryAddonManager &binaryAddonManager) :
  m_advancedSettings(CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()),
  m_addonManager(addonManager),
  m_binaryAddonManager(binaryAddonManager)
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

std::string CFileExtensionProvider::GetAddonExtensions(const TYPE &type) const
{
  auto it = m_addonExtensions.find(type);
  if (it != m_addonExtensions.end())
    return it->second;

  return "";
}

std::string CFileExtensionProvider::GetAddonFileFolderExtensions(const TYPE &type) const
{
  auto it = m_addonExtensions.find(type);
  if (it != m_addonExtensions.end())
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
  BinaryAddonBaseList addonInfos;
  m_binaryAddonManager.GetAddonInfos(addonInfos, true, type);
  for (const auto& addonInfo : addonInfos)
  {
    std::string info = ADDON_VFS == type ? "@extensions" : "@extension";
    std::string ext = addonInfo->Type(type)->GetValue(info).asString();
    if (!ext.empty())
    {
      extensions.push_back(ext);
      if (type == ADDON_VFS || type == ADDON_AUDIODECODER)
      {
        std::string info2 = ADDON_VFS == type ? "@filedirectories" : "@tracks";
        if (addonInfo->Type(type)->GetValue(info2).asBoolean())
          fileFolderExtensions.push_back(ext);
      }
      if (type == ADDON_VFS)
      {
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

  m_addonExtensions.insert(make_pair(type, StringUtils::Join(extensions, "|")));
  if (!fileFolderExtensions.empty())
    m_addonFileFolderExtensions.insert(make_pair(type, StringUtils::Join(fileFolderExtensions, "|")));
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
