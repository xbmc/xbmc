#pragma once
/*
 *      Copyright (C) 2015-2016 Team KODI
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
 *  along with KODI; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

struct DemuxPacket;

API_NAMESPACE

namespace KodiAPI
{

  /*!
  \defgroup CPP_KodiAPI_InputStream 5. Input Stream
  \ingroup cpp
  @brief <b>Demux packet handle and codec identification class</b>

  This class  brings the  support  for obtaining required codec  identifications
  and packages for handling stream demuxer.

  These are pure static functions them no other initialization need.

  It has the header \ref InputStream.hpp "#include <kodi/api3/inputstream/InputStream.hpp>"
  to enjoy it.
  */

  //============================================================================
  /// \ingroup CPP_KodiAPI_InputStream_CodecDescriptor_Defs
  /// @brief Codec identifier
  ///
  typedef unsigned int kodi_codec_id;
  //----------------------------------------------------------------------------


  //============================================================================
  /// \ingroup CPP_KodiAPI_InputStream_CodecDescriptor_Defs
  /// @brief Codecs stream type definitions
  ///
  typedef enum kodi_codec_type
  {
    /// Unknown identifier
    KODI_CODEC_TYPE_UNKNOWN = -1,
    /// Video stream format
    KODI_CODEC_TYPE_VIDEO,
    /// Audio stream format
    KODI_CODEC_TYPE_AUDIO,
    /// Data stream format
    KODI_CODEC_TYPE_DATA,
    /// Subtitle stream format
    KODI_CODEC_TYPE_SUBTITLE,
    /// Radio RDS stream format
    KODI_CODEC_TYPE_RDS,
    /// Not used
    KODI_CODEC_TYPE_NB
  } kodi_codec_type;
  //----------------------------------------------------------------------------


  //============================================================================
  /// \ingroup CPP_KodiAPI_InputStream_CodecDescriptor_Defs
  /// @brief Codec identification structure.
  ///
  typedef struct kodi_codec
  {
    /// Stream type identification
    kodi_codec_type codec_type;
    /// Needed codec identification (is code based upon ffmpeg)
    kodi_codec_id   codec_id;
  } kodi_codec;

  #define KODI_INVALID_CODEC_ID 0
  #define KODI_INVALID_CODEC    { API_NAMESPACE_NAME::KodiAPI::KODI_CODEC_TYPE_UNKNOWN, KODI_INVALID_CODEC_ID }
  //----------------------------------------------------------------------------

} /* namespace KodiAPI */

END_NAMESPACE()
