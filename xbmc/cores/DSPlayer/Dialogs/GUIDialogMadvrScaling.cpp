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
#define SET_CHROMA_SUPER_RES_PASSES            "madvr.chromasuperrespasses"
#define SET_CHROMA_SUPER_RES_STRENGTH          "madvr.chromasuperresstrength"
#define SET_CHROMA_SUPER_RES_SOFTNESS          "madvr.chromasuperressoftness"

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

#define SET_FAKE_DOUBLE_CHROMA                "madvr.fakedoublechroma"
#define SET_FAKE_QUADRUPLE_CHROMA             "madvr.fakequadruplechroma"
#define SET_FAKE_DOUBLE_CHROMA_FACTOR         "madvr.fakedoublechromafactor"
#define SET_FAKE_QUADRUPLE_CHROMA_FACTOR      "madvr.fakequadruplechromafactor"


#define SET_IMAGE_UPSHARPENEDGES               "madvr.upsharpenedges"
#define SET_IMAGE_UPSHARPENEDGES_STRENGTH      "madvr.upsharpenedgesstrength"
#define SET_IMAGE_UPCRISPENEDGES               "madvr.upcrispenedges"
#define SET_IMAGE_UPCRISPENEDGES_STRENGTH      "madvr.upcrispenedgesstrength"
#define SET_IMAGE_UPTHINEDGES                  "madvr.upthinedges"
#define SET_IMAGE_UPTHINEDGES_STRENGTH         "madvr.upthinedgesstrength"
#define SET_IMAGE_UPENHANCEDETAIL              "madvr.upenhancedetail"
#define SET_IMAGE_UPENHANCEDETAIL_STRENGTH     "madvr.upenhancedetailstrength"

#define SET_IMAGE_UPLUMASHARPEN                "madvr.uplumasharpen"
#define SET_IMAGE_UPLUMASHARPEN_STRENGTH       "madvr.uplumasharpenstrength"
#define SET_IMAGE_UPADAPTIVESHARPEN            "madvr.upadaptivesharpen"
#define SET_IMAGE_UPADAPTIVESHARPEN_STRENGTH   "madvr.upadaptivesharpenstrength"
#define SET_IMAGE_SUPER_RES                    "madvr.superres"
#define SET_IMAGE_SUPER_RES_STRENGTH           "madvr.superresstrength"
#define SET_IMAGE_SUPER_RES_LINEAR             "madvr.superreslinear"

#define SET_IMAGE_REFINE_ONCE                  "madvr.refineonce"

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

  HideUnused();
}

void CGUIDialogMadvrScaling::SetupView()
{
  CGUIDialogSettingsManualBase::SetupView();

  SetHeading(70000);

  if (CSettings::GetInstance().GetInt(CSettings::SETTING_DSPLAYER_MANAGEMADVRWITHKODI) == KODIGUI_LOAD_MADVR)
  {
    std::string profile;
    CMadvrCallback::Get()->GetProfileActiveName("scalingParent",&profile);
    if (profile != "")
    {
      CStdString sHeading;
      sHeading.Format("%s: %s", g_localizeStrings.Get(20093).c_str(), profile.c_str());
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

  CSettingGroup *groupMadvrUpSharp = AddGroup(category);
  if (groupMadvrUpSharp == NULL)
  {
    CLog::Log(LOGERROR, "CGUIDialogVideoSettings: unable to setup settings");
    return;
  }

  StaticIntegerSettingOptions entries, entriesDoubleFactor, entriesQuadrupleFactor;
  CMadvrSettings &madvrSettings = CMediaSettings::GetInstance().GetCurrentMadvrSettings();
  CMadvrCallback::Get()->LoadSettings(MADVR_LOAD_SCALING);

  //MADVR CHROMA UPSCALING
  entries.clear();
  CMadvrCallback::Get()->AddEntry(MADVR_LIST_CHROMAUP, &entries);

  AddList(groupMadvrChromaScaling, SET_CHROMA_UPSCALING, 70028, 0, static_cast<int>(madvrSettings.m_ChromaUpscaling), entries, 70028);
  AddToggle(groupMadvrChromaScaling, SET_CHROMA_ANTIRING, 70031, 0, madvrSettings.m_ChromaAntiRing);
  AddToggle(groupMadvrChromaScaling, SET_CHROMA_SUPER_RES, 70041, 0, madvrSettings.m_ChromaSuperRes);
  AddSpinner(groupMadvrChromaScaling, SET_CHROMA_SUPER_RES_PASSES, 70130, 0, madvrSettings.m_ChromaSuperResPasses, 1, 1, 10);
  AddSlider(groupMadvrChromaScaling, SET_CHROMA_SUPER_RES_STRENGTH, 70122, 0, madvrSettings.m_ChromaSuperResStrength, "%1.2f", 0.0f, 0.01f, 1.0f, 70041, usePopup);
  AddSlider(groupMadvrChromaScaling, SET_CHROMA_SUPER_RES_SOFTNESS, 70131, 0, madvrSettings.m_ChromaSuperResSoftness, "%1.2f", 0.0f, 0.01f, 1.0f, 70041, usePopup);

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
  AddList(groupMadvrDoubling, SET_FAKE_DOUBLE_CHROMA, 70101, 0, static_cast<int>(madvrSettings.m_ImageDoubleChroma), entries, 70101);
  AddList(groupMadvrDoubling, SET_FAKE_DOUBLE_CHROMA_FACTOR, 70116, 0, static_cast<int>(madvrSettings.m_ImageDoubleChromaFactor), entriesDoubleFactor, 70116);

  AddList(groupMadvrDoubling, SET_IMAGE_QUADRUPLE_LUMA, 70102, 0, static_cast<int>(madvrSettings.m_ImageQuadrupleLuma), entries, 70102);
  AddList(groupMadvrDoubling, SET_IMAGE_QUADRUPLE_LUMA_FACTOR, 70116, 0, static_cast<int>(madvrSettings.m_ImageQuadrupleLumaFactor), entriesQuadrupleFactor, 70116);

  AddList(groupMadvrDoubling, SET_IMAGE_QUADRUPLE_CHROMA, 70103, 0, static_cast<int>(madvrSettings.m_ImageQuadrupleChroma), entries, 70103);
  AddList(groupMadvrDoubling, SET_IMAGE_QUADRUPLE_CHROMA_FACTOR, 70116, 0, static_cast<int>(madvrSettings.m_ImageQuadrupleChromaFactor), entriesQuadrupleFactor, 70116);
  AddList(groupMadvrDoubling, SET_FAKE_QUADRUPLE_CHROMA, 70103, 0, static_cast<int>(madvrSettings.m_ImageQuadrupleChroma), entries, 70103);
  AddList(groupMadvrDoubling, SET_FAKE_QUADRUPLE_CHROMA_FACTOR, 70116, 0, static_cast<int>(madvrSettings.m_ImageQuadrupleChromaFactor), entriesQuadrupleFactor, 70116);

  //UPSCALING REFINEMENTS
  AddToggle(groupMadvrUpSharp, SET_IMAGE_UPSHARPENEDGES, 70138, 0, madvrSettings.m_UpRefSharpenEdges);
  AddSlider(groupMadvrUpSharp, SET_IMAGE_UPSHARPENEDGES_STRENGTH, 70122, 0, madvrSettings.m_UpRefSharpenEdgesStrength, "%1.1f", 0.0f, 0.1f, 4.0f, 70138, usePopup);
  AddToggle(groupMadvrUpSharp, SET_IMAGE_UPCRISPENEDGES, 70139, 0, madvrSettings.m_UpRefCrispenEdges);
  AddSlider(groupMadvrUpSharp, SET_IMAGE_UPCRISPENEDGES_STRENGTH, 70122, 0, madvrSettings.m_UpRefCrispenEdgesStrength, "%1.1f", 0.0f, 0.1f, 4.0f, 70139, usePopup);
  AddToggle(groupMadvrUpSharp, SET_IMAGE_UPTHINEDGES, 70140, 0, madvrSettings.m_UpRefThinEdges);
  AddSlider(groupMadvrUpSharp, SET_IMAGE_UPTHINEDGES_STRENGTH, 70122, 0, madvrSettings.m_UpRefThinEdgesStrength, "%1.1f", 0.0f, 0.1f, 4.0f, 70140, usePopup);
  AddToggle(groupMadvrUpSharp, SET_IMAGE_UPENHANCEDETAIL, 70141, 0, madvrSettings.m_UpRefEnhanceDetail);
  AddSlider(groupMadvrUpSharp, SET_IMAGE_UPENHANCEDETAIL_STRENGTH, 70122, 0, madvrSettings.m_UpRefEnhanceDetailStrength, "%1.1f", 0.0f, 0.1f, 4.0f, 70141, usePopup);

  AddToggle(groupMadvrUpSharp, SET_IMAGE_UPLUMASHARPEN, 70124, 0, madvrSettings.m_UpRefLumaSharpen);
  AddSlider(groupMadvrUpSharp, SET_IMAGE_UPLUMASHARPEN_STRENGTH, 70122, 0, madvrSettings.m_UpRefLumaSharpenStrength, "%1.2f", 0.0f, 0.01f, 3.0f, 70124, usePopup);
  AddToggle(groupMadvrUpSharp, SET_IMAGE_UPADAPTIVESHARPEN, 70125, 0, madvrSettings.m_UpRefAdaptiveSharpen);
  AddSlider(groupMadvrUpSharp, SET_IMAGE_UPADAPTIVESHARPEN_STRENGTH, 70122, 0, madvrSettings.m_UpRefAdaptiveSharpenStrength, "%1.1f", 0.0f, 0.1f, 1.5f, 70125, usePopup);
  AddToggle(groupMadvrUpSharp, SET_IMAGE_SUPER_RES, 70121, 0, madvrSettings.m_superRes);
  AddSlider(groupMadvrUpSharp, SET_IMAGE_SUPER_RES_STRENGTH, 70122, 0, madvrSettings.m_superResStrength, "%1.0f", 0.0f, 1.0f, 4.0f, 70121, usePopup);
  AddToggle(groupMadvrUpSharp, SET_IMAGE_SUPER_RES_LINEAR, 70133, 0, madvrSettings.m_superResLinear);

  AddToggle(groupMadvrUpSharp, SET_IMAGE_REFINE_ONCE, 70126, 0, madvrSettings.m_refineOnce);
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
  else if (settingId == SET_CHROMA_SUPER_RES_PASSES)
  {
    madvrSettings.m_ChromaSuperResPasses = static_cast<int>(static_cast<const CSettingInt*>(setting)->GetValue());
    CMadvrCallback::Get()->SetInt("superChromaResPasses", madvrSettings.m_ChromaSuperResPasses);
  }
  else if (settingId == SET_CHROMA_SUPER_RES_STRENGTH)
  {
    madvrSettings.m_ChromaSuperResStrength = static_cast<float>(static_cast<const CSettingNumber*>(setting)->GetValue());
    CMadvrCallback::Get()->SetFloat("superChromaResStrength", madvrSettings.m_ChromaSuperResStrength);
  }
  else if (settingId == SET_CHROMA_SUPER_RES_SOFTNESS)
  {
    madvrSettings.m_ChromaSuperResSoftness = static_cast<float>(static_cast<const CSettingNumber*>(setting)->GetValue());
    CMadvrCallback::Get()->SetFloat("superChromaResSoftness", madvrSettings.m_ChromaSuperResSoftness);
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
  }
  else if (settingId == SET_IMAGE_DOUBLE_CHROMA)
  {
    madvrSettings.m_ImageDoubleChroma = static_cast<int>(static_cast<const CSettingInt*>(setting)->GetValue());
  }
  else if (settingId == SET_IMAGE_QUADRUPLE_LUMA)
  {
    madvrSettings.m_ImageQuadrupleLuma = static_cast<int>(static_cast<const CSettingInt*>(setting)->GetValue());
  }
  else if (settingId == SET_IMAGE_QUADRUPLE_CHROMA)
  {
    madvrSettings.m_ImageQuadrupleChroma = static_cast<int>(static_cast<const CSettingInt*>(setting)->GetValue());
  }
  else if (settingId == SET_IMAGE_DOUBLE_LUMA_FACTOR)
  {
    madvrSettings.m_ImageDoubleLumaFactor = static_cast<int>(static_cast<const CSettingInt*>(setting)->GetValue());
  }
  else if (settingId == SET_IMAGE_DOUBLE_CHROMA_FACTOR)
  {
    madvrSettings.m_ImageDoubleChromaFactor = static_cast<int>(static_cast<const CSettingInt*>(setting)->GetValue());
  }
  else if (settingId == SET_IMAGE_QUADRUPLE_LUMA_FACTOR)
  {
    madvrSettings.m_ImageQuadrupleLumaFactor = static_cast<int>(static_cast<const CSettingInt*>(setting)->GetValue());
  }
  else if (settingId == SET_IMAGE_QUADRUPLE_CHROMA_FACTOR)
  {
    madvrSettings.m_ImageQuadrupleChromaFactor = static_cast<int>(static_cast<const CSettingInt*>(setting)->GetValue());
  }
  else if (settingId == SET_IMAGE_UPSHARPENEDGES)
  {
    madvrSettings.m_UpRefSharpenEdges = static_cast<const CSettingBool*>(setting)->GetValue();
    CMadvrCallback::Get()->SetBool("upRefSharpenEdges", madvrSettings.m_UpRefSharpenEdges);
  }
  else if (settingId == SET_IMAGE_UPSHARPENEDGES_STRENGTH)
  {
    madvrSettings.m_UpRefSharpenEdgesStrength = static_cast<float>(static_cast<const CSettingNumber*>(setting)->GetValue());
    CMadvrCallback::Get()->SetFloat("upRefSharpenEdgesStrength", madvrSettings.m_UpRefSharpenEdgesStrength, 10);
  }
  else if (settingId == SET_IMAGE_UPCRISPENEDGES)
  {
    madvrSettings.m_UpRefCrispenEdges = static_cast<const CSettingBool*>(setting)->GetValue();
    CMadvrCallback::Get()->SetBool("upRefCrispenEdges", madvrSettings.m_UpRefCrispenEdges);
  }
  else if (settingId == SET_IMAGE_UPCRISPENEDGES_STRENGTH)
  {
    madvrSettings.m_UpRefCrispenEdgesStrength = static_cast<float>(static_cast<const CSettingNumber*>(setting)->GetValue());
    CMadvrCallback::Get()->SetFloat("upRefCrispenEdgesStrength", madvrSettings.m_UpRefCrispenEdgesStrength, 10);
  }
  else if (settingId == SET_IMAGE_UPTHINEDGES)
  {
    madvrSettings.m_UpRefThinEdges = static_cast<const CSettingBool*>(setting)->GetValue();
    CMadvrCallback::Get()->SetBool("upRefThinEdges", madvrSettings.m_UpRefThinEdges);
  }
  else if (settingId == SET_IMAGE_UPTHINEDGES_STRENGTH)
  {
    madvrSettings.m_UpRefThinEdgesStrength = static_cast<float>(static_cast<const CSettingNumber*>(setting)->GetValue());
    CMadvrCallback::Get()->SetFloat("upRefThinEdgesStrength", madvrSettings.m_UpRefThinEdgesStrength, 10);
  }
  else if (settingId == SET_IMAGE_UPENHANCEDETAIL)
  {
    madvrSettings.m_UpRefEnhanceDetail = static_cast<const CSettingBool*>(setting)->GetValue();
    CMadvrCallback::Get()->SetBool("upRefEnhanceDetail", madvrSettings.m_UpRefEnhanceDetail);
  }
  else if (settingId == SET_IMAGE_UPENHANCEDETAIL_STRENGTH)
  {
    madvrSettings.m_UpRefEnhanceDetailStrength = static_cast<float>(static_cast<const CSettingNumber*>(setting)->GetValue());
    CMadvrCallback::Get()->SetFloat("upRefEnhanceDetailStrength", madvrSettings.m_UpRefEnhanceDetailStrength, 10);
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
  else if (settingId == SET_IMAGE_SUPER_RES_LINEAR)
  {
    madvrSettings.m_superResLinear = static_cast<const CSettingBool*>(setting)->GetValue();
    CMadvrCallback::Get()->SetBool("superResLinear", madvrSettings.m_superResLinear);
  }
  else if (settingId == SET_IMAGE_REFINE_ONCE)
  {
    madvrSettings.m_refineOnce = static_cast<const CSettingBool*>(setting)->GetValue();
    CMadvrCallback::Get()->SetBool("refineOnce", !madvrSettings.m_refineOnce);
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

  int value;
  bool bValue;
  bool bValue1;
  bool bValue2;
  bool bValue3;
  bool bValue4;
  bool bValue5;
  bool bValue6;

  CSetting *setting;

  // HIDE / SHOW

  // CHROMAUP
  setting = m_settingsManager->GetSetting(SET_CHROMA_SUPER_RES);
  bValue = static_cast<const CSettingBool*>(setting)->GetValue();
  SetVisible(SET_CHROMA_SUPER_RES_PASSES, bValue);
  SetVisible(SET_CHROMA_SUPER_RES_STRENGTH, bValue);
  SetVisible(SET_CHROMA_SUPER_RES_SOFTNESS, bValue);

  // UPDATE IMAGE DOUBLE
  CMadvrCallback::Get()->UpdateImageDouble();
  CMadvrSettings &madvrSettings = CMediaSettings::GetInstance().GetCurrentMadvrSettings();

  int iDoubleLuma = madvrSettings.m_ImageDoubleLuma;
  int iDoubleChroma = madvrSettings.m_ImageDoubleChroma;
  int iQuadrupleLuma = madvrSettings.m_ImageQuadrupleLuma;
  int iQuadrupleChroma = madvrSettings.m_ImageQuadrupleChroma;
  int iDoubleLumaFactor = madvrSettings.m_ImageDoubleLumaFactor;
  int iDoubleChromaFactor = madvrSettings.m_ImageDoubleChromaFactor;
  int iQuadrupleLumaFactor = madvrSettings.m_ImageQuadrupleLumaFactor;
  int iQuadrupleChromaFactor = madvrSettings.m_ImageQuadrupleChromaFactor;

  // IMAGE DOUBLE VISIBILITY  
  SetVisible(SET_IMAGE_DOUBLE_LUMA_FACTOR, IsEnabled(iDoubleLuma));
  SetVisible(SET_IMAGE_DOUBLE_CHROMA_FACTOR, IsEnabled(iDoubleChroma) && IsNNEDI3(iDoubleLuma));
  SetVisible(SET_IMAGE_QUADRUPLE_LUMA_FACTOR, IsEnabled(iQuadrupleLuma));
  SetVisible(SET_IMAGE_QUADRUPLE_CHROMA_FACTOR, IsEnabled(iQuadrupleChroma) && IsNNEDI3(iQuadrupleLuma));
  SetVisible(SET_IMAGE_DOUBLE_CHROMA, IsEnabled(iDoubleLuma) && IsNNEDI3(iDoubleLuma));
  SetVisible(SET_IMAGE_QUADRUPLE_LUMA, IsEnabled(iDoubleLuma));
  SetVisible(SET_IMAGE_QUADRUPLE_CHROMA, IsEnabled(iQuadrupleLuma) && IsNNEDI3(iQuadrupleLuma) && (IsEnabled(iDoubleChroma) || !IsNNEDI3(iQuadrupleLuma)));
  
  // SET NEW DOUBLE VALUE
  m_settingsManager->SetInt(SET_IMAGE_DOUBLE_CHROMA_FACTOR, iDoubleChromaFactor);
  m_settingsManager->SetInt(SET_IMAGE_QUADRUPLE_CHROMA_FACTOR, iQuadrupleChromaFactor);
  m_settingsManager->SetInt(SET_IMAGE_DOUBLE_CHROMA, iDoubleChroma);
  m_settingsManager->SetInt(SET_IMAGE_QUADRUPLE_LUMA, iQuadrupleLuma);
  m_settingsManager->SetInt(SET_IMAGE_QUADRUPLE_CHROMA, iQuadrupleChroma);

  // SET FAKE DISABLED BUTTON FOR DOUBLEHCROMA E QUADRUPLECHROMA
  m_settingsManager->SetInt(SET_FAKE_DOUBLE_CHROMA, iDoubleChroma);
  m_settingsManager->SetInt(SET_FAKE_QUADRUPLE_CHROMA, iQuadrupleChroma);
  m_settingsManager->SetInt(SET_FAKE_DOUBLE_CHROMA_FACTOR, iDoubleChromaFactor);
  m_settingsManager->SetInt(SET_FAKE_QUADRUPLE_CHROMA_FACTOR, iQuadrupleChromaFactor);  
  SetVisibleFake(SET_FAKE_DOUBLE_CHROMA, IsEnabled(iDoubleLuma) && !IsNNEDI3(iDoubleLuma));
  SetVisibleFake(SET_FAKE_DOUBLE_CHROMA_FACTOR, IsEnabled(iDoubleChroma) && !IsNNEDI3(iDoubleLuma));
  SetVisibleFake(SET_FAKE_QUADRUPLE_CHROMA, IsEnabled(iQuadrupleLuma) && !IsNNEDI3(iQuadrupleLuma) && (IsEnabled(iDoubleChroma) || !IsNNEDI3(iQuadrupleLuma)));
  SetVisibleFake(SET_FAKE_QUADRUPLE_CHROMA_FACTOR, IsEnabled(iQuadrupleChroma) && !IsNNEDI3(iQuadrupleLuma));

  // SHARP
  setting = m_settingsManager->GetSetting(SET_IMAGE_UPSHARPENEDGES);
  bValue = static_cast<const CSettingBool*>(setting)->GetValue();
  SetVisible(SET_IMAGE_UPSHARPENEDGES_STRENGTH, bValue);

  setting = m_settingsManager->GetSetting(SET_IMAGE_UPCRISPENEDGES);
  bValue = static_cast<const CSettingBool*>(setting)->GetValue();
  SetVisible(SET_IMAGE_UPCRISPENEDGES_STRENGTH, bValue);

  setting = m_settingsManager->GetSetting(SET_IMAGE_UPTHINEDGES);
  bValue = static_cast<const CSettingBool*>(setting)->GetValue();
  SetVisible(SET_IMAGE_UPTHINEDGES_STRENGTH, bValue);

  setting = m_settingsManager->GetSetting(SET_IMAGE_UPENHANCEDETAIL);
  bValue = static_cast<const CSettingBool*>(setting)->GetValue();
  SetVisible(SET_IMAGE_UPENHANCEDETAIL_STRENGTH, bValue);

  setting = m_settingsManager->GetSetting(SET_IMAGE_UPLUMASHARPEN);
  bValue = static_cast<const CSettingBool*>(setting)->GetValue();
  SetVisible(SET_IMAGE_UPLUMASHARPEN_STRENGTH, bValue);

  setting = m_settingsManager->GetSetting(SET_IMAGE_UPADAPTIVESHARPEN);
  bValue = static_cast<const CSettingBool*>(setting)->GetValue();
  SetVisible(SET_IMAGE_UPADAPTIVESHARPEN_STRENGTH, bValue);

  setting = m_settingsManager->GetSetting(SET_IMAGE_SUPER_RES);
  bValue = static_cast<const CSettingBool*>(setting)->GetValue();
  SetVisible(SET_IMAGE_SUPER_RES_STRENGTH, bValue);
  SetVisible(SET_IMAGE_SUPER_RES_LINEAR, bValue);

  setting = m_settingsManager->GetSetting(SET_IMAGE_UPSHARPENEDGES);
  bValue1 = static_cast<const CSettingBool*>(setting)->GetValue();
  setting = m_settingsManager->GetSetting(SET_IMAGE_UPCRISPENEDGES);
  bValue2 = static_cast<const CSettingBool*>(setting)->GetValue();
  setting = m_settingsManager->GetSetting(SET_IMAGE_UPTHINEDGES);
  bValue3 = static_cast<const CSettingBool*>(setting)->GetValue();
  setting = m_settingsManager->GetSetting(SET_IMAGE_UPENHANCEDETAIL);
  bValue4 = static_cast<const CSettingBool*>(setting)->GetValue();

  setting = m_settingsManager->GetSetting(SET_IMAGE_UPLUMASHARPEN);
  bValue5 = static_cast<const CSettingBool*>(setting)->GetValue();
  setting = m_settingsManager->GetSetting(SET_IMAGE_UPADAPTIVESHARPEN);
  bValue6 = static_cast<const CSettingBool*>(setting)->GetValue();
  SetVisible(SET_IMAGE_REFINE_ONCE, (bValue1||bValue2||bValue3||bValue4||bValue5||bValue6));

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

void CGUIDialogMadvrScaling::SetVisibleFake(CStdString id, bool visible)
{
  CSetting *setting = m_settingsManager->GetSetting(id);

  setting->SetVisible(visible);  
  setting->SetEnabled(visible);
  setting->SetEnabled(!visible);
}
