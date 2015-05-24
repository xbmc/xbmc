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

#include <limits.h>

#include "GUIDialogContentSettings.h"
#include "FileItem.h"
#include "addons/AddonManager.h"
#include "addons/GUIDialogAddonSettings.h"
#include "filesystem/AddonsDirectory.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "guilib/GUIWindowManager.h"
#include "input/Key.h"
#include "interfaces/Builtins.h"
#include "settings/lib/Setting.h"
#include "settings/lib/SettingDependency.h"
#include "settings/lib/SettingsManager.h"
#include "utils/log.h"
#include "video/VideoInfoScanner.h"

#define CONTROL_CONTENT_TYPE       20
#define CONTROL_SCRAPER_LIST       21
#define CONTROL_SCRAPER_SETTINGS   22
#define CONTROL_START              30

#define SETTING_SCAN_RECURSIVE        "scanrecursive"
#define SETTING_USE_DIRECTORY_NAMES   "usedirectorynames"
#define SETTING_CONTAINS_SINGLE_ITEM  "containssingleitem"
#define SETTING_EXCLUDE               "exclude"
#define SETTING_NO_UPDATING           "noupdating"

using namespace std;
using namespace ADDON;

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
  delete m_vecItems;
}

bool CGUIDialogContentSettings::OnMessage(CGUIMessage &message)
{
  switch (message.GetMessage())
  {
    case GUI_MSG_WINDOW_DEINIT:
    {
      m_scrapers.clear();
      m_vecItems->Clear();

      break;
    }

    case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();

      if (iControl == CONTROL_CONTENT_TYPE)
      {
        CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_CONTENT_TYPE);
        OnMessage(msg);
        m_content = static_cast<CONTENT_TYPE>(msg.GetParam1());
        SetupView();
      }
      else if (iControl == CONTROL_SCRAPER_LIST)
      {
        // we handle only select actions
        int action = message.GetParam1();
        if (action != ACTION_SELECT_ITEM && action != ACTION_MOUSE_LEFT_CLICK)
          break;

        CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_SCRAPER_LIST);
        OnMessage(msg);
        int iSelected = msg.GetParam1();
        if (iSelected == m_vecItems->Size() - 1)
        { // Get More... item, path 'addons://more/<content>'
          // This is tricky - ideally we want to completely save the state of this dialog,
          // close it while linking to the addon manager, then reopen it on return.
          // For now, we just close the dialog + send the GetPath() to open the addons window
          std::string content = m_vecItems->Get(iSelected)->GetPath().substr(14);
          OnCancel();
          Close();
          CBuiltins::Execute("ActivateWindow(AddonBrowser,addons://all/xbmc.metadata.scraper." + content + ",return)");
          return true;
        }

        AddonPtr last = m_scraper;
        m_scraper = std::dynamic_pointer_cast<CScraper>(m_scrapers[m_content][iSelected]);
        m_lastSelected[m_content] = m_scraper;

        if (m_scraper != last)
          SetupView();

        if (m_scraper != last)
          m_needsSaving = true;
        CONTROL_ENABLE_ON_CONDITION(CONTROL_SCRAPER_SETTINGS, m_scraper->HasSettings());
        SET_CONTROL_FOCUS(CONTROL_START, 0);
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
    if (!scraper->Enabled())
      CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Error, g_localizeStrings.Get(24024), scraper->Name(), 2000, true);
  }

  dialog->SetScanSettings(settings);
  dialog->DoModal();

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
  m_lastSelected.clear();

  // save our current scraper (if any)
  if (m_scraper != NULL)
    m_lastSelected[m_content] = m_scraper;

  FillContentTypes();
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
  CGUIMessage msgReset(GUI_MSG_LABEL_RESET, GetID(), CONTROL_SCRAPER_LIST);
  OnMessage(msgReset);

  m_vecItems->Clear();
  if (m_content == CONTENT_NONE)
  {
    m_showScanSettings = false;
    SET_CONTROL_HIDDEN(CONTROL_SCRAPER_LIST);
    CONTROL_DISABLE(CONTROL_SCRAPER_SETTINGS);
  }
  else
  {
    FillScraperList();
    SET_CONTROL_VISIBLE(CONTROL_SCRAPER_LIST);
    if (m_scraper != NULL && m_scraper->Enabled())
    {
      m_showScanSettings = true;
      if (m_scraper && m_scraper->Supports(m_content) && m_scraper->HasSettings())
        CONTROL_ENABLE(CONTROL_SCRAPER_SETTINGS);
    }
    else
      CONTROL_DISABLE(CONTROL_SCRAPER_SETTINGS);
  }

  SET_CONTROL_VISIBLE(CONTROL_CONTENT_TYPE);

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

void CGUIDialogContentSettings::FillContentTypes()
{
  std::vector< std::pair<std::string, int> > labels;

  if (m_content == CONTENT_ALBUMS || m_content == CONTENT_ARTISTS)
  {
    FillContentTypes(m_content);
    labels.push_back(make_pair(ADDON::TranslateContent(m_content, true), m_content));
  }
  else
  {
    FillContentTypes(CONTENT_MOVIES);
    FillContentTypes(CONTENT_TVSHOWS);
    FillContentTypes(CONTENT_MUSICVIDEOS);

    labels.push_back(make_pair(ADDON::TranslateContent(CONTENT_MOVIES, true), CONTENT_MOVIES));
    labels.push_back(make_pair(ADDON::TranslateContent(CONTENT_TVSHOWS, true), CONTENT_TVSHOWS));
    labels.push_back(make_pair(ADDON::TranslateContent(CONTENT_MUSICVIDEOS, true), CONTENT_MUSICVIDEOS));
    labels.push_back(make_pair(ADDON::TranslateContent(CONTENT_NONE, true), CONTENT_NONE));
  }

  SET_CONTROL_LABELS(CONTROL_CONTENT_TYPE, m_content, &labels);
}

void CGUIDialogContentSettings::FillContentTypes(CONTENT_TYPE content)
{
  // grab all scrapers which support this content-type
  VECADDONS addons;
  TYPE type = ADDON::ScraperTypeFromContent(content);
  if (!CAddonMgr::Get().GetAddons(type, addons))
    return;

  AddonPtr addon;
  std::string defaultID;
  if (CAddonMgr::Get().GetDefault(type, addon))
    defaultID = addon->ID();

  for (IVECADDONS it = addons.begin(); it != addons.end(); ++it)
  {
    bool isDefault = ((*it)->ID() == defaultID);
    map<CONTENT_TYPE, VECADDONS>::iterator iter = m_scrapers.find(content);

    AddonPtr scraper = (*it)->Clone();

    if (m_scraper != NULL && m_scraper->ID() == (*it)->ID())
    { // don't overwrite preconfigured scraper
      scraper = m_scraper;
    }

    if (iter != m_scrapers.end())
    {
      if (isDefault)
        iter->second.insert(iter->second.begin(), scraper);
      else
        iter->second.push_back(scraper);
    }
    else
    {
      VECADDONS vec;
      vec.push_back(scraper);
      m_scrapers.insert(make_pair(content,vec));
    }
  }
}

void CGUIDialogContentSettings::FillScraperList()
{
  int iIndex = 0;
  int selectedIndex = 0;

  if (m_lastSelected.find(m_content) != m_lastSelected.end())
    m_scraper = std::dynamic_pointer_cast<CScraper>(m_lastSelected[m_content]);
  else
  {
    AddonPtr scraperAddon;
    CAddonMgr::Get().GetDefault(ADDON::ScraperTypeFromContent(m_content), scraperAddon);
    m_scraper = std::dynamic_pointer_cast<CScraper>(scraperAddon);
  }

  for (IVECADDONS iter = m_scrapers.find(m_content)->second.begin(); iter != m_scrapers.find(m_content)->second.end(); ++iter)
  {
    CFileItemPtr item(new CFileItem((*iter)->Name()));
    item->SetPath((*iter)->ID());
    item->SetArt("thumb", (*iter)->Icon());
    if (m_scraper && (*iter)->ID() == m_scraper->ID())
    {
      item->Select(true);
      selectedIndex = iIndex;
    }
    m_vecItems->Add(item);

    iIndex++;
  }

  // add the "Get More..." item
  m_vecItems->Add(XFILE::CAddonsDirectory::GetMoreItem(ADDON::TranslateContent(m_content)));

  CGUIMessage msg(GUI_MSG_LABEL_BIND, GetID(), CONTROL_SCRAPER_LIST, 0, 0, m_vecItems);
  OnMessage(msg);
  CGUIMessage msg2(GUI_MSG_ITEM_SELECT, GetID(), CONTROL_SCRAPER_LIST, selectedIndex);
  OnMessage(msg2);
}
