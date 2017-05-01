/*
 *      Copyright (C) 2005-2014 Team XBMC
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

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <limits.h>

#include "GUIDialogContentSettings.h"
#include "addons/AddonSystemSettings.h"
#include "addons/GUIDialogAddonSettings.h"
#include "addons/GUIWindowAddonBrowser.h"
#include "filesystem/AddonsDirectory.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "dialogs/GUIDialogSelect.h"
#include "guilib/GUIWindowManager.h"
#include "interfaces/builtins/Builtins.h"
#include "settings/lib/Setting.h"
#include "settings/lib/SettingsManager.h"
#include "settings/windows/GUIControlSettings.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "video/VideoInfoScanner.h"

#define SETTING_CONTENT_TYPE          "contenttype"
#define SETTING_SCRAPER_LIST          "scraperlist"
#define SETTING_SCRAPER_SETTINGS      "scrapersettings"
#define SETTING_SCAN_RECURSIVE        "scanrecursive"
#define SETTING_USE_DIRECTORY_NAMES   "usedirectorynames"
#define SETTING_CONTAINS_SINGLE_ITEM  "containssingleitem"
#define SETTING_EXCLUDE               "exclude"
#define SETTING_NO_UPDATING           "noupdating"

using namespace ADDON;


CGUIDialogContentSettings::CGUIDialogContentSettings()
  : CGUIDialogSettingsManualBase(WINDOW_DIALOG_CONTENT_SETTINGS, "DialogSettings.xml"),
    m_content(CONTENT_NONE),
    m_originalContent(CONTENT_NONE),
    m_showScanSettings(false),
    m_scanRecursive(false),
    m_useDirectoryNames(false),
    m_containsSingleItem(false),
    m_exclude(false),
    m_noUpdating(false)
{ }

void CGUIDialogContentSettings::SetContent(CONTENT_TYPE content)
{
  m_content = m_originalContent = content;
}

void CGUIDialogContentSettings::ResetContent()
{
  SetContent(CONTENT_NONE);
}

void CGUIDialogContentSettings::SetScanSettings(const VIDEO::SScanSettings &scanSettings)
{
  m_scanRecursive       = (scanSettings.recurse > 0 && !scanSettings.parent_name) ||
                          (scanSettings.recurse > 1 && scanSettings.parent_name);
  m_useDirectoryNames   = scanSettings.parent_name;
  m_exclude             = scanSettings.exclude;
  m_containsSingleItem  = scanSettings.parent_name_root;
  m_noUpdating          = scanSettings.noupdate;
}

bool CGUIDialogContentSettings::Show(ADDON::ScraperPtr& scraper, CONTENT_TYPE content /* = CONTENT_NONE */)
{
  VIDEO::SScanSettings dummy;
  return Show(scraper, dummy, content);
}

bool CGUIDialogContentSettings::Show(ADDON::ScraperPtr& scraper, VIDEO::SScanSettings& settings, CONTENT_TYPE content /* = CONTENT_NONE */)
{
  CGUIDialogContentSettings *dialog = g_windowManager.GetWindow<CGUIDialogContentSettings>(WINDOW_DIALOG_CONTENT_SETTINGS);
  if (dialog == NULL)
    return false;

  if (scraper != NULL)
  {
    dialog->SetContent(content != CONTENT_NONE ? content : scraper->Content());
    dialog->SetScraper(scraper);
    // toast selected but disabled scrapers
    if (CAddonMgr::GetInstance().IsAddonDisabled(scraper->ID()))
      CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Error, g_localizeStrings.Get(24024), scraper->Name(), 2000, true);
  }

  dialog->SetScanSettings(settings);
  dialog->Open();

  bool confirmed = dialog->IsConfirmed();
  if (confirmed)
  {
    scraper = dialog->GetScraper();
    content = dialog->GetContent();

    if (scraper == NULL || content == CONTENT_NONE)
      settings.exclude = dialog->GetExclude();
    else
    {
      settings.exclude = false;
      settings.noupdate = dialog->GetNoUpdating();
      scraper->SetPathSettings(content, "");

      if (content == CONTENT_TVSHOWS)
      {
        settings.parent_name = settings.parent_name_root = dialog->GetContainsSingleItem();
        settings.recurse = 0;
      }
      else if (content == CONTENT_MOVIES || content == CONTENT_MUSICVIDEOS)
      {
        if (dialog->GetUseDirectoryNames())
        {
          settings.parent_name = true;
          settings.parent_name_root = false;
          settings.recurse = dialog->GetScanRecursive() ? INT_MAX : 1;

          if (dialog->GetContainsSingleItem())
          {
            settings.parent_name_root = true;
            settings.recurse = 0;
          }
        }
        else
        {
          settings.parent_name = false;
          settings.parent_name_root = false;
          settings.recurse = dialog->GetScanRecursive() ? INT_MAX : 0;
        }
      }
    }
  }

  // now that we have evaluated all settings we need to reset the content
  dialog->ResetContent();

  return confirmed;
}

void CGUIDialogContentSettings::OnInitWindow()
{
  CGUIDialogSettingsManualBase::OnInitWindow();
}

void CGUIDialogContentSettings::OnSettingChanged(std::shared_ptr<const CSetting> setting)
{
  if (setting == NULL)
    return;

  CGUIDialogSettingsManualBase::OnSettingChanged(setting);

  const std::string &settingId = setting->GetId();
  if (settingId == SETTING_CONTAINS_SINGLE_ITEM)
    m_containsSingleItem = std::static_pointer_cast<const CSettingBool>(setting)->GetValue();
  else if (settingId == SETTING_NO_UPDATING)
    m_noUpdating = std::static_pointer_cast<const CSettingBool>(setting)->GetValue();
  else if (settingId == SETTING_USE_DIRECTORY_NAMES)
    m_useDirectoryNames = std::static_pointer_cast<const CSettingBool>(setting)->GetValue();
  else if (settingId == SETTING_SCAN_RECURSIVE)
  {
    m_scanRecursive = std::static_pointer_cast<const CSettingBool>(setting)->GetValue();
    GetSettingsManager()->SetBool(SETTING_CONTAINS_SINGLE_ITEM, false);
  }
  else if (settingId == SETTING_EXCLUDE)
    m_exclude = std::static_pointer_cast<const CSettingBool>(setting)->GetValue();
}

void CGUIDialogContentSettings::OnSettingAction(std::shared_ptr<const CSetting> setting)
{
  if (setting == NULL)
    return;

  CGUIDialogSettingsManualBase::OnSettingAction(setting);

  const std::string &settingId = setting->GetId();

  if (settingId == SETTING_CONTENT_TYPE)
  {
    std::vector<std::pair<std::string, int>> labels;
    if (m_content == CONTENT_ALBUMS || m_content == CONTENT_ARTISTS)
    {
      labels.push_back(std::make_pair(ADDON::TranslateContent(m_content, true), m_content));
    }
    else
    {
      labels.push_back(std::make_pair(ADDON::TranslateContent(CONTENT_NONE, true), CONTENT_NONE));
      labels.push_back(std::make_pair(ADDON::TranslateContent(CONTENT_MOVIES, true), CONTENT_MOVIES));
      labels.push_back(std::make_pair(ADDON::TranslateContent(CONTENT_TVSHOWS, true), CONTENT_TVSHOWS));
      labels.push_back(std::make_pair(ADDON::TranslateContent(CONTENT_MUSICVIDEOS, true), CONTENT_MUSICVIDEOS));
    }
    std::sort(labels.begin(), labels.end());

    CGUIDialogSelect *dialog = g_windowManager.GetWindow<CGUIDialogSelect>(WINDOW_DIALOG_SELECT);
    if (dialog)
    {
      dialog->SetHeading(CVariant{ 20344 }); //Label "This directory contains"

      int iIndex = 0;
      int iSelected = 0;
      for (const auto &label : labels)
      {
        dialog->Add(label.first);

        if (m_content == label.second)
          iSelected = iIndex;
        iIndex++;
      }

      dialog->SetSelected(iSelected);

      dialog->Open();
      // Selected item has not changes - in case of cancel or the user selecting the same item
      int newSelected = dialog->GetSelectedItem();
      if (!dialog->IsConfirmed() || newSelected < 0 || newSelected == iSelected)
        return;

      auto selected = labels.at(newSelected);
      m_content = static_cast<CONTENT_TYPE>(selected.second);

      AddonPtr scraperAddon;
      CAddonSystemSettings::GetInstance().GetActive(ADDON::ScraperTypeFromContent(m_content),
          scraperAddon);
      m_scraper = std::dynamic_pointer_cast<CScraper>(scraperAddon);

      SetupView();
      SetFocus(SETTING_CONTENT_TYPE);
    }
  }
  else if (settingId == SETTING_SCRAPER_LIST)
  {
    ADDON::TYPE type = ADDON::ScraperTypeFromContent(m_content);
    std::string currentScraperId;
    if (m_scraper != nullptr)
      currentScraperId = m_scraper->ID();
    std::string selectedAddonId = currentScraperId;

    if (CGUIWindowAddonBrowser::SelectAddonID(type, selectedAddonId, false) == 1
        && selectedAddonId != currentScraperId)
    {
      AddonPtr scraperAddon;
      CAddonMgr::GetInstance().GetAddon(selectedAddonId, scraperAddon);
      m_scraper = std::dynamic_pointer_cast<CScraper>(scraperAddon);

      SetupView();
      SetFocus(SETTING_SCRAPER_LIST);
    }
  }
  else if (settingId == SETTING_SCRAPER_SETTINGS)
    CGUIDialogAddonSettings::ShowAndGetInput(m_scraper, false);
}

void CGUIDialogContentSettings::Save()
{
  //Should be saved by caller of ::Show
}

void CGUIDialogContentSettings::SetupView()
{
  CGUIDialogSettingsManualBase::SetupView();
  SetHeading(20333);

  SET_CONTROL_HIDDEN(CONTROL_SETTINGS_CUSTOM_BUTTON);
  SET_CONTROL_LABEL(CONTROL_SETTINGS_OKAY_BUTTON, 186);
  SET_CONTROL_LABEL(CONTROL_SETTINGS_CANCEL_BUTTON, 222);

  SetLabel2(SETTING_CONTENT_TYPE, ADDON::TranslateContent(m_content, true));

  if (m_content == CONTENT_NONE)
  {
    ToggleState(SETTING_SCRAPER_LIST, false);
    ToggleState(SETTING_SCRAPER_SETTINGS, false);
  }
  else
  {
    ToggleState(SETTING_SCRAPER_LIST, true);
    if (m_scraper != NULL && !CAddonMgr::GetInstance().IsAddonDisabled(m_scraper->ID()))
    {
      SetLabel2(SETTING_SCRAPER_LIST, m_scraper->Name());
      if (m_scraper && m_scraper->Supports(m_content) && m_scraper->HasSettings())
        ToggleState(SETTING_SCRAPER_SETTINGS, true);
      else
        ToggleState(SETTING_SCRAPER_SETTINGS, false);
    }
    else
    {
      SetLabel2(SETTING_SCRAPER_LIST, g_localizeStrings.Get(231)); //Set label2 to "None"
      ToggleState(SETTING_SCRAPER_SETTINGS, false);
    }
  }
}

void CGUIDialogContentSettings::InitializeSettings()
{
  CGUIDialogSettingsManualBase::InitializeSettings();

  if (m_content == CONTENT_NONE)
    m_showScanSettings = false;
  else if (m_scraper != NULL && !CAddonMgr::GetInstance().IsAddonDisabled(m_scraper->ID()))
    m_showScanSettings = true;

  std::shared_ptr<CSettingCategory> category = AddCategory("contentsettings", -1);
  if (category == NULL)
  {
    CLog::Log(LOGERROR, "CGUIDialogContentSettings: unable to setup settings");
    return;
  }

  std::shared_ptr<CSettingGroup> group = AddGroup(category);
  if (group == NULL)
  {
    CLog::Log(LOGERROR, "CGUIDialogContentSettings: unable to setup settings");
    return;
  }

  AddButton(group, SETTING_CONTENT_TYPE, 20344, 0);
  AddButton(group, SETTING_SCRAPER_LIST, 38025, 0);
  std::shared_ptr<CSettingAction> subsetting = AddButton(group, SETTING_SCRAPER_SETTINGS, 10004, 0);
  if (subsetting != NULL)
    subsetting->SetParent(SETTING_SCRAPER_LIST);

  std::shared_ptr<CSettingGroup> groupDetails = AddGroup(category, 20322);
  if (groupDetails == NULL)
  {
    CLog::Log(LOGERROR, "CGUIDialogContentSettings: unable to setup scanning settings");
    return;
  }
  switch (m_content)
  {
    case CONTENT_TVSHOWS:
    {
      AddToggle(groupDetails, SETTING_CONTAINS_SINGLE_ITEM, 20379, 0, m_containsSingleItem, false, m_showScanSettings);
      AddToggle(groupDetails, SETTING_NO_UPDATING, 20432, 0, m_noUpdating, false, m_showScanSettings);
      break;
    }

    case CONTENT_MOVIES:
    case CONTENT_MUSICVIDEOS:
    {
      AddToggle(groupDetails, SETTING_USE_DIRECTORY_NAMES, m_content == CONTENT_MOVIES ? 20329 : 20330, 0, m_useDirectoryNames, false, m_showScanSettings);
      std::shared_ptr<CSettingBool> settingScanRecursive = AddToggle(groupDetails, SETTING_SCAN_RECURSIVE, 20346, 0, m_scanRecursive, false, m_showScanSettings);
      std::shared_ptr<CSettingBool> settingContainsSingleItem = AddToggle(groupDetails, SETTING_CONTAINS_SINGLE_ITEM, 20383, 0, m_containsSingleItem, false, m_showScanSettings);
      AddToggle(groupDetails, SETTING_NO_UPDATING, 20432, 0, m_noUpdating, false, m_showScanSettings);
      
      // define an enable dependency with (m_useDirectoryNames && !m_containsSingleItem) || !m_useDirectoryNames
      CSettingDependency dependencyScanRecursive(SettingDependencyTypeEnable, GetSettingsManager());
      dependencyScanRecursive.Or()
        ->Add(CSettingDependencyConditionCombinationPtr((new CSettingDependencyConditionCombination(BooleanLogicOperationAnd, GetSettingsManager()))                                     // m_useDirectoryNames && !m_containsSingleItem
          ->Add(CSettingDependencyConditionPtr(new CSettingDependencyCondition(SETTING_USE_DIRECTORY_NAMES, "true", SettingDependencyOperatorEquals, false, GetSettingsManager())))      // m_useDirectoryNames
          ->Add(CSettingDependencyConditionPtr(new CSettingDependencyCondition(SETTING_CONTAINS_SINGLE_ITEM, "false", SettingDependencyOperatorEquals, false, GetSettingsManager())))))  // !m_containsSingleItem
        ->Add(CSettingDependencyConditionPtr(new CSettingDependencyCondition(SETTING_USE_DIRECTORY_NAMES, "false", SettingDependencyOperatorEquals, false, GetSettingsManager())));      // !m_useDirectoryNames

      // define an enable dependency with m_useDirectoryNames && !m_scanRecursive
      CSettingDependency dependencyContainsSingleItem(SettingDependencyTypeEnable, GetSettingsManager());
      dependencyContainsSingleItem.And()
        ->Add(CSettingDependencyConditionPtr(new CSettingDependencyCondition(SETTING_USE_DIRECTORY_NAMES, "true", SettingDependencyOperatorEquals, false, GetSettingsManager())))        // m_useDirectoryNames
        ->Add(CSettingDependencyConditionPtr(new CSettingDependencyCondition(SETTING_SCAN_RECURSIVE, "false", SettingDependencyOperatorEquals, false, GetSettingsManager())));           // !m_scanRecursive

      SettingDependencies deps;
      deps.push_back(dependencyScanRecursive);
      settingScanRecursive->SetDependencies(deps);

      deps.clear();
      deps.push_back(dependencyContainsSingleItem);
      settingContainsSingleItem->SetDependencies(deps);
      break;
    }

    case CONTENT_ALBUMS:
    case CONTENT_ARTISTS:
      break;

    case CONTENT_NONE:
    default:
      AddToggle(groupDetails, SETTING_EXCLUDE, 20380, 0, m_exclude, false, !m_showScanSettings);
      break;
  }
}

void CGUIDialogContentSettings::SetLabel2(const std::string &settingid, const std::string &label)
{
  BaseSettingControlPtr settingControl = GetSettingControl(settingid);
  if (settingControl != NULL && settingControl->GetControl() != NULL)
    SET_CONTROL_LABEL2(settingControl->GetID(), label);
}

void CGUIDialogContentSettings::ToggleState(const std::string &settingid, bool enabled)
{
  BaseSettingControlPtr settingControl = GetSettingControl(settingid);
  if (settingControl != NULL && settingControl->GetControl() != NULL)
  {
    if (enabled)
      CONTROL_ENABLE(settingControl->GetID());
    else
      CONTROL_DISABLE(settingControl->GetID());
  }
}

void CGUIDialogContentSettings::SetFocus(const std::string &settingid)
{
  BaseSettingControlPtr settingControl = GetSettingControl(settingid);
  if (settingControl != NULL && settingControl->GetControl() != NULL)
    SET_CONTROL_FOCUS(settingControl->GetID(), 0);
}
