/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "utils/Variant.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#define SETTING_XML_ROOT "settings"
#define SETTING_XML_ROOT_VERSION "version"

#define SETTING_XML_ELM_SECTION "section"
#define SETTING_XML_ELM_CATEGORY "category"
#define SETTING_XML_ELM_GROUP "group"
#define SETTING_XML_ELM_SETTING "setting"
#define SETTING_XML_ELM_VISIBLE "visible"
#define SETTING_XML_ELM_REQUIREMENT "requirement"
#define SETTING_XML_ELM_CONDITION "condition"
#define SETTING_XML_ELM_ENABLED "enable"
#define SETTING_XML_ELM_LEVEL "level"
#define SETTING_XML_ELM_DEFAULT "default"
#define SETTING_XML_ELM_VALUE "value"
#define SETTING_XML_ELM_CONTROL "control"
#define SETTING_XML_ELM_CONSTRAINTS "constraints"
#define SETTING_XML_ELM_OPTIONS "options"
#define SETTING_XML_ELM_OPTION "option"
#define SETTING_XML_ELM_MINIMUM "minimum"
#define SETTING_XML_ELM_STEP "step"
#define SETTING_XML_ELM_MAXIMUM "maximum"
#define SETTING_XML_ELM_ALLOWEMPTY "allowempty"
#define SETTING_XML_ELM_ALLOWNEWOPTION "allownewoption"
#define SETTING_XML_ELM_DEPENDENCIES "dependencies"
#define SETTING_XML_ELM_DEPENDENCY "dependency"
#define SETTING_XML_ELM_UPDATES "updates"
#define SETTING_XML_ELM_UPDATE "update"
#define SETTING_XML_ELM_ACCESS "access"
#define SETTING_XML_ELM_DELIMITER "delimiter"
#define SETTING_XML_ELM_MINIMUM_ITEMS "minimumitems"
#define SETTING_XML_ELM_MAXIMUM_ITEMS "maximumitems"
#define SETTING_XML_ELM_DATA "data"

#define SETTING_XML_ATTR_ID "id"
#define SETTING_XML_ATTR_REFERENCE "ref"
#define SETTING_XML_ATTR_LABEL "label"
#define SETTING_XML_ATTR_HELP "help"
#define SETTING_XML_ATTR_TYPE "type"
#define SETTING_XML_ATTR_PARENT "parent"
#define SETTING_XML_ATTR_FORMAT "format"
#define SETTING_XML_ATTR_DELAYED "delayed"
#define SETTING_XML_ATTR_ON "on"
#define SETTING_XML_ATTR_OPERATOR "operator"
#define SETTING_XML_ATTR_NAME "name"
#define SETTING_XML_ATTR_SETTING "setting"
#define SETTING_XML_ATTR_BEFORE "before"
#define SETTING_XML_ATTR_AFTER "after"

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
using IntegerSettingOptionsFiller = void (*)(const std::shared_ptr<const CSetting>& setting,
                                             IntegerSettingOptions& list,
                                             int& current,
                                             void* data);
using StringSettingOptionsFiller = void (*)(const std::shared_ptr<const CSetting>& setting,
                                            StringSettingOptions& list,
                                            std::string& current,
                                            void* data);

enum class SettingOptionsSort
{
  NoSorting,
  Ascending,
  Descending
};
