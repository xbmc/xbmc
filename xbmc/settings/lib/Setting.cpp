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
#include "SettingDefinitions.h"
#include "SettingsManager.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/XBMCTinyXML.h"
#include "utils/XMLUtils.h"

template<typename TKey, typename TValue>
bool CheckSettingOptionsValidity(const TValue& value, const std::vector<std::pair<TKey, TValue>>& options)
{
  for (auto it : options)
  {
    if (it.second == value)
      return true;
  }

  return false;
}

CSetting::CSetting(const std::string &id, CSettingsManager *settingsManager /* = nullptr */)
  : ISetting(id, settingsManager)
{ }
  
CSetting::CSetting(const std::string &id, const CSetting &setting)
  : ISetting(id, setting.m_settingsManager)
{
  m_id = id;
  Copy(setting);
}

bool CSetting::Deserialize(const TiXmlNode *node, bool update /* = false */)
{
  // handle <visible> conditions
  if (!ISetting::Deserialize(node, update))
    return false;

  auto element = node->ToElement();
  if (element == nullptr)
    return false;

  auto parentSetting = element->Attribute(SETTING_XML_ATTR_PARENT);
  if (parentSetting != nullptr)
    m_parentSetting = parentSetting;

  // get the <level>
  int level = -1;
  if (XMLUtils::GetInt(node, SETTING_XML_ELM_LEVEL, level))
    m_level = static_cast<SettingLevel>(level);
    
  if (m_level < SettingLevel::Basic || m_level > SettingLevel::Internal)
    m_level = SettingLevel::Standard;

  auto dependencies = node->FirstChild(SETTING_XML_ELM_DEPENDENCIES);
  if (dependencies != nullptr)
  {
    auto dependencyNode = dependencies->FirstChild(SETTING_XML_ELM_DEPENDENCY);
    while (dependencyNode != nullptr)
    {
      CSettingDependency dependency(m_settingsManager);
      if (dependency.Deserialize(dependencyNode))
        m_dependencies.push_back(dependency);
      else
        CLog::Log(LOGWARNING, "CSetting: error reading <dependency> tag of \"%s\"", m_id.c_str());

      dependencyNode = dependencyNode->NextSibling(SETTING_XML_ELM_DEPENDENCY);
    }
  }

  auto control = node->FirstChildElement(SETTING_XML_ELM_CONTROL);
  if (control != nullptr)
  {
    auto controlType = control->Attribute(SETTING_XML_ATTR_TYPE);
    if (controlType == nullptr)
    {
      CLog::Log(LOGERROR, "CSetting: error reading \"type\" attribute of <control> tag of \"%s\"", m_id.c_str());
      return false;
    }

    m_control = m_settingsManager->CreateControl(controlType);
    if (m_control == nullptr || !m_control->Deserialize(control, update))
    {
      CLog::Log(LOGERROR, "CSetting: error reading <control> tag of \"%s\"", m_id.c_str());
      return false;
    }
  }
  else if (!update && m_level < SettingLevel::Internal && GetType() != SettingType::Reference)
  {
    CLog::Log(LOGERROR, "CSetting: missing <control> tag of \"%s\"", m_id.c_str());
    return false;
  }

  auto updates = node->FirstChild(SETTING_XML_ELM_UPDATES);
  if (updates != nullptr)
  {
    auto updateElem = updates->FirstChildElement(SETTING_XML_ELM_UPDATE);
    while (updateElem != nullptr)
    {
      CSettingUpdate settingUpdate;
      if (settingUpdate.Deserialize(updateElem))
      {
        if (!m_updates.insert(settingUpdate).second)
          CLog::Log(LOGWARNING, "CSetting: duplicate <update> definition for \"%s\"", m_id.c_str());
      }
      else
        CLog::Log(LOGWARNING, "CSetting: error reading <update> tag of \"%s\"", m_id.c_str());

      updateElem = updateElem->NextSiblingElement(SETTING_XML_ELM_UPDATE);
    }
  }
    
  return true;
}
  
bool CSetting::IsEnabled() const
{
  if (m_dependencies.empty() && m_parentSetting.empty())
    return m_enabled;

  // if the setting has a parent setting and that parent setting is disabled
  // the setting should automatically also be disabled
  if (!m_parentSetting.empty())
  {
    SettingPtr parentSetting = m_settingsManager->GetSetting(m_parentSetting);
    if (parentSetting != nullptr && !parentSetting->IsEnabled())
      return false;
  }

  bool enabled = true;
  for (auto dep : m_dependencies)
  {
    if (dep.GetType() != SettingDependencyType::Enable)
      continue;

    if (!dep.Check())
    {
      enabled = false;
      break;
    }
  }

  return enabled;
}

void CSetting::SetEnabled(bool enabled)
{
  if (!m_dependencies.empty() || m_enabled == enabled)
    return;

  m_enabled = enabled;
  OnSettingPropertyChanged(shared_from_this(), "enabled");
}

bool CSetting::IsVisible() const
{
  if (!ISetting::IsVisible())
    return false;

  bool visible = true;
  for (auto dep : m_dependencies)
  {
    if (dep.GetType() != SettingDependencyType::Visible)
      continue;

    if (!dep.Check())
    {
      visible = false;
      break;
    }
  }

  return visible;
}

bool CSetting::OnSettingChanging(std::shared_ptr<const CSetting> setting)
{
  if (m_callback == nullptr)
    return true;
    
  return m_callback->OnSettingChanging(setting);
}
  
void CSetting::OnSettingChanged(std::shared_ptr<const CSetting> setting)
{
  if (m_callback == nullptr)
    return;

  m_callback->OnSettingChanged(setting);
}

void CSetting::OnSettingAction(std::shared_ptr<const CSetting> setting)
{
  if (m_callback == nullptr)
    return;

  m_callback->OnSettingAction(setting);
}

bool CSetting::OnSettingUpdate(std::shared_ptr<CSetting> setting, const char *oldSettingId, const TiXmlNode *oldSettingNode)
{
  if (m_callback == nullptr)
    return false;

  return m_callback->OnSettingUpdate(setting, oldSettingId, oldSettingNode);
}

void CSetting::OnSettingPropertyChanged(std::shared_ptr<const CSetting> setting, const char *propertyName)
{
  if (m_callback == nullptr)
    return;

  m_callback->OnSettingPropertyChanged(setting, propertyName);
}

void CSetting::Copy(const CSetting &setting)
{
  SetVisible(setting.IsVisible());
  SetRequirementsMet(setting.MeetsRequirements());
  m_callback = setting.m_callback;
  m_level = setting.m_level;
 
  if (setting.m_control != nullptr)
  {
    m_control = m_settingsManager->CreateControl(setting.m_control->GetType());
    *m_control = *setting.m_control;
  }
  else
    m_control = nullptr;

  m_dependencies = setting.m_dependencies;
  m_updates = setting.m_updates;
  m_changed = setting.m_changed;
}

CSettingReference::CSettingReference(const std::string &id, CSettingsManager *settingsManager /* = nullptr */)
  : CSetting("#" + id, settingsManager)
  , m_referencedId(id)
{ }

CSettingReference::CSettingReference(const std::string &id, const CSettingReference &setting)
  : CSetting("#" + id, setting)
  , m_referencedId(id)
{ }

std::shared_ptr<CSetting> CSettingReference::Clone(const std::string &id) const
{
  return std::make_shared<CSettingReference>(id, *this);
}

CSettingList::CSettingList(const std::string &id, std::shared_ptr<CSetting> settingDefinition, CSettingsManager *settingsManager /* = nullptr */)
  : CSetting(id, settingsManager)
  , m_definition(settingDefinition)
{ }

CSettingList::CSettingList(const std::string &id, std::shared_ptr<CSetting> settingDefinition, int label, CSettingsManager *settingsManager /* = nullptr */)
  : CSetting(id, settingsManager)
  , m_definition(settingDefinition)
{
  SetLabel(label);
}

CSettingList::CSettingList(const std::string &id, const CSettingList &setting)
  : CSetting(id, setting)
{
  copy(setting);
}

SettingPtr CSettingList::Clone(const std::string &id) const
{
  if (m_definition == nullptr)
    return nullptr;

  return std::make_shared<CSettingList>(id, *this);
}

bool CSettingList::Deserialize(const TiXmlNode *node, bool update /* = false */)
{
  CExclusiveLock lock(m_critical);

  if (m_definition == nullptr)
    return false;

  if (!CSetting::Deserialize(node, update))
    return false;

  auto element = node->ToElement();
  if (element == nullptr)
  {
    CLog::Log(LOGWARNING, "CSettingList: unable to read type of list setting of %s", m_id.c_str());
    return false;
  }

  // deserialize the setting definition in update mode because we don't care
  // about an invalid <default> value (which is never used)
  if (!m_definition->Deserialize(node, true))
    return false;

  auto constraints = node->FirstChild(SETTING_XML_ELM_CONSTRAINTS);
  if (constraints != nullptr)
  {
    // read the delimiter
    std::string delimiter;
    if (XMLUtils::GetString(constraints, SETTING_XML_ELM_DELIMITER, delimiter) && !delimiter.empty())
      m_delimiter = delimiter;

    XMLUtils::GetInt(constraints, SETTING_XML_ELM_MINIMUM_ITEMS, m_minimumItems);
    if (m_minimumItems < 0)
      m_minimumItems = 0;
    XMLUtils::GetInt(constraints, SETTING_XML_ELM_MAXIMUM_ITEMS, m_maximumItems);
    if (m_maximumItems <= 0)
      m_maximumItems = -1;
    else if (m_maximumItems < m_minimumItems)
    {
      CLog::Log(LOGWARNING, "CSettingList: invalid <minimum> (%d) and/or <maximum> (%d) of %s", m_minimumItems, m_maximumItems, m_id.c_str());
      return false;
    }
  }

  // read the default and initial values
  std::string values;
  if (XMLUtils::GetString(node, SETTING_XML_ELM_DEFAULT, values))
  {
    if (!fromString(values, m_defaults))
    {
      CLog::Log(LOGWARNING, "CSettingList: invalid <default> definition \"%s\" of %s", values.c_str(), m_id.c_str());
      return false;
    }
    Reset();
  }

  return true;
}

SettingType CSettingList::GetElementType() const
{
  CSharedLock lock(m_critical);
  
  if (m_definition == nullptr)
    return SettingType::Unknown;

  return m_definition->GetType();
}

bool CSettingList::FromString(const std::string &value)
{
  SettingList values;
  if (!fromString(value, values))
    return false;

  return SetValue(values);
}

std::string CSettingList::ToString() const
{
  return toString(m_values);
}

bool CSettingList::Equals(const std::string &value) const
{
  SettingList values;
  if (!fromString(value, values) || values.size() != m_values.size())
    return false;

  bool ret = true;
  for (size_t index = 0; index < values.size(); index++)
  {
    if (!m_values[index]->Equals(values[index]->ToString()))
    {
      ret = false;
      break;
    }
  }

  return ret;
}

bool CSettingList::CheckValidity(const std::string &value) const
{
  SettingList values;
  return fromString(value, values);
}

void CSettingList::Reset()
{
  CExclusiveLock lock(m_critical);
  SettingList values;
  for (auto it : m_defaults)
    values.push_back(SettingPtr(it->Clone(it->GetId())));

  SetValue(values);
}

bool CSettingList::FromString(const std::vector<std::string> &value)
{
  SettingList values;
  if (!fromValues(value, values))
    return false;

  return SetValue(values);
}

bool CSettingList::SetValue(const SettingList &values)
{
  CExclusiveLock lock(m_critical);

  if ((int)values.size() < m_minimumItems ||
     (m_maximumItems > 0 && (int)values.size() > m_maximumItems))
    return false;

  bool equal = values.size() == m_values.size();
  for (size_t index = 0; index < values.size(); index++)
  {
    if (values[index]->GetType() != GetElementType())
      return false;

    if (equal &&
        !values[index]->Equals(m_values[index]->ToString()))
      equal = false;
  }

  if (equal)
    return true;

  SettingList oldValues = m_values;
  m_values.clear();
  m_values.insert(m_values.begin(), values.begin(), values.end());

  if (!OnSettingChanging(shared_from_base<CSettingList>()))
  {
    m_values = oldValues;

    // the setting couldn't be changed because one of the
    // callback handlers failed the OnSettingChanging()
    // callback so we need to let all the callback handlers
    // know that the setting hasn't changed
    OnSettingChanging(shared_from_base<CSettingList>());
    return false;
  }

  m_changed = toString(m_values) != toString(m_defaults);
  OnSettingChanged(shared_from_base<CSettingList>());
  return true;
}

void CSettingList::SetDefault(const SettingList &values)
{
  CExclusiveLock lock(m_critical);

  m_defaults.clear();
  m_defaults.insert(m_defaults.begin(), values.begin(), values.end());

  if (!m_changed)
  {
    m_values.clear();
    for (auto it : m_defaults)
      m_values.push_back(SettingPtr(it->Clone(it->GetId())));
  }
}

void CSettingList::copy(const CSettingList &setting)
{
  CSetting::Copy(setting);

  copy(setting.m_values, m_values);
  copy(setting.m_defaults, m_defaults);
  
  if (setting.m_definition != nullptr)
  {
    auto definitionCopy = setting.m_definition->Clone(m_id + ".definition");
    if (definitionCopy != nullptr)
      m_definition = definitionCopy;
  }

  m_delimiter = setting.m_delimiter;
  m_minimumItems = setting.m_minimumItems;
  m_maximumItems = setting.m_maximumItems;
}

void CSettingList::copy(const SettingList &srcValues, SettingList &dstValues)
{
  dstValues.clear();

  for (auto value : srcValues)
  {
    if (value == nullptr)
      continue;

    SettingPtr valueCopy = value->Clone(value->GetId());
    if (valueCopy == nullptr)
      continue;

    dstValues.emplace_back(valueCopy);
  }
}

bool CSettingList::fromString(const std::string &strValue, SettingList &values) const
{
  return fromValues(StringUtils::Split(strValue, m_delimiter), values);
}

bool CSettingList::fromValues(const std::vector<std::string> &strValues, SettingList &values) const
{
  if ((int)strValues.size() < m_minimumItems ||
     (m_maximumItems > 0 && (int)strValues.size() > m_maximumItems))
    return false;

  bool ret = true;
  int index = 0;
  for (auto value : strValues)
  {
    auto settingValue = m_definition->Clone(StringUtils::Format("%s.%d", m_id.c_str(), index++));
    if (settingValue == nullptr ||
        !settingValue->FromString(value))
    {
      ret = false;
      break;
    }

    values.emplace_back(settingValue);
  }

  if (!ret)
    values.clear();

  return ret;
}

std::string CSettingList::toString(const SettingList &values) const
{
  std::vector<std::string> strValues;
  for (auto value : values)
  {
    if (value != nullptr)
      strValues.push_back(value->ToString());
  }

  return StringUtils::Join(strValues, m_delimiter);
}
  
CSettingBool::CSettingBool(const std::string &id, CSettingsManager *settingsManager /* = nullptr */)
  : CTraitedSetting(id, settingsManager)
{ }
  
CSettingBool::CSettingBool(const std::string &id, const CSettingBool &setting)
  : CTraitedSetting(id, setting)
{
  copy(setting);
}

CSettingBool::CSettingBool(const std::string &id, int label, bool value, CSettingsManager *settingsManager /* = nullptr */)
  : CTraitedSetting(id, settingsManager)
  , m_value(value)
  , m_default(value)
{
  SetLabel(label);
}

SettingPtr CSettingBool::Clone(const std::string &id) const
{
  return std::make_shared<CSettingBool>(id, *this);
}

bool CSettingBool::Deserialize(const TiXmlNode *node, bool update /* = false */)
{
  CExclusiveLock lock(m_critical);

  if (!CSetting::Deserialize(node, update))
    return false;
    
  // get the default value
  bool value;
  if (XMLUtils::GetBoolean(node, SETTING_XML_ELM_DEFAULT, value))
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
  CExclusiveLock lock(m_critical);

  if (value == m_value)
    return true;

  bool oldValue = m_value;
  m_value = value;

  if (!OnSettingChanging(shared_from_base<CSettingBool>()))
  {
    m_value = oldValue;

    // the setting couldn't be changed because one of the
    // callback handlers failed the OnSettingChanging()
    // callback so we need to let all the callback handlers
    // know that the setting hasn't changed
    OnSettingChanging(shared_from_base<CSettingBool>());
    return false;
  }

  m_changed = m_value != m_default;
  OnSettingChanged(shared_from_base<CSettingBool>());
  return true;
}
  
void CSettingBool::SetDefault(bool value)
{
  CExclusiveLock lock(m_critical);

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

CSettingInt::CSettingInt(const std::string &id, CSettingsManager *settingsManager /* = nullptr */)
  : CTraitedSetting(id, settingsManager)
{ }
  
CSettingInt::CSettingInt(const std::string &id, const CSettingInt &setting)
  : CTraitedSetting(id, setting)
{
  copy(setting);
}

CSettingInt::CSettingInt(const std::string &id, int label, int value, CSettingsManager *settingsManager /* = nullptr */)
  : CTraitedSetting(id, settingsManager)
  , m_value(value)
  , m_default(value)
{
  SetLabel(label);
}

CSettingInt::CSettingInt(const std::string &id, int label, int value, int minimum, int step, int maximum, CSettingsManager *settingsManager /* = nullptr */)
  : CTraitedSetting(id, settingsManager)
  , m_value(value)
  , m_default(value)
  , m_min(minimum)
  , m_step(step)
  , m_max(maximum)
{
  SetLabel(label);
}

CSettingInt::CSettingInt(const std::string &id, int label, int value, const TranslatableIntegerSettingOptions &options, CSettingsManager *settingsManager /* = nullptr */)
  : CTraitedSetting(id, settingsManager)
  , m_value(value)
  , m_default(value)
  , m_translatableOptions(options)
{
  SetLabel(label);
}

SettingPtr CSettingInt::Clone(const std::string &id) const
{
  return std::make_shared<CSettingInt>(id, *this);
}

bool CSettingInt::Deserialize(const TiXmlNode *node, bool update /* = false */)
{
  CExclusiveLock lock(m_critical);

  if (!CSetting::Deserialize(node, update))
    return false;

  // get the default value
  int value;
  if (XMLUtils::GetInt(node, SETTING_XML_ELM_DEFAULT, value))
    m_value = m_default = value;
  else if (!update)
  {
    CLog::Log(LOGERROR, "CSettingInt: error reading the default value of \"%s\"", m_id.c_str());
    return false;
  }

  auto constraints = node->FirstChild(SETTING_XML_ELM_CONSTRAINTS);
  if (constraints != nullptr)
  {
    // get the entries
    auto options = constraints->FirstChild(SETTING_XML_ELM_OPTIONS);
    if (options != nullptr && options->FirstChild() != nullptr)
    {
      if (options->FirstChild()->Type() == TiXmlNode::TINYXML_TEXT)
      {
        m_optionsFillerName = options->FirstChild()->ValueStr();
        if (!m_optionsFillerName.empty())
        {
          m_optionsFiller = reinterpret_cast<IntegerSettingOptionsFiller>(m_settingsManager->GetSettingOptionsFiller(shared_from_base<CSettingInt>()));
          if (m_optionsFiller == nullptr)
            CLog::Log(LOGWARNING, "CSettingInt: unknown options filler \"%s\" of \"%s\"", m_optionsFillerName.c_str(), m_id.c_str());
        }
      }
      else
      {
        m_translatableOptions.clear();
        auto optionElement = options->FirstChildElement(SETTING_XML_ELM_OPTION);
        while (optionElement != nullptr)
        {
          std::pair<int, int> entry;
          if (optionElement->QueryIntAttribute(SETTING_XML_ATTR_LABEL, &entry.first) == TIXML_SUCCESS && entry.first > 0)
          {
            entry.second = strtol(optionElement->FirstChild()->Value(), nullptr, 10);
            m_translatableOptions.push_back(entry);
          }

          optionElement = optionElement->NextSiblingElement(SETTING_XML_ELM_OPTION);
        }
      }
    }

    // get minimum
    XMLUtils::GetInt(constraints, SETTING_XML_ELM_MINIMUM, m_min);
    // get step
    XMLUtils::GetInt(constraints, SETTING_XML_ELM_STEP, m_step);
    // get maximum
    XMLUtils::GetInt(constraints, SETTING_XML_ELM_MAXIMUM, m_max);
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
  if (!m_translatableOptions.empty())
  {
    if (!CheckSettingOptionsValidity(value, m_translatableOptions))
      return false;
  }
  else if (!m_options.empty())
  {
    if (!CheckSettingOptionsValidity(value, m_options))
      return false;
  }
  else if (m_optionsFillerName.empty() && m_optionsFiller == nullptr &&
           m_min != m_max && (value < m_min || value > m_max))
    return false;

  return true;
}

bool CSettingInt::SetValue(int value)
{
  CExclusiveLock lock(m_critical);

  if (value == m_value)
    return true;

  if (!CheckValidity(value))
    return false;

  int oldValue = m_value;
  m_value = value;

  if (!OnSettingChanging(shared_from_base<CSettingInt>()))
  {
    m_value = oldValue;

    // the setting couldn't be changed because one of the
    // callback handlers failed the OnSettingChanging()
    // callback so we need to let all the callback handlers
    // know that the setting hasn't changed
    OnSettingChanging(shared_from_base<CSettingInt>());
    return false;
  }

  m_changed = m_value != m_default;
  OnSettingChanged(shared_from_base<CSettingInt>());
  return true;
}

void CSettingInt::SetDefault(int value)
{
  CExclusiveLock lock(m_critical);

  m_default = value;
  if (!m_changed)
    m_value = m_default;
}

SettingOptionsType CSettingInt::GetOptionsType() const
{
  CSharedLock lock(m_critical);
  if (!m_translatableOptions.empty())
    return SettingOptionsType::StaticTranslatable;
  if (!m_options.empty())
    return SettingOptionsType::Static;
  if (!m_optionsFillerName.empty() || m_optionsFiller != nullptr)
    return SettingOptionsType::Dynamic;

  return SettingOptionsType::Unknown;
}

IntegerSettingOptions CSettingInt::UpdateDynamicOptions()
{
  CExclusiveLock lock(m_critical);
  IntegerSettingOptions options;
  if (m_optionsFiller == nullptr &&
     (m_optionsFillerName.empty() || m_settingsManager == nullptr))
    return options;

  if (m_optionsFiller == nullptr)
  {
    m_optionsFiller = reinterpret_cast<IntegerSettingOptionsFiller>(m_settingsManager->GetSettingOptionsFiller(shared_from_base<CSettingInt>()));
    if (m_optionsFiller == nullptr)
      return options;
  }

  int bestMatchingValue = m_value;
  m_optionsFiller(shared_from_base<CSettingInt>(), options, bestMatchingValue, m_optionsFillerData);

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
    OnSettingPropertyChanged(shared_from_base<CSettingInt>(), "options");
  }

  return options;
}

void CSettingInt::copy(const CSettingInt &setting)
{
  CSetting::Copy(setting);

  CExclusiveLock lock(m_critical);

  m_value = setting.m_value;
  m_default = setting.m_default;
  m_min = setting.m_min;
  m_step = setting.m_step;
  m_max = setting.m_max;
  m_translatableOptions = setting.m_translatableOptions;
  m_options = setting.m_options;
  m_optionsFillerName = setting.m_optionsFillerName;
  m_optionsFiller = setting.m_optionsFiller;
  m_optionsFillerData = setting.m_optionsFillerData;
  m_dynamicOptions = setting.m_dynamicOptions;
}

bool CSettingInt::fromString(const std::string &strValue, int &value)
{
  if (strValue.empty())
    return false;

  char *end = nullptr;
  value = (int)strtol(strValue.c_str(), &end, 10);
  if (end != nullptr && *end != '\0')
    return false; 

  return true;
}

CSettingNumber::CSettingNumber(const std::string &id, CSettingsManager *settingsManager /* = nullptr */)
  : CTraitedSetting(id, settingsManager)
{ }
  
CSettingNumber::CSettingNumber(const std::string &id, const CSettingNumber &setting)
  : CTraitedSetting(id, setting)
{
  copy(setting);
}

CSettingNumber::CSettingNumber(const std::string &id, int label, float value, CSettingsManager *settingsManager /* = nullptr */)
  : CTraitedSetting(id, settingsManager)
  , m_value(value)
  , m_default(value)
{
  SetLabel(label);
}

CSettingNumber::CSettingNumber(const std::string &id, int label, float value, float minimum, float step, float maximum, CSettingsManager *settingsManager /* = nullptr */)
  : CTraitedSetting(id, settingsManager)
  , m_value(value)
  , m_default(value)
  , m_min(minimum)
  , m_step(step)
  , m_max(maximum)
{
  SetLabel(label);
}

SettingPtr CSettingNumber::Clone(const std::string &id) const
{
  return std::make_shared<CSettingNumber>(id, *this);
}

bool CSettingNumber::Deserialize(const TiXmlNode *node, bool update /* = false */)
{
  CExclusiveLock lock(m_critical);

  if (!CSetting::Deserialize(node, update))
    return false;
    
  // get the default value
  double value;
  if (XMLUtils::GetDouble(node, SETTING_XML_ELM_DEFAULT, value))
    m_value = m_default = value;
  else if (!update)
  {
    CLog::Log(LOGERROR, "CSettingNumber: error reading the default value of \"%s\"", m_id.c_str());
    return false;
  }
    
  auto constraints = node->FirstChild(SETTING_XML_ELM_CONSTRAINTS);
  if (constraints != nullptr)
  {
    // get the minimum value
    XMLUtils::GetDouble(constraints, SETTING_XML_ELM_MINIMUM, m_min);
    // get the step value
    XMLUtils::GetDouble(constraints, SETTING_XML_ELM_STEP, m_step);
    // get the maximum value
    XMLUtils::GetDouble(constraints, SETTING_XML_ELM_MAXIMUM, m_max);
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
  CSharedLock lock(m_critical);
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
  CSharedLock lock(m_critical);
  if (m_min != m_max &&
     (value < m_min || value > m_max))
    return false;

  return true;
}

bool CSettingNumber::SetValue(double value)
{
  CExclusiveLock lock(m_critical);

  if (value == m_value)
    return true;

  if (!CheckValidity(value))
    return false;

  double oldValue = m_value;
  m_value = value;

  if (!OnSettingChanging(shared_from_base<CSettingNumber>()))
  {
    m_value = oldValue;

    // the setting couldn't be changed because one of the
    // callback handlers failed the OnSettingChanging()
    // callback so we need to let all the callback handlers
    // know that the setting hasn't changed
    OnSettingChanging(shared_from_base<CSettingNumber>());
    return false;
  }

  m_changed = m_value != m_default;
  OnSettingChanged(shared_from_base<CSettingNumber>());
  return true;
}

void CSettingNumber::SetDefault(double value)
{
  CExclusiveLock lock(m_critical);

  m_default = value;
  if (!m_changed)
    m_value = m_default;
}

void CSettingNumber::copy(const CSettingNumber &setting)
{
  CSetting::Copy(setting);
  CExclusiveLock lock(m_critical);

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

  char *end = nullptr;
  value = strtod(strValue.c_str(), &end);
  if (end != nullptr && *end != '\0')
    return false;

  return true;
}

CSettingString::CSettingString(const std::string &id, CSettingsManager *settingsManager /* = nullptr */)
  : CTraitedSetting(id, settingsManager)
{ }
  
CSettingString::CSettingString(const std::string &id, const CSettingString &setting)
  : CTraitedSetting(id, setting)
{
  copy(setting);
}

CSettingString::CSettingString(const std::string &id, int label, const std::string &value, CSettingsManager *settingsManager /* = nullptr */)
  : CTraitedSetting(id, settingsManager)
  , m_value(value)
  , m_default(value)
{
  SetLabel(label);
}

SettingPtr CSettingString::Clone(const std::string &id) const
{
  return std::make_shared<CSettingString>(id, *this);
}

bool CSettingString::Deserialize(const TiXmlNode *node, bool update /* = false */)
{
  CExclusiveLock lock(m_critical);

  if (!CSetting::Deserialize(node, update))
    return false;

  auto constraints = node->FirstChild(SETTING_XML_ELM_CONSTRAINTS);
  if (constraints != nullptr)
  {
    // get allowempty (needs to be parsed before parsing the default value)
    XMLUtils::GetBoolean(constraints, SETTING_XML_ELM_ALLOWEMPTY, m_allowEmpty);

    // get the entries
    auto options = constraints->FirstChild(SETTING_XML_ELM_OPTIONS);
    if (options != nullptr && options->FirstChild() != nullptr)
    {
      if (options->FirstChild()->Type() == TiXmlNode::TINYXML_TEXT)
      {
        m_optionsFillerName = options->FirstChild()->ValueStr();
        if (!m_optionsFillerName.empty())
        {
          m_optionsFiller = reinterpret_cast<StringSettingOptionsFiller>(m_settingsManager->GetSettingOptionsFiller(shared_from_base<CSettingString>()));
          if (m_optionsFiller == nullptr)
            CLog::Log(LOGWARNING, "CSettingString: unknown options filler \"%s\" of \"%s\"", m_optionsFillerName.c_str(), m_id.c_str());
        }
      }
      else
      {
        m_translatableOptions.clear();
        auto optionElement = options->FirstChildElement(SETTING_XML_ELM_OPTION);
        while (optionElement != nullptr)
        {
          TranslatableStringSettingOption entry;
          if (optionElement->QueryIntAttribute(SETTING_XML_ATTR_LABEL, &entry.first) == TIXML_SUCCESS && entry.first > 0)
          {
            entry.second = optionElement->FirstChild()->Value();
            m_translatableOptions.push_back(entry);
          }

          optionElement = optionElement->NextSiblingElement(SETTING_XML_ELM_OPTION);
        }
      }
    }
  }

  // get the default value
  std::string value;
  if (XMLUtils::GetString(node, SETTING_XML_ELM_DEFAULT, value) &&
     (!value.empty() || m_allowEmpty))
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
  CSharedLock lock(m_critical);
  if (!m_allowEmpty && value.empty())
    return false;

  if (!m_translatableOptions.empty())
  {
    if (!CheckSettingOptionsValidity(value, m_translatableOptions))
      return false;
  }
  else if (!m_options.empty())
  {
    if (!CheckSettingOptionsValidity(value, m_options))
      return false;
  }

  return true;
}

bool CSettingString::SetValue(const std::string &value)
{
  CExclusiveLock lock(m_critical);

  if (value == m_value)
    return true;
    
  if (!CheckValidity(value))
    return false;

  std::string oldValue = m_value;
  m_value = value;

  if (!OnSettingChanging(shared_from_base<CSettingString>()))
  {
    m_value = oldValue;

    // the setting couldn't be changed because one of the
    // callback handlers failed the OnSettingChanging()
    // callback so we need to let all the callback handlers
    // know that the setting hasn't changed
    OnSettingChanging(shared_from_base<CSettingString>());
    return false;
  }

  m_changed = m_value != m_default;
  OnSettingChanged(shared_from_base<CSettingString>());
  return true;
}

void CSettingString::SetDefault(const std::string &value)
{
  CSharedLock lock(m_critical);

  m_default = value;
  if (!m_changed)
    m_value = m_default;
}

SettingOptionsType CSettingString::GetOptionsType() const
{
  CSharedLock lock(m_critical);
  if (!m_translatableOptions.empty())
    return SettingOptionsType::StaticTranslatable;
  if (!m_options.empty())
    return SettingOptionsType::Static;
  if (!m_optionsFillerName.empty() || m_optionsFiller != nullptr)
    return SettingOptionsType::Dynamic;

  return SettingOptionsType::Unknown;
}

StringSettingOptions CSettingString::UpdateDynamicOptions()
{
  CExclusiveLock lock(m_critical);
  StringSettingOptions options;
  if (m_optionsFiller == nullptr &&
     (m_optionsFillerName.empty() || m_settingsManager == nullptr))
    return options;

  if (m_optionsFiller == nullptr)
  {
    m_optionsFiller = reinterpret_cast<StringSettingOptionsFiller>(m_settingsManager->GetSettingOptionsFiller(shared_from_base<CSettingString>()));
    if (m_optionsFiller == nullptr)
      return options;
  }

  std::string bestMatchingValue = m_value;
  m_optionsFiller(shared_from_base<CSettingString>(), options, bestMatchingValue, m_optionsFillerData);

  if (bestMatchingValue != m_value)
    SetValue(bestMatchingValue);

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
    OnSettingPropertyChanged(shared_from_base<CSettingString>(), "options");
  }

  return options;
}

void CSettingString::copy(const CSettingString &setting)
{
  CSetting::Copy(setting);

  CExclusiveLock lock(m_critical);
  m_value = setting.m_value;
  m_default = setting.m_default;
  m_allowEmpty = setting.m_allowEmpty;
  m_translatableOptions = setting.m_translatableOptions;
  m_options = setting.m_options;
  m_optionsFillerName = setting.m_optionsFillerName;
  m_optionsFiller = setting.m_optionsFiller;
  m_optionsFillerData = setting.m_optionsFillerData;
  m_dynamicOptions = setting.m_dynamicOptions;
}
  
CSettingAction::CSettingAction(const std::string &id, CSettingsManager *settingsManager /* = nullptr */)
  : CSetting(id, settingsManager)
{ }
  
CSettingAction::CSettingAction(const std::string &id, int label, CSettingsManager *settingsManager /* = nullptr */)
  : CSetting(id, settingsManager)
{
  SetLabel(label);
}
  
CSettingAction::CSettingAction(const std::string &id, const CSettingAction &setting)
  : CSetting(id, setting)
  , m_data(setting.m_data)
{ }

SettingPtr CSettingAction::Clone(const std::string &id) const
{
  return std::make_shared<CSettingAction>(id, *this);
}

bool CSettingAction::Deserialize(const TiXmlNode *node, bool update /* = false */)
{
  CSharedLock lock(m_critical);

  if (!CSetting::Deserialize(node, update))
    return false;

  m_data = XMLUtils::GetString(node, SETTING_XML_ELM_DATA);
    
  return true;
}
