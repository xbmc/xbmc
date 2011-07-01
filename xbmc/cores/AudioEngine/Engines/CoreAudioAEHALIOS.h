/*
 *      Copyright (C) 2005-2009 Team XBMC
 *      http://www.xbmc.org
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

#ifndef __COREAUDIOAEHALIOS_H__
#define __COREAUDIOAEHALIOS_H__

#ifdef __arm__

#include "ICoreAudioAEHAL.h"

#include <AudioUnit/AudioUnit.h>
#include <AudioToolbox/AudioToolbox.h>
#include <AudioToolbox/AudioServices.h>
#include <CoreAudio/CoreAudioTypes.h>
#include <list>
#include <vector>

#include "utils/StdString.h"

#define kOutputBus 0
#define kInputBus 1

// Forward declarations
class CCoreAudioAE;

typedef std::list<AudioComponentInstance> IOSCoreAudioDeviceList;

// There is only one AudioSystemObject instance system-side.
// Therefore, all CIOSCoreAudioHardware methods are static
class CIOSCoreAudioHardware
{
public:
  static AudioComponentInstance FindAudioDevice(CStdString deviceName);
  static AudioComponentInstance GetDefaultOutputDevice();
  static UInt32 GetOutputDevices(IOSCoreAudioDeviceList* pList);
};

class CIOSCoreAudioDevice
{
public:
  CIOSCoreAudioDevice();
  virtual ~CIOSCoreAudioDevice();
  
  AudioComponentInstance GetId() {return m_AudioUnit;}
  const char* GetName(CStdString& name);
  CIOSCoreAudioDevice(AudioComponentInstance deviceId);
  UInt32 GetTotalOutputChannels();
  
  void Attach(AudioUnit audioUnit) {m_AudioUnit = audioUnit;}
  AudioComponentInstance GetComponent(){return m_AudioUnit;}
  
  void SetupInfo();
  bool Init(bool bPassthrough, AudioStreamBasicDescription* pDesc, AURenderCallback renderCallback, void *pClientData);
  bool Open();  
  void Close();
  void Start();
  void Stop();
  
  bool EnableInput(AudioComponentInstance componentInstance, AudioUnitElement bus);
  bool EnableOutput(AudioComponentInstance componentInstance, AudioUnitElement bus);
  bool GetFormat(AudioComponentInstance componentInstance, AudioUnitScope scope,
                 AudioUnitElement bus, AudioStreamBasicDescription* pDesc);
  bool SetFormat(AudioComponentInstance componentInstance, AudioUnitScope scope,
                 AudioUnitElement bus, AudioStreamBasicDescription* pDesc);
  Float32 GetCurrentVolume();
  bool SetCurrentVolume(Float32 vol);
  int  FramesPerSlice(int nSlices);
  void AudioChannelLayout(int layoutTag);
  bool SetRenderProc(AudioComponentInstance componentInstance, AudioUnitElement bus,
                     AURenderCallback callback, void* pClientData);
  bool SetSessionListener(AudioSessionPropertyID inID, 
                          AudioSessionPropertyListener inProc, void* pClientData);
  
  AudioComponentInstance m_AudioUnit;
  AudioComponentInstance m_MixerUnit;
  bool m_Passthrough;
};

class CCoreAudioAEHALIOS : public ICoreAudioAEHAL
{
protected:
  CIOSCoreAudioDevice  *m_AudioDevice;
  bool                  m_Initialized;
  bool                  m_Passthrough;
public:

  AEAudioFormat     m_format;
  unsigned int      m_BytesPerFrame;
  unsigned int      m_BytesPerSec;
  unsigned int      m_NumLatencyFrames;
  unsigned int      m_OutputBufferIndex;
  CCoreAudioAE     *m_ae;

  CCoreAudioAEHALIOS();
  virtual ~CCoreAudioAEHALIOS();

  virtual bool  Initialize(IAE *ae, bool passThrough, AEAudioFormat &format, CStdString &device);
  virtual void  Deinitialize();
  virtual void  EnumerateOutputDevices(AEDeviceList &devices, bool passthrough);
  virtual void  Stop();
  virtual bool  Start();
  virtual float GetDelay();
};

#endif
#endif
