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

#include <sstream>

#include "Setting.h"
#include "SettingsManager.h"
#include "threads/SingleLock.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/XBMCTinyXML.h"
#include "utils/XMLUtils.h"

#define XML_ELM_LEVEL       "level"
#define XML_ELM_DEFAULT     "default"
#define XML_ELM_VALUE       "value"

#define XML_ELM_CONTROL       "control"
#define XML_ELM_CONSTRAINTS   "constraints"
#define XML_ELM_OPTIONS       "options"
#define XML_ELM_OPTION        "option"
#define XML_ELM_MINIMUM       "minimum"
#define XML_ELM_STEP          "step"
#define XML_ELM_MAXIMUM       "maximum"
#define XML_ELM_DEPENDENCIES  "dependencies"
#define XML_ELM_DEPENDENCY    "dependency"
#define XML_ELM_UPDATES       "updates"
#define XML_ELM_UPDATE        "update"

CSetting::CSetting(const std::string &id, CSettingsManager *settingsManager /* = NULL */)
  : ISetting(id, settingsManager),
    m_callback(NULL),
    m_label(-1), m_help(-1),
    m_level(SettingLevelStandard),
    m_changed(false)
{ }
  
CSetting::CSetting(const std::string &id, const CSetting &setting)
  : ISetting(id, setting.m_settingsManager),
    m_callback(NULL),
    m_label(-1), m_help(-1),
    m_level(SettingLevelStandard),
    m_changed(false)
{
  m_id = id;
  Copy(setting);
}

bool CSetting::Deserialize(const TiXmlNode *node, bool update /* = false */)
{
  // handle <visible> conditions
  if (!ISetting::Deserialize(node, update))
    return false;

  const TiXmlElement *element = node->ToElement();
  if (element == NULL)
    return false;

  // get the attributes label and help
  int tmp = -1;
  if (element->QueryIntAttribute(XML_ATTR_LABEL, &tmp) == TIXML_SUCCESS && tmp > 0)
    m_label = tmp;
  
  tmp = -1;
  if (element->QueryIntAttribute(XML_ATTR_HELP, &tmp) == TIXML_SUCCESS && tmp > 0)
    m_help = tmp;
  const char *parentSetting = element->Attribute("parent");
  if (parentSetting != NULL)
    m_parentSetting = parentSetting;

  // get the <level>
  int level = -1;
  if (XMLUtils::GetInt(node, XML_ELM_LEVEL, level))
    m_level = (SettingLevel)level;
    
  if (m_level < (int)SettingLevelBasic || m_level > (int)SettingLevelInternal)
    m_level = SettingLevelStandard;

  const TiXmlNode *dependencies = node->FirstChild(XML_ELM_DEPENDENCIES);
  if (dependencies != NULL)
  {
    const TiXmlNode *dependencyNode = dependencies->FirstChild(XML_ELM_DEPENDENCY);
    while (dependencyNode != NULL)
    {
      CSettingDependency dependency(m_settingsManager);
      if (dependency.Deserialize(dependencyNode))
        m_dependencies.push_back(dependency);
      else
        CLog::Log(LOGWARNING, "CSetting: error reading <dependency> tag of \"%s\"", m_id.c_str());

      dependencyNode = dependencyNode->NextSibling(XML_ELM_DEPENDENCY);
    }
  }

  const TiXmlElement *control = node->FirstChildElement(XML_ELM_CONTROL);
  if (control != NULL)
  {
    if (!m_control.Deserialize(control, update) ||
        m_control.GetType() == SettingControlTypeNone)
    {
      CLog::Log(LOGERROR, "CSetting: error reading <control> tag of \"%s\"", m_id.c_str());
      return false;
    }
  }

  const TiXmlNode *updates = node->FirstChild(XML_ELM_UPDATES);
  if (updates != NULL)
  {
    const TiXmlElement *updateElem = updates->FirstChildElement(XML_ELM_UPDATE);
    while (updateElem != NULL)
    {
      CSettingUpdate update;
      if (update.Deserialize(updateElem))
      {
        if (!m_updates.insert(update).second)
          CLog::Log(LOGWARNING, "CSetting: duplicate <update> definition for \"%s\"", m_id.c_str());
      }
      else
        CLog::Log(LOGWARNING, "CSetting: error reading <update> tag of \"%s\"", m_id.c_str());

      updateElem = updateElem->NextSiblingElement(XML_ELM_UPDATE);
    }
  }

  if ((m_control.GetType() == SettingControlTypeSpinner || m_control.GetType() == SettingControlTypeEdit) &&
        m_control.GetFormat() == SettingControlFormatNone)
  {
    CLog::Log(LOGERROR, "CSetting: invalid <control> tag of \"%s\"", m_id.c_str());
    return false;
  }
    
  return true;
}
  
bool CSetting::IsEnabled() const
{
  bool enabled = true;
  for (SettingDependencies::const_iterator depIt = m_dependencies.begin(); depIt != m_dependencies.end(); ++depIt)
  {
    if (depIt->GetType() != SettingDependencyTypeEnable)
      continue;

    if (!depIt->Check())
    {
      enabled = false;
      break;
    }
  }

  return enabled;
}

bool CSetting::IsVisible() const
{
  if (!ISetting::IsVisible())
    return false;

  bool visible = true;
  for (SettingDependencies::const_iterator depIt = m_dependencies.begin(); depIt != m_dependencies.end(); ++depIt)
  {
    if (depIt->GetType() != SettingDependencyTypeVisible)
      continue;

    if (!depIt->Check())
    {
      visible = false;
      break;
    }
  }

  return visible;
}

bool CSetting::OnSettingChanging(const CSetting *setting)
{
  if (m_callback == NULL)
    return true;
    
  return m_callback->OnSettingChanging(setting);
}
  
void CSetting::OnSettingChanged(const CSetting *setting)
{
  if (m_callback == NULL)
    return;

  m_callback->OnSettingChanged(setting);
}

void CSetting::OnSettingAction(const CSetting *setting)
{
  if (m_callback == NULL)
    return;

  m_callback->OnSettingAction(setting);
}

bool CSetting::OnSettingUpdate(CSetting* &setting, const char *oldSettingId, const TiXmlNode *oldSettingNode)
{
  if (m_callback == NULL)
    return false;

  return m_callback->OnSettingUpdate(setting, oldSettingId, oldSettingNode);
}

void CSetting::OnSettingPropertyChanged(const CSetting *setting, const char *propertyName)
{
  if (m_callback == NULL)
    return;

  m_callback->OnSettingPropertyChanged(setting, propertyName);
}

void CSetting::Copy(const CSetting &setting)
{
  SetVisible(setting.IsVisible());
  SetRequirementsMet(setting.MeetsRequirements());
  m_callback = setting.m_callback;
  m_label = setting.m_label;
  m_help = setting.m_help;
  m_level = setting.m_level;
  m_control = setting.m_control;
  m_dependencies = setting.m_dependencies;
  m_updates = setting.m_updates;
  m_changed = setting.m_changed;
}
  
CSettingBool::CSettingBool(const std::string &id, CSettingsManager *settingsManager /* = NULL */)
  : CSetting(id, settingsManager),
    m_value(false), m_default(false)
{
  m_control.SetType(SettingControlTypeCheckmark);
  m_control.SetFormat(SettingControlFormatBoolean);
  m_control.SetAttributes(SettingControlAttributeNone);
}
  
CSettingBool::CSettingBool(const std::string &id, const CSettingBool &setting)
  : CSetting(id, setting)
{
  copy(setting);
}

CSettingBool::CSettingBool(const std::string &id, int label, bool value, CSettingsManager *settingsManager /* = NULL */)
  : CSetting(id, settingsManager),
    m_value(value), m_default(value)
{
  m_label = label;

  m_control.SetType(SettingControlTypeCheckmark);
  m_control.SetFormat(SettingControlFormatBoolean);
  m_control.SetAttributes(SettingControlAttributeNone);
}

bool CSettingBool::Deserialize(const TiXmlNode *node, bool update /* = false */)
{
  CSingleLock lock(m_critical);

  if (!CSetting::Deserialize(node, update))
    return false;

  if (m_control.GetType() != SettingControlTypeCheckmark ||
      m_control.GetFormat() != SettingControlFormatBoolean ||
      m_control.GetAttributes() != SettingControlAttributeNone)
  {
    CLog::Log(LOGERROR, "CSettingBool: invalid <control> of \"%s\"", m_id.c_str());
    return false;
  }
    
  // get the default value
  bool value;
  if (XMLUtils::GetBoolean(node, XML_ELM_DEFAULT, value))
    m_value = m_default = value;
  else if (!update)
  {
    CLog::Log(LOGERROR, "CSettingBool: error reading the default value of \"%s\"", m_id.c_str());
    return false;
  }

  return true;
}
  
bool CSettingBool::FromString(const std::string &value)
{
  bool bValue;
  if (!fromString(value, bValue))
    return false;

  return SetValue(bValue);
}

std::string CSettingBool::ToString() const
{
  return m_value ? "true" : "false";
}

bool CSettingBool::Equals(const std::string &value) const
{
  bool bValue;
  return (fromString(value, bValue) && m_value == bValue);
}

bool CSettingBool::CheckValidity(const std::string &value) const
{
  bool bValue;
  return fromString(value, bValue);
}

bool CSettingBool::SetValue(bool value)
{
  CSingleLock lock(m_critical);

  if (value == m_value)
    return true;

  bool oldValue = m_value;
  m_value = value;

  if (!OnSettingChanging(this))
  {
    m_value = oldValue;

    // the setting couldn't be changed because one of the
    // callback handlers failed the OnSettingChanging()
    // callback so we need to let all the callback handlers
    // know that the setting hasn't changed
    OnSettingChanging(this);
    return false;
  }

  m_changed = m_value != m_default;
  OnSettingChanged(this);
  return true;
}
  
void CSettingBool::SetDefault(bool value)
{
  CSingleLock lock(m_critical);

  m_default = value;
  if (!m_changed)
    m_value = m_default;
}

void CSettingBool::copy(const CSettingBool &setting)
{
  CSetting::Copy(setting);

  m_value = setting.m_value;
  m_default = setting.m_default;
}
  
bool CSettingBool::fromString(const std::string &strValue, bool &value) const
{
  CSingleLock lock(m_critical);

  if (StringUtils::EqualsNoCase(strValue, "true"))
  {
    value = true;
    return true;
  }
  if (StringUtils::EqualsNoCase(strValue, "false"))
  {
    value = false;
    return true;
  }

  return false;
}

CSettingInt::CSettingInt(const std::string &id, CSettingsManager *settingsManager /* = NULL */)
  : CSetting(id, settingsManager),
    m_value(0), m_default(0),
    m_min(0), m_step(1), m_max(0),
    m_format(-1), m_labelMin(-1), m_strFormat("%i")
{ }
  
CSettingInt::CSettingInt(const std::string &id, const CSettingInt &setting)
  : CSetting(id, setting)
{
  copy(setting);
}

CSettingInt::CSettingInt(const std::string &id, int label, int value, int minimum, int step, int maximum, int format, int minimumLabel, CSettingsManager *settingsManager /* = NULL */)
  : CSetting(id, settingsManager),
    m_value(value), m_default(value),
    m_min(minimum), m_step(step), m_max(maximum),
    m_format(format), m_labelMin(minimumLabel), m_strFormat("%i")
{
  m_label = label;

  m_control.SetType(SettingControlTypeSpinner);
  if (format < 0)
    m_control.SetFormat(SettingControlFormatInteger);
  else
    m_control.SetFormat(SettingControlFormatString);
  m_control.SetAttributes(SettingControlAttributeNone);
}

CSettingInt::CSettingInt(const std::string &id, int label, int value, int minimum, int step, int maximum, const std::string &format, CSettingsManager *settingsManager /* = NULL */)
  : CSetting(id, settingsManager),
    m_value(value), m_default(value),
    m_min(minimum), m_step(step), m_max(maximum),
    m_format(-1), m_labelMin(-1), m_strFormat(format)
{
  m_label = label;

  m_control.SetType(SettingControlTypeSpinner);
  if (format.empty())
  {
      m_strFormat = "%i";
      m_control.SetFormat(SettingControlFormatInteger);
  }
  else
    m_control.SetFormat(SettingControlFormatString);
  m_control.SetAttributes(SettingControlAttributeNone);
}

CSettingInt::CSettingInt(const std::string &id, int label, int value, const StaticIntegerSettingOptions &options, CSettingsManager *settingsManager /* = NULL */)
  : CSetting(id, settingsManager),
    m_value(value), m_default(value),
    m_min(0), m_step(1), m_max(0),
    m_format(-1), m_labelMin(-1), m_strFormat("%i"),
    m_options(options)
{
  m_label = label;

  m_control.SetType(SettingControlTypeSpinner);
  m_control.SetFormat(SettingControlFormatString);
  m_control.SetAttributes(SettingControlAttributeNone);
}

bool CSettingInt::Deserialize(const TiXmlNode *node, bool update /* = false */)
{
  CSingleLock lock(m_critical);

  if (!CSetting::Deserialize(node, update))
    return false;

  if (m_control.GetType() == SettingControlTypeCheckmark)
  {
    CLog::Log(LOGERROR, "CSettingInt: invalid <control> of \"%s\"", m_id.c_str());
    return false;
  }

  // get the default value
  int value;
  if (XMLUtils::GetInt(node, XML_ELM_DEFAULT, value))
    m_value = m_default = value;
  else if (!update)
  {
    CLog::Log(LOGERROR, "CSettingInt: error reading the default value of \"%s\"", m_id.c_str());
    return false;
  }

  if (m_control.GetFormat() == SettingControlFormatString)
  {
    const TiXmlNode *control = node->FirstChild(XML_ELM_CONTROL);
    if (control != NULL)
      XMLUtils::GetInt(control, "formatlabel", m_format);
  }

  const TiXmlNode *constraints = node->FirstChild(XML_ELM_CONSTRAINTS);
  if (constraints != NULL)
  {
    // get the entries
    const TiXmlNode *options = constraints->FirstChild(XML_ELM_OPTIONS);
    if (options != NULL && options->FirstChild() != NULL)
    {
      if (options->FirstChild()->Type() == TiXmlNode::TINYXML_TEXT)
        m_optionsFiller = options->FirstChild()->ValueStr();
      else
      {
        m_options.clear();
        const TiXmlElement *optionElement = options->FirstChildElement(XML_ELM_OPTION);
        while (optionElement != NULL)
        {
          std::pair<int, int> entry;
          if (optionElement->QueryIntAttribute("label", &entry.first) == TIXML_SUCCESS && entry.first > 0)
          {
            entry.second = strtol(optionElement->FirstChild()->Value(), NULL, 10);
            m_options.push_back(entry);
          }

          optionElement = optionElement->NextSiblingElement(XML_ELM_OPTION);
        }
      }
    }

    // get minimum
    if (XMLUtils::GetInt(constraints, XML_ELM_MINIMUM, m_min) &&
        m_control.GetFormat() == SettingControlFormatString)
    {
      const TiXmlElement *minimumElement = constraints->FirstChildElement(XML_ELM_MINIMUM);
      if (minimumElement->QueryIntAttribute(XML_ATTR_LABEL, &m_labelMin) != TIXML_SUCCESS)
        m_labelMin = -1;
    }
    // get step
    XMLUtils::GetInt(constraints, XML_ELM_STEP, m_step);
    // get maximum
    XMLUtils::GetInt(constraints, XML_ELM_MAXIMUM, m_max);

    if (m_control.GetFormat() == SettingControlFormatString && m_labelMin < 0)
    {
      CStdString strFormat;
      if (XMLUtils::GetString(constraints, "format", strFormat) && !strFormat.empty())
        m_strFormat = strFormat;
    }
  }

  return true;
}

bool CSettingInt::FromString(const std::string &value)
{
  int iValue;
  if (!fromString(value, iValue))
    return false;

  return SetValue(iValue);
}

std::string CSettingInt::ToString() const
{
  std::ostringstream oss;
  oss << m_value;

  return oss.str();
}

bool CSettingInt::Equals(const std::string &value) const
{
  int iValue;
  return (fromString(value, iValue) && m_value == iValue);
}

bool CSettingInt::CheckValidity(const std::string &value) const
{
  int iValue;
  if (!fromString(value, iValue))
    return false;

  return CheckValidity(iValue);
}

bool CSettingInt::CheckValidity(int value) const
{
  if (!m_options.empty())
  {
    //if the setting is an std::map, check if we got a valid value before assigning it
    bool ok = false;
    for (StaticIntegerSettingOptions::const_iterator it = m_options.begin(); it != m_options.end(); ++it)
    {
      if (it->second == value)
      {
        ok = true;
        break;
      }
    }

    if (!ok)
      return false;
  }
  else if (m_optionsFiller.empty() && m_min != m_max &&
          (value < m_min || value > m_max))
    return false;

  return true;
}

bool CSettingInt::SetValue(int value)
{
  CSingleLock lock(m_critical);

  if (value == m_value)
    return true;

  if (!CheckValidity(value))
    return false;

  int oldValue = m_value;
  m_value = value;

  if (!OnSettingChanging(this))
  {
    m_value = oldValue;

    // the setting couldn't be changed because one of the
    // callback handlers failed the OnSettingChanging()
    // callback so we need to let all the callback handlers
    // know that the setting hasn't changed
    OnSettingChanging(this);
    return false;
  }

  m_changed = m_value != m_default;
  OnSettingChanged(this);
  return true;
}

void CSettingInt::SetDefault(int value)
{
  CSingleLock lock(m_critical);

  m_default = value;
  if (!m_changed)
    m_value = m_default;
}

SettingOptionsType CSettingInt::GetOptionsType() const
{
  if (!m_options.empty())
    return SettingOptionsTypeStatic;
  if (!m_optionsFiller.empty())
    return SettingOptionsTypeDynamic;

  return SettingOptionsTypeNone;
}

DynamicIntegerSettingOptions CSettingInt::UpdateDynamicOptions()
{
  DynamicIntegerSettingOptions options;
  if (m_optionsFiller.empty() || m_settingsManager == NULL)
    return options;

  IntegerSettingOptionsFiller filler = (IntegerSettingOptionsFiller)m_settingsManager->GetSettingOptionsFiller(this);
  if (filler == NULL)
    return options;

  int bestMatchingValue = m_value;
  filler(this, options, bestMatchingValue);

  if (bestMatchingValue != m_value)
    SetValue(bestMatchingValue);

  bool changed = m_dynamicOptions.size() != options.size();
  if (!changed)
  {
    for (size_t index = 0; index < options.size(); index++)
    {
      if (options[index].first.compare(m_dynamicOptions[index].first) != 0 ||
          options[index].second != m_dynamicOptions[index].second)
      {
        changed = true;
        break;
      }
    }
  }

  if (changed)
  {
    m_dynamicOptions = options;
    OnSettingPropertyChanged(this, "options");
  }

  return options;
}

void CSettingInt::copy(const CSettingInt &setting)
{
  CSetting::Copy(setting);

  m_value = setting.m_value;
  m_default = setting.m_default;
  m_min = setting.m_min;
  m_step = setting.m_step;
  m_max = setting.m_max;
  m_format = setting.m_format;
  m_labelMin = setting.m_labelMin;
  m_strFormat = setting.m_strFormat;
  m_options = setting.m_options;
  m_optionsFiller = setting.m_optionsFiller;
}

bool CSettingInt::fromString(const std::string &strValue, int &value)
{
  if (strValue.empty())
    return false;

  char *end = NULL;
  value = (int)strtol(strValue.c_str(), &end, 10);
  if (end != NULL && *end != '\0')
    return false; 

  return true;
}

CSettingNumber::CSettingNumber(const std::string &id, CSettingsManager *settingsManager /* = NULL */)
  : CSetting(id, settingsManager),
    m_value(0.0), m_default(0.0),
    m_min(0.0), m_step(1.0), m_max(0.0)
{ }
  
CSettingNumber::CSettingNumber(const std::string &id, const CSettingNumber &setting)
  : CSetting(id, setting)
{
  copy(setting);
}

CSettingNumber::CSettingNumber(const std::string &id, int label, float value, float minimum, float step, float maximum, CSettingsManager *settingsManager /* = NULL */)
  : CSetting(id, settingsManager),
    m_value(value), m_default(value),
    m_min(minimum), m_step(step), m_max(maximum)
{
  m_label = label;

  m_control.SetType(SettingControlTypeSpinner);
  m_control.SetFormat(SettingControlFormatNumber);
  m_control.SetAttributes(SettingControlAttributeNone);
}

bool CSettingNumber::Deserialize(const TiXmlNode *node, bool update /* = false */)
{
  CSingleLock lock(m_critical);

  if (!CSetting::Deserialize(node, update))
    return false;
    
  if (m_control.GetType() == SettingControlTypeCheckmark ||
      m_control.GetType() == SettingControlTypeList)
  {
    CLog::Log(LOGERROR, "CSettingInt: invalid <control> of \"%s\"", m_id.c_str());
    return false;
  }
    
  // get the default value
  double value;
  if (XMLUtils::GetDouble(node, XML_ELM_DEFAULT, value))
    m_value = m_default = value;
  else if (!update)
  {
    CLog::Log(LOGERROR, "CSettingNumber: error reading the default value of \"%s\"", m_id.c_str());
    return false;
  }
    
  const TiXmlNode *constraints = node->FirstChild(XML_ELM_CONSTRAINTS);
  if (constraints != NULL)
  {
    // get the minimum value
    XMLUtils::GetDouble(constraints, XML_ELM_MINIMUM, m_min);
    // get the step value
    XMLUtils::GetDouble(constraints, XML_ELM_STEP, m_step);
    // get the maximum value
    XMLUtils::GetDouble(constraints, XML_ELM_MAXIMUM, m_max);
  }

  return true;
}

bool CSettingNumber::FromString(const std::string &value)
{
  double dValue;
  if (!fromString(value, dValue))
    return false;

  return SetValue(dValue);
}

std::string CSettingNumber::ToString() const
{
  std::ostringstream oss;
  oss << m_value;

  return oss.str();
}

bool CSettingNumber::Equals(const std::string &value) const
{
  double dValue;
  return (fromString(value, dValue) && m_value == dValue);
}

bool CSettingNumber::CheckValidity(const std::string &value) const
{
  double dValue;
  if (!fromString(value, dValue))
    return false;

  return CheckValidity(dValue);
}

bool CSettingNumber::CheckValidity(double value) const
{
  if (m_min != m_max &&
     (value < m_min || value > m_max))
    return false;

  return true;
}

bool CSettingNumber::SetValue(double value)
{
  CSingleLock lock(m_critical);

  if (value == m_value)
    return true;

  if (!CheckValidity(value))
    return false;

  double oldValue = m_value;
  m_value = value;

  if (!OnSettingChanging(this))
  {
    m_value = oldValue;

    // the setting couldn't be changed because one of the
    // callback handlers failed the OnSettingChanging()
    // callback so we need to let all the callback handlers
    // know that the setting hasn't changed
    OnSettingChanging(this);
    return false;
  }

  m_changed = m_value != m_default;
  OnSettingChanged(this);
  return true;
}

void CSettingNumber::SetDefault(double value)
{
  CSingleLock lock(m_critical);

  m_default = value;
  if (!m_changed)
    m_value = m_default;
}

void CSettingNumber::copy(const CSettingNumber &setting)
{
  CSetting::Copy(setting);

  m_value = setting.m_value;
  m_default = setting.m_default;
  m_min = setting.m_min;
  m_step = setting.m_step;
  m_max = setting.m_max;
}

bool CSettingNumber::fromString(const std::string &strValue, double &value)
{
  if (strValue.empty())
    return false;

  char *end = NULL;
  value = (int)strtod(strValue.c_str(), &end);
  if (end != NULL && *end != '\0')
    return false;

  return true;
}

CSettingString::CSettingString(const std::string &id, CSettingsManager *settingsManager /* = NULL */)
  : CSetting(id, settingsManager),
    m_allowEmpty(false), m_heading(-1)
{ }
  
CSettingString::CSettingString(const std::string &id, const CSettingString &setting)
  : CSetting(id, setting)
{
  copy(setting);
}

CSettingString::CSettingString(const std::string &id, int label, const std::string &value, CSettingsManager *settingsManager /* = NULL */)
  : CSetting(id, settingsManager),
    m_value(value), m_default(value),
    m_allowEmpty(false), m_heading(-1)
{
  m_label = label;

  m_control.SetType(SettingControlTypeEdit);
  m_control.SetFormat(SettingControlFormatString);
  m_control.SetAttributes(SettingControlAttributeNone);
}

bool CSettingString::Deserialize(const TiXmlNode *node, bool update /* = false */)
{
  CSingleLock lock(m_critical);

  if (!CSetting::Deserialize(node, update))
    return false;

  if (m_control.GetType() == SettingControlTypeCheckmark ||
      (m_control.GetType() == SettingControlTypeSpinner && (m_control.GetFormat() == SettingControlFormatInteger || m_control.GetFormat() == SettingControlFormatNumber)) ||
      (m_control.GetType() == SettingControlTypeEdit && m_control.GetFormat() == SettingControlFormatNumber))
  {
    CLog::Log(LOGERROR, "CSettingString: invalid <control> of \"%s\"", m_id.c_str());
    return false;
  }

  const TiXmlNode *control = node->FirstChild(XML_ELM_CONTROL);
  if (control != NULL)
  {
    // get heading
    XMLUtils::GetInt(control, "heading", m_heading);
  }

  const TiXmlNode *constraints = node->FirstChild(XML_ELM_CONSTRAINTS);
  if (constraints != NULL)
  {
    // get allowempty (needs to be parsed before parsing the default value)
    XMLUtils::GetBoolean(constraints, "allowempty", m_allowEmpty);

    // get the entries
    const TiXmlNode *options = constraints->FirstChild(XML_ELM_OPTIONS);
    if (options != NULL && options->FirstChild() != NULL &&
        options->FirstChild()->Type() == TiXmlNode::TINYXML_TEXT)
      m_optionsFiller = options->FirstChild()->ValueStr();
  }

  // get the default value
  CStdString value;
  if (XMLUtils::GetString(node, XML_ELM_DEFAULT, value) && !value.empty())
    m_value = m_default = value;
  else if (!update && !m_allowEmpty)
  {
    CLog::Log(LOGERROR, "CSettingString: error reading the default value of \"%s\"", m_id.c_str());
    return false;
  }

  return true;
}

bool CSettingString::CheckValidity(const std::string &value) const
{
  if (!m_allowEmpty && value.empty())
    return false;

  return true;
}

bool CSettingString::SetValue(const std::string &value)
{
  CSingleLock lock(m_critical);

  if (value == m_value)
    return true;
    
  if (!CheckValidity(value))
    return false;

  std::string oldValue = m_value;
  m_value = value;

  if (!OnSettingChanging(this))
  {
    m_value = oldValue;

    // the setting couldn't be changed because one of the
    // callback handlers failed the OnSettingChanging()
    // callback so we need to let all the callback handlers
    // know that the setting hasn't changed
    OnSettingChanging(this);
    return false;
  }

  m_changed = m_value != m_default;
  OnSettingChanged(this);
  return true;
}

void CSettingString::SetDefault(const std::string &value)
{
  CSingleLock lock(m_critical);

  m_default = value;
  if (!m_changed)
    m_value = m_default;
}

SettingOptionsType CSettingString::GetOptionsType() const
{
  if (!m_optionsFiller.empty())
    return SettingOptionsTypeDynamic;

  return SettingOptionsTypeNone;
}

DynamicStringSettingOptions CSettingString::UpdateDynamicOptions()
{
  DynamicStringSettingOptions options;
  if (m_optionsFiller.empty() || m_settingsManager == NULL)
    return options;

  StringSettingOptionsFiller filler = (StringSettingOptionsFiller)m_settingsManager->GetSettingOptionsFiller(this);
  if (filler == NULL)
    return options;

  std::string bestMatchingValue = m_value;
  filler(this, options, bestMatchingValue);

  // check if the list of items has changed
  bool changed = m_dynamicOptions.size() != options.size();
  if (!changed)
  {
    for (size_t index = 0; index < options.size(); index++)
    {
      if (options[index].first.compare(m_dynamicOptions[index].first) != 0 ||
          options[index].second.compare(m_dynamicOptions[index].second) != 0)
      {
        changed = true;
        break;
      }
    }
  }

  if (changed)
  {
    m_dynamicOptions = options;
    OnSettingPropertyChanged(this, "options");
  }

  return options;
}

void CSettingString::copy(const CSettingString &setting)
{
  CSetting::Copy(setting);

  m_value = setting.m_value;
  m_default = setting.m_default;
  m_allowEmpty = setting.m_allowEmpty;
  m_heading = setting.m_heading;
}
  
CSettingAction::CSettingAction(const std::string &id, CSettingsManager *settingsManager /* = NULL */)
  : CSetting(id, settingsManager)
{
  m_control.SetType(SettingControlTypeButton);
  m_control.SetFormat(SettingControlFormatAction);
  m_control.SetAttributes(SettingControlAttributeNone);
}
  
CSettingAction::CSettingAction(const std::string &id, const CSettingAction &setting)
  : CSetting(id, setting)
{
  m_control.SetType(SettingControlTypeButton);
  m_control.SetFormat(SettingControlFormatAction);
  m_control.SetAttributes(SettingControlAttributeNone);
}

bool CSettingAction::Deserialize(const TiXmlNode *node, bool update /* = false */)
{
  CSingleLock lock(m_critical);

  if (!CSetting::Deserialize(node, update))
    return false;
    
  if (m_control.GetType() != SettingControlTypeButton ||
      m_control.GetFormat() != SettingControlFormatAction ||
      m_control.GetAttributes() != SettingControlAttributeNone)
  {
    CLog::Log(LOGERROR, "CSettingAction: invalid <control> of \"%s\"", m_id.c_str());
    return false;
  }
    
  return true;
}
