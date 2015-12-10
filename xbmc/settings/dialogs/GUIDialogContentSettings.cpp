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
#include "addons/GUIDialogAddonSettings.h"
#include "addons/GUIWindowAddonBrowser.h"
#include "filesystem/AddonsDirectory.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "dialogs/GUIDialogSelect.h"
#include "guilib/GUIWindowManager.h"
#include "interfaces/builtins/Builtins.h"
#include "settings/lib/Setting.h"
#include "settings/lib/SettingsManager.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "video/VideoInfoScanner.h"

#define CONTROL_CONTENT_TYPE_BUTTON     20
#define CONTROL_SCRAPER_LIST_BUTTON     21
#define CONTROL_SCRAPER_SETTINGS        22
#define CONTROL_START                   30

#define SETTING_SCAN_RECURSIVE        "scanrecursive"
#define SETTING_USE_DIRECTORY_NAMES   "usedirectorynames"
#define SETTING_CONTAINS_SINGLE_ITEM  "containssingleitem"
#define SETTING_EXCLUDE               "exclude"
#define SETTING_NO_UPDATING           "noupdating"

using namespace ADDON;

bool ByAddonName(const AddonPtr& lhs, const AddonPtr& rhs)
{
  return StringUtils::CompareNoCase(lhs->Name(), rhs->Name()) < 0;
}

CGUIDialogContentSettings::CGUIDialogContentSettings()
  : CGUIDialogSettingsManualBase(WINDOW_DIALOG_CONTENT_SETTINGS, "DialogContentSettings.xml"),
    m_needsSaving(false),
    m_content(CONTENT_NONE),
    m_originalContent(CONTENT_NONE),
    m_showScanSettings(false),
    m_scanRecursive(false),
    m_useDirectoryNames(false),
    m_containsSingleItem(false),
    m_exclude(false),
    m_noUpdating(false),
    m_vecItems(new CFileItemList)
{ }

CGUIDialogContentSettings::~CGUIDialogContentSettings()
{
  m_scraper = nullptr;
  delete m_vecItems;
}

bool CGUIDialogContentSettings::OnMessage(CGUIMessage &message)
{
  switch (message.GetMessage())
  {
    case GUI_MSG_WINDOW_DEINIT:
    {
      m_vecItems->Clear();

      break;
    }

    case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();

      if (iControl == CONTROL_CONTENT_TYPE_BUTTON)
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

        CGUIDialogSelect *dialog = (CGUIDialogSelect *)g_windowManager.GetWindow(WINDOW_DIALOG_SELECT);
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
          if (dialog->GetSelectedLabel() == iSelected)
            return true;
          for (const auto &label : labels)
          {
            if (dialog->GetSelectedLabelText() == label.first)
            {
              m_content = static_cast<CONTENT_TYPE>(label.second);
              break;
            }
          }

          AddonPtr scraperAddon;
          CAddonMgr::GetInstance().GetDefault(ADDON::ScraperTypeFromContent(m_content), scraperAddon);
          m_scraper = std::dynamic_pointer_cast<CScraper>(scraperAddon);

          SetupView();
          SET_CONTROL_LABEL2(CONTROL_CONTENT_TYPE_BUTTON, dialog->GetSelectedLabelText());
          SET_CONTROL_FOCUS(CONTROL_CONTENT_TYPE_BUTTON, 0);
        }
      }
      else if (iControl == CONTROL_SCRAPER_LIST_BUTTON)
      {
        ADDON::TYPE type = ADDON::ScraperTypeFromContent(m_content);
        std::string selectedAddonId = m_scraper->ID();

        if (CGUIWindowAddonBrowser::SelectAddonID(type, selectedAddonId, false) == 1)
        {
          AddonPtr last = m_scraper;

          AddonPtr scraperAddon;
          CAddonMgr::GetInstance().GetAddon(selectedAddonId, scraperAddon);
          m_scraper = std::dynamic_pointer_cast<CScraper>(scraperAddon);

          SET_CONTROL_LABEL2(CONTROL_SCRAPER_LIST_BUTTON, m_scraper->Name());

          if (m_scraper != last)
            SetupView();

          if (m_scraper != last)
            m_needsSaving = true;
          CONTROL_ENABLE_ON_CONDITION(CONTROL_SCRAPER_SETTINGS, m_scraper->HasSettings());
          SET_CONTROL_FOCUS(CONTROL_SCRAPER_LIST_BUTTON, 0);
        }
      }
      else if (iControl == CONTROL_SCRAPER_SETTINGS)
      {
        bool result = CGUIDialogAddonSettings::ShowAndGetInput(m_scraper, false);
        if (result)
          m_needsSaving = true;
        return result;
      }

      break;
    }

    default:
      break;
  }

  return CGUIDialogSettingsManualBase::OnMessage(message);
}

CFileItemPtr CGUIDialogContentSettings::GetCurrentListItem(int offset)
{
  int currentItem = -1;
  if (m_exclude)
    return CFileItemPtr();

  for (int i = 0; i < m_vecItems->Size(); ++i)
  {
    if (m_vecItems->Get(i)->IsSelected())
    {
      currentItem = i;
      break;
    }
  }

  if (currentItem == -1)
    return CFileItemPtr();

  return m_vecItems->Get((currentItem + offset) % m_vecItems->Size());
}

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
  CGUIDialogContentSettings *dialog = (CGUIDialogContentSettings *)g_windowManager.GetWindow(WINDOW_DIALOG_CONTENT_SETTINGS);
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
  SET_CONTROL_LABEL(CONTROL_CONTENT_TYPE_BUTTON, 20344);
  SET_CONTROL_LABEL(CONTROL_SCRAPER_LIST_BUTTON, 38025);
  m_needsSaving = false;

  CGUIDialogSettingsManualBase::OnInitWindow();
}

void CGUIDialogContentSettings::OnSettingChanged(const CSetting *setting)
{
  if (setting == NULL)
    return;

  CGUIDialogSettingsManualBase::OnSettingChanged(setting);

  const std::string &settingId = setting->GetId();
  if (settingId == SETTING_CONTAINS_SINGLE_ITEM)
    m_containsSingleItem = static_cast<const CSettingBool*>(setting)->GetValue();
  else if (settingId == SETTING_NO_UPDATING)
    m_noUpdating = static_cast<const CSettingBool*>(setting)->GetValue();
  else if (settingId == SETTING_USE_DIRECTORY_NAMES)
    m_useDirectoryNames = static_cast<const CSettingBool*>(setting)->GetValue();
  else if (settingId == SETTING_SCAN_RECURSIVE)
  {
    m_scanRecursive = static_cast<const CSettingBool*>(setting)->GetValue();
    m_settingsManager->SetBool(SETTING_CONTAINS_SINGLE_ITEM, false);
  }
  else if (settingId == SETTING_EXCLUDE)
    m_exclude = static_cast<const CSettingBool*>(setting)->GetValue();

  m_needsSaving = true;
}

void CGUIDialogContentSettings::Save()
{
  if (!m_needsSaving ||
      m_scraper == NULL)
    return;

  if (m_content == CONTENT_NONE)
  {
    m_scraper.reset();
    return;
  }
}

void CGUIDialogContentSettings::OnOkay()
{
  // watch for content change, but same scraper
  if (m_content != m_originalContent)
    m_needsSaving = true;

  CGUIDialogSettingsManualBase::OnOkay();
}

void CGUIDialogContentSettings::OnCancel()
{
  m_needsSaving = false;

  CGUIDialogSettingsManualBase::OnCancel();
}

void CGUIDialogContentSettings::SetupView()
{
  SET_CONTROL_LABEL2(CONTROL_CONTENT_TYPE_BUTTON, ADDON::TranslateContent(m_content, true));

  m_vecItems->Clear();
  if (m_content == CONTENT_NONE)
  {
    m_showScanSettings = false;
    SET_CONTROL_HIDDEN(CONTROL_SCRAPER_LIST_BUTTON);
    CONTROL_DISABLE(CONTROL_SCRAPER_SETTINGS);
  }
  else
  {
    SET_CONTROL_VISIBLE(CONTROL_SCRAPER_LIST_BUTTON);
    if (m_scraper != NULL && !CAddonMgr::GetInstance().IsAddonDisabled(m_scraper->ID()))
    {
      SET_CONTROL_LABEL2(CONTROL_SCRAPER_LIST_BUTTON, m_scraper->Name());

      m_showScanSettings = true;
      if (m_scraper && m_scraper->Supports(m_content) && m_scraper->HasSettings())
        CONTROL_ENABLE(CONTROL_SCRAPER_SETTINGS);
    }
    else
    {
      SET_CONTROL_LABEL2(CONTROL_SCRAPER_LIST_BUTTON, 231); //Set label2 to "None"
      CONTROL_DISABLE(CONTROL_SCRAPER_SETTINGS);
    }
  }

  CGUIDialogSettingsManualBase::SetupView();
}

void CGUIDialogContentSettings::InitializeSettings()
{
  CGUIDialogSettingsManualBase::InitializeSettings();

  CSettingCategory *category = AddCategory("contentsettings", -1);
  if (category == NULL)
  {
    CLog::Log(LOGERROR, "CGUIDialogContentSettings: unable to setup settings");
    return;
  }

  CSettingGroup *group = AddGroup(category);
  if (group == NULL)
  {
    CLog::Log(LOGERROR, "CGUIDialogContentSettings: unable to setup settings");
    return;
  }

  switch (m_content)
  {
    case CONTENT_TVSHOWS:
    {
      AddToggle(group, SETTING_CONTAINS_SINGLE_ITEM, 20379, 0, m_containsSingleItem, false, m_showScanSettings);
      AddToggle(group, SETTING_NO_UPDATING, 20432, 0, m_noUpdating, false, m_showScanSettings);
      break;
    }

    case CONTENT_MOVIES:
    case CONTENT_MUSICVIDEOS:
    {
      AddToggle(group, SETTING_USE_DIRECTORY_NAMES, m_content == CONTENT_MOVIES ? 20329 : 20330, 0, m_useDirectoryNames, false, m_showScanSettings);
      CSettingBool *settingScanRecursive = AddToggle(group, SETTING_SCAN_RECURSIVE, 20346, 0, m_scanRecursive, false, m_showScanSettings);
      CSettingBool *settingContainsSingleItem = AddToggle(group, SETTING_CONTAINS_SINGLE_ITEM, 20383, 0, m_containsSingleItem, false, m_showScanSettings);
      AddToggle(group, SETTING_NO_UPDATING, 20432, 0, m_noUpdating, false, m_showScanSettings);
      
      // define an enable dependency with (m_useDirectoryNames && !m_containsSingleItem) || !m_useDirectoryNames
      CSettingDependency dependencyScanRecursive(SettingDependencyTypeEnable, m_settingsManager);
      dependencyScanRecursive.Or()
        ->Add(CSettingDependencyConditionCombinationPtr((new CSettingDependencyConditionCombination(BooleanLogicOperationAnd, m_settingsManager))                                     // m_useDirectoryNames && !m_containsSingleItem
          ->Add(CSettingDependencyConditionPtr(new CSettingDependencyCondition(SETTING_USE_DIRECTORY_NAMES, "true", SettingDependencyOperatorEquals, false, m_settingsManager)))      // m_useDirectoryNames
          ->Add(CSettingDependencyConditionPtr(new CSettingDependencyCondition(SETTING_CONTAINS_SINGLE_ITEM, "false", SettingDependencyOperatorEquals, false, m_settingsManager)))))  // !m_containsSingleItem
        ->Add(CSettingDependencyConditionPtr(new CSettingDependencyCondition(SETTING_USE_DIRECTORY_NAMES, "false", SettingDependencyOperatorEquals, false, m_settingsManager)));      // !m_useDirectoryNames

      // define an enable dependency with m_useDirectoryNames && !m_scanRecursive
      CSettingDependency depdendencyContainsSingleItem(SettingDependencyTypeEnable, m_settingsManager);
      depdendencyContainsSingleItem.And()
        ->Add(CSettingDependencyConditionPtr(new CSettingDependencyCondition(SETTING_USE_DIRECTORY_NAMES, "true", SettingDependencyOperatorEquals, false, m_settingsManager)))        // m_useDirectoryNames
        ->Add(CSettingDependencyConditionPtr(new CSettingDependencyCondition(SETTING_SCAN_RECURSIVE, "false", SettingDependencyOperatorEquals, false, m_settingsManager)));           // !m_scanRecursive

      SettingDependencies deps;
      deps.push_back(dependencyScanRecursive);
      settingScanRecursive->SetDependencies(deps);

      deps.clear();
      deps.push_back(depdendencyContainsSingleItem);
      settingContainsSingleItem->SetDependencies(deps);
      break;
    }

    case CONTENT_ALBUMS:
    case CONTENT_ARTISTS:
      break;

    case CONTENT_NONE:
    default:
      AddToggle(group, SETTING_EXCLUDE, 20380, 0, m_exclude, false, !m_showScanSettings);
      break;
  }
}
