/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SettingsOperations.h"

#include "ServiceBroker.h"
#include "addons/Addon.h"
#include "addons/Skin.h"
#include "addons/addoninfo/AddonInfo.h"
#include "guilib/LocalizeStrings.h"
#include "settings/SettingAddon.h"
#include "settings/SettingControl.h"
#include "settings/SettingDateTime.h"
#include "settings/SettingPath.h"
#include "settings/SettingUtils.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "settings/SkinSettings.h"
#include "settings/lib/Setting.h"
#include "settings/lib/SettingDefinitions.h"
#include "settings/lib/SettingSection.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"

using namespace JSONRPC;

JSONRPC_STATUS CSettingsOperations::GetSections(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  SettingLevel level = ParseSettingLevel(parameterObject["level"].asString());
  bool listCategories = !parameterObject["properties"].empty() && parameterObject["properties"][0].asString() == "categories";

  result["sections"] = CVariant(CVariant::VariantTypeArray);

  // apply the level filter
  SettingSectionList allSections = CServiceBroker::GetSettingsComponent()->GetSettings()->GetSections();
  for (const auto& itSection : allSections)
  {
    SettingCategoryList categories = itSection->GetCategories(level);
    if (categories.empty())
      continue;

    CVariant varSection(CVariant::VariantTypeObject);
    if (!SerializeSettingSection(itSection, varSection))
      continue;

    if (listCategories)
    {
      varSection["categories"] = CVariant(CVariant::VariantTypeArray);
      for (const auto& itCategory : categories)
      {
        CVariant varCategory(CVariant::VariantTypeObject);
        if (!SerializeSettingCategory(itCategory, varCategory))
          continue;

        varSection["categories"].push_back(varCategory);
      }
    }

    result["sections"].push_back(varSection);
  }

  return OK;
}

JSONRPC_STATUS CSettingsOperations::GetCategories(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  SettingLevel level = ParseSettingLevel(parameterObject["level"].asString());
  std::string strSection = parameterObject["section"].asString();
  bool listSettings = !parameterObject["properties"].empty() && parameterObject["properties"][0].asString() == "settings";

  std::vector<SettingSectionPtr> sections;
  if (!strSection.empty())
  {
    SettingSectionPtr section = CServiceBroker::GetSettingsComponent()->GetSettings()->GetSection(strSection);
    if (section == NULL)
      return InvalidParams;

    sections.push_back(section);
  }
  else
    sections = CServiceBroker::GetSettingsComponent()->GetSettings()->GetSections();

  result["categories"] = CVariant(CVariant::VariantTypeArray);

  for (const auto& itSection : sections)
  {
    SettingCategoryList categories = itSection->GetCategories(level);
    for (const auto& itCategory : categories)
    {
      CVariant varCategory(CVariant::VariantTypeObject);
      if (!SerializeSettingCategory(itCategory, varCategory))
        continue;

      if (listSettings)
      {
        varCategory["groups"] = CVariant(CVariant::VariantTypeArray);

        SettingGroupList groups = itCategory->GetGroups(level);
        for (const auto& itGroup : groups)
        {
          CVariant varGroup(CVariant::VariantTypeObject);
          if (!SerializeSettingGroup(itGroup, varGroup))
            continue;

          varGroup["settings"] = CVariant(CVariant::VariantTypeArray);
          SettingList settings = itGroup->GetSettings(level);
          for (const auto& itSetting : settings)
          {
            if (itSetting->IsVisible())
            {
              CVariant varSetting(CVariant::VariantTypeObject);
              if (!SerializeSetting(itSetting, varSetting))
                continue;

              varGroup["settings"].push_back(varSetting);
            }
          }

          varCategory["groups"].push_back(varGroup);
        }
      }

      result["categories"].push_back(varCategory);
    }
  }

  return OK;
}

JSONRPC_STATUS CSettingsOperations::GetSettings(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  SettingLevel level = ParseSettingLevel(parameterObject["level"].asString());
  const CVariant &filter = parameterObject["filter"];
  bool doFilter = filter.isMember("section") && filter.isMember("category");
  std::string strSection, strCategory;
  if (doFilter)
  {
    strSection = filter["section"].asString();
    strCategory = filter["category"].asString();
  }

  std::vector<SettingSectionPtr> sections;

  if (doFilter)
  {
    SettingSectionPtr section = CServiceBroker::GetSettingsComponent()->GetSettings()->GetSection(strSection);
    if (section == NULL)
      return InvalidParams;

    sections.push_back(section);
  }
  else
    sections = CServiceBroker::GetSettingsComponent()->GetSettings()->GetSections();

  result["settings"] = CVariant(CVariant::VariantTypeArray);

  for (const auto& itSection : sections)
  {
    SettingCategoryList categories = itSection->GetCategories(level);
    bool found = !doFilter;
    for (const auto& itCategory : categories)
    {
      if (!doFilter || StringUtils::EqualsNoCase(itCategory->GetId(), strCategory))
      {
        SettingGroupList groups = itCategory->GetGroups(level);
        for (const auto& itGroup : groups)
        {
          SettingList settings = itGroup->GetSettings(level);
          for (const auto& itSetting : settings)
          {
            if (itSetting->IsVisible())
            {
              CVariant varSetting(CVariant::VariantTypeObject);
              if (!SerializeSetting(itSetting, varSetting))
                continue;

              result["settings"].push_back(varSetting);
            }
          }
        }
        found = true;

        if (doFilter)
          break;
      }
    }

    if (doFilter && !found)
      return InvalidParams;
  }

  return OK;
}

JSONRPC_STATUS CSettingsOperations::GetSettingValue(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  std::string settingId = parameterObject["setting"].asString();

  SettingPtr setting = CServiceBroker::GetSettingsComponent()->GetSettings()->GetSetting(settingId);
  if (setting == NULL ||
      !setting->IsVisible())
    return InvalidParams;

  CVariant value;
  switch (setting->GetType())
  {
  case SettingType::Boolean:
    value = std::static_pointer_cast<CSettingBool>(setting)->GetValue();
    break;

  case SettingType::Integer:
    value = std::static_pointer_cast<CSettingInt>(setting)->GetValue();
    break;

  case SettingType::Number:
    value = std::static_pointer_cast<CSettingNumber>(setting)->GetValue();
    break;

  case SettingType::String:
    value = std::static_pointer_cast<CSettingString>(setting)->GetValue();
    break;

  case SettingType::List:
  {
    SerializeSettingListValues(CServiceBroker::GetSettingsComponent()->GetSettings()->GetList(settingId), value);
    break;
  }

  case SettingType::Unknown:
  case SettingType::Action:
  default:
    return InvalidParams;
  }

  result["value"] = value;

  return OK;
}

JSONRPC_STATUS CSettingsOperations::SetSettingValue(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  std::string settingId = parameterObject["setting"].asString();
  CVariant value = parameterObject["value"];

  SettingPtr setting = CServiceBroker::GetSettingsComponent()->GetSettings()->GetSetting(settingId);
  if (setting == NULL ||
      !setting->IsVisible())
    return InvalidParams;

  switch (setting->GetType())
  {
  case SettingType::Boolean:
    if (!value.isBoolean())
      return InvalidParams;

    result = std::static_pointer_cast<CSettingBool>(setting)->SetValue(value.asBoolean());
    break;

  case SettingType::Integer:
    if (!value.isInteger() && !value.isUnsignedInteger())
      return InvalidParams;

    result = std::static_pointer_cast<CSettingInt>(setting)->SetValue((int)value.asInteger());
    break;

  case SettingType::Number:
    if (!value.isDouble())
      return InvalidParams;

    result = std::static_pointer_cast<CSettingNumber>(setting)->SetValue(value.asDouble());
    break;

  case SettingType::String:
    if (!value.isString())
      return InvalidParams;

    result = std::static_pointer_cast<CSettingString>(setting)->SetValue(value.asString());
    break;

  case SettingType::List:
  {
    if (!value.isArray())
      return InvalidParams;

    std::vector<CVariant> values;
    for (CVariant::const_iterator_array itValue = value.begin_array(); itValue != value.end_array(); ++itValue)
      values.push_back(*itValue);

    result = CServiceBroker::GetSettingsComponent()->GetSettings()->SetList(settingId, values);
    break;
  }

  case SettingType::Unknown:
  case SettingType::Action:
  default:
    return InvalidParams;
  }

  return OK;
}

JSONRPC_STATUS CSettingsOperations::ResetSettingValue(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  std::string settingId = parameterObject["setting"].asString();

  SettingPtr setting = CServiceBroker::GetSettingsComponent()->GetSettings()->GetSetting(settingId);
  if (setting == NULL ||
      !setting->IsVisible())
    return InvalidParams;

  switch (setting->GetType())
  {
  case SettingType::Boolean:
  case SettingType::Integer:
  case SettingType::Number:
  case SettingType::String:
  case SettingType::List:
    setting->Reset();
    break;

  case SettingType::Unknown:
  case SettingType::Action:
  default:
    return InvalidParams;
  }

  return ACK;
}

SettingLevel CSettingsOperations::ParseSettingLevel(const std::string &strLevel)
{
  if (StringUtils::EqualsNoCase(strLevel, "basic"))
    return SettingLevel::Basic;
  if (StringUtils::EqualsNoCase(strLevel, "advanced"))
    return SettingLevel::Advanced;
  if (StringUtils::EqualsNoCase(strLevel, "expert"))
    return SettingLevel::Expert;

  return SettingLevel::Standard;
}

bool CSettingsOperations::SerializeISetting(const std::shared_ptr<const ISetting>& setting,
                                            CVariant& obj)
{
  if (setting == NULL)
    return false;

  obj["id"] = setting->GetId();

  return true;
}

bool CSettingsOperations::SerializeSettingSection(
    const std::shared_ptr<const CSettingSection>& setting, CVariant& obj)
{
  if (!SerializeISetting(setting, obj))
    return false;

  obj["label"] = g_localizeStrings.Get(setting->GetLabel());
  if (setting->GetHelp() >= 0)
    obj["help"] = g_localizeStrings.Get(setting->GetHelp());

  return true;
}

bool CSettingsOperations::SerializeSettingCategory(
    const std::shared_ptr<const CSettingCategory>& setting, CVariant& obj)
{
  if (!SerializeISetting(setting, obj))
    return false;

  obj["label"] = g_localizeStrings.Get(setting->GetLabel());
  if (setting->GetHelp() >= 0)
    obj["help"] = g_localizeStrings.Get(setting->GetHelp());

  return true;
}

bool CSettingsOperations::SerializeSettingGroup(const std::shared_ptr<const CSettingGroup>& setting,
                                                CVariant& obj)
{
  return SerializeISetting(setting, obj);
}

bool CSettingsOperations::SerializeSetting(const std::shared_ptr<const CSetting>& setting,
                                           CVariant& obj)
{
  if (!SerializeISetting(setting, obj))
    return false;

  obj["label"] = g_localizeStrings.Get(setting->GetLabel());
  if (setting->GetHelp() >= 0)
    obj["help"] = g_localizeStrings.Get(setting->GetHelp());

  switch (setting->GetLevel())
  {
    case SettingLevel::Basic:
      obj["level"] = "basic";
      break;

    case SettingLevel::Standard:
      obj["level"] = "standard";
      break;

    case SettingLevel::Advanced:
      obj["level"] = "advanced";
      break;

    case SettingLevel::Expert:
      obj["level"] = "expert";
      break;

    default:
      return false;
  }

  obj["enabled"] = setting->IsEnabled();
  obj["parent"] = setting->GetParent();

  obj["control"] = CVariant(CVariant::VariantTypeObject);
  if (!SerializeSettingControl(setting->GetControl(), obj["control"]))
    return false;

  switch (setting->GetType())
  {
    case SettingType::Boolean:
      obj["type"] = "boolean";
      if (!SerializeSettingBool(std::static_pointer_cast<const CSettingBool>(setting), obj))
        return false;
      break;

    case SettingType::Integer:
      obj["type"] = "integer";
      if (!SerializeSettingInt(std::static_pointer_cast<const CSettingInt>(setting), obj))
        return false;
      break;

    case SettingType::Number:
      obj["type"] = "number";
      if (!SerializeSettingNumber(std::static_pointer_cast<const CSettingNumber>(setting), obj))
        return false;
      break;

    case SettingType::String:
      obj["type"] = "string";
      if (!SerializeSettingString(std::static_pointer_cast<const CSettingString>(setting), obj))
        return false;
      break;

    case SettingType::Action:
      obj["type"] = "action";
      if (!SerializeSettingAction(std::static_pointer_cast<const CSettingAction>(setting), obj))
        return false;
      break;

    case SettingType::List:
      obj["type"] = "list";
      if (!SerializeSettingList(std::static_pointer_cast<const CSettingList>(setting), obj))
        return false;
      break;

    default:
      return false;
  }

  return true;
}

bool CSettingsOperations::SerializeSettingBool(const std::shared_ptr<const CSettingBool>& setting,
                                               CVariant& obj)
{
  if (setting == NULL)
    return false;

  obj["value"] = setting->GetValue();
  obj["default"] = setting->GetDefault();

  return true;
}

bool CSettingsOperations::SerializeSettingInt(const std::shared_ptr<const CSettingInt>& setting,
                                              CVariant& obj)
{
  if (setting == NULL)
    return false;

  obj["default"] = setting->GetDefault();

  switch (setting->GetOptionsType())
  {
    case SettingOptionsType::StaticTranslatable:
    {
      obj["options"] = CVariant(CVariant::VariantTypeArray);
      const TranslatableIntegerSettingOptions& options = setting->GetTranslatableOptions();
      for (const auto& itOption : options)
      {
        CVariant varOption(CVariant::VariantTypeObject);
        varOption["label"] = g_localizeStrings.Get(itOption.label);
        varOption["value"] = itOption.value;
        obj["options"].push_back(varOption);
      }
      break;
    }

    case SettingOptionsType::Static:
    {
      obj["options"] = CVariant(CVariant::VariantTypeArray);
      const IntegerSettingOptions& options = setting->GetOptions();
      for (const auto& itOption : options)
      {
        CVariant varOption(CVariant::VariantTypeObject);
        varOption["label"] = itOption.label;
        varOption["value"] = itOption.value;
        obj["options"].push_back(varOption);
      }
      break;
    }

    case SettingOptionsType::Dynamic:
    {
      obj["options"] = CVariant(CVariant::VariantTypeArray);
      IntegerSettingOptions options = std::const_pointer_cast<CSettingInt>(setting)->UpdateDynamicOptions();
      for (const auto& itOption : options)
      {
        CVariant varOption(CVariant::VariantTypeObject);
        varOption["label"] = itOption.label;
        varOption["value"] = itOption.value;
        obj["options"].push_back(varOption);
      }
      break;
    }

    case SettingOptionsType::Unknown:
    default:
      obj["minimum"] = setting->GetMinimum();
      obj["step"] = setting->GetStep();
      obj["maximum"] = setting->GetMaximum();
      break;
  }

  // this must be done after potentially calling CSettingInt::UpdateDynamicOptions() because it can
  // change the value of the setting
  obj["value"] = setting->GetValue();

  return true;
}

bool CSettingsOperations::SerializeSettingNumber(
    const std::shared_ptr<const CSettingNumber>& setting, CVariant& obj)
{
  if (setting == NULL)
    return false;

  obj["value"] = setting->GetValue();
  obj["default"] = setting->GetDefault();

  obj["minimum"] = setting->GetMinimum();
  obj["step"] = setting->GetStep();
  obj["maximum"] = setting->GetMaximum();

  return true;
}

bool CSettingsOperations::SerializeSettingString(
    const std::shared_ptr<const CSettingString>& setting, CVariant& obj)
{
  if (setting == NULL)
    return false;

  obj["default"] = setting->GetDefault();

  obj["allowempty"] = setting->AllowEmpty();
  obj["allownewoption"] = setting->AllowNewOption();

  switch (setting->GetOptionsType())
  {
    case SettingOptionsType::StaticTranslatable:
    {
      obj["options"] = CVariant(CVariant::VariantTypeArray);
      const TranslatableStringSettingOptions& options = setting->GetTranslatableOptions();
      for (const auto& itOption : options)
      {
        CVariant varOption(CVariant::VariantTypeObject);
        varOption["label"] = g_localizeStrings.Get(itOption.first);
        varOption["value"] = itOption.second;
        obj["options"].push_back(varOption);
      }
      break;
    }

    case SettingOptionsType::Static:
    {
      obj["options"] = CVariant(CVariant::VariantTypeArray);
      const StringSettingOptions& options = setting->GetOptions();
      for (const auto& itOption : options)
      {
        CVariant varOption(CVariant::VariantTypeObject);
        varOption["label"] = itOption.label;
        varOption["value"] = itOption.value;
        obj["options"].push_back(varOption);
      }
      break;
    }

    case SettingOptionsType::Dynamic:
    {
      obj["options"] = CVariant(CVariant::VariantTypeArray);
      StringSettingOptions options = std::const_pointer_cast<CSettingString>(setting)->UpdateDynamicOptions();
      for (const auto& itOption : options)
      {
        CVariant varOption(CVariant::VariantTypeObject);
        varOption["label"] = itOption.label;
        varOption["value"] = itOption.value;
        obj["options"].push_back(varOption);
      }
      break;
    }

    case SettingOptionsType::Unknown:
    default:
      break;
  }

  // this must be done after potentially calling CSettingString::UpdateDynamicOptions() because it
  // can change the value of the setting
  obj["value"] = setting->GetValue();

  std::shared_ptr<const ISettingControl> control = setting->GetControl();
  if (control->GetFormat() == "path")
  {
    if (!SerializeSettingPath(std::static_pointer_cast<const CSettingPath>(setting), obj))
      return false;
  }
  if (control->GetFormat() == "addon")
  {
    if (!SerializeSettingAddon(std::static_pointer_cast<const CSettingAddon>(setting), obj))
      return false;
  }
  if (control->GetFormat() == "date")
  {
    if (!SerializeSettingDate(std::static_pointer_cast<const CSettingDate>(setting), obj))
      return false;
  }
  if (control->GetFormat() == "time")
  {
    if (!SerializeSettingTime(std::static_pointer_cast<const CSettingTime>(setting), obj))
      return false;
  }

  return true;
}

bool CSettingsOperations::SerializeSettingAction(
    const std::shared_ptr<const CSettingAction>& setting, CVariant& obj)
{
  if (setting == NULL)
    return false;

  obj["data"] = setting->GetData();

  return true;
}

bool CSettingsOperations::SerializeSettingList(const std::shared_ptr<const CSettingList>& setting,
                                               CVariant& obj)
{
  if (setting == NULL ||
      !SerializeSetting(setting->GetDefinition(), obj["definition"]))
    return false;

  SerializeSettingListValues(CSettingUtils::GetList(setting), obj["value"]);
  SerializeSettingListValues(CSettingUtils::ListToValues(setting, setting->GetDefault()), obj["default"]);

  obj["elementtype"] = obj["definition"]["type"];
  obj["delimiter"] = setting->GetDelimiter();
  obj["minimumItems"] = setting->GetMinimumItems();
  obj["maximumItems"] = setting->GetMaximumItems();

  return true;
}

bool CSettingsOperations::SerializeSettingPath(const std::shared_ptr<const CSettingPath>& setting,
                                               CVariant& obj)
{
  if (setting == NULL)
    return false;

  obj["type"] = "path";
  obj["writable"] = setting->Writable();
  obj["sources"] = setting->GetSources();

  return true;
}

bool CSettingsOperations::SerializeSettingAddon(const std::shared_ptr<const CSettingAddon>& setting,
                                                CVariant& obj)
{
  if (setting == NULL)
    return false;

  obj["type"] = "addon";
  obj["addontype"] = ADDON::CAddonInfo::TranslateType(setting->GetAddonType());

  return true;
}

bool CSettingsOperations::SerializeSettingDate(const std::shared_ptr<const CSettingDate>& setting,
                                               CVariant& obj)
{
  if (setting == NULL)
    return false;

  obj["type"] = "date";

  return true;
}

bool CSettingsOperations::SerializeSettingTime(const std::shared_ptr<const CSettingTime>& setting,
                                               CVariant& obj)
{
  if (setting == NULL)
    return false;

  obj["type"] = "time";

  return true;
}

bool CSettingsOperations::SerializeSettingControl(
    const std::shared_ptr<const ISettingControl>& control, CVariant& obj)
{
  if (control == NULL)
    return false;

  const std::string& type = control->GetType();
  obj["type"] = type;
  obj["format"] = control->GetFormat();
  obj["delayed"] = control->GetDelayed();

  if (type == "spinner")
  {
    std::shared_ptr<const CSettingControlSpinner> spinner = std::static_pointer_cast<const CSettingControlSpinner>(control);
    if (spinner->GetFormatLabel() >= 0)
      obj["formatlabel"] = g_localizeStrings.Get(spinner->GetFormatLabel());
    else if (!spinner->GetFormatString().empty() && spinner->GetFormatString() != "{:d}")
      obj["formatlabel"] = spinner->GetFormatString();
    if (spinner->GetMinimumLabel() >= 0)
      obj["minimumlabel"] = g_localizeStrings.Get(spinner->GetMinimumLabel());
  }
  else if (type == "edit")
  {
    std::shared_ptr<const CSettingControlEdit> edit = std::static_pointer_cast<const CSettingControlEdit>(control);
    obj["hidden"] = edit->IsHidden();
    obj["verifynewvalue"] = edit->VerifyNewValue();
    if (edit->GetHeading() >= 0)
      obj["heading"] = g_localizeStrings.Get(edit->GetHeading());
  }
  else if (type == "button")
  {
    std::shared_ptr<const CSettingControlButton> button = std::static_pointer_cast<const CSettingControlButton>(control);
    if (button->GetHeading() >= 0)
      obj["heading"] = g_localizeStrings.Get(button->GetHeading());
  }
  else if (type == "list")
  {
    std::shared_ptr<const CSettingControlList> list = std::static_pointer_cast<const CSettingControlList>(control);
    if (list->GetHeading() >= 0)
      obj["heading"] = g_localizeStrings.Get(list->GetHeading());
    obj["multiselect"] = list->CanMultiSelect();
  }
  else if (type == "slider")
  {
    std::shared_ptr<const CSettingControlSlider> slider = std::static_pointer_cast<const CSettingControlSlider>(control);
    if (slider->GetHeading() >= 0)
      obj["heading"] = g_localizeStrings.Get(slider->GetHeading());
    obj["popup"] = slider->UsePopup();
    if (slider->GetFormatLabel() >= 0)
      obj["formatlabel"] = g_localizeStrings.Get(slider->GetFormatLabel());
    else
      obj["formatlabel"] = slider->GetFormatString();
  }
  else if (type == "range")
  {
    std::shared_ptr<const CSettingControlRange> range = std::static_pointer_cast<const CSettingControlRange>(control);
    if (range->GetFormatLabel() >= 0)
      obj["formatlabel"] = g_localizeStrings.Get(range->GetFormatLabel());
    else
      obj["formatlabel"] = "";
    if (range->GetValueFormatLabel() >= 0)
      obj["formatvalue"] = g_localizeStrings.Get(range->GetValueFormatLabel());
    else
      obj["formatvalue"] = range->GetValueFormat();
  }
  else if (type != "toggle" && type != "label")
    return false;

  return true;
}

void CSettingsOperations::SerializeSettingListValues(const std::vector<CVariant> &values, CVariant &obj)
{
  obj = CVariant(CVariant::VariantTypeArray);
  for (const auto& itValue : values)
    obj.push_back(itValue);
}

JSONRPC_STATUS CSettingsOperations::GetSkinSettings(const std::string& method,
                                                    ITransportLayer* transport,
                                                    IClient* client,
                                                    const CVariant& parameterObject,
                                                    CVariant& result)
{
  const std::set<ADDON::CSkinSettingPtr> settings = CSkinSettings::GetInstance().GetSettings();
  CVariant varSettings(CVariant::VariantTypeArray);

  for (const auto& setting : settings)
  {
    CVariant varSetting(CVariant::VariantTypeObject);
    varSetting["id"] = setting->name;

    if (setting->GetType() == "bool")
    {
      varSetting["value"] = std::static_pointer_cast<ADDON::CSkinSettingBool>(setting)->value;
      varSetting["type"] = "boolean";
    }
    else if (setting->GetType() == "string")
    {
      varSetting["value"] = std::static_pointer_cast<ADDON::CSkinSettingString>(setting)->value;
      varSetting["type"] = setting->GetType();
    }
    else
      continue;

    varSettings.push_back(varSetting);
  }

  result["skin"] = CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(
      CSettings::SETTING_LOOKANDFEEL_SKIN);
  result["settings"] = varSettings;
  return OK;
}

JSONRPC_STATUS CSettingsOperations::GetSkinSettingValue(const std::string& method,
                                                        ITransportLayer* transport,
                                                        IClient* client,
                                                        const CVariant& parameterObject,
                                                        CVariant& result)
{
  const std::string settingId = parameterObject["setting"].asString();
  ADDON::CSkinSettingPtr setting = CSkinSettings::GetInstance().GetSetting(settingId);

  if (setting == nullptr)
    return InvalidParams;

  CVariant value;
  if (setting->GetType() == "string")
    value = std::static_pointer_cast<ADDON::CSkinSettingString>(setting)->value;
  else if (setting->GetType() == "bool")
    value = std::static_pointer_cast<ADDON::CSkinSettingBool>(setting)->value;
  else
    return InvalidParams;

  result["value"] = value;
  return OK;
}

JSONRPC_STATUS CSettingsOperations::SetSkinSettingValue(const std::string& method,
                                                        ITransportLayer* transport,
                                                        IClient* client,
                                                        const CVariant& parameterObject,
                                                        CVariant& result)
{
  const std::string settingId = parameterObject["setting"].asString();
  ADDON::CSkinSettingPtr setting = CSkinSettings::GetInstance().GetSetting(settingId);

  if (setting == nullptr)
    return InvalidParams;

  CVariant value = parameterObject["value"];
  if (setting->GetType() == "string")
  {
    if (!value.isString())
      return InvalidParams;

    result = std::static_pointer_cast<ADDON::CSkinSettingString>(setting)->value = value.asString();
  }
  else if (setting->GetType() == "bool")
  {
    if (!value.isBoolean())
      return InvalidParams;

    result = std::static_pointer_cast<ADDON::CSkinSettingBool>(setting)->value = value.asBoolean();
  }
  else
  {
    return InvalidParams;
  }

  return OK;
}
