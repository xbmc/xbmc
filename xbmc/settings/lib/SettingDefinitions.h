#pragma once
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

#include <memory>
#include <string>
#include <utility>
#include <vector>

#define SETTING_XML_ROOT              "settings"
#define SETTING_XML_ROOT_VERSION      "version"

#define SETTING_XML_ELM_SECTION       "section"
#define SETTING_XML_ELM_CATEGORY      "category"
#define SETTING_XML_ELM_GROUP         "group"
#define SETTING_XML_ELM_SETTING       "setting"
#define SETTING_XML_ELM_VISIBLE       "visible"
#define SETTING_XML_ELM_REQUIREMENT   "requirement"
#define SETTING_XML_ELM_CONDITION     "condition"
#define SETTING_XML_ELM_LEVEL         "level"
#define SETTING_XML_ELM_DEFAULT       "default"
#define SETTING_XML_ELM_VALUE         "value"
#define SETTING_XML_ELM_CONTROL       "control"
#define SETTING_XML_ELM_CONSTRAINTS   "constraints"
#define SETTING_XML_ELM_OPTIONS       "options"
#define SETTING_XML_ELM_OPTION        "option"
#define SETTING_XML_ELM_MINIMUM       "minimum"
#define SETTING_XML_ELM_STEP          "step"
#define SETTING_XML_ELM_MAXIMUM       "maximum"
#define SETTING_XML_ELM_ALLOWEMPTY    "allowempty"
#define SETTING_XML_ELM_DEPENDENCIES  "dependencies"
#define SETTING_XML_ELM_DEPENDENCY    "dependency"
#define SETTING_XML_ELM_UPDATES       "updates"
#define SETTING_XML_ELM_UPDATE        "update"
#define SETTING_XML_ELM_ACCESS        "access"
#define SETTING_XML_ELM_DELIMITER     "delimiter"
#define SETTING_XML_ELM_MINIMUM_ITEMS "minimumitems"
#define SETTING_XML_ELM_MAXIMUM_ITEMS "maximumitems"
#define SETTING_XML_ELM_DATA          "data"

#define SETTING_XML_ATTR_ID           "id"
#define SETTING_XML_ATTR_LABEL        "label"
#define SETTING_XML_ATTR_HELP         "help"
#define SETTING_XML_ATTR_TYPE         "type"
#define SETTING_XML_ATTR_PARENT       "parent"
#define SETTING_XML_ATTR_FORMAT       "format"
#define SETTING_XML_ATTR_DELAYED      "delayed"
#define SETTING_XML_ATTR_ON           "on"
#define SETTING_XML_ATTR_OPERATOR     "operator"
#define SETTING_XML_ATTR_NAME         "name"
#define SETTING_XML_ATTR_SETTING      "setting"
#define SETTING_XML_ATTR_BEFORE       "before"
#define SETTING_XML_ATTR_AFTER        "after"

typedef std::pair<int, int> TranslatableIntegerSettingOption;
typedef std::vector<TranslatableIntegerSettingOption> TranslatableIntegerSettingOptions;
typedef std::pair<std::string, int> IntegerSettingOption;
typedef std::vector<IntegerSettingOption> IntegerSettingOptions;
typedef std::pair<int, std::string> TranslatableStringSettingOption;
typedef std::vector<TranslatableStringSettingOption> TranslatableStringSettingOptions;
typedef std::pair<std::string, std::string> StringSettingOption;
typedef std::vector<StringSettingOption> StringSettingOptions;

class CSetting;
typedef void (*IntegerSettingOptionsFiller)(std::shared_ptr<const CSetting> setting, IntegerSettingOptions &list, int &current, void *data);
typedef void (*StringSettingOptionsFiller)(std::shared_ptr<const CSetting> setting, StringSettingOptions &list, std::string &current, void *data);
