/*
 *      Copyright (C) 2013 Team XBMC
 *      http://xbmc.org
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

CSettingPath::CSettingPath(const std::string &id, CSettingsManager *settingsManager /* = NULL */)
  : CSettingString(id, settingsManager),
    m_writable(true)
{ }

CSettingPath::CSettingPath(const std::string &id, int label, const std::string &value, CSettingsManager *settingsManager /* = NULL */)
  : CSettingString(id, label, value, settingsManager),
    m_writable(true)
{ }
  
CSettingPath::CSettingPath(const std::string &id, const CSettingPath &setting)
  : CSettingString(id, setting)
{
  copy(setting);
}

CSetting* CSettingPath::Clone(const std::string &id) const
{
  return new CSettingPath(id, *this);
}

bool CSettingPath::Deserialize(const TiXmlNode *node, bool update /* = false */)
{
  CExclusiveLock lock(m_critical);

  if (!CSettingString::Deserialize(node, update))
    return false;
    
  if (m_control != NULL &&
     (m_control->GetType() != "button" || m_control->GetFormat() != "path"))
  {
    CLog::Log(LOGERROR, "CSettingPath: invalid <control> of \"%s\"", m_id.c_str());
    return false;
  }
    
  const TiXmlNode *constraints = node->FirstChild(XML_ELM_CONSTRAINTS);
  if (constraints != NULL)
  {
    // get writable
    XMLUtils::GetBoolean(constraints, "writable", m_writable);

    // get sources
    const TiXmlNode *sources = constraints->FirstChild("sources");
    if (sources != NULL)
    {
      m_sources.clear();
      const TiXmlNode *source = sources->FirstChild("source");
      while (source != NULL)
      {
        std::string strSource = source->FirstChild()->ValueStr();
        if (!strSource.empty())
          m_sources.push_back(strSource);

        source = source->NextSibling("source");
      }
    }
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
}
