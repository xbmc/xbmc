#pragma once
/*
 *      Copyright (C) 2015-2016 Team KODI
 *      Copyright (C) 2005-2013 Team XBMC
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

#include "../definitions.hpp"

#include <algorithm>
#include <map>
#include <vector>
#include "../.internal/AddonLib_internal.hpp"

API_NAMESPACE

namespace KodiAPI
{
namespace PVR
{

  //============================================================================
  ///
  /// \defgroup CPP_KodiAPI_PVR_StreamUtils Stream Utils
  /// \ingroup CPP_KodiAPI_PVR

  //==========================================================================
  ///
  /// \defgroup CPP_KodiAPI_PVR_CPVRStream CPVRStream
  /// \ingroup CPP_KodiAPI_PVR_StreamUtils
  /// @{
  /// @brief <b>PVR stream properties data class</b>
  ///
  /// Represents a single stream. It extends the PODS to provide some operators
  /// overloads.
  ///
  /// All for one PVR stream needed data can be becomes stored inside this class
  /// to have it available on requests.
  ///
  ///
  /// ------------------------------------------------------------------------
  ///
  /// <b>PVR stream properties:</b>
  ///
  /// Structure here shows the construction that is used on the PVR system and
  /// in this class (CPVRStream) can be edited.
  ///
  /// ~~~~~~~~~~~~~{.cpp}
  /// /*
  ///  * PVR stream properties
  ///  *
  ///  * Only as example shown here! See always the original structure on related header.
  ///  */
  /// typedef struct PVR_STREAM_PROPERTIES
  /// {
  ///   unsigned int iStreamCount;
  ///   struct PVR_STREAM
  ///   {
  ///     unsigned int      iPhysicalId;        /* physical index */
  ///     kodi_codec_type   iCodecType;         /* codec type this stream */
  ///     kodi_codec_id     iCodecId;           /* codec id of this stream */
  ///     char              strLanguage[4];     /* language id */
  ///     int               iFPSScale;          /* scale of 1000 and a rate of 29970 will result in 29.97 fps */
  ///     int               iFPSRate;           /* FPS rate */
  ///     int               iHeight;            /* height of the stream reported by the demuxer */
  ///     int               iWidth;             /* width of the stream reported by the demuxer */
  ///     float             fAspect;            /* display aspect ratio of the stream */
  ///     int               iChannels;          /* amount of channels */
  ///     int               iSampleRate;        /* sample rate */
  ///     int               iBlockAlign;        /* block alignment */
  ///     int               iBitRate;           /* bit rate */
  ///     int               iBitsPerSample;     /* bits per sample */
  ///    } stream[PVR_STREAM_MAX_STREAMS];      /* the streams */
  /// } ATTRIBUTE_PACKED PVR_STREAM_PROPERTIES;
  /// ~~~~~~~~~~~~~
  ///
  /// It has the header \ref StreamUtils.hpp "#include <kodi/api2/pvr/StreamUtils.hpp>" be included
  /// to enjoy it.
  ///
  ///
  /// ------------------------------------------------------------------------
  ///
  /// **Code Example:**
  /// ~~~~~~~~~~~~~{.cpp}
  /// /*
  ///  * Note: This is only a example to show the easiest use of them, for
  ///  * them is always only one and the same stream type present.
  ///  *
  ///  * The class CStreamProperties is designed to store several streams
  ///  * and to alow updates of codec Id's during usage of them.
  ///  */
  /// #include <kodi/api2/pvr/StreamUtils.hpp>
  ///
  /// KodiAPI::PVR::StreamUtils::CStreamProperties activeChannelStreams;
  /// ...
  /// void UpdateStreams()
  /// {
  ///   CodecDescriptor codecId = KodiAPI::InputStream::GetCodecByName("pcm_f32le");
  ///   if (codecId.Codec().codec_type == XBMC_CODEC_TYPE_AUDIO)
  ///   {
  ///     KodiAPI::PVR::StreamUtils::CPVRStream newStream;
  ///     /* Get the stream data if already present and do only update */
  ///     activeChannelStreams.GetStreamData(1, &newStream);
  ///
  ///     newStream.iCodecType      = codecId.Codec().codec_type;
  ///     newStream.iCodecId        = codecId.Codec().codec_id;
  ///     newStream.iChannels       = 2;
  ///     newStream.iSampleRate     = 48000;
  ///     newStream.iBitsPerSample  = 32;
  ///     newStream.iBitRate        = newStream.iSampleRate * newStream.iChannels * newStream.iBitsPerSample;
  ///     newStream.strLanguage[0]  = 0;
  ///     newStream.strLanguage[1]  = 0;
  ///     newStream.strLanguage[2]  = 0;
  ///     newStream.strLanguage[3]  = 0;
  ///
  ///     newStreams.push_back(newStream);
  ///     activeChannelStreams.UpdateStreams(newStreams);
  ///   }
  ///   else
  ///   {
  ///     activeChannelStreams.Clear();
  ///   }
  /// }
  /// ~~~~~~~~~~~~~
  ///
  class CPVRStream : public PVR_STREAM_PROPERTIES::PVR_STREAM
  {
  public:
    //==========================================================================
    ///
    /// @ingroup CPP_KodiAPI_PVR_CPVRStream
    /// @brief Class constructor
    ///
    CPVRStream();
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// @ingroup CPP_KodiAPI_PVR_CPVRStream
    /// @brief Class constructor to create copy of them
    ///
    /// @param[in] other      The other stream class to copy from
    ///
    CPVRStream(const CPVRStream &other);
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// @ingroup CPP_KodiAPI_PVR_CPVRStream
    /// @brief Copy stream class
    ///
    /// Overwrite stream class with data from other stream
    ///
    /// @param[in] other      The other stream class to copy from
    ///
    CPVRStream& operator=(const CPVRStream &other);
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// @ingroup CPP_KodiAPI_PVR_CPVRStream
    ///
    /// Compares this stream based on another stream
    ///
    /// @param[in] other Other stream to compare from
    /// @return true if equal, otherwise false
    ///
    bool operator==(const CPVRStream &other) const;
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// @ingroup CPP_KodiAPI_PVR_CPVRStream
    /// @brief Compare about Id is above
    ///
    /// Compares this stream with another one so that video streams are sorted
    /// before any other streams and the others are sorted by the physical ID
    ///
    /// @param[in] other Other stream to compare from
    /// @return true if the other have a higher Id
    ///
    bool operator<(const CPVRStream &other) const;
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// @ingroup CPP_KodiAPI_PVR_CPVRStream
    ///
    /// Clears the present stream data on class.
    ///
    void Clear();
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// @ingroup CPP_KodiAPI_PVR_CPVRStream
    ///
    /// Checks whether the stream has been cleared or not.
    ///
    /// @return true if cleared
    ///
    bool IsCleared() const;
    //--------------------------------------------------------------------------
  };
  /// @}
  /*\___________________________________________________________________________
  \*/

  ///
  /// \defgroup CPP_KodiAPI_PVR_CStreamProperties CStreamProperties
  /// \ingroup CPP_KodiAPI_PVR_StreamUtils
  /// @{
  /// @brief <b>PVR stream properties handle class</b>
  ///
  /// This can be used to view the available streams from their own system for
  /// the playback to store and a call to
  /// <tt><b>PVR_ERROR GetStreamProperties(PVR_STREAM_PROPERTIES pProperties*)</b></tt> by
  /// encoding the data requested with CStreamProperties::GetProperties(...).
  ///
  /// It has the header \ref StreamUtils.hpp "#include <kodi/api2/pvr/StreamUtils.hpp>" be included
  /// to enjoy it.
  ///
  class CStreamProperties
  {
  public:
    ///
    /// @ingroup CPP_KodiAPI_PVR_CStreamProperties
    /// @brief Type definition for stream_vector
    ///
    typedef std::vector<CPVRStream> stream_vector;
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// @ingroup CPP_KodiAPI_PVR_CStreamProperties
    /// @brief Class constructor
    ///
    CStreamProperties(void);
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// @ingroup CPP_KodiAPI_PVR_CStreamProperties
    /// @brief Class Descructor
    ///
    virtual ~CStreamProperties(void);
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// @ingroup CPP_KodiAPI_PVR_CStreamProperties
    /// @brief Clear stored stream list.
    ///
    /// Resets all the stored streams.
    ///
    void Clear(void);
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// @ingroup CPP_KodiAPI_PVR_CStreamProperties
    /// @brief Returns the index of the stream
    ///
    /// Returns the index of the stream with the specified physical ID, or -1 if
    /// there no stream is found. This method is called very often which is why
    /// we keep a separate map for this.
    ///
    /// @param[in] iPhysicalId The own used physical Id to identify
    /// @return Kodi's used stream Id if present, or -1 if not found
    ///
    int GetStreamId(unsigned int iPhysicalId) const;
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// @ingroup CPP_KodiAPI_PVR_CStreamProperties
    /// @brief Get the stored stream if present
    ///
    /// Returns the stream with the specified physical ID, or null if no such
    /// stream exists
    ///
    /// @note Call returns data constant and is not editable.
    ///
    /// @param[in] iPhysicalId The own used physical Id to identify
    /// @return The stored stream data or null if not exist
    ///
    CPVRStream* GetStreamById(unsigned int iPhysicalId) const;
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// @ingroup CPP_KodiAPI_PVR_CStreamProperties
    /// @brief To populates the specified stream
    ///
    /// Populates the specified stream with the stream having the specified
    /// physical ID. If the stream is not found only target stream's physical ID
    /// will be populated.
    ///
    /// @param[in] iPhysicalId The own used physical Id to identify
    /// @param[out] stream Place to store stream data
    ///
    void GetStreamData(unsigned int iPhysicalId, CPVRStream* stream);
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// @ingroup CPP_KodiAPI_PVR_CStreamProperties
    /// @brief To populates properties for Kodi
    ///
    /// Populates props with the current streams and returns whether there are
    /// any streams at the moment or not.
    ///
    /// @param[out] props Address where the PVR system stream properties becomes stored
    /// @return True if successed, otherwise false
    ///
    ///
    /// ------------------------------------------------------------------------
    ///
    /// **Code Example:**
    /// ~~~~~~~~~~~~~{.cpp}
    /// #include <kodi/api2/pvr/StreamUtils.hpp>
    ///
    /// KodiAPI::PVR::StreamUtils::CStreamProperties activeChannelStreams;
    /// ...
    /// /* The function here is for calls from Kodi to add-on on PVR system.
    ///  * Therefore asks Kodi the available data of the running streams.
    ///  */
    /// PVR_ERROR GetStreamProperties(PVR_STREAM_PROPERTIES* pProperties)
    /// {
    ///   return (activeChannelStreams.GetProperties(pProperties) ? PVR_ERROR_NO_ERROR : PVR_ERROR_SERVER_ERROR);
    /// }
    /// ~~~~~~~~~~~~~
    ///
    bool GetProperties(PVR_STREAM_PROPERTIES* props);
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// @ingroup CPP_KodiAPI_PVR_CStreamProperties
    /// @brief Update streams
    ///
    /// Merges new streams into the current list of streams. Identical streams
    /// will retain their respective indexes and new streams will replace unused
    /// indexes or be appended.
    ///
    /// @param[in] newStreams New stream data to add
    ///
    ///
    /// ------------------------------------------------------------------------
    ///
    /// **Code Example:**
    /// ~~~~~~~~~~~~~{.cpp}
    /// /*
    ///  * Note: This is only a example to show the easiest use of them, for
    ///  * them is always only one and the same stream type present.
    ///  *
    ///  * The class CStreamProperties is designed to store several streams
    ///  * and to alow updates of codec Id's during usage of them.
    ///  */
    /// #include <kodi/api2/pvr/StreamUtils.hpppp>
    ///
    /// KodiAPI::PVR::StreamUtils::CStreamProperties activeChannelStreams;
    /// ...
    /// void UpdateStreams()
    /// {
    ///   KodiAPI::InputStream::CodecDescriptor codecId = KodiAPI::InputStream::GetCodecByName("pcm_f32le");
    ///   if (codecId.Codec().codec_type == XBMC_CODEC_TYPE_AUDIO)
    ///   {
    ///     KodiAPI::PVR::StreamUtils::CPVRStream newStream;
    ///     /* Get the stream data if already present and do only update */
    ///     activeChannelStreams.GetStreamData(1, &newStream);
    ///
    ///     newStream.iCodecType      = codecId.Codec().codec_type;
    ///     newStream.iCodecId        = codecId.Codec().codec_id;
    ///     newStream.iChannels       = 2;
    ///     newStream.iSampleRate     = 48000;
    ///     newStream.iBitsPerSample  = 32;
    ///     newStream.iBitRate        = newStream.iSampleRate * newStream.iChannels * newStream.iBitsPerSample;
    ///     newStream.strLanguage[0]  = 0;
    ///     newStream.strLanguage[1]  = 0;
    ///     newStream.strLanguage[2]  = 0;
    ///     newStream.strLanguage[3]  = 0;
    ///
    ///     newStreams.push_back(newStream);
    ///     activeChannelStreams.UpdateStreams(newStreams);
    ///   }
    ///   else
    ///   {
    ///     activeChannelStreams.Clear();
    ///   }
    /// }
    /// ~~~~~~~~~~~~~
    ///
    void UpdateStreams(stream_vector &newStreams);
    //--------------------------------------------------------------------------

  #ifndef DOXYGEN_SHOULD_SKIP_THIS
    IMPL_STREAM_PROPS;
  #endif
  };
  /// @}

} /* namespace PVR */
} /* namespace KodiAPI */

END_NAMESPACE()
