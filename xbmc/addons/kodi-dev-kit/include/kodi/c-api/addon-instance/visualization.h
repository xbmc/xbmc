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

  struct VIS_INFO
  {
    bool bWantsFreq;
    int iSyncDelay;
  };

  struct VIS_TRACK
  {
    const char *title;
    const char *artist;
    const char *album;
    const char *albumArtist;
    const char *genre;
    const char *comment;
    const char *lyrics;

    const char *reserved1;
    const char *reserved2;

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
    const char* name;
    const char* presets;
    const char* profile;
  } AddonProps_Visualization;

  typedef struct AddonToKodiFuncTable_Visualization
  {
    KODI_HANDLE kodiInstance;
    void(__cdecl* transfer_preset)(KODI_HANDLE kodiInstance, const char* preset);
    void(__cdecl* clear_presets)(KODI_HANDLE kodiInstance);
  } AddonToKodiFuncTable_Visualization;

  struct AddonInstance_Visualization;

  typedef struct KodiToAddonFuncTable_Visualization
  {
    KODI_HANDLE addonInstance;
    bool(__cdecl* start)(const struct AddonInstance_Visualization* instance,
                         int channels,
                         int samples_per_sec,
                         int bits_per_sample,
                         const char* song_name);
    void(__cdecl* stop)(const struct AddonInstance_Visualization* instance);

    void(__cdecl* get_info)(const struct AddonInstance_Visualization* instance,
                            struct VIS_INFO* info);

    void(__cdecl* audio_data)(const struct AddonInstance_Visualization* instance,
                              const float* audio_data,
                              int audio_data_length,
                              float* freq_data,
                              int freq_data_length);
    bool(__cdecl* is_dirty)(const struct AddonInstance_Visualization* instance);
    void(__cdecl* render)(const struct AddonInstance_Visualization* instance);

    unsigned int(__cdecl* get_presets)(const struct AddonInstance_Visualization* instance);
    int(__cdecl* get_active_preset)(const struct AddonInstance_Visualization* instance);
    bool(__cdecl* prev_preset)(const struct AddonInstance_Visualization* instance);
    bool(__cdecl* next_preset)(const struct AddonInstance_Visualization* instance);
    bool(__cdecl* load_preset)(const struct AddonInstance_Visualization* instance, int select);
    bool(__cdecl* random_preset)(const struct AddonInstance_Visualization* instance);
    bool(__cdecl* lock_preset)(const struct AddonInstance_Visualization* instance);
    bool(__cdecl* rate_preset)(const struct AddonInstance_Visualization* instance, bool plus_minus);
    bool(__cdecl* is_locked)(const struct AddonInstance_Visualization* instance);

    bool(__cdecl* update_albumart)(const struct AddonInstance_Visualization* instance,
                                   const char* albumart);
    bool(__cdecl* update_track)(const struct AddonInstance_Visualization* instance,
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
