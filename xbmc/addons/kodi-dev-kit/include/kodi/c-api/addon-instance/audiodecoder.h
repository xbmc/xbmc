/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#ifndef C_API_ADDONINSTANCE_AUDIODECODER_H
#define C_API_ADDONINSTANCE_AUDIODECODER_H

#include "../addon_base.h"
#include "../audio_engine.h"

//============================================================================
/// @ingroup cpp_kodi_addon_audiodecoder_Defs
/// @brief Identifier which is attached to stream files and with defined names
/// in addon.xml (`name="???"`) if addon supports "tracks" (set in addon.xml
/// `tracks="true"`).
///
/// @note This macro is largely unnecessary to use directly on the addon,
/// addon can use the @ref KODI_ADDON_AUDIODECODER_GET_TRACK_EXT around its
/// associated name.
///
///@{
#define KODI_ADDON_AUDIODECODER_TRACK_EXT "_adecstrm"
///@}
//----------------------------------------------------------------------------

//============================================================================
/// @ingroup cpp_kodi_addon_audiodecoder_Defs
/// @brief Macro to get file extension to track supported files.
///
/// This macro can be used if `tracks="true"` is set in addon.xml, in this
/// case the addon.xml field `name="???"` is used to identify stream.
/// Which must then also be used for here.
///
///@{
#define KODI_ADDON_AUDIODECODER_GET_TRACK_EXT(name) "." name KODI_ADDON_AUDIODECODER_TRACK_EXT
///@}
//----------------------------------------------------------------------------

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

  typedef KODI_ADDON_INSTANCE_HDL KODI_ADDON_AUDIODECODER_HDL;

  //============================================================================
  /// @defgroup cpp_kodi_addon_audiodecoder_Defs_AUDIODECODER_READ_RETURN enum AUDIODECODER_READ_RETURN
  /// @ingroup cpp_kodi_addon_audiodecoder_Defs
  /// @brief **Return value about** @ref kodi::addon::CInstanceAudioDecoder::ReadPCM()
  ///
  /// Possible values are:
  /// | Value | enum                           | Description
  /// |:-----:|:-------------------------------|:--------------------------
  /// |   0   | @ref AUDIODECODER_READ_SUCCESS | on success
  /// |  -1   | @ref AUDIODECODER_READ_EOF     | on end of stream
  /// |   1   | @ref AUDIODECODER_READ_ERROR   | on failure
  ///
  ///@{
  typedef enum AUDIODECODER_READ_RETURN
  {
    /// @brief On end of stream
    AUDIODECODER_READ_EOF = -1,

    /// @brief On success
    AUDIODECODER_READ_SUCCESS = 0,

    /// @brief On failure
    AUDIODECODER_READ_ERROR = 1
  } AUDIODECODER_READ_RETURN;
  ///@}
  //----------------------------------------------------------------------------

  struct KODI_ADDON_AUDIODECODER_INFO_TAG
  {
    char* title;
    char* artist;
    char* album;
    char* album_artist;
    char* media_type;
    char* genre;
    int duration;
    int track;
    int disc;
    char* disc_subtitle;
    int disc_total;
    char* release_date;
    char* lyrics;
    int samplerate;
    int channels;
    int bitrate;
    char* comment;
    char* cover_art_path;
    char* cover_art_mem_mimetype;
    uint8_t* cover_art_mem;
    size_t cover_art_mem_size;
  };

  typedef bool(ATTR_APIENTRYP PFN_KODI_ADDON_AUDIODECODER_SUPPORTS_FILE_V1)(
      const KODI_ADDON_AUDIODECODER_HDL hdl, const char* file);
  typedef bool(ATTR_APIENTRYP PFN_KODI_ADDON_AUDIODECODER_INIT_V1)(
      const KODI_ADDON_AUDIODECODER_HDL hdl,
      const char* file,
      unsigned int filecache,
      int* channels,
      int* samplerate,
      int* bitspersample,
      int64_t* totaltime,
      int* bitrate,
      enum AudioEngineDataFormat* format,
      enum AudioEngineChannel info[AUDIOENGINE_CH_MAX]);
  typedef int(ATTR_APIENTRYP PFN_KODI_ADDON_AUDIODECODER_READ_PCM_V1)(
      const KODI_ADDON_AUDIODECODER_HDL hdl, uint8_t* buffer, size_t size, size_t* actualsize);
  typedef int64_t(ATTR_APIENTRYP PFN_KODI_ADDON_AUDIODECODER_SEEK_V1)(
      const KODI_ADDON_AUDIODECODER_HDL hdl, int64_t time);
  typedef bool(ATTR_APIENTRYP PFN_KODI_ADDON_AUDIODECODER_READ_TAG_V1)(
      const KODI_ADDON_AUDIODECODER_HDL hdl,
      const char* file,
      struct KODI_ADDON_AUDIODECODER_INFO_TAG* tag);
  typedef int(ATTR_APIENTRYP PFN_KODI_ADDON_AUDIODECODER_TRACK_COUNT_V1)(
      const KODI_ADDON_AUDIODECODER_HDL hdl, const char* file);

  typedef struct AddonToKodiFuncTable_AudioDecoder
  {
    KODI_HANDLE kodiInstance;
  } AddonToKodiFuncTable_AudioDecoder;

  typedef struct KodiToAddonFuncTable_AudioDecoder
  {
    PFN_KODI_ADDON_AUDIODECODER_SUPPORTS_FILE_V1 supports_file;
    PFN_KODI_ADDON_AUDIODECODER_INIT_V1 init;
    PFN_KODI_ADDON_AUDIODECODER_READ_PCM_V1 read_pcm;
    PFN_KODI_ADDON_AUDIODECODER_SEEK_V1 seek;
    PFN_KODI_ADDON_AUDIODECODER_READ_TAG_V1 read_tag;
    PFN_KODI_ADDON_AUDIODECODER_TRACK_COUNT_V1 track_count;
  } KodiToAddonFuncTable_AudioDecoder;

  typedef struct AddonInstance_AudioDecoder
  {
    struct AddonToKodiFuncTable_AudioDecoder* toKodi;
    struct KodiToAddonFuncTable_AudioDecoder* toAddon;
  } AddonInstance_AudioDecoder;

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* !C_API_ADDONINSTANCE_AUDIODECODER_H */
