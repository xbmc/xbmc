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

#ifndef __COREAUDIO_H__
#define __COREAUDIO_H__

#include <CoreServices/CoreServices.h>
#include <AudioUnit/AudioUnit.h>
#include <list>

// Forward declarations
class CCoreAudioHardware;
class CCoreAudioDevice;
class CCoreAudioStream;
class CCoreAudioUnit;

typedef std::list<CCoreAudioDevice> CoreAudioDeviceList;

// kAudioHardwarePropertyDevices                     
// kAudioHardwarePropertyDefaultInputDevice          
// kAudioHardwarePropertyDefaultOutputDevice         
// kAudioHardwarePropertyDefaultSystemOutputDevice   
// kAudioHardwarePropertyDeviceForUID                
// kAudioHardwarePropertyHogModeIsAllowed            

class CCoreAudioHardware
{
public:
  AudioDeviceID FindAudioDevice(CStdString deviceName);
  AudioDeviceID GetDefaultOutputDevice();
};

// kAudioDevicePropertyDeviceIsRunning, kAudioDevicePropertyDeviceIsRunningSomewhere, kAudioDevicePropertyHogMode, kAudioDevicePropertyLatency, 
// kAudioDevicePropertyBufferFrameSize, kAudioDevicePropertyBufferFrameSizeRange, kAudioDevicePropertyUsesVariableBufferFrameSizes,
// kAudioDevicePropertyStreams, kAudioDevicePropertySafetyOffset, kAudioDevicePropertyIOCycleUsage, kAudioDevicePropertyStreamConfiguration
// kAudioDevicePropertyIOProcStreamUsage, kAudioDevicePropertyPreferredChannelsForStereo, kAudioDevicePropertyPreferredChannelLayout,
// kAudioDevicePropertyNominalSampleRate, kAudioDevicePropertyAvailableNominalSampleRates, kAudioDevicePropertyActualSampleRate,
// kAudioDevicePropertyTransportType

typedef std::list<AudioStreamID> AudioStreamIdList;

class CCoreAudioDevice
{
public:
  CCoreAudioDevice();
  virtual ~CCoreAudioDevice();
  
  bool Open(AudioDeviceID deviceId);
  void Close();
  
  AudioDeviceID GetId() {return m_DeviceId;}
  bool GetStreams(AudioStreamIdList* pList);
  bool SetHogStatus(bool hog);
  bool GetHogStatus();
  bool SetMixingSupport(bool mix);
  bool GetMixingSupport();
  
protected:
  AudioDeviceID m_DeviceId;
  pid_t m_Hog;
  int m_MixerRestore;
  
};

typedef std::list<AudioStreamRangedDescription> StreamFormatList;

class CCoreAudioStream
{
public:
  CCoreAudioStream();
  virtual ~CCoreAudioStream();
  
  bool Open(AudioStreamID streamId);
  void Close();
  
  AudioStreamID GetId() {return m_StreamId;}
  UInt32 GetDirection();
  UInt32 GetTerminalType();
  UInt32 GetLatency(); // Returns number of frames
  bool GetVirtualFormat(AudioStreamBasicDescription* pDesc);
  bool GetPhysicalFormat(AudioStreamBasicDescription* pDesc);
  bool SetVirtualFormat(AudioStreamBasicDescription* pDesc);
  bool SetPhysicalFormat(AudioStreamBasicDescription* pDesc);
  bool GetAvailableVirtualFormats(StreamFormatList* pList);
  bool GetAvailablePhysicalFormats(StreamFormatList* pList);
  
protected:
  AudioStreamID m_StreamId;
  AudioStreamBasicDescription m_OriginalVirtualFormat;  
};

class CCoreAudioUnit
  {
  public:
    CCoreAudioUnit();
    virtual ~CCoreAudioUnit();
    
    bool Open(ComponentDescription desc);
    void Close();
    bool SetCurrentDevice(AudioDeviceID deviceId);
    bool Initialize();
    bool SetRenderProc(AURenderCallback callback, void* pClientData);
    UInt32 GetBufferFrameSize();
    bool SetMaxFramesPerSlice(UInt32 maxFrames);
    
    bool GetInputFormat(AudioStreamBasicDescription* pDesc);
    bool GetOutputFormat(AudioStreamBasicDescription* pDesc);    
    bool SetInputFormat(AudioStreamBasicDescription* pDesc);
    bool SetOutputFormat(AudioStreamBasicDescription* pDesc);    
    
    void Start();
    void Stop();
    Float32 GetCurrentVolume();
    bool SetCurrentVolume(Float32 vol);
    
  protected:
    AudioUnit m_Component;
    bool m_Initialized;
  };

  // Helper Function
void UInt32ToFourCC(char* fourCC, Uint32 val); // fourCC must be at least 5 BYTES wide

#endif // __COREAUDIO_H__