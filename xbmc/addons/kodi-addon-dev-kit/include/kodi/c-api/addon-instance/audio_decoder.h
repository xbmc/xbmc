/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "../addon_base.h"
#include "../audio_engine.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

  typedef struct AddonProps_AudioDecoder
  {
    int dummy;
  } AddonProps_AudioDecoder;

  typedef struct AddonToKodiFuncTable_AudioDecoder
  {
    KODI_HANDLE kodiInstance;
  } AddonToKodiFuncTable_AudioDecoder;

  struct AddonInstance_AudioDecoder;
  typedef struct KodiToAddonFuncTable_AudioDecoder
  {
    KODI_HANDLE addonInstance;
    bool(__cdecl* init)(const struct AddonInstance_AudioDecoder* instance,
                        const char* file,
                        unsigned int filecache,
                        int* channels,
                        int* samplerate,
                        int* bitspersample,
                        int64_t* totaltime,
                        int* bitrate,
                        enum AudioEngineDataFormat* format,
                        const enum AudioEngineChannel** info);
    int(__cdecl* read_pcm)(const struct AddonInstance_AudioDecoder* instance,
                           uint8_t* buffer,
                           int size,
                           int* actualsize);
    int64_t(__cdecl* seek)(const struct AddonInstance_AudioDecoder* instance, int64_t time);
    bool(__cdecl* read_tag)(const struct AddonInstance_AudioDecoder* instance,
                            const char* file,
                            char* title,
                            char* artist,
                            int* length);
    int(__cdecl* track_count)(const struct AddonInstance_AudioDecoder* instance, const char* file);
  } KodiToAddonFuncTable_AudioDecoder;

  typedef struct AddonInstance_AudioDecoder
  {
    struct AddonProps_AudioDecoder* props;
    struct AddonToKodiFuncTable_AudioDecoder* toKodi;
    struct KodiToAddonFuncTable_AudioDecoder* toAddon;
  } AddonInstance_AudioDecoder;

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */
