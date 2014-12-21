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

std::vector<CVariant> CSettingUtils::GetList(const CSettingList *settingList)
{
  return ListToValues(settingList, settingList->GetValue());
}

bool CSettingUtils::SetList(CSettingList *settingList, const std::vector<CVariant> &value)
{
  SettingPtrList newValues;
  if (!ValuesToList(settingList, value, newValues))
    return false;

  return settingList->SetValue(newValues);
}

std::vector<CVariant> CSettingUtils::ListToValues(const CSettingList *setting, const std::vector< std::shared_ptr<CSetting> > &values)
{
  std::vector<CVariant> realValues;

  if (setting == NULL)
    return realValues;

  for (SettingPtrList::const_iterator it = values.begin(); it != values.end(); ++it)
  {
    switch (setting->GetElementType())
    {
      case SettingTypeBool:
        realValues.push_back(static_cast<const CSettingBool*>(it->get())->GetValue());
        break;

      case SettingTypeInteger:
        realValues.push_back(static_cast<const CSettingInt*>(it->get())->GetValue());
        break;

      case SettingTypeNumber:
        realValues.push_back(static_cast<const CSettingNumber*>(it->get())->GetValue());
        break;

      case SettingTypeString:
        realValues.push_back(static_cast<const CSettingString*>(it->get())->GetValue());
        break;

      default:
        break;
    }
  }

  return realValues;
}

bool CSettingUtils::ValuesToList(const CSettingList *setting, const std::vector<CVariant> &values,
                                 std::vector< std::shared_ptr<CSetting> > &newValues)
{
  if (setting == NULL)
    return false;

  int index = 0;
  bool ret = true;
  for (std::vector<CVariant>::const_iterator itValue = values.begin(); itValue != values.end(); ++itValue)
  {
    CSetting *settingValue = setting->GetDefinition()->Clone(StringUtils::Format("%s.%d", setting->GetId().c_str(), index++));
    if (settingValue == NULL)
      return false;

    switch (setting->GetElementType())
    {
      case SettingTypeBool:
        if (!itValue->isBoolean())
          ret = false;
        else
          ret = static_cast<CSettingBool*>(settingValue)->SetValue(itValue->asBoolean());
        break;

      case SettingTypeInteger:
        if (!itValue->isInteger())
          ret = false;
        else
          ret = static_cast<CSettingInt*>(settingValue)->SetValue((int)itValue->asInteger());
        break;

      case SettingTypeNumber:
        if (!itValue->isDouble())
          ret = false;
        else
          ret = static_cast<CSettingNumber*>(settingValue)->SetValue(itValue->asDouble());
        break;

      case SettingTypeString:
        if (!itValue->isString())
          ret = false;
        else
          ret = static_cast<CSettingString*>(settingValue)->SetValue(itValue->asString());
        break;

      default:
        ret = false;
        break;
    }

    if (!ret)
    {
      delete settingValue;
      return false;
    }

    newValues.push_back(SettingPtr(settingValue));
  }

  return true;
}
