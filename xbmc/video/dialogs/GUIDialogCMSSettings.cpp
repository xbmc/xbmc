/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIDialogCMSSettings.h"

#include "FileItem.h"
#include "GUIPassword.h"
#include "ServiceBroker.h"
#include "addons/Skin.h"
#include "cores/VideoPlayer/VideoRenderers/ColorManager.h"
#include "cores/VideoPlayer/VideoRenderers/RenderManager.h"
#include "filesystem/Directory.h"
#include "guilib/GUIWindowManager.h"
#include "profiles/ProfileManager.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "settings/lib/Setting.h"
#include "settings/lib/SettingDefinitions.h"
#include "settings/lib/SettingsManager.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "utils/log.h"
#include "video/VideoDatabase.h"

#include <memory>
#include <vector>

#define SETTING_VIDEO_CMSENABLE           "videoscreen.cmsenabled"
#define SETTING_VIDEO_CMSMODE             "videoscreen.cmsmode"
#define SETTING_VIDEO_CMS3DLUT            "videoscreen.cms3dlut"
#define SETTING_VIDEO_CMSWHITEPOINT       "videoscreen.cmswhitepoint"
#define SETTING_VIDEO_CMSPRIMARIES        "videoscreen.cmsprimaries"
#define SETTING_VIDEO_CMSGAMMAMODE        "videoscreen.cmsgammamode"
#define SETTING_VIDEO_CMSGAMMA            "videoscreen.cmsgamma"
#define SETTING_VIDEO_CMSLUTSIZE          "videoscreen.cmslutsize"

CGUIDialogCMSSettings::CGUIDialogCMSSettings()
    : CGUIDialogSettingsManualBase(WINDOW_DIALOG_CMS_OSD_SETTINGS, "DialogSettings.xml")
{ }

CGUIDialogCMSSettings::~CGUIDialogCMSSettings() = default;

void CGUIDialogCMSSettings::SetupView()
{
  CGUIDialogSettingsManualBase::SetupView();

  SetHeading(36560);
  SET_CONTROL_HIDDEN(CONTROL_SETTINGS_OKAY_BUTTON);
  SET_CONTROL_HIDDEN(CONTROL_SETTINGS_CUSTOM_BUTTON);
  SET_CONTROL_LABEL(CONTROL_SETTINGS_CANCEL_BUTTON, 15067);
}

void CGUIDialogCMSSettings::InitializeSettings()
{
  CGUIDialogSettingsManualBase::InitializeSettings();

  const std::shared_ptr<CSettingCategory> category = AddCategory("cms", -1);
  if (category == NULL)
  {
    CLog::Log(LOGERROR, "CGUIDialogCMSSettings: unable to setup settings");
    return;
  }

  // get all necessary setting groups
  const std::shared_ptr<CSettingGroup> groupColorManagement = AddGroup(category);
  if (groupColorManagement == NULL)
  {
    CLog::Log(LOGERROR, "CGUIDialogCMSSettings: unable to setup settings");
    return;
  }

  bool usePopup = g_SkinInfo->HasSkinFile("DialogSlider.xml");

  TranslatableIntegerSettingOptions entries;

  // create "depsCmsEnabled" for settings depending on CMS being enabled
  CSettingDependency dependencyCmsEnabled(SettingDependencyType::Enable, GetSettingsManager());
  dependencyCmsEnabled.Or()->Add(std::make_shared<CSettingDependencyCondition>(
      SETTING_VIDEO_CMSENABLE, "true", SettingDependencyOperator::Equals, false,
      GetSettingsManager()));
  SettingDependencies depsCmsEnabled;
  depsCmsEnabled.push_back(dependencyCmsEnabled);

  // create "depsCms3dlut" for 3dlut settings
  CSettingDependency dependencyCms3dlut(SettingDependencyType::Visible, GetSettingsManager());
  dependencyCms3dlut.And()->Add(std::make_shared<CSettingDependencyCondition>(
      SETTING_VIDEO_CMSMODE, std::to_string(CMS_MODE_3DLUT), SettingDependencyOperator::Equals,
      false, GetSettingsManager()));
  SettingDependencies depsCms3dlut;
  depsCms3dlut.push_back(dependencyCmsEnabled);
  depsCms3dlut.push_back(dependencyCms3dlut);

  // create "depsCmsIcc" for display settings with icc profile
  CSettingDependency dependencyCmsIcc(SettingDependencyType::Visible, GetSettingsManager());
  dependencyCmsIcc.And()->Add(std::make_shared<CSettingDependencyCondition>(
      SETTING_VIDEO_CMSMODE, std::to_string(CMS_MODE_PROFILE), SettingDependencyOperator::Equals,
      false, GetSettingsManager()));
  SettingDependencies depsCmsIcc;
  depsCmsIcc.push_back(dependencyCmsEnabled);
  depsCmsIcc.push_back(dependencyCmsIcc);

  // create "depsCmsGamma" for effective gamma adjustment (not available with bt.1886)
  CSettingDependency dependencyCmsGamma(SettingDependencyType::Visible, GetSettingsManager());
  dependencyCmsGamma.And()->Add(std::make_shared<CSettingDependencyCondition>(
      SETTING_VIDEO_CMSGAMMAMODE, std::to_string(CMS_TRC_BT1886), SettingDependencyOperator::Equals,
      true, GetSettingsManager()));
  SettingDependencies depsCmsGamma;
  depsCmsGamma.push_back(dependencyCmsEnabled);
  depsCmsGamma.push_back(dependencyCmsIcc);
  depsCmsGamma.push_back(dependencyCmsGamma);

  const std::shared_ptr<CSettings> settings = CServiceBroker::GetSettingsComponent()->GetSettings();

  // color management settings
  AddToggle(groupColorManagement, SETTING_VIDEO_CMSENABLE, 36560, SettingLevel::Basic, settings->GetBool(SETTING_VIDEO_CMSENABLE));

  int currentMode = settings->GetInt(SETTING_VIDEO_CMSMODE);
  entries.clear();
  // entries.push_back(TranslatableIntegerSettingOption(16039, CMS_MODE_OFF)); // FIXME: get from CMS class
  entries.push_back(TranslatableIntegerSettingOption(36580, CMS_MODE_3DLUT));
#ifdef HAVE_LCMS2
  entries.push_back(TranslatableIntegerSettingOption(36581, CMS_MODE_PROFILE));
#endif
  std::shared_ptr<CSettingInt> settingCmsMode = AddSpinner(groupColorManagement, SETTING_VIDEO_CMSMODE, 36562, SettingLevel::Basic, currentMode, entries);
  settingCmsMode->SetDependencies(depsCmsEnabled);

  std::string current3dLUT = settings->GetString(SETTING_VIDEO_CMS3DLUT);
  std::shared_ptr<CSettingString> settingCms3dlut = AddList(groupColorManagement, SETTING_VIDEO_CMS3DLUT, 36564, SettingLevel::Basic, current3dLUT, Cms3dLutsFiller, 36564);
  settingCms3dlut->SetDependencies(depsCms3dlut);

  // display settings
  int currentWhitepoint = settings->GetInt(SETTING_VIDEO_CMSWHITEPOINT);
  entries.clear();
  entries.push_back(TranslatableIntegerSettingOption(36586, CMS_WHITEPOINT_D65));
  entries.push_back(TranslatableIntegerSettingOption(36587, CMS_WHITEPOINT_D93));
  std::shared_ptr<CSettingInt> settingCmsWhitepoint = AddSpinner(groupColorManagement, SETTING_VIDEO_CMSWHITEPOINT, 36568, SettingLevel::Basic, currentWhitepoint, entries);
  settingCmsWhitepoint->SetDependencies(depsCmsIcc);

  int currentPrimaries = settings->GetInt(SETTING_VIDEO_CMSPRIMARIES);
  entries.clear();
  entries.push_back(TranslatableIntegerSettingOption(36588, CMS_PRIMARIES_AUTO));
  entries.push_back(TranslatableIntegerSettingOption(36589, CMS_PRIMARIES_BT709));
  entries.push_back(TranslatableIntegerSettingOption(36579, CMS_PRIMARIES_BT2020));
  entries.push_back(TranslatableIntegerSettingOption(36590, CMS_PRIMARIES_170M));
  entries.push_back(TranslatableIntegerSettingOption(36591, CMS_PRIMARIES_BT470M));
  entries.push_back(TranslatableIntegerSettingOption(36592, CMS_PRIMARIES_BT470BG));
  entries.push_back(TranslatableIntegerSettingOption(36593, CMS_PRIMARIES_240M));
  std::shared_ptr<CSettingInt> settingCmsPrimaries = AddSpinner(groupColorManagement, SETTING_VIDEO_CMSPRIMARIES, 36570, SettingLevel::Basic, currentPrimaries, entries);
  settingCmsPrimaries->SetDependencies(depsCmsIcc);

  int currentGammaMode = settings->GetInt(SETTING_VIDEO_CMSGAMMAMODE);
  entries.clear();
  entries.push_back(TranslatableIntegerSettingOption(36582, CMS_TRC_BT1886));
  entries.push_back(TranslatableIntegerSettingOption(36583, CMS_TRC_INPUT_OFFSET));
  entries.push_back(TranslatableIntegerSettingOption(36584, CMS_TRC_OUTPUT_OFFSET));
  entries.push_back(TranslatableIntegerSettingOption(36585, CMS_TRC_ABSOLUTE));
  std::shared_ptr<CSettingInt> settingCmsGammaMode = AddSpinner(groupColorManagement, SETTING_VIDEO_CMSGAMMAMODE, 36572, SettingLevel::Basic, currentGammaMode, entries);
  settingCmsGammaMode->SetDependencies(depsCmsIcc);

  float currentGamma = settings->GetInt(SETTING_VIDEO_CMSGAMMA)/100.0f;
  if (currentGamma == 0.0f)
    currentGamma = 2.20f;
  std::shared_ptr<CSettingNumber> settingCmsGamma = AddSlider(groupColorManagement, SETTING_VIDEO_CMSGAMMA, 36574, SettingLevel::Basic, currentGamma, 36597, 1.6, 0.05, 2.8, 36574, usePopup);
  settingCmsGamma->SetDependencies(depsCmsGamma);

  int currentLutSize = settings->GetInt(SETTING_VIDEO_CMSLUTSIZE);
  entries.clear();
  entries.push_back(TranslatableIntegerSettingOption(36594, 4));
  entries.push_back(TranslatableIntegerSettingOption(36595, 6));
  entries.push_back(TranslatableIntegerSettingOption(36596, 8));
  std::shared_ptr<CSettingInt> settingCmsLutSize = AddSpinner(groupColorManagement, SETTING_VIDEO_CMSLUTSIZE, 36576, SettingLevel::Basic, currentLutSize, entries);
  settingCmsLutSize->SetDependencies(depsCmsIcc);
}

void CGUIDialogCMSSettings::OnSettingChanged(const std::shared_ptr<const CSetting>& setting)
{
  if (setting == NULL)
    return;

  CGUIDialogSettingsManualBase::OnSettingChanged(setting);

  const std::string &settingId = setting->GetId();
  if (settingId == SETTING_VIDEO_CMSENABLE)
    CServiceBroker::GetSettingsComponent()->GetSettings()->SetBool(SETTING_VIDEO_CMSENABLE, (std::static_pointer_cast<const CSettingBool>(setting)->GetValue()));
  else if (settingId == SETTING_VIDEO_CMSMODE)
    CServiceBroker::GetSettingsComponent()->GetSettings()->SetInt(SETTING_VIDEO_CMSMODE, std::static_pointer_cast<const CSettingInt>(setting)->GetValue());
  else if (settingId == SETTING_VIDEO_CMS3DLUT)
    CServiceBroker::GetSettingsComponent()->GetSettings()->SetString(SETTING_VIDEO_CMS3DLUT, std::static_pointer_cast<const CSettingString>(setting)->GetValue());
  else if (settingId == SETTING_VIDEO_CMSWHITEPOINT)
    CServiceBroker::GetSettingsComponent()->GetSettings()->SetInt(SETTING_VIDEO_CMSWHITEPOINT, std::static_pointer_cast<const CSettingInt>(setting)->GetValue());
  else if (settingId == SETTING_VIDEO_CMSPRIMARIES)
    CServiceBroker::GetSettingsComponent()->GetSettings()->SetInt(SETTING_VIDEO_CMSPRIMARIES, std::static_pointer_cast<const CSettingInt>(setting)->GetValue());
  else if (settingId == SETTING_VIDEO_CMSGAMMAMODE)
    CServiceBroker::GetSettingsComponent()->GetSettings()->SetInt(SETTING_VIDEO_CMSGAMMAMODE, std::static_pointer_cast<const CSettingInt>(setting)->GetValue());
  else if (settingId == SETTING_VIDEO_CMSGAMMA)
    CServiceBroker::GetSettingsComponent()->GetSettings()->SetInt(SETTING_VIDEO_CMSGAMMA, static_cast<float>(std::static_pointer_cast<const CSettingNumber>(setting)->GetValue())*100);
  else if (settingId == SETTING_VIDEO_CMSLUTSIZE)
    CServiceBroker::GetSettingsComponent()->GetSettings()->SetInt(SETTING_VIDEO_CMSLUTSIZE, std::static_pointer_cast<const CSettingInt>(setting)->GetValue());
}

bool CGUIDialogCMSSettings::OnBack(int actionID)
{
  Save();
  return CGUIDialogSettingsBase::OnBack(actionID);
}

bool CGUIDialogCMSSettings::Save()
{
  CLog::Log(LOGINFO, "CGUIDialogCMSSettings: Save() called");
  CServiceBroker::GetSettingsComponent()->GetSettings()->Save();

  return true;
}

void CGUIDialogCMSSettings::Cms3dLutsFiller(const SettingConstPtr& setting,
                                            std::vector<StringSettingOption>& list,
                                            std::string& current,
                                            void* data)
{
  // get 3dLut directory from settings
  CFileItemList items;

  // list .3dlut files
  std::string current3dlut = CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(SETTING_VIDEO_CMS3DLUT);
  if (!current3dlut.empty())
    current3dlut = URIUtils::GetDirectory(current3dlut);
  XFILE::CDirectory::GetDirectory(current3dlut, items, ".3dlut", XFILE::DIR_FLAG_DEFAULTS);

  for (int i = 0; i < items.Size(); i++)
  {
    list.emplace_back(items[i]->GetLabel(), items[i]->GetPath());
  }
}
