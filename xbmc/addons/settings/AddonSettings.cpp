/*
 *      Copyright (C) 2017 Team XBMC
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

#include <algorithm>
#include <vector>

#include "AddonSettings.h"
#include "FileItem.h"
#include "GUIInfoManager.h"
#include "addons/Addon.h"
#include "addons/settings/GUIDialogAddonSettings.h"
#include "addons/settings/SettingUrlEncodedString.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "guilib/LocalizeStrings.h"
#include "messaging/ApplicationMessenger.h"
#include "settings/AdvancedSettings.h"
#include "settings/MediaSourceSettings.h"
#include "settings/SettingAddon.h"
#include "settings/SettingControl.h"
#include "settings/SettingDateTime.h"
#include "settings/SettingPath.h"
#include "settings/lib/Setting.h"
#include "settings/lib/SettingSection.h"
#include "settings/lib/SettingsManager.h"
#include "storage/MediaManager.h"
#include "threads/SingleLock.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/XBMCTinyXML.h"
#include "utils/XMLUtils.h"

static const std::string OldSettingValuesSeparator = "|";

static const int UnknownSettingLabelIdStart = 100000;

bool InfoBool(const std::string &condition, const std::string &value, SettingConstPtr setting, void *data)
{
  return g_infoManager.EvaluateBool(value);
}

template<class TSetting>
SettingPtr InitializeFromOldSettingWithoutDefinition(ADDON::CAddonSettings& settings, const std::string& settingId, typename TSetting::Value defaultValue)
{
  std::shared_ptr<TSetting> setting = std::make_shared<TSetting>(settingId, settings.GetSettingsManager());
  setting->SetLevel(SettingLevel::Internal);
  setting->SetVisible(false);
  setting->SetDefault(defaultValue);

  return setting;
}

template<>
SettingPtr InitializeFromOldSettingWithoutDefinition<CSettingString>(ADDON::CAddonSettings& settings, const std::string& settingId, typename CSettingString::Value defaultValue)
{
  std::shared_ptr<CSettingString> setting = std::make_shared<CSettingString>(settingId, settings.GetSettingsManager());
  setting->SetLevel(SettingLevel::Internal);
  setting->SetVisible(false);
  setting->SetDefault(defaultValue);
  setting->SetAllowEmpty(true);

  return setting;
}

template<class TSetting>
SettingPtr AddSettingWithoutDefinition(ADDON::CAddonSettings& settings, const std::string& settingId, typename TSetting::Value defaultValue)
{
  if (settingId.empty())
    return nullptr;

  // if necessary try to initialize the settings manager on-the-fly without any definitions
  if (!settings.IsInitialized() && !settings.Initialize(CXBMCTinyXML(), true))
  {
    CLog::Log(LOGWARNING, "CAddonSettings[%s]: failed to initialize settings on-the-fly", settings.GetAddon()->ID().c_str());
    return nullptr;
  }

  // check if we need to add a section on-the-fly
  auto sections = settings.GetSettingsManager()->GetSections();
  SettingSectionPtr section;
  if (sections.empty())
    section = std::make_shared<CSettingSection>(settings.GetAddon()->ID(), settings.GetSettingsManager());
  else
    section = sections.back();

  // check if we need to add a category on-the-fly
  auto categories = section->GetCategories();
  SettingCategoryPtr category;
  if (categories.empty())
    category = std::make_shared<CSettingCategory>("category0", settings.GetSettingsManager());
  else
    category = categories.back();

  // check if we need to add a group on-the-fly
  auto groups = category->GetGroups();
  SettingGroupPtr group;
  if (groups.empty())
    group = std::make_shared<CSettingGroup>("0", settings.GetSettingsManager());
  else
    group = groups.back();

  // create a new setting on-the-fly
  auto setting = InitializeFromOldSettingWithoutDefinition<TSetting>(settings, settingId, defaultValue);
  if (setting == nullptr)
  {
    CLog::Log(LOGWARNING, "CAddonSettings[%s]: failed to create setting \"%s\" on-the-fly", settings.GetAddon()->ID().c_str(), settingId.c_str());
    return nullptr;
  }

  // add the setting (and if necessary the section, category and/or group)
  if (!settings.GetSettingsManager()->AddSetting(setting, section, category, group))
  {
    CLog::Log(LOGWARNING, "CAddonSettings[%s]: failed to add setting \"%s\" on-the-fly", settings.GetAddon()->ID().c_str(), settingId.c_str());
    return nullptr;
  }

  return setting;
}

namespace ADDON
{
CAddonSettings::CAddonSettings(std::shared_ptr<const IAddon> addon)
  : CSettingsBase()
  , m_addon(addon)
  , m_addonId(addon->ID())
  , m_addonPath(addon->Path())
  , m_addonProfile(addon->Profile())
  , m_unidentifiedSettingId(0)
  , m_unknownSettingLabelId(UnknownSettingLabelIdStart)
{ }

std::shared_ptr<CSetting> CAddonSettings::CreateSetting(const std::string &settingType, const std::string &settingId, CSettingsManager *settingsManager /* = NULL */) const
{
  if (StringUtils::EqualsNoCase(settingType, "urlencodedstring"))
    return std::make_shared<CSettingUrlEncodedString>(settingId, settingsManager);

  return CSettingCreator::CreateSetting(settingType, settingId, settingsManager);
}

void CAddonSettings::OnSettingAction(std::shared_ptr<const CSetting> setting)
{
  std::string actionData;
  bool closeDialog = false;

  // check if it's an action setting
  if (setting->GetType() == SettingType::Action)
  {
    auto settingAction = std::dynamic_pointer_cast<const CSettingAction>(setting);
    if (settingAction != nullptr && settingAction->HasData())
      actionData = settingAction->GetData();
  }

  // check if the setting control's is a button and its format is action
  if (setting->GetControl()->GetType() == "button" && setting->GetControl()->GetFormat() == "action")
  {
    auto controlButton = std::dynamic_pointer_cast<const CSettingControlButton>(setting->GetControl());
    if (controlButton != nullptr)
    {
      if (actionData.empty() && controlButton->HasActionData())
        actionData = controlButton->GetActionData();

      closeDialog = controlButton->CloseDialog();
    }
  }

  if (actionData.empty())
    return;

  if (closeDialog)
    CGUIDialogAddonSettings::SaveAndClose();

  KODI::MESSAGING::CApplicationMessenger::GetInstance().SendMsg(TMSG_EXECUTE_BUILT_IN, -1, -1, nullptr, actionData);
}

bool CAddonSettings::Initialize(const CXBMCTinyXML& doc, bool allowEmpty /* = false */)
{
  CSingleLock lock(m_critical);
  if (m_initialized)
    return false;

  // register custom setting types
  InitializeSettingTypes();
  // register custom setting controls
  InitializeControls();

  // conditions need to be initialized before the setting definitions
  InitializeConditions();

  // load the settings definitions
  if (!InitializeDefinitions(doc) && !allowEmpty)
    return false;

  GetSettingsManager()->SetInitialized();

  m_initialized = true;

  return true;
}

bool CAddonSettings::Load(const CXBMCTinyXML& doc)
{
  CSingleLock lock(m_critical);
  if (!m_initialized)
    return false;

  CLog::Log(LOGDEBUG, "CAddonSettings[%s]: loading setting values",
    m_addonId.c_str());

  // figure out the version of the setting definitions
  uint32_t version = 0;
  if (!ParseSettingVersion(doc, version))
  {
    CLog::Log(LOGERROR, "CAddonSettings[%s]: failed to determine setting values version",
      m_addonId.c_str());
    return false;
  }

  std::map<std::string, std::string> settingValues;

  // for new/"normal" setting values use the standard process
  if (version != 0)
  {
    bool updated;
    if (!LoadValuesFromXml(doc, updated))
      return false;

    // helper lambda for parsing a setting's ID and value from XML
    auto parseSettingValue = [&settingValues](const TiXmlNode* setting, const std::string& categoryId = "")
    {
      // put together the setting ID
      auto settingId = categoryId;
      if (!settingId.empty())
        settingId += ".";
      auto id = setting->ToElement()->Attribute("id");
      if (id)
        settingId += id;

      // parse the setting value
      std::string settingValue;
      if (setting->FirstChild())
        settingValue = setting->FirstChild()->ValueStr();

      // add the setting to the map
      settingValues.emplace(std::make_pair(settingId, settingValue));
    };

    // check if there were any setting values without a definition
    auto category = doc.RootElement()->FirstChild();
    while (category != nullptr)
    {
      // check if this really is a category with setting elements
      if (category->FirstChild() && category->FirstChild()->Type() == CXBMCTinyXML::TINYXML_ELEMENT)
      {
        const auto categoryId = category->ValueStr();
        auto setting = category->FirstChild();
        while (setting != nullptr)
        {
          parseSettingValue(setting, categoryId);

          setting = setting->NextSibling();
        }
      }
      else
        parseSettingValue(category);

      category = category->NextSibling();
    }
  }
  // for old setting values do it manually
  else if (!LoadOldSettingValues(doc, settingValues))
  {
    CLog::Log(LOGERROR, "CAddonSettings[%s]: failed to determine setting values from old format",
      m_addonId.c_str());
    return false;
  }

  // process all settings
  for (const auto& setting : settingValues)
  {
    // ignore setting values without a setting identifier
    if (setting.first.empty())
      continue;

    // try to find a matching setting
    SettingPtr newSetting = GetSetting(setting.first);
    if (newSetting == nullptr)
    {
      CLog::Log(LOGDEBUG, "CAddonSettings[%s]: failed to find definition for setting %s. Creating a setting on-the-fly...",
        m_addonId.c_str(), setting.first.c_str());

      // create a hidden/internal string setting on-the-fly
      newSetting = AddSettingWithoutDefinition<CSettingString>(*this, setting.first, setting.second);
    }

    // try to load the old setting value
    if (!newSetting->FromString(setting.second))
    {
      CLog::Log(LOGWARNING, "CAddonSettings[%s]: failed to load value \"%s\" for setting %s",
        m_addonId.c_str(), setting.second.c_str(), setting.first.c_str());
    }
  }

  SetLoaded();

  return true;
}

bool CAddonSettings::Save(CXBMCTinyXML& doc) const
{
  CSingleLock lock(m_critical);
  if (!m_initialized)
    return false;

  if (!SaveValuesToXml(doc))
  {
    CLog::Log(LOGERROR, "CAddonSettings[%s]: failed to save settings", m_addonId.c_str());
    return false;
  }

  return true;
}

bool CAddonSettings::HasSettings() const
{
  return IsInitialized() && !GetSettingsManager()->GetSections().empty();
}

std::string CAddonSettings::GetSettingLabel(int label) const
{
  if (label < UnknownSettingLabelIdStart || label >= m_unknownSettingLabelId)
    return "";

  const auto labelIt = m_unknownSettingLabels.find(label);
  if (labelIt == m_unknownSettingLabels.end())
    return "";

  return labelIt->second;
}

std::shared_ptr<CSetting> CAddonSettings::AddSetting(const std::string& settingId, bool value)
{
  return AddSettingWithoutDefinition<CSettingBool>(*this, settingId, value);
}

std::shared_ptr<CSetting> CAddonSettings::AddSetting(const std::string& settingId, int value)
{
  return AddSettingWithoutDefinition<CSettingInt>(*this, settingId, value);
}

std::shared_ptr<CSetting> CAddonSettings::AddSetting(const std::string& settingId, double value)
{
  return AddSettingWithoutDefinition<CSettingNumber>(*this, settingId, value);
}

std::shared_ptr<CSetting> CAddonSettings::AddSetting(const std::string& settingId, const std::string& value)
{
  return AddSettingWithoutDefinition<CSettingString>(*this, settingId, value);
}

void CAddonSettings::InitializeSettingTypes()
{
  GetSettingsManager()->RegisterSettingType("addon", this);
  GetSettingsManager()->RegisterSettingType("date", this);
  GetSettingsManager()->RegisterSettingType("path", this);
  GetSettingsManager()->RegisterSettingType("time", this);
  GetSettingsManager()->RegisterSettingType("urlencodedstring", this);
}

void CAddonSettings::InitializeControls()
{
  GetSettingsManager()->RegisterSettingControl("toggle", this);
  GetSettingsManager()->RegisterSettingControl("spinner", this);
  GetSettingsManager()->RegisterSettingControl("edit", this);
  GetSettingsManager()->RegisterSettingControl("button", this);
  GetSettingsManager()->RegisterSettingControl("list", this);
  GetSettingsManager()->RegisterSettingControl("slider", this);
  GetSettingsManager()->RegisterSettingControl("range", this);
  GetSettingsManager()->RegisterSettingControl("title", this);
}

void CAddonSettings::InitializeConditions()
{
  GetSettingsManager()->AddCondition("InfoBool", InfoBool);
}

bool CAddonSettings::InitializeDefinitions(const CXBMCTinyXML& doc)
{
  CLog::Log(LOGDEBUG, "CAddonSettings[%s]: loading setting definitions", m_addonId.c_str());

  // figure out the version of the setting definitions
  uint32_t version = 0;
  if (!ParseSettingVersion(doc, version))
  {
    CLog::Log(LOGERROR, "CAddonSettings[%s]: failed to determine setting definitions version", m_addonId.c_str());
    return false;
  }

  // for new/"normal" setting definitions use the standard process
  if (version != 0)
    return InitializeDefinitionsFromXml(doc);

  // for old setting definitions do it manually
  return InitializeFromOldSettingDefinitions(doc);
}

bool CAddonSettings::ParseSettingVersion(const CXBMCTinyXML& doc, uint32_t& version) const
{
  const TiXmlElement* root = doc.RootElement();
  if (root == nullptr)
    return false;

  if (!StringUtils::EqualsNoCase(root->ValueStr(), SETTING_XML_ROOT))
  {
    CLog::Log(LOGERROR, "CAddonSettings[%s]: error reading setting definitions: no <settings> tag", m_addonId.c_str());
    return false;
  }

  version = GetSettingsManager()->ParseVersion(root);
  return true;
}

std::shared_ptr<CSettingGroup> CAddonSettings::ParseOldSettingElement(const TiXmlElement* categoryElement, std::shared_ptr<CSettingCategory> category, std::set<std::string>& actionSettings)
{
    // build a vector of settings from the same category
    std::vector<std::shared_ptr<const CSetting>> categorySettings;
    std::set<std::string> settingIds;

    // prepare for settings with enable/visible conditions
    struct SettingWithConditions {
      SettingPtr setting;
      std::string enableCondition;
      std::string visibleCondition;
      SettingDependencies deps;
    };
    std::vector<SettingWithConditions> settingsWithConditions;

    auto group = std::make_shared<CSettingGroup>("0", GetSettingsManager());
    uint32_t groupId = 1;

    // go through all settings in the category
    const TiXmlElement* settingElement = categoryElement->FirstChildElement("setting");
    while (settingElement != nullptr)
    {
      // read the possible attributes
      const auto settingType = XMLUtils::GetAttribute(settingElement, "type");
      const auto settingId = XMLUtils::GetAttribute(settingElement, "id");
      const auto defaultValue = XMLUtils::GetAttribute(settingElement, "default");
      const auto settingValues = XMLUtils::GetAttribute(settingElement, "values");
      const auto settingLValues = StringUtils::Split(XMLUtils::GetAttribute(settingElement, "lvalues"), OldSettingValuesSeparator);
      int settingLabel = -1;
      bool settingLabelParsed = ParseOldLabel(settingElement, settingId, settingLabel);

      SettingPtr setting;
      if (settingType == "sep" || settingType == "lsep")
      {
        // check if we need to create a new group
        if (!group->GetSettings().empty())
        {
          // add the current group to the category
          category->AddGroup(group);

          // and create a new one
          group.reset(new CSettingGroup(StringUtils::Format("%u", groupId), GetSettingsManager()));
          groupId += 1;
        }

        if (settingType == "lsep" && settingLabelParsed)
          group->SetLabel(settingLabel);
      }
      else if (settingId.empty() || settingType == "action")
      {
        if (settingType == "action")
        {
          setting = InitializeFromOldSettingAction(settingId, settingElement, defaultValue);
          actionSettings.insert(setting->GetId());
        }
        else
          setting = InitializeFromOldSettingLabel();
      }
      else if (settingType == "bool")
        setting = InitializeFromOldSettingBool(settingId, settingElement, defaultValue);
      else if (settingType == "text" || settingType == "ipaddress")
        setting = InitializeFromOldSettingTextIpAddress(settingId, settingType, settingElement, defaultValue, settingLabel);
      else if (settingType == "number")
        setting = InitializeFromOldSettingNumber(settingId, settingElement, defaultValue, settingLabel);
      else if (settingType == "video" || settingType == "audio" ||
                settingType == "image" || settingType == "executable" ||
                settingType == "file" || settingType == "folder")
        setting = InitializeFromOldSettingPath(settingId, settingType, settingElement, defaultValue, settingLabel);
      else if (settingType == "date")
        setting = InitializeFromOldSettingDate(settingId, settingElement, defaultValue, settingLabel);
      else if (settingType == "time")
        setting = InitializeFromOldSettingTime(settingId, settingElement, defaultValue, settingLabel);
      else if (settingType == "select")
        setting = InitializeFromOldSettingSelect(settingId, settingElement, defaultValue, settingLabel, settingValues, settingLValues);
      else if (settingType == "addon")
        setting = InitializeFromOldSettingAddon(settingId, settingElement, defaultValue, settingLabel);
      else if (settingType == "enum" || settingType == "labelenum")
        setting = InitializeFromOldSettingEnums(settingId, settingType, settingElement, defaultValue, settingValues, settingLValues);
      else if (settingType == "fileenum")
        setting = InitializeFromOldSettingFileEnum(settingId, settingElement, defaultValue, settingValues);
      else if (settingType == "rangeofnum")
        setting = InitializeFromOldSettingRangeOfNum(settingId, settingElement, defaultValue);
      else if (settingType == "slider")
        setting = InitializeFromOldSettingSlider(settingId, settingElement, defaultValue);
      else if (settingType.empty())
      {
        // setting definitions without a type are considered as "text" / strings but are hidden
        setting = InitializeFromOldSettingTextIpAddress(settingId, "text", settingElement, defaultValue, settingLabel);
        setting->SetLevel(SettingLevel::Internal);
      }
      else
      {
        CLog::Log(LOGWARNING, "CAddonSettings[%s]: failed to parse old setting definition for \"%s\" of type \"%s\"",
          m_addonId.c_str(), settingId.c_str(), settingType.c_str());
      }

      // process general properties
      if (setting != nullptr)
      {
        // use the setting's ID if there's no label
        if (settingLabel < 0)
        {
          settingLabel = m_unknownSettingLabelId;
          m_unknownSettingLabelId += 1;

          m_unknownSettingLabels.emplace(settingLabel, settingId);
        }

        // set the setting's label
        setting->SetLabel(settingLabel);

        // handle subsettings
        bool isSubsetting = false;
        if (settingElement->QueryBoolAttribute("subsetting", &isSubsetting) == TIXML_SUCCESS && isSubsetting)
        {
          // find the last non-subsetting in the current group and use that as the parent setting
          const auto groupSettings = group->GetSettings();
          const auto parentSetting = std::find_if(groupSettings.crbegin(), groupSettings.crend(),
            [](SettingConstPtr setting)
            {
              return setting->GetParent().empty();
            });

          if (parentSetting != groupSettings.crend())
          {
            if ((*parentSetting)->GetType() == SettingType::Reference)
              setting->SetParent(std::static_pointer_cast<const CSettingReference>(*parentSetting)->GetReferencedId());
            else
              setting->SetParent((*parentSetting)->GetId());
          }
        }

        SettingWithConditions settingWithConditions = {
          setting
        };

        // parse enable status
        const auto conditionEnable = XMLUtils::GetAttribute(settingElement, "enable");
        if (StringUtils::EqualsNoCase(conditionEnable, "true"))
          setting->SetEnabled(true);
        else if (StringUtils::EqualsNoCase(conditionEnable, "false"))
          setting->SetEnabled(false);
        else if (!conditionEnable.empty())
          settingWithConditions.enableCondition = conditionEnable;

        // parse visible status
        const auto conditionVisible = XMLUtils::GetAttribute(settingElement, "visible");
        if (StringUtils::EqualsNoCase(conditionVisible, "true"))
          setting->SetVisible(true);
        else if (StringUtils::EqualsNoCase(conditionVisible, "false"))
          setting->SetVisible(false);
        else if (!conditionVisible.empty())
          settingWithConditions.visibleCondition = conditionVisible;

        if (!settingWithConditions.enableCondition.empty() || !settingWithConditions.visibleCondition.empty())
          settingsWithConditions.push_back(settingWithConditions);

        // check if there already is a setting with the setting identifier
        if (settingIds.find(settingId) != settingIds.end())
        {
          // turn the setting into a reference setting
          setting = std::make_shared<CSettingReference>(settingId, GetSettingsManager());
        }

        // add the setting's identifier to the list of all identifiers
        settingIds.insert(setting->GetId());

        // add the setting to the list of settings from the same category
        categorySettings.push_back(setting);

        // add the setting to the current group
        group->AddSetting(setting);
      }
      else
      {
        // add a dummy setting for the group / separator to the list of settings from the same category
        categorySettings.push_back(nullptr);
      }

      // look for the next setting
      settingElement = settingElement->NextSiblingElement("setting");
    }

    // process settings with enable/visible conditions
    for (auto setting : settingsWithConditions) {
      if (!setting.enableCondition.empty())
      {
        CSettingDependency dependencyEnable(SettingDependencyType::Enable, GetSettingsManager());
        if (ParseOldCondition(setting.setting, categorySettings, setting.enableCondition, dependencyEnable))
          setting.deps.push_back(dependencyEnable);
        else
        {
          CLog::Log(LOGWARNING, "CAddonSettings[%s]: failed to parse enable condition \"%s\" of old setting definition for \"%s\"",
            m_addonId.c_str(), setting.enableCondition.c_str(), setting.setting->GetId().c_str());
        }
      }

      if (!setting.visibleCondition.empty())
      {
        CSettingDependency dependencyVisible(SettingDependencyType::Visible, GetSettingsManager());
        if (ParseOldCondition(setting.setting, categorySettings, setting.visibleCondition, dependencyVisible))
          setting.deps.push_back(dependencyVisible);
        else
        {
          CLog::Log(LOGWARNING, "CAddonSettings[%s]: failed to parse visible condition \"%s\" of old setting definition for \"%s\"",
            m_addonId.c_str(), setting.visibleCondition.c_str(), setting.setting->GetId().c_str());
        }
      }

      // set dependencies
      setting.setting->SetDependencies(setting.deps);
    }

    return group;
}

std::shared_ptr<CSettingCategory> CAddonSettings::ParseOldCategoryElement(uint32_t &categoryId, const TiXmlElement * categoryElement, std::set<std::string> &actionSettings)
{
  // create the category
  auto category = std::make_shared<CSettingCategory>(StringUtils::Format("category%u", categoryId), GetSettingsManager());
  categoryId += 1;

  // try to get the category's label and fall back to "General"
  int categoryLabel = 128;
  ParseOldLabel(categoryElement, g_localizeStrings.Get(categoryLabel), categoryLabel);
  category->SetLabel(categoryLabel);

  // prepare a setting group
  auto group = ParseOldSettingElement(categoryElement, category, actionSettings);

  // add the group to the category
  category->AddGroup(group);

  return category;
}

bool CAddonSettings::InitializeFromOldSettingDefinitions(const CXBMCTinyXML& doc)
{
  CLog::Log(LOGDEBUG, "CAddonSettings[%s]: trying to load setting definitions from old format...", m_addonId.c_str());

  const TiXmlElement* root = doc.RootElement();
  if (root == nullptr)
    return false;

  std::shared_ptr<CSettingSection> section = std::make_shared<CSettingSection>(m_addonId, GetSettingsManager());

  std::shared_ptr<CSettingCategory> category;
  uint32_t categoryId = 0;
  std::set<std::string> actionSettings;


  // Special case for no category settings
  section->AddCategory(ParseOldCategoryElement(categoryId, root, actionSettings));

  const TiXmlElement *categoryElement = root->FirstChildElement("category");
  while (categoryElement != nullptr)
  {
    section->AddCategory(ParseOldCategoryElement(categoryId, categoryElement, actionSettings));

    // look for the next category
    categoryElement = categoryElement->NextSiblingElement("category");
  }

  // add the section to the settingsmanager
  GetSettingsManager()->AddSection(section);

  // register the callback for action settings
  GetSettingsManager()->RegisterCallback(this, actionSettings);
  
  return true;
}

SettingPtr CAddonSettings::InitializeFromOldSettingAction(std::string settingId, const TiXmlElement *settingElement, const std::string& defaultValue)
{
  // parse the action attribute
  std::string action = XMLUtils::GetAttribute(settingElement, "action");
  // replace $CWD with the url of the add-on
  StringUtils::Replace(action, "$CWD", m_addonPath);
  // replace $ID with the id of the add-on
  StringUtils::Replace(action, "$ID", m_addonId);

  // prepare the setting's control
  auto control = std::make_shared<CSettingControlButton>();
  control->SetFormat("action");

  SettingPtr setting = nullptr;
  // action settings don't require a setting id
  if (settingId.empty())
  {
    auto actionSettingId = StringUtils::Format("action%u", m_unidentifiedSettingId);
    m_unidentifiedSettingId += 1;

    auto settingAction = std::make_shared<CSettingAction>(actionSettingId, GetSettingsManager());
    settingAction->SetData(action);

    setting = settingAction;
  }
  else
  {
    // assume that the setting might store a value as a string
    auto settingString = std::make_shared<CSettingString>(settingId, GetSettingsManager());
    settingString->SetDefault(defaultValue);
    settingString->SetAllowEmpty(true);

    control->SetActionData(action);

    setting = settingString;
  }

  // get any options
  std::string option = XMLUtils::GetAttribute(settingElement, "option");
  // handle the "close" option
  if (StringUtils::EqualsNoCase(option, "close"))
    control->SetCloseDialog(true);

  setting->SetControl(control);

  return setting;
}

std::shared_ptr<CSetting> CAddonSettings::InitializeFromOldSettingLabel()
{
  // label settings don't require a setting id
  auto labelSettingId = StringUtils::Format("label%u", m_unidentifiedSettingId);
  m_unidentifiedSettingId += 1;

  auto settingLabel = std::make_shared<CSettingString>(labelSettingId, GetSettingsManager());

  // create the setting's control
  settingLabel->SetControl(std::make_shared<CSettingControlLabel>());

  return settingLabel;
}

SettingPtr CAddonSettings::InitializeFromOldSettingBool(const std::string& settingId, const TiXmlElement *settingElement, const std::string& defaultValue)
{
  auto setting = std::make_shared<CSettingBool>(settingId, GetSettingsManager());
  if (setting->FromString(defaultValue))
    setting->SetDefault(setting->GetValue());

  setting->SetControl(std::make_shared<CSettingControlCheckmark>());

  return setting;
}

SettingPtr CAddonSettings::InitializeFromOldSettingTextIpAddress(const std::string& settingId, const std::string& settingType, const TiXmlElement *settingElement, const std::string& defaultValue, const int settingLabel)
{
  std::shared_ptr<CSettingString> setting;
  auto control = std::make_shared<CSettingControlEdit>();
  control->SetHeading(settingLabel);

  // get any options
  std::string option = XMLUtils::GetAttribute(settingElement, "option");

  if (settingType == "ipaddress")
  {
    setting = std::make_shared<CSettingString>(settingId, GetSettingsManager());
    control->SetFormat("ip");
  }
  else if (settingType == "text")
  {

    if (StringUtils::EqualsNoCase(option, "urlencoded"))
    {
      setting = std::make_shared<CSettingUrlEncodedString>(settingId, GetSettingsManager());
      control->SetFormat("urlencoded");
    }
    else
    {
      setting = std::make_shared<CSettingString>(settingId, GetSettingsManager());
      control->SetFormat("string");
      control->SetHidden(StringUtils::EqualsNoCase(option, "hidden"));
    }
  }
  
  setting->SetDefault(defaultValue);
  setting->SetAllowEmpty(true);
  setting->SetControl(control);

  return setting;
}

SettingPtr CAddonSettings::InitializeFromOldSettingNumber(const std::string& settingId, const TiXmlElement *settingElement, const std::string& defaultValue, const int settingLabel)
{
  auto setting = std::make_shared<CSettingInt>(settingId, GetSettingsManager());
  if (setting->FromString(defaultValue))
    setting->SetDefault(setting->GetValue());

  auto control = std::make_shared<CSettingControlEdit>();
  control->SetHeading(settingLabel);
  control->SetFormat("integer");
  setting->SetControl(control);

  return setting;
}

SettingPtr CAddonSettings::InitializeFromOldSettingPath(const std::string& settingId, const std::string& settingType, const TiXmlElement *settingElement, const std::string& defaultValue, const int settingLabel)
{
  auto setting = std::make_shared<CSettingPath>(settingId, GetSettingsManager());
  setting->SetDefault(defaultValue);

  // parse sources/shares
  const auto source = XMLUtils::GetAttribute(settingElement, "source");
  if (!source.empty())
    setting->SetSources({ source });

  // setup masking
  const auto audioMask = g_advancedSettings.GetMusicExtensions();
  const auto videoMask = g_advancedSettings.m_videoExtensions;
  const auto imageMask = g_advancedSettings.GetPictureExtensions();
  auto execMask = "";
#if defined(TARGET_WINDOWS)
  execMask = ".exe|.bat|.cmd|.py";
#endif  // defined(TARGET_WINDOWS)

  std::string mask = XMLUtils::GetAttribute(settingElement, "mask");
  if (!mask.empty())
  {
    // convert mask qualifiers
    StringUtils::Replace(mask, "$AUDIO", audioMask);
    StringUtils::Replace(mask, "$VIDEO", videoMask);
    StringUtils::Replace(mask, "$IMAGE", imageMask);
    StringUtils::Replace(mask, "$EXECUTABLE", execMask);
  }
  else
  {
    if (settingType == "video")
      mask = videoMask;
    else if (settingType == "audio")
      mask = audioMask;
    else if (settingType == "image")
      mask = imageMask;
    else if (settingType == "executable")
      mask = execMask;
  }
  setting->SetMasking(mask);

  // parse options
  const auto option = XMLUtils::GetAttribute(settingElement, "option");
  setting->SetWritable(StringUtils::EqualsNoCase(option, "writeable"));

  auto control = std::make_shared<CSettingControlButton>();
  if (settingType == "folder")
    control->SetFormat("path");
  else if (settingType == "image")
    control->SetFormat("image");
  else
  {
    control->SetFormat("file");

    // parse the options
    const auto options = StringUtils::Split(option, OldSettingValuesSeparator);
    control->SetUseImageThumbs(std::find(options.cbegin(), options.cend(), "usethumbs") != options.cend());
    control->SetUseFileDirectories(std::find(options.cbegin(), options.cend(), "treatasfolder") != options.cend());
  }
  control->SetHeading(settingLabel);
  setting->SetControl(control);

  return setting;
}

SettingPtr CAddonSettings::InitializeFromOldSettingDate(const std::string& settingId, const TiXmlElement *settingElement, const std::string& defaultValue, const int settingLabel)
{
  auto setting = std::make_shared<CSettingDate>(settingId, GetSettingsManager());
  if (setting->FromString(defaultValue))
    setting->SetDefault(setting->GetValue());

  auto control = std::make_shared<CSettingControlButton>();
  control->SetFormat("date");
  control->SetHeading(settingLabel);
  setting->SetControl(control);

  return setting;
}

SettingPtr CAddonSettings::InitializeFromOldSettingTime(const std::string& settingId, const TiXmlElement *settingElement, const std::string& defaultValue, const int settingLabel)
{
  auto setting = std::make_shared<CSettingTime>(settingId, GetSettingsManager());
  if (setting->FromString(defaultValue))
    setting->SetDefault(setting->GetValue());

  auto control = std::make_shared<CSettingControlButton>();
  control->SetFormat("time");
  control->SetHeading(settingLabel);
  setting->SetControl(control);

  return setting;
}

SettingPtr CAddonSettings::InitializeFromOldSettingSelect(const std::string& settingId, const TiXmlElement *settingElement, const std::string& defaultValue, const int settingLabel, const std::string& settingValues, const std::vector<std::string>& settingLValues)
{
  // process values and lvalues
  std::vector<std::string> values;
  if (!settingLValues.empty())
    values = settingLValues;
  else
    values = StringUtils::Split(settingValues, OldSettingValuesSeparator);

  SettingPtr setting = nullptr;
  if (!values.empty())
  {
    if (settingLValues.empty())
    {
      auto settingString = std::make_shared<CSettingString>(settingId, GetSettingsManager());
      settingString->SetDefault(defaultValue);

      StringSettingOptions options;
      for (const auto& value : values)
        options.push_back(std::make_pair(value, value));
      settingString->SetOptions(options);

      setting = settingString;
    }
    else
    {
      auto settingInt = std::make_shared<CSettingInt>(settingId, GetSettingsManager());
      if (settingInt->FromString(defaultValue))
        settingInt->SetDefault(settingInt->GetValue());

      TranslatableIntegerSettingOptions options;
      for (uint32_t i = 0; i < values.size(); ++i)
        options.push_back(std::make_pair(static_cast<int>(strtol(values[i].c_str(), nullptr, 0)), i));
      settingInt->SetTranslatableOptions(options);

      setting = settingInt;
    }
  }
  else
  {
    // parse sources/shares
    const auto source = XMLUtils::GetAttribute(settingElement, "source");
    if (!source.empty())
      setting = InitializeFromOldSettingFileWithSource(settingId, settingElement, defaultValue, settingValues);
    else
      CLog::Log(LOGWARNING, "CAddonSettings[%s]: failed to parse old setting definition for \"%s\" of type \"select\"",
        m_addonId.c_str(), settingId.c_str());
  }

  if (setting != nullptr)
  {
    auto control = std::make_shared<CSettingControlList>();
    control->SetHeading(settingLabel);
    control->SetFormat("string");
    setting->SetControl(control);
  }

  return setting;
}

SettingPtr CAddonSettings::InitializeFromOldSettingAddon(const std::string& settingId, const TiXmlElement *settingElement, const std::string& defaultValue, const int settingLabel)
{
  // get addon types
  std::string addonTypeStr = XMLUtils::GetAttribute(settingElement, "addontype");
  const auto addonTypesStr = StringUtils::Split(addonTypeStr, ",");
  std::set<ADDON::TYPE> addonTypes;
  for (auto addonType : addonTypesStr)
  {
    auto type = ADDON::CAddonInfo::TranslateType(StringUtils::Trim(addonType));
    if (type != ADDON::ADDON_UNKNOWN)
      addonTypes.insert(type);
  }

  if (addonTypes.empty())
  {
    CLog::Log(LOGERROR, "CAddonSettings[%s]: missing addon type for addon setting \"%s\"", m_addonId.c_str(), settingId.c_str());
    return nullptr;
  }

  // TODO: support multiple addon types
  if (addonTypes.size() > 1)
  {
    CLog::Log(LOGERROR, "CAddonSettings[%s]: multiple addon types are not supported (addon setting \"%s\")", m_addonId.c_str(), settingId.c_str());
    return nullptr;
  }

  // parse addon ids
  auto addonIds = StringUtils::Split(defaultValue, ",");

  // parse multiselect option
  bool multiselect = false;
  settingElement->QueryBoolAttribute("multiselect", &multiselect);

  // sanity check
  if (addonIds.size() > 1 && !multiselect)
  {
    CLog::Log(LOGWARNING, "CAddonSettings[%s]: multiple default addon ids on non-multiselect addon setting \"%s\"", m_addonId.c_str(), settingId.c_str());
    addonIds.erase(++addonIds.begin(), addonIds.end());
  }

  auto settingAddon = std::make_shared<CSettingAddon>(settingId, GetSettingsManager());
  settingAddon->SetAddonType(*addonTypes.begin());

  SettingPtr setting = settingAddon;
  if (multiselect)
  {
    auto settingList = std::make_shared<CSettingList>(settingId, settingAddon, GetSettingsManager());
    settingList->SetDelimiter(",");
    if (settingList->FromString(addonIds))
      settingList->SetDefault(settingList->GetValue());

    setting = settingList;
  }
  else if (!addonIds.empty())
    settingAddon->SetDefault(addonIds.front());

  auto control = std::make_shared<CSettingControlButton>();
  control->SetFormat("addon");
  control->SetHeading(settingLabel);
  setting->SetControl(control);

  return setting;
}

SettingPtr CAddonSettings::InitializeFromOldSettingEnums(const std::string& settingId, const std::string& settingType, const TiXmlElement *settingElement, const std::string& defaultValue, const std::string& settingValues, const std::vector<std::string>& settingLValues)
{
  // process values and lvalues
  std::vector<std::string> values;
  if (!settingLValues.empty())
    values = settingLValues;
  else if (settingValues == "$HOURS")
  {
    for (uint32_t hour = 0; hour < 24; hour++)
      values.push_back(CDateTime(2000, 1, 1, hour, 0, 0).GetAsLocalizedTime(g_langInfo.GetTimeFormat(), false));
  }
  else
    values = StringUtils::Split(settingValues, OldSettingValuesSeparator);

  // process entries
  const auto settingEntries = StringUtils::Split(XMLUtils::GetAttribute(settingElement, "entries"), OldSettingValuesSeparator);

  // process sort
  std::string sort = XMLUtils::GetAttribute(settingElement, "sort");
  if (sort == "true" || sort == "yes")
    std::sort(values.begin(), values.end(), sortstringbyname());

  SettingPtr setting = nullptr;
  if (settingType == "enum")
  {
    auto settingInt = std::make_shared<CSettingInt>(settingId, GetSettingsManager());

    if (settingLValues.empty())
    {
      IntegerSettingOptions options;
      for (uint32_t i = 0; i < values.size(); ++i)
      {
        std::string label = values[i];
        int value = i;
        if (settingEntries.size() > i)
          value = static_cast<int>(strtol(settingEntries[i].c_str(), nullptr, 0));

        options.push_back(std::make_pair(label, value));
      }

      settingInt->SetOptions(options);
    }
    else
    {
      TranslatableIntegerSettingOptions options;
      for (uint32_t i = 0; i < values.size(); ++i)
      {
        int label = static_cast<int>(strtol(values[i].c_str(), nullptr, 0));
        int value = i;
        if (settingEntries.size() > i)
          value = static_cast<int>(strtol(settingEntries[i].c_str(), nullptr, 0));

        options.push_back(std::make_pair(label, value));
      }

      settingInt->SetTranslatableOptions(options);
    }

    // set the default value
    if (settingInt->FromString(defaultValue))
      settingInt->SetDefault(settingInt->GetValue());

    setting = settingInt;
  }
  else
  {
    auto settingString = std::make_shared<CSettingString>(settingId, GetSettingsManager());

    if (settingLValues.empty())
    {
      StringSettingOptions options;
      for (uint32_t i = 0; i < values.size(); ++i)
      {
        std::string value = values[i];
        if (settingEntries.size() > i)
          value = settingEntries[i];

        options.push_back(std::make_pair(value, value));
      }

      settingString->SetOptions(options);
    }
    else
    {
      TranslatableStringSettingOptions options;
      for (uint32_t i = 0; i < values.size(); ++i)
      {
        int label = static_cast<int>(strtol(values[i].c_str(), nullptr, 0));
        std::string value = g_localizeStrings.GetAddonString(m_addonId, label);
        if (settingEntries.size() > i)
          value = settingEntries[i];

        options.push_back(std::make_pair(label, value));
      }

      settingString->SetTranslatableOptions(options);
    }

    // set the default value
    settingString->SetDefault(defaultValue);

    setting = settingString;
  }

  auto control = std::make_shared<CSettingControlSpinner>();
  control->SetFormat("string");
  setting->SetControl(control);

  return setting;
}

SettingPtr CAddonSettings::InitializeFromOldSettingFileEnum(const std::string& settingId, const TiXmlElement *settingElement, const std::string& defaultValue, const std::string& settingValues)
{
  auto setting = InitializeFromOldSettingFileWithSource(settingId, settingElement, defaultValue, settingValues);

  auto control = std::make_shared<CSettingControlSpinner>();
  control->SetFormat("string");
  setting->SetControl(control);

  return setting;
}

SettingPtr CAddonSettings::InitializeFromOldSettingRangeOfNum(const std::string& settingId, const TiXmlElement *settingElement, const std::string& defaultValue)
{
  auto setting = std::make_shared<CSettingNumber>(settingId, GetSettingsManager());
  if (setting->FromString(defaultValue))
    setting->SetDefault(setting->GetValue());

  // parse rangestart and rangeend
  double rangeStart = 0.0, rangeEnd = 1.0;
  settingElement->QueryDoubleAttribute("rangestart", &rangeStart);
  settingElement->QueryDoubleAttribute("rangeend", &rangeEnd);
  setting->SetMinimum(rangeStart);
  setting->SetMaximum(rangeEnd);

  // parse elements
  uint32_t elements = 2;
  settingElement->QueryUnsignedAttribute("elements", &elements);
  if (elements > 1)
    setting->SetStep((rangeEnd - rangeStart) / (elements - 1));

  // parse valueformat
  int valueFormat = -1;
  settingElement->QueryIntAttribute("valueformat", &valueFormat);

  auto control = std::make_shared<CSettingControlSpinner>();
  control->SetFormat("string");
  control->SetFormatLabel(valueFormat);
  setting->SetControl(control);

  return setting;
}

SettingPtr CAddonSettings::InitializeFromOldSettingSlider(const std::string& settingId, const TiXmlElement *settingElement, const std::string& defaultValue)
{
  // parse range
  double min = 0.0, max = 100.0, step = 1.0;
  const auto range = StringUtils::Split(XMLUtils::GetAttribute(settingElement, "range"), ',');

  if (range.size() > 1)
  {
    min = strtod(range[0].c_str(), nullptr);

    if (range.size() > 2)
    {
      max = strtod(range[2].c_str(), nullptr);
      step = strtod(range[1].c_str(), nullptr);
    }
    else
      max = strtod(range[1].c_str(), nullptr);
  }

  // parse option
  auto option = XMLUtils::GetAttribute(settingElement, "option");
  if (option.empty() || StringUtils::EqualsNoCase(option, "float"))
  {
    auto setting = std::make_shared<CSettingNumber>(settingId, GetSettingsManager());
    if (setting->FromString(defaultValue))
      setting->SetDefault(setting->GetValue());

    setting->SetMinimum(min);
    setting->SetStep(step);
    setting->SetMaximum(max);

    auto control = std::make_shared<CSettingControlSlider>();
    control->SetFormat("number");
    control->SetPopup(false);
    setting->SetControl(control);

    return setting;
  }

  if (StringUtils::EqualsNoCase(option, "int") || StringUtils::EqualsNoCase(option, "percent"))
  {
    auto setting = std::make_shared<CSettingInt>(settingId, GetSettingsManager());
    if (setting->FromString(defaultValue))
      setting->SetDefault(setting->GetValue());

    setting->SetMinimum(static_cast<int>(min));
    setting->SetStep(static_cast<int>(step));
    setting->SetMaximum(static_cast<int>(max));

    auto control = std::make_shared<CSettingControlSlider>();
    control->SetFormat(StringUtils::EqualsNoCase(option, "int") ? "integer" : "percentage");
    control->SetPopup(false);
    setting->SetControl(control);

    return setting;
  }

  CLog::Log(LOGWARNING, "CAddonSettings[%s]: ignoring old setting definition for \"%s\" of type \"slider\" because of unknown option \"%s\"",
    m_addonId.c_str(), settingId.c_str(), option.c_str());

  return nullptr;
}

SettingPtr CAddonSettings::InitializeFromOldSettingFileWithSource(const std::string& settingId, const TiXmlElement *settingElement, const std::string& defaultValue, std::string source)
{
  auto setting = std::make_shared<CSettingPath>(settingId, GetSettingsManager());
  setting->SetDefault(defaultValue);

  if (source.find("$PROFILE") != std::string::npos)
    StringUtils::Replace(source, "$PROFILE", m_addonProfile);
  else
    source = URIUtils::AddFileToFolder(m_addonPath, source);

  setting->SetSources({ source });

  // process the path/file mask
  setting->SetMasking(XMLUtils::GetAttribute(settingElement, "mask"));

  // process option
  std::string option = XMLUtils::GetAttribute(settingElement, "option");
  setting->SetHideExtension(StringUtils::EqualsNoCase(option, "hideext"));

  setting->SetOptionsFiller(FileEnumSettingOptionsFiller);

  return setting;
}

bool CAddonSettings::LoadOldSettingValues(const CXBMCTinyXML& doc, std::map<std::string, std::string>& settings) const
{
  if (!doc.RootElement())
    return false;

  const TiXmlElement* category = doc.RootElement()->FirstChildElement("category");
  if (category == nullptr)
    category = doc.RootElement();

  while (category != nullptr)
  {
    const TiXmlElement *setting = category->FirstChildElement("setting");
    while (setting != nullptr)
    {
      const char *id = setting->Attribute("id");
      const char *value = setting->Attribute("value");
      if (id != nullptr && value != nullptr)
        settings[id] = value;

      setting = setting->NextSiblingElement("setting");
    }

    category = category->NextSiblingElement("category");
  }

  return !settings.empty();
}

bool CAddonSettings::ParseOldLabel(const TiXmlElement* element, const std::string settingId, int& labelId)
{
  labelId = -1;
  if (element == nullptr)
    return false;

  // try to parse the label as a translation number
  if (element->QueryIntAttribute("label", &labelId) == TIXML_SUCCESS && labelId >= 0)
    return true;

  std::string labelString;

  // try to parse the label as a string
  const auto labelStringPtr = element->Attribute("label");
  if (labelStringPtr != nullptr)
    labelString = labelStringPtr;

  bool parsed = !labelString.empty();
  // as a last resort use the setting's identifier as a label
  if (!parsed)
    labelString = settingId;

  labelId = m_unknownSettingLabelId;
  m_unknownSettingLabelId += 1;
  m_unknownSettingLabels.emplace(labelId, labelString);

  return parsed;
}

bool CAddonSettings::ParseOldCondition(std::shared_ptr<const CSetting> setting, const std::vector<std::shared_ptr<const CSetting>> settings, const std::string& condition, CSettingDependency& dependeny) const
{
  if (setting == nullptr)
    return false;

  if (condition.empty())
    return true;

  // find the index of the setting in the list of all settings of the category
  auto settingIt = std::find_if(settings.cbegin(), settings.cend(), [setting](SettingConstPtr otherSetting) {
    if (otherSetting == nullptr)
      return false;

    std::string id = setting->GetId();
    if (setting->GetType() == SettingType::Reference)
      id = std::static_pointer_cast<const CSettingReference>(setting)->GetReferencedId();
    std::string otherId = otherSetting->GetId();
    if (otherSetting->GetType() == SettingType::Reference)
      otherId = std::static_pointer_cast<const CSettingReference>(otherSetting)->GetReferencedId();

    return id == otherId;
  });
  if (settingIt == settings.cend()) {
    CLog::Log(LOGWARNING, "CAddonSettings[%s]: failed to parse old setting conditions \"%s\" for \"%s\"",
      m_addonId.c_str(), condition.c_str(), setting->GetId().c_str());
    return false;
  }
  int32_t currentSettingIndex = std::distance(settings.cbegin(), settingIt);

  CSettingDependencyConditionCombinationPtr dependencyCombination;
  std::vector<std::string> conditions;
  if (condition.find('+') != std::string::npos)
  {
    StringUtils::Tokenize(condition, conditions, '+');
    dependencyCombination = dependeny.And();
  }
  else
  {
    StringUtils::Tokenize(condition, conditions, '|');
    dependencyCombination = dependeny.Or();
  }

  bool error = false;
  for (const auto& cond : conditions)
  {
    ConditionExpression expression;
    if (!ParseOldConditionExpression(cond, expression))
      continue;

    // determine the absolute setting index
    int32_t absoluteSettingIndex = currentSettingIndex + expression.m_relativeSettingIndex;

    // we cannot handle relative indices pointing to settings not belonging to the same category
    if (absoluteSettingIndex < 0 || static_cast<size_t>(absoluteSettingIndex) >= settings.size())
    {
      CLog::Log(LOGWARNING, "CAddonSettings[%s]: cannot reference setting (relative index: %d; absolute index: %d) in another category in old setting condition \"%s\" for \"%s\"",
        m_addonId.c_str(), expression.m_relativeSettingIndex, absoluteSettingIndex, cond.c_str(), setting->GetId().c_str());
      error = true;
      continue;
    }

    SettingConstPtr referencedSetting = settings.at(absoluteSettingIndex);
    if (referencedSetting == nullptr)
    {
      CLog::Log(LOGWARNING, "CAddonSettings[%s]: cannot reference separator setting in old setting condition \"%s\" for \"%s\"",
        m_addonId.c_str(), cond.c_str(), setting->GetId().c_str());
      error = true;
      continue;
    }

    // try to handle some odd cases where the setting is of type string but the comparison value references the index of the value in the list of options
    if (referencedSetting->GetType() == SettingType::String && StringUtils::IsNaturalNumber(expression.m_value))
    {
      // try to parse the comparison value
      size_t valueIndex = static_cast<size_t>(strtoul(expression.m_value.c_str(), nullptr, 10));

      const auto referencedSettingString = std::static_pointer_cast<const CSettingString>(referencedSetting);
      switch (referencedSettingString->GetOptionsType())
      {
        case SettingOptionsType::Static:
        {
          const auto& options = referencedSettingString->GetOptions();
          if (options.size() > valueIndex)
            expression.m_value = options.at(valueIndex).second;
          break;
        }

        case SettingOptionsType::StaticTranslatable:
        {
          const auto& options = referencedSettingString->GetTranslatableOptions();
          if (options.size() > valueIndex)
            expression.m_value = options.at(valueIndex).second;
          break;
        }

        default:
          break;
      }
    }

    // add the condition to the value of the referenced setting
    dependencyCombination->Add(std::make_shared<CSettingDependencyCondition>(referencedSetting->GetId(), expression.m_value, expression.m_operator, expression.m_negated, GetSettingsManager()));
  }

  // if the condition doesn't depend on other settings it might be an infobool expression
  if (!error && dependencyCombination->GetOperations().empty() && dependencyCombination->GetValues().empty())
    dependencyCombination->Add(std::make_shared<CSettingDependencyCondition>("InfoBool", condition, "", false, GetSettingsManager()));

  return !error;
}

bool CAddonSettings::ParseOldConditionExpression(std::string str, ConditionExpression& expression)
{
  StringUtils::ToLower(str);
  StringUtils::Trim(str);

  size_t posOpen = str.find('(');
  size_t posSep = str.find(',', posOpen);
  size_t posClose = str.find(')', posSep);

  if (posOpen == std::string::npos ||
      posSep == std::string::npos ||
      posClose == std::string::npos)
    return false;

  auto op = str.substr(0, posOpen);

  // check if the operator is negated
  expression.m_negated = StringUtils::StartsWith(op, "!");
  if (expression.m_negated)
    op = op.substr(1);

  // parse the operator
  if (StringUtils::EqualsNoCase(op, "eq"))
    expression.m_operator = SettingDependencyOperator::Equals;
  else if (StringUtils::EqualsNoCase(op, "gt"))
    expression.m_operator = SettingDependencyOperator::GreaterThan;
  else if (StringUtils::EqualsNoCase(op, "lt"))
    expression.m_operator = SettingDependencyOperator::LessThan;
  else
    return false;

  expression.m_relativeSettingIndex = static_cast<int32_t>(strtol(str.substr(posOpen + 1, posSep - posOpen - 1).c_str(), nullptr, 10));
  expression.m_value = str.substr(posSep + 1, posClose - posSep - 1);

  return true;
}

void CAddonSettings::FileEnumSettingOptionsFiller(std::shared_ptr<const CSetting> setting, std::vector< std::pair<std::string, std::string> > &list, std::string &current, void *data)
{
  if (setting == nullptr)
    return;

  auto settingPath = std::dynamic_pointer_cast<const CSettingPath>(setting);
  if (settingPath == nullptr)
    return;

  if (settingPath->GetSources().empty())
    return;

  const std::string& masking = settingPath->GetMasking();

  // fetch the matching files/directories
  CFileItemList items;
  XFILE::CDirectory::GetDirectory(settingPath->GetSources().front(), items, masking, XFILE::DIR_FLAG_NO_FILE_DIRS);
  
  // process the matching files/directories
  for (auto item : items)
  {
    if ((masking == "/" && item->m_bIsFolder) || !item->m_bIsFolder)
    {
      if (settingPath->HideExtension())
        item->RemoveExtension();
      list.push_back(std::make_pair(item->GetLabel(), item->GetLabel()));
    }
  }
}

} /* namespace ADDON */
