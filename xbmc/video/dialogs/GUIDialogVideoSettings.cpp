/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIDialogVideoSettings.h"

#include "GUIPassword.h"
#include "ServiceBroker.h"
#include "addons/Skin.h"
#include "application/ApplicationComponents.h"
#include "application/ApplicationPlayer.h"
#include "dialogs/GUIDialogYesNo.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "profiles/ProfileManager.h"
#include "settings/MediaSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "settings/lib/Setting.h"
#include "settings/lib/SettingDefinitions.h"
#include "settings/lib/SettingsManager.h"
#include "utils/LangCodeExpander.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"
#include "utils/log.h"
#include "video/VideoDatabase.h"
#include "video/ViewModeSettings.h"

#include <utility>

#define SETTING_VIDEO_VIEW_MODE           "video.viewmode"
#define SETTING_VIDEO_ZOOM                "video.zoom"
#define SETTING_VIDEO_PIXEL_RATIO         "video.pixelratio"
#define SETTING_VIDEO_BRIGHTNESS          "video.brightness"
#define SETTING_VIDEO_CONTRAST            "video.contrast"
#define SETTING_VIDEO_GAMMA               "video.gamma"
#define SETTING_VIDEO_NONLIN_STRETCH      "video.nonlinearstretch"
#define SETTING_VIDEO_POSTPROCESS         "video.postprocess"
#define SETTING_VIDEO_VERTICAL_SHIFT      "video.verticalshift"
#define SETTING_VIDEO_TONEMAP_METHOD      "video.tonemapmethod"
#define SETTING_VIDEO_TONEMAP_PARAM       "video.tonemapparam"
#define SETTING_VIDEO_ORIENTATION         "video.orientation"

#define SETTING_VIDEO_VDPAU_NOISE         "vdpau.noise"
#define SETTING_VIDEO_VDPAU_SHARPNESS     "vdpau.sharpness"

#define SETTING_VIDEO_INTERLACEMETHOD     "video.interlacemethod"
#define SETTING_VIDEO_SCALINGMETHOD       "video.scalingmethod"

#define SETTING_VIDEO_STEREOSCOPICMODE    "video.stereoscopicmode"
#define SETTING_VIDEO_STEREOSCOPICINVERT  "video.stereoscopicinvert"

#define SETTING_VIDEO_MAKE_DEFAULT        "video.save"
#define SETTING_VIDEO_CALIBRATION         "video.calibration"
#define SETTING_VIDEO_STREAM              "video.stream"

CGUIDialogVideoSettings::CGUIDialogVideoSettings()
    : CGUIDialogSettingsManualBase(WINDOW_DIALOG_VIDEO_OSD_SETTINGS, "DialogSettings.xml")
{ }

CGUIDialogVideoSettings::~CGUIDialogVideoSettings() = default;

void CGUIDialogVideoSettings::OnSettingChanged(const std::shared_ptr<const CSetting>& setting)
{
  if (setting == NULL)
    return;

  CGUIDialogSettingsManualBase::OnSettingChanged(setting);

  auto& components = CServiceBroker::GetAppComponents();
  const auto appPlayer = components.GetComponent<CApplicationPlayer>();

  const std::string &settingId = setting->GetId();
  if (settingId == SETTING_VIDEO_INTERLACEMETHOD)
  {
    CVideoSettings vs = appPlayer->GetVideoSettings();
    vs.m_InterlaceMethod = static_cast<EINTERLACEMETHOD>(std::static_pointer_cast<const CSettingInt>(setting)->GetValue());
    appPlayer->SetVideoSettings(vs);
  }
  else if (settingId == SETTING_VIDEO_SCALINGMETHOD)
  {
    CVideoSettings vs = appPlayer->GetVideoSettings();
    vs.m_ScalingMethod = static_cast<ESCALINGMETHOD>(std::static_pointer_cast<const CSettingInt>(setting)->GetValue());
    appPlayer->SetVideoSettings(vs);
  }
  else if (settingId == SETTING_VIDEO_STREAM)
  {
    m_videoStream = std::static_pointer_cast<const CSettingInt>(setting)->GetValue();
    // only change the video stream if a different one has been asked for
    if (appPlayer->GetVideoStream() != m_videoStream)
    {
      appPlayer->SetVideoStream(m_videoStream); // Set the video stream to the one selected
    }
  }
  else if (settingId == SETTING_VIDEO_VIEW_MODE)
  {
    int value = std::static_pointer_cast<const CSettingInt>(setting)->GetValue();
    const CVideoSettings vs = appPlayer->GetVideoSettings();

    appPlayer->SetRenderViewMode(value, vs.m_CustomZoomAmount, vs.m_CustomPixelRatio,
                                 vs.m_CustomVerticalShift, vs.m_CustomNonLinStretch);

    m_viewModeChanged = true;
    GetSettingsManager()->SetNumber(SETTING_VIDEO_ZOOM, static_cast<double>(vs.m_CustomZoomAmount));
    GetSettingsManager()->SetNumber(SETTING_VIDEO_PIXEL_RATIO,
                                    static_cast<double>(vs.m_CustomPixelRatio));
    GetSettingsManager()->SetNumber(SETTING_VIDEO_VERTICAL_SHIFT,
                                    static_cast<double>(vs.m_CustomVerticalShift));
    GetSettingsManager()->SetBool(SETTING_VIDEO_NONLIN_STRETCH, vs.m_CustomNonLinStretch);
    m_viewModeChanged = false;
  }
  else if (settingId == SETTING_VIDEO_ZOOM ||
           settingId == SETTING_VIDEO_VERTICAL_SHIFT ||
           settingId == SETTING_VIDEO_PIXEL_RATIO ||
           settingId == SETTING_VIDEO_NONLIN_STRETCH)
  {
    CVideoSettings vs = appPlayer->GetVideoSettings();
    if (settingId == SETTING_VIDEO_ZOOM)
      vs.m_CustomZoomAmount = static_cast<float>(std::static_pointer_cast<const CSettingNumber>(setting)->GetValue());
    else if (settingId == SETTING_VIDEO_VERTICAL_SHIFT)
      vs.m_CustomVerticalShift = static_cast<float>(std::static_pointer_cast<const CSettingNumber>(setting)->GetValue());
    else if (settingId == SETTING_VIDEO_PIXEL_RATIO)
      vs.m_CustomPixelRatio = static_cast<float>(std::static_pointer_cast<const CSettingNumber>(setting)->GetValue());
    else if (settingId == SETTING_VIDEO_NONLIN_STRETCH)
      vs.m_CustomNonLinStretch = std::static_pointer_cast<const CSettingBool>(setting)->GetValue();

    // try changing the view mode to custom. If it already is set to custom
    // manually call the render manager
    if (GetSettingsManager()->GetInt(SETTING_VIDEO_VIEW_MODE) != ViewModeCustom)
      GetSettingsManager()->SetInt(SETTING_VIDEO_VIEW_MODE, ViewModeCustom);
    else
      appPlayer->SetRenderViewMode(vs.m_ViewMode, vs.m_CustomZoomAmount, vs.m_CustomPixelRatio,
                                   vs.m_CustomVerticalShift, vs.m_CustomNonLinStretch);
  }
  else if (settingId == SETTING_VIDEO_POSTPROCESS)
  {
    CVideoSettings vs = appPlayer->GetVideoSettings();
    vs.m_PostProcess = std::static_pointer_cast<const CSettingBool>(setting)->GetValue();
    appPlayer->SetVideoSettings(vs);
  }
  else if (settingId == SETTING_VIDEO_BRIGHTNESS)
  {
    CVideoSettings vs = appPlayer->GetVideoSettings();
    vs.m_Brightness = static_cast<float>(std::static_pointer_cast<const CSettingInt>(setting)->GetValue());
    appPlayer->SetVideoSettings(vs);
  }
  else if (settingId == SETTING_VIDEO_CONTRAST)
  {
    CVideoSettings vs = appPlayer->GetVideoSettings();
    vs.m_Contrast = static_cast<float>(std::static_pointer_cast<const CSettingInt>(setting)->GetValue());
    appPlayer->SetVideoSettings(vs);
  }
  else if (settingId == SETTING_VIDEO_GAMMA)
  {
    CVideoSettings vs = appPlayer->GetVideoSettings();
    vs.m_Gamma = static_cast<float>(std::static_pointer_cast<const CSettingInt>(setting)->GetValue());
    appPlayer->SetVideoSettings(vs);
  }
  else if (settingId == SETTING_VIDEO_VDPAU_NOISE)
  {
    CVideoSettings vs = appPlayer->GetVideoSettings();
    vs.m_NoiseReduction = static_cast<float>(std::static_pointer_cast<const CSettingNumber>(setting)->GetValue());
    appPlayer->SetVideoSettings(vs);
  }
  else if (settingId == SETTING_VIDEO_VDPAU_SHARPNESS)
  {
    CVideoSettings vs = appPlayer->GetVideoSettings();
    vs.m_Sharpness = static_cast<float>(std::static_pointer_cast<const CSettingNumber>(setting)->GetValue());
    appPlayer->SetVideoSettings(vs);
  }
  else if (settingId == SETTING_VIDEO_TONEMAP_METHOD)
  {
    CVideoSettings vs = appPlayer->GetVideoSettings();
    vs.m_ToneMapMethod = static_cast<ETONEMAPMETHOD>(
        std::static_pointer_cast<const CSettingInt>(setting)->GetValue());
    appPlayer->SetVideoSettings(vs);
  }
  else if (settingId == SETTING_VIDEO_TONEMAP_PARAM)
  {
    CVideoSettings vs = appPlayer->GetVideoSettings();
    vs.m_ToneMapParam = static_cast<float>(std::static_pointer_cast<const CSettingNumber>(setting)->GetValue());
    appPlayer->SetVideoSettings(vs);
  }
  else if (settingId == SETTING_VIDEO_ORIENTATION)
  {
    CVideoSettings vs = appPlayer->GetVideoSettings();
    vs.m_Orientation = std::static_pointer_cast<const CSettingInt>(setting)->GetValue();
    appPlayer->SetVideoSettings(vs);
  }
  else if (settingId == SETTING_VIDEO_STEREOSCOPICMODE)
  {
    CVideoSettings vs = appPlayer->GetVideoSettings();
    vs.m_StereoMode = std::static_pointer_cast<const CSettingInt>(setting)->GetValue();
    appPlayer->SetVideoSettings(vs);
  }
  else if (settingId == SETTING_VIDEO_STEREOSCOPICINVERT)
  {
    CVideoSettings vs = appPlayer->GetVideoSettings();
    vs.m_StereoInvert = std::static_pointer_cast<const CSettingBool>(setting)->GetValue();
    appPlayer->SetVideoSettings(vs);
  }
}

void CGUIDialogVideoSettings::OnSettingAction(const std::shared_ptr<const CSetting>& setting)
{
  if (setting == NULL)
    return;

  CGUIDialogSettingsManualBase::OnSettingChanged(setting);

  const std::string &settingId = setting->GetId();
  if (settingId == SETTING_VIDEO_CALIBRATION)
  {
    const std::shared_ptr<CProfileManager> profileManager = CServiceBroker::GetSettingsComponent()->GetProfileManager();

    auto settingsComponent = CServiceBroker::GetSettingsComponent();
    if (!settingsComponent)
      return;

    auto settings = settingsComponent->GetSettings();
    if (!settings)
      return;

    auto calibsetting = settings->GetSetting(CSettings::SETTING_VIDEOSCREEN_GUICALIBRATION);
    if (!calibsetting)
    {
      CLog::Log(LOGERROR, "Failed to load setting for: {}",
                CSettings::SETTING_VIDEOSCREEN_GUICALIBRATION);
      return;
    }

    // launch calibration window
    if (profileManager->GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE &&
        g_passwordManager.CheckSettingLevelLock(calibsetting->GetLevel()))
      return;

    CServiceBroker::GetGUI()->GetWindowManager().ForceActivateWindow(WINDOW_SCREEN_CALIBRATION);
  }
  //! @todo implement
  else if (settingId == SETTING_VIDEO_MAKE_DEFAULT)
    Save();
}

bool CGUIDialogVideoSettings::Save()
{
  const std::shared_ptr<CProfileManager> profileManager = CServiceBroker::GetSettingsComponent()->GetProfileManager();

  if (profileManager->GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE &&
      !g_passwordManager.CheckSettingLevelLock(::SettingLevel::Expert))
    return true;

  // prompt user if they are sure
  if (CGUIDialogYesNo::ShowAndGetInput(CVariant(12376), CVariant(12377)))
  { // reset the settings
    CVideoDatabase db;
    if (!db.Open())
      return true;
    db.EraseAllVideoSettings();
    db.Close();

    const auto& components = CServiceBroker::GetAppComponents();
    const auto appPlayer = components.GetComponent<CApplicationPlayer>();

    CMediaSettings::GetInstance().GetDefaultVideoSettings() = appPlayer->GetVideoSettings();
    CMediaSettings::GetInstance().GetDefaultVideoSettings().m_SubtitleStream = -1;
    CMediaSettings::GetInstance().GetDefaultVideoSettings().m_AudioStream = -1;
    CServiceBroker::GetSettingsComponent()->GetSettings()->Save();
  }

  return true;
}

void CGUIDialogVideoSettings::SetupView()
{
  CGUIDialogSettingsManualBase::SetupView();

  SetHeading(13395);
  SET_CONTROL_HIDDEN(CONTROL_SETTINGS_OKAY_BUTTON);
  SET_CONTROL_HIDDEN(CONTROL_SETTINGS_CUSTOM_BUTTON);
  SET_CONTROL_LABEL(CONTROL_SETTINGS_CANCEL_BUTTON, 15067);
}

void CGUIDialogVideoSettings::InitializeSettings()
{
  CGUIDialogSettingsManualBase::InitializeSettings();

  const std::shared_ptr<CSettingCategory> category = AddCategory("videosettings", -1);
  if (category == NULL)
  {
    CLog::Log(LOGERROR, "CGUIDialogVideoSettings: unable to setup settings");
    return;
  }

  // get all necessary setting groups
  const std::shared_ptr<CSettingGroup> groupVideoStream = AddGroup(category);
  if (groupVideoStream == NULL)
  {
    CLog::Log(LOGERROR, "CGUIDialogVideoSettings: unable to setup settings");
    return;
  }
  const std::shared_ptr<CSettingGroup> groupVideo = AddGroup(category);
  if (groupVideo == NULL)
  {
    CLog::Log(LOGERROR, "CGUIDialogVideoSettings: unable to setup settings");
    return;
  }
  const std::shared_ptr<CSettingGroup> groupStereoscopic = AddGroup(category);
  if (groupStereoscopic == NULL)
  {
    CLog::Log(LOGERROR, "CGUIDialogVideoSettings: unable to setup settings");
    return;
  }
  const std::shared_ptr<CSettingGroup> groupSaveAsDefault = AddGroup(category);
  if (groupSaveAsDefault == NULL)
  {
    CLog::Log(LOGERROR, "CGUIDialogVideoSettings: unable to setup settings");
    return;
  }

  bool usePopup = g_SkinInfo->HasSkinFile("DialogSlider.xml");

  const auto& components = CServiceBroker::GetAppComponents();
  const auto appPlayer = components.GetComponent<CApplicationPlayer>();

  const CVideoSettings videoSettings = appPlayer->GetVideoSettings();

  TranslatableIntegerSettingOptions entries;

  entries.clear();
  entries.emplace_back(16039, VS_INTERLACEMETHOD_NONE);
  entries.emplace_back(16019, VS_INTERLACEMETHOD_AUTO);
  entries.emplace_back(20131, VS_INTERLACEMETHOD_RENDER_BLEND);
  entries.emplace_back(20129, VS_INTERLACEMETHOD_RENDER_WEAVE);
  entries.emplace_back(16021, VS_INTERLACEMETHOD_RENDER_BOB);
  entries.emplace_back(16020, VS_INTERLACEMETHOD_DEINTERLACE);
  entries.emplace_back(16036, VS_INTERLACEMETHOD_DEINTERLACE_HALF);
  entries.emplace_back(16311, VS_INTERLACEMETHOD_VDPAU_TEMPORAL_SPATIAL);
  entries.emplace_back(16310, VS_INTERLACEMETHOD_VDPAU_TEMPORAL);
  entries.emplace_back(16325, VS_INTERLACEMETHOD_VDPAU_BOB);
  entries.emplace_back(16318, VS_INTERLACEMETHOD_VDPAU_TEMPORAL_SPATIAL_HALF);
  entries.emplace_back(16317, VS_INTERLACEMETHOD_VDPAU_TEMPORAL_HALF);
  entries.emplace_back(16327, VS_INTERLACEMETHOD_VAAPI_BOB);
  entries.emplace_back(16328, VS_INTERLACEMETHOD_VAAPI_MADI);
  entries.emplace_back(16329, VS_INTERLACEMETHOD_VAAPI_MACI);
  entries.emplace_back(16320, VS_INTERLACEMETHOD_DXVA_AUTO);

  /* remove unsupported methods */
  for (TranslatableIntegerSettingOptions::iterator it = entries.begin(); it != entries.end(); )
  {
    if (appPlayer->Supports(static_cast<EINTERLACEMETHOD>(it->value)))
      ++it;
    else
      it = entries.erase(it);
  }

  if (!entries.empty())
  {
    EINTERLACEMETHOD method = videoSettings.m_InterlaceMethod;
    if (!appPlayer->Supports(method))
    {
      method = appPlayer->GetDeinterlacingMethodDefault();
    }
    AddSpinner(groupVideo, SETTING_VIDEO_INTERLACEMETHOD, 16038, SettingLevel::Basic, static_cast<int>(method), entries);
  }

  entries.clear();
  entries.emplace_back(16301, VS_SCALINGMETHOD_NEAREST);
  entries.emplace_back(16302, VS_SCALINGMETHOD_LINEAR);
  entries.emplace_back(16303, VS_SCALINGMETHOD_CUBIC_B_SPLINE);
  entries.emplace_back(16314, VS_SCALINGMETHOD_CUBIC_MITCHELL);
  entries.emplace_back(16321, VS_SCALINGMETHOD_CUBIC_CATMULL);
  entries.emplace_back(16326, VS_SCALINGMETHOD_CUBIC_0_075);
  entries.emplace_back(16330, VS_SCALINGMETHOD_CUBIC_0_1);
  entries.emplace_back(16304, VS_SCALINGMETHOD_LANCZOS2);
  entries.emplace_back(16323, VS_SCALINGMETHOD_SPLINE36_FAST);
  entries.emplace_back(16315, VS_SCALINGMETHOD_LANCZOS3_FAST);
  entries.emplace_back(16322, VS_SCALINGMETHOD_SPLINE36);
  entries.emplace_back(16305, VS_SCALINGMETHOD_LANCZOS3);
  entries.emplace_back(16306, VS_SCALINGMETHOD_SINC8);
  entries.emplace_back(16307, VS_SCALINGMETHOD_BICUBIC_SOFTWARE);
  entries.emplace_back(16308, VS_SCALINGMETHOD_LANCZOS_SOFTWARE);
  entries.emplace_back(16309, VS_SCALINGMETHOD_SINC_SOFTWARE);
  entries.emplace_back(13120, VS_SCALINGMETHOD_VDPAU_HARDWARE);
  entries.emplace_back(16319, VS_SCALINGMETHOD_DXVA_HARDWARE);
  entries.emplace_back(16316, VS_SCALINGMETHOD_AUTO);

  /* remove unsupported methods */
  for(TranslatableIntegerSettingOptions::iterator it = entries.begin(); it != entries.end(); )
  {
    if (appPlayer->Supports(static_cast<ESCALINGMETHOD>(it->value)))
      ++it;
    else
      it = entries.erase(it);
  }

  AddSpinner(groupVideo, SETTING_VIDEO_SCALINGMETHOD, 16300, SettingLevel::Basic, static_cast<int>(videoSettings.m_ScalingMethod), entries);

  AddVideoStreams(groupVideoStream, SETTING_VIDEO_STREAM);

  if (appPlayer->Supports(RENDERFEATURE_STRETCH) || appPlayer->Supports(RENDERFEATURE_PIXEL_RATIO))
  {
    AddList(groupVideo, SETTING_VIDEO_VIEW_MODE, 629, SettingLevel::Basic, videoSettings.m_ViewMode, CViewModeSettings::ViewModesFiller, 629);
  }
  if (appPlayer->Supports(RENDERFEATURE_ZOOM))
    AddSlider(groupVideo, SETTING_VIDEO_ZOOM, 216, SettingLevel::Basic,
              videoSettings.m_CustomZoomAmount, "{:2.2f}", 0.5f, 0.01f, 2.0f, 216, usePopup);
  if (appPlayer->Supports(RENDERFEATURE_VERTICAL_SHIFT))
    AddSlider(groupVideo, SETTING_VIDEO_VERTICAL_SHIFT, 225, SettingLevel::Basic,
              videoSettings.m_CustomVerticalShift, "{:2.2f}", -2.0f, 0.01f, 2.0f, 225, usePopup);
  if (appPlayer->Supports(RENDERFEATURE_PIXEL_RATIO))
    AddSlider(groupVideo, SETTING_VIDEO_PIXEL_RATIO, 217, SettingLevel::Basic,
              videoSettings.m_CustomPixelRatio, "{:2.2f}", 0.5f, 0.01f, 2.0f, 217, usePopup);

  AddList(groupVideo, SETTING_VIDEO_ORIENTATION, 21843, SettingLevel::Basic, videoSettings.m_Orientation, CGUIDialogVideoSettings::VideoOrientationFiller, 21843);

  if (appPlayer->Supports(RENDERFEATURE_POSTPROCESS))
    AddToggle(groupVideo, SETTING_VIDEO_POSTPROCESS, 16400, SettingLevel::Basic, videoSettings.m_PostProcess);
  if (appPlayer->Supports(RENDERFEATURE_BRIGHTNESS))
    AddPercentageSlider(groupVideo, SETTING_VIDEO_BRIGHTNESS, 464, SettingLevel::Basic, static_cast<int>(videoSettings.m_Brightness), 14047, 1, 464, usePopup);
  if (appPlayer->Supports(RENDERFEATURE_CONTRAST))
    AddPercentageSlider(groupVideo, SETTING_VIDEO_CONTRAST, 465, SettingLevel::Basic, static_cast<int>(videoSettings.m_Contrast), 14047, 1, 465, usePopup);
  if (appPlayer->Supports(RENDERFEATURE_GAMMA))
    AddPercentageSlider(groupVideo, SETTING_VIDEO_GAMMA, 466, SettingLevel::Basic, static_cast<int>(videoSettings.m_Gamma), 14047, 1, 466, usePopup);
  if (appPlayer->Supports(RENDERFEATURE_NOISE))
    AddSlider(groupVideo, SETTING_VIDEO_VDPAU_NOISE, 16312, SettingLevel::Basic,
              videoSettings.m_NoiseReduction, "{:2.2f}", 0.0f, 0.01f, 1.0f, 16312, usePopup);
  if (appPlayer->Supports(RENDERFEATURE_SHARPNESS))
    AddSlider(groupVideo, SETTING_VIDEO_VDPAU_SHARPNESS, 16313, SettingLevel::Basic,
              videoSettings.m_Sharpness, "{:2.2f}", -1.0f, 0.02f, 1.0f, 16313, usePopup);
  if (appPlayer->Supports(RENDERFEATURE_NONLINSTRETCH))
    AddToggle(groupVideo, SETTING_VIDEO_NONLIN_STRETCH, 659, SettingLevel::Basic, videoSettings.m_CustomNonLinStretch);

  // tone mapping
  if (appPlayer->Supports(RENDERFEATURE_TONEMAP))
  {
    const bool visible = !CServiceBroker::GetWinSystem()->IsHDRDisplaySettingEnabled();
    entries.clear();
    entries.emplace_back(36554, VS_TONEMAPMETHOD_OFF);
    entries.emplace_back(36555, VS_TONEMAPMETHOD_REINHARD);
    entries.emplace_back(36557, VS_TONEMAPMETHOD_ACES);
    entries.emplace_back(36558, VS_TONEMAPMETHOD_HABLE);

    AddSpinner(groupVideo, SETTING_VIDEO_TONEMAP_METHOD, 36553, SettingLevel::Basic,
               videoSettings.m_ToneMapMethod, entries, false, visible);
    AddSlider(groupVideo, SETTING_VIDEO_TONEMAP_PARAM, 36556, SettingLevel::Basic,
              videoSettings.m_ToneMapParam, "{:2.2f}", 0.1f, 0.1f, 5.0f, 36556, usePopup, false,
              visible);
  }

  // stereoscopic settings
  entries.clear();
  entries.emplace_back(16316, RENDER_STEREO_MODE_OFF);
  entries.emplace_back(36503, RENDER_STEREO_MODE_SPLIT_HORIZONTAL);
  entries.emplace_back(36504, RENDER_STEREO_MODE_SPLIT_VERTICAL);
  AddSpinner(groupStereoscopic, SETTING_VIDEO_STEREOSCOPICMODE, 36535, SettingLevel::Basic, videoSettings.m_StereoMode, entries);
  AddToggle(groupStereoscopic, SETTING_VIDEO_STEREOSCOPICINVERT, 36536, SettingLevel::Basic, videoSettings.m_StereoInvert);

  // general settings
  AddButton(groupSaveAsDefault, SETTING_VIDEO_MAKE_DEFAULT, 12376, SettingLevel::Basic);
  AddButton(groupSaveAsDefault, SETTING_VIDEO_CALIBRATION, 214, SettingLevel::Basic);
}

void CGUIDialogVideoSettings::AddVideoStreams(const std::shared_ptr<CSettingGroup>& group,
                                              const std::string& settingId)
{
  if (group == NULL || settingId.empty())
    return;

  auto& components = CServiceBroker::GetAppComponents();
  const auto appPlayer = components.GetComponent<CApplicationPlayer>();

  m_videoStream = appPlayer->GetVideoStream();
  if (m_videoStream < 0)
    m_videoStream = 0;

  AddList(group, settingId, 38031, SettingLevel::Basic, m_videoStream, VideoStreamsOptionFiller, 38031);
}

void CGUIDialogVideoSettings::VideoStreamsOptionFiller(
    const std::shared_ptr<const CSetting>& setting,
    std::vector<IntegerSettingOption>& list,
    int& current,
    void* data)
{
  const auto& components = CServiceBroker::GetAppComponents();
  const auto appPlayer = components.GetComponent<CApplicationPlayer>();

  int videoStreamCount = appPlayer->GetVideoStreamCount();
  // cycle through each video stream and add it to our list control
  for (int i = 0; i < videoStreamCount; ++i)
  {
    std::string strItem;
    std::string strLanguage;

    VideoStreamInfo info;
    appPlayer->GetVideoStreamInfo(i, info);

    g_LangCodeExpander.Lookup(info.language, strLanguage);

    if (!info.name.empty())
    {
      if (!strLanguage.empty())
        strItem = StringUtils::Format("{} - {}", strLanguage, info.name);
      else
        strItem = info.name;
    }
    else if (!strLanguage.empty())
    {
        strItem = strLanguage;
    }

    if (info.codecName.empty())
      strItem += StringUtils::Format(" ({}x{}", info.width, info.height);
    else
      strItem += StringUtils::Format(" ({}, {}x{}", info.codecName, info.width, info.height);

    if (info.bitrate)
      strItem += StringUtils::Format(", {} bps)", info.bitrate);
    else
      strItem += ")";

    strItem += FormatFlags(info.flags);
    strItem += StringUtils::Format(" ({}/{})", i + 1, videoStreamCount);
    list.emplace_back(strItem, i);
  }

  if (list.empty())
  {
    list.emplace_back(g_localizeStrings.Get(231), -1);
    current = -1;
  }
}

void CGUIDialogVideoSettings::VideoOrientationFiller(const std::shared_ptr<const CSetting>& setting,
                                                     std::vector<IntegerSettingOption>& list,
                                                     int& current,
                                                     void* data)
{
  list.emplace_back(g_localizeStrings.Get(687), 0);
  list.emplace_back(g_localizeStrings.Get(35229), 90);
  list.emplace_back(g_localizeStrings.Get(35230), 180);
  list.emplace_back(g_localizeStrings.Get(35231), 270);
}

std::string CGUIDialogVideoSettings::FormatFlags(StreamFlags flags)
{
  std::vector<std::string> localizedFlags;
  if (flags & StreamFlags::FLAG_DEFAULT)
    localizedFlags.emplace_back(g_localizeStrings.Get(39105));
  if (flags & StreamFlags::FLAG_FORCED)
    localizedFlags.emplace_back(g_localizeStrings.Get(39106));
  if (flags & StreamFlags::FLAG_HEARING_IMPAIRED)
    localizedFlags.emplace_back(g_localizeStrings.Get(39107));
  if (flags &  StreamFlags::FLAG_VISUAL_IMPAIRED)
    localizedFlags.emplace_back(g_localizeStrings.Get(39108));

  std::string formated = StringUtils::Join(localizedFlags, ", ");

  if (!formated.empty())
    formated = StringUtils::Format(" [{}]", formated);

  return formated;
}
