/*
 *  Copyright (C) 2017-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Settings.h"

#include "settings/SettingsBase.h"
#include "settings/lib/Setting.h"

#include <algorithm>
#include <functional>
#include <utility>

namespace XBMCAddon
{
namespace xbmcaddon
{

template<class TSetting>
bool GetSettingValue(const std::shared_ptr<CSettingsBase>& settings,
                     const std::string& key,
                     typename TSetting::Value& value)
{
  if (key.empty() || !settings->IsLoaded())
    return false;

  auto setting = settings->GetSetting(key);
  if (setting == nullptr || setting->GetType() != TSetting::Type())
    return false;

  value = std::static_pointer_cast<TSetting>(setting)->GetValue();
  return true;
}

template<class TSetting>
bool GetSettingValueList(const std::shared_ptr<CSettingsBase>& settings,
                         const std::string& key,
                         std::function<typename TSetting::Value(CVariant)> transform,
                         std::vector<typename TSetting::Value>& values)
{
  if (key.empty() || !settings->IsLoaded())
    return false;

  auto setting = settings->GetSetting(key);
  if (setting == nullptr || setting->GetType() != SettingType::List ||
      std::static_pointer_cast<CSettingList>(setting)->GetElementType() != TSetting::Type())
    return false;

  const auto variantValues = settings->GetList(key);
  std::transform(variantValues.begin(), variantValues.end(), std::back_inserter(values), transform);
  return true;
}

template<class TSetting>
bool SetSettingValue(const std::shared_ptr<CSettingsBase>& settings,
                     const std::string& key,
                     typename TSetting::Value value)
{
  if (key.empty() || !settings->IsLoaded())
    return false;

  // try to get the setting
  auto setting = settings->GetSetting(key);
  if (setting == nullptr || setting->GetType() != TSetting::Type())
    return false;

  return std::static_pointer_cast<TSetting>(setting)->SetValue(value);
}

template<class TSetting>
bool SetSettingValueList(const std::shared_ptr<CSettingsBase>& settings,
                         const std::string& key,
                         const std::vector<typename TSetting::Value>& values)
{
  if (key.empty() || !settings->IsLoaded())
    return false;

  // try to get the setting
  auto setting = settings->GetSetting(key);
  if (setting == nullptr || setting->GetType() != SettingType::List ||
      std::static_pointer_cast<CSettingList>(setting)->GetElementType() != TSetting::Type())
    return false;

  std::vector<CVariant> variantValues;
  std::transform(values.begin(), values.end(), std::back_inserter(variantValues),
                 [](typename TSetting::Value value) { return CVariant(value); });

  return settings->SetList(key, variantValues);
}

Settings::Settings(std::shared_ptr<CSettingsBase> settings) : settings(std::move(settings))
{
}

bool Settings::getBool(const char* id) throw(XBMCAddon::WrongTypeException)
{
  bool value = false;
  if (!GetSettingValue<CSettingBool>(settings, id, value))
    throw XBMCAddon::WrongTypeException("Invalid setting type \"boolean\" for \"%s\"", id);

  return value;
}

int Settings::getInt(const char* id) throw(XBMCAddon::WrongTypeException)
{
  int value = 0;
  if (!GetSettingValue<CSettingInt>(settings, id, value))
    throw XBMCAddon::WrongTypeException("Invalid setting type \"integer\" for \"%s\"", id);

  return value;
}

double Settings::getNumber(const char* id) throw(XBMCAddon::WrongTypeException)
{
  double value = 0.0;
  if (!GetSettingValue<CSettingNumber>(settings, id, value))
    throw XBMCAddon::WrongTypeException("Invalid setting type \"number\" for \"%s\"", id);

  return value;
}

String Settings::getString(const char* id) throw(XBMCAddon::WrongTypeException)
{
  std::string value;
  if (!GetSettingValue<CSettingString>(settings, id, value))
    throw XBMCAddon::WrongTypeException("Invalid setting type \"string\" for \"%s\"", id);

  return value;
}

std::vector<bool> Settings::getBoolList(const char* id) throw(XBMCAddon::WrongTypeException)
{
  const auto transform = [](const CVariant& value) { return value.asBoolean(); };
  std::vector<bool> values;
  if (!GetSettingValueList<CSettingBool>(settings, id, transform, values))
    throw XBMCAddon::WrongTypeException("Invalid setting type \"list[boolean]\" for \"%s\"", id);

  return values;
}

std::vector<int> Settings::getIntList(const char* id) throw(XBMCAddon::WrongTypeException)
{
  const auto transform = [](const CVariant& value) { return value.asInteger32(); };
  std::vector<int> values;
  if (!GetSettingValueList<CSettingInt>(settings, id, transform, values))
    throw XBMCAddon::WrongTypeException("Invalid setting type \"list[integer]\" for \"%s\"", id);

  return values;
}

std::vector<double> Settings::getNumberList(const char* id) throw(XBMCAddon::WrongTypeException)
{
  const auto transform = [](const CVariant& value) { return value.asDouble(); };
  std::vector<double> values;
  if (!GetSettingValueList<CSettingNumber>(settings, id, transform, values))
    throw XBMCAddon::WrongTypeException("Invalid setting type \"list[number]\" for \"%s\"", id);

  return values;
}

std::vector<String> Settings::getStringList(const char* id) throw(XBMCAddon::WrongTypeException)
{
  const auto transform = [](const CVariant& value) { return value.asString(); };
  std::vector<std::string> values;
  if (!GetSettingValueList<CSettingString>(settings, id, transform, values))
    throw XBMCAddon::WrongTypeException("Invalid setting type \"list[string]\" for \"%s\"", id);

  return values;
}

void Settings::setBool(const char* id, bool value) throw(XBMCAddon::WrongTypeException)
{
  if (!SetSettingValue<CSettingBool>(settings, id, value))
    throw XBMCAddon::WrongTypeException("Invalid setting type \"boolean\" for \"%s\"", id);
  settings->Save();
}

void Settings::setInt(const char* id, int value) throw(XBMCAddon::WrongTypeException)
{
  if (!SetSettingValue<CSettingInt>(settings, id, value))
    throw XBMCAddon::WrongTypeException("Invalid setting type \"integer\" for \"%s\"", id);
  settings->Save();
}

void Settings::setNumber(const char* id, double value) throw(XBMCAddon::WrongTypeException)
{
  if (!SetSettingValue<CSettingNumber>(settings, id, value))
    throw XBMCAddon::WrongTypeException("Invalid setting type \"number\" for \"%s\"", id);
  settings->Save();
}

void Settings::setString(const char* id, const String& value) throw(XBMCAddon::WrongTypeException)
{
  if (!SetSettingValue<CSettingString>(settings, id, value))
    throw XBMCAddon::WrongTypeException("Invalid setting type \"string\" for \"%s\"", id);
  settings->Save();
}

void Settings::setBoolList(const char* id,
                           const std::vector<bool>& values) throw(XBMCAddon::WrongTypeException)
{
  if (!SetSettingValueList<CSettingBool>(settings, id, values))
    throw XBMCAddon::WrongTypeException("Invalid setting type \"list[boolean]\" for \"%s\"", id);
  settings->Save();
}

void Settings::setIntList(const char* id,
                          const std::vector<int>& values) throw(XBMCAddon::WrongTypeException)
{
  if (!SetSettingValueList<CSettingInt>(settings, id, values))
    throw XBMCAddon::WrongTypeException("Invalid setting type \"list[integer]\" for \"%s\"", id);
  settings->Save();
}

void Settings::setNumberList(const char* id,
                             const std::vector<double>& values) throw(XBMCAddon::WrongTypeException)
{
  if (!SetSettingValueList<CSettingNumber>(settings, id, values))
    throw XBMCAddon::WrongTypeException("Invalid setting type \"list[number]\" for \"%s\"", id);
  settings->Save();
}

void Settings::setStringList(const char* id,
                             const std::vector<String>& values) throw(XBMCAddon::WrongTypeException)
{
  if (!SetSettingValueList<CSettingString>(settings, id, values))
    throw XBMCAddon::WrongTypeException("Invalid setting type \"list[string]\" for \"%s\"", id);
  settings->Save();
}

} // namespace xbmcaddon
} // namespace XBMCAddon
