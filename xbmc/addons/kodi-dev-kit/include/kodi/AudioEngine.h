/*
 *  Copyright (C) 2005-2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "AddonBase.h"
#include "c-api/audio_engine.h"

#ifdef __cplusplus

namespace kodi
{
namespace audioengine
{

//¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
// Main page text for audio engine group by Doxygen.
//{{{

//==============================================================================
///
/// @defgroup cpp_kodi_audioengine  Interface - kodi::audioengine
/// @ingroup cpp
/// @brief **Audio engine functions**\n
/// This interface contains auxiliary functions and classes which allow an addon
/// to play their own individual audio stream in Kodi.
///
/// Using @ref cpp_kodi_audioengine_CAEStream "kodi::audioengine::CAEStream",
/// a class can be created in this regard, about which the necessary stream data and
/// information are given to Kodi.
///
/// Via @ref kodi::audioengine::GetCurrentSinkFormat(), the audio formats currently
/// processed in Kodi can be called up beforehand in order to adapt your own stream
/// to them.
///
/// However, the created stream can also differ from this because Kodi changes
/// it to suit it.
///
///
/// ------------------------------------------------------------------------
///
/// **Example:**
/// ~~~~~~~~~~~~~{.cpp}
///
/// #include <kodi/AudioEngine.h>
///
/// ...
///
/// kodi::audioengine::AudioEngineFormat format;
/// if (!kodi::audioengine::GetCurrentSinkFormat(format))
///   return false;
///
/// format.SetDataFormat(AUDIOENGINE_FMT_FLOATP);
/// format.SetChannelLayout(std::vector<AudioEngineChannel>(AUDIOENGINE_CH_FL, AUDIOENGINE_CH_FR));
///
/// unsigned int myUsedSampleRate = format.GetSampleRate();
///
/// ...
///
/// kodi::audioengine::CAEStream* stream = new kodi::audioengine::CAEStream(format, AUDIO_STREAM_AUTOSTART);
///
/// ~~~~~~~~~~~~~
///
/// ------------------------------------------------------------------------
///
/// It has the header @ref AudioEngine.h "#include <kodi/AudioEngine.h>" be included
/// to enjoy it.
///
//------------------------------------------------------------------------------

//==============================================================================
///
/// @defgroup cpp_kodi_audioengine_Defs Definitions, structures and enumerators
/// @ingroup cpp_kodi_audioengine
/// @brief **Library definition values**\n
/// All audio engine functions associated data structures.
///
//------------------------------------------------------------------------------

//}}}

//¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
// "C++" related audio engine definitions
//{{{

//==============================================================================
/// @defgroup cpp_kodi_audioengine_Defs_AudioEngineFormat class AudioEngineFormat
/// @ingroup cpp_kodi_audioengine_Defs
/// @brief **Audio format structure**\n
/// The audio format structure that fully defines a stream's audio
/// information.
///
/// With the help of this format information, Kodi adjusts its processing
/// accordingly.
///
///@{
class ATTRIBUTE_HIDDEN AudioEngineFormat
  : public addon::CStructHdl<AudioEngineFormat, AUDIO_ENGINE_FORMAT>
{
public:
  /*! \cond PRIVATE */
  AudioEngineFormat()
  {
    m_cStructure->m_dataFormat = AUDIOENGINE_FMT_INVALID;
    m_cStructure->m_sampleRate = 0;
    m_cStructure->m_encodedRate = 0;
    m_cStructure->m_frames = 0;
    m_cStructure->m_frameSize = 0;
    m_cStructure->m_channelCount = 0;

    for (size_t ch = 0; ch < AUDIOENGINE_CH_MAX; ++ch)
      m_cStructure->m_channels[ch] = AUDIOENGINE_CH_NULL;
  }
  AudioEngineFormat(const AudioEngineFormat& channel) : CStructHdl(channel) {}
  AudioEngineFormat(const AUDIO_ENGINE_FORMAT* channel) : CStructHdl(channel) {}
  AudioEngineFormat(AUDIO_ENGINE_FORMAT* channel) : CStructHdl(channel) {}
  /*! \endcond */

  /// @defgroup cpp_kodi_audioengine_Defs_AudioEngineFormat_Help *Value Help*
  /// @ingroup cpp_kodi_audioengine_Defs_AudioEngineFormat
  ///
  /// <b>The following table contains values that can be set with @ref cpp_kodi_audioengine_Defs_AudioEngineFormat :</b>
  /// | Name | Type | Set call | Get call
  /// |------|------|----------|----------
  /// | **Data format**, see @ref AudioEngineDataFormat for available types | enum | @ref AudioEngineFormat::SetDataFormat "SetDataFormat" | @ref AudioEngineFormat::GetDataFormat "GetDataFormat"
  /// | **Sample rate** | unsigned int | @ref AudioEngineFormat::SetSampleRate "SetSampleRate" | @ref AudioEngineFormat::GetSampleRate "GetSampleRate"
  /// | **Encoded rate** | unsigned int | @ref AudioEngineFormat::SetEncodedRate "SetEncodedRate" | @ref AudioEngineFormat::GetEncodedRate "GetEncodedRate"
  /// | **Channel layout**, see @ref AudioEngineChannel for available types | std::vector<enum AudioEngineChannel> | @ref AudioEngineFormat::SetChannelLayout "SetChannelLayout" | @ref AudioEngineFormat::GetChannelLayout "GetChannelLayout"
  /// | **Frames amount** | unsigned int | @ref AudioEngineFormat::SetFramesAmount "SetFramesAmount" | @ref AudioEngineFormat::GetFramesAmount "GetFramesAmount"
  /// | **Frame size** | unsigned int | @ref AudioEngineFormat::SetFrameSize "SetFrameSize" | @ref AudioEngineFormat::GetFrameSize "GetFrameSize"
  ///
  /// Further is @ref AudioEngineFormat::CompareFormat "CompareFormat" included to compare this class with another.
  ///

  /// @addtogroup cpp_kodi_audioengine_Defs_AudioEngineFormat
  /// @copydetails cpp_kodi_audioengine_Defs_AudioEngineFormat_Help
  ///@{

  /// @brief The stream's data format (eg, AUDIOENGINE_FMT_S16LE)
  void SetDataFormat(enum AudioEngineDataFormat format) { m_cStructure->m_dataFormat = format; }

  /// @brief To get with @ref SetDataFormat changed values.
  enum AudioEngineDataFormat GetDataFormat() const { return m_cStructure->m_dataFormat; }

  /// @brief The stream's sample rate (eg, 48000)
  void SetSampleRate(unsigned int rate) { m_cStructure->m_sampleRate = rate; }

  /// @brief To get with @ref SetSampleRate changed values.
  unsigned int GetSampleRate() const { return m_cStructure->m_sampleRate; }

  /// @brief The encoded streams sample rate if a bitstream, otherwise undefined
  void SetEncodedRate(unsigned int rate) { m_cStructure->m_encodedRate = rate; }

  /// @brief To get with @ref SetEncodedRate changed values.
  unsigned int GetEncodedRate() const { return m_cStructure->m_encodedRate; }

  /// @brief The stream's channel layout
  void SetChannelLayout(const std::vector<enum AudioEngineChannel>& layout)
  {
    // Reset first all to empty values to AUDIOENGINE_CH_NULL, in case given list is empty
    m_cStructure->m_channelCount = 0;
    for (size_t ch = 0; ch < AUDIOENGINE_CH_MAX; ++ch)
      m_cStructure->m_channels[ch] = AUDIOENGINE_CH_NULL;

    for (size_t ch = 0; ch < layout.size() && ch < AUDIOENGINE_CH_MAX; ++ch)
    {
      m_cStructure->m_channels[ch] = layout[ch];
      m_cStructure->m_channelCount++;
    }
  }

  /// @brief To get with @ref SetChannelLayout changed values.
  std::vector<enum AudioEngineChannel> GetChannelLayout() const
  {
    std::vector<enum AudioEngineChannel> channels;
    for (size_t ch = 0; ch < AUDIOENGINE_CH_MAX; ++ch)
    {
      if (m_cStructure->m_channels[ch] == AUDIOENGINE_CH_NULL)
        break;

      channels.push_back(m_cStructure->m_channels[ch]);
    }
    return channels;
  }

  /// @brief The number of frames per period
  void SetFramesAmount(unsigned int frames) { m_cStructure->m_frames = frames; }

  /// @brief To get with @ref SetFramesAmount changed values.
  unsigned int GetFramesAmount() const { return m_cStructure->m_frames; }

  /// @brief The size of one frame in bytes
  void SetFrameSize(unsigned int frameSize) { m_cStructure->m_frameSize = frameSize; }

  /// @brief To get with @ref SetFrameSize changed values.
  unsigned int GetFrameSize() const { return m_cStructure->m_frameSize; }

  /// @brief Function to compare the format structure with another
  bool CompareFormat(const AudioEngineFormat* fmt)
  {
    if (!fmt)
    {
      return false;
    }

    if (m_cStructure->m_dataFormat != fmt->m_cStructure->m_dataFormat ||
        m_cStructure->m_sampleRate != fmt->m_cStructure->m_sampleRate ||
        m_cStructure->m_encodedRate != fmt->m_cStructure->m_encodedRate ||
        m_cStructure->m_frames != fmt->m_cStructure->m_frames ||
        m_cStructure->m_frameSize != fmt->m_cStructure->m_frameSize ||
        m_cStructure->m_channelCount != fmt->m_cStructure->m_channelCount)
    {
      return false;
    }

    for (unsigned int ch = 0; ch < AUDIOENGINE_CH_MAX; ++ch)
    {
      if (fmt->m_cStructure->m_channels[ch] != m_cStructure->m_channels[ch])
      {
        return false;
      }
    }

    return true;
  }
  ///@}
};
///@}
//----------------------------------------------------------------------------

//}}}

//¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
// "C++" AudioEngine addon interface
//{{{

//============================================================================
///
/// @defgroup cpp_kodi_audioengine_CAEStream class CAEStream
/// @ingroup cpp_kodi_audioengine
/// @brief **Audio Engine Stream Class**\n
/// Class that can be created by the addon in order to be able to transfer
/// audiostream data processed on the addon to Kodi and output it audibly.
///
/// This can create individually several times and performed in different
/// processes simultaneously.
///
/// It has the header @ref AudioEngine.h "#include <kodi/AudioEngine.h>" be
/// included to enjoy it.
///
//----------------------------------------------------------------------------
class ATTRIBUTE_HIDDEN CAEStream
{
public:
  //==========================================================================
  /// @ingroup cpp_kodi_audioengine_CAEStream
  /// @brief Contructs new class to an Kodi IAEStream in the format specified.
  ///
  /// @param[in] format       The data format the incoming audio will be in
  ///                         (e.g. @ref AUDIOENGINE_FMT_S16LE)
  /// @param[in] options      [opt] A bit field of stream options (see: enum @ref AudioEngineStreamOptions)
  ///
  ///
  /// ------------------------------------------------------------------------
  ///
  /// @copydetails cpp_kodi_audioengine_Defs_AudioEngineFormat_Help
  ///
  /// ------------------------------------------------------------------------
  ///
  /// **Bit options to pass (on Kodi by <c>IAE::MakeStream</c>)**
  ///
  /// | enum AEStreamOptions        | Value: | Description:
  /// |----------------------------:|:------:|:-----------------------------------
  /// | AUDIO_STREAM_FORCE_RESAMPLE | 1 << 0 | Force resample even if rates match
  /// | AUDIO_STREAM_PAUSED         | 1 << 1 | Create the stream paused
  /// | AUDIO_STREAM_AUTOSTART      | 1 << 2 | Autostart the stream when enough data is buffered
  ///
  ///
  /// ------------------------------------------------------------------------
  ///
  /// **Example:**
  /// ~~~~~~~~~~~~~{.cpp}
  ///
  /// #include <kodi/AudioEngine.h>
  ///
  /// ...
  ///
  /// kodi::audioengine::AudioEngineFormat format;
  ///
  /// format.SetDataFormat(AUDIOENGINE_FMT_FLOATP); /* The stream's data format (eg, AUDIOENGINE_FMT_S16LE) */
  /// format.SetChannelLayout(std::vector<AudioEngineChannel>(AUDIOENGINE_CH_FL, AUDIOENGINE_CH_FR)); /* The stream's channel layout */
  /// format.SetSampleRate(48000); /* The stream's sample rate (eg, 48000) */
  /// format.SetFrameSize(sizeof(float)*2); /* The size of one frame in bytes */
  /// format.SetFramesAmount(882); /* The number of samples in one frame */
  ///
  /// kodi::audioengine::CAEStream* stream = new kodi::audioengine::CAEStream(format, AUDIO_STREAM_AUTOSTART);
  ///
  /// ~~~~~~~~~~~~~
  ///
  CAEStream(AudioEngineFormat& format, unsigned int options = 0)
    : m_kodiBase(::kodi::addon::CAddonBase::m_interface->toKodi->kodiBase),
      m_cb(::kodi::addon::CAddonBase::m_interface->toKodi->kodi_audioengine)
  {
    m_StreamHandle = m_cb->make_stream(m_kodiBase, format, options);
    if (m_StreamHandle == nullptr)
    {
      kodi::Log(ADDON_LOG_FATAL, "CAEStream: make_stream failed!");
    }
  }
  //--------------------------------------------------------------------------

  //==========================================================================
  /// @ingroup cpp_kodi_audioengine_CAEStream
  /// @brief Class destructor.
  ///
  ~CAEStream()
  {
    if (m_StreamHandle)
    {
      m_cb->free_stream(m_kodiBase, m_StreamHandle);
      m_StreamHandle = nullptr;
    }
  }
  //--------------------------------------------------------------------------

  //==========================================================================
  /// @ingroup cpp_kodi_audioengine_CAEStream
  /// @brief Returns the amount of space available in the stream.
  ///
  /// @return The number of bytes AddData will consume
  ///
  unsigned int GetSpace() { return m_cb->aestream_get_space(m_kodiBase, m_StreamHandle); }
  //--------------------------------------------------------------------------

  //==========================================================================
  /// @ingroup cpp_kodi_audioengine_CAEStream
  /// @brief Add planar or interleaved PCM data to the stream.
  ///
  /// @param[in] data array of pointers to the planes
  /// @param[in] offset to frame in frames
  /// @param[in] frames number of frames
  /// @param[in] pts [opt] presentation timestamp, default is 0
  /// @param[in] hasDownmix [opt] set true if downmix is present, default is false
  /// @param[in] centerMixLevel [opt] level to mix left and right to center default is 1.0
  /// @return The number of frames consumed
  ///
  unsigned int AddData(uint8_t* const* data,
                       unsigned int offset,
                       unsigned int frames,
                       double pts = 0,
                       bool hasDownmix = false,
                       double centerMixLevel = 1.0)
  {
    return m_cb->aestream_add_data(m_kodiBase, m_StreamHandle, data, offset, frames, pts,
                                   hasDownmix, centerMixLevel);
  }
  //--------------------------------------------------------------------------

  //==========================================================================
  /// @ingroup cpp_kodi_audioengine_CAEStream
  /// @brief Returns the time in seconds that it will take for the next added
  /// packet to be heard from the speakers.
  ///
  /// @return seconds
  ///
  double GetDelay() { return m_cb->aestream_get_delay(m_kodiBase, m_StreamHandle); }
  //--------------------------------------------------------------------------

  //==========================================================================
  /// @ingroup cpp_kodi_audioengine_CAEStream
  /// @brief Returns if the stream is buffering.
  ///
  /// @return True if the stream is buffering
  ///
  bool IsBuffering() { return m_cb->aestream_is_buffering(m_kodiBase, m_StreamHandle); }
  //--------------------------------------------------------------------------

  //==========================================================================
  /// @ingroup cpp_kodi_audioengine_CAEStream
  /// @brief Returns the time in seconds of the stream's cached audio samples.
  /// Engine buffers excluded.
  ///
  /// @return seconds
  ///
  double GetCacheTime() { return m_cb->aestream_get_cache_time(m_kodiBase, m_StreamHandle); }
  //--------------------------------------------------------------------------

  //==========================================================================
  /// @ingroup cpp_kodi_audioengine_CAEStream
  /// @brief Returns the total time in seconds of the cache.
  ///
  /// @return seconds
  ///
  double GetCacheTotal() { return m_cb->aestream_get_cache_total(m_kodiBase, m_StreamHandle); }
  //--------------------------------------------------------------------------

  //==========================================================================
  /// @ingroup cpp_kodi_audioengine_CAEStream
  /// @brief Pauses the stream playback.
  ///
  void Pause() { return m_cb->aestream_pause(m_kodiBase, m_StreamHandle); }
  //--------------------------------------------------------------------------

  //==========================================================================
  /// @ingroup cpp_kodi_audioengine_CAEStream
  /// @brief Resumes the stream after pausing
  ///
  void Resume() { return m_cb->aestream_resume(m_kodiBase, m_StreamHandle); }
  //--------------------------------------------------------------------------

  //==========================================================================
  /// @ingroup cpp_kodi_audioengine_CAEStream
  /// @brief Start draining the stream.
  ///
  /// @param[in] wait [opt] Wait until drain is finished if set to true,
  ///                 otherwise it returns direct
  ///
  /// @note Once called AddData will not consume more data.
  ///
  void Drain(bool wait = true) { return m_cb->aestream_drain(m_kodiBase, m_StreamHandle, wait); }
  //--------------------------------------------------------------------------

  //==========================================================================
  /// @ingroup cpp_kodi_audioengine_CAEStream
  /// @brief Returns true if the is stream draining.
  ///
  bool IsDraining() { return m_cb->aestream_is_draining(m_kodiBase, m_StreamHandle); }
  //--------------------------------------------------------------------------

  //==========================================================================
  /// @ingroup cpp_kodi_audioengine_CAEStream
  /// @brief Returns true if the is stream has finished draining.
  ///
  bool IsDrained() { return m_cb->aestream_is_drained(m_kodiBase, m_StreamHandle); }
  //--------------------------------------------------------------------------

  //==========================================================================
  /// @ingroup cpp_kodi_audioengine_CAEStream
  /// @brief Flush all buffers dropping the audio data.
  ///
  void Flush() { return m_cb->aestream_flush(m_kodiBase, m_StreamHandle); }
  //--------------------------------------------------------------------------

  //==========================================================================
  /// @ingroup cpp_kodi_audioengine_CAEStream
  /// @brief Return the stream's current volume level.
  ///
  /// @return The volume level between 0.0 and 1.0
  ///
  float GetVolume() { return m_cb->aestream_get_volume(m_kodiBase, m_StreamHandle); }
  //--------------------------------------------------------------------------

  //==========================================================================
  /// @ingroup cpp_kodi_audioengine_CAEStream
  /// @brief Set the stream's volume level.
  ///
  /// @param[in] volume               The new volume level between 0.0 and 1.0
  ///
  void SetVolume(float volume)
  {
    return m_cb->aestream_set_volume(m_kodiBase, m_StreamHandle, volume);
  }
  //--------------------------------------------------------------------------

  //==========================================================================
  /// @ingroup cpp_kodi_audioengine_CAEStream
  /// @brief Gets the stream's volume amplification in linear units.
  ///
  /// @return The volume amplification factor between 1.0 and 1000.0
  ///
  float GetAmplification() { return m_cb->aestream_get_amplification(m_kodiBase, m_StreamHandle); }
  //--------------------------------------------------------------------------

  //==========================================================================
  /// @ingroup cpp_kodi_audioengine_CAEStream
  /// @brief Sets the stream's volume amplification in linear units.
  ///
  /// @param[in] amplify The volume amplification factor between 1.0 and 1000.0
  ///
  void SetAmplification(float amplify)
  {
    return m_cb->aestream_set_amplification(m_kodiBase, m_StreamHandle, amplify);
  }
  //--------------------------------------------------------------------------

  //==========================================================================
  /// @ingroup cpp_kodi_audioengine_CAEStream
  /// @brief Returns the size of one audio frame in bytes (channelCount * resolution).
  ///
  /// @return The size in bytes of one frame
  ///
  unsigned int GetFrameSize() const
  {
    return m_cb->aestream_get_frame_size(m_kodiBase, m_StreamHandle);
  }
  //--------------------------------------------------------------------------

  //==========================================================================
  /// @ingroup cpp_kodi_audioengine_CAEStream
  /// @brief Returns the number of channels the stream is configured to accept.
  ///
  /// @return The channel count
  ///
  unsigned int GetChannelCount() const
  {
    return m_cb->aestream_get_channel_count(m_kodiBase, m_StreamHandle);
  }
  //--------------------------------------------------------------------------

  //==========================================================================
  /// @ingroup cpp_kodi_audioengine_CAEStream
  /// @brief Returns the stream's sample rate, if the stream is using a dynamic
  /// sample rate, this value will NOT reflect any changes made by calls to
  /// SetResampleRatio().
  ///
  /// @return The stream's sample rate (eg, 48000)
  ///
  unsigned int GetSampleRate() const
  {
    return m_cb->aestream_get_sample_rate(m_kodiBase, m_StreamHandle);
  }
  //--------------------------------------------------------------------------

  //==========================================================================
  /// @ingroup cpp_kodi_audioengine_CAEStream
  /// @brief Return the data format the stream has been configured with.
  ///
  /// @return The stream's data format (eg, AUDIOENGINE_FMT_S16LE)
  ///
  AudioEngineDataFormat GetDataFormat() const
  {
    return m_cb->aestream_get_data_format(m_kodiBase, m_StreamHandle);
  }
  //--------------------------------------------------------------------------

  //==========================================================================
  /// @ingroup cpp_kodi_audioengine_CAEStream
  /// @brief Return the resample ratio.
  ///
  /// @note This will return an undefined value if the stream is not resampling.
  ///
  /// @return the current resample ratio or undefined if the stream is not resampling
  ///
  double GetResampleRatio()
  {
    return m_cb->aestream_get_resample_ratio(m_kodiBase, m_StreamHandle);
  }
  //--------------------------------------------------------------------------

  //==========================================================================
  /// @ingroup cpp_kodi_audioengine_CAEStream
  /// @brief Sets the resample ratio.
  ///
  /// @note This function may return false if the stream is not resampling, if
  /// you wish to use this be sure to set the AESTREAM_FORCE_RESAMPLE option.
  ///
  /// @param[in] ratio the new sample rate ratio, calculated by
  ///                  ((double)desiredRate / (double)GetSampleRate())
  ///
  void SetResampleRatio(double ratio)
  {
    m_cb->aestream_set_resample_ratio(m_kodiBase, m_StreamHandle, ratio);
  }
  //--------------------------------------------------------------------------

private:
  void* m_kodiBase;
  AddonToKodiFuncTable_kodi_audioengine* m_cb;
  AEStreamHandle* m_StreamHandle;
};
//----------------------------------------------------------------------------

//============================================================================
/// @ingroup cpp_kodi_audioengine
/// @brief Get the current sink data format.
///
/// @param[in] format Current sink data format. For more details see AudioEngineFormat.
/// @return Returns true on success, else false.
///
///
/// ------------------------------------------------------------------------
///
/// **Example:**
/// ~~~~~~~~~~~~~{.cpp}
///
/// #include <kodi/AudioEngine.h>
///
/// ...
///
/// kodi::audioengine::AudioEngineFormat format;
/// if (!kodi::audioengine::GetCurrentSinkFormat(format))
///   return false;
///
/// std::vector<AudioEngineChannel> layout = format.GetChannelLayout();
///
/// ...
/// return true;
///
/// ~~~~~~~~~~~~~
///
inline bool ATTRIBUTE_HIDDEN GetCurrentSinkFormat(AudioEngineFormat& format)
{
  using namespace kodi::addon;
  return CAddonBase::m_interface->toKodi->kodi_audioengine->get_current_sink_format(
      CAddonBase::m_interface->toKodi->kodiBase, format);
}
//----------------------------------------------------------------------------

//}}}

} // namespace audioengine
} // namespace kodi

#endif /* __cplusplus */
