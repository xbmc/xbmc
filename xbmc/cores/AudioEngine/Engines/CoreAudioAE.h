/*
 *      Copyright (C) 2005-2010 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#ifndef __COREAUDIOAE_H__
#define __COREAUDIOAE_H__

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

#define COREAUDIO_IS_RAW(x) ((x) == AE_FMT_AC3 || (x) == AE_FMT_DTS || (x) == AE_FMT_LPCM || (x) == AE_FMT_EAC3 || (x) == AE_FMT_DTSHD || (x) == AE_FMT_TRUEHD)

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

  /* Give the HAL access to the engine */
  friend class CCoreAudioAEHAL;
  CCoreAudioAEHAL  *HAL;
  
public:
  virtual void Shutdown();

  virtual bool  Initialize();
  virtual void  OnSettingsChange(std::string setting);

  unsigned int      GetSampleRate();
  unsigned int      GetEncodedSampleRate();
  CAEChannelInfo    GetChannelLayout();
  unsigned int      GetChannelCount();
  enum AEDataFormat GetDataFormat();
  AEAudioFormat     GetAudioFormat();

  virtual double GetDelay();
  virtual float GetVolume();
  virtual void  SetVolume(float volume);
  virtual void  SetMute(const bool enabled);
  virtual bool  IsMuted();
  

  virtual bool SupportsRaw();
  
  CCoreAudioAEHAL  *GetHAL();
  
  /* returns a new stream for data in the specified format */
  virtual IAEStream *MakeStream(enum AEDataFormat dataFormat, 
                               unsigned int sampleRate,
                               unsigned int encodedSamplerate,
                               CAEChannelInfo channelLayout, 
                               unsigned int options = 0);
  
  virtual IAEStream *FreeStream(IAEStream *stream);
    
  /* returns a new sound object */
  virtual IAESound *MakeSound(const std::string& file);
  void StopAllSounds();
  virtual void FreeSound(IAESound *sound);
  virtual void PlaySound(IAESound *sound);
  virtual void StopSound(IAESound *sound);
  void MixSounds(float *buffer, unsigned int samples);

  /* free's sounds that have expired */
  virtual void GarbageCollect();

  virtual void EnumerateOutputDevices(AEDeviceList &devices, bool passthrough);

  virtual OSStatus Render(AudioUnitRenderActionFlags* actionFlags, 
                          const AudioTimeStamp* pTimeStamp, 
                          UInt32 busNumber, 
                          UInt32 frameCount, 
                          AudioBufferList* pBufList);
  

private:
  CCriticalSection  m_callbackLock;
  CCriticalSection  m_streamLock;
  CCriticalSection  m_soundLock;
  CCriticalSection  m_soundSampleLock;
  
  /* currently playing sounds */
  typedef struct {
    CCoreAudioAESound *owner;
    float        *samples;
    unsigned int  sampleCount;
  } SoundState;

  typedef std::list<CCoreAudioAEStream*> StreamList;
  typedef std::list<CCoreAudioAESound* > SoundList;
  typedef std::list<SoundState         > SoundStateList;

  StreamList     m_streams;
  SoundList      m_sounds;
  SoundStateList m_playing_sounds;
  
  bool              m_Initialized; // Prevent multiple init/deinit
  bool              m_callbackRunning;
    
  AEAudioFormat     m_format;
  unsigned int      m_chLayoutCount;
  bool              m_rawPassthrough;

  enum AEStdChLayout m_stdChLayout;
  
  bool OpenCoreAudio(unsigned int sampleRate = 44100, bool forceRaw = false, enum AEDataFormat rawDataFormat = AE_FMT_AC3);
  
  void Deinitialize();
  void Start();
  void Stop();

  OSStatus OnRender(AudioUnitRenderActionFlags *actionFlags, 
                    const AudioTimeStamp *inTimeStamp, 
                    UInt32 inBusNumber, 
                    UInt32 inNumberFrames, 
                    AudioBufferList *ioData);
  float m_volume;
  float m_volumeBeforeMute;  
  bool  m_muted;
};
#endif
