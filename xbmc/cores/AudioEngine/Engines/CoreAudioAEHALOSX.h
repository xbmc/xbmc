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

#ifndef __COREAUDIOAEHALOSX_H__
#define __COREAUDIOAEHALOSX_H__

#ifndef __arm__

#include "ICoreAudioAEHAL.h"

#include <AudioUnit/AudioUnit.h>
#include <AudioToolbox/AudioToolbox.h>
#include <list>
#include <vector>
#include "utils/StdString.h"

#define kOutputBus 0
#define kInputBus 1

// Forward declarations
class CCoreAudioHardware;
class CCoreAudioDevice;
class CCoreAudioStream;
class CCoreAudioUnit;
class CCoreAudioAE;

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
  static AudioStreamBasicDescription *CCoreAudioHardware::FormatsList(AudioStreamID stream);
  static AudioStreamID *StreamsList(AudioDeviceID device);
  static void ResetAudioDevices();
  static void ResetStream(AudioStreamID stream);
  static AudioDeviceID FindAudioDevice(CStdString deviceName);
  static AudioDeviceID GetDefaultOutputDevice();
  static void GetOutputDeviceName(CStdString& name);
  static UInt32 GetOutputDevices(CoreAudioDeviceList* pList);
};

// Not yet implemented
// kAudioDevicePropertyDeviceIsRunning, kAudioDevicePropertyDeviceIsRunningSomewhere, kAudioDevicePropertyLatency, 
// kAudioDevicePropertyBufferFrameSize, kAudioDevicePropertyBufferFrameSizeRange, kAudioDevicePropertyUsesVariableBufferFrameSizes,
// kAudioDevicePropertySafetyOffset, kAudioDevicePropertyIOCycleUsage, kAudioDevicePropertyStreamConfiguration
// kAudioDevicePropertyIOProcStreamUsage, kAudioDevicePropertyPreferredChannelsForStereo, kAudioDevicePropertyPreferredChannelLayout,
// kAudioDevicePropertyAvailableNominalSampleRates, kAudioDevicePropertyActualSampleRate,
// kAudioDevicePropertyTransportType

typedef std::list<AudioStreamID> AudioStreamIdList;
typedef std::vector<SInt32> CoreAudioChannelList;
typedef std::list<UInt32> CoreAudioDataSourceList;

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
  void RemoveObjectListenerProc(AudioObjectPropertyListenerProc callback, void* pClientData);
  bool SetObjectListenerProc(AudioObjectPropertyListenerProc callback, void* pClientData);
  bool AddIOProc(AudioDeviceIOProc ioProc, void* pCallbackData);
  void RemoveIOProc();
  
  AudioDeviceID GetId() {return m_DeviceId;}
  const char* GetName(CStdString& name);
  UInt32 GetTotalOutputChannels();
  bool GetStreams(AudioStreamIdList* pList);
  bool IsRunning();
  Boolean IsAudioPropertySettable(AudioObjectID id,
                                  AudioObjectPropertySelector selector,
                                  Boolean *outData);
  UInt32 GetAudioPropertyArray(AudioObjectID id,
                               AudioObjectPropertySelector selector,
                               AudioObjectPropertyScope scope,
                               void **outData);
  UInt32 GetGlobalAudioPropertyArray(AudioObjectID id,
                                     AudioObjectPropertySelector selector,
                                     void **outData);
  OSStatus GetAudioPropertyString(AudioObjectID id,
                                  AudioObjectPropertySelector selector,
                                  char **outData);
  OSStatus SetAudioProperty(AudioObjectID id,
                            AudioObjectPropertySelector selector,
                            UInt32 inDataSize, void *inData);
  OSStatus GetAudioProperty(AudioObjectID id,
                            AudioObjectPropertySelector selector,
                            UInt32 outSize, void *outData);
  bool SetHogStatus(bool hog);
  bool SetMixingSupport(UInt32 mix);
  bool GetMixingSupport();
  bool GetPreferredChannelLayout(CoreAudioChannelList *pChannelMap);
  bool GetDataSources(CoreAudioDataSourceList* pList);
  Float64 GetNominalSampleRate();
  bool SetNominalSampleRate(Float64 sampleRate);
  UInt32 GetNumLatencyFrames();
protected:
  AudioDeviceID m_DeviceId;
  bool m_Started;
  int m_MixerRestore;
  AudioDeviceIOProc m_IoProc;
  AudioObjectPropertyListenerProc m_ObjectListenerProc;
  
  Float64 m_SampleRateRestore;
  pid_t m_HogPid;
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

class CCoreAudioUnit
{
public:
  CCoreAudioUnit();
  virtual ~CCoreAudioUnit();
  
  bool Open(ComponentDescription desc);
  bool Open(OSType type, OSType subType, OSType manufacturer);
  void Attach(AudioUnit audioUnit) {m_Component = audioUnit;}
  AudioUnit GetComponent(){return m_Component;}
  void Close();
  bool Initialize();
  bool IsInitialized() {return m_Initialized;}
  bool SetRenderProc(AURenderCallback callback, void* pClientData);
  bool GetFormat(AudioStreamBasicDescription* pDesc, AudioUnitScope scope, AudioUnitElement bus);    
  bool SetFormat(AudioStreamBasicDescription* pDesc, AudioUnitScope scope, AudioUnitElement bus);
  bool SetMaxFramesPerSlice(UInt32 maxFrames);
protected:
  AudioUnit m_Component;
  bool m_Initialized;
};

class CAUOutputDevice : public CCoreAudioUnit
{
public:
  CAUOutputDevice();
  virtual ~CAUOutputDevice();
  bool SetCurrentDevice(AudioDeviceID deviceId);
  bool GetInputChannelMap(std::list<SInt32> &pChannelMap);
  bool SetInputChannelMap(std::list<SInt32> &pChannelMap);
  bool SetOutputChannelMap(std::list<SInt32> &pChannelMap);
  UInt32 GetBufferFrameSize();
  UInt32 SetBufferFrameSize(UInt32 frames);
  
  void Start();
  void Stop();
  bool IsRunning();
  
  Float32 GetCurrentVolume();
  bool SetCurrentVolume(Float32 vol);  
protected:
  AudioDeviceID m_DeviceId;
};

class CAUMatrixMixer : public CCoreAudioUnit
{
public:
  CAUMatrixMixer();
  virtual ~CAUMatrixMixer();
protected:
};

class CCoreAudioAEHALOSX : public ICoreAudioAEHAL
{
protected:
  CAUOutputDevice  *m_AUOutput;
  CCoreAudioUnit   *m_MixerUnit;
  CCoreAudioDevice *m_AudioDevice;
  CCoreAudioStream *m_OutputStream;
  bool              m_Initialized;
  bool              m_Passthrough;
public:

  AEAudioFormat     m_format;
  unsigned int      m_BytesPerFrame;
  unsigned int      m_BytesPerSec;
  unsigned int      m_NumLatencyFrames;
  unsigned int      m_OutputBufferIndex;
  CCoreAudioAE     *m_ae;

  CCoreAudioAEHALOSX();
  virtual ~CCoreAudioAEHALOSX();

  virtual bool  InitializePCM(AEAudioFormat &format, CStdString &device, unsigned int bps);
  virtual bool  InitializePCMEncoded(AEAudioFormat &format, CStdString &device, unsigned int bps);
  virtual bool  InitializeEncoded(AudioDeviceID outputDevice, AEAudioFormat &format, unsigned int bps);
  virtual bool  Initialize(IAE *ae, bool passThrough, AEAudioFormat &format, CStdString &device);
  virtual void  Deinitialize();
  virtual void  EnumerateOutputDevices(AEDeviceList &devices, bool passthrough);
  virtual void  Stop();
  virtual bool  Start();
  virtual float GetDelay();
};

#endif
#endif
