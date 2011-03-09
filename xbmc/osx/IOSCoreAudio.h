/*
 *      Copyright (C) 2010 Team XBMC
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

#ifndef __COREAUDIO_H__
#define __COREAUDIO_H__

#if defined(__APPLE__)
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
class CIOSCoreAudioHardware;
class CIOSCoreAudioDevice;
class CIOSCoreAudioUnit;

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

// Helper Functions
char* IOSUInt32ToFourCC(UInt32* val);
const char* IOSStreamDescriptionToString(AudioStreamBasicDescription desc, CStdString& str);

#define CONVERT_OSSTATUS(x) IOSUInt32ToFourCC((UInt32*)&ret)

#endif
#endif // __COREAUDIO_H__
