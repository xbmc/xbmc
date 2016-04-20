#pragma once
/*
 *      Copyright (C) 2005-2014 Team KODI
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
#include "../.internal/AddonLib_internal.hpp"

API_NAMESPACE

namespace KodiAPI
{
namespace AudioEngine
{

  //============================================================================
  ///
  /// \defgroup CPP_KodiAPI_AudioEngine_CStream Test stream (class CStream)
  /// \ingroup CPP_KodiAPI_AudioEngine
  /// @{
  /// @brief <b>Audio Engine Stream Class</b>
  ///

  //============================================================================
  ///
  /// \defgroup CPP_KodiAPI_AudioEngine_CStream_Defs Definitions, structures and enumerators
  /// \ingroup CPP_KodiAPI_AudioEngine_CStream
  /// @brief <b>Library definition values</b>
  ///

  class CStream
  {
  public:
    //============================================================================
    ///
    /// @ingroup CPP_KodiAPI_AudioEngine_CStream
    /// @brief Class constructor
    ///
    /// @param[in] Format        Used data format for stream
    /// @param[in] Options [opt] A bit field of stream options (see: enum AEStreamOptions)
    ///
    ///
    /// ------------------------------------------------------------------------
    ///
    /// <b>Audio engine format information:</b>
    /// @code
    /// /*
    ///  * Audio engine format information
    ///  *
    ///  * Only as example shown here! See always the original structure on related header.
    ///  */
    /// typedef struct AudioEngineFormat
    /// {
    ///   enum AEDataFormat m_dataFormat;           /* The stream's data format (eg, AE_FMT_S16LE) */
    ///   unsigned int      m_sampleRate;           /* The stream's sample rate (eg, 48000) */
    ///   unsigned int      m_encodedRate;          /* The encoded streams sample rate if a bitstream, otherwise undefined */
    ///   unsigned int      m_channelCount;         /* The amount of used speaker channels */
    ///   enum AEChannel    m_channels[AE_CH_MAX];  /* The stream's channel layout */
    ///   unsigned int      m_frames;               /* The number of frames per period */
    ///   unsigned int      m_frameSamples;         /* The number of samples in one frame */
    ///   unsigned int      m_frameSize;            /* The size of one frame in bytes */
    ///
    ///   /* Function to compare the format structure with another */
    ///   bool compareFormat(const AudioEngineFormat *fmt);
    /// } AudioEngineFormat;
    /// @endcode
    ///
    /// ------------------------------------------------------------------------
    ///
    /// <b>Bit options to pass to CAELib_Stream (on Kodi by <c>IAE::MakeStream</c>)</b>
    ///
    /// | enum AEStreamOptions    | Value: | Description:
    /// |------------------------:|:------:|:-----------------------------------
    /// | AESTREAM_FORCE_RESAMPLE | 1 << 0 | Force resample even if rates match
    /// | AESTREAM_PAUSED         | 1 << 1 | Create the stream paused
    /// | AESTREAM_AUTOSTART      | 1 << 2 | Autostart the stream when enough data is buffered
    /// | AESTREAM_BYPASS_ADSP    | 1 << 3 | if this option is set the ADSP-System is bypassed and the raw stream will be passed through IAESink 
    ///
    CStream(AudioEngineFormat Format, unsigned int Options = 0);
    //----------------------------------------------------------------------------


    //============================================================================
    ///
    /// @ingroup CPP_KodiAPI_AudioEngine_CStream
    /// @brief Class Descructor
    ///
    ~CStream();
    //----------------------------------------------------------------------------


    //============================================================================
    ///
    /// @ingroup CPP_KodiAPI_AudioEngine_CStream
    /// @brief <b>Get space</b>
    ///
    /// Returns the amount of space available in the stream
    ///
    /// @return The number of bytes AddData will consume
    ///
    unsigned int GetSpace();
    //----------------------------------------------------------------------------


    //============================================================================
    ///
    /// @ingroup CPP_KodiAPI_AudioEngine_CStream
    /// @brief <b>Add data</b>
    ///
    /// Add planar or interleaved PCM data to the stream
    ///
    /// @param[in] Data array of pointers to the planes
    /// @param[in] Offset to frame in frames
    /// @param[in] Frames number of frames
    ///
    /// @return The number of frames consumed
    ///
    unsigned int AddData(uint8_t* const *Data, unsigned int Offset, unsigned int Frames);
    //----------------------------------------------------------------------------


    //============================================================================
    ///
    /// @ingroup CPP_KodiAPI_AudioEngine_CStream
    /// @brief <b>Get current delay</b>
    ///
    /// Returns the time in seconds that it will take
    /// for the next added packet to be heard from the speakers.
    ///
    /// @return seconds
    ///
    double GetDelay();
    //----------------------------------------------------------------------------


    //============================================================================
    ///
    /// @ingroup CPP_KodiAPI_AudioEngine_CStream
    /// @brief <b>Is buffering</b>
    ///
    /// Returns if the stream is buffering
    ///
    /// @return True if the stream is buffering
    ///
    bool IsBuffering();
    //----------------------------------------------------------------------------


    //============================================================================
    ///
    /// @ingroup CPP_KodiAPI_AudioEngine_CStream
    /// @brief <b>Get the cache time</b>
    ///
    /// Returns the time in seconds that it will take
    /// to underrun the cache if no sample is added.
    ///
    /// @return seconds
    ///
    double GetCacheTime();
    //----------------------------------------------------------------------------


    //============================================================================
    ///
    /// @ingroup CPP_KodiAPI_AudioEngine_CStream
    /// @brief Returns the total time in seconds of the cache
    ///
    /// @return seconds
    ///
    double GetCacheTotal();
    //----------------------------------------------------------------------------


    //============================================================================
    ///
    /// @ingroup CPP_KodiAPI_AudioEngine_CStream
    /// Pauses the stream playback
    ///
    void Pause();
    //----------------------------------------------------------------------------


    //============================================================================
    ///
    /// @ingroup CPP_KodiAPI_AudioEngine_CStream
    /// Resumes the stream after pausing
    ///
    void Resume();
    //----------------------------------------------------------------------------


    //============================================================================
    ///
    /// @ingroup CPP_KodiAPI_AudioEngine_CStream
    /// Start draining the stream
    ///
    /// @note Once called AddData will not consume more data.
    ///
    /// @param[in] Wait
    ///
    void Drain(bool Wait);
    //----------------------------------------------------------------------------


    //============================================================================
    ///
    /// @ingroup CPP_KodiAPI_AudioEngine_CStream
    ///
    /// Returns true if the is stream draining
    ///
    /// @return true if draining, otherwise false
    ///
    bool IsDraining();
    //----------------------------------------------------------------------------


    //============================================================================
    ///
    /// @ingroup CPP_KodiAPI_AudioEngine_CStream
    ///
    /// Returns true if the is stream has finished draining
    ///
    /// @return If draining finished is true, otherwise false
    ///
    bool IsDrained();
    //----------------------------------------------------------------------------


    //============================================================================
    ///
    /// @ingroup CPP_KodiAPI_AudioEngine_CStream
    ///
    /// Flush all buffers dropping the audio data
    ///
    void Flush();
    //----------------------------------------------------------------------------


    //============================================================================
    ///
    /// @ingroup CPP_KodiAPI_AudioEngine_CStream
    /// Return the stream's current volume level
    ///
    /// @return The volume level between 0.0 and 1.0
    ///
    float GetVolume();
    //----------------------------------------------------------------------------


    //============================================================================
    ///
    /// @ingroup CPP_KodiAPI_AudioEngine_CStream
    /// Set the stream's volume level
    ///
    /// @param[in] volume The new volume level between 0.0 and 1.0
    ///
    void SetVolume(float volume);
    //----------------------------------------------------------------------------


    //============================================================================
    ///
    /// @ingroup CPP_KodiAPI_AudioEngine_CStream
    /// Gets the stream's volume amplification in linear units.
    ///
    /// @return The volume amplification factor between 1.0 and 1000.0
    ///
    float GetAmplification();
    //----------------------------------------------------------------------------


    //============================================================================
    ///
    /// @ingroup CPP_KodiAPI_AudioEngine_CStream
    /// Sets the stream's volume amplification in linear units.
    ///
    /// @param[in] Amplify The volume amplification factor between 1.0 and 1000.0
    ///
    void SetAmplification(float Amplify);
    //----------------------------------------------------------------------------


    //============================================================================
    ///
    /// @ingroup CPP_KodiAPI_AudioEngine_CStream
    /// Returns the size of one audio frame in bytes (channelCount/// resolution)
    ///
    /// @return The size in bytes of one frame
    ///
    const unsigned int GetFrameSize() const;
    //----------------------------------------------------------------------------


    //============================================================================
    ///
    /// @ingroup CPP_KodiAPI_AudioEngine_CStream
    /// Returns the number of channels the stream is configured to accept
    ///
    /// @return The channel count
    ///
    const unsigned int GetChannelCount() const;
    //----------------------------------------------------------------------------


    //============================================================================
    ///
    /// @ingroup CPP_KodiAPI_AudioEngine_CStream
    /// Returns the stream's sample rate, if the stream is using a dynamic sample
    /// rate, this value will NOT reflect any changes made by calls to
    /// SetResampleRatio()
    ///
    /// @return The stream's sample rate (eg, 48000)
    ///
    const unsigned int GetSampleRate() const;
    //----------------------------------------------------------------------------


    //============================================================================
    ///
    /// @ingroup CPP_KodiAPI_AudioEngine_CStream
    /// Returns the stream's encoded sample rate if the stream is RAW
    ///
    /// @return The stream's encoded sample rate
    ///
    const unsigned int GetEncodedSampleRate() const;
    //----------------------------------------------------------------------------


    //============================================================================
    ///
    /// @ingroup CPP_KodiAPI_AudioEngine_CStream
    /// Return the data format the stream has been configured with
    ///
    /// @return The stream's data format (eg, AE_FMT_S16LE)
    ///
    const AEDataFormat GetDataFormat() const;
    //----------------------------------------------------------------------------


    //============================================================================
    ///
    /// @ingroup CPP_KodiAPI_AudioEngine_CStream
    /// Return the resample ratio
    ///
    /// @note This will return an undefined value if the stream is not resampling
    ///
    /// @return the current resample ratio or undefined if the stream is not
    ///         resampling
    ///
    double GetResampleRatio();
    //----------------------------------------------------------------------------


    //============================================================================
    ///
    /// @ingroup CPP_KodiAPI_AudioEngine_CStream
    /// Sets the resample ratio
    ///
    /// @note This function may return false if the stream is not resampling, if you
    /// wish to use this be sure to set the AESTREAM_FORCE_RESAMPLE option
    ///
    /// @param[in] ratio the new sample rate ratio, calculated by
    ///                  ((double)desiredRate / (double)GetSampleRate())
    ///
    void SetResampleRatio(double ratio);
    //----------------------------------------------------------------------------

  #ifndef DOXYGEN_SHOULD_SKIP_THIS
    IMPLEMENT_ADDON_AE_STREAM;
  #endif
  };
  /// @}

}; /* namespace AudioEngine */
}; /* namespace KodiAPI */

END_NAMESPACE()
