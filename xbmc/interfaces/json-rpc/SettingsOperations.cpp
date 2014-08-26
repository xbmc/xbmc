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

#include "SettingsOperations.h"
#include "addons/Addon.h"
#include "settings/SettingAddon.h"
#include "settings/SettingControl.h"
#include "settings/SettingPath.h"
#include "settings/Settings.h"
#include "settings/SettingUtils.h"
#include "settings/lib/Setting.h"
#include "settings/lib/SettingSection.h"
#include "guilib/LocalizeStrings.h"
#include "utils/StringUtils.h"

using namespace std;
using namespace JSONRPC;

JSONRPC_STATUS CSettingsOperations::GetSections(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  SettingLevel level = (SettingLevel)ParseSettingLevel(parameterObject["level"].asString());
  bool listCategories = !parameterObject["properties"].empty() && parameterObject["properties"][0].asString() == "categories";

  result["sections"] = CVariant(CVariant::VariantTypeArray);

  // apply the level filter
  vector<CSettingSection*> allSections = CSettings::Get().GetSections();
  for (vector<CSettingSection*>::const_iterator itSection = allSections.begin(); itSection != allSections.end(); ++itSection)
  {
    SettingCategoryList categories = (*itSection)->GetCategories(level);
    if (categories.empty())
      continue;

    CVariant varSection(CVariant::VariantTypeObject);
    if (!SerializeSettingSection(*itSection, varSection))
      continue;

    if (listCategories)
    {
      varSection["categories"] = CVariant(CVariant::VariantTypeArray);
      for (SettingCategoryList::const_iterator itCategory = categories.begin(); itCategory != categories.end(); ++itCategory)
      {
        CVariant varCategory(CVariant::VariantTypeObject);
        if (!SerializeSettingCategory(*itCategory, varCategory))
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
  SettingLevel level = (SettingLevel)ParseSettingLevel(parameterObject["level"].asString());
  std::string strSection = parameterObject["section"].asString();
  bool listSettings = !parameterObject["properties"].empty() && parameterObject["properties"][0].asString() == "settings";

  vector<CSettingSection*> sections;
  if (!strSection.empty())
  {
    CSettingSection *section = CSettings::Get().GetSection(strSection);
    if (section == NULL)
      return InvalidParams;

    sections.push_back(section);
  }
  else
    sections = CSettings::Get().GetSections();

  result["categories"] = CVariant(CVariant::VariantTypeArray);

  for (vector<CSettingSection*>::const_iterator itSection = sections.begin(); itSection != sections.end(); ++itSection)
  {
    SettingCategoryList categories = (*itSection)->GetCategories(level);
    for (SettingCategoryList::const_iterator itCategory = categories.begin(); itCategory != categories.end(); ++itCategory)
    {
      CVariant varCategory(CVariant::VariantTypeObject);
      if (!SerializeSettingCategory(*itCategory, varCategory))
        continue;

      if (listSettings)
      {
        varCategory["groups"] = CVariant(CVariant::VariantTypeArray);

        SettingGroupList groups = (*itCategory)->GetGroups(level);
        for (SettingGroupList::const_iterator itGroup = groups.begin(); itGroup != groups.end(); ++itGroup)
        {
          CVariant varGroup(CVariant::VariantTypeObject);
          if (!SerializeSettingGroup(*itGroup, varGroup))
            continue;

          varGroup["settings"] = CVariant(CVariant::VariantTypeArray);
          SettingList settings = (*itGroup)->GetSettings(level);
          for (SettingList::const_iterator itSetting = settings.begin(); itSetting != settings.end(); ++itSetting)
          {
            if ((*itSetting)->IsVisible())
            {
              CVariant varSetting(CVariant::VariantTypeObject);
              if (!SerializeSetting(*itSetting, varSetting))
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
  SettingLevel level = (SettingLevel)ParseSettingLevel(parameterObject["level"].asString());
  const CVariant &filter = parameterObject["filter"];
  bool doFilter = filter.isObject() && filter.isMember("section") && filter.isMember("category");
  string strSection, strCategory;
  if (doFilter)
  {
    strSection = filter["section"].asString();
    strCategory = filter["category"].asString();
  }
 
  vector<CSettingSection*> sections;

  if (doFilter)
  {
    CSettingSection *section = CSettings::Get().GetSection(strSection);
    if (section == NULL)
      return InvalidParams;

    sections.push_back(section);
  }
  else
    sections = CSettings::Get().GetSections();

  result["settings"] = CVariant(CVariant::VariantTypeArray);

  for (vector<CSettingSection*>::const_iterator itSection = sections.begin(); itSection != sections.end(); ++itSection)
  {
    SettingCategoryList categories = (*itSection)->GetCategories(level);
    bool found = !doFilter;
    for (SettingCategoryList::const_iterator itCategory = categories.begin(); itCategory != categories.end(); ++itCategory)
    {
      if (!doFilter || StringUtils::EqualsNoCase((*itCategory)->GetId(), strCategory))
      {
        SettingGroupList groups = (*itCategory)->GetGroups(level);
        for (SettingGroupList::const_iterator itGroup = groups.begin(); itGroup != groups.end(); ++itGroup)
        {
          SettingList settings = (*itGroup)->GetSettings(level);
          for (SettingList::const_iterator itSetting = settings.begin(); itSetting != settings.end(); ++itSetting)
          {
            if ((*itSetting)->IsVisible())
            {
              CVariant varSetting(CVariant::VariantTypeObject);
              if (!SerializeSetting(*itSetting, varSetting))
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
  string settingId = parameterObject["setting"].asString();

  CSetting* setting = CSettings::Get().GetSetting(settingId);
  if (setting == NULL ||
      !setting->IsVisible())
    return InvalidParams;

  CVariant value;
  switch (setting->GetType())
  {
  case SettingTypeBool:
    value = static_cast<CSettingBool*>(setting)->GetValue();
    break;

  case SettingTypeInteger:
    value = static_cast<CSettingInt*>(setting)->GetValue();
    break;

  case SettingTypeNumber:
    value = static_cast<CSettingNumber*>(setting)->GetValue();
    break;

  case SettingTypeString:
    value = static_cast<CSettingString*>(setting)->GetValue();
    break;

  case SettingTypeList:
  {
    SerializeSettingListValues(CSettings::Get().GetList(settingId), value);
    break;
  }

  case SettingTypeNone:
  case SettingTypeAction:
  default:
    return InvalidParams;
  }

  result["value"] = value;

  return OK;
}

JSONRPC_STATUS CSettingsOperations::SetSettingValue(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  string settingId = parameterObject["setting"].asString();
  CVariant value = parameterObject["value"];

  CSetting* setting = CSettings::Get().GetSetting(settingId);
  if (setting == NULL ||
      !setting->IsVisible())
    return InvalidParams;

  switch (setting->GetType())
  {
  case SettingTypeBool:
    if (!value.isBoolean())
      return InvalidParams;

    result = static_cast<CSettingBool*>(setting)->SetValue(value.asBoolean());
    break;

  case SettingTypeInteger:
    if (!value.isInteger() && !value.isUnsignedInteger())
      return InvalidParams;

    result = static_cast<CSettingInt*>(setting)->SetValue((int)value.asInteger());
    break;

  case SettingTypeNumber:
    if (!value.isDouble())
      return InvalidParams;

    result = static_cast<CSettingNumber*>(setting)->SetValue(value.asDouble());
    break;

  case SettingTypeString:
    if (!value.isString())
      return InvalidParams;

    result = static_cast<CSettingString*>(setting)->SetValue(value.asString());
    break;

  case SettingTypeList:
  {
    if (!value.isArray())
      return InvalidParams;

    std::vector<CVariant> values;
    for (CVariant::const_iterator_array itValue = value.begin_array(); itValue != value.end_array(); ++itValue)
      values.push_back(*itValue);

    result = CSettings::Get().SetList(settingId, values);
    break;
  }

  case SettingTypeNone:
  case SettingTypeAction:
  default:
    return InvalidParams;
  }

  return OK;
}

JSONRPC_STATUS CSettingsOperations::ResetSettingValue(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  string settingId = parameterObject["setting"].asString();

  CSetting* setting = CSettings::Get().GetSetting(settingId);
  if (setting == NULL ||
      !setting->IsVisible())
    return InvalidParams;

  switch (setting->GetType())
  {
  case SettingTypeBool:
  case SettingTypeInteger:
  case SettingTypeNumber:
  case SettingTypeString:
  case SettingTypeList:
    setting->Reset();
    break;

  case SettingTypeNone:
  case SettingTypeAction:
  default:
    return InvalidParams;
  }

  return ACK;
}

int CSettingsOperations::ParseSettingLevel(const std::string &strLevel)
{
  if (StringUtils::EqualsNoCase(strLevel, "basic"))
    return SettingLevelBasic;
  if (StringUtils::EqualsNoCase(strLevel, "advanced"))
    return SettingLevelAdvanced;
  if (StringUtils::EqualsNoCase(strLevel, "expert"))
    return SettingLevelExpert;

  return SettingLevelStandard;
}

bool CSettingsOperations::SerializeISetting(const ISetting* setting, CVariant &obj)
{
  if (setting == NULL)
    return false;

  obj["id"] = setting->GetId();

  return true;
}

bool CSettingsOperations::SerializeSettingSection(const CSettingSection* setting, CVariant &obj)
{
  if (!SerializeISetting(setting, obj))
    return false;

  obj["label"] = g_localizeStrings.Get(setting->GetLabel());
  if (setting->GetHelp() >= 0)
    obj["help"] = g_localizeStrings.Get(setting->GetHelp());

  return true;
}

bool CSettingsOperations::SerializeSettingCategory(const CSettingCategory* setting, CVariant &obj)
{
  if (!SerializeISetting(setting, obj))
    return false;

  obj["label"] = g_localizeStrings.Get(setting->GetLabel());
  if (setting->GetHelp() >= 0)
    obj["help"] = g_localizeStrings.Get(setting->GetHelp());

  return true;
}

bool CSettingsOperations::SerializeSettingGroup(const CSettingGroup* setting, CVariant &obj)
{
  return SerializeISetting(setting, obj);
}

bool CSettingsOperations::SerializeSetting(const CSetting* setting, CVariant &obj)
{
  if (!SerializeISetting(setting, obj))
    return false;

  obj["label"] = g_localizeStrings.Get(setting->GetLabel());
  if (setting->GetHelp() >= 0)
    obj["help"] = g_localizeStrings.Get(setting->GetHelp());

  switch (setting->GetLevel())
  {
    case SettingLevelBasic:
      obj["level"] = "basic";
      break;

    case SettingLevelStandard:
      obj["level"] = "standard";
      break;

    case SettingLevelAdvanced:
      obj["level"] = "advanced";
      break;

    case SettingLevelExpert:
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
    case SettingTypeBool:
      obj["type"] = "boolean";
      if (!SerializeSettingBool(static_cast<const CSettingBool*>(setting), obj))
        return false;
      break;

    case SettingTypeInteger:
      obj["type"] = "integer";
      if (!SerializeSettingInt(static_cast<const CSettingInt*>(setting), obj))
        return false;
      break;

    case SettingTypeNumber:
      obj["type"] = "number";
      if (!SerializeSettingNumber(static_cast<const CSettingNumber*>(setting), obj))
        return false;
      break;

    case SettingTypeString:
      obj["type"] = "string";
      if (!SerializeSettingString(static_cast<const CSettingString*>(setting), obj))
        return false;
      break;

    case SettingTypeAction:
      obj["type"] = "action";
      if (!SerializeSettingAction(static_cast<const CSettingAction*>(setting), obj))
        return false;
      break;

    case SettingTypeList:
      obj["type"] = "list";
      if (!SerializeSettingList(static_cast<const CSettingList*>(setting), obj))
        return false;
      break;

    default:
      return false;
  }

  return true;
}

bool CSettingsOperations::SerializeSettingBool(const CSettingBool* setting, CVariant &obj)
{
  if (setting == NULL)
    return false;

  obj["value"] = setting->GetValue();
  obj["default"] = setting->GetDefault();

  return true;
}

bool CSettingsOperations::SerializeSettingInt(const CSettingInt* setting, CVariant &obj)
{
  if (setting == NULL)
    return false;

  obj["value"] = setting->GetValue();
  obj["default"] = setting->GetDefault();

  switch (setting->GetOptionsType())
  {
    case SettingOptionsTypeStatic:
    {
      obj["options"] = CVariant(CVariant::VariantTypeArray);
      const StaticIntegerSettingOptions& options = setting->GetOptions();
      for (StaticIntegerSettingOptions::const_iterator itOption = options.begin(); itOption != options.end(); ++itOption)
      {
        CVariant varOption(CVariant::VariantTypeObject);
        varOption["label"] = g_localizeStrings.Get(itOption->first);
        varOption["value"] = itOption->second;
        obj["options"].push_back(varOption);
      }
      break;
    }

    case SettingOptionsTypeDynamic:
    {
      obj["options"] = CVariant(CVariant::VariantTypeArray);
      DynamicIntegerSettingOptions options = const_cast<CSettingInt*>(setting)->UpdateDynamicOptions();
      for (DynamicIntegerSettingOptions::const_iterator itOption = options.begin(); itOption != options.end(); ++itOption)
      {
        CVariant varOption(CVariant::VariantTypeObject);
        varOption["label"] = itOption->first;
        varOption["value"] = itOption->second;
        obj["options"].push_back(varOption);
      }
      break;
    }

    case SettingOptionsTypeNone:
    default:
      obj["minimum"] = setting->GetMinimum();
      obj["step"] = setting->GetStep();
      obj["maximum"] = setting->GetMaximum();
      break;
  }

  return true;
}

bool CSettingsOperations::SerializeSettingNumber(const CSettingNumber* setting, CVariant &obj)
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

bool CSettingsOperations::SerializeSettingString(const CSettingString* setting, CVariant &obj)
{
  if (setting == NULL)
    return false;

  obj["value"] = setting->GetValue();
  obj["default"] = setting->GetDefault();

  obj["allowempty"] = setting->AllowEmpty();

  if (setting->GetOptionsType() == SettingOptionsTypeDynamic)
  {
    obj["options"] = CVariant(CVariant::VariantTypeArray);
    DynamicStringSettingOptions options = const_cast<CSettingString*>(setting)->UpdateDynamicOptions();
    for (DynamicStringSettingOptions::const_iterator itOption = options.begin(); itOption != options.end(); ++itOption)
    {
      CVariant varOption(CVariant::VariantTypeObject);
      varOption["label"] = itOption->first;
      varOption["value"] = itOption->second;
      obj["options"].push_back(varOption);
    }
  }

  const ISettingControl* control = setting->GetControl();
  if (control->GetFormat() == "path")
  {
    if (!SerializeSettingPath(static_cast<const CSettingPath*>(setting), obj))
      return false;
  }
  if (control->GetFormat() == "addon")
  {
    if (!SerializeSettingAddon(static_cast<const CSettingAddon*>(setting), obj))
      return false;
  }

  return true;
}

bool CSettingsOperations::SerializeSettingAction(const CSettingAction* setting, CVariant &obj)
{
  if (setting == NULL)
    return false;

  return true;
}

bool CSettingsOperations::SerializeSettingList(const CSettingList* setting, CVariant &obj)
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

bool CSettingsOperations::SerializeSettingPath(const CSettingPath* setting, CVariant &obj)
{
  if (setting == NULL)
    return false;

  obj["type"] = "path";
  obj["writable"] = setting->Writable();
  obj["sources"] = setting->GetSources();

  return true;
}

bool CSettingsOperations::SerializeSettingAddon(const CSettingAddon* setting, CVariant &obj)
{
  if (setting == NULL)
    return false;

  obj["type"] = "addon";
  obj["addontype"] = ADDON::TranslateType(setting->GetAddonType());

  return true;
}

bool CSettingsOperations::SerializeSettingControl(const ISettingControl* control, CVariant &obj)
{
  if (control == NULL)
    return false;
  
  const std::string& type = control->GetType();
  obj["type"] = type;
  obj["format"] = control->GetFormat();
  obj["delayed"] = control->GetDelayed();

  if (type == "spinner")
  {
    const CSettingControlSpinner* spinner = static_cast<const CSettingControlSpinner*>(control);
    if (spinner == NULL)
      return false;

    if (spinner->GetFormatLabel() >= 0)
      obj["formatlabel"] = g_localizeStrings.Get(spinner->GetFormatLabel());
    else if (!spinner->GetFormatString().empty() && spinner->GetFormatString() != "%i")
      obj["formatlabel"] = spinner->GetFormatString();
    if (spinner->GetMinimumLabel() >= 0)
      obj["minimumlabel"] = g_localizeStrings.Get(spinner->GetMinimumLabel());
  }
  else if (type == "edit")
  {
    const CSettingControlEdit* edit = static_cast<const CSettingControlEdit*>(control);
    if (edit == NULL)
      return false;

    obj["hidden"] = edit->IsHidden();
    obj["verifynewvalue"] = edit->VerifyNewValue();
    if (edit->GetHeading() >= 0)
      obj["heading"] = g_localizeStrings.Get(edit->GetHeading());
  }
  else if (type == "button")
  {
    const CSettingControlButton* button = static_cast<const CSettingControlButton*>(control);
    if (button == NULL)
      return false;

    if (button->GetHeading() >= 0)
      obj["heading"] = g_localizeStrings.Get(button->GetHeading());
  }
  else if (type == "list")
  {
    const CSettingControlList* list = static_cast<const CSettingControlList*>(control);
    if (list == NULL)
      return false;

    if (list->GetHeading() >= 0)
      obj["heading"] = g_localizeStrings.Get(list->GetHeading());
    obj["multiselect"] = list->CanMultiSelect();
  }
  else if (type == "slider")
  {
    const CSettingControlSlider* slider = static_cast<const CSettingControlSlider*>(control);
    if (slider == NULL)
      return false;

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
    const CSettingControlRange* range = static_cast<const CSettingControlRange*>(control);
    if (range == NULL)
      return false;

    if (range->GetFormatLabel() >= 0)
      obj["formatlabel"] = g_localizeStrings.Get(range->GetFormatLabel());
    else
      obj["formatlabel"] = "";
    if (range->GetValueFormatLabel() >= 0)
      obj["formatvalue"] = g_localizeStrings.Get(range->GetValueFormatLabel());
    else
      obj["formatvalue"] = range->GetValueFormat();
  }
  else if (type != "toggle")
    return false;

  return true;
}

void CSettingsOperations::SerializeSettingListValues(const std::vector<CVariant> &values, CVariant &obj)
{
  obj = CVariant(CVariant::VariantTypeArray);
  for (std::vector<CVariant>::const_iterator itValue = values.begin(); itValue != values.end(); ++itValue)
    obj.push_back(*itValue);
}
