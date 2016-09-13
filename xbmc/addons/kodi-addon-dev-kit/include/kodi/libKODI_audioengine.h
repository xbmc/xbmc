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

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <string.h>
#include <vector>

#include "kodi_audioengine_types.h"
#ifdef BUILD_KODI_ADDON
  #include "kodi/AudioEngine/AEChannelData.h"
  #include "kodi/AudioEngine/AEChannelInfo.h"
  #include "kodi/AudioEngine/AEStreamData.h"
#else
  #include "cores/AudioEngine/Utils/AEChannelData.h"
  #include "cores/AudioEngine/Utils/AEChannelInfo.h"
  #include "cores/AudioEngine/Utils/AEStreamData.h"
#endif

#include "libXBMC_addon.h"

extern "C"
{
namespace KodiAPI
{
namespace AudioEngine
{

typedef struct CB_AudioEngineLib
{
  AEStreamHandle* (*MakeStream) (AudioEngineFormat Format, unsigned int Options);
  void (*FreeStream) (AEStreamHandle *stream);
  bool (*GetCurrentSinkFormat) (void *addonData, AudioEngineFormat *SinkFormat);

  // Audio Engine Stream definitions
  unsigned int (*AEStream_GetSpace) (void *addonData, AEStreamHandle *handle);
  unsigned int (*AEStream_AddData) (void *addonData, AEStreamHandle *handle, uint8_t* const *Data, unsigned int Offset, unsigned int Frames);
  double (*AEStream_GetDelay)(void *addonData, AEStreamHandle *handle);
  bool (*AEStream_IsBuffering)(void *addonData, AEStreamHandle *handle);
  double (*AEStream_GetCacheTime)(void *addonData, AEStreamHandle *handle);
  double (*AEStream_GetCacheTotal)(void *addonData, AEStreamHandle *handle);
  void (*AEStream_Pause)(void *addonData, AEStreamHandle *handle);
  void (*AEStream_Resume)(void *addonData, AEStreamHandle *handle);
  void (*AEStream_Drain)(void *addonData, AEStreamHandle *handle, bool Wait);
  bool (*AEStream_IsDraining)(void *addonData, AEStreamHandle *handle);
  bool (*AEStream_IsDrained)(void *addonData, AEStreamHandle *handle);
  void (*AEStream_Flush)(void *addonData, AEStreamHandle *handle);
  float (*AEStream_GetVolume)(void *addonData, AEStreamHandle *handle);
  void (*AEStream_SetVolume)(void *addonData, AEStreamHandle *handle, float Volume);
  float (*AEStream_GetAmplification)(void *addonData, AEStreamHandle *handle);
  void (*AEStream_SetAmplification)(void *addonData, AEStreamHandle *handle, float Amplify);
  const unsigned int (*AEStream_GetFrameSize)(void *addonData, AEStreamHandle *handle);
  const unsigned int (*AEStream_GetChannelCount)(void *addonData, AEStreamHandle *handle);
  const unsigned int (*AEStream_GetSampleRate)(void *addonData, AEStreamHandle *handle);
  const AEDataFormat (*AEStream_GetDataFormat)(void *addonData, AEStreamHandle *handle);
  double (*AEStream_GetResampleRatio)(void *addonData, AEStreamHandle *handle);
  void (*AEStream_SetResampleRatio)(void *addonData, AEStreamHandle *handle, double Ratio);
} CB_AudioEngineLib;

} /* namespace AudioEngine */
} /* namespace KodiAPI */
} /* extern "C" */


// Audio Engine Stream Class
class CAddonAEStream
{
public:
  CAddonAEStream(AddonCB* addon, KodiAPI::AudioEngine::CB_AudioEngineLib* callbacks, AEStreamHandle* streamHandle)
    : m_Handle(addon),
      m_cb(callbacks),
      m_StreamHandle(streamHandle) {}

  ~CAddonAEStream()
  {
    if (m_StreamHandle)
    {
      m_cb->FreeStream(m_StreamHandle);
      m_StreamHandle = nullptr;
    }
  }

  /**
  * Returns the amount of space available in the stream
  * @return The number of bytes AddData will consume
  */
  unsigned int GetSpace()
  {
    return m_cb->AEStream_GetSpace(m_Handle->addonData, m_StreamHandle);
  }

  /**
  * Add planar or interleaved PCM data to the stream
  * @param Data array of pointers to the planes
  * @param Offset to frame in frames
  * @param Frames number of frames
  * @return The number of frames consumed
  */
  unsigned int AddData(uint8_t* const *Data, unsigned int Offset, unsigned int Frames)
  {
    return m_cb->AEStream_AddData(m_Handle->addonData, m_StreamHandle, Data, Offset, Frames);
  }

  /**
  * Returns the time in seconds that it will take
  * for the next added packet to be heard from the speakers.
  * @return seconds
  */
  double GetDelay()
  {
    return m_cb->AEStream_GetDelay(m_Handle->addonData, m_StreamHandle);
  }

  /**
  * Returns if the stream is buffering
  * @return True if the stream is buffering
  */
  bool IsBuffering()
  {
    return m_cb->AEStream_IsBuffering(m_Handle->addonData, m_StreamHandle);
  }

  /**
   * Returns the time in seconds of the stream's
   * cached audio samples. Engine buffers excluded.
  * @return seconds
  */
  double GetCacheTime()
  {
    return m_cb->AEStream_GetCacheTime(m_Handle->addonData, m_StreamHandle);
  }

  /**
  * Returns the total time in seconds of the cache
  * @return seconds
  */
  double GetCacheTotal()
  {
    return m_cb->AEStream_GetCacheTotal(m_Handle->addonData, m_StreamHandle);
  }

  /**
  * Pauses the stream playback
  */
  void Pause()
  {
    return m_cb->AEStream_Pause(m_Handle->addonData, m_StreamHandle);
  }

  /**
  * Resumes the stream after pausing
  */
  void Resume()
  {
    return m_cb->AEStream_Resume(m_Handle->addonData, m_StreamHandle);
  }

  /**
  * Start draining the stream
  * @note Once called AddData will not consume more data.
  */
  void Drain(bool Wait)
  {
    return m_cb->AEStream_Drain(m_Handle->addonData, m_StreamHandle, Wait);
  }

  /**
  * Returns true if the is stream draining
  */
  bool IsDraining()
  {
    return m_cb->AEStream_IsDraining(m_Handle->addonData, m_StreamHandle);
  }

  /**
  * Returns true if the is stream has finished draining
  */
  bool IsDrained()
  {
    return m_cb->AEStream_IsDrained(m_Handle->addonData, m_StreamHandle);
  }

  /**
  * Flush all buffers dropping the audio data
  */
  void Flush()
  {
    return m_cb->AEStream_Flush(m_Handle->addonData, m_StreamHandle);
  }

  /**
  * Return the stream's current volume level
  * @return The volume level between 0.0 and 1.0
  */
  float GetVolume()
  {
    return m_cb->AEStream_GetVolume(m_Handle->addonData, m_StreamHandle);
  }

  /**
  * Set the stream's volume level
  * @param volume The new volume level between 0.0 and 1.0
  */
  void  SetVolume(float Volume)
  {
    return m_cb->AEStream_SetVolume(m_Handle->addonData, m_StreamHandle, Volume);
  }

  /**
  * Gets the stream's volume amplification in linear units.
  * @return The volume amplification factor between 1.0 and 1000.0
  */
  float GetAmplification()
  {
    return m_cb->AEStream_GetAmplification(m_Handle->addonData, m_StreamHandle);
  }

  /**
  * Sets the stream's volume amplification in linear units.
  * @param The volume amplification factor between 1.0 and 1000.0
  */
  void SetAmplification(float Amplify)
  {
    return m_cb->AEStream_SetAmplification(m_Handle->addonData, m_StreamHandle, Amplify);
  }

  /**
  * Returns the size of one audio frame in bytes (channelCount * resolution)
  * @return The size in bytes of one frame
  */
  const unsigned int GetFrameSize() const
  {
    return m_cb->AEStream_GetFrameSize(m_Handle->addonData, m_StreamHandle);
  }

  /**
  * Returns the number of channels the stream is configured to accept
  * @return The channel count
  */
  const unsigned int GetChannelCount() const
  {
    return m_cb->AEStream_GetChannelCount(m_Handle->addonData, m_StreamHandle);
  }

  /**
  * Returns the stream's sample rate, if the stream is using a dynamic sample rate, this value will NOT reflect any changes made by calls to SetResampleRatio()
  * @return The stream's sample rate (eg, 48000)
  */
  const unsigned int GetSampleRate() const
  {
    return m_cb->AEStream_GetSampleRate(m_Handle->addonData, m_StreamHandle);
  }

  /**
  * Return the data format the stream has been configured with
  * @return The stream's data format (eg, AE_FMT_S16LE)
  */
  const AEDataFormat GetDataFormat() const
  {
    return m_cb->AEStream_GetDataFormat(m_Handle->addonData, m_StreamHandle);
  }

  /**
  * Return the resample ratio
  * @note This will return an undefined value if the stream is not resampling
  * @return the current resample ratio or undefined if the stream is not resampling
  */
  double GetResampleRatio()
  {
    return m_cb->AEStream_GetResampleRatio(m_Handle->addonData, m_StreamHandle);
  }

  /**
  * Sets the resample ratio
  * @note This function may return false if the stream is not resampling, if you wish to use this be sure to set the AESTREAM_FORCE_RESAMPLE option
  * @param ratio the new sample rate ratio, calculated by ((double)desiredRate / (double)GetSampleRate())
  */
  void SetResampleRatio(double Ratio)
  {
    m_cb->AEStream_SetResampleRatio(m_Handle->addonData, m_StreamHandle, Ratio);
  }

private:
  AddonCB* m_Handle;
  KodiAPI::AudioEngine::CB_AudioEngineLib *m_cb;
  AEStreamHandle  *m_StreamHandle;
};

class CHelper_libKODI_audioengine
{
public:
  CHelper_libKODI_audioengine(void)
  {
    m_Handle = nullptr;
    m_Callbacks = nullptr;
  }

  ~CHelper_libKODI_audioengine(void)
  {
    if (m_Handle && m_Callbacks)
    {
      m_Handle->AudioEngineLib_UnRegisterMe(m_Handle->addonData, m_Callbacks);
    }
  }

  /*!
   * @brief Resolve all callback methods
   * @param handle Pointer to the add-on
   * @return True when all methods were resolved, false otherwise.
   */
  bool RegisterMe(void* handle)
  {
    m_Handle = static_cast<AddonCB*>(handle);
    if (m_Handle)
      m_Callbacks = (KodiAPI::AudioEngine::CB_AudioEngineLib*)m_Handle->AudioEngineLib_RegisterMe(m_Handle->addonData);
    if (!m_Callbacks)
      fprintf(stderr, "libKODI_audioengine-ERROR: AudioEngineLib_RegisterMe can't get callback table from Kodi !!!\n");

    return m_Callbacks != nullptr;
  }

  /**
   * Creates and returns a new handle to an IAEStream in the format specified, this function should never fail
   * @param DataFormat The data format the incoming audio will be in (eg, AE_FMT_S16LE)
   * @param SampleRate The sample rate of the audio data (eg, 48000)
   * @param ChannelLayout The order of the channels in the audio data
   * @param Options A bit field of stream options (see: enum AEStreamOptions)
   * @return a new Handle to an IAEStream that will accept data in the requested format
   */
  CAddonAEStream* MakeStream(AudioEngineFormat Format, unsigned int Options = 0)
  {
    AEStreamHandle *streamHandle = m_Callbacks->MakeStream(Format, Options);
    if (!streamHandle)
    {
      fprintf(stderr, "libKODI_audioengine-ERROR: AudioEngine_make_stream MakeStream failed!\n");
      return nullptr;
    }

    return new CAddonAEStream(m_Handle, m_Callbacks, streamHandle);
  }

  /**
  * This method will remove the specifyed stream from the engine.
  * For OSX/IOS this is essential to reconfigure the audio output.
  * @param stream The stream to be altered
  * @return NULL
  */
  void FreeStream(CAddonAEStream **Stream)
  {
    delete *Stream;
    *Stream = nullptr;
  }

  /**
   * Get the current sink data format
   *
   * @param Current sink data format. For more details see AudioEngineFormat.
   * @return Returns true on success, else false.
   */
  bool GetCurrentSinkFormat(AudioEngineFormat &SinkFormat)
  {
    return m_Callbacks->GetCurrentSinkFormat(m_Handle->addonData, &SinkFormat);
  }

private:
  AddonCB* m_Handle;
  KodiAPI::AudioEngine::CB_AudioEngineLib *m_Callbacks;
};
