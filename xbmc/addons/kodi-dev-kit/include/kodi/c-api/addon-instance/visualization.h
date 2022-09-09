/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#ifndef C_API_ADDONINSTANCE_VISUALIZATION_H
#define C_API_ADDONINSTANCE_VISUALIZATION_H

#include "../addon_base.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

  typedef KODI_ADDON_INSTANCE_HDL KODI_ADDON_VISUALIZATION_HDL;

  struct KODI_ADDON_VISUALIZATION_TRACK
  {
    const char* title;
    const char* artist;
    const char* album;
    const char* albumArtist;
    const char* genre;
    const char* comment;
    const char* lyrics;

    const char* reserved1;
    const char* reserved2;

    int trackNumber;
    int discNumber;
    int duration;
    int year;
    int rating;

    int reserved3;
    int reserved4;
  };

  struct KODI_ADDON_VISUALIZATION_PROPS
  {
    ADDON_HARDWARE_CONTEXT device;
    int x;
    int y;
    int width;
    int height;
    float pixelRatio;
  };

  typedef bool(ATTR_APIENTRYP PFN_KODI_ADDON_VISUALIZATION_START_V1)(
      const KODI_ADDON_VISUALIZATION_HDL hdl,
      int channels,
      int samples_per_sec,
      int bits_per_sample,
      const char* song_name);
  typedef void(ATTR_APIENTRYP PFN_KODI_ADDON_VISUALIZATION_STOP_V1)(
      const KODI_ADDON_VISUALIZATION_HDL hdl);

  typedef int(ATTR_APIENTRYP PFN_KODI_ADDON_VISUALIZATION_GET_SYNC_DELAY_V1)(
      const KODI_ADDON_VISUALIZATION_HDL hdl);

  typedef void(ATTR_APIENTRYP PFN_KODI_ADDON_VISUALIZATION_AUDIO_DATA_V1)(
      const KODI_ADDON_VISUALIZATION_HDL hdl, const float* audio_data, size_t audio_data_length);
  typedef bool(ATTR_APIENTRYP PFN_KODI_ADDON_VISUALIZATION_IS_DIRTY_V1)(
      const KODI_ADDON_VISUALIZATION_HDL hdl);
  typedef void(ATTR_APIENTRYP PFN_KODI_ADDON_VISUALIZATION_RENDER_V1)(
      const KODI_ADDON_VISUALIZATION_HDL hdl);

  typedef unsigned int(ATTR_APIENTRYP PFN_KODI_ADDON_VISUALIZATION_GET_PRESETS_V1)(
      const KODI_ADDON_VISUALIZATION_HDL hdl);
  typedef int(ATTR_APIENTRYP PFN_KODI_ADDON_VISUALIZATION_GET_ACTIVE_PRESET_V1)(
      const KODI_ADDON_VISUALIZATION_HDL hdl);
  typedef bool(ATTR_APIENTRYP PFN_KODI_ADDON_VISUALIZATION_PREV_PRESET_V1)(
      const KODI_ADDON_VISUALIZATION_HDL hdl);
  typedef bool(ATTR_APIENTRYP PFN_KODI_ADDON_VISUALIZATION_NEXT_PRESET_V1)(
      const KODI_ADDON_VISUALIZATION_HDL hdl);
  typedef bool(ATTR_APIENTRYP PFN_KODI_ADDON_VISUALIZATION_LOAD_PRESET_V1)(
      const KODI_ADDON_VISUALIZATION_HDL hdl, int select);
  typedef bool(ATTR_APIENTRYP PFN_KODI_ADDON_VISUALIZATION_RANDOM_PRESET_V1)(
      const KODI_ADDON_VISUALIZATION_HDL hdl);
  typedef bool(ATTR_APIENTRYP PFN_KODI_ADDON_VISUALIZATION_LOCK_PRESET_V1)(
      const KODI_ADDON_VISUALIZATION_HDL hdl);
  typedef bool(ATTR_APIENTRYP PFN_KODI_ADDON_VISUALIZATION_RATE_PRESET_V1)(
      const KODI_ADDON_VISUALIZATION_HDL hdl, bool plus_minus);
  typedef bool(ATTR_APIENTRYP PFN_KODI_ADDON_VISUALIZATION_IS_LOCKED_V1)(
      const KODI_ADDON_VISUALIZATION_HDL hdl);

  typedef bool(ATTR_APIENTRYP PFN_KODI_ADDON_VISUALIZATION_UPDATE_ALBUMART_V1)(
      const KODI_ADDON_VISUALIZATION_HDL hdl, const char* albumart);
  typedef bool(ATTR_APIENTRYP PFN_KODI_ADDON_VISUALIZATION_UPDATE_TRACK_V1)(
      const KODI_ADDON_VISUALIZATION_HDL hdl, const struct KODI_ADDON_VISUALIZATION_TRACK* track);

  typedef struct KodiToAddonFuncTable_Visualization
  {
    PFN_KODI_ADDON_VISUALIZATION_START_V1 start;
    PFN_KODI_ADDON_VISUALIZATION_STOP_V1 stop;

    PFN_KODI_ADDON_VISUALIZATION_GET_SYNC_DELAY_V1 get_sync_delay;

    PFN_KODI_ADDON_VISUALIZATION_AUDIO_DATA_V1 audio_data;
    PFN_KODI_ADDON_VISUALIZATION_IS_DIRTY_V1 is_dirty;
    PFN_KODI_ADDON_VISUALIZATION_RENDER_V1 render;

    PFN_KODI_ADDON_VISUALIZATION_GET_PRESETS_V1 get_presets;
    PFN_KODI_ADDON_VISUALIZATION_GET_ACTIVE_PRESET_V1 get_active_preset;
    PFN_KODI_ADDON_VISUALIZATION_PREV_PRESET_V1 prev_preset;
    PFN_KODI_ADDON_VISUALIZATION_NEXT_PRESET_V1 next_preset;
    PFN_KODI_ADDON_VISUALIZATION_LOAD_PRESET_V1 load_preset;
    PFN_KODI_ADDON_VISUALIZATION_RANDOM_PRESET_V1 random_preset;
    PFN_KODI_ADDON_VISUALIZATION_LOCK_PRESET_V1 lock_preset;
    PFN_KODI_ADDON_VISUALIZATION_RATE_PRESET_V1 rate_preset;
    PFN_KODI_ADDON_VISUALIZATION_IS_LOCKED_V1 is_locked;

    PFN_KODI_ADDON_VISUALIZATION_UPDATE_ALBUMART_V1 update_albumart;
    PFN_KODI_ADDON_VISUALIZATION_UPDATE_TRACK_V1 update_track;
  } KodiToAddonFuncTable_Visualization;

  typedef struct AddonToKodiFuncTable_Visualization
  {
    void (*get_properties)(const KODI_HANDLE hdl, struct KODI_ADDON_VISUALIZATION_PROPS* props);
    void (*transfer_preset)(const KODI_HANDLE hdl, const char* preset);
    void (*clear_presets)(const KODI_HANDLE hdl);
  } AddonToKodiFuncTable_Visualization;

  typedef struct AddonInstance_Visualization
  {
    struct AddonToKodiFuncTable_Visualization* toKodi;
    struct KodiToAddonFuncTable_Visualization* toAddon;
  } AddonInstance_Visualization;

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* !C_API_ADDONINSTANCE_VISUALIZATION_H */
