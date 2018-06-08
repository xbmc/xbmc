/*
 *      Copyright (C) 2013 Team XBMC
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "SettingPath.h"
#include "settings/lib/SettingsManager.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/XBMCTinyXML.h"
#include "utils/XMLUtils.h"

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
  CExclusiveLock lock(m_critical);

  if (!CSettingString::Deserialize(node, update))
    return false;

  if (m_control != nullptr &&
     (m_control->GetType() != "button" || (m_control->GetFormat() != "path" && m_control->GetFormat() != "file")))
  {
    CLog::Log(LOGERROR, "CSettingPath: invalid <control> of \"%s\"", m_id.c_str());
    return false;
  }

  auto constraints = node->FirstChild(XML_ELM_CONSTRAINTS);
  if (constraints != nullptr)
  {
    // get writable
    XMLUtils::GetBoolean(constraints, "writable", m_writable);

    // get sources
    auto sources = constraints->FirstChild("sources");
    if (sources != nullptr)
    {
      m_sources.clear();
      auto source = sources->FirstChild("source");
      while (source != nullptr)
      {
        std::string strSource = source->FirstChild()->ValueStr();
        if (!strSource.empty())
          m_sources.push_back(strSource);

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

void CSettingPath::copy(const CSettingPath &setting)
{
  CSettingString::Copy(setting);

  CExclusiveLock lock(m_critical);
  m_writable = setting.m_writable;
  m_sources = setting.m_sources;
  m_hideExtension = setting.m_hideExtension;
  m_masking = setting.m_masking;
}
