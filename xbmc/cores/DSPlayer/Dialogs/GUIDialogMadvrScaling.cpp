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

#include "GUIDialogMadvrScaling.h"
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

#define SET_CHROMA_UPSCALING                   "madvr.chromaupscaling"
#define SET_CHROMA_ANTIRING                    "madvr.chromaantiring"
#define SET_CHROMA_SUPER_RES                   "madvr.chromasuperres"

#define SET_IMAGE_DOUBLING                     "madvr.imagedoubling"

#define SET_IMAGE_UPSCALING                    "madvr.imageupscaling"
#define SET_IMAGE_UP_ANTIRING                  "madvr.imageupantiring"
#define SET_IMAGE_UP_LINEAR                    "madvr.imageuplinear"

#define SET_IMAGE_DOWNSCALING                  "madvr.imagedownscaling"
#define SET_IMAGE_DOWN_ANTIRING                "madvr.imagedownantiring"
#define SET_IMAGE_DOWN_LINEAR                  "madvr.imagedownlinear"

#define SET_IMAGE_DOUBLE_LUMA                  "madvr.imagedoubleluma"
#define SET_IMAGE_DOUBLE_CHROMA                "madvr.imagedoublechroma"
#define SET_IMAGE_QUADRUPLE_LUMA               "madvr.imagequadrupleluma"
#define SET_IMAGE_QUADRUPLE_CHROMA             "madvr.imagequadruplechroma"

#define SET_IMAGE_DOUBLE_LUMA_FACTOR           "madvr.imagedoublelumafactor"
#define SET_IMAGE_DOUBLE_CHROMA_FACTOR         "madvr.imagedoublechromafactor"
#define SET_IMAGE_QUADRUPLE_LUMA_FACTOR        "madvr.imagequadruplelumafactor"
#define SET_IMAGE_QUADRUPLE_CHROMA_FACTOR      "madvr.imagequadruplechromafactor"

#define SET_IMAGE_FINESHARP                    "madvr.finsharp"
#define SET_IMAGE_FINESHARP_STRENGTH           "madvr.finsharpstrength"
#define SET_IMAGE_LUMASHARPEN                  "madvr.lumasharpen"
#define SET_IMAGE_LUMASHARPEN_STRENGTH         "madvr.lumasharpenstrength"
#define SET_IMAGE_ADAPTIVESHARPEN              "madvr.adaptivesharpen"
#define SET_IMAGE_ADAPTIVESHARPEN_STRENGTH     "madvr.adaptivesharpenstrength"

#define SET_IMAGE_UPFINESHARP                  "madvr.upfinsharp"
#define SET_IMAGE_UPFINESHARP_STRENGTH         "madvr.upfinsharpstrength"
#define SET_IMAGE_UPLUMASHARPEN                "madvr.uplumasharpen"
#define SET_IMAGE_UPLUMASHARPEN_STRENGTH       "madvr.uplumasharpenstrength"
#define SET_IMAGE_UPADAPTIVESHARPEN            "madvr.upadaptivesharpen"
#define SET_IMAGE_UPADAPTIVESHARPEN_STRENGTH   "madvr.upadaptivesharpenstrength"
#define SET_IMAGE_SUPER_RES                    "madvr.superres"
#define SET_IMAGE_SUPER_RES_STRENGTH           "madvr.superresstrength"

#define SET_IMAGE_REFINE_ONCE                  "madvr.refineonce"
#define SET_IMAGE_SUPER_RES_FIRST              "madvr.superresfirst"

#ifndef countof
#define countof(array) (sizeof(array)/sizeof(array[0]))
#endif

using namespace std;

CGUIDialogMadvrScaling::CGUIDialogMadvrScaling()
  : CGUIDialogSettingsManualBase(WINDOW_DIALOG_MADVR, "VideoOSDSettings.xml")
{
  m_allowchange = true;
}


CGUIDialogMadvrScaling::~CGUIDialogMadvrScaling()
{ }

void CGUIDialogMadvrScaling::OnInitWindow()
{
  CGUIDialogSettingsManualBase::OnInitWindow();

  LoadMadvrSettings();
  HideUnused();
}

void CGUIDialogMadvrScaling::LoadMadvrSettings()
{
  if (CSettings::GetInstance().GetInt(CSettings::SETTING_DSPLAYER_MANAGEMADVRWITHKODI) != KODIGUI_LOAD_MADVR)
    return;

  CMadvrCallback::Get()->LoadSettings(MADVR_LOAD_SCALING);
  CMadvrSettings &madvrSettings = CMediaSettings::GetInstance().GetCurrentMadvrSettings();

  m_settingsManager->SetInt(SET_CHROMA_UPSCALING, madvrSettings.m_ChromaUpscaling);
  m_settingsManager->SetBool(SET_CHROMA_ANTIRING, madvrSettings.m_ChromaAntiRing);
  m_settingsManager->SetBool(SET_CHROMA_SUPER_RES, madvrSettings.m_ChromaSuperRes);
  m_settingsManager->SetInt(SET_IMAGE_UPSCALING, madvrSettings.m_ImageUpscaling);
  m_settingsManager->SetBool(SET_IMAGE_UP_ANTIRING, madvrSettings.m_ImageUpAntiRing);
  m_settingsManager->SetBool(SET_IMAGE_UP_LINEAR, madvrSettings.m_ImageUpLinear);
  m_settingsManager->SetInt(SET_IMAGE_DOWNSCALING, madvrSettings.m_ImageDownscaling);
  m_settingsManager->SetBool(SET_IMAGE_DOWN_ANTIRING, madvrSettings.m_ImageDownAntiRing);
  m_settingsManager->SetBool(SET_IMAGE_DOWN_LINEAR, madvrSettings.m_ImageDownLinear);
  m_settingsManager->SetInt(SET_IMAGE_DOUBLE_LUMA, madvrSettings.m_ImageDoubleLuma);
  m_settingsManager->SetInt(SET_IMAGE_DOUBLE_LUMA_FACTOR, madvrSettings.m_ImageDoubleLumaFactor);
  m_settingsManager->SetInt(SET_IMAGE_DOUBLE_CHROMA, madvrSettings.m_ImageDoubleChroma);
  m_settingsManager->SetInt(SET_IMAGE_DOUBLE_CHROMA_FACTOR, madvrSettings.m_ImageDoubleChromaFactor);
  m_settingsManager->SetInt(SET_IMAGE_QUADRUPLE_LUMA, madvrSettings.m_ImageQuadrupleLuma);
  m_settingsManager->SetInt(SET_IMAGE_QUADRUPLE_LUMA_FACTOR, madvrSettings.m_ImageQuadrupleLumaFactor);
  m_settingsManager->SetInt(SET_IMAGE_QUADRUPLE_CHROMA, madvrSettings.m_ImageQuadrupleChroma);
  m_settingsManager->SetInt(SET_IMAGE_QUADRUPLE_CHROMA_FACTOR, madvrSettings.m_ImageQuadrupleChromaFactor);
  m_settingsManager->SetBool(SET_IMAGE_FINESHARP, madvrSettings.m_fineSharp);
  m_settingsManager->SetNumber(SET_IMAGE_FINESHARP_STRENGTH, madvrSettings.m_fineSharpStrength);
  m_settingsManager->SetBool(SET_IMAGE_LUMASHARPEN, madvrSettings.m_lumaSharpen);
  m_settingsManager->SetNumber(SET_IMAGE_LUMASHARPEN_STRENGTH, madvrSettings.m_lumaSharpenStrength);
  m_settingsManager->SetBool(SET_IMAGE_ADAPTIVESHARPEN, madvrSettings.m_adaptiveSharpen);
  m_settingsManager->SetNumber(SET_IMAGE_ADAPTIVESHARPEN_STRENGTH, madvrSettings.m_adaptiveSharpenStrength);
  m_settingsManager->SetBool(SET_IMAGE_UPFINESHARP, madvrSettings.m_UpRefFineSharp);
  m_settingsManager->SetNumber(SET_IMAGE_UPFINESHARP_STRENGTH, madvrSettings.m_UpRefFineSharpStrength);
  m_settingsManager->SetBool(SET_IMAGE_UPLUMASHARPEN, madvrSettings.m_UpRefLumaSharpen);
  m_settingsManager->SetNumber(SET_IMAGE_UPLUMASHARPEN_STRENGTH, madvrSettings.m_UpRefLumaSharpenStrength);
  m_settingsManager->SetBool(SET_IMAGE_UPADAPTIVESHARPEN, madvrSettings.m_UpRefAdaptiveSharpen);
  m_settingsManager->SetNumber(SET_IMAGE_UPADAPTIVESHARPEN_STRENGTH, madvrSettings.m_UpRefAdaptiveSharpenStrength);
  m_settingsManager->SetBool(SET_IMAGE_SUPER_RES, madvrSettings.m_superRes);
  m_settingsManager->SetNumber(SET_IMAGE_SUPER_RES_STRENGTH, madvrSettings.m_superResStrength);
  m_settingsManager->SetBool(SET_IMAGE_REFINE_ONCE, madvrSettings.m_refineOnce);
  m_settingsManager->SetBool(SET_IMAGE_SUPER_RES_FIRST, madvrSettings.m_superResFirst);
}

void CGUIDialogMadvrScaling::SetupView()
{
  CGUIDialogSettingsManualBase::SetupView();

  SetHeading(70000);

  if (CSettings::GetInstance().GetInt(CSettings::SETTING_DSPLAYER_MANAGEMADVRWITHKODI) == KODIGUI_LOAD_MADVR)
  {
    std::string profile;
    CMadvrCallback::Get()->GetProfileActiveName(&profile);
    if (profile != "")
    {
      CStdString sHeading;
      sHeading.Format("%s %s: %s", g_localizeStrings.Get(70000).c_str(), g_localizeStrings.Get(20093).c_str(), profile.c_str());
      SetHeading(sHeading);
    }
  }
}

void CGUIDialogMadvrScaling::InitializeSettings()
{
  CGUIDialogSettingsManualBase::InitializeSettings();

  bool usePopup = g_SkinInfo->HasSkinFile("DialogSlider.xml");

  CSettingCategory *category = AddCategory("dsplayermadvrsettings", -1);
  if (category == NULL)
  {
    CLog::Log(LOGERROR, "CGUIDialogMadvrScaling: unable to setup settings");
    return;
  }

  // get all necessary setting groups
  CSettingGroup *groupMadvrChromaScaling = AddGroup(category);
  if (groupMadvrChromaScaling == NULL)
  {
    CLog::Log(LOGERROR, "CGUIDialogMadvrScaling: unable to setup settings");
    return;
  }

  // get all necessary setting groups
  CSettingGroup *groupMadvrUpScaling = AddGroup(category);
  if (groupMadvrUpScaling == NULL)
  {
    CLog::Log(LOGERROR, "CGUIDialogMadvrScaling: unable to setup settings");
    return;
  }

  // get all necessary setting groups
  CSettingGroup *groupMadvrDownScaling = AddGroup(category);
  if (groupMadvrDownScaling == NULL)
  {
    CLog::Log(LOGERROR, "CGUIDialogMadvrScaling: unable to setup settings");
    return;
  }

  CSettingGroup *groupMadvrDoubling = AddGroup(category);
  if (groupMadvrDoubling == NULL)
  {
    CLog::Log(LOGERROR, "CGUIDialogVideoSettings: unable to setup settings");
    return;
  }

  CSettingGroup *groupMadvrSharp = AddGroup(category);
  if (groupMadvrSharp == NULL)
  {
    CLog::Log(LOGERROR, "CGUIDialogVideoSettings: unable to setup settings");
    return;
  }

  CSettingGroup *groupMadvrUpSharp = AddGroup(category);
  if (groupMadvrUpSharp == NULL)
  {
    CLog::Log(LOGERROR, "CGUIDialogVideoSettings: unable to setup settings");
    return;
  }

  StaticIntegerSettingOptions entries, entriesDoubleFactor, entriesQuadrupleFactor;
  CMadvrSettings &madvrSettings = CMediaSettings::GetInstance().GetCurrentMadvrSettings();

  //MADVR CHROMA UPSCALING
  entries.clear();
  CMadvrCallback::Get()->AddEntry(MADVR_LIST_CHROMAUP, &entries);

  AddList(groupMadvrChromaScaling, SET_CHROMA_UPSCALING, 70028, 0, static_cast<int>(madvrSettings.m_ChromaUpscaling), entries, 70028);
  AddToggle(groupMadvrChromaScaling, SET_CHROMA_ANTIRING, 70031, 0, madvrSettings.m_ChromaAntiRing);
  AddToggle(groupMadvrChromaScaling, SET_CHROMA_SUPER_RES, 70041, 0, madvrSettings.m_ChromaSuperRes);

  //MADVR IMAGE UPSCALING
  entries.clear();
  CMadvrCallback::Get()->AddEntry(MADVR_LIST_LUMAUP, &entries);

  AddList(groupMadvrUpScaling, SET_IMAGE_UPSCALING, 70029, 0, static_cast<int>(madvrSettings.m_ImageUpscaling), entries, 70029);
  AddToggle(groupMadvrUpScaling, SET_IMAGE_UP_ANTIRING, 70031, 0, madvrSettings.m_ImageUpAntiRing);
  AddToggle(groupMadvrUpScaling, SET_IMAGE_UP_LINEAR, 70032, 0, madvrSettings.m_ImageUpLinear);

  //MADVR IMAGE DOWNSCALING
  entries.clear();
  CMadvrCallback::Get()->AddEntry(MADVR_LIST_LUMADOWN, &entries);

  AddList(groupMadvrDownScaling, SET_IMAGE_DOWNSCALING, 70030, 0, static_cast<int>(madvrSettings.m_ImageDownscaling), entries, 70030);
  AddToggle(groupMadvrDownScaling, SET_IMAGE_DOWN_ANTIRING, 70031, 0, madvrSettings.m_ImageDownAntiRing);
  AddToggle(groupMadvrDownScaling, SET_IMAGE_DOWN_LINEAR, 70032, 0, madvrSettings.m_ImageDownLinear);

  // MADVR IMAGE DOUBLING / QUADRUPLING
  entries.clear();
  entries.push_back(make_pair(70117, -1));
  CMadvrCallback::Get()->AddEntry(MADVR_LIST_DOUBLEQUALITY, &entries);

  entriesDoubleFactor.clear();
  CMadvrCallback::Get()->AddEntry(MADVR_LIST_DOUBLEFACTOR, &entriesDoubleFactor);

  entriesQuadrupleFactor.clear();
  CMadvrCallback::Get()->AddEntry(MADVR_LIST_QUADRUPLEFACTOR, &entriesQuadrupleFactor);

  AddList(groupMadvrDoubling, SET_IMAGE_DOUBLE_LUMA, 70100, 0, static_cast<int>(madvrSettings.m_ImageDoubleLuma), entries, 70100);
  AddList(groupMadvrDoubling, SET_IMAGE_DOUBLE_LUMA_FACTOR, 70116, 0, static_cast<int>(madvrSettings.m_ImageDoubleLumaFactor), entriesDoubleFactor, 70116);

  AddList(groupMadvrDoubling, SET_IMAGE_DOUBLE_CHROMA, 70101, 0, static_cast<int>(madvrSettings.m_ImageDoubleChroma), entries, 70101);
  AddList(groupMadvrDoubling, SET_IMAGE_DOUBLE_CHROMA_FACTOR, 70116, 0, static_cast<int>(madvrSettings.m_ImageDoubleChromaFactor), entriesDoubleFactor, 70116);

  AddList(groupMadvrDoubling, SET_IMAGE_QUADRUPLE_LUMA, 70102, 0, static_cast<int>(madvrSettings.m_ImageQuadrupleLuma), entries, 70102);
  AddList(groupMadvrDoubling, SET_IMAGE_QUADRUPLE_LUMA_FACTOR, 70116, 0, static_cast<int>(madvrSettings.m_ImageQuadrupleLumaFactor), entriesQuadrupleFactor, 70116);

  AddList(groupMadvrDoubling, SET_IMAGE_QUADRUPLE_CHROMA, 70103, 0, static_cast<int>(madvrSettings.m_ImageQuadrupleChroma), entries, 70103);
  AddList(groupMadvrDoubling, SET_IMAGE_QUADRUPLE_CHROMA_FACTOR, 70116, 0, static_cast<int>(madvrSettings.m_ImageQuadrupleChromaFactor), entriesQuadrupleFactor, 70116);

  //IMAGE ENHANCEMENTS
  AddToggle(groupMadvrSharp, SET_IMAGE_FINESHARP, 70118, 0, madvrSettings.m_fineSharp);
  AddSlider(groupMadvrSharp, SET_IMAGE_FINESHARP_STRENGTH, 70122, 0, madvrSettings.m_fineSharpStrength, "%1.1f", 0.0f, 0.1f, 8.0f, 70118, usePopup);
  AddToggle(groupMadvrSharp, SET_IMAGE_LUMASHARPEN, 70119, 0, madvrSettings.m_lumaSharpen);
  AddSlider(groupMadvrSharp, SET_IMAGE_LUMASHARPEN_STRENGTH, 70122, 0, madvrSettings.m_lumaSharpenStrength, "%1.2f", 0.0f, 0.01f, 3.0f, 70119, usePopup);
  AddToggle(groupMadvrSharp, SET_IMAGE_ADAPTIVESHARPEN, 70120, 0, madvrSettings.m_adaptiveSharpen);
  AddSlider(groupMadvrSharp, SET_IMAGE_ADAPTIVESHARPEN_STRENGTH, 70122, 0, madvrSettings.m_adaptiveSharpenStrength, "%1.1f", 0.0f, 0.1f, 1.5f, 70120, usePopup);

  //UPSCALING REFINEMENTS
  AddToggle(groupMadvrUpSharp, SET_IMAGE_UPFINESHARP, 70123, 0, madvrSettings.m_UpRefFineSharp);
  AddSlider(groupMadvrUpSharp, SET_IMAGE_UPFINESHARP_STRENGTH, 70122, 0, madvrSettings.m_UpRefFineSharpStrength, "%1.1f", 0.0f, 0.1f, 8.0f, 70123, usePopup);
  AddToggle(groupMadvrUpSharp, SET_IMAGE_UPLUMASHARPEN, 70124, 0, madvrSettings.m_UpRefLumaSharpen);
  AddSlider(groupMadvrUpSharp, SET_IMAGE_UPLUMASHARPEN_STRENGTH, 70122, 0, madvrSettings.m_UpRefLumaSharpenStrength, "%1.2f", 0.0f, 0.01f, 3.0f, 70124, usePopup);
  AddToggle(groupMadvrUpSharp, SET_IMAGE_UPADAPTIVESHARPEN, 70125, 0, madvrSettings.m_UpRefAdaptiveSharpen);
  AddSlider(groupMadvrUpSharp, SET_IMAGE_UPADAPTIVESHARPEN_STRENGTH, 70122, 0, madvrSettings.m_UpRefAdaptiveSharpenStrength, "%1.1f", 0.0f, 0.1f, 1.5f, 70125, usePopup);
  AddToggle(groupMadvrUpSharp, SET_IMAGE_SUPER_RES, 70121, 0, madvrSettings.m_superRes);
  AddSlider(groupMadvrUpSharp, SET_IMAGE_SUPER_RES_STRENGTH, 70122, 0, madvrSettings.m_superResStrength, "%1.0f", 0.0f, 1.0f, 4.0f, 70121, usePopup);

  AddToggle(groupMadvrUpSharp, SET_IMAGE_REFINE_ONCE, 70126, 0, madvrSettings.m_refineOnce);
  AddToggle(groupMadvrUpSharp, SET_IMAGE_SUPER_RES_FIRST, 70127, 0, madvrSettings.m_superResFirst);
}

void CGUIDialogMadvrScaling::OnSettingChanged(const CSetting *setting)
{
  if (setting == NULL)
    return;

  CGUIDialogSettingsManualBase::OnSettingChanged(setting);

  CMadvrSettings &madvrSettings = CMediaSettings::GetInstance().GetCurrentMadvrSettings();

  const std::string &settingId = setting->GetId();

  if (settingId == SET_CHROMA_UPSCALING)
  {
    madvrSettings.m_ChromaUpscaling = static_cast<int>(static_cast<const CSettingInt*>(setting)->GetValue());
    CMadvrCallback::Get()->SetStr("chromaUp", CMadvrCallback::Get()->GetSettingsName(MADVR_LIST_CHROMAUP, madvrSettings.m_ChromaUpscaling));
  }
  else if (settingId == SET_CHROMA_ANTIRING)
  {
    madvrSettings.m_ChromaAntiRing = static_cast<const CSettingBool*>(setting)->GetValue();
    CMadvrCallback::Get()->SetBool("chromaAntiRinging", madvrSettings.m_ChromaAntiRing);
  }
  else if (settingId == SET_CHROMA_SUPER_RES)
  {
    madvrSettings.m_ChromaSuperRes = static_cast<const CSettingBool*>(setting)->GetValue();
    CMadvrCallback::Get()->SetBool("superChromaRes", madvrSettings.m_ChromaSuperRes);
  }
  else if (settingId == SET_IMAGE_UPSCALING)
  {
    madvrSettings.m_ImageUpscaling = static_cast<int>(static_cast<const CSettingInt*>(setting)->GetValue());
    CMadvrCallback::Get()->SetStr("lumaUp", CMadvrCallback::Get()->GetSettingsName(MADVR_LIST_LUMAUP, madvrSettings.m_ImageUpscaling));
  }
  else if (settingId == SET_IMAGE_UP_ANTIRING)
  {
    madvrSettings.m_ImageUpAntiRing = static_cast<const CSettingBool*>(setting)->GetValue();
    CMadvrCallback::Get()->SetBool("lumaUpAntiRinging", madvrSettings.m_ImageUpAntiRing);
  }
  else if (settingId == SET_IMAGE_UP_LINEAR)
  {
    madvrSettings.m_ImageUpLinear = static_cast<const CSettingBool*>(setting)->GetValue();
    CMadvrCallback::Get()->SetBool("lumaUpLinear", madvrSettings.m_ImageUpLinear);
  }
  else if (settingId == SET_IMAGE_DOWNSCALING)
  {
    madvrSettings.m_ImageDownscaling = static_cast<int>(static_cast<const CSettingInt*>(setting)->GetValue());
    CMadvrCallback::Get()->SetStr("lumaDown", CMadvrCallback::Get()->GetSettingsName(MADVR_LIST_LUMADOWN, madvrSettings.m_ImageDownscaling));
  }
  else if (settingId == SET_IMAGE_DOWN_ANTIRING)
  {
    madvrSettings.m_ImageDownAntiRing = static_cast<const CSettingBool*>(setting)->GetValue();
    CMadvrCallback::Get()->SetBool("lumaDownAntiRinging", madvrSettings.m_ImageDownAntiRing);
  }
  else if (settingId == SET_IMAGE_DOWN_LINEAR)
  {
    madvrSettings.m_ImageDownLinear = static_cast<const CSettingBool*>(setting)->GetValue();
    CMadvrCallback::Get()->SetBool("lumaDownLinear", madvrSettings.m_ImageDownLinear);
  }
  else if (settingId == SET_IMAGE_DOUBLE_LUMA)
  {
    madvrSettings.m_ImageDoubleLuma = static_cast<int>(static_cast<const CSettingInt*>(setting)->GetValue());
    CMadvrCallback::Get()->SetDoubling("DL", madvrSettings.m_ImageDoubleLuma);
    CMadvrCallback::Get()->SetStr("nnediDLScalingFactor", CMadvrCallback::Get()->GetSettingsName(MADVR_LIST_DOUBLEFACTOR, madvrSettings.m_ImageDoubleLumaFactor));
  }
  else if (settingId == SET_IMAGE_DOUBLE_CHROMA)
  {
    madvrSettings.m_ImageDoubleChroma = static_cast<int>(static_cast<const CSettingInt*>(setting)->GetValue());
    CMadvrCallback::Get()->SetDoubling("DC", madvrSettings.m_ImageDoubleChroma);
    CMadvrCallback::Get()->SetStr("nnediDCScalingFactor", CMadvrCallback::Get()->GetSettingsName(MADVR_LIST_DOUBLEFACTOR, madvrSettings.m_ImageDoubleChromaFactor));
  }
  else if (settingId == SET_IMAGE_QUADRUPLE_LUMA)
  {
    madvrSettings.m_ImageQuadrupleLuma = static_cast<int>(static_cast<const CSettingInt*>(setting)->GetValue());
    CMadvrCallback::Get()->SetDoubling("QL", madvrSettings.m_ImageQuadrupleLuma);
    CMadvrCallback::Get()->SetStr("nnediQLScalingFactor", CMadvrCallback::Get()->GetSettingsName(MADVR_LIST_QUADRUPLEFACTOR, madvrSettings.m_ImageQuadrupleLumaFactor));
  }
  else if (settingId == SET_IMAGE_QUADRUPLE_CHROMA)
  {
    madvrSettings.m_ImageQuadrupleChroma = static_cast<int>(static_cast<const CSettingInt*>(setting)->GetValue());
    CMadvrCallback::Get()->SetDoubling("QC", madvrSettings.m_ImageQuadrupleChroma);
    CMadvrCallback::Get()->SetStr("nnediQCScalingFactor", CMadvrCallback::Get()->GetSettingsName(MADVR_LIST_QUADRUPLEFACTOR, madvrSettings.m_ImageQuadrupleChromaFactor));
  }
  else if (settingId == SET_IMAGE_DOUBLE_LUMA_FACTOR)
  {
    madvrSettings.m_ImageDoubleLumaFactor = static_cast<int>(static_cast<const CSettingInt*>(setting)->GetValue());
    CMadvrCallback::Get()->SetStr("nnediDLScalingFactor", CMadvrCallback::Get()->GetSettingsName(MADVR_LIST_DOUBLEFACTOR, madvrSettings.m_ImageDoubleLumaFactor));
  }
  else if (settingId == SET_IMAGE_DOUBLE_CHROMA_FACTOR)
  {
    madvrSettings.m_ImageDoubleChromaFactor = static_cast<int>(static_cast<const CSettingInt*>(setting)->GetValue());
    CMadvrCallback::Get()->SetStr("nnediDCScalingFactor", CMadvrCallback::Get()->GetSettingsName(MADVR_LIST_DOUBLEFACTOR, madvrSettings.m_ImageDoubleChromaFactor));
  }
  else if (settingId == SET_IMAGE_QUADRUPLE_LUMA_FACTOR)
  {
    madvrSettings.m_ImageQuadrupleLumaFactor = static_cast<int>(static_cast<const CSettingInt*>(setting)->GetValue());
    CMadvrCallback::Get()->SetStr("nnediQLScalingFactor", CMadvrCallback::Get()->GetSettingsName(MADVR_LIST_QUADRUPLEFACTOR, madvrSettings.m_ImageQuadrupleLumaFactor));
  }
  else if (settingId == SET_IMAGE_QUADRUPLE_CHROMA_FACTOR)
  {
    madvrSettings.m_ImageQuadrupleChromaFactor = static_cast<int>(static_cast<const CSettingInt*>(setting)->GetValue());
    CMadvrCallback::Get()->SetStr("nnediQCScalingFactor", CMadvrCallback::Get()->GetSettingsName(MADVR_LIST_QUADRUPLEFACTOR, madvrSettings.m_ImageQuadrupleChromaFactor));
  }
  else if (settingId == SET_IMAGE_FINESHARP)
  {
    madvrSettings.m_fineSharp = static_cast<const CSettingBool*>(setting)->GetValue();
    CMadvrCallback::Get()->SetBool("fineSharp", madvrSettings.m_fineSharp);
  }
  else if (settingId == SET_IMAGE_FINESHARP_STRENGTH)
  {
    madvrSettings.m_fineSharpStrength = static_cast<float>(static_cast<const CSettingNumber*>(setting)->GetValue());
    CMadvrCallback::Get()->SetFloat("fineSharpStrength", madvrSettings.m_fineSharpStrength, 10);
  }
  else if (settingId == SET_IMAGE_LUMASHARPEN)
  {
    madvrSettings.m_lumaSharpen = static_cast<const CSettingBool*>(setting)->GetValue();
    CMadvrCallback::Get()->SetBool("lumaSharpen", madvrSettings.m_lumaSharpen);
  }
  else if (settingId == SET_IMAGE_LUMASHARPEN_STRENGTH)
  {
    madvrSettings.m_lumaSharpenStrength = static_cast<float>(static_cast<const CSettingNumber*>(setting)->GetValue());
    CMadvrCallback::Get()->SetFloat("lumaSharpenStrength", madvrSettings.m_lumaSharpenStrength);
  }
  else if (settingId == SET_IMAGE_ADAPTIVESHARPEN)
  {
    madvrSettings.m_adaptiveSharpen = static_cast<const CSettingBool*>(setting)->GetValue();
    CMadvrCallback::Get()->SetBool("adaptiveSharpen", madvrSettings.m_adaptiveSharpen);
  }
  else if (settingId == SET_IMAGE_ADAPTIVESHARPEN_STRENGTH)
  {
    madvrSettings.m_adaptiveSharpenStrength = static_cast<float>(static_cast<const CSettingNumber*>(setting)->GetValue());
    CMadvrCallback::Get()->SetFloat("adaptiveSharpenStrength", madvrSettings.m_adaptiveSharpenStrength, 10);
  }

  else if (settingId == SET_IMAGE_UPFINESHARP)
  {
    madvrSettings.m_UpRefFineSharp = static_cast<const CSettingBool*>(setting)->GetValue();
    CMadvrCallback::Get()->SetBool("upRefFineSharp", madvrSettings.m_UpRefFineSharp);
  }
  else if (settingId == SET_IMAGE_UPFINESHARP_STRENGTH)
  {
    madvrSettings.m_UpRefFineSharpStrength = static_cast<float>(static_cast<const CSettingNumber*>(setting)->GetValue());
    CMadvrCallback::Get()->SetFloat("upRefFineSharpStrength", madvrSettings.m_UpRefFineSharpStrength, 10);
  }
  else if (settingId == SET_IMAGE_UPLUMASHARPEN)
  {
    madvrSettings.m_UpRefLumaSharpen = static_cast<const CSettingBool*>(setting)->GetValue();
    CMadvrCallback::Get()->SetBool("upRefLumaSharpen", madvrSettings.m_UpRefLumaSharpen);
  }
  else if (settingId == SET_IMAGE_UPLUMASHARPEN_STRENGTH)
  {
    madvrSettings.m_UpRefLumaSharpenStrength = static_cast<float>(static_cast<const CSettingNumber*>(setting)->GetValue());
    CMadvrCallback::Get()->SetFloat("upRefLumaSharpenStrength", madvrSettings.m_UpRefLumaSharpenStrength);
  }
  else if (settingId == SET_IMAGE_UPADAPTIVESHARPEN)
  {
    madvrSettings.m_UpRefAdaptiveSharpen = static_cast<const CSettingBool*>(setting)->GetValue();
    CMadvrCallback::Get()->SetBool("upRefAdaptiveSharpen", madvrSettings.m_UpRefAdaptiveSharpen);
  }
  else if (settingId == SET_IMAGE_UPADAPTIVESHARPEN_STRENGTH)
  {
    madvrSettings.m_UpRefAdaptiveSharpenStrength = static_cast<float>(static_cast<const CSettingNumber*>(setting)->GetValue());
    CMadvrCallback::Get()->SetFloat("UpRefAdaptiveSharpenStrength", madvrSettings.m_UpRefAdaptiveSharpenStrength, 10);
  }

  else if (settingId == SET_IMAGE_SUPER_RES)
  {
    madvrSettings.m_superRes = static_cast<const CSettingBool*>(setting)->GetValue();
    CMadvrCallback::Get()->SetBool("superRes", madvrSettings.m_superRes);
  }
  else if (settingId == SET_IMAGE_SUPER_RES_STRENGTH)
  {
    madvrSettings.m_superResStrength = static_cast<float>(static_cast<const CSettingNumber*>(setting)->GetValue());
    CMadvrCallback::Get()->SetFloat("superResStrength", madvrSettings.m_superResStrength,1);
  }
  else if (settingId == SET_IMAGE_REFINE_ONCE)
  {
    madvrSettings.m_refineOnce = static_cast<const CSettingBool*>(setting)->GetValue();
    CMadvrCallback::Get()->SetBool("refineOnce", !madvrSettings.m_refineOnce);
  }
  else if (settingId == SET_IMAGE_SUPER_RES_FIRST)
  {
    madvrSettings.m_superResFirst = static_cast<const CSettingBool*>(setting)->GetValue();
    CMadvrCallback::Get()->SetBool("superResFirst", madvrSettings.m_superResFirst);
  }
  HideUnused();
}

void CGUIDialogMadvrScaling::OnSettingAction(const CSetting *setting)
{
  if (setting == NULL)
    return;
}

void CGUIDialogMadvrScaling::HideUnused()
{
  if (!m_allowchange)
    return;

  m_allowchange = false;

  CMadvrSettings &madvrSettings = CMediaSettings::GetInstance().GetCurrentMadvrSettings();

  int value;
  bool bValue;
  bool bValue1;
  bool bValue2;
  bool bValue3;

  CSetting *setting;

  // HIDE / SHOW
  setting = m_settingsManager->GetSetting(SET_IMAGE_DOUBLE_LUMA);
  value = static_cast<int>(static_cast<const CSettingInt*>(setting)->GetValue());
  SetVisible(SET_IMAGE_DOUBLE_LUMA_FACTOR, (value>-1));

  setting = m_settingsManager->GetSetting(SET_IMAGE_DOUBLE_CHROMA);
  value = static_cast<int>(static_cast<const CSettingInt*>(setting)->GetValue());
  SetVisible(SET_IMAGE_DOUBLE_CHROMA_FACTOR, (value > -1));

  setting = m_settingsManager->GetSetting(SET_IMAGE_QUADRUPLE_LUMA);
  value = static_cast<int>(static_cast<const CSettingInt*>(setting)->GetValue());
  SetVisible(SET_IMAGE_QUADRUPLE_LUMA_FACTOR, (value > -1));

  setting = m_settingsManager->GetSetting(SET_IMAGE_QUADRUPLE_CHROMA);
  value = static_cast<int>(static_cast<const CSettingInt*>(setting)->GetValue());
  SetVisible(SET_IMAGE_QUADRUPLE_CHROMA_FACTOR, (value > -1));

  setting = m_settingsManager->GetSetting(SET_IMAGE_FINESHARP);
  bValue = static_cast<const CSettingBool*>(setting)->GetValue();
  SetVisible(SET_IMAGE_FINESHARP_STRENGTH , bValue);

  setting = m_settingsManager->GetSetting(SET_IMAGE_LUMASHARPEN);
  bValue = static_cast<const CSettingBool*>(setting)->GetValue();
  SetVisible(SET_IMAGE_LUMASHARPEN_STRENGTH, bValue);

  setting = m_settingsManager->GetSetting(SET_IMAGE_ADAPTIVESHARPEN);
  bValue = static_cast<const CSettingBool*>(setting)->GetValue();
  SetVisible(SET_IMAGE_ADAPTIVESHARPEN_STRENGTH, bValue);

  setting = m_settingsManager->GetSetting(SET_IMAGE_UPFINESHARP);
  bValue = static_cast<const CSettingBool*>(setting)->GetValue();
  SetVisible(SET_IMAGE_UPFINESHARP_STRENGTH, bValue);

  setting = m_settingsManager->GetSetting(SET_IMAGE_UPLUMASHARPEN);
  bValue = static_cast<const CSettingBool*>(setting)->GetValue();
  SetVisible(SET_IMAGE_UPLUMASHARPEN_STRENGTH, bValue);

  setting = m_settingsManager->GetSetting(SET_IMAGE_UPADAPTIVESHARPEN);
  bValue = static_cast<const CSettingBool*>(setting)->GetValue();
  SetVisible(SET_IMAGE_UPADAPTIVESHARPEN_STRENGTH, bValue);

  setting = m_settingsManager->GetSetting(SET_IMAGE_SUPER_RES);
  bValue = static_cast<const CSettingBool*>(setting)->GetValue();
  SetVisible(SET_IMAGE_SUPER_RES_STRENGTH, bValue);
  SetVisible(SET_IMAGE_SUPER_RES_FIRST, bValue);
  setting = m_settingsManager->GetSetting(SET_IMAGE_UPFINESHARP);
  bValue1 = static_cast<const CSettingBool*>(setting)->GetValue();
  setting = m_settingsManager->GetSetting(SET_IMAGE_UPLUMASHARPEN);
  bValue2 = static_cast<const CSettingBool*>(setting)->GetValue();
  setting = m_settingsManager->GetSetting(SET_IMAGE_UPADAPTIVESHARPEN);
  bValue3 = static_cast<const CSettingBool*>(setting)->GetValue();
  SetVisible(SET_IMAGE_REFINE_ONCE, (bValue||bValue1||bValue2||bValue3));

  m_allowchange = true;
}

void CGUIDialogMadvrScaling::SetVisible(CStdString id, bool visible)
{
  CSetting *setting = m_settingsManager->GetSetting(id);
  if (setting->IsVisible() && visible)
    return;
  setting->SetVisible(visible);
  setting->SetEnabled(visible);
}
