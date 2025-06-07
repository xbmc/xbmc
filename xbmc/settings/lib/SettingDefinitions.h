/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "utils/Variant.h"

#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

constexpr const char* SETTING_XML_ROOT = "settings";
constexpr const char* SETTING_XML_ROOT_VERSION = "version";

constexpr const char* SETTING_XML_ELM_SECTION = "section";
constexpr const char* SETTING_XML_ELM_CATEGORY = "category";
constexpr const char* SETTING_XML_ELM_GROUP = "group";
constexpr const char* SETTING_XML_ELM_SETTING = "setting";
constexpr const char* SETTING_XML_ELM_VISIBLE = "visible";
constexpr const char* SETTING_XML_ELM_REQUIREMENT = "requirement";
constexpr const char* SETTING_XML_ELM_CONDITION = "condition";
constexpr const char* SETTING_XML_ELM_ENABLED = "enable";
constexpr const char* SETTING_XML_ELM_LEVEL = "level";
constexpr const char* SETTING_XML_ELM_DEFAULT = "default";
constexpr const char* SETTING_XML_ELM_VALUE = "value";
constexpr const char* SETTING_XML_ELM_CONTROL = "control";
constexpr const char* SETTING_XML_ELM_CONSTRAINTS = "constraints";
constexpr const char* SETTING_XML_ELM_OPTIONS = "options";
constexpr const char* SETTING_XML_ELM_OPTION = "option";
constexpr const char* SETTING_XML_ELM_MINIMUM = "minimum";
constexpr const char* SETTING_XML_ELM_STEP = "step";
constexpr const char* SETTING_XML_ELM_MAXIMUM = "maximum";
constexpr const char* SETTING_XML_ELM_ALLOWEMPTY = "allowempty";
constexpr const char* SETTING_XML_ELM_ALLOWNEWOPTION = "allownewoption";
constexpr const char* SETTING_XML_ELM_DEPENDENCIES = "dependencies";
constexpr const char* SETTING_XML_ELM_DEPENDENCY = "dependency";
constexpr const char* SETTING_XML_ELM_UPDATES = "updates";
constexpr const char* SETTING_XML_ELM_UPDATE = "update";
constexpr const char* SETTING_XML_ELM_ACCESS = "access";
constexpr const char* SETTING_XML_ELM_DELIMITER = "delimiter";
constexpr const char* SETTING_XML_ELM_MINIMUM_ITEMS = "minimumitems";
constexpr const char* SETTING_XML_ELM_MAXIMUM_ITEMS = "maximumitems";
constexpr const char* SETTING_XML_ELM_DATA = "data";

constexpr const char* SETTING_XML_ATTR_ID = "id";
constexpr const char* SETTING_XML_ATTR_REFERENCE = "ref";
constexpr const char* SETTING_XML_ATTR_LABEL = "label";
constexpr const char* SETTING_XML_ATTR_HELP = "help";
constexpr const char* SETTING_XML_ATTR_TYPE = "type";
constexpr const char* SETTING_XML_ATTR_PARENT = "parent";
constexpr const char* SETTING_XML_ATTR_FORMAT = "format";
constexpr const char* SETTING_XML_ATTR_DELAYED = "delayed";
constexpr const char* SETTING_XML_ATTR_ON = "on";
constexpr const char* SETTING_XML_ATTR_OPERATOR = "operator";
constexpr const char* SETTING_XML_ATTR_NAME = "name";
constexpr const char* SETTING_XML_ATTR_SETTING = "setting";
constexpr const char* SETTING_XML_ATTR_BEFORE = "before";
constexpr const char* SETTING_XML_ATTR_AFTER = "after";

struct IntegerSettingOption
{
  IntegerSettingOption(const std::string& _label, int _value)
  : label(_label), value(_value) {}

  IntegerSettingOption(const std::string& _label,
                       const std::string& _label2,
                       int _value,
                       const std::vector<std::pair<std::string, CVariant>>& props)
    : label(_label), label2(_label2), value(_value), properties(props)
  {
  }

  std::string label;
  std::string label2;
  int value = 0;
  std::vector<std::pair<std::string, CVariant>> properties;
};

struct StringSettingOption
{
  StringSettingOption(const std::string& _label, const std::string& _value)
  : label(_label), value(_value) {}

  StringSettingOption(const std::string& _label,
                      const std::string& _label2,
                      const std::string& _value,
                      const std::vector<std::pair<std::string, CVariant>>& props)
    : label(_label), label2(_label2), value(_value), properties(props)
  {
  }

  std::string label;
  std::string label2;
  std::string value;
  std::vector<std::pair<std::string, CVariant>> properties;
};

struct TranslatableIntegerSettingOption
{
  TranslatableIntegerSettingOption() = default;
  TranslatableIntegerSettingOption(int _label, int _value, const std::string& _addonId = "")
    : label(_label), value(_value), addonId(_addonId)
  {
  }

  int label = 0;
  int value = 0;
  std::string addonId; // Leaved empty for Kodi labels
};

using TranslatableIntegerSettingOptions = std::vector<TranslatableIntegerSettingOption>;
using IntegerSettingOptions = std::vector<IntegerSettingOption>;
using TranslatableStringSettingOption = std::pair<int, std::string>;
using TranslatableStringSettingOptions = std::vector<TranslatableStringSettingOption>;
using StringSettingOptions = std::vector<StringSettingOption>;

class CSetting;
using IntegerSettingOptionsFiller = std::function<void(
    const std::shared_ptr<const CSetting>& setting, IntegerSettingOptions& list, int& current)>;
using StringSettingOptionsFiller =
    std::function<void(const std::shared_ptr<const CSetting>& setting,
                       StringSettingOptions& list,
                       std::string& current)>;

enum class SettingOptionsSort
{
  NoSorting,
  Ascending,
  Descending
};
