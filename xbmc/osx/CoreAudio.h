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

#if !defined(__arm__)
#include "utils/StdString.h"

#include <AudioUnit/AudioUnit.h>
#include <AudioToolbox/AudioToolbox.h>
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
// kAudioDevicePropertyDeviceIsRunning, kAudioDevicePropertyDeviceIsRunningSomewhere, kAudioDevicePropertyLatency, 
// kAudioDevicePropertyBufferFrameSize, kAudioDevicePropertyBufferFrameSizeRange, kAudioDevicePropertyUsesVariableBufferFrameSizes,
// kAudioDevicePropertySafetyOffset, kAudioDevicePropertyIOCycleUsage, kAudioDevicePropertyStreamConfiguration
// kAudioDevicePropertyIOProcStreamUsage, kAudioDevicePropertyPreferredChannelsForStereo, kAudioDevicePropertyPreferredChannelLayout,
// kAudioDevicePropertyAvailableNominalSampleRates, kAudioDevicePropertyActualSampleRate,
// kAudioDevicePropertyTransportType

typedef std::list<AudioStreamID> AudioStreamIdList;
typedef std::list<UInt32> CoreAudioDataSourceList;
typedef std::vector<SInt32> CoreAudioChannelList;

class CCoreAudioChannelLayout
{
public:
  CCoreAudioChannelLayout();
  CCoreAudioChannelLayout(AudioChannelLayout& layout);
  virtual ~CCoreAudioChannelLayout();
  operator AudioChannelLayout*() {return m_pLayout;}
  bool CopyLayout(AudioChannelLayout& layout);
  bool SetLayout(AudioChannelLayoutTag layoutTag);
  UInt32 GetChannelCount();
  AudioChannelLabel GetChannelLabel(UInt32 index);
  static UInt32 GetChannelCountForLayout(AudioChannelLayout& layout);
  static const char* ChannelLabelToString(UInt32 label);
  static const char* ChannelLayoutToString(AudioChannelLayout& layout, CStdString& str);
protected:
  AudioChannelLayout* m_pLayout;
};

class CCoreAudioDevice
{
public:
  CCoreAudioDevice();
  CCoreAudioDevice(AudioDeviceID deviceId);
  virtual ~CCoreAudioDevice();
  
  bool Open(AudioDeviceID deviceId);
  void Close();
  
  void Start();
  void Stop();
  bool AddIOProc(AudioDeviceIOProc ioProc, void* pCallbackData);
  void RemoveIOProc();
  
  AudioDeviceID GetId() {return m_DeviceId;}
  const char* GetName(CStdString& name);
  const char* GetName();
  UInt32 GetTotalOutputChannels();
  bool GetStreams(AudioStreamIdList* pList);
  bool IsRunning();
  bool SetHogStatus(bool hog);
  pid_t GetHogStatus();
  bool SetMixingSupport(bool mix);
  bool GetMixingSupport();
  bool GetPreferredChannelLayout(CCoreAudioChannelLayout& layout);
  bool GetDataSources(CoreAudioDataSourceList* pList);
  Float64 GetNominalSampleRate();
  bool SetNominalSampleRate(Float64 sampleRate);
  UInt32 GetNumLatencyFrames();
  UInt32 GetBufferSize();
  bool SetBufferSize(UInt32 size);
protected:
  AudioDeviceID m_DeviceId;
  bool m_Started;
  pid_t m_Hog;
  int m_MixerRestore;
  AudioDeviceIOProc m_IoProc;
  Float64 m_SampleRateRestore;
  UInt32 m_BufferSizeRestore;
  CStdString m_Name;
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
  UInt32 GetNumLatencyFrames();
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

class ICoreAudioSource
{
public:
  virtual ~ICoreAudioSource() {};
  // Function to request rendered data from a data source
  virtual OSStatus Render(AudioUnitRenderActionFlags* actionFlags, const AudioTimeStamp* pTimeStamp, UInt32 busNumber, UInt32 frameCount, AudioBufferList* pBufList) = 0;
};

typedef std::list<AudioChannelLayoutTag> AudioChannelLayoutList;

class CCoreAudioUnit
{
public:
  CCoreAudioUnit();
  virtual ~CCoreAudioUnit();
  
  virtual bool Open(ComponentDescription desc);
  virtual bool Open(OSType type, OSType subType, OSType manufacturer);
  virtual void Attach(AudioUnit audioUnit) {m_Component = audioUnit;}
  virtual AudioUnit GetComponent(){return m_Component;}
  virtual void Close();
  virtual bool Initialize();
  virtual bool IsInitialized() {return m_Initialized;}
  virtual bool SetInputSource(ICoreAudioSource* pSource);
  virtual bool GetInputFormat(AudioStreamBasicDescription* pDesc);
  virtual bool GetOutputFormat(AudioStreamBasicDescription* pDesc);    
  virtual bool SetInputFormat(AudioStreamBasicDescription* pDesc);
  virtual bool SetOutputFormat(AudioStreamBasicDescription* pDesc);
  virtual bool SetMaxFramesPerSlice(UInt32 maxFrames);
  virtual bool GetSupportedChannelLayouts(AudioChannelLayoutList* pLayouts);
protected:
  bool SetRenderProc(AURenderCallback callback, void* pClientData);
  static OSStatus RenderCallback(void *inRefCon, AudioUnitRenderActionFlags *ioActionFlags, const AudioTimeStamp *inTimeStamp, UInt32 inBusNumber, UInt32 inNumberFrames, AudioBufferList *ioData);
  virtual OSStatus OnRender(AudioUnitRenderActionFlags *ioActionFlags, const AudioTimeStamp *inTimeStamp, UInt32 inBusNumber, UInt32 inNumberFrames, AudioBufferList *ioData);
  
  ICoreAudioSource* m_pSource;
  AudioUnit m_Component;
  bool m_Initialized;
};

class CAUGenericSource : public CCoreAudioUnit, public ICoreAudioSource
{
public:
  CAUGenericSource();
  virtual ~CAUGenericSource();
  virtual OSStatus Render(AudioUnitRenderActionFlags* actionFlags, const AudioTimeStamp* pTimeStamp, UInt32 busNumber, UInt32 frameCount, AudioBufferList* pBufList);
};

class CAUOutputDevice : public CCoreAudioUnit
{
public:
  CAUOutputDevice();
  virtual ~CAUOutputDevice();
  bool SetCurrentDevice(AudioDeviceID deviceId);
  bool GetChannelMap(CoreAudioChannelList* pChannelMap);
  bool SetChannelMap(CoreAudioChannelList* pChannelMap);
  UInt32 GetBufferFrameSize();
  
  void Start();
  void Stop();
  bool IsRunning();
  
  Float32 GetCurrentVolume();
  bool SetCurrentVolume(Float32 vol);  
protected:
  virtual OSStatus OnRender(AudioUnitRenderActionFlags *ioActionFlags, const AudioTimeStamp *inTimeStamp, UInt32 inBusNumber, UInt32 inNumberFrames, AudioBufferList *ioData);
};

class CAUMatrixMixer : public CAUGenericSource
{
public:
  CAUMatrixMixer();
  virtual ~CAUMatrixMixer();
  bool Open(OSType type, OSType subType, OSType manufacturer);
  bool Open();
  OSStatus Render(AudioUnitRenderActionFlags* actionFlags, const AudioTimeStamp* pTimeStamp, UInt32 busNumber, UInt32 frameCount, AudioBufferList* pBufList);
  bool Initialize();
  
  UInt32 GetInputBusCount();
  bool SetInputBusCount(UInt32 busCount);
  UInt32 GetOutputBusCount();
  bool SetOutputBusCount(UInt32 busCount);
  
  Float32 GetGlobalVolume();
  bool SetGlobalVolume(Float32 vol);
  Float32 GetInputVolume(UInt32 element);
  bool SetInputVolume(UInt32 element, Float32 vol);
  Float32 GetOutputVolume(UInt32 element);
  bool SetOutputVolume(UInt32 element, Float32 vol);
protected:
};

class CAUMultibandCompressor : public CAUGenericSource
{
public:
  CAUMultibandCompressor();
  virtual ~CAUMultibandCompressor();

  bool Open(OSType type, OSType subType, OSType manufacturer);
  bool Open();

  OSStatus Render(AudioUnitRenderActionFlags* actionFlags, const AudioTimeStamp* pTimeStamp, UInt32 busNumber, UInt32 frameCount, AudioBufferList* pBufList);
  bool Initialize();
  
  bool SetPostGain(Float32 gain);
  Float32 GetPostGain();
  bool SetPreGain(Float32 gain);
  Float32 GetPreGain();
  bool SetAttackTime(Float32 time);
  Float32 GetAttackTime();
  bool SetReleaseTime(Float32 time);
  Float32 GetReleaseTime();
  
protected:  
};

class CAUDynamicsProcessor : public CAUGenericSource
{
public:
  CAUDynamicsProcessor();
  virtual ~CAUDynamicsProcessor();
  
  bool Open(OSType type, OSType subType, OSType manufacturer);
  bool Open();
  
  OSStatus Render(AudioUnitRenderActionFlags* actionFlags, const AudioTimeStamp* pTimeStamp, UInt32 busNumber, UInt32 frameCount, AudioBufferList* pBufList);
  bool Initialize();
  
  bool SetCompressionThreshold(Float32 db) {return SetFloatParam(kDynamicsProcessorParam_Threshold, 0, db);}
  Float32 SetCompressionThreshold() {return GetFloatParam(kDynamicsProcessorParam_Threshold, 0);}
  bool SetHeadroom(Float32 db) {return SetFloatParam(kDynamicsProcessorParam_HeadRoom, 0, db);}
  Float32 GetHeadroom() {return GetFloatParam(kDynamicsProcessorParam_HeadRoom, 0);}
  bool SetExpansionRatio(Float32 ratio) {return SetFloatParam(kDynamicsProcessorParam_ExpansionRatio, 0, ratio);}
  Float32 GetExpansionRatio() {return GetFloatParam(kDynamicsProcessorParam_ExpansionRatio, 0);}
  bool SetExpansionThreshold(Float32 ratio) {return SetFloatParam(kDynamicsProcessorParam_ExpansionThreshold, 0, ratio);}
  Float32 GetExpansionThreshold() {return GetFloatParam(kDynamicsProcessorParam_ExpansionThreshold, 0);}
  bool SetAttackTime(Float32 time) {return SetFloatParam(kDynamicsProcessorParam_AttackTime, 0, time);}
  Float32 GetAttackTime() {return GetFloatParam(kDynamicsProcessorParam_AttackTime, 0);}
  bool SetReleaseTime(Float32 time) {return SetFloatParam(kDynamicsProcessorParam_ReleaseTime, 0, time);}
  Float32 GetReleaseTime() {return GetFloatParam(kDynamicsProcessorParam_ReleaseTime, 0);}
  bool SetMasterGain(Float32 gain) {return SetFloatParam(kDynamicsProcessorParam_MasterGain, 0, gain);}
  Float32 GetMasterGain() {return GetFloatParam(kDynamicsProcessorParam_MasterGain, 0);}
  
protected:
  bool SetFloatParam(UInt32 param, UInt32 element, Float32 val);
  Float32 GetFloatParam(UInt32 param, UInt32 element);
};

// Helper Functions
char* UInt32ToFourCC(UInt32* val);
const char* StreamDescriptionToString(AudioStreamBasicDescription desc, CStdString& str);

#define CONVERT_OSSTATUS(x) UInt32ToFourCC((UInt32*)&ret)

#endif // __arm__
#endif // __COREAUDIO_H__
