/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#ifndef C_API_ADDONINSTANCE_AUDIO_ENCODER_H
#define C_API_ADDONINSTANCE_AUDIO_ENCODER_H

#include "../addon_base.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

  typedef void* KODI_ADDON_AUDIOENCODER_HDL;

  struct KODI_ADDON_AUDIOENCODER_INFO_TAG
  {
    const char* title;
    const char* artist;
    const char* album;
    const char* album_artist;
    const char* media_type;
    const char* genre;
    int duration;
    int track;
    int disc;
    const char* disc_subtitle;
    int disc_total;
    const char* release_date;
    const char* lyrics;
    int samplerate;
    int channels;
    int bits_per_sample;
    int track_length;
    const char* comment;
  };

  typedef struct AddonToKodiFuncTable_AudioEncoder
  {
    KODI_HANDLE kodiInstance;
    int (*write)(KODI_HANDLE kodiInstance, const uint8_t* data, int len);
    int64_t (*seek)(KODI_HANDLE kodiInstance, int64_t pos, int whence);
  } AddonToKodiFuncTable_AudioEncoder;

  typedef struct KodiToAddonFuncTable_AudioEncoder
  {
    KODI_HANDLE addonInstance;
    bool(__cdecl* start)(const KODI_ADDON_AUDIOENCODER_HDL hdl,
                         const struct KODI_ADDON_AUDIOENCODER_INFO_TAG* tag);
    int(__cdecl* encode)(const KODI_ADDON_AUDIOENCODER_HDL hdl,
                         int num_bytes_read,
                         const uint8_t* pbt_stream);
    bool(__cdecl* finish)(const KODI_ADDON_AUDIOENCODER_HDL hdl);
  } KodiToAddonFuncTable_AudioEncoder;

  typedef struct AddonInstance_AudioEncoder
  {
    struct AddonToKodiFuncTable_AudioEncoder* toKodi;
    struct KodiToAddonFuncTable_AudioEncoder* toAddon;
  } AddonInstance_AudioEncoder;

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* !C_API_ADDONINSTANCE_AUDIO_ENCODER_H */
