/*
 *      Copyright (C) 2005-2014 Team XBMC
 *      http://xbmc.org
 *
 *      Copyright (C) 2014-2015 Aracnoz
 *      http://github.com/aracnoz/xbmc
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

#include "GUIDialogMadvrSettingsBase.h"
#include "Application.h"
#include "URL.h"
#include "dialogs/GUIDialogYesNo.h"
#include "filesystem/File.h"
#include "guilib/LocalizeStrings.h"
#include "profiles/ProfilesManager.h"
#include "settings/Settings.h"
#include "settings/lib/Setting.h"
#include "settings/lib/SettingsManager.h"
#include "utils/LangCodeExpander.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "guilib/GUIWindowManager.h"
#include "dialogs/GUIDialogSelect.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "input/Key.h"
#include "DSRendererCallback.h"
#include "settings/MediaSettings.h"
#include "settings/Settings.h"
#include "settings/lib/Setting.h"
#include "settings/lib/SettingsManager.h"
#include "addons/Skin.h"
#include "DSPlayerDatabase.h"

#define SETTING_VIDEO_MAKE_DEFAULT        "video.save"

using namespace std;

int CGUIDialogMadvrSettingsBase::m_iSectionId = -1;
int CGUIDialogMadvrSettingsBase::m_label = -1;

CGUIDialogMadvrSettingsBase::CGUIDialogMadvrSettingsBase(int windowId, const std::string &xmlFile)
  : CGUIDialogSettingsManualBase(windowId, xmlFile)
{
  m_bMadvr = false;
  m_iSectionIdInternal = -1;
}

CGUIDialogMadvrSettingsBase::~CGUIDialogMadvrSettingsBase()
{
}

void CGUIDialogMadvrSettingsBase::SaveControlStates()
{
  CGUIDialogSettingsManualBase::SaveControlStates();

  m_focusPositions[m_iSectionIdInternal] = GetFocusedControlID();
}

void CGUIDialogMadvrSettingsBase::RestoreControlStates()
{
  CGUIDialogSettingsManualBase::RestoreControlStates();

  const auto &it = m_focusPositions.find(m_iSectionIdInternal);
  if (it != m_focusPositions.end())
    SET_CONTROL_FOCUS(it->second, 0);
}

void CGUIDialogMadvrSettingsBase::InitializeSettings()
{
  CGUIDialogSettingsManualBase::InitializeSettings();

  m_bMadvr = CDSRendererCallback::Get()->UsingDS(DIRECTSHOW_RENDERER_MADVR) && (CSettings::GetInstance().GetInt(CSettings::SETTING_DSPLAYER_MANAGEMADVRWITHKODI) > KODIGUI_NEVER);
  m_iSectionIdInternal = m_iSectionId;

  bool usePopup = g_SkinInfo->HasSkinFile("DialogSlider.xml");

  m_category = AddCategory("videosettings", -1);
  if (m_category == NULL)
  {
    CLog::Log(LOGERROR, "CGUIDialogMadvrSettings: unable to setup settings");
    return;
  }

  if (!m_bMadvr)
    return;

  if (m_bMadvr && m_iSectionId == MADVR_VIDEO_ROOT)
  {
    CSettingGroup *groupMadvrSave = AddGroup(m_category);
    if (groupMadvrSave == NULL)
    {
      CLog::Log(LOGERROR, "CGUIDialogMadvrSettings: unable to setup settings");
      return;
    }
    //SAVE DEFAULT SETTINGS...
    AddButton(groupMadvrSave, SETTING_VIDEO_MAKE_DEFAULT, 70600, 0);
  }

  std::map<int, CSettingGroup *> groups;
  CMadvrSettings &madvrSettings = CMediaSettings::GetInstance().GetCurrentMadvrSettings();
  CDSRendererCallback::Get()->LoadSettings(m_iSectionId);

  for (const auto &it : madvrSettings.m_gui[m_iSectionId])
  {
    CSetting *setting;

    if (groups[it->group] == NULL)
    {
      groups[it->group] = AddGroup(m_category);
      if (groups[it->group] == NULL)
      {
        CLog::Log(LOGERROR, "CGUIDialogMadvrSettings: unable to setup settings");
        return;
      }
    }

    if (it->type.find("button_") != std::string::npos)
    {
      setting = AddButton(groups[it->group], it->dialogId, it->label, 0);
    }
    else if (it->type == "bool")
    {
      setting = AddToggle(groups[it->group], it->dialogId, it->label, 0, madvrSettings.m_db[it->name].asBoolean());
    }
    else if (it->type == "float")
    {
      setting = AddSlider(groups[it->group], it->dialogId, it->label, 0, madvrSettings.m_db[it->name].asFloat(),
        it->slider->format, it->slider->min, it->slider->step, it->slider->max, it->slider->parentLabel, usePopup);
    }
    else if (it->type.find("list_") != std::string::npos)
    {
      if (!it->optionsInt.empty())
        setting = AddList(groups[it->group], it->dialogId, it->label, 0, madvrSettings.m_db[it->name].asInteger(), it->optionsInt, it->label);
      else
        setting = AddList(groups[it->group], it->dialogId, it->label, 0, madvrSettings.m_db[it->name].asString(), MadvrSettingsOptionsString, it->label);
    }

    if (!it->dependencies.empty() && setting)
      CDSRendererCallback::Get()->AddDependencies(it->dependencies, m_settingsManager, setting);

    if (!it->parent.empty() && setting)
      setting->SetParent(it->parent);
  }
}

void CGUIDialogMadvrSettingsBase::OnSettingChanged(const CSetting *setting)
{
  if (setting == NULL)
    return;

  CGUIDialogSettingsManualBase::OnSettingChanged(setting);

  if (!m_bMadvr)
    return;

  CDSRendererCallback::Get()->OnSettingChanged(m_iSectionId, m_settingsManager, setting);
}

void CGUIDialogMadvrSettingsBase::OnSettingAction(const CSetting *setting)
{
  if (setting == NULL)
    return;

  CGUIDialogSettingsManualBase::OnSettingAction(setting);

  if (!m_bMadvr)
    return;

  CMadvrSettings &madvrSettings = CMediaSettings::GetInstance().GetCurrentMadvrSettings();

  const std::string &settingId = setting->GetId();

  auto it = std::find_if(madvrSettings.m_gui[m_iSectionId].begin(), madvrSettings.m_gui[m_iSectionId].end(),
    [settingId](const CMadvrListSettings* setting){
    return setting->dialogId == settingId;
  });

  if (it != madvrSettings.m_gui[m_iSectionId].end())
  {
    if ((*it)->type == "button_section")
    {  
      if (m_iSectionId == MADVR_VIDEO_ROOT)
      { 
        SetSection((*it)->sectionId, (*it)->label);
        g_windowManager.ActivateWindow(WINDOW_DIALOG_MADVR);
      }
      else
      {    
        SetSection((*it)->sectionId, (*it)->label);
        SaveControlStates();
        Close();
        Open();
      }
    }
    else if ((*it)->type == "button_debug")
    {
      CDSRendererCallback::Get()->ListSettings((*it)->value);
    }
  }
}

void CGUIDialogMadvrSettingsBase::SaveMadvrSettings()
{
  CGUIDialogSelect *pDlg = (CGUIDialogSelect *)g_windowManager.GetWindow(WINDOW_DIALOG_SELECT);
  if (!pDlg)
    return;

  CMadvrSettings &madvrSettings = CMediaSettings::GetInstance().GetCurrentMadvrSettings();

  if (!madvrSettings.m_TvShowName.empty())
    pDlg->Add(StringUtils::Format(g_localizeStrings.Get(70605).c_str(), madvrSettings.m_TvShowName.c_str()));

  pDlg->Add(g_localizeStrings.Get(70601).c_str());
  pDlg->Add(g_localizeStrings.Get(70602).c_str());
  pDlg->Add(g_localizeStrings.Get(70603).c_str());
  pDlg->Add(g_localizeStrings.Get(70604).c_str());
  pDlg->Add(g_localizeStrings.Get(70606).c_str());
  pDlg->Add(g_localizeStrings.Get(70607).c_str());

  pDlg->SetHeading(70600);
  pDlg->Open();

  if (pDlg->GetSelectedItem() < 0)
    return;

  int label;
  int selected = -1;
  std::string strSelected = pDlg->GetSelectedLabelText();

  //TVSHOW
  if (strSelected == StringUtils::Format(g_localizeStrings.Get(70605).c_str(), madvrSettings.m_TvShowName.c_str()))
  {
    selected = MADVR_RES_TVSHOW;
    label = 70605;
  }
  //SD
  else if (strSelected == g_localizeStrings.Get(70601))
  {
    selected = MADVR_RES_SD;
    label = 70601;
  }
  //720
  else if (strSelected == g_localizeStrings.Get(70602))
  {
    selected = MADVR_RES_720;
    label = 70602;
  }
  //1080
  else if (strSelected == g_localizeStrings.Get(70603))
  {
    selected = MADVR_RES_1080;
    label = 70603;
  }
  //2160
  else if (strSelected == g_localizeStrings.Get(70604))
  {
    selected = MADVR_RES_2160;
    label = 70604;
  }
  //ALL
  else if (strSelected == g_localizeStrings.Get(70606))
  {
    selected = MADVR_RES_ALL;
    label = 70606;
  }
  //RESET TO DEFAULT
  else if (strSelected == g_localizeStrings.Get(70607))
  {
    selected = MADVR_RES_DEFAULT;
    label = 70607;
  }

  if (selected > -1)
  {
    if (CGUIDialogYesNo::ShowAndGetInput(StringUtils::Format(g_localizeStrings.Get(label).c_str(), madvrSettings.m_TvShowName.c_str()), 750, 0, 12377))
    {
      CDSPlayerDatabase dspdb;
      if (!dspdb.Open())
        return;

      if (selected == MADVR_RES_TVSHOW)
      {
        dspdb.EraseTvShowSettings(madvrSettings.m_TvShowName.c_str());
        dspdb.SetTvShowSettings(madvrSettings.m_TvShowName.c_str(), madvrSettings);
      }
      else if (selected == MADVR_RES_ALL)
      {
        dspdb.EraseVideoSettings();
        dspdb.SetResSettings(selected, madvrSettings);
      }
      else if (selected == MADVR_RES_DEFAULT)
      {
        dspdb.EraseVideoSettings();
        CMediaSettings::GetInstance().GetCurrentMadvrSettings().RestoreDefaultSettings();
        CDSRendererCallback::Get()->RestoreSettings();
        Close();
      }
      else
      {
        dspdb.EraseResSettings(selected);
        dspdb.SetResSettings(selected, madvrSettings);
      }
      dspdb.Close();
    }
  }
}

void CGUIDialogMadvrSettingsBase::MadvrSettingsOptionsString(const CSetting *setting, std::vector< std::pair<std::string, std::string> > &list, std::string &current, void *data)
{
  CMadvrSettings &madvrSettings = CMediaSettings::GetInstance().GetCurrentMadvrSettings();

  const std::string &settingId = setting->GetId();

  auto it = std::find_if(madvrSettings.m_gui[m_iSectionId].begin(), madvrSettings.m_gui[m_iSectionId].end(),
    [settingId](const CMadvrListSettings* setting){
    return setting->dialogId == settingId;
  });

  if (it != madvrSettings.m_gui[m_iSectionId].end())
  {
    for (const auto &option : (*it)->optionsString)
      list.push_back(std::make_pair(g_localizeStrings.Get(option.first), option.second));
  }
}