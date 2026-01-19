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

#include "cores/DataCacheCore.h"

#include "settings/DisplaySettings.h"

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

static inline int find_closest_lut_index(int value, const int *lut, int lut_size)
{
  int low = 0, high = (lut_size - 1);
  if (value <= lut[low]) return low;
  if (value >= lut[high]) return high;
  while ((high - low) > 1) {
    int mid = (low + high) / 2;
    if (lut[mid] == value) return mid;
    (lut[mid] < value) ? (low = mid) : (high = mid);
  }
  return ((value - lut[low]) < (lut[high] - value)) ? low : high;
}

static const int max_direct_to_pq_lut[128] = {
  2081, 2249, 2372, 2467, 2547, 2614, 2672, 2724, 2771, 2813, 2851, 2887, 2920, 2950, 2979, 3006,
  3032, 3056, 3079, 3101, 3121, 3141, 3160, 3178, 3196, 3213, 3229, 3245, 3260, 3275, 3289, 3302,
  3316, 3329, 3341, 3354, 3365, 3377, 3388, 3399, 3410, 3421, 3431, 3441, 3451, 3460, 3470, 3479,
  3488, 3497, 3505, 3514, 3522, 3530, 3538, 3546, 3554, 3561, 3569, 3576, 3583, 3590, 3597, 3604,
  3611, 3618, 3624, 3631, 3637, 3643, 3649, 3656, 3662, 3668, 3673, 3679, 3685, 3690, 3696, 3702,
  3707, 3712, 3718, 3723, 3728, 3733, 3738, 3743, 3748, 3753, 3758, 3762, 3767, 3772, 3776, 3781,
  3785, 3790, 3794, 3799, 3803, 3807, 3811, 3816, 3820, 3824, 3828, 3832, 3836, 3840, 3844, 3848,
  3852, 3855, 3859, 3863, 3867, 3870, 3874, 3878, 3881, 3885, 3888, 3892, 3895, 3899, 3902, 3906
};

static const int max_direct_to_nits_lut[128] = {
   100,  150,  200,  250,  300,  350,  400,  450,  500,  550,  600,  650,  700,  750,  800,  850,
   900,  950, 1000, 1050, 1100, 1150, 1200, 1250, 1300, 1350, 1400, 1450, 1500, 1550, 1600, 1650,
  1700, 1750, 1800, 1850, 1900, 1950, 2000, 2050, 2100, 2150, 2200, 2250, 2300, 2350, 2400, 2450,
  2500, 2550, 2600, 2650, 2700, 2750, 2800, 2850, 2900, 2950, 3000, 3050, 3100, 3150, 3200, 3250,
  3300, 3350, 3400, 3450, 3500, 3550, 3600, 3650, 3700, 3750, 3800, 3850, 3900, 3950, 4000, 4050,
  4100, 4150, 4200, 4250, 4300, 4350, 4400, 4450, 4500, 4550, 4600, 4650, 4700, 4750, 4800, 4850,
  4900, 4950, 5000, 5050, 5100, 5150, 5200, 5250, 5300, 5350, 5400, 5450, 5500, 5550, 5600, 5650,
  5700, 5750, 5800, 5850, 5900, 5950, 6000, 6050, 6100, 6150, 6200, 6250, 6300, 6350, 6400, 6450
};

// spreadsheet pq
static const int max_direct_to_pq_lut_2[32] = {
  2055, 2120, 2185, 2250, 2315, 2380, 2445, 2510, 2575, 2640, 2705, 2770, 2835, 2900, 2965, 3030,
  3095, 3160, 3225, 3290, 3355, 3420, 3485, 3550, 3615, 3680, 3745, 3810, 3875, 3940, 4005, 4070
};

// adjusted nits based on pq of spreadsheet
static const int max_direct_to_nits_lut_2_adj[32] = {
    94,  110,  129,  150,  175,  204,  237,  276,  320,  372,  431,  499,  578,  670,  775,  897,
  1037, 1200, 1387, 1605, 1856, 2147, 2485, 2876, 3330, 3857, 4470, 5183, 6014, 6982, 8113, 9434
};

// adjusted pq based on nits of spreadsheet
// static const int max_direct_to_pq_lut_2_adj[32] = {
//  2068, 2133, 2198, 2264, 2329, 2395, 2460, 2525, 2591, 2656, 2722, 2787, 2852, 2918, 2983, 3049,
//  3114, 3179, 3245, 3310, 3376, 3441, 3506, 3572, 3637, 3703, 3768, 3833, 3899, 3964, 4029, 4095
//};

// adjusted pq based on nits of spreadsheet and fine tuned for certain values 
// static const int max_direct_to_pq_lut_2_adj_ft[32] = {
//  2068, 2133, 2198, 2264, 2329, 2395, 2460, 2525, 2580, 2656, 2722, 2787, 2852, 2918, 2983, 3049,
//  3101, 3179, 3245, 3310, 3376, 3399, 3506, 3572, 3637, 3702, 3768, 3833, 3899, 3964, 4029, 4095
//};

// spreadsheet nits
static const int max_direct_to_nits_lut_2[32] = {
    96,  113,  132,  155,  181,  211,  245,  285,  332,  385,  447,  518,  601,  696,  807,  934,
  1082, 1252, 1450, 1678, 1943, 2250, 2607, 3020, 3501, 4060, 4710, 5467, 6351, 7382, 8588, 10000
};

void CalculateVSVDBPayload()
{
  int max_lum_nits_value(settings()->GetInt(CSettings::SETTING_COREELEC_AMLOGIC_DV_VSVDB_MAX_LUM));
  int max_lum_idx = find_closest_lut_index(max_lum_nits_value, max_direct_to_nits_lut, 128);
  int cs(settings()->GetInt(CSettings::SETTING_COREELEC_AMLOGIC_DV_VSVDB_CS));

  int max_lum_pq_value = 0;
 
  bool dv_dolby_vsvdb_inject(settings()->GetBool(CSettings::SETTING_COREELEC_AMLOGIC_DV_VSVDB_INJECT));
  if (dv_dolby_vsvdb_inject)
  {
    if ((cs == 3) && aml_display_support_dv())
    {
      aml_get_dv_cap();
      switch (xbmc_dv_cap::dv_gx_i)
	    {
        case 43:
          cs = 1;
          break;
        case 67:
          cs = 0;
          break;
        case 76:
          cs = 2;
          break;
        default:
          cs = 3;
          break;
      }
      settings()->SetInt(CSettings::SETTING_COREELEC_AMLOGIC_DV_VSVDB_CS, cs);
    }
    else if ((cs == 3) && !aml_display_support_dv())
    {
      cs = 1;
      settings()->SetInt(CSettings::SETTING_COREELEC_AMLOGIC_DV_VSVDB_CS, cs);
    }
  }
  else if (!dv_dolby_vsvdb_inject)
  {
    if (aml_display_support_dv())
    {
      aml_get_dv_cap();
      switch (xbmc_dv_cap::dv_gx_i)
  	  {
        case 43:
          cs = 1;
          break;
        case 67:
          cs = 0;
          break;
        case 76:
          cs = 2;
          break;
        default:
          cs = 3;
          break;
      }
      settings()->SetInt(CSettings::SETTING_COREELEC_AMLOGIC_DV_VSVDB_CS, cs);
      switch (xbmc_dv_cap::dv_ver_i)
      {
        case 1:
        {
          max_lum_idx = xbmc_dv_cap::dv_max_v1_i;
          max_lum_nits_value = max_direct_to_nits_lut[max_lum_idx];
          settings()->SetInt(CSettings::SETTING_COREELEC_AMLOGIC_DV_VSVDB_MAX_LUM, max_lum_nits_value);
          break;
        }
        case 2:
        {
          max_lum_nits_value = max_direct_to_nits_lut_2[xbmc_dv_cap::dv_max_v2_i];
          settings()->SetInt(CSettings::SETTING_COREELEC_AMLOGIC_DV_VSVDB_MAX_LUM, max_lum_nits_value);
          max_lum_pq_value = max_direct_to_pq_lut_2[xbmc_dv_cap::dv_max_v2_i];
          max_lum_idx = find_closest_lut_index(max_lum_pq_value, max_direct_to_pq_lut, 128);
          if ((max_lum_idx > 17) && (max_lum_idx < 127)) max_lum_idx = max_lum_idx + 1;
          break;
        }
        default:
          break;
      }
    }
    else
    {
      if (cs == 3)
      {
        cs = 1;
        settings()->SetInt(CSettings::SETTING_COREELEC_AMLOGIC_DV_VSVDB_CS, cs);
      }
    }
  }

  if ((cs == 3) && ((xbmc_dv_cap::dv_rx_i == 0) || (xbmc_dv_cap::dv_ry_i == 0) || (xbmc_dv_cap::dv_gx_i == 0) ||
      (xbmc_dv_cap::dv_gy_i == 0) || (xbmc_dv_cap::dv_bx_i == 0) || (xbmc_dv_cap::dv_by_i == 0)))
  {
    cs = 1;
    settings()->SetInt(CSettings::SETTING_COREELEC_AMLOGIC_DV_VSVDB_CS, cs);
  }

  unsigned char byte[7];
  
  byte[0] = (1 << 5) |
            (1 << 2) |
            (1 << 1) |
            (1 << 0);

  byte[1] = (max_lum_idx << 1) |
            (0 << 0);

  byte[2] = (0 << 1) |
            (1 << 0);

  const double one_256 = 0.00390625;

  if (cs == 3)
  {
      byte[3] = (xbmc_dv_cap::dv_bx_i << 5) |
                ((xbmc_dv_cap::dv_by_i << 2) & 0x1C) |
                (1 << 0);

      byte[4] = (xbmc_dv_cap::dv_gx_i << 1) |
                (xbmc_dv_cap::dv_ry_i & 0x01);

      byte[5] = (xbmc_dv_cap::dv_gy_i << 1) |
                ((xbmc_dv_cap::dv_ry_i >> 1) & 0x01);

      byte[6] = (xbmc_dv_cap::dv_rx_i << 3) |
                ((xbmc_dv_cap::dv_ry_i >> 2 ) & 0x07);
  }
  else
  {
      byte[3] = (static_cast<int>(colour_space_data[cs][4] / one_256) << 5) |
                ((static_cast<int>(colour_space_data[cs][5] / one_256) << 2) & 0x1C) |
                (1 << 0);

      byte[4] = (static_cast<int>(colour_space_data[cs][2] / one_256) << 1) |
                (static_cast<int>(colour_space_data[cs][1] / one_256) & 0x01);

      byte[5] = (static_cast<int>(colour_space_data[cs][3] / one_256) << 1) |
                ((static_cast<int>(colour_space_data[cs][1] / one_256) >> 1) & 0x01);

      byte[6] = (static_cast<int>(colour_space_data[cs][0] / one_256) << 3) |
                ((static_cast<int>(colour_space_data[cs][1] / one_256) >> 2 ) & 0x07);
  }
  std::stringstream ss;
  for (size_t i = 0; i < 7; i++) {
    ss << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << static_cast<int>(byte[i]);
  }
  settings()->SetString(CSettings::SETTING_COREELEC_AMLOGIC_DV_VSVDB_PAYLOAD, ss.str());
}

void CalculateVSVDBPayload_2()
{
  int max_lum_nits_value(settings()->GetInt(CSettings::SETTING_COREELEC_AMLOGIC_DV_VSVDB_MAX_LUM));
  int max_lum_idx = find_closest_lut_index(max_lum_nits_value, max_direct_to_nits_lut_2_adj, 32);
  int cs(settings()->GetInt(CSettings::SETTING_COREELEC_AMLOGIC_DV_VSVDB_CS));

  bool dv_dolby_vsvdb_inject(settings()->GetBool(CSettings::SETTING_COREELEC_AMLOGIC_DV_VSVDB_INJECT));
  if (dv_dolby_vsvdb_inject)
  {
    if ((cs == 3) && aml_display_support_dv())
    {
      aml_get_dv_cap();
      switch (xbmc_dv_cap::dv_gx_i)
      {
        case 43:
          cs = 1;
          break;
        case 67:
          cs = 0;
          break;
        case 76:
          cs = 2;
          break;
        default:
          cs = 3;
          break;
      }
      settings()->SetInt(CSettings::SETTING_COREELEC_AMLOGIC_DV_VSVDB_CS, cs);
    }
    else if ((cs == 3) && !aml_display_support_dv())
    {
      cs = 1;
      settings()->SetInt(CSettings::SETTING_COREELEC_AMLOGIC_DV_VSVDB_CS, cs);
    }
  }
  else if (!dv_dolby_vsvdb_inject)
  {
    if (aml_display_support_dv())
    {
      aml_get_dv_cap();
      switch (xbmc_dv_cap::dv_gx_i)
	    {
        case 43:
          cs = 1;
          break;
        case 67:
          cs = 0;
          break;
        case 76:
          cs = 2;
          break;
        default:
          cs = 3;
          break;
      }
      settings()->SetInt(CSettings::SETTING_COREELEC_AMLOGIC_DV_VSVDB_CS, cs);
      switch (xbmc_dv_cap::dv_ver_i)
      {
        case 1:
        {
          max_lum_nits_value = max_direct_to_nits_lut[xbmc_dv_cap::dv_max_v1_i];
          settings()->SetInt(CSettings::SETTING_COREELEC_AMLOGIC_DV_VSVDB_MAX_LUM, max_lum_nits_value);
          max_lum_idx = find_closest_lut_index(max_lum_nits_value, max_direct_to_nits_lut_2_adj, 32);
          break;
        }
       case 2:
       {
          max_lum_idx = xbmc_dv_cap::dv_max_v2_i;
          max_lum_nits_value = max_direct_to_nits_lut_2[max_lum_idx];
          settings()->SetInt(CSettings::SETTING_COREELEC_AMLOGIC_DV_VSVDB_MAX_LUM, max_lum_nits_value);
          break;
       }
       default:
          break;
      }
    }
    else
    {
      if (cs == 3)
      {
        cs = 1;
        settings()->SetInt(CSettings::SETTING_COREELEC_AMLOGIC_DV_VSVDB_CS, cs);
      }
    }
  }

  if ((cs == 3) && ((xbmc_dv_cap::dv_rx_i == 0) || (xbmc_dv_cap::dv_ry_i == 0) || (xbmc_dv_cap::dv_gx_i == 0) ||
      (xbmc_dv_cap::dv_gy_i == 0) || (xbmc_dv_cap::dv_bx_i == 0) || (xbmc_dv_cap::dv_by_i == 0)))
  {
    cs = 1;
    settings()->SetInt(CSettings::SETTING_COREELEC_AMLOGIC_DV_VSVDB_CS, cs);
  }

  unsigned char byte[7];
  
  byte[0] = (2 << 5) |       // Version (2 [010]) in 7-5
            (2 << 2) |       // DM Version (2 [010]) in 4-2
            (0 << 1) |       // Backlight Control (Not Supported [0]) in 1
            (1 << 0);        // YUV 12 Bit (Supported [1]) in 0

  byte[1] = (0 << 3) |       // Minimum Luminance (PQ) in 7-3
            (0 << 2) |       // Global Dimming (Unsupported [0]) in 2
            (3 << 0);        // Backlight Min Lum (Disabled 3 [11]) in 1-0

  enum DV_TYPE dv_type(static_cast<DV_TYPE>(settings()->GetInt(CSettings::SETTING_COREELEC_AMLOGIC_DV_TYPE)));
  int dv_type_bits = (dv_type == DV_TYPE_DISPLAY_LED) ? 2 : 1;

  byte[2] = (max_lum_idx << 3) | // Maximum Luminance (PQ) in 7-3
            (0 << 2) |           // Reserved ([0]]) in 2
            (dv_type_bits << 0); // DV type in 1-0

  const double one_256 = 0.00390625;
  int dv_12b_444_bits = (dv_type == DV_TYPE_DISPLAY_LED) ? 0 : 1;

  if (cs == 3)
  {
      byte[3] = (xbmc_dv_cap::dv_gx_i << 1) |
                (dv_12b_444_bits << 0);

      byte[4] = (xbmc_dv_cap::dv_gy_i << 1) |
                (0 << 0);

      byte[5] = (xbmc_dv_cap::dv_rx_i << 3) |
                (xbmc_dv_cap::dv_bx_i << 0);

      byte[6] = (xbmc_dv_cap::dv_ry_i << 3) |
                (xbmc_dv_cap::dv_by_i << 0);
  }
  else
  {
    byte[3] = (static_cast<int>(colour_space_data[cs][2] / one_256) << 1) | // Gx in 7-1
              (dv_12b_444_bits << 0);                                       // 12b 444 (Unsupported [0]) in 0

    byte[4] = (static_cast<int>(colour_space_data[cs][3] / one_256) << 1) | // Gy in 7-1
              (0 << 0);                                                     // 10b 444 (Unsupported [0]) in 0

    byte[5] = (static_cast<int>(colour_space_data[cs][0] / one_256) << 3) | // Rx in 7-3
              (static_cast<int>(colour_space_data[cs][4] / one_256) << 0);  // Bx in 2-0

    byte[6] = (static_cast<int>(colour_space_data[cs][1] / one_256) << 3) | // Ry in 7-3
              (static_cast<int>(colour_space_data[cs][5] / one_256) << 0);  // By in 2-0
  }
  std::stringstream ss;
  for (size_t i = 0; i < 7; i++) {
    ss << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << static_cast<int>(byte[i]);
  }
  settings()->SetString(CSettings::SETTING_COREELEC_AMLOGIC_DV_VSVDB_PAYLOAD, ss.str());
}

#if 0
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
  list.emplace_back("PQ 2055 (96 cd/m^2)", 0);      // 2068
  list.emplace_back("PQ 2120 (113 cd/m^2)", 1);     // 2133
  list.emplace_back("PQ 2185 (132 cd/m^2)", 2);     // 2198
  list.emplace_back("PQ 2250 (155 cd/m^2)", 3);     // 2264
  list.emplace_back("PQ 2315 (181 cd/m^2)", 4);     // 2329
  list.emplace_back("PQ 2380 (211 cd/m^2)", 5);     // 2395
  list.emplace_back("PQ 2445 (245 cd/m^2)", 6);     // 2460
  list.emplace_back("PQ 2510 (285 cd/m^2)", 7);     // 2525
  list.emplace_back("PQ 2575 (332 cd/m^2)", 8);     // 2591
  list.emplace_back("PQ 2640 (385 cd/m^2)", 9);     // 2656
  list.emplace_back("PQ 2705 (447 cd/m^2)", 10);    // 2722
  list.emplace_back("PQ 2770 (518 cd/m^2)", 11);    // 2787
  list.emplace_back("PQ 2835 (601 cd/m^2)", 12);    // 2852
  list.emplace_back("PQ 2900 (696 cd/m^2)", 13);    // 2918
  list.emplace_back("PQ 2965 (807 cd/m^2)", 14);    // 2983
  list.emplace_back("PQ 3030 (934 cd/m^2)", 15);    // 3049
  list.emplace_back("PQ 3095 (1082 cd/m^2)", 16);   // 3114
  list.emplace_back("PQ 3160 (1252 cd/m^2)", 17);   // 3179
  list.emplace_back("PQ 3225 (1450 cd/m^2)", 18);   // 3245
  list.emplace_back("PQ 3290 (1678 cd/m^2)", 19);   // 3310
  list.emplace_back("PQ 3355 (1943 cd/m^2)", 20);   // 3376
  list.emplace_back("PQ 3420 (2250 cd/m^2)", 21);   // 3441
  list.emplace_back("PQ 3485 (2607 cd/m^2)", 22);   // 3506
  list.emplace_back("PQ 3550 (3020 cd/m^2)", 23);   // 3572
  list.emplace_back("PQ 3615 (3501 cd/m^2)", 24);   // 3637
  list.emplace_back("PQ 3680 (4060 cd/m^2)", 25);   // 3703
  list.emplace_back("PQ 3745 (4710 cd/m^2)", 26);   // 3768
  list.emplace_back("PQ 3810 (5467 cd/m^2)", 27);   // 3833
  list.emplace_back("PQ 3875 (6351 cd/m^2)", 38);   // 3899
  list.emplace_back("PQ 3940 (7382 cd/m^2)", 29);   // 3964
  list.emplace_back("PQ 4005 (8588 cd/m^2)", 30);   // 4029
  list.emplace_back("PQ 4070 (10000 cd/m^2)", 31);  // 4095
}
#endif

static bool support_dv() {
  // enum DV_TYPE dv_type(static_cast<DV_TYPE>(settings()->GetInt(CSettings::SETTING_COREELEC_AMLOGIC_DV_TYPE)));
  // return ((aml_display_support_dv_std() || aml_display_support_dv_ll() || aml_display_support_hdr_pq()) && (dv_type != DV_TYPE_VS10_ONLY));
  return (aml_display_support_dv_std() || aml_display_support_dv_ll() || aml_display_support_hdr_pq());
}

void dv_type_filler(const SettingConstPtr& setting, std::vector<IntegerSettingOption>& list, int& current, void* data) {
  list.clear();
  if (aml_display_support_dv_std()) list.emplace_back(g_localizeStrings.Get(60023), DV_TYPE_DISPLAY_LED);
  if (aml_display_support_dv_ll()) list.emplace_back(g_localizeStrings.Get(60024), DV_TYPE_PLAYER_LED_LLDV);
  if (aml_display_support_hdr_pq()) list.emplace_back(g_localizeStrings.Get(60025), DV_TYPE_PLAYER_LED_HDR);
  if (aml_display_support_hdr_pq()) list.emplace_back(g_localizeStrings.Get(60579), DV_TYPE_PLAYER_LED_HDR2);
  list.emplace_back(g_localizeStrings.Get(60026), DV_TYPE_VS10_ONLY);
}

void dv_processor_filler(const SettingConstPtr& setting, std::vector<IntegerSettingOption>& list, int& current, void* data) {
  list.clear();
  list.emplace_back(g_localizeStrings.Get(60503), 0);
  if (aml_display_support_hdr_pq()) list.emplace_back(g_localizeStrings.Get(60580), 2);
  if (aml_display_support_hdr_pq()) list.emplace_back(g_localizeStrings.Get(60504), 1);
  if (aml_display_support_dv_ll()) list.emplace_back(g_localizeStrings.Get(60506), 4);
  if (aml_display_support_dv_ll()) list.emplace_back(g_localizeStrings.Get(60505), 3);
}

// 0 Off
// 1 DV as HDR10 444 12 Bit
// 2 DV as HDR10 422 12 Bit
// 3 DV as LLDV 422 12 Bit
// 4 DV as LLDV 444 12 Bit
// 5 DV as LLDV RGB Full 12 Bit

void add_vs10_bypass(std::vector<IntegerSettingOption>& list) {list.emplace_back(g_localizeStrings.Get(60063), DOLBY_VISION_OUTPUT_MODE_BYPASS);}
void add_vs10_dv_bypass(std::vector<IntegerSettingOption>& list) {list.emplace_back(g_localizeStrings.Get(60063), DOLBY_VISION_OUTPUT_MODE_IPT);}
void add_vs10_sdr(std::vector<IntegerSettingOption>& list) {list.emplace_back(g_localizeStrings.Get(60064), DOLBY_VISION_OUTPUT_MODE_SDR10);}
void add_vs10_hdr10(std::vector<IntegerSettingOption>& list) {list.emplace_back(g_localizeStrings.Get(60065), DOLBY_VISION_OUTPUT_MODE_HDR10);}
void add_vs10_dv(std::vector<IntegerSettingOption>& list) {list.emplace_back(g_localizeStrings.Get(60066), DOLBY_VISION_OUTPUT_MODE_IPT);}

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
  add_vs10_sdr(list);
  if (support_dv()) add_vs10_dv_bypass(list);
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
  settingsManager->RegisterSettingOptionsFiller("DolbyVisionProcessor", dv_processor_filler);
// settingsManager->RegisterSettingOptionsFiller("DolbyVisionVSVDBMinLum", vsvdb_min_filler);
// settingsManager->RegisterSettingOptionsFiller("DolbyVisionVSVDBMaxLum", vsvdb_max_filler);
  settingsManager->RegisterSettingOptionsFiller("DolbyVisionVS10SDR8", vs10_sdr_filler);
  settingsManager->RegisterSettingOptionsFiller("DolbyVisionVS10SDR10", vs10_sdr_filler);
  settingsManager->RegisterSettingOptionsFiller("DolbyVisionVS10HDR10", vs10_hdr10_filler);
  settingsManager->RegisterSettingOptionsFiller("DolbyVisionVS10HDR10Plus", vs10_hdr10_filler);
  settingsManager->RegisterSettingOptionsFiller("DolbyVisionVS10HDRHLG", vs10_hdr_hlg_filler);
  settingsManager->RegisterSettingOptionsFiller("DolbyVisionVS10DV", vs10_dv_filler);

  set_visible(CSettings::SETTING_COREELEC_AMLOGIC_DV_MODE_ON_LUMINANCE, true);
  set_visible(CSettings::SETTING_COREELEC_AMLOGIC_DV_TYPE, true);
  set_visible(CSettings::SETTING_COREELEC_AMLOGIC_DV_VIDEO_PROCESSOR, true);
  set_visible(CSettings::SETTING_COREELEC_AMLOGIC_DV_VIDEO_PROCESSOR_TM, true);
  set_visible(CSettings::SETTING_COREELEC_AMLOGIC_DV_TYPE_VP_AUTO, true);
  set_visible(CSettings::SETTING_COREELEC_AMLOGIC_DV_VSVDB_INJECT, true);
  set_visible(CSettings::SETTING_COREELEC_AMLOGIC_DV_VSVDB_PAYLOAD, true);
  set_visible(CSettings::SETTING_COREELEC_AMLOGIC_DV_VSVDB_CS, true);
  set_visible(CSettings::SETTING_COREELEC_AMLOGIC_DV_VSVDB_MIN_LUM, true);
  set_visible(CSettings::SETTING_COREELEC_AMLOGIC_DV_VSVDB_MAX_LUM, true);
  set_visible(CSettings::SETTING_COREELEC_AMLOGIC_DV_STD_SOURCE_LEVEL_5, true);
  set_visible(CSettings::SETTING_COREELEC_AMLOGIC_DV_STD_SOURCE_LEVEL_5_OSDST, true);
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
  set_visible(CSettings::SETTING_COREELEC_AMLOGIC_DV_AUDIO_SEAMLESSBRANCH, true);

  // Register for ui dv mode change - to change on the fly.
  std::set<std::string> settingSet;
  settingSet.insert(CSettings::SETTING_COREELEC_AMLOGIC_DV_MODE);
  settingSet.insert(CSettings::SETTING_COREELEC_AMLOGIC_DV_MODE_ON_LUMINANCE);
  settingSet.insert(CSettings::SETTING_COREELEC_AMLOGIC_DV_TYPE);
  settingSet.insert(CSettings::SETTING_COREELEC_AMLOGIC_DV_VIDEO_PROCESSOR);
  settingSet.insert(CSettings::SETTING_COREELEC_AMLOGIC_DV_TYPE_VP_AUTO);
  settingSet.insert(CSettings::SETTING_COREELEC_AMLOGIC_DV_VSVDB_INJECT);  
  settingSet.insert(CSettings::SETTING_COREELEC_AMLOGIC_DV_VSVDB_CS);
  settingSet.insert(CSettings::SETTING_COREELEC_AMLOGIC_DV_VSVDB_MIN_LUM);
  settingSet.insert(CSettings::SETTING_COREELEC_AMLOGIC_DV_VSVDB_MAX_LUM);
  settingSet.insert(CSettings::SETTING_COREELEC_AMLOGIC_DV_DUAL_PRIORITY);
  settingSet.insert(CSettings::SETTING_COREELEC_AMLOGIC_DV_HDR10PLUS_CONVERT);
  settingSet.insert(CSettings::SETTING_COREELEC_AMLOGIC_DV_HDR10PLUS_PREFER_CONVERT);
  settingSet.insert(CSettings::SETTING_COREELEC_AMLOGIC_DV_VS10_DV);
  settingSet.insert(CSettings::SETTING_COREELEC_AMLOGIC_DV_STD_SOURCE_LEVEL_5);
  settingSet.insert(CSettings::SETTING_COREELEC_AMLOGIC_DV_STD_SOURCE_LEVEL_5_OSDST);
// settingSet.insert(CSettings::SETTING_COREELEC_AMLOGIC_DV_HDR10PLUS_PEAK_BRIGHTNESS_SOURCE);
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

  DOVIStreamMetadata dovi_stream_metadata;
  dovi_stream_metadata = CServiceBroker::GetDataCacheCore().GetVideoDoViStreamMetadata();
  int source_max_pq = static_cast<int>(dovi_stream_metadata.source_max_pq);
  enum DV_TYPE dv_type(static_cast<DV_TYPE>(settings()->GetInt(CSettings::SETTING_COREELEC_AMLOGIC_DV_TYPE)));
  int max_lum_nits_value(settings()->GetInt(CSettings::SETTING_COREELEC_AMLOGIC_DV_VSVDB_MAX_LUM));

  static enum DV_TYPE previous_dv_type = DV_TYPE_DISPLAY_LED;
  bool reset_dv_vs10_dv = false;
  if ((previous_dv_type == DV_TYPE_VS10_ONLY) && (dv_type != DV_TYPE_VS10_ONLY)) reset_dv_vs10_dv = true;
  previous_dv_type = dv_type;

  enum DV_MODE dv_mode(static_cast<DV_MODE>(settings()->GetInt(CSettings::SETTING_COREELEC_AMLOGIC_DV_MODE)));
  unsigned int dv_vp(settings()->GetInt(CSettings::SETTING_COREELEC_AMLOGIC_DV_VIDEO_PROCESSOR));

  bool dv_type_vp_auto(settings()->GetBool(CSettings::SETTING_COREELEC_AMLOGIC_DV_TYPE_VP_AUTO));

  const std::string& settingId = setting->GetId();
  
  if (settingId == CSettings::SETTING_COREELEC_AMLOGIC_DV_MODE)
  {
    // Not working for some cases - needs video playback for mode switch to work correctly everytime.
    // enum DV_MODE dv_mode(static_cast<DV_MODE>(std::dynamic_pointer_cast<const CSettingInt>(setting)->GetValue()));
    // if (dv_mode == DV_MODE_ON) ? aml_dv_on(DOLBY_VISION_OUTPUT_MODE_IPT) : aml_dv_off();
    set_vsvdb_payload_ver(dv_type, max_lum_nits_value, source_max_pq);
    if ((dv_mode == DV_MODE_ON) && (dv_vp != 0)) settings()->SetInt(CSettings::SETTING_COREELEC_AMLOGIC_DV_VIDEO_PROCESSOR, 0);
    if ((dv_mode == DV_MODE_ON) && dv_type_vp_auto) settings()->SetBool(CSettings::SETTING_COREELEC_AMLOGIC_DV_TYPE_VP_AUTO, false);
  }
  else if (settingId == CSettings::SETTING_COREELEC_AMLOGIC_DV_TYPE)
  {
    // Not working for some cases - needs video playback for mode switch to work correctly everytime.
    // aml_dv_start();
    set_vsvdb_payload_ver(dv_type, max_lum_nits_value, source_max_pq);
    if (reset_dv_vs10_dv) settings()->SetInt(CSettings::SETTING_COREELEC_AMLOGIC_DV_VS10_DV, DOLBY_VISION_OUTPUT_MODE_IPT);
    if (dv_type == DV_TYPE_VS10_ONLY) settings()->SetInt(CSettings::SETTING_COREELEC_AMLOGIC_DV_VS10_DV, DOLBY_VISION_OUTPUT_MODE_SDR10);
    // aml_kodi_set_cd_cs(1);
  }
  else if (settingId == CSettings::SETTING_COREELEC_AMLOGIC_DV_VIDEO_PROCESSOR)
  {
    set_vsvdb_payload_ver(dv_type, max_lum_nits_value, source_max_pq);
    if ((dv_vp != 0) && (dv_mode == DV_MODE_ON)) settings()->SetInt(CSettings::SETTING_COREELEC_AMLOGIC_DV_MODE, DV_MODE_ON_DEMAND);
    // aml_kodi_set_cd_cs(1);
  }
  else if (settingId == CSettings::SETTING_COREELEC_AMLOGIC_DV_MODE_ON_LUMINANCE) 
  {
    int max(std::dynamic_pointer_cast<const CSettingInt>(setting)->GetValue());
    aml_dv_set_osd_max(max);
    set_vsvdb_payload_ver(dv_type, max_lum_nits_value, source_max_pq);
  }
//  else if (settingId == CSettings::SETTING_COREELEC_AMLOGIC_DV_HDR10PLUS_PEAK_BRIGHTNESS_SOURCE)
//  {
//    set_vsvdb_payload_ver(dv_type, max_lum_nits_value, source_max_pq);
//  }
  else if (settingId == CSettings::SETTING_COREELEC_AMLOGIC_DV_TYPE_VP_AUTO)
  {
    set_vsvdb_payload_ver(dv_type, max_lum_nits_value, source_max_pq);
    if (dv_type_vp_auto && (dv_mode == DV_MODE_ON)) settings()->SetInt(CSettings::SETTING_COREELEC_AMLOGIC_DV_MODE, DV_MODE_ON_DEMAND);
  }
  else if (settingId == CSettings::SETTING_COREELEC_AMLOGIC_DV_VSVDB_INJECT)
  {
    set_vsvdb_payload_ver(dv_type, max_lum_nits_value, source_max_pq);
  }
  else if (settingId == CSettings::SETTING_COREELEC_AMLOGIC_DV_VSVDB_CS)
  {
//    set_vsvdb_payload_ver(dv_type, max_lum_nits_value, source_max_pq);
  }
  else if (settingId == CSettings::SETTING_COREELEC_AMLOGIC_DV_VSVDB_MIN_LUM)
  {
//    set_vsvdb_payload_ver(dv_type, max_lum_nits_value, source_max_pq);
  }
  else if (settingId == CSettings::SETTING_COREELEC_AMLOGIC_DV_VSVDB_MAX_LUM)
  {
//    set_vsvdb_payload_ver(dv_type, max_lum_nits_value, source_max_pq);
  }
  else if (settingId == CSettings::SETTING_COREELEC_AMLOGIC_DV_DUAL_PRIORITY)
  {
    set_vsvdb_payload_ver(dv_type, max_lum_nits_value, source_max_pq);
  }
  else if (settingId == CSettings::SETTING_COREELEC_AMLOGIC_DV_HDR10PLUS_CONVERT)
  {
    set_vsvdb_payload_ver(dv_type, max_lum_nits_value, source_max_pq);
  }
  else if (settingId == CSettings::SETTING_COREELEC_AMLOGIC_DV_HDR10PLUS_PREFER_CONVERT)
  {
    set_vsvdb_payload_ver(dv_type, max_lum_nits_value, source_max_pq);
  }
  else if (settingId == CSettings::SETTING_COREELEC_AMLOGIC_DV_VS10_DV)
  {
    set_vsvdb_payload_ver(dv_type, max_lum_nits_value, source_max_pq);
    if (dv_type == DV_TYPE_VS10_ONLY) settings()->SetInt(CSettings::SETTING_COREELEC_AMLOGIC_DV_VS10_DV, DOLBY_VISION_OUTPUT_MODE_SDR10);
  }
  else if (settingId == CSettings::SETTING_COREELEC_AMLOGIC_DV_STD_SOURCE_LEVEL_5)
  {
    set_vsvdb_payload_ver(dv_type, max_lum_nits_value, source_max_pq);
  }
  else if (settingId == CSettings::SETTING_COREELEC_AMLOGIC_DV_STD_SOURCE_LEVEL_5_OSDST)
  {
    set_vsvdb_payload_ver(dv_type, max_lum_nits_value, source_max_pq);
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
