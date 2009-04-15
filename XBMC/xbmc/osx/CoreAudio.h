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

#include <AudioUnit/AudioUnit.h>

#include <StdString.h>
#include <list>
#include <vector>

// Forward declarations
class CCoreAudioHardware;
class CCoreAudioDevice;
class CCoreAudioStream;
class CCoreAudioUnit;

typedef std::list<AudioDeviceID> CoreAudioDeviceList;

// Not yet implemented
// kAudioHardwarePropertyDevices                     
// kAudioHardwarePropertyDefaultInputDevice              
// kAudioHardwarePropertyDefaultSystemOutputDevice   
// kAudioHardwarePropertyDeviceForUID                          

// There is only one AudioSystemObject instance system-side.
// Therefore, all CCoreAudioHardware methods are static
class CCoreAudioHardware
{
public:
  static AudioDeviceID FindAudioDevice(CStdString deviceName);
  static AudioDeviceID GetDefaultOutputDevice();
  static UInt32 GetOutputDevices(CoreAudioDeviceList* pList);
  static bool GetAutoHogMode();
  static void SetAutoHogMode(bool enable);
};

// Not yet implemented
// kAudioDevicePropertyDeviceIsRunning, kAudioDevicePropertyDeviceIsRunningSomewhere, kAudioDevicePropertyHogMode, kAudioDevicePropertyLatency, 
// kAudioDevicePropertyBufferFrameSize, kAudioDevicePropertyBufferFrameSizeRange, kAudioDevicePropertyUsesVariableBufferFrameSizes,
// kAudioDevicePropertyStreams, kAudioDevicePropertySafetyOffset, kAudioDevicePropertyIOCycleUsage, kAudioDevicePropertyStreamConfiguration
// kAudioDevicePropertyIOProcStreamUsage, kAudioDevicePropertyPreferredChannelsForStereo, kAudioDevicePropertyPreferredChannelLayout,
// kAudioDevicePropertyNominalSampleRate, kAudioDevicePropertyAvailableNominalSampleRates, kAudioDevicePropertyActualSampleRate,
// kAudioDevicePropertyTransportType

typedef std::list<AudioStreamID> AudioStreamIdList;
typedef std::vector<SInt32> CoreAudioChannelList;
typedef std::list<UInt32> CoreAudioDataSourceList;

class CCoreAudioDevice
{
public:
  CCoreAudioDevice();
  CCoreAudioDevice(AudioDeviceID deviceId) {Open(deviceId);}
  virtual ~CCoreAudioDevice();
  
  bool Open(AudioDeviceID deviceId);
  void Close();
  
  void Start();
  void Stop();
  bool AddIOProc(AudioDeviceIOProc ioProc, void* pCallbackData);
  void RemoveIOProc();
  
  AudioDeviceID GetId() {return m_DeviceId;}
  const char* GetName(CStdString& name);
  UInt32 GetTotalOutputChannels();
  bool GetStreams(AudioStreamIdList* pList);
  bool SetHogStatus(bool hog);
  bool GetHogStatus();
  bool SetMixingSupport(bool mix);
  bool GetMixingSupport();
  bool GetPreferredChannelLayout(CoreAudioChannelList* pChannelMap);
  bool GetDataSources(CoreAudioDataSourceList* pList);
  Float64 GetNominalSampleRate();
  bool SetNominalSampleRate(Float64 sampleRate);
protected:
  AudioDeviceID m_DeviceId;
  pid_t m_Hog;
  int m_MixerRestore;
  AudioDeviceIOProc m_IoProc;
  Float64 m_SampleRateRestore;
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
  AudioStreamBasicDescription m_OriginalPhysicalFormat;  
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
    bool IsInitialized() {return m_Initialized;}
    bool SetRenderProc(AURenderCallback callback, void* pClientData);
    UInt32 GetBufferFrameSize();
    bool SetMaxFramesPerSlice(UInt32 maxFrames);
    
    bool GetInputFormat(AudioStreamBasicDescription* pDesc);
    bool GetOutputFormat(AudioStreamBasicDescription* pDesc);    
    bool SetInputFormat(AudioStreamBasicDescription* pDesc);
    bool SetOutputFormat(AudioStreamBasicDescription* pDesc);
    bool GetInputChannelMap(CoreAudioChannelList* pChannelMap);
    bool SetInputChannelMap(CoreAudioChannelList* pChannelMap);
    
    void Start();
    void Stop();
    Float32 GetCurrentVolume();
    bool SetCurrentVolume(Float32 vol);
    
  protected:
    AudioUnit m_Component;
    bool m_Initialized;
  };

// Helper Function
char* UInt32ToFourCC(UInt32* val);
const char* StreamDescriptionToString(AudioStreamBasicDescription desc, CStdString& str);

#endif // __COREAUDIO_H__