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

#define AUDIOENGINE_HELPER_DLL KODI_DLL("audioengine")
#define AUDIOENGINE_HELPER_DLL_NAME KODI_DLL_NAME("audioengine")

class CAddonAEStream;

class CHelper_libKODI_audioengine
{
public:
  CHelper_libKODI_audioengine(void)
  {
    m_libKODI_audioengine = NULL;
    m_Handle              = NULL;
  }

  ~CHelper_libKODI_audioengine(void)
  {
    if (m_libKODI_audioengine)
    {
      AudioEngine_unregister_me(m_Handle, m_Callbacks);
      dlclose(m_libKODI_audioengine);
    }
  }

  /*!
   * @brief Resolve all callback methods
   * @param handle Pointer to the add-on
   * @return True when all methods were resolved, false otherwise.
   */
  bool RegisterMe(void* handle)
  {
    m_Handle = handle;

    std::string libBasePath;
    libBasePath  = ((cb_array*)m_Handle)->libPath;
    libBasePath += AUDIOENGINE_HELPER_DLL;

    m_libKODI_audioengine = dlopen(libBasePath.c_str(), RTLD_LAZY);
    if (m_libKODI_audioengine == NULL)
    {
      fprintf(stderr, "Unable to load %s\n", dlerror());
      return false;
    }

    AudioEngine_register_me = (void* (*)(void *HANDLE))
      dlsym(m_libKODI_audioengine, "AudioEngine_register_me");
    if (AudioEngine_register_me == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    AudioEngine_unregister_me = (void(*)(void* HANDLE, void* CB))
      dlsym(m_libKODI_audioengine, "AudioEngine_unregister_me");
    if (AudioEngine_unregister_me == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    AudioEngine_MakeStream = (CAddonAEStream* (*)(void*, void*, AudioEngineFormat, unsigned int))
      dlsym(m_libKODI_audioengine, "AudioEngine_make_stream");
    if (AudioEngine_MakeStream == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    AudioEngine_FreeStream = (void(*)(CAddonAEStream*))
      dlsym(m_libKODI_audioengine, "AudioEngine_free_stream");
    if (AudioEngine_FreeStream == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    AudioEngine_GetCurrentSinkFormat = (bool(*)(void*, void*, AudioEngineFormat*))
      dlsym(m_libKODI_audioengine, "AudioEngine_get_current_sink_Format");
    if (AudioEngine_GetCurrentSinkFormat == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    m_Callbacks = AudioEngine_register_me(m_Handle);
    return m_Callbacks != NULL;
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
    return AudioEngine_MakeStream(m_Handle, m_Callbacks, Format, Options);
  }

  /**
  * This method will remove the specifyed stream from the engine.
  * For OSX/IOS this is essential to reconfigure the audio output.
  * @param stream The stream to be altered
  * @return NULL
  */
  void FreeStream(CAddonAEStream **Stream)
  {
    AudioEngine_FreeStream(*Stream);
    *Stream = NULL;
  }

  /**
   * Get the current sink data format
   *
   * @param Current sink data format. For more details see AudioEngineFormat.
   * @return Returns true on success, else false.
   */
  bool GetCurrentSinkFormat(AudioEngineFormat &SinkFormat)
  {
    return AudioEngine_GetCurrentSinkFormat(m_Handle, m_Callbacks, &SinkFormat);
  }

protected:
  void* (*AudioEngine_register_me)(void*);
  void (*AudioEngine_unregister_me)(void*, void*);
  CAddonAEStream* (*AudioEngine_MakeStream)(void*, void*, AudioEngineFormat, unsigned int);
  bool (*AudioEngine_GetCurrentSinkFormat)(void*, void*, AudioEngineFormat *SinkFormat);
  void (*AudioEngine_FreeStream)(CAddonAEStream*);

private:
  void* m_libKODI_audioengine;
  void* m_Handle;
  void* m_Callbacks;
  struct cb_array
  {
    const char* libPath;
  };
};

// Audio Engine Stream Class
class CAddonAEStream
{
public:
  CAddonAEStream(void *Addon, void *Callbacks, AEStreamHandle *StreamHandle);
  virtual ~CAddonAEStream();

  /**
  * Returns the amount of space available in the stream
  * @return The number of bytes AddData will consume
  */
  virtual unsigned int GetSpace();

  /**
  * Add planar or interleaved PCM data to the stream
  * @param Data array of pointers to the planes
  * @param Offset to frame in frames
  * @param Frames number of frames
  * @return The number of frames consumed
  */
  virtual unsigned int AddData(uint8_t* const *Data, unsigned int Offset, unsigned int Frames);

  /**
  * Returns the time in seconds that it will take
  * for the next added packet to be heard from the speakers.
  * @return seconds
  */
  virtual double GetDelay();

  /**
  * Returns if the stream is buffering
  * @return True if the stream is buffering
  */
  virtual bool IsBuffering();

  /**
   * Returns the time in seconds of the stream's
   * cached audio samples. Engine buffers excluded.
  * @return seconds
  */
  virtual double GetCacheTime();

  /**
  * Returns the total time in seconds of the cache
  * @return seconds
  */
  virtual double GetCacheTotal();

  /**
  * Pauses the stream playback
  */
  virtual void Pause();

  /**
  * Resumes the stream after pausing
  */
  virtual void Resume();

  /**
  * Start draining the stream
  * @note Once called AddData will not consume more data.
  */
  virtual void Drain(bool Wait);

  /**
  * Returns true if the is stream draining
  */
  virtual bool IsDraining();

  /**
  * Returns true if the is stream has finished draining
  */
  virtual bool IsDrained();

  /**
  * Flush all buffers dropping the audio data
  */
  virtual void Flush();

  /**
  * Return the stream's current volume level
  * @return The volume level between 0.0 and 1.0
  */
  virtual float GetVolume();

  /**
  * Set the stream's volume level
  * @param volume The new volume level between 0.0 and 1.0
  */
  virtual void  SetVolume(float Volume);

  /**
  * Gets the stream's volume amplification in linear units.
  * @return The volume amplification factor between 1.0 and 1000.0
  */
  virtual float GetAmplification();

  /**
  * Sets the stream's volume amplification in linear units.
  * @param The volume amplification factor between 1.0 and 1000.0
  */
  virtual void SetAmplification(float Amplify);

  /**
  * Returns the size of one audio frame in bytes (channelCount * resolution)
  * @return The size in bytes of one frame
  */
  virtual const unsigned int GetFrameSize() const;

  /**
  * Returns the number of channels the stream is configured to accept
  * @return The channel count
  */
  virtual const unsigned int GetChannelCount() const;

  /**
  * Returns the stream's sample rate, if the stream is using a dynamic sample rate, this value will NOT reflect any changes made by calls to SetResampleRatio()
  * @return The stream's sample rate (eg, 48000)
  */
  virtual const unsigned int GetSampleRate() const;

  /**
  * Return the data format the stream has been configured with
  * @return The stream's data format (eg, AE_FMT_S16LE)
  */
  virtual const AEDataFormat GetDataFormat() const;

  /**
  * Return the resample ratio
  * @note This will return an undefined value if the stream is not resampling
  * @return the current resample ratio or undefined if the stream is not resampling
  */
  virtual double GetResampleRatio();

  /**
  * Sets the resample ratio
  * @note This function may return false if the stream is not resampling, if you wish to use this be sure to set the AESTREAM_FORCE_RESAMPLE option
  * @param ratio the new sample rate ratio, calculated by ((double)desiredRate / (double)GetSampleRate())
  */
  virtual void SetResampleRatio(double Ratio);

  private:
    AEStreamHandle  *m_StreamHandle;
    void            *m_Callbacks;
    void            *m_AddonHandle;
};
