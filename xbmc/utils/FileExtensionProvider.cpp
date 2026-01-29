/*
 *  Copyright (C) 2012-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FileExtensionProvider.h"

#include "ServiceBroker.h"
#include "addons/AddonEvents.h"
#include "addons/AddonManager.h"
#include "addons/AudioDecoder.h"
#include "addons/ExtsMimeSupportList.h"
#include "addons/ImageDecoder.h"
#include "addons/addoninfo/AddonInfo.h"
#include "addons/addoninfo/AddonType.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"

#include <array>
#include <ranges>
#include <string>
#include <string_view>
#include <vector>

using namespace ADDON;
using namespace KODI::ADDONS;

constexpr std::array ADDON_TYPES{AddonType::VFS, AddonType::IMAGEDECODER, AddonType::AUDIODECODER};

CFileExtensionProvider::CFileExtensionProvider(ADDON::CAddonMgr& addonManager)
  : m_advancedSettings(CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()),
    m_addonManager(addonManager)
{
  SetAddonExtensions();

  m_addonManager.Events().Subscribe(this,
                                    [this](const AddonEvent& event)
                                    {
                                      if (typeid(event) == typeid(AddonEvents::Enabled) ||
                                          typeid(event) == typeid(AddonEvents::Disabled) ||
                                          typeid(event) == typeid(AddonEvents::ReInstalled))
                                      {
                                        for (auto& type : ADDON_TYPES)
                                        {
                                          if (m_addonManager.HasType(event.addonId, type))
                                          {
                                            std::lock_guard lock{m_critSection};
                                            SetAddonExtensions(type);
                                            break;
                                          }
                                        }
                                      }
                                      else if (typeid(event) == typeid(AddonEvents::UnInstalled))
                                      {
                                        std::lock_guard lock{m_critSection};
                                        SetAddonExtensions();
                                      }
                                    });

  m_callbackId =
      m_advancedSettings->RegisterSettingsLoadedCallback([this]() { OnAdvancedSettingsLoaded(); });
}

CFileExtensionProvider::~CFileExtensionProvider()
{
  if (m_callbackId.has_value())
    m_advancedSettings->UnregisterSettingsLoadedCallback(m_callbackId.value());

  m_addonManager.Events().Unsubscribe(this);

  m_advancedSettings.reset();
  m_addonExtensions.clear();
}

const std::string& CFileExtensionProvider::GetDiscStubExtensions() const
{
  std::lock_guard lock{m_critSection};

  if (!m_discStubExtensions)
    m_discStubExtensions = m_advancedSettings->m_discStubExtensions;

  return m_discStubExtensions.value();
}

std::string CFileExtensionProvider::GetMusicExtensions() const
{
  std::lock_guard lock{m_critSection};

  if (!m_musicExtensions)
  {
    m_musicExtensions = m_advancedSettings->m_musicExtensions + '|' +
                        GetAddonExtensions(AddonType::VFS) + '|' +
                        GetAddonExtensions(AddonType::AUDIODECODER);
  }
  return m_musicExtensions.value();
}

std::string CFileExtensionProvider::GetPictureExtensions() const
{
  std::lock_guard lock{m_critSection};

  if (!m_pictureExtensions)
  {
    m_pictureExtensions = m_advancedSettings->m_pictureExtensions + '|' +
                          GetAddonExtensions(AddonType::VFS) + '|' +
                          GetAddonExtensions(AddonType::IMAGEDECODER);
  }
  return m_pictureExtensions.value();
}

std::string CFileExtensionProvider::GetSubtitleExtensions() const
{
  std::lock_guard lock{m_critSection};

  if (!m_subtitlesExtensions)
  {
    m_subtitlesExtensions =
        m_advancedSettings->m_subtitlesExtensions + '|' + GetAddonExtensions(AddonType::VFS);
  }
  return m_subtitlesExtensions.value();
}

std::string CFileExtensionProvider::GetVideoExtensions() const
{
  std::lock_guard lock{m_critSection};

  if (!m_videoExtensions)
  {
    std::string extensions(m_advancedSettings->m_videoExtensions);
    if (!extensions.empty())
      extensions += '|';
    extensions += GetAddonExtensions(AddonType::VFS);

    m_videoExtensions = std::move(extensions);
  }
  return m_videoExtensions.value();
}

namespace
{
std::string GetSingleExtensions(const std::string& extensions)
{
  std::string out;
  for (const auto& ext : StringUtils::Split(extensions, "|"))
  {
    if (ext.empty())
      continue;

    if (std::ranges::count(ext, '.') == 1)
      out.append(ext + "|");
  }
  if (!out.empty())
    out.pop_back();
  return out;
}

std::string GetCompoundExtensions(std::string_view extensions)
{
  std::string out;
  for (const auto& ext : StringUtils::Split(extensions, "|"))
  {
    if (ext.empty())
      continue;

    if (std::ranges::count(ext, '.') > 1)
      out.append(ext + "|");
  }
  if (!out.empty())
    out.pop_back();
  return out;
}
} // namespace

std::string CFileExtensionProvider::GetArchiveExtensions() const
{
  std::lock_guard lock{m_critSection};

  if (!m_archiveExtensions)
  {
    std::string extensions(m_advancedSettings->m_archiveExtensions);
    // @todo user customization of extensions list through advancedsettings.xml
    extensions += '|' + GetSingleExtensions(GetAddonExtensions(AddonType::VFS));

    m_archiveExtensions = std::move(extensions);
  }
  return m_archiveExtensions.value();
}

std::string CFileExtensionProvider::GetCompoundArchiveExtensions() const
{
  std::lock_guard lock{m_critSection};

  if (!m_compoundArchiveExtensions)
  {

    std::string extensions(m_advancedSettings->m_compoundArchiveExtensions);
    // @todo user customization of extensions list through advancedsettings.xml
    extensions += '|' + GetCompoundExtensions(GetAddonExtensions(AddonType::VFS));

    m_compoundArchiveExtensions = std::move(extensions);
  }
  return m_compoundArchiveExtensions.value();
}

std::string CFileExtensionProvider::GetFileFolderExtensions() const
{
  std::lock_guard lock{m_critSection};

  if (!m_fileFolderExtensions)
  {
    std::string extensions(GetAddonFileFolderExtensions(AddonType::VFS));
    if (!extensions.empty())
      extensions += '|';
    extensions += GetAddonFileFolderExtensions(AddonType::AUDIODECODER);

    m_fileFolderExtensions = std::move(extensions);
  }
  return m_fileFolderExtensions.value();
}

bool CFileExtensionProvider::CanOperateExtension(const std::string& path)
{
  /*!
   * @todo Improve this function to support all cases and not only audio decoder.
   */

  // Get file extensions to find addon related to it.
  std::string strExtension = URIUtils::GetExtension(path);
  StringUtils::ToLower(strExtension);
  if (!strExtension.empty() && CServiceBroker::IsAddonInterfaceUp())
  {
    std::vector<std::unique_ptr<KODI::ADDONS::IAddonSupportCheck>> supportList;

    auto addonInfos = CServiceBroker::GetExtsMimeSupportList().GetExtensionSupportedAddonInfos(
        strExtension, CExtsMimeSupportList::FilterSelect::all);
    for (const auto& addonInfo : addonInfos)
    {
      switch (addonInfo.first)
      {
        case AddonType::AUDIODECODER:
          supportList.emplace_back(new CAudioDecoder(addonInfo.second));
          break;
        case AddonType::IMAGEDECODER:
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

std::string CFileExtensionProvider::GetAddonExtensions(AddonType type) const
{
  auto it = m_addonExtensions.find(type);
  if (it != m_addonExtensions.end())
    return it->second;

  return "";
}

std::string CFileExtensionProvider::GetAddonFileFolderExtensions(AddonType type) const
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

void CFileExtensionProvider::SetAddonExtensions(AddonType type)
{
  std::vector<std::string> extensions;
  std::vector<std::string> fileFolderExtensions;

  if (type == AddonType::AUDIODECODER || type == AddonType::IMAGEDECODER)
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
    // Invalidate dependent cached extensions lists
    m_musicExtensions.reset();
    m_pictureExtensions.reset();
    m_fileFolderExtensions.reset();
  }
  else if (type == AddonType::VFS)
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
    // Invalidate dependent cached extensions lists
    m_musicExtensions.reset();
    m_pictureExtensions.reset();
    m_subtitlesExtensions.reset();
    m_videoExtensions.reset();
    m_archiveExtensions.reset();
    m_compoundArchiveExtensions.reset();
    m_fileFolderExtensions.reset();
  }

  m_addonExtensions[type] = StringUtils::Join(extensions, "|");
  m_addonFileFolderExtensions[type] = StringUtils::Join(fileFolderExtensions, "|");
}

bool CFileExtensionProvider::EncodedHostName(const std::string& protocol) const
{
  std::lock_guard lock{m_critSection};

  return std::ranges::find(m_encoded, protocol) != m_encoded.end();
}

void CFileExtensionProvider::OnAdvancedSettingsLoaded()
{
  std::lock_guard lock{m_critSection};

  m_discStubExtensions.reset();
  m_musicExtensions.reset();
  m_pictureExtensions.reset();
  m_subtitlesExtensions.reset();
  m_videoExtensions.reset();
  m_archiveExtensions.reset();
  m_compoundArchiveExtensions.reset();
}
