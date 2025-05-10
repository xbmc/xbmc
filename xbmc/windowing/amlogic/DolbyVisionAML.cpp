/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DolbyVisionAML.h"

#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>

#include "ServiceBroker.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "settings/lib/SettingsManager.h"
#include "settings/lib/Setting.h"
#include "guilib/LocalizeStrings.h"
#include "utils/AMLUtils.h"
#include "utils/log.h"
#include "interfaces/AnnouncementManager.h"

using namespace KODI;

static std::shared_ptr<CSettings> settings()
{
  return CServiceBroker::GetSettingsComponent()->GetSettings();
}

static void set_visible(const std::string& id, bool visible) {
  if (auto setting = settings()->GetSetting(id)) setting->SetVisible(visible);
}

// Dolby VSVDB Color space data
static double colour_space_data[3][6] = {
    // Rx--[5/8]-------  Ry--[1/4]------  Gx--[1]-  Gy--[1/2]-----  Bx--[1/8]-------  By--[1/32]--------
      {(0.6800 - 0.625), (0.3200 - 0.25), (0.2650), (0.6900 - 0.5), (0.1500 - 0.125), (0.0600 - 0.03125)}, // DCI-P3  - https://en.wikipedia.org/wiki/DCI-P3
      {(0.7080 - 0.625), (0.2920 - 0.25), (0.1700), (0.7970 - 0.5), (0.1310 - 0.125), (0.0460 - 0.03125)}, // BT.2020 - https://en.wikipedia.org/wiki/Rec._2020
      {(0.6400 - 0.625), (0.3300 - 0.25), (0.3000), (0.6000 - 0.5), (0.1500 - 0.125), (0.0600 - 0.03125)}  // BT.709  - https://en.wikipedia.org/wiki/Rec._709
};

static void CalculateVSVDBPayload()
{
  bool inject(settings()->GetBool(CSettings::SETTING_COREELEC_AMLOGIC_DV_VSVDB_INJECT));
  if (!inject) return;

  enum DV_TYPE dv_type(static_cast<DV_TYPE>(settings()->GetInt(CSettings::SETTING_COREELEC_AMLOGIC_DV_TYPE)));
  int min_lum(settings()->GetInt(CSettings::SETTING_COREELEC_AMLOGIC_DV_VSVDB_MIN_LUM));
  int max_lum(settings()->GetInt(CSettings::SETTING_COREELEC_AMLOGIC_DV_VSVDB_MAX_LUM));
  int cs(settings()->GetInt(CSettings::SETTING_COREELEC_AMLOGIC_DV_VSVDB_CS));

  int dv_type_bits = (dv_type == DV_TYPE_DISPLAY_LED) ? 2 : 0;

  const double one_256 = 0.00390625;

  unsigned char byte[7];

  byte[0] = (2 << 5) |       // Version (2 [010]) in 7-5
            (2 << 2) |       // DM Version (2 [010]) in 4-2
            (0 << 1) |       // Backlight Control (Not Supported [0]) in 1
            (1 << 0);        // YUV 12 Bit (Supported [1]) in 0

  byte[1] = (min_lum << 3) | // Minimum Luminance (PQ) in 7-3
            (0 << 2) |       // Global Dimming (Unsupported [0]) in 2
            (3 << 0);        // Backlight Min Lum (Disabled 3 [11]) in 1-0

  byte[2] = (max_lum << 3) |     // Maximum Luminance (PQ) in 7-3
            (0 << 2) |           // Reserved ([0]]) in 2
            (dv_type_bits << 0); // DV type in 1-0

  byte[3] = (static_cast<int>(colour_space_data[cs][2] / one_256) << 1) | // Gx in 7-1
            (0 << 0);                                                     // 12b 444 (Unsupported [0]) in 0

  byte[4] = (static_cast<int>(colour_space_data[cs][3] / one_256) << 1) | // Gy in 7-1
            (0 << 0);                                                     // 10b 444 (Unsupported [0]) in 0

  byte[5] = (static_cast<int>(colour_space_data[cs][0] / one_256) << 3) | // Rx in 7-3
            (static_cast<int>(colour_space_data[cs][4] / one_256) << 0);  // Bx in 2-0

  byte[6] = (static_cast<int>(colour_space_data[cs][1] / one_256) << 3) | // Ry in 7-3
            (static_cast<int>(colour_space_data[cs][5] / one_256) << 0);  // By in 2-0

  std::stringstream ss;
  for (size_t i = 0; i < 7; i++) {
    ss << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << static_cast<int>(byte[i]);
  }

  settings()->SetString(CSettings::SETTING_COREELEC_AMLOGIC_DV_VSVDB_PAYLOAD, ss.str());
}

static bool support_dv() {
  enum DV_TYPE dv_type(static_cast<DV_TYPE>(settings()->GetInt(CSettings::SETTING_COREELEC_AMLOGIC_DV_TYPE)));
  return ((aml_display_support_dv_std() || aml_display_support_dv_ll() || aml_display_support_hdr_pq()) && (dv_type != DV_TYPE_VS10_ONLY));
}

void dv_type_filler(const SettingConstPtr& setting, std::vector<IntegerSettingOption>& list, int& current, void* data) {
  list.clear();
  if (aml_display_support_dv_std()) list.emplace_back(g_localizeStrings.Get(50023), DV_TYPE_DISPLAY_LED);
  if (aml_display_support_dv_ll()) list.emplace_back(g_localizeStrings.Get(50024), DV_TYPE_PLAYER_LED_LLDV);
  if (aml_display_support_hdr_pq()) list.emplace_back(g_localizeStrings.Get(50025), DV_TYPE_PLAYER_LED_HDR);
  list.emplace_back(g_localizeStrings.Get(50026), DV_TYPE_VS10_ONLY);
}

void vsvdb_min_filler(const SettingConstPtr& setting, std::vector<IntegerSettingOption>& list, int& current, void* data) {
  list.clear();
  list.emplace_back("PQ 0 (0.00000000 cd/m^2)", 0);
  list.emplace_back("PQ 20 (0.00064354 cd/m^2)", 1);
  list.emplace_back("PQ 40 (0.00223738 cd/m^2)", 2);
  list.emplace_back("PQ 60 (0.00478965 cd/m^2)", 3);
  list.emplace_back("PQ 80 (0.00837904 cd/m^2)", 4);
  list.emplace_back("PQ 100 (0.01310152 cd/m^2)", 5);
  list.emplace_back("PQ 120 (0.01906315 cd/m^2)", 6);
  list.emplace_back("PQ 140 (0.02637791 cd/m^2)", 7);
  list.emplace_back("PQ 160 (0.03516709 cd/m^2)", 8);
  list.emplace_back("PQ 180 (0.04555910 cd/m^2)", 9);
  list.emplace_back("PQ 200 (0.05768953 cd/m^2)", 10);
  list.emplace_back("PQ 220 (0.07170139 cd/m^2)", 11);
  list.emplace_back("PQ 240 (0.08774531 cd/m^2)", 12);
  list.emplace_back("PQ 260 (0.10597988 cd/m^2)", 13);
  list.emplace_back("PQ 280 (0.12657199 cd/m^2)", 14);
  list.emplace_back("PQ 300 (0.14969718 cd/m^2)", 15);
  list.emplace_back("PQ 320 (0.17554001 cd/m^2)", 16);
  list.emplace_back("PQ 340 (0.20429448 cd/m^2)", 17);
  list.emplace_back("PQ 360 (0.23616447 cd/m^2)", 18);
  list.emplace_back("PQ 380 (0.27136414 cd/m^2)", 19);
  list.emplace_back("PQ 400 (0.31011844 cd/m^2)", 20);
  list.emplace_back("PQ 420 (0.35266356 cd/m^2)", 21);
  list.emplace_back("PQ 440 (0.39924746 cd/m^2)", 22);
  list.emplace_back("PQ 460 (0.45013035 cd/m^2)", 23);
  list.emplace_back("PQ 480 (0.50558532 cd/m^2)", 24);
  list.emplace_back("PQ 500 (0.56589883 cd/m^2)", 25);
  list.emplace_back("PQ 520 (0.63137136 cd/m^2)", 26);
  list.emplace_back("PQ 540 (0.70231800 cd/m^2)", 27);
  list.emplace_back("PQ 560 (0.77906912 cd/m^2)", 28);
  list.emplace_back("PQ 580 (0.86197104 cd/m^2)", 29);
  list.emplace_back("PQ 600 (0.95138673 cd/m^2)", 30);
  list.emplace_back("PQ 620 (1.04769654 cd/m^2)", 31);
}

void vsvdb_max_filler(const SettingConstPtr& setting, std::vector<IntegerSettingOption>& list, int& current, void* data) {
  list.clear();
  list.emplace_back("PQ 2055 (96 cd/m^2)", 0);
  list.emplace_back("PQ 2120 (113 cd/m^2)", 1);
  list.emplace_back("PQ 2185 (132 cd/m^2)", 2);
  list.emplace_back("PQ 2250 (155 cd/m^2)", 3);
  list.emplace_back("PQ 2315 (181 cd/m^2)", 4);
  list.emplace_back("PQ 2380 (211 cd/m^2)", 5);
  list.emplace_back("PQ 2445 (245 cd/m^2)", 6);
  list.emplace_back("PQ 2510 (285 cd/m^2)", 7);
  list.emplace_back("PQ 2575 (332 cd/m^2)", 8);
  list.emplace_back("PQ 2640 (385 cd/m^2)", 9);
  list.emplace_back("PQ 2705 (447 cd/m^2)", 10);
  list.emplace_back("PQ 2770 (518 cd/m^2)", 11);
  list.emplace_back("PQ 2835 (601 cd/m^2)", 12);
  list.emplace_back("PQ 2900 (696 cd/m^2)", 13);
  list.emplace_back("PQ 2965 (807 cd/m^2)", 14);
  list.emplace_back("PQ 3030 (934 cd/m^2)", 15);
  list.emplace_back("PQ 3095 (1082 cd/m^2)", 16);
  list.emplace_back("PQ 3160 (1252 cd/m^2)", 17);
  list.emplace_back("PQ 3225 (1450 cd/m^2)", 18);
  list.emplace_back("PQ 3290 (1678 cd/m^2)", 19);
  list.emplace_back("PQ 3355 (1943 cd/m^2)", 20);
  list.emplace_back("PQ 3420 (2250 cd/m^2)", 21);
  list.emplace_back("PQ 3485 (2607 cd/m^2)", 22);
  list.emplace_back("PQ 3550 (3020 cd/m^2)", 23);
  list.emplace_back("PQ 3615 (3501 cd/m^2)", 24);
  list.emplace_back("PQ 3680 (4060 cd/m^2)", 25);
  list.emplace_back("PQ 3745 (4710 cd/m^2)", 26);
  list.emplace_back("PQ 3810 (5467 cd/m^2)", 27);
  list.emplace_back("PQ 3875 (6351 cd/m^2)", 38);
  list.emplace_back("PQ 3940 (7382 cd/m^2)", 29);
  list.emplace_back("PQ 4005 (8588 cd/m^2)", 30);
  list.emplace_back("PQ 4070 (10000 cd/m^2)", 31);
}

void add_vs10_bypass(std::vector<IntegerSettingOption>& list) {list.emplace_back(g_localizeStrings.Get(50063), DOLBY_VISION_OUTPUT_MODE_BYPASS);}
void add_vs10_dv_bypass(std::vector<IntegerSettingOption>& list) {list.emplace_back(g_localizeStrings.Get(50063), DOLBY_VISION_OUTPUT_MODE_IPT);}
void add_vs10_sdr(std::vector<IntegerSettingOption>& list) {list.emplace_back(g_localizeStrings.Get(50064), DOLBY_VISION_OUTPUT_MODE_SDR10);}
void add_vs10_hdr10(std::vector<IntegerSettingOption>& list) {list.emplace_back(g_localizeStrings.Get(50065), DOLBY_VISION_OUTPUT_MODE_HDR10);}
void add_vs10_dv(std::vector<IntegerSettingOption>& list) {list.emplace_back(g_localizeStrings.Get(50066), DOLBY_VISION_OUTPUT_MODE_IPT);}

void vs10_sdr_filler(const SettingConstPtr& setting, std::vector<IntegerSettingOption>& list, int& current, void* data)
{
  list.clear();
  add_vs10_bypass(list);
  add_vs10_sdr(list);
  if (aml_display_support_hdr_pq()) add_vs10_hdr10(list);
  if (support_dv()) add_vs10_dv(list);
}

void vs10_hdr10_filler(const SettingConstPtr& setting, std::vector<IntegerSettingOption>& list, int& current, void* data)
{
  list.clear();
  if (aml_display_support_hdr_pq()) add_vs10_bypass(list);
  add_vs10_sdr(list);
  if (support_dv()) add_vs10_dv(list);
}

void vs10_hdr_hlg_filler(const SettingConstPtr& setting, std::vector<IntegerSettingOption>& list, int& current, void* data)
{
  list.clear();
  if (aml_display_support_hdr_hlg()) add_vs10_bypass(list);
  add_vs10_sdr(list);
  if (aml_display_support_hdr_pq()) add_vs10_hdr10(list);
  if (support_dv()) add_vs10_dv(list);
}

void vs10_dv_filler(const SettingConstPtr& setting, std::vector<IntegerSettingOption>& list, int& current, void* data)
{
  list.clear();
  if (support_dv()) add_vs10_dv_bypass(list);
  add_vs10_sdr(list);
}

CDolbyVisionAML::CDolbyVisionAML()
{
}

bool CDolbyVisionAML::Setup()
{
  CLog::Log(LOGDEBUG, "CDolbyVisionAML::Setup - Begin");

  if (!aml_support_dolby_vision())
  {
    set_visible(CSettings::SETTING_COREELEC_AMLOGIC_DV_MODE, false);
    settings()->SetInt(CSettings::SETTING_COREELEC_AMLOGIC_DV_MODE, DV_MODE_OFF);
    CLog::Log(LOGDEBUG, "CDolbyVisionAML::Setup - Device does not support Dolby Vision - exiting setup");
    return false;
  }

  const auto settingsManager = settings()->GetSettingsManager();

  settingsManager->RegisterSettingOptionsFiller("DolbyVisionType", dv_type_filler);
  settingsManager->RegisterSettingOptionsFiller("DolbyVisionVSVDBMinLum", vsvdb_min_filler);
  settingsManager->RegisterSettingOptionsFiller("DolbyVisionVSVDBMaxLum", vsvdb_max_filler);
  settingsManager->RegisterSettingOptionsFiller("DolbyVisionVS10SDR8", vs10_sdr_filler);
  settingsManager->RegisterSettingOptionsFiller("DolbyVisionVS10SDR10", vs10_sdr_filler);
  settingsManager->RegisterSettingOptionsFiller("DolbyVisionVS10HDR10", vs10_hdr10_filler);
  settingsManager->RegisterSettingOptionsFiller("DolbyVisionVS10HDR10Plus", vs10_hdr10_filler);
  settingsManager->RegisterSettingOptionsFiller("DolbyVisionVS10HDRHLG", vs10_hdr_hlg_filler);
  settingsManager->RegisterSettingOptionsFiller("DolbyVisionVS10DV", vs10_dv_filler);

  set_visible(CSettings::SETTING_COREELEC_AMLOGIC_DV_MODE_ON_LUMINANCE, true);
  set_visible(CSettings::SETTING_COREELEC_AMLOGIC_DV_TYPE, true);
  set_visible(CSettings::SETTING_COREELEC_AMLOGIC_DV_VSVDB_INJECT, true);
  set_visible(CSettings::SETTING_COREELEC_AMLOGIC_DV_VSVDB_PAYLOAD, true);
  set_visible(CSettings::SETTING_COREELEC_AMLOGIC_DV_VSVDB_CS, true);
  set_visible(CSettings::SETTING_COREELEC_AMLOGIC_DV_VSVDB_MIN_LUM, true);
  set_visible(CSettings::SETTING_COREELEC_AMLOGIC_DV_VSVDB_MAX_LUM, true);
  set_visible(CSettings::SETTING_COREELEC_AMLOGIC_DV_HDR_INJECT, true);
  set_visible(CSettings::SETTING_COREELEC_AMLOGIC_DV_HDR_PAYLOAD, true);
  set_visible(CSettings::SETTING_COREELEC_AMLOGIC_DV_COLORIMETRY_FOR_STD, true);
  set_visible(CSettings::SETTING_COREELEC_AMLOGIC_DV_STD_SOURCE_LEVELS_METADATA, true);
  set_visible(CSettings::SETTING_COREELEC_AMLOGIC_DV_STD_SOURCE_LEVEL_5, true);
  set_visible(CSettings::SETTING_COREELEC_AMLOGIC_DV_STD_SOURCE_LEVEL_6, true);
  set_visible(CSettings::SETTING_COREELEC_AMLOGIC_DV_STD_RESTRICT_OVERLAYS, true);
  set_visible(CSettings::SETTING_COREELEC_AMLOGIC_DV_LL_VSVDB_LIMIT, true);
  set_visible(CSettings::SETTING_COREELEC_AMLOGIC_DV_LL_VSVDB_LIMIT_BRIGHTNESS, true);
  set_visible(CSettings::SETTING_COREELEC_AMLOGIC_DV_VS10_SDR8, true);
  set_visible(CSettings::SETTING_COREELEC_AMLOGIC_DV_VS10_SDR10, true);
  set_visible(CSettings::SETTING_COREELEC_AMLOGIC_DV_VS10_HDR10, true);
  set_visible(CSettings::SETTING_COREELEC_AMLOGIC_DV_VS10_HDR10PLUS, true);
  set_visible(CSettings::SETTING_COREELEC_AMLOGIC_DV_VS10_HDRHLG, true);
  set_visible(CSettings::SETTING_COREELEC_AMLOGIC_DV_VS10_DV, true);
  set_visible(CSettings::SETTING_COREELEC_AMLOGIC_DV_DUAL_PRIORITY, true);
  set_visible(CSettings::SETTING_COREELEC_AMLOGIC_DV_HDR10PLUS_CONVERT, true);
  set_visible(CSettings::SETTING_COREELEC_AMLOGIC_DV_HDR10PLUS_PREFER_CONVERT, true);
  set_visible(CSettings::SETTING_COREELEC_AMLOGIC_DV_HDR10PLUS_PEAK_BRIGHTNESS_SOURCE, true);
  set_visible(CSettings::SETTING_VIDEOPLAYER_CONVERTDOVI, true);

  // Register for ui dv mode change - to change on the fly.
  std::set<std::string> settingSet;
  settingSet.insert(CSettings::SETTING_COREELEC_AMLOGIC_DV_MODE);
  settingSet.insert(CSettings::SETTING_COREELEC_AMLOGIC_DV_MODE_ON_LUMINANCE);
  settingSet.insert(CSettings::SETTING_COREELEC_AMLOGIC_DV_TYPE);
  settingSet.insert(CSettings::SETTING_COREELEC_AMLOGIC_DV_VSVDB_CS);
  settingSet.insert(CSettings::SETTING_COREELEC_AMLOGIC_DV_VSVDB_MIN_LUM);
  settingSet.insert(CSettings::SETTING_COREELEC_AMLOGIC_DV_VSVDB_MAX_LUM);
  settingsManager->RegisterCallback(this, settingSet);

  // register for announcements to capture OnWake and re-apply DV if needed.
  auto announcer = CServiceBroker::GetAnnouncementManager();
  announcer->AddAnnouncer(this);

  // Turn on dv - if dv mode is on, limit the menu luminance as menu now can be in DV/HDR.
  aml_dv_start();

  CLog::Log(LOGDEBUG, "CDolbyVisionAML::Setup - Complete");

  return true;
}

void CDolbyVisionAML::OnSettingChanged(const std::shared_ptr<const CSetting>& setting)
{
  if (!setting) return;

  const std::string& settingId = setting->GetId();
  if (settingId == CSettings::SETTING_COREELEC_AMLOGIC_DV_MODE)
  {
    // Not working for some cases - needs video playback for mode switch to work correctly everytime.
    // enum DV_MODE dv_mode(static_cast<DV_MODE>(std::dynamic_pointer_cast<const CSettingInt>(setting)->GetValue()));
    // if (dv_mode == DV_MODE_ON) ? aml_dv_on(DOLBY_VISION_OUTPUT_MODE_IPT) : aml_dv_off();
  }
  else if (settingId == CSettings::SETTING_COREELEC_AMLOGIC_DV_MODE_ON_LUMINANCE)
  {
    int max(std::dynamic_pointer_cast<const CSettingInt>(setting)->GetValue());
    aml_dv_set_osd_max(max);
  }
  else if (settingId == CSettings::SETTING_COREELEC_AMLOGIC_DV_TYPE)
  {
    // Not working for some cases - needs video playback for mode switch to work correctly everytime.
    // aml_dv_start();
  }
  else if (settingId == CSettings::SETTING_COREELEC_AMLOGIC_DV_VSVDB_CS)
  {
    CalculateVSVDBPayload();
  }
  else if (settingId == CSettings::SETTING_COREELEC_AMLOGIC_DV_VSVDB_MIN_LUM)
  {
    CalculateVSVDBPayload();
  }
  else if (settingId == CSettings::SETTING_COREELEC_AMLOGIC_DV_VSVDB_MAX_LUM)
  {
    CalculateVSVDBPayload();
  }
}

void CDolbyVisionAML::Announce(ANNOUNCEMENT::AnnouncementFlag flag,
              const std::string& sender,
              const std::string& message,
              const CVariant& data)
{
  // When Wake from Suspend re-trigger DV if in DV_MODE_ON
  if ((flag == ANNOUNCEMENT::System) && (message == "OnWake")) aml_dv_start();
}
