#pragma once

/*
 *      Copyright (C) 2005-2015 Team Kodi
 *      http://kodi.tv
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
 *  along with Kodi; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#ifndef TARGET_WINDOWS
#ifndef __cdecl
#define __cdecl
#endif
#ifndef __declspec
#define __declspec(X)
#endif
#endif

#include "xbmc_addon_types.h"
#include <stdint.h>

extern "C"
{
  struct AUDIOENC_INFO
  {
    int dummy;
  };

  typedef int (*audioenc_write_callback)(void* opaque, uint8_t* data, int len);
  typedef int64_t (*audioenc_seek_callback)(void* opaque, int64_t pos, int whence);

  typedef struct
  {
    void*                   opaque;
    audioenc_write_callback write;
    audioenc_seek_callback  seek;
  } audioenc_callbacks;

  typedef struct AddonProps_AudioEncoder
  {
    int dummy;
  } AddonProps_AudioEncoder;

  typedef struct AddonToKodiFuncTable_AudioEncoder
  {
    KODI_HANDLE kodiInstance;
  } AddonToKodiFuncTable_AudioEncoder;

  typedef struct KodiToAddonFuncTable_AudioEncoder
  {
    /*! \brief Create encoder context
     \param callbacks Pointer to audioenc_callbacks structure.
     \return opaque pointer to encoder context, to be passed to other methods.
     \sa IEncoder::Init
     */
    void (*(__cdecl *Create) (audioenc_callbacks* callbacks));

    /*! \brief Start encoder
     \param context Encoder context from Create.
     \param iInChannels Number of channels
     \param iInRate Sample rate of input data
     \param iInBits Bits per sample in input data
     \param title The title of the song
     \param artist The artist of the song
     \param albumartist The albumartist of the song
     \param year The year of the song
     \param track The track number of the song
     \param genre The genre of the song
     \param comment A comment to attach to the song
     \param iTrackLength Total track length in seconds
     \sa IEncoder::Init
     */
    bool (__cdecl* Start) (void* context, int iInChannels, int iInRate, int iInBits,
                           const char* title, const char* artist,
                           const char* albumartist, const char* album,
                           const char* year, const char* track,
                           const char* genre, const char* comment,
                           int iTrackLength);

    /*! \brief Encode a chunk of audio
     \param context Encoder context from Create.
     \param nNumBytesRead Number of bytes in input buffer
     \param pbtStream the input buffer
     \return Number of bytes consumed
     \sa IEncoder::Encode
     */
    int  (__cdecl* Encode) (void* context, int nNumBytesRead, uint8_t* pbtStream);

    /*! \brief Finalize encoding
     \param context Encoder context from Create.
     \return True on success, false on failure.
     */
    bool (__cdecl* Finish) (void* context);

    /*! \brief Free encoder context
     \param context Encoder context to free.
     */
    void (__cdecl* Free)(void* context);
  } KodiToAddonFuncTable_AudioEncoder;

  typedef struct AddonInstance_AudioEncoder
  {
    AddonProps_AudioEncoder props;
    AddonToKodiFuncTable_AudioEncoder toKodi;
    KodiToAddonFuncTable_AudioEncoder toAddon;
  } AddonInstance_AudioEncoder;
}

