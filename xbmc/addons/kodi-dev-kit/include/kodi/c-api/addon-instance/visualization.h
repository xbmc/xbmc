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

#define VIZ_LYRICS_SIZE 32768

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

  typedef KODI_ADDON_INSTANCE_HDL KODI_ADDON_VISUALIZATION_HDL;

  struct VIS_INFO
  {
    bool bWantsFreq;
    int iSyncDelay;
  };

  struct VIS_TRACK
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

  typedef struct AddonProps_Visualization
  {
    ADDON_HARDWARE_CONTEXT device;
    int x;
    int y;
    int width;
    int height;
    float pixelRatio;
  } AddonProps_Visualization;

  typedef struct AddonToKodiFuncTable_Visualization
  {
    void(__cdecl* transfer_preset)(const KODI_HANDLE hdl, const char* preset);
    void(__cdecl* clear_presets)(const KODI_HANDLE hdl);
  } AddonToKodiFuncTable_Visualization;

  struct AddonInstance_Visualization;

  typedef struct KodiToAddonFuncTable_Visualization
  {
    bool(__cdecl* start)(const KODI_ADDON_VISUALIZATION_HDL hdl,
                         int channels,
                         int samples_per_sec,
                         int bits_per_sample,
                         const char* song_name);
    void(__cdecl* stop)(const KODI_ADDON_VISUALIZATION_HDL hdl);

    void(__cdecl* get_info)(const KODI_ADDON_VISUALIZATION_HDL hdl, struct VIS_INFO* info);

    void(__cdecl* audio_data)(const KODI_ADDON_VISUALIZATION_HDL hdl,
                              const float* audio_data,
                              int audio_data_length,
                              float* freq_data,
                              int freq_data_length);
    bool(__cdecl* is_dirty)(const KODI_ADDON_VISUALIZATION_HDL hdl);
    void(__cdecl* render)(const KODI_ADDON_VISUALIZATION_HDL hdl);

    unsigned int(__cdecl* get_presets)(const KODI_ADDON_VISUALIZATION_HDL hdl);
    int(__cdecl* get_active_preset)(const KODI_ADDON_VISUALIZATION_HDL hdl);
    bool(__cdecl* prev_preset)(const KODI_ADDON_VISUALIZATION_HDL hdl);
    bool(__cdecl* next_preset)(const KODI_ADDON_VISUALIZATION_HDL hdl);
    bool(__cdecl* load_preset)(const KODI_ADDON_VISUALIZATION_HDL hdl, int select);
    bool(__cdecl* random_preset)(const KODI_ADDON_VISUALIZATION_HDL hdl);
    bool(__cdecl* lock_preset)(const KODI_ADDON_VISUALIZATION_HDL hdl);
    bool(__cdecl* rate_preset)(const KODI_ADDON_VISUALIZATION_HDL hdl, bool plus_minus);
    bool(__cdecl* is_locked)(const KODI_ADDON_VISUALIZATION_HDL hdl);

    bool(__cdecl* update_albumart)(const KODI_ADDON_VISUALIZATION_HDL hdl, const char* albumart);
    bool(__cdecl* update_track)(const KODI_ADDON_VISUALIZATION_HDL hdl,
                                const struct VIS_TRACK* track);
  } KodiToAddonFuncTable_Visualization;

  typedef struct AddonInstance_Visualization
  {
    struct AddonProps_Visualization* props;
    struct AddonToKodiFuncTable_Visualization* toKodi;
    struct KodiToAddonFuncTable_Visualization* toAddon;
  } AddonInstance_Visualization;

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* !C_API_ADDONINSTANCE_VISUALIZATION_H */
