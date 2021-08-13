/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Setting.h"

#include "ServiceBroker.h"
#include "SettingDefinitions.h"
#include "SettingsManager.h"
#include "utils/StringUtils.h"
#include "utils/XBMCTinyXML.h"
#include "utils/XMLUtils.h"
#include "utils/log.h"

#include <sstream>
#include <utility>

template<typename TKey, typename TValue>
bool CheckSettingOptionsValidity(const TValue& value, const std::vector<std::pair<TKey, TValue>>& options)
{
  for (const auto& it : options)
  {
    if (it.second == value)
      return true;
  }

  return false;
}

template<typename TKey, typename TValue>
bool CheckSettingOptionsValidity(const TValue& value, const std::vector<TKey>& options)
{
  for (const auto& it : options)
  {
    if (it.value == value)
      return true;
  }

  return false;
}

bool DeserializeOptionsSort(const TiXmlElement* optionsElement, SettingOptionsSort& optionsSort)
{
  optionsSort = SettingOptionsSort::NoSorting;

  std::string sort;
  if (optionsElement->QueryStringAttribute("sort", &sort) != TIXML_SUCCESS)
    return true;

  if (StringUtils::EqualsNoCase(sort, "false") || StringUtils::EqualsNoCase(sort, "off") ||
    StringUtils::EqualsNoCase(sort, "no") || StringUtils::EqualsNoCase(sort, "disabled"))
    optionsSort = SettingOptionsSort::NoSorting;
  else if (StringUtils::EqualsNoCase(sort, "asc") || StringUtils::EqualsNoCase(sort, "ascending") ||
    StringUtils::EqualsNoCase(sort, "true") || StringUtils::EqualsNoCase(sort, "on") ||
    StringUtils::EqualsNoCase(sort, "yes") || StringUtils::EqualsNoCase(sort, "enabled"))
    optionsSort = SettingOptionsSort::Ascending;
  else if (StringUtils::EqualsNoCase(sort, "desc") || StringUtils::EqualsNoCase(sort, "descending"))
    optionsSort = SettingOptionsSort::Descending;
  else
    return false;

  return true;
}

Logger CSetting::s_logger;

CSetting::CSetting(const std::string& id, CSettingsManager* settingsManager /* = nullptr */)
  : ISetting(id, settingsManager)
{
  if (s_logger == nullptr)
    s_logger = CServiceBroker::GetLogging().GetLogger("CSetting");
}

CSetting::CSetting(const std::string& id, const CSetting& setting)
  : CSetting(id, setting.m_settingsManager)
{
  Copy(setting);
}

void CSetting::MergeBasics(const CSetting& other)
{
  // ISetting
  SetVisible(other.GetVisible());
  SetLabel(other.GetLabel());
  SetHelp(other.GetHelp());
  SetRequirementsMet(other.MeetsRequirements());
  // CSetting
  SetEnabled(other.GetEnabled());
  SetParent(other.GetParent());
  SetLevel(other.GetLevel());
  SetControl(const_cast<CSetting&>(other).GetControl());
  SetDependencies(other.GetDependencies());
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

  // get <enable>
  bool value;
  if (XMLUtils::GetBoolean(node, SETTING_XML_ELM_ENABLED, value))
    m_enabled = value;

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
        s_logger->warn("error reading <{}> tag of \"{}\"", SETTING_XML_ELM_DEPENDENCY, m_id);

      dependencyNode = dependencyNode->NextSibling(SETTING_XML_ELM_DEPENDENCY);
    }
  }

  auto control = node->FirstChildElement(SETTING_XML_ELM_CONTROL);
  if (control != nullptr)
  {
    auto controlType = control->Attribute(SETTING_XML_ATTR_TYPE);
    if (controlType == nullptr)
    {
      s_logger->error("error reading \"{}\" attribute of <control> tag of \"{}\"",
                      SETTING_XML_ATTR_TYPE, m_id);
      return false;
    }

    m_control = m_settingsManager->CreateControl(controlType);
    if (m_control == nullptr || !m_control->Deserialize(control, update))
    {
      s_logger->error("error reading <{}> tag of \"{}\"", SETTING_XML_ELM_CONTROL, m_id);
      return false;
    }
  }
  else if (!update && m_level < SettingLevel::Internal && !IsReference())
  {
    s_logger->error("missing <{}> tag of \"{}\"", SETTING_XML_ELM_CONTROL, m_id);
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
          s_logger->warn("duplicate <{}> definition for \"{}\"", SETTING_XML_ELM_UPDATE, m_id);
      }
      else
        s_logger->warn("error reading <{}> tag of \"{}\"", SETTING_XML_ELM_UPDATE, m_id);

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

  bool enabled = m_enabled;
  for (const auto& dep : m_dependencies)
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

void CSetting::MakeReference(const std::string& referencedId /* = "" */)
{
  auto tmpReferencedId = referencedId;
  if (referencedId.empty())
    tmpReferencedId = m_id;

  m_id = StringUtils::Format("#{}[{}]", tmpReferencedId, StringUtils::CreateUUID());
  m_referencedId = tmpReferencedId;
}

bool CSetting::IsVisible() const
{
  if (!ISetting::IsVisible())
    return false;

  bool visible = true;
  for (const auto& dep : m_dependencies)
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

bool CSetting::OnSettingChanging(const std::shared_ptr<const CSetting>& setting)
{
  if (m_callback == nullptr)
    return true;

  return m_callback->OnSettingChanging(setting);
}

void CSetting::OnSettingChanged(const std::shared_ptr<const CSetting>& setting)
{
  if (m_callback == nullptr)
    return;

  m_callback->OnSettingChanged(setting);
}

void CSetting::OnSettingAction(const std::shared_ptr<const CSetting>& setting)
{
  if (m_callback == nullptr)
    return;

  m_callback->OnSettingAction(setting);
}

bool CSetting::DeserializeIdentification(const TiXmlNode* node,
                                         std::string& identification,
                                         bool& isReference)
{
  isReference = false;

  // first check if we can simply retrieve the setting's identifier
  if (ISetting::DeserializeIdentification(node, identification))
    return true;

  // otherwise try to retrieve a reference to another setting's identifier
  if (!DeserializeIdentificationFromAttribute(node, SETTING_XML_ATTR_REFERENCE, identification))
    return false;

  isReference = true;
  return true;
}

bool CSetting::OnSettingUpdate(const std::shared_ptr<CSetting>& setting,
                               const char* oldSettingId,
                               const TiXmlNode* oldSettingNode)
{
  if (m_callback == nullptr)
    return false;

  return m_callback->OnSettingUpdate(setting, oldSettingId, oldSettingNode);
}

void CSetting::OnSettingPropertyChanged(const std::shared_ptr<const CSetting>& setting,
                                        const char* propertyName)
{
  if (m_callback == nullptr)
    return;

  m_callback->OnSettingPropertyChanged(setting, propertyName);
}

void CSetting::Copy(const CSetting &setting)
{
  SetVisible(setting.IsVisible());
  SetLabel(setting.GetLabel());
  SetHelp(setting.GetHelp());
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

Logger CSettingList::s_logger;

CSettingList::CSettingList(const std::string& id,
                           std::shared_ptr<CSetting> settingDefinition,
                           CSettingsManager* settingsManager /* = nullptr */)
  : CSetting(id, settingsManager), m_definition(std::move(settingDefinition))
{
  if (s_logger == nullptr)
    s_logger = CServiceBroker::GetLogging().GetLogger("CSettingList");
}

CSettingList::CSettingList(const std::string& id,
                           std::shared_ptr<CSetting> settingDefinition,
                           int label,
                           CSettingsManager* settingsManager /* = nullptr */)
  : CSettingList(id, std::move(settingDefinition), settingsManager)
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

void CSettingList::MergeDetails(const CSetting& other)
{
  if (other.GetType() != SettingType::List)
    return;

  const auto& listSetting = static_cast<const CSettingList&>(other);
  if (m_definition == nullptr && listSetting.m_definition != nullptr)
    m_definition = listSetting.m_definition;
  if (m_defaults.empty() && !listSetting.m_defaults.empty())
    m_defaults = listSetting.m_defaults;
  if (m_values.empty() && !listSetting.m_values.empty())
    m_values = listSetting.m_values;
  if (m_delimiter == "|" && listSetting.m_delimiter != "|")
    m_delimiter = listSetting.m_delimiter;
  if (m_minimumItems == 0 && listSetting.m_minimumItems != 0)
    m_minimumItems = listSetting.m_minimumItems;
  if (m_maximumItems == -1 && listSetting.m_maximumItems != -1)
    m_maximumItems = listSetting.m_maximumItems;
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
    s_logger->warn("unable to read type of list setting of {}", m_id);
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
      s_logger->warn("invalid <{}> ({}) and/or <{}> ({}) of {}", SETTING_XML_ELM_MINIMUM_ITEMS,
                     m_minimumItems, SETTING_XML_ELM_MAXIMUM_ITEMS, m_maximumItems, m_id);
      return false;
    }
  }

  // read the default and initial values
  std::string values;
  if (XMLUtils::GetString(node, SETTING_XML_ELM_DEFAULT, values))
  {
    if (!fromString(values, m_defaults))
    {
      s_logger->warn("invalid <{}> definition \"{}\" of {}", SETTING_XML_ELM_DEFAULT, values, m_id);
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
  for (const auto& it : m_defaults)
    values.push_back(it->Clone(it->GetId()));

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
    for (const auto& it : m_defaults)
      m_values.push_back(it->Clone(it->GetId()));
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

  for (const auto& value : srcValues)
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
  for (const auto& value : strValues)
  {
    auto settingValue = m_definition->Clone(StringUtils::Format("{}.{}", m_id, index++));
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
  for (const auto& value : values)
  {
    if (value != nullptr)
      strValues.push_back(value->ToString());
  }

  return StringUtils::Join(strValues, m_delimiter);
}

Logger CSettingBool::s_logger;

CSettingBool::CSettingBool(const std::string& id, CSettingsManager* settingsManager /* = nullptr */)
  : CSettingBool(id, DefaultLabel, DefaultValue, settingsManager)
{
}

CSettingBool::CSettingBool(const std::string& id, const CSettingBool& setting)
  : CSettingBool(id, setting.m_settingsManager)
{
  copy(setting);
}

CSettingBool::CSettingBool(const std::string& id,
                           int label,
                           bool value,
                           CSettingsManager* settingsManager /* = nullptr */)
  : CTraitedSetting(id, settingsManager), m_value(value), m_default(value)
{
  SetLabel(label);

  if (s_logger == nullptr)
    s_logger = CServiceBroker::GetLogging().GetLogger("CSettingBool");
}

SettingPtr CSettingBool::Clone(const std::string &id) const
{
  return std::make_shared<CSettingBool>(id, *this);
}

void CSettingBool::MergeDetails(const CSetting& other)
{
  if (other.GetType() != SettingType::Boolean)
    return;

  const auto& boolSetting = static_cast<const CSettingBool&>(other);
  if (m_default == false && boolSetting.m_default == true)
    m_default = boolSetting.m_default;
  if (m_value == m_default && boolSetting.m_value != m_default)
    m_value = boolSetting.m_value;
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
    s_logger->error("error reading the default value of \"{}\"", m_id);
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

Logger CSettingInt::s_logger;

CSettingInt::CSettingInt(const std::string& id, CSettingsManager* settingsManager /* = nullptr */)
  : CSettingInt(id, DefaultLabel, DefaultValue, settingsManager)
{ }

CSettingInt::CSettingInt(const std::string& id, const CSettingInt& setting)
  : CSettingInt(id, setting.m_settingsManager)
{
  copy(setting);
}

CSettingInt::CSettingInt(const std::string& id,
                         int label,
                         int value,
                         CSettingsManager* settingsManager /* = nullptr */)
  : CSettingInt(id, label, value, DefaultMin, DefaultStep, DefaultMax, settingsManager)
{
  SetLabel(label);
}

CSettingInt::CSettingInt(const std::string& id,
                         int label,
                         int value,
                         int minimum,
                         int step,
                         int maximum,
                         CSettingsManager* settingsManager /* = nullptr */)
  : CTraitedSetting(id, settingsManager),
    m_value(value),
    m_default(value),
    m_min(minimum),
    m_step(step),
    m_max(maximum)
{
  SetLabel(label);

  if (s_logger == nullptr)
    s_logger = CServiceBroker::GetLogging().GetLogger("CSettingInt");
}

CSettingInt::CSettingInt(const std::string& id,
                         int label,
                         int value,
                         const TranslatableIntegerSettingOptions& options,
                         CSettingsManager* settingsManager /* = nullptr */)
  : CSettingInt(id, label, value, settingsManager)
{
  SetTranslatableOptions(options);
}

SettingPtr CSettingInt::Clone(const std::string &id) const
{
  return std::make_shared<CSettingInt>(id, *this);
}

void CSettingInt::MergeDetails(const CSetting& other)
{
  if (other.GetType() != SettingType::Integer)
    return;

  const auto& intSetting = static_cast<const CSettingInt&>(other);
  if (m_default == 0.0 && intSetting.m_default != 0.0)
    m_default = intSetting.m_default;
  if (m_value == m_default && intSetting.m_value != m_default)
    m_value = intSetting.m_value;
  if (m_min == 0.0 && intSetting.m_min != 0.0)
    m_min = intSetting.m_min;
  if (m_step == 1.0 && intSetting.m_step != 1.0)
    m_step = intSetting.m_step;
  if (m_max == 0.0 && intSetting.m_max != 0.0)
    m_max = intSetting.m_max;
  if (m_translatableOptions.empty() && !intSetting.m_translatableOptions.empty())
    m_translatableOptions = intSetting.m_translatableOptions;
  if (m_options.empty() && !intSetting.m_options.empty())
    m_options = intSetting.m_options;
  if (m_optionsFillerName.empty() && !intSetting.m_optionsFillerName.empty())
    m_optionsFillerName = intSetting.m_optionsFillerName;
  if (m_optionsFiller == nullptr && intSetting.m_optionsFiller != nullptr)
    m_optionsFiller = intSetting.m_optionsFiller;
  if (m_optionsFillerData == nullptr && intSetting.m_optionsFillerData != nullptr)
    m_optionsFillerData = intSetting.m_optionsFillerData;
  if (m_dynamicOptions.empty() && !intSetting.m_dynamicOptions.empty())
    m_dynamicOptions = intSetting.m_dynamicOptions;
  if (m_optionsSort == SettingOptionsSort::NoSorting &&
      intSetting.m_optionsSort != SettingOptionsSort::NoSorting)
    m_optionsSort = intSetting.m_optionsSort;
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
    s_logger->error("error reading the default value of \"{}\"", m_id);
    return false;
  }

  auto constraints = node->FirstChild(SETTING_XML_ELM_CONSTRAINTS);
  if (constraints != nullptr)
  {
    // get the entries
    auto options = constraints->FirstChildElement(SETTING_XML_ELM_OPTIONS);
    if (options != nullptr && options->FirstChild() != nullptr)
    {
      if (!DeserializeOptionsSort(options, m_optionsSort))
        s_logger->warn("invalid \"sort\" attribute of <" SETTING_XML_ELM_OPTIONS "> for \"{}\"",
                       m_id);

      if (options->FirstChild()->Type() == TiXmlNode::TINYXML_TEXT)
      {
        m_optionsFillerName = options->FirstChild()->ValueStr();
        if (!m_optionsFillerName.empty())
        {
          m_optionsFiller = reinterpret_cast<IntegerSettingOptionsFiller>(m_settingsManager->GetSettingOptionsFiller(shared_from_base<CSettingInt>()));
        }
      }
      else
      {
        m_translatableOptions.clear();
        auto optionElement = options->FirstChildElement(SETTING_XML_ELM_OPTION);
        while (optionElement != nullptr)
        {
          TranslatableIntegerSettingOption entry;
          if (optionElement->QueryIntAttribute(SETTING_XML_ATTR_LABEL, &entry.label) ==
                  TIXML_SUCCESS &&
              entry.label > 0)
          {
            entry.value = strtol(optionElement->FirstChild()->Value(), nullptr, 10);
            m_translatableOptions.push_back(entry);
          }
          else
          {
            std::string label;
            if (optionElement->QueryStringAttribute(SETTING_XML_ATTR_LABEL, &label) ==
                TIXML_SUCCESS)
            {
              int value = strtol(optionElement->FirstChild()->Value(), nullptr, 10);
              m_options.emplace_back(label, value);
            }
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
    {
      s_logger->warn("unknown options filler \"{}\" of \"{}\"", m_optionsFillerName, m_id);
      return options;
    }
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
      if (options[index].label.compare(m_dynamicOptions[index].label) != 0 ||
          options[index].value != m_dynamicOptions[index].value)
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

Logger CSettingNumber::s_logger;

CSettingNumber::CSettingNumber(const std::string& id,
                               CSettingsManager* settingsManager /* = nullptr */)
  : CSettingNumber(id, DefaultLabel, DefaultValue, settingsManager)
{ }

CSettingNumber::CSettingNumber(const std::string& id, const CSettingNumber& setting)
  : CSettingNumber(id, setting.m_settingsManager)
{
  copy(setting);
}

CSettingNumber::CSettingNumber(const std::string& id,
                               int label,
                               float value,
                               CSettingsManager* settingsManager /* = nullptr */)
  : CSettingNumber(id, label, value, DefaultMin, DefaultStep, DefaultMax, settingsManager)
{
}

CSettingNumber::CSettingNumber(const std::string& id,
                               int label,
                               float value,
                               float minimum,
                               float step,
                               float maximum,
                               CSettingsManager* settingsManager /* = nullptr */)
  : CTraitedSetting(id, settingsManager),
    m_value(static_cast<double>(value)),
    m_default(static_cast<double>(value)),
    m_min(static_cast<double>(minimum)),
    m_step(static_cast<double>(step)),
    m_max(static_cast<double>(maximum))
{
  SetLabel(label);

  if (s_logger == nullptr)
    s_logger = CServiceBroker::GetLogging().GetLogger("CSettingNumber");
}

SettingPtr CSettingNumber::Clone(const std::string &id) const
{
  return std::make_shared<CSettingNumber>(id, *this);
}

void CSettingNumber::MergeDetails(const CSetting& other)
{
  if (other.GetType() != SettingType::Number)
    return;

  const auto& numberSetting = static_cast<const CSettingNumber&>(other);
  if (m_default == 0.0 && numberSetting.m_default != 0.0)
    m_default = numberSetting.m_default;
  if (m_value == m_default && numberSetting.m_value != m_default)
    m_value = numberSetting.m_value;
  if (m_min == 0.0 && numberSetting.m_min != 0.0)
    m_min = numberSetting.m_min;
  if (m_step == 1.0 && numberSetting.m_step != 1.0)
    m_step = numberSetting.m_step;
  if (m_max == 0.0 && numberSetting.m_max != 0.0)
    m_max = numberSetting.m_max;
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
    s_logger->error("error reading the default value of \"{}\"", m_id);
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

const CSettingString::Value CSettingString::DefaultValue;
Logger CSettingString::s_logger;

CSettingString::CSettingString(const std::string& id,
                               CSettingsManager* settingsManager /* = nullptr */)
  : CSettingString(id, DefaultLabel, DefaultValue, settingsManager)
{ }

CSettingString::CSettingString(const std::string& id, const CSettingString& setting)
  : CSettingString(id, setting.m_settingsManager)
{
  copy(setting);
}

CSettingString::CSettingString(const std::string& id,
                               int label,
                               const std::string& value,
                               CSettingsManager* settingsManager /* = nullptr */)
  : CTraitedSetting(id, settingsManager), m_value(value), m_default(value)
{
  SetLabel(label);

  if (s_logger == nullptr)
    s_logger = CServiceBroker::GetLogging().GetLogger("CSettingString");
}

SettingPtr CSettingString::Clone(const std::string &id) const
{
  return std::make_shared<CSettingString>(id, *this);
}

void CSettingString::MergeDetails(const CSetting& other)
{
  if (other.GetType() != SettingType::String)
    return;

  const auto& stringSetting = static_cast<const CSettingString&>(other);
  if (m_default.empty() && !stringSetting.m_default.empty())
    m_default = stringSetting.m_default;
  if (m_value == m_default && stringSetting.m_value != m_default)
    m_value = stringSetting.m_value;
  if (m_allowEmpty == false && stringSetting.m_allowEmpty == true)
    m_allowEmpty = stringSetting.m_allowEmpty;
  if (m_allowNewOption == false && stringSetting.m_allowNewOption == true)
    m_allowNewOption = stringSetting.m_allowNewOption;
  if (m_translatableOptions.empty() && !stringSetting.m_translatableOptions.empty())
    m_translatableOptions = stringSetting.m_translatableOptions;
  if (m_options.empty() && !stringSetting.m_options.empty())
    m_options = stringSetting.m_options;
  if (m_optionsFillerName.empty() && !stringSetting.m_optionsFillerName.empty())
    m_optionsFillerName = stringSetting.m_optionsFillerName;
  if (m_optionsFiller == nullptr && stringSetting.m_optionsFiller != nullptr)
    m_optionsFiller = stringSetting.m_optionsFiller;
  if (m_optionsFillerData == nullptr && stringSetting.m_optionsFillerData != nullptr)
    m_optionsFillerData = stringSetting.m_optionsFillerData;
  if (m_dynamicOptions.empty() && !stringSetting.m_dynamicOptions.empty())
    m_dynamicOptions = stringSetting.m_dynamicOptions;
  if (m_optionsSort == SettingOptionsSort::NoSorting &&
      stringSetting.m_optionsSort != SettingOptionsSort::NoSorting)
    m_optionsSort = stringSetting.m_optionsSort;
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

    // Values other than those in options contraints allowed to be added
    XMLUtils::GetBoolean(constraints, SETTING_XML_ELM_ALLOWNEWOPTION, m_allowNewOption);

    // get the entries
    auto options = constraints->FirstChildElement(SETTING_XML_ELM_OPTIONS);
    if (options != nullptr && options->FirstChild() != nullptr)
    {
      if (!DeserializeOptionsSort(options, m_optionsSort))
        s_logger->warn("invalid \"sort\" attribute of <" SETTING_XML_ELM_OPTIONS "> for \"{}\"",
                       m_id);

      if (options->FirstChild()->Type() == TiXmlNode::TINYXML_TEXT)
      {
        m_optionsFillerName = options->FirstChild()->ValueStr();
        if (!m_optionsFillerName.empty())
        {
          m_optionsFiller = reinterpret_cast<StringSettingOptionsFiller>(m_settingsManager->GetSettingOptionsFiller(shared_from_base<CSettingString>()));
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
          else
          {
            const std::string value = optionElement->FirstChild()->Value();
            // if a specific "label" attribute is present use it otherwise use the value as label
            std::string label = value;
            optionElement->QueryStringAttribute(SETTING_XML_ATTR_LABEL, &label);

            m_options.emplace_back(label, value);
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
    s_logger->error("error reading the default value of \"{}\"", m_id);
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
  else if (!m_options.empty() && !m_allowNewOption)
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
    {
      s_logger->error("unknown options filler \"{}\" of \"{}\"", m_optionsFillerName, m_id);
      return options;
    }
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
      if (options[index].label.compare(m_dynamicOptions[index].label) != 0 ||
          options[index].value.compare(m_dynamicOptions[index].value) != 0)
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
  m_allowNewOption = setting.m_allowNewOption;
  m_translatableOptions = setting.m_translatableOptions;
  m_options = setting.m_options;
  m_optionsFillerName = setting.m_optionsFillerName;
  m_optionsFiller = setting.m_optionsFiller;
  m_optionsFillerData = setting.m_optionsFillerData;
  m_dynamicOptions = setting.m_dynamicOptions;
}

Logger CSettingAction::s_logger;

CSettingAction::CSettingAction(const std::string& id,
                               CSettingsManager* settingsManager /* = nullptr */)
  : CSettingAction(id, DefaultLabel, settingsManager)
{ }

CSettingAction::CSettingAction(const std::string& id,
                               int label,
                               CSettingsManager* settingsManager /* = nullptr */)
  : CSetting(id, settingsManager)
{
  SetLabel(label);

  if (s_logger == nullptr)
    s_logger = CServiceBroker::GetLogging().GetLogger("CSettingAction");
}

CSettingAction::CSettingAction(const std::string& id, const CSettingAction& setting)
  : CSettingAction(id, setting.m_settingsManager)
{
  copy(setting);
}

SettingPtr CSettingAction::Clone(const std::string &id) const
{
  return std::make_shared<CSettingAction>(id, *this);
}

void CSettingAction::MergeDetails(const CSetting& other)
{
  if (other.GetType() != SettingType::Action)
    return;

  const auto& actionSetting = static_cast<const CSettingAction&>(other);
  if (!HasData() && actionSetting.HasData())
    SetData(actionSetting.GetData());
}

bool CSettingAction::Deserialize(const TiXmlNode *node, bool update /* = false */)
{
  CSharedLock lock(m_critical);

  if (!CSetting::Deserialize(node, update))
    return false;

  m_data = XMLUtils::GetString(node, SETTING_XML_ELM_DATA);

  return true;
}

void CSettingAction::copy(const CSettingAction& setting)
{
  CSetting::Copy(setting);

  CExclusiveLock lock(m_critical);
  m_data = setting.m_data;
}
