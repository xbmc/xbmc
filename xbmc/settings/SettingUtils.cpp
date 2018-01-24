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

#include "SettingUtils.h"
#include "settings/lib/Setting.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"

std::vector<CVariant> CSettingUtils::GetList(std::shared_ptr<const CSettingList> settingList)
{
  return ListToValues(settingList, settingList->GetValue());
}

bool CSettingUtils::SetList(std::shared_ptr<CSettingList> settingList, const std::vector<CVariant> &value)
{
  SettingList newValues;
  if (!ValuesToList(settingList, value, newValues))
    return false;

  return settingList->SetValue(newValues);
}

std::vector<CVariant> CSettingUtils::ListToValues(std::shared_ptr<const CSettingList> setting, const std::vector< std::shared_ptr<CSetting> > &values)
{
  std::vector<CVariant> realValues;

  if (setting == NULL)
    return realValues;

  for (const auto& value : values)
  {
    switch (setting->GetElementType())
    {
      case SettingType::Boolean:
        realValues.push_back(std::static_pointer_cast<const CSettingBool>(value)->GetValue());
        break;

      case SettingType::Integer:
        realValues.push_back(std::static_pointer_cast<const CSettingInt>(value)->GetValue());
        break;

      case SettingType::Number:
        realValues.push_back(std::static_pointer_cast<const CSettingNumber>(value)->GetValue());
        break;

      case SettingType::String:
        realValues.push_back(std::static_pointer_cast<const CSettingString>(value)->GetValue());
        break;

      default:
        break;
    }
  }

  return realValues;
}

bool CSettingUtils::ValuesToList(std::shared_ptr<const CSettingList> setting, const std::vector<CVariant> &values,
                                 std::vector< std::shared_ptr<CSetting> > &newValues)
{
  if (setting == NULL)
    return false;

  int index = 0;
  bool ret = true;
  for (const auto& value : values)
  {
    SettingPtr settingValue = setting->GetDefinition()->Clone(StringUtils::Format("%s.%d", setting->GetId().c_str(), index++));
    if (settingValue == NULL)
      return false;

    switch (setting->GetElementType())
    {
      case SettingType::Boolean:
        if (!value.isBoolean())
          ret = false;
        else
          ret = std::static_pointer_cast<CSettingBool>(settingValue)->SetValue(value.asBoolean());
        break;

      case SettingType::Integer:
        if (!value.isInteger())
          ret = false;
        else
          ret = std::static_pointer_cast<CSettingInt>(settingValue)->SetValue(static_cast<int>(value.asInteger()));
        break;

      case SettingType::Number:
        if (!value.isDouble())
          ret = false;
        else
          ret = std::static_pointer_cast<CSettingNumber>(settingValue)->SetValue(value.asDouble());
        break;

      case SettingType::String:
        if (!value.isString())
          ret = false;
        else
          ret = std::static_pointer_cast<CSettingString>(settingValue)->SetValue(value.asString());
        break;

      default:
        ret = false;
        break;
    }

    if (!ret)
      return false;

    newValues.push_back(std::const_pointer_cast<CSetting>(settingValue));
  }

  return true;
}
