/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SettingPath.h"

#include "settings/lib/SettingsManager.h"
#include "utils/FileExtensionProvider.h"
#include "utils/StringUtils.h"
#include "utils/XBMCTinyXML.h"
#include "utils/XMLUtils.h"
#include "utils/log.h"

#include <mutex>

#define XML_ELM_DEFAULT     "default"
#define XML_ELM_CONSTRAINTS "constraints"

CSettingPath::CSettingPath(const std::string &id, CSettingsManager *settingsManager /* = nullptr */)
  : CSettingString(id, settingsManager)
{ }

CSettingPath::CSettingPath(const std::string &id, int label, const std::string &value, CSettingsManager *settingsManager /* = nullptr */)
  : CSettingString(id, label, value, settingsManager)
{ }

CSettingPath::CSettingPath(const std::string &id, const CSettingPath &setting)
  : CSettingString(id, setting)
{
  copy(setting);
}

SettingPtr CSettingPath::Clone(const std::string &id) const
{
  return std::make_shared<CSettingPath>(id, *this);
}

bool CSettingPath::Deserialize(const TiXmlNode *node, bool update /* = false */)
{
  std::unique_lock<CSharedSection> lock(m_critical);

  if (!CSettingString::Deserialize(node, update))
    return false;

  if (m_control != nullptr &&
     (m_control->GetType() != "button" || (m_control->GetFormat() != "path" && m_control->GetFormat() != "file" && m_control->GetFormat() != "image")))
  {
    CLog::Log(LOGERROR, "CSettingPath: invalid <control> of \"{}\"", m_id);
    return false;
  }

  auto constraints = node->FirstChild(XML_ELM_CONSTRAINTS);
  if (constraints != nullptr)
  {
    // get writable
    XMLUtils::GetBoolean(constraints, "writable", m_writable);
    // get hide extensions
    XMLUtils::GetBoolean(constraints, "hideextensions", m_hideExtension);

    // get sources
    auto sources = constraints->FirstChild("sources");
    if (sources != nullptr)
    {
      m_sources.clear();
      auto source = sources->FirstChild("source");
      while (source != nullptr)
      {
        auto child = source->FirstChild();
        if (child != nullptr)
        {
          const std::string& strSource = child->ValueStr();
          if (!strSource.empty())
            m_sources.push_back(strSource);
        }

        source = source->NextSibling("source");
      }
    }

    // get masking
    auto masking = constraints->FirstChild("masking");
    if (masking != nullptr)
      m_masking = masking->FirstChild()->ValueStr();
  }

  return true;
}

bool CSettingPath::SetValue(const std::string &value)
{
  // for backwards compatibility to Frodo
  if (StringUtils::EqualsNoCase(value, "select folder") ||
      StringUtils::EqualsNoCase(value, "select writable folder"))
    return CSettingString::SetValue("");

  return CSettingString::SetValue(value);
}

std::string CSettingPath::GetMasking(const CFileExtensionProvider& fileExtensionProvider) const
{
  if (m_masking.empty())
    return m_masking;

  // setup masking
  auto audioMask = fileExtensionProvider.GetMusicExtensions();
  auto videoMask = fileExtensionProvider.GetVideoExtensions();
  auto imageMask = fileExtensionProvider.GetPictureExtensions();
  auto execMask = "";
#if defined(TARGET_WINDOWS)
  execMask = ".exe|.bat|.cmd|.py";
#endif // defined(TARGET_WINDOWS)

  std::string masking = m_masking;
  if (masking == "video")
    return videoMask;
  if (masking == "audio")
    return audioMask;
  if (masking == "image")
    return imageMask;
  if (masking == "executable")
    return execMask;

  // convert mask qualifiers
  StringUtils::Replace(masking, "$AUDIO", audioMask);
  StringUtils::Replace(masking, "$VIDEO", videoMask);
  StringUtils::Replace(masking, "$IMAGE", imageMask);
  StringUtils::Replace(masking, "$EXECUTABLE", execMask);

  return masking;
}

void CSettingPath::copy(const CSettingPath& setting)
{
  CSettingString::Copy(setting);

  std::unique_lock<CSharedSection> lock(m_critical);
  m_writable = setting.m_writable;
  m_sources = setting.m_sources;
  m_hideExtension = setting.m_hideExtension;
  m_masking = setting.m_masking;
}
