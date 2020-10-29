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

  typedef struct AddonProps_AudioEncoder
  {
    int dummy;
  } AddonProps_AudioEncoder;

  typedef struct AddonToKodiFuncTable_AudioEncoder
  {
    KODI_HANDLE kodiInstance;
    int (*write)(KODI_HANDLE kodiInstance, const uint8_t* data, int len);
    int64_t (*seek)(KODI_HANDLE kodiInstance, int64_t pos, int whence);
  } AddonToKodiFuncTable_AudioEncoder;

  struct AddonInstance_AudioEncoder;
  typedef struct KodiToAddonFuncTable_AudioEncoder
  {
    KODI_HANDLE addonInstance;
    bool(__cdecl* start)(const struct AddonInstance_AudioEncoder* instance,
                         int in_channels,
                         int in_rate,
                         int in_bits,
                         const char* title,
                         const char* artist,
                         const char* albumartist,
                         const char* album,
                         const char* year,
                         const char* track,
                         const char* genre,
                         const char* comment,
                         int track_length);
    int(__cdecl* encode)(const struct AddonInstance_AudioEncoder* instance,
                         int num_bytes_read,
                         const uint8_t* pbt_stream);
    bool(__cdecl* finish)(const struct AddonInstance_AudioEncoder* instance);
  } KodiToAddonFuncTable_AudioEncoder;

  typedef struct AddonInstance_AudioEncoder
  {
    struct AddonProps_AudioEncoder* props;
    struct AddonToKodiFuncTable_AudioEncoder* toKodi;
    struct KodiToAddonFuncTable_AudioEncoder* toAddon;
  } AddonInstance_AudioEncoder;

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* !C_API_ADDONINSTANCE_AUDIO_ENCODER_H */
