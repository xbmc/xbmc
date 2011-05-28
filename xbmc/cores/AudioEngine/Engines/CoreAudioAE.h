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

#include "AE.h"
#include "CoreAudioAEStream.h"
#include "CoreAudioAESound.h"
#include "threads/XBMC_mutex.h"

#ifdef __arm__
#include "CoreAudioAEHALIOS.h"
#else
#include "CoreAudioAEHALOSX.h"
#endif

#define COREAUDIO_IS_RAW(x) ((x) == AE_FMT_AC3 || (x) == AE_FMT_DTS || (x) == AE_FMT_EAC3)

class CCoreAudioAEStream;
class CCoreAudioAESound;

class CCoreAudioAE : public IAE
{
protected:
	/* Give the HAL access to the engine */
#ifdef __arm__
	friend class CCoreAudioAEHALIOS;
	CCoreAudioAEHALIOS	*HAL;
#else
	friend class CCoreAudioAEHALOSX;
	CCoreAudioAEHALOSX	*HAL;
#endif
	
public:
  /* this should NEVER be called directly, use CAEFactory */
  CCoreAudioAE();
  virtual ~CCoreAudioAE();

  virtual bool  Initialize();
  virtual void  OnSettingsChange(CStdString setting);

  virtual unsigned int      GetSampleRate();
  virtual AEChLayout        GetChannelLayout();
  virtual unsigned int      GetChannelCount();
  virtual enum AEDataFormat GetDataFormat();
  virtual AEAudioFormat     GetAudioFormat();

  virtual float GetDelay();
  virtual float GetVolume();
  virtual void  SetVolume(float volume);

  virtual bool SupportsRaw();
  
  /* returns a new stream for data in the specified format */
  virtual IAEStream *GetStream(enum AEDataFormat dataFormat, 
                               unsigned int sampleRate, 
                               unsigned int channelCount, 
                               AEChLayout channelLayout, 
                               unsigned int options = 0);
  
  virtual IAEStream *AlterStream(IAEStream *stream, 
                                 enum AEDataFormat dataFormat, 
                                 unsigned int sampleRate, 
                                 unsigned int channelCount, 
                                 AEChLayout channelLayout, 
                                 unsigned int options = 0);

	virtual void RemoveStream(IAEStream *stream);
	
  virtual IAEStream *FreeStream(IAEStream *stream);
  
  /* returns a new sound object */
  virtual IAESound *GetSound(CStdString file);
  virtual void FreeSound(IAESound *sound);
  virtual void PlaySound(IAESound *sound);
  virtual void StopSound(IAESound *sound);
  void MixSounds(float *buffer, unsigned int samples);

  /* free's sounds that have expired */
  virtual void GarbageCollect();

  virtual void EnumerateOutputDevices(AEDeviceList &devices, bool passthrough);

#ifdef __SSE__
  inline static void SSEMulAddArray(float *data, float *add, const float mul, uint32_t count);
  inline static void SSEMulArray   (float *data, const float mul, uint32_t count);
#endif

  void LockEngine();
  void UnlockEngine();
  
private:
  SDL_mutex *m_MutexLockEngine;
  bool       m_EngineLock;
  SDL_mutex *m_Mutex;
  SDL_cond*  m_callbackCond;
  
  std::list<CCoreAudioAEStream*> m_streams;
  std::list<CCoreAudioAESound* > m_sounds;
  /* currently playing sounds */
  typedef struct {
    CCoreAudioAESound *owner;
    float        *samples;
    unsigned int  sampleCount;
  } SoundState;
  std::list<SoundState> m_playing_sounds;
  
  bool              m_Initialized; // Prevent multiple init/deinit
		
  AEAudioFormat     m_format;
  bool              m_rawPassthrough;
  
  float             *m_OutputBuffer;
  int               m_OutputBufferSize;
  uint8_t          *m_StreamBuffer;
  int               m_StreamBufferSize;
  
  enum AEChannel    *m_RemapChannelLayout;
  
  bool OpenCoreAudio(unsigned int sampleRate = 44100, bool forceRaw = false, enum AEDataFormat rawFormat = AE_FMT_AC3);
	
  void Deinitialize();
  void Start();
  void Stop();
  
  void ReorderSmpteToCA(void *buf, uint frames, AEDataFormat dataFormat);

  OSStatus OnRenderCallback(AudioUnitRenderActionFlags *ioActionFlags, 
                            const AudioTimeStamp *inTimeStamp, 
                            UInt32 inBusNumber, 
                            UInt32 inNumberFrames, 
                            AudioBufferList *ioData);
  
  static OSStatus RenderCallback(void *inRefCon, 
                                 AudioUnitRenderActionFlags *ioActionFlags, 
                                 const AudioTimeStamp *inTimeStamp, 
                                 UInt32 inBusNumber, 
                                 UInt32 inNumberFrames, 
                                 AudioBufferList *ioData);
  
#ifndef __arm__  
  OSStatus OnRenderCallbackDirect( AudioDeviceID inDevice,
                                  const AudioTimeStamp * inNow,
                                  const void * inInputData,
                                  const AudioTimeStamp * inInputTime,
                                  AudioBufferList * outOutputData,
                                  const AudioTimeStamp * inOutputTime,
                                  void * threadGlobals );
  static OSStatus RenderCallbackDirect(AudioDeviceID inDevice, 
                                       const AudioTimeStamp* inNow, 
                                       const AudioBufferList* inInputData, 
                                       const AudioTimeStamp* inInputTime, 
                                       AudioBufferList* outOutputData, 
                                       const AudioTimeStamp* inOutputTime, 
                                       void* inClientData);
#endif  
  float m_volume;
};

// Helper Functions
char* UInt32ToFourCC(UInt32* val);
const char* StreamDescriptionToString(AudioStreamBasicDescription desc, CStdString& str);
void CheckOutputBufferSize(void **buffer, int *oldSize, int newSize);

#define CONVERT_OSSTATUS(x) UInt32ToFourCC((UInt32*)&ret)

#endif
