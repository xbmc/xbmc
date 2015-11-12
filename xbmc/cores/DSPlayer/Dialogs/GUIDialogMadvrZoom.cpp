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

#include "GUIDialogMadvrZoom.h"
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
#include "MadvrCallback.h"
#include "settings/MediaSettings.h"
#include "settings/Settings.h"
#include "settings/lib/Setting.h"
#include "settings/lib/SettingsManager.h"
#include "addons/Skin.h"

#define SET_ZOOM_NOSMALLSCALING           "madvr.nosmallscaling"
#define SET_ZOOM_MOVESUBS                 "madvr.movesubs"
#define SET_ZOOM_DETECTBARS               "madvr.detectbars"

#define SET_ZOOM_ARCHANGE                 "madvr.archange"
#define SET_ZOOM_QUICKARCHANGE            "madvr.quickarchange"
#define SET_ZOOM_SHIFTIMAGE               "madvr.shiftimage"
#define SET_ZOOM_DONTCROPSUBS             "madvr.dontcropsubs"
#define SET_ZOOM_CLEANBORDERS             "madvr.cleanborders"
#define SET_ZOOM_REDUCEBIGBARS            "madvr.reducebigbars"
#define SET_ZOOM_CROPSMALLBARS            "madvr.cropsmallbars"
#define SET_ZOOM_CROPBARS                 "madvr.cropbars"

#ifndef countof
#define countof(array) (sizeof(array)/sizeof(array[0]))
#endif

using namespace std;

CGUIDialogMadvrZoom::CGUIDialogMadvrZoom()
  : CGUIDialogSettingsManualBase(WINDOW_DIALOG_MADVRZOOM, "VideoOSDSettings.xml")
{
  m_allowchange = true;
}


CGUIDialogMadvrZoom::~CGUIDialogMadvrZoom()
{ }

void CGUIDialogMadvrZoom::OnInitWindow()
{
  CGUIDialogSettingsManualBase::OnInitWindow();

  HideUnused();
}

void CGUIDialogMadvrZoom::SetupView()
{
  CGUIDialogSettingsManualBase::SetupView();

  SetHeading(70264);

  if (CSettings::GetInstance().GetInt(CSettings::SETTING_DSPLAYER_MANAGEMADVRWITHKODI) == KODIGUI_LOAD_MADVR)
  {
    std::string profile;
    CMadvrCallback::Get()->GetProfileActiveName("processing", &profile);
    if (profile != "")
    {
      CStdString sHeading;
      sHeading.Format("%s: %s", g_localizeStrings.Get(20093).c_str(), profile.c_str());
      SetHeading(sHeading);
    }
  }
}

void CGUIDialogMadvrZoom::InitializeSettings()
{
  CGUIDialogSettingsManualBase::InitializeSettings();

  bool usePopup = g_SkinInfo->HasSkinFile("DialogSlider.xml");

  CSettingCategory *category = AddCategory("dsplayermadvrzoom", -1);
  if (category == NULL)
  {
    CLog::Log(LOGERROR, "CGUIDialogMadvrZoom: unable to setup settings");
    return;
  }

  // get all necessary setting groups
  CSettingGroup *groupMadvrZoomArea = AddGroup(category);
  if (groupMadvrZoomArea == NULL)
  {
    CLog::Log(LOGERROR, "CGUIDialogMadvrZoom: unable to setup settings");
    return;
  }
  CSettingGroup *groupMadvrZoomControl = AddGroup(category);
  if (groupMadvrZoomControl == NULL)
  {
    CLog::Log(LOGERROR, "CGUIDialogMadvrZoom: unable to setup settings");
    return;
  }

  StaticIntegerSettingOptions entries, entriesDoubleFactor, entriesQuadrupleFactor;
  CMadvrSettings &madvrSettings = CMediaSettings::GetInstance().GetCurrentMadvrSettings();
  CMadvrCallback::Get()->LoadSettings(MADVR_LOAD_ZOOM);

  // NOSMALLSCALING
  entries.clear();
  entries.push_back(std::make_pair(70117, -1));
  CMadvrCallback::Get()->AddEntry(MADVR_LIST_NOSMALLSCALING, &entries);
  AddList(groupMadvrZoomArea, SET_ZOOM_NOSMALLSCALING, 70208, 0, static_cast<int>(madvrSettings.m_noSmallScaling), entries, 70208);

  // MOVESUBS
  entries.clear();
  entries.push_back(std::make_pair(70117, -1));
  CMadvrCallback::Get()->AddEntry(MADVR_LIST_MOVESUBS, &entries);
  AddList(groupMadvrZoomArea, SET_ZOOM_MOVESUBS, 70217, 0, static_cast<int>(madvrSettings.m_moveSubs), entries, 70217);

  // AUTO DETECT BARS
  AddToggle(groupMadvrZoomArea, SET_ZOOM_DETECTBARS, 70220, 0, madvrSettings.m_detectBars);

  // ARCHANGE
  entries.clear();
  entries.push_back(std::make_pair(70117, -1));
  CMadvrCallback::Get()->AddEntry(MADVR_LIST_ARCHANGE, &entries);
  AddList(groupMadvrZoomControl, SET_ZOOM_ARCHANGE, 70221, 0, static_cast<int>(madvrSettings.m_arChange), entries, 70221);

  // QUICK ARCHANGE
  entries.clear();
  entries.push_back(std::make_pair(70117, -1));
  CMadvrCallback::Get()->AddEntry(MADVR_LIST_QUICKARCHANGE, &entries);
  AddList(groupMadvrZoomControl, SET_ZOOM_QUICKARCHANGE, 70227, 0, static_cast<int>(madvrSettings.m_quickArChange), entries, 70227);

  // SHIFTIMAGE
  entries.clear();
  entries.push_back(std::make_pair(70117, -1));
  CMadvrCallback::Get()->AddEntry(MADVR_LIST_SHIFTIMAGE, &entries);
  AddList(groupMadvrZoomControl, SET_ZOOM_SHIFTIMAGE, 70232, 0, static_cast<int>(madvrSettings.m_shiftImage), entries, 70232);

  // DONT CROP SUBS
  entries.clear();
  entries.push_back(std::make_pair(70117, -1));
  CMadvrCallback::Get()->AddEntry(MADVR_LIST_DONTCROPSUBS, &entries);
  AddList(groupMadvrZoomControl, SET_ZOOM_DONTCROPSUBS, 70235, 0, static_cast<int>(madvrSettings.m_dontCropSubs), entries, 70235);

  // CLEAN IMAGE BORDERS
  entries.clear();
  entries.push_back(std::make_pair(70117, -1));
  CMadvrCallback::Get()->AddEntry(MADVR_LIST_CLEANBORDERS, &entries);
  AddList(groupMadvrZoomControl, SET_ZOOM_CLEANBORDERS, 70244, 0, static_cast<int>(madvrSettings.m_cleanBorders), entries, 70244);

  // REDUCE BIG BARS
  entries.clear();
  entries.push_back(std::make_pair(70117, -1));
  CMadvrCallback::Get()->AddEntry(MADVR_LIST_REDUCEBIGBARS, &entries);
  AddList(groupMadvrZoomControl, SET_ZOOM_REDUCEBIGBARS, 70257, 0, static_cast<int>(madvrSettings.m_reduceBigBars), entries, 70257);

  // CROP SMALL BARS
  AddToggle(groupMadvrZoomControl, SET_ZOOM_CROPSMALLBARS, 70262, 0, madvrSettings.m_cropSmallBars);

  // CROP BARS
  AddToggle(groupMadvrZoomControl, SET_ZOOM_CROPBARS, 70263, 0, madvrSettings.m_cropBars);
}

void CGUIDialogMadvrZoom::OnSettingChanged(const CSetting *setting)
{
  if (setting == NULL)
    return;

  CGUIDialogSettingsManualBase::OnSettingChanged(setting);

  CMadvrSettings &madvrSettings = CMediaSettings::GetInstance().GetCurrentMadvrSettings();

  const std::string &settingId = setting->GetId();

  if (settingId == SET_ZOOM_NOSMALLSCALING)
  {
    madvrSettings.m_noSmallScaling = static_cast<int>(static_cast<const CSettingInt*>(setting)->GetValue());
    CMadvrCallback::Get()->SetBoolValue("noSmallScaling", "noSmallScalingValue", madvrSettings.m_noSmallScaling);
  }
  else if (settingId == SET_ZOOM_MOVESUBS)
  {
    madvrSettings.m_moveSubs = static_cast<int>(static_cast<const CSettingInt*>(setting)->GetValue());
    CMadvrCallback::Get()->SetMultiBool("moveSubs", "moveSubsUp", madvrSettings.m_moveSubs);
  }
  else if (settingId == SET_ZOOM_DETECTBARS)
  {
    madvrSettings.m_detectBars = static_cast<const CSettingBool*>(setting)->GetValue();
    CMadvrCallback::Get()->SetBool("detectBars", madvrSettings.m_detectBars);
  }
  else if (settingId == SET_ZOOM_ARCHANGE)
  {
    madvrSettings.m_arChange = static_cast<int>(static_cast<const CSettingInt*>(setting)->GetValue());
    CMadvrCallback::Get()->SetBoolValue("arChange", "arChangeValue", madvrSettings.m_arChange);
  }
  else if (settingId == SET_ZOOM_QUICKARCHANGE)
  {
    madvrSettings.m_quickArChange = static_cast<int>(static_cast<const CSettingInt*>(setting)->GetValue());
    CMadvrCallback::Get()->SetQuickArChange("", madvrSettings.m_quickArChange);
  }
  else if (settingId == SET_ZOOM_SHIFTIMAGE)
  {
    madvrSettings.m_shiftImage = static_cast<int>(static_cast<const CSettingInt*>(setting)->GetValue());
    CMadvrCallback::Get()->SetMultiBool("shiftImage", "shiftImageUp", madvrSettings.m_shiftImage);
  }
  else if (settingId == SET_ZOOM_DONTCROPSUBS)
  {
    madvrSettings.m_dontCropSubs = static_cast<int>(static_cast<const CSettingInt*>(setting)->GetValue());
    CMadvrCallback::Get()->SetBoolValue("dontCropSubs", "dontCropSubsValue", madvrSettings.m_dontCropSubs);
  }
  else if (settingId == SET_ZOOM_CLEANBORDERS)
  {
    madvrSettings.m_cleanBorders = static_cast<int>(static_cast<const CSettingInt*>(setting)->GetValue());
    CMadvrCallback::Get()->SetCleanBorders("", madvrSettings.m_cleanBorders);
  }
  else if (settingId == SET_ZOOM_REDUCEBIGBARS)
  {
    madvrSettings.m_reduceBigBars = static_cast<int>(static_cast<const CSettingInt*>(setting)->GetValue());
    CMadvrCallback::Get()->SetBoolValue("reduceBigBars", "reduceBigBarsValues", madvrSettings.m_reduceBigBars);
  }
  else if (settingId == SET_ZOOM_CROPSMALLBARS)
  {
    madvrSettings.m_cropSmallBars = static_cast<const CSettingBool*>(setting)->GetValue();
    CMadvrCallback::Get()->SetBool("cropSmallBars", madvrSettings.m_cropSmallBars);
  }
  else if (settingId == SET_ZOOM_CROPBARS)
  {
    madvrSettings.m_cropBars = static_cast<const CSettingBool*>(setting)->GetValue();
    CMadvrCallback::Get()->SetBool("cropBars", madvrSettings.m_cropBars);
  }
  HideUnused();
}

void CGUIDialogMadvrZoom::OnSettingAction(const CSetting *setting)
{
  if (setting == NULL)
    return;
}

void CGUIDialogMadvrZoom::HideUnused()
{
  if (!m_allowchange)
    return;

  m_allowchange = false;

  int iValue;
  bool bValue;
  CSetting *setting;

  // HIDE / SHOW
  setting = m_settingsManager->GetSetting(SET_ZOOM_ARCHANGE);
  iValue = static_cast<const CSettingInt*>(setting)->GetValue();
  setting = m_settingsManager->GetSetting(SET_ZOOM_DETECTBARS);
  bValue = static_cast<const CSettingBool*>(setting)->GetValue();
  SetEnabled(SET_ZOOM_ARCHANGE, bValue);

  SetEnabled(SET_ZOOM_QUICKARCHANGE, bValue && (iValue == -1), (iValue != -1 && bValue));
  SetEnabled(SET_ZOOM_SHIFTIMAGE, bValue);
  SetEnabled(SET_ZOOM_DONTCROPSUBS, bValue);
  SetEnabled(SET_ZOOM_CLEANBORDERS, bValue);
  SetEnabled(SET_ZOOM_REDUCEBIGBARS, bValue);
  SetEnabled(SET_ZOOM_CROPSMALLBARS, bValue);
  SetEnabled(SET_ZOOM_CROPBARS, bValue);

  m_allowchange = true;
}

void CGUIDialogMadvrZoom::SetEnabled(CStdString id, bool bEnabled, bool bReset)
{
  CSetting *setting = m_settingsManager->GetSetting(id);
  if (setting->IsEnabled() && bEnabled)
    return;

  setting->SetVisible(true);
  setting->SetEnabled(bEnabled);
  
  if (bReset)
    m_settingsManager->SetInt(id, -1);
}
