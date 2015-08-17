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
}

void CGUIDialogMadvrScaling::InitializeSettings()
{
  CGUIDialogSettingsManualBase::InitializeSettings();

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

  StaticIntegerSettingOptions entries, entriesDoubleFactor, entriesQuadrupleFactor;
  CMadvrSettings &madvrSettings = CMediaSettings::Get().GetCurrentMadvrSettings();

  //MADVR CHROMA UPSCALING

  entries.clear();
  entries.push_back(make_pair(70001, MADVR_SCALING_NEAREST_NEIGHBOR));
  entries.push_back(make_pair(70002, MADVR_SCALING_BILINEAR));
  entries.push_back(make_pair(70004, MADVR_SCALING_MITCHEL_NETRAVALI));
  entries.push_back(make_pair(70005, MADVR_SCALING_CATMULL_ROM));
  entries.push_back(make_pair(70006, MADVR_SCALING_BICUBIC_50));
  entries.push_back(make_pair(70007, MADVR_SCALING_BICUBIC_60));
  entries.push_back(make_pair(70008, MADVR_SCALING_BICUBIC_75));
  entries.push_back(make_pair(70009, MADVR_SCALING_BICUBIC_100));
  entries.push_back(make_pair(70010, MADVR_SCALING_SOFTCUBIC_50));
  entries.push_back(make_pair(70011, MADVR_SCALING_SOFTCUBIC_60));
  entries.push_back(make_pair(70012, MADVR_SCALING_SOFTCUBIC_70));
  entries.push_back(make_pair(70013, MADVR_SCALING_SOFTCUBIC_80));
  entries.push_back(make_pair(70014, MADVR_SCALING_SOFTCUBIC_100));
  entries.push_back(make_pair(70015, MADVR_SCALING_LANCZOS_3));
  entries.push_back(make_pair(70016, MADVR_SCALING_LANCZOS_4));
  entries.push_back(make_pair(70018, MADVR_SCALING_SPLINE_36));
  entries.push_back(make_pair(70019, MADVR_SCALING_SPLINE_64));
  entries.push_back(make_pair(70020, MADVR_SCALING_JINC_3));
  entries.push_back(make_pair(70033, MADVR_SCALING_BILATERAL));
  entries.push_back(make_pair(70034, MADVR_SCALING_SUPERXBR25));
  entries.push_back(make_pair(70035, MADVR_SCALING_SUPERXBR50));
  entries.push_back(make_pair(70036, MADVR_SCALING_SUPERXBR75));
  entries.push_back(make_pair(70037, MADVR_SCALING_SUPERXBR100));
  entries.push_back(make_pair(70038, MADVR_SCALING_SUPERXBR125));
  entries.push_back(make_pair(70039, MADVR_SCALING_SUPERXBR150));
  entries.push_back(make_pair(70040, MADVR_SCALING_NEDI));
  entries.push_back(make_pair(70023, MADVR_SCALING_NNEDI3_16));
  entries.push_back(make_pair(70024, MADVR_SCALING_NNEDI3_32));
  entries.push_back(make_pair(70025, MADVR_SCALING_NNEDI3_64));
  entries.push_back(make_pair(70026, MADVR_SCALING_NNEDI3_128));
  entries.push_back(make_pair(70027, MADVR_SCALING_NNEDI3_256));

  AddList(groupMadvrChromaScaling, SET_CHROMA_UPSCALING, 70028, 0, static_cast<int>(madvrSettings.m_ChromaUpscaling), entries, 70028);
  AddToggle(groupMadvrChromaScaling, SET_CHROMA_ANTIRING, 70031, 0, madvrSettings.m_ChromaAntiRing);
  AddToggle(groupMadvrChromaScaling, SET_CHROMA_SUPER_RES, 70041, 0, madvrSettings.m_ChromaSuperRes);

  //MADVR IMAGE UPSCALING
  entries.clear();
  entries.push_back(make_pair(70001, MADVR_SCALING_NEAREST_NEIGHBOR));
  entries.push_back(make_pair(70002, MADVR_SCALING_BILINEAR));
  entries.push_back(make_pair(70003, MADVR_SCALING_DXVA2));
  entries.push_back(make_pair(70004, MADVR_SCALING_MITCHEL_NETRAVALI));
  entries.push_back(make_pair(70005, MADVR_SCALING_CATMULL_ROM));
  entries.push_back(make_pair(70006, MADVR_SCALING_BICUBIC_50));
  entries.push_back(make_pair(70007, MADVR_SCALING_BICUBIC_60));
  entries.push_back(make_pair(70008, MADVR_SCALING_BICUBIC_75));
  entries.push_back(make_pair(70009, MADVR_SCALING_BICUBIC_100));
  entries.push_back(make_pair(70010, MADVR_SCALING_SOFTCUBIC_50));
  entries.push_back(make_pair(70011, MADVR_SCALING_SOFTCUBIC_60));
  entries.push_back(make_pair(70012, MADVR_SCALING_SOFTCUBIC_70));
  entries.push_back(make_pair(70013, MADVR_SCALING_SOFTCUBIC_80));
  entries.push_back(make_pair(70014, MADVR_SCALING_SOFTCUBIC_100));
  entries.push_back(make_pair(70015, MADVR_SCALING_LANCZOS_3));
  entries.push_back(make_pair(70016, MADVR_SCALING_LANCZOS_4));
  entries.push_back(make_pair(70018, MADVR_SCALING_SPLINE_36));
  entries.push_back(make_pair(70019, MADVR_SCALING_SPLINE_64));
  entries.push_back(make_pair(70020, MADVR_SCALING_JINC_3));
  AddList(groupMadvrUpScaling, SET_IMAGE_UPSCALING, 70029, 0, static_cast<int>(madvrSettings.m_ImageUpscaling), entries, 70029);
  AddToggle(groupMadvrUpScaling, SET_IMAGE_UP_ANTIRING, 70031, 0, madvrSettings.m_ImageUpAntiRing);
  AddToggle(groupMadvrUpScaling, SET_IMAGE_UP_LINEAR, 70032, 0, madvrSettings.m_ImageUpLinear);

  //MADVR IMAGE DOWNSCALING
  entries.clear();
  entries.push_back(make_pair(70001, MADVR_SCALING_NEAREST_NEIGHBOR));
  entries.push_back(make_pair(70002, MADVR_SCALING_BILINEAR));
  entries.push_back(make_pair(70003, MADVR_SCALING_DXVA2));
  entries.push_back(make_pair(70004, MADVR_SCALING_MITCHEL_NETRAVALI));
  entries.push_back(make_pair(70005, MADVR_SCALING_CATMULL_ROM));
  entries.push_back(make_pair(70006, MADVR_SCALING_BICUBIC_50));
  entries.push_back(make_pair(70007, MADVR_SCALING_BICUBIC_60));
  entries.push_back(make_pair(70008, MADVR_SCALING_BICUBIC_75));
  entries.push_back(make_pair(70009, MADVR_SCALING_BICUBIC_100));
  entries.push_back(make_pair(70010, MADVR_SCALING_SOFTCUBIC_50));
  entries.push_back(make_pair(70011, MADVR_SCALING_SOFTCUBIC_60));
  entries.push_back(make_pair(70012, MADVR_SCALING_SOFTCUBIC_70));
  entries.push_back(make_pair(70013, MADVR_SCALING_SOFTCUBIC_80));
  entries.push_back(make_pair(70014, MADVR_SCALING_SOFTCUBIC_100));
  entries.push_back(make_pair(70015, MADVR_SCALING_LANCZOS_3));
  entries.push_back(make_pair(70016, MADVR_SCALING_LANCZOS_4));
  entries.push_back(make_pair(70017, MADVR_SCALING_LANCZOS_8));
  entries.push_back(make_pair(70018, MADVR_SCALING_SPLINE_36));
  entries.push_back(make_pair(70019, MADVR_SCALING_SPLINE_64));
  entries.push_back(make_pair(70020, MADVR_SCALING_JINC_3));
  entries.push_back(make_pair(70021, MADVR_SCALING_JINC_4));
  entries.push_back(make_pair(70022, MADVR_SCALING_JINC_8));
  AddList(groupMadvrDownScaling, SET_IMAGE_DOWNSCALING, 70030, 0, static_cast<int>(madvrSettings.m_ImageDownscaling), entries, 70030);
  AddToggle(groupMadvrDownScaling, SET_IMAGE_DOWN_ANTIRING, 70031, 0, madvrSettings.m_ImageDownAntiRing);
  AddToggle(groupMadvrDownScaling, SET_IMAGE_DOWN_LINEAR, 70032, 0, madvrSettings.m_ImageDownLinear);

  // MADVR IMAGE DOUBLING

  entries.clear();
  entries.push_back(make_pair(70117, -1));
  entries.push_back(make_pair(70104, MADVR_NNEDI3_16NEURONS));
  entries.push_back(make_pair(70105, MADVR_NNEDI3_32NEURONS));
  entries.push_back(make_pair(70106, MADVR_NNEDI3_64NEURONS));
  entries.push_back(make_pair(70107, MADVR_NNEDI3_128NEURONS));
  entries.push_back(make_pair(70108, MADVR_NNEDI3_256NEURONS));

  entriesDoubleFactor.clear();
  entriesDoubleFactor.push_back(make_pair(70109, MADVR_DOUBLE_FACTOR_2_0));
  entriesDoubleFactor.push_back(make_pair(70110, MADVR_DOUBLE_FACTOR_1_5));
  entriesDoubleFactor.push_back(make_pair(70111, MADVR_DOUBLE_FACTOR_1_2));
  entriesDoubleFactor.push_back(make_pair(70112, MADVR_DOUBLE_FACTOR_ALWAYS));

  entriesQuadrupleFactor.clear();
  entriesQuadrupleFactor.push_back(make_pair(70113, MADVR_QUADRUPLE_FACTOR_4_0));
  entriesQuadrupleFactor.push_back(make_pair(70114, MADVR_QUADRUPLE_FACTOR_3_0));
  entriesQuadrupleFactor.push_back(make_pair(70115, MADVR_QUADRUPLE_FACTOR_2_4));
  entriesQuadrupleFactor.push_back(make_pair(70112, MADVR_QUADRUPLE_FACTOR_ALWAYS));

  AddList(groupMadvrDoubling, SET_IMAGE_DOUBLE_LUMA, 70100, 0, static_cast<int>(madvrSettings.m_ImageDoubleLuma), entries, 70100);
  AddList(groupMadvrDoubling, SET_IMAGE_DOUBLE_LUMA_FACTOR, 70116, 0, static_cast<int>(madvrSettings.m_ImageDoubleLumaFactor), entriesDoubleFactor, 70116);

  AddList(groupMadvrDoubling, SET_IMAGE_DOUBLE_CHROMA, 70101, 0, static_cast<int>(madvrSettings.m_ImageDoubleChroma), entries, 70101);
  AddList(groupMadvrDoubling, SET_IMAGE_DOUBLE_CHROMA_FACTOR, 70116, 0, static_cast<int>(madvrSettings.m_ImageDoubleChromaFactor), entriesDoubleFactor, 70116);

  AddList(groupMadvrDoubling, SET_IMAGE_QUADRUPLE_LUMA, 70102, 0, static_cast<int>(madvrSettings.m_ImageQuadrupleLuma), entries, 70102);
  AddList(groupMadvrDoubling, SET_IMAGE_QUADRUPLE_LUMA_FACTOR, 70116, 0, static_cast<int>(madvrSettings.m_ImageQuadrupleLumaFactor), entriesQuadrupleFactor, 70116);

  AddList(groupMadvrDoubling, SET_IMAGE_QUADRUPLE_CHROMA, 70103, 0, static_cast<int>(madvrSettings.m_ImageQuadrupleChroma), entries, 70103);
  AddList(groupMadvrDoubling, SET_IMAGE_QUADRUPLE_CHROMA_FACTOR, 70116, 0, static_cast<int>(madvrSettings.m_ImageQuadrupleChromaFactor), entriesQuadrupleFactor, 70116);

}

void CGUIDialogMadvrScaling::OnSettingChanged(const CSetting *setting)
{
  if (setting == NULL)
    return;

  CGUIDialogSettingsManualBase::OnSettingChanged(setting);

  CMadvrSettings &madvrSettings = CMediaSettings::Get().GetCurrentMadvrSettings();

  const std::string &settingId = setting->GetId();

  if (settingId == SET_CHROMA_UPSCALING)
  {
    madvrSettings.m_ChromaUpscaling = static_cast<MADVR_SCALING>(static_cast<const CSettingInt*>(setting)->GetValue());
    CMadvrCallback::Get()->GetCallback()->SettingSetScaling("chromaUp", madvrSettings.m_ChromaUpscaling);
  }
  else if (settingId == SET_CHROMA_ANTIRING)
  {
    madvrSettings.m_ChromaAntiRing = static_cast<const CSettingBool*>(setting)->GetValue();
    CMadvrCallback::Get()->GetCallback()->SettingSetBool("chromaAntiRinging", madvrSettings.m_ChromaAntiRing);
  }
  else if (settingId == SET_CHROMA_SUPER_RES)
  {
    madvrSettings.m_ChromaSuperRes = static_cast<const CSettingBool*>(setting)->GetValue();
    CMadvrCallback::Get()->GetCallback()->SettingSetBool("superChromaRes", madvrSettings.m_ChromaSuperRes);
  }
  else if (settingId == SET_IMAGE_UPSCALING)
  {
    madvrSettings.m_ImageUpscaling = static_cast<MADVR_SCALING>(static_cast<const CSettingInt*>(setting)->GetValue());
    CMadvrCallback::Get()->GetCallback()->SettingSetScaling("LumaUp", madvrSettings.m_ImageUpscaling);
  }
  else if (settingId == SET_IMAGE_UP_ANTIRING)
  {
    madvrSettings.m_ImageUpAntiRing = static_cast<const CSettingBool*>(setting)->GetValue();
    CMadvrCallback::Get()->GetCallback()->SettingSetBool("lumaUpAntiRinging", madvrSettings.m_ImageUpAntiRing);
  }
  else if (settingId == SET_IMAGE_UP_LINEAR)
  {
    madvrSettings.m_ImageUpLinear = static_cast<const CSettingBool*>(setting)->GetValue();
    CMadvrCallback::Get()->GetCallback()->SettingSetBool("lumaUpLinear", madvrSettings.m_ImageUpLinear);
  }
  else if (settingId == SET_IMAGE_DOWNSCALING)
  {
    madvrSettings.m_ImageDownscaling = static_cast<MADVR_SCALING>(static_cast<const CSettingInt*>(setting)->GetValue());
    CMadvrCallback::Get()->GetCallback()->SettingSetScaling("LumaDown", madvrSettings.m_ImageDownscaling);
  }
  else if (settingId == SET_IMAGE_DOWN_ANTIRING)
  {
    madvrSettings.m_ImageDownAntiRing = static_cast<const CSettingBool*>(setting)->GetValue();
    CMadvrCallback::Get()->GetCallback()->SettingSetBool("lumaDownAntiRinging", madvrSettings.m_ImageDownAntiRing);
  }
  else if (settingId == SET_IMAGE_DOWN_LINEAR)
  {
    madvrSettings.m_ImageDownLinear = static_cast<const CSettingBool*>(setting)->GetValue();
    CMadvrCallback::Get()->GetCallback()->SettingSetBool("lumaDownLinear", madvrSettings.m_ImageDownLinear);
  }
  else if (settingId == SET_IMAGE_DOUBLE_LUMA)
  {
    madvrSettings.m_ImageDoubleLuma = static_cast<int>(static_cast<const CSettingInt*>(setting)->GetValue());
    CMadvrCallback::Get()->GetCallback()->SettingSetDoubling("nnediDL", madvrSettings.m_ImageDoubleLuma);
    CMadvrCallback::Get()->GetCallback()->SettingSetDoublingCondition("nnediDLScalingFactor", madvrSettings.m_ImageDoubleLumaFactor);
  }
  else if (settingId == SET_IMAGE_DOUBLE_CHROMA)
  {
    madvrSettings.m_ImageDoubleChroma = static_cast<int>(static_cast<const CSettingInt*>(setting)->GetValue());
    CMadvrCallback::Get()->GetCallback()->SettingSetDoubling("nnediDC", madvrSettings.m_ImageDoubleChroma);
    CMadvrCallback::Get()->GetCallback()->SettingSetDoublingCondition("nnediDCScalingFactor", madvrSettings.m_ImageDoubleChromaFactor);
  }
  else if (settingId == SET_IMAGE_QUADRUPLE_LUMA)
  {
    madvrSettings.m_ImageQuadrupleLuma = static_cast<int>(static_cast<const CSettingInt*>(setting)->GetValue());
    CMadvrCallback::Get()->GetCallback()->SettingSetDoubling("nnediQL", madvrSettings.m_ImageQuadrupleLuma);
    CMadvrCallback::Get()->GetCallback()->SettingSetQuadrupleCondition("nnediQLScalingFactor", madvrSettings.m_ImageQuadrupleLumaFactor);
  }
  else if (settingId == SET_IMAGE_QUADRUPLE_CHROMA)
  {
    madvrSettings.m_ImageQuadrupleChroma = static_cast<int>(static_cast<const CSettingInt*>(setting)->GetValue());
    CMadvrCallback::Get()->GetCallback()->SettingSetDoubling("nnediQC", madvrSettings.m_ImageQuadrupleChroma);
    CMadvrCallback::Get()->GetCallback()->SettingSetQuadrupleCondition("nnediQCScalingFactor", madvrSettings.m_ImageQuadrupleChromaFactor);
  }
  else if (settingId == SET_IMAGE_DOUBLE_LUMA_FACTOR)
  {
    madvrSettings.m_ImageDoubleLumaFactor = static_cast<int>(static_cast<const CSettingInt*>(setting)->GetValue());
    CMadvrCallback::Get()->GetCallback()->SettingSetDoublingCondition("nnediDLScalingFactor", madvrSettings.m_ImageDoubleLumaFactor);
  }
  else if (settingId == SET_IMAGE_DOUBLE_CHROMA_FACTOR)
  {
    madvrSettings.m_ImageDoubleChromaFactor = static_cast<int>(static_cast<const CSettingInt*>(setting)->GetValue());
    CMadvrCallback::Get()->GetCallback()->SettingSetDoublingCondition("nnediDCScalingFactor", madvrSettings.m_ImageDoubleChromaFactor);
  }
  else if (settingId == SET_IMAGE_QUADRUPLE_LUMA_FACTOR)
  {
    madvrSettings.m_ImageQuadrupleLumaFactor = static_cast<int>(static_cast<const CSettingInt*>(setting)->GetValue());
    CMadvrCallback::Get()->GetCallback()->SettingSetQuadrupleCondition("nnediQLScalingFactor", madvrSettings.m_ImageQuadrupleLumaFactor);
  }
  else if (settingId == SET_IMAGE_QUADRUPLE_CHROMA_FACTOR)
  {
    madvrSettings.m_ImageQuadrupleChromaFactor = static_cast<int>(static_cast<const CSettingInt*>(setting)->GetValue());
    CMadvrCallback::Get()->GetCallback()->SettingSetQuadrupleCondition("nnediQCScalingFactor", madvrSettings.m_ImageQuadrupleChromaFactor);
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

  CMadvrSettings &madvrSettings = CMediaSettings::Get().GetCurrentMadvrSettings();

  int value;
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
