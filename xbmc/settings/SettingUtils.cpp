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

  for (SettingList::const_iterator it = values.begin(); it != values.end(); ++it)
  {
    switch (setting->GetElementType())
    {
      case SettingTypeBool:
        realValues.push_back(std::static_pointer_cast<const CSettingBool>(*it)->GetValue());
        break;

      case SettingTypeInteger:
        realValues.push_back(std::static_pointer_cast<const CSettingInt>(*it)->GetValue());
        break;

      case SettingTypeNumber:
        realValues.push_back(std::static_pointer_cast<const CSettingNumber>(*it)->GetValue());
        break;

      case SettingTypeString:
        realValues.push_back(std::static_pointer_cast<const CSettingString>(*it)->GetValue());
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
  for (std::vector<CVariant>::const_iterator itValue = values.begin(); itValue != values.end(); ++itValue)
  {
    SettingPtr settingValue = setting->GetDefinition()->Clone(StringUtils::Format("%s.%d", setting->GetId().c_str(), index++));
    if (settingValue == NULL)
      return false;

    switch (setting->GetElementType())
    {
      case SettingTypeBool:
        if (!itValue->isBoolean())
          ret = false;
        else
          ret = std::static_pointer_cast<CSettingBool>(settingValue)->SetValue(itValue->asBoolean());
        break;

      case SettingTypeInteger:
        if (!itValue->isInteger())
          ret = false;
        else
          ret = std::static_pointer_cast<CSettingInt>(settingValue)->SetValue((int)itValue->asInteger());
        break;

      case SettingTypeNumber:
        if (!itValue->isDouble())
          ret = false;
        else
          ret = std::static_pointer_cast<CSettingNumber>(settingValue)->SetValue(itValue->asDouble());
        break;

      case SettingTypeString:
        if (!itValue->isString())
          ret = false;
        else
          ret = std::static_pointer_cast<CSettingString>(settingValue)->SetValue(itValue->asString());
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
