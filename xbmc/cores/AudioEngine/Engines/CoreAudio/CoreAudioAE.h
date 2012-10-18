#pragma once
/*
 *      Copyright (C) 2011-2012 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <list>
#include <map>

#include "system.h"

#include "Interfaces/AE.h"
#include "ICoreAudioAEHAL.h"
#include "ICoreAudioSource.h"
#include "CoreAudioAEStream.h"
#include "CoreAudioAESound.h"
#include "threads/CriticalSection.h"

#if defined(TARGET_DARWIN_IOS)
#include "CoreAudioAEHALIOS.h"
#else
#include "CoreAudioAEHALOSX.h"
#endif

#define COREAUDIO_IS_RAW(x) \
(                           \
  (x) == AE_FMT_AC3   ||    \
  (x) == AE_FMT_DTS   ||    \
  (x) == AE_FMT_LPCM  ||    \
  (x) == AE_FMT_EAC3  ||    \
  (x) == AE_FMT_DTSHD ||    \
  (x) == AE_FMT_TRUEHD      \
)

#if defined(TARGET_DARWIN_IOS)
# define CCoreAudioAEHAL CCoreAudioAEHALIOS
#else
# define CCoreAudioAEHAL CCoreAudioAEHALOSX
#endif

class CCoreAudioAEStream;
class CCoreAudioAESound;
class CCoreAudioAEEventThread;

class CCoreAudioAE : public IAE, public ICoreAudioSource
{
protected:
  friend class CAEFactory;
  CCoreAudioAE();
  virtual ~CCoreAudioAE();

  // Give the HAL access to the engine
  friend class CCoreAudioAEHAL;
  CCoreAudioAEHAL  *HAL;

public:
  virtual void      Shutdown();

  virtual bool      Initialize();
  virtual void      OnSettingsChange(const std::string& setting);

  virtual bool      Suspend(); /* Suspend output and de-initialize "hog-mode" sink for external players and power savings */
  virtual bool      Resume();  /* Resume ouput and re-initialize sink after Suspend() above */
  virtual bool      IsSuspended(); /* Returns true if in Suspend mode - used by players */

  unsigned int      GetSampleRate();
  unsigned int      GetEncodedSampleRate();
  CAEChannelInfo    GetChannelLayout();
  unsigned int      GetChannelCount();
  enum AEDataFormat GetDataFormat();
  AEAudioFormat     GetAudioFormat();

  virtual double    GetDelay();
  virtual float     GetVolume();
  virtual void      SetVolume(float volume);
  virtual void      SetMute(const bool enabled);
  virtual bool      IsMuted();
  virtual void      SetSoundMode(const int mode);


  virtual bool      SupportsRaw();

  CCoreAudioAEHAL*  GetHAL();

  // returns a new stream for data in the specified format
  virtual IAEStream* MakeStream(enum AEDataFormat dataFormat,
    unsigned int sampleRate,unsigned int encodedSamplerate,
    CAEChannelInfo channelLayout, unsigned int options = 0);

  virtual IAEStream* FreeStream(IAEStream *stream);

  // returns a new sound object
  virtual IAESound* MakeSound(const std::string& file);
  void              StopAllSounds();
  virtual void      FreeSound(IAESound *sound);
  virtual void      PlaySound(IAESound *sound);
  virtual void      StopSound(IAESound *sound);
  void              MixSounds(float *buffer, unsigned int samples);

  // free's sounds that have expired
  virtual void      GarbageCollect();

  virtual void      EnumerateOutputDevices(AEDeviceList &devices, bool passthrough);

  virtual OSStatus  Render(AudioUnitRenderActionFlags* actionFlags,
    const AudioTimeStamp* pTimeStamp, UInt32 busNumber,
    UInt32 frameCount, AudioBufferList* pBufList);


private:
  CCriticalSection  m_callbackLock;
  CCriticalSection  m_streamLock;
  CCriticalSection  m_soundLock;
  CCriticalSection  m_soundSampleLock;

  // currently playing sounds
  typedef struct {
    CCoreAudioAESound *owner;
    float             *samples;
    unsigned int      sampleCount;
  } SoundState;

  typedef std::list<CCoreAudioAEStream*> StreamList;
  typedef std::list<CCoreAudioAESound* > SoundList;
  typedef std::list<SoundState         > SoundStateList;

  StreamList        m_streams;
  SoundList         m_sounds;
  SoundStateList    m_playing_sounds;

  // Prevent multiple init/deinit
  bool              m_Initialized;
  bool              m_callbackRunning;

  AEAudioFormat     m_format;
  unsigned int      m_chLayoutCount;
  bool              m_rawPassthrough;

  enum AEStdChLayout m_stdChLayout;

  bool              OpenCoreAudio(unsigned int sampleRate, bool forceRaw, enum AEDataFormat rawDataFormat);
  void              Deinitialize();
  void              Start();
  void              Stop();

  OSStatus          OnRender(AudioUnitRenderActionFlags *actionFlags,
                      const AudioTimeStamp *inTimeStamp, UInt32 inBusNumber,
                      UInt32 inNumberFrames, AudioBufferList *ioData);

  float             m_volume;
  float             m_volumeBeforeMute;
  bool              m_muted;
  int               m_soundMode;
  bool              m_streamsPlaying;
  bool              m_isSuspended;
};
