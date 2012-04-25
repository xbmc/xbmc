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
#include "ICoreAudioSource.h"

#include <AudioUnit/AudioUnit.h>
#include <AudioToolbox/AudioToolbox.h>
#include <AudioToolbox/AUGraph.h>
#include <list>
#include <vector>
#include "utils/StdString.h"
#include "CoreAudioAEHAL.h"

#define kOutputBus 0
#define kInputBus 1
#define MAX_CONNECTION_LIMIT  8
#define INVALID_BUS -1
#define MAXIMUM_MIXER_CHANNELS 9
#define MAX_CHANNEL_LABEL 15

// Forward declarations
class CCoreAudioHardware;
class CCoreAudioDevice;
class CCoreAudioStream;
class CCoreAudioUnit;
class CCoreAudioAE;
class CCoreAudioChannelLayout;
class CAUGenericSource;

typedef std::list<AudioDeviceID> CoreAudioDeviceList;

#if MAC_OS_X_VERSION_MAX_ALLOWED <= 1040
/* AudioDeviceIOProcID does not exist in Mac OS X 10.4. We can emulate
 * this by using AudioDeviceAddIOProc() and AudioDeviceRemoveIOProc(). */
#define AudioDeviceIOProcID AudioDeviceIOProc
#define AudioDeviceDestroyIOProcID AudioDeviceRemoveIOProc
static OSStatus AudioDeviceCreateIOProcID(AudioDeviceID dev,
                                          AudioDeviceIOProc proc,
                                          void *data,
                                          AudioDeviceIOProcID *procid)
{
  *procid = proc;
  return AudioDeviceAddIOProc(dev, proc, data);
}
#endif

// There is only one AudioSystemObject instance system-side.
// Therefore, all CCoreAudioHardware methods are static
class CCoreAudioHardware
{
public:
  static bool GetAutoHogMode();
  static void SetAutoHogMode(bool enable);
  static AudioStreamBasicDescription *CCoreAudioHardware::FormatsList(AudioStreamID stream);
  static AudioStreamID *StreamsList(AudioDeviceID device);
  static void ResetAudioDevices();
  static void ResetStream(AudioStreamID stream);
  static AudioDeviceID FindAudioDevice(std::string deviceName);
  static AudioDeviceID GetDefaultOutputDevice();
  static void GetOutputDeviceName(std::string& name);
  static UInt32 GetOutputDevices(CoreAudioDeviceList* pList);
};

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
  
  AudioDeviceID GetId() {return m_DeviceId;}
  std::string GetName();
  UInt32 GetTotalOutputChannels();
  bool GetStreams(AudioStreamIdList* pList);
  bool IsRunning();
  bool SetHogStatus(bool hog);
  pid_t GetHogStatus();
  bool SetMixingSupport(UInt32 mix);
  bool GetMixingSupport();
  bool SetCurrentVolume(Float32 vol);
  bool GetPreferredChannelLayout(CCoreAudioChannelLayout& layout);
  bool GetDataSources(CoreAudioDataSourceList* pList);
  Float64 GetNominalSampleRate();
  bool SetNominalSampleRate(Float64 sampleRate);
  UInt32 GetNumLatencyFrames();
  UInt32 GetBufferSize();
  bool SetBufferSize(UInt32 size);
  virtual bool SetInputSource(ICoreAudioSource* pSource, unsigned int frameSize, unsigned int outputBufferIndex);
protected:
  bool AddIOProc();
  bool RemoveIOProc();
  ICoreAudioSource*   m_pSource;
  static OSStatus DirectRenderCallback(AudioDeviceID inDevice, 
                                       const AudioTimeStamp* inNow, 
                                       const AudioBufferList* inInputData, 
                                       const AudioTimeStamp* inInputTime, 
                                       AudioBufferList* outOutputData, 
                                       const AudioTimeStamp* inOutputTime, 
                                       void* inClientData);
  AudioDeviceID m_DeviceId;
  bool m_Started;
  int m_MixerRestore;
  AudioDeviceIOProc m_IoProc;
  AudioObjectPropertyListenerProc m_ObjectListenerProc;
  
  Float64 m_SampleRateRestore;
  pid_t m_HogPid;
  unsigned int m_frameSize;
  unsigned int m_OutputBufferIndex;
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

typedef std::list<AudioChannelLayoutTag> AudioChannelLayoutList;

class CCoreAudioUnit
{
public:
  CCoreAudioUnit();
  virtual ~CCoreAudioUnit();
  
  virtual bool Open(AUGraph audioGraph, ComponentDescription desc);
  virtual bool Open(AUGraph audioGraph, OSType type, OSType subType, OSType manufacturer);
  virtual void Close();
  virtual bool SetInputSource(ICoreAudioSource* pSource);
  virtual bool IsInitialized() {return m_Initialized;}
  virtual bool GetFormat(AudioStreamBasicDescription* pDesc, AudioUnitScope scope, AudioUnitElement bus);    
  virtual bool SetFormat(AudioStreamBasicDescription* pDesc, AudioUnitScope scope, AudioUnitElement bus);
  virtual bool SetMaxFramesPerSlice(UInt32 maxFrames);
  virtual bool GetSupportedChannelLayouts(AudioChannelLayoutList* pLayouts);
  virtual void GetFormatDesc(AEAudioFormat format, 
                             AudioStreamBasicDescription *streamDesc,
                             AudioStreamBasicDescription *coreaudioDesc);
  virtual float GetLatency();
  virtual bool Stop();
  virtual bool Start();
  virtual AudioUnit   GetUnit  (){return m_audioUnit;}
  virtual AUGraph     GetGraph (){return m_audioGraph;}
  virtual AUNode      GetNode  (){return m_audioNode;}
  virtual int         GetBus   (){return m_busNumber;}
  virtual void        SetBus   (int busNumber){m_busNumber = busNumber;}
protected:
  bool SetRenderProc();
  bool RemoveRenderProc();
  static OSStatus RenderCallback(void *inRefCon, 
                                 AudioUnitRenderActionFlags *ioActionFlags, 
                                 const AudioTimeStamp *inTimeStamp, 
                                 UInt32 inBusNumber, 
                                 UInt32 inNumberFrames, 
                                 AudioBufferList *ioData);
  ICoreAudioSource*   m_pSource;
  AudioUnit           m_audioUnit;
  AUNode              m_audioNode;
  AUGraph             m_audioGraph;
  bool                m_Initialized;
  AURenderCallback    m_renderProc;
  int                 m_busNumber;
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
  
  Float32 GetCurrentVolume();
  bool SetCurrentVolume(Float32 vol);
  bool EnableInputOuput();
  virtual bool GetPreferredChannelLayout(CCoreAudioChannelLayout& layout);
protected:
  AudioDeviceID m_DeviceId;
};

class CAUMatrixMixer : public CAUOutputDevice
{
public:
  CAUMatrixMixer();
  virtual ~CAUMatrixMixer();
  bool InitMatrixMixerVolumes();
  
  UInt32 GetInputBusCount();
  bool SetInputBusFormat(UInt32 busCount, AudioStreamBasicDescription *pFormat);
  bool SetInputBusCount(UInt32 busCount);
  UInt32 GetOutputBusCount();
  bool SetOutputBusCount(UInt32 busCount);
  
  Float32 GetGlobalVolume();
  bool SetGlobalVolume(Float32 vol);
  Float32 GetInputVolume(UInt32 element);
  bool SetInputVolume(UInt32 element, Float32 vol);
  Float32 GetOutputVolume(UInt32 element);
  bool SetOutputVolume(UInt32 element, Float32 vol);
};

class CCoreAudioMixMap
{
public:
  CCoreAudioMixMap();
  CCoreAudioMixMap(AudioChannelLayout& inLayout, AudioChannelLayout& outLayout);
  virtual ~CCoreAudioMixMap();
  operator Float32*() const {return m_pMap;}
  const Float32* GetBuffer() {return m_pMap;}
  UInt32 GetInputChannels() {return m_inChannels;}
  UInt32 GetOutputChannels() {return m_outChannels;}
  bool IsValid() {return m_isValid;}
  void Rebuild(AudioChannelLayout& inLayout, AudioChannelLayout& outLayout);
  static CCoreAudioMixMap *CreateMixMap(CAUOutputDevice  *audioUnit, AEAudioFormat &format, AudioChannelLayoutTag layoutTag);
  static bool SetMixingMatrix(CAUMatrixMixer  *mixerUnit, CCoreAudioMixMap *mixMap, AudioStreamBasicDescription *inputFormat, AudioStreamBasicDescription *fmt, int channelOffset);
private:
  Float32* m_pMap;
  UInt32 m_inChannels;
  UInt32 m_outChannels;
  bool m_isValid;
};

class CCoreAudioChannelLayout
{
public:
  CCoreAudioChannelLayout();
  CCoreAudioChannelLayout(AudioChannelLayout& layout);
  virtual ~CCoreAudioChannelLayout();
  operator AudioChannelLayout*() {return m_pLayout;}
  bool CopyLayout(AudioChannelLayout& layout);
  static UInt32 GetChannelCountForLayout(AudioChannelLayout& layout);
  static const char* ChannelLabelToString(UInt32 label);
  static const char* ChannelLayoutToString(AudioChannelLayout& layout, std::string& str);
  bool AllChannelUnknown();
protected:
  AudioChannelLayout* m_pLayout;
};

class CCoreAudioGraph
{
private:
  AUGraph           m_audioGraph;
  
  CAUOutputDevice  *m_audioUnit;
  CAUMatrixMixer   *m_mixerUnit;
  CAUOutputDevice  *m_inputUnit;
  
  int m_reservedBusNumber[MAX_CONNECTION_LIMIT];
  bool              m_initialized;
  AudioDeviceID     m_deviceId;
  bool              m_allowMixing;
  CCoreAudioMixMap *m_mixMap;

  typedef std::list<CAUOutputDevice*> AUUnitList;
  AUUnitList        m_auUnitList;

  bool              m_ATV1;

public:
  CCoreAudioGraph();
  ~CCoreAudioGraph();
  
  bool Open(ICoreAudioSource *pSource, AEAudioFormat &format, AudioDeviceID deviceId, bool allowMixing, AudioChannelLayoutTag layoutTag);
  bool Close();
  bool Start();
  bool Stop();
  AudioChannelLayoutTag GetChannelLayoutTag(int layout);
  bool SetInputSource(ICoreAudioSource* pSource);
  bool SetCurrentVolume(Float32 vol);
  CAUOutputDevice *DestroyUnit(CAUOutputDevice *outputUnit);
  CAUOutputDevice *CreateUnit(AEAudioFormat &format);
  int GetFreeBus();
  void ReleaseBus(int busNumber);
  bool IsBusFree(int busNumber);
  int GetMixerChannelOffset(int busNumber);
  void ShowGraph();
};

class CCoreAudioAEHALOSX : public ICoreAudioAEHAL
{
protected:
  CCoreAudioGraph  *m_audioGraph;
  CCoreAudioDevice *m_AudioDevice;
  CCoreAudioStream *m_OutputStream;
  bool              m_Initialized;
  bool              m_Passthrough;
  AEAudioFormat     m_initformat;
  bool              m_allowMixing;
  bool              m_encoded;
  AEDataFormat      m_rawDataFormat;
public:
  unsigned int      m_NumLatencyFrames;
  unsigned int      m_OutputBufferIndex;
  CCoreAudioAE     *m_ae;

  CCoreAudioAEHALOSX();
  virtual ~CCoreAudioAEHALOSX();

  virtual bool  InitializePCM(ICoreAudioSource *pSource, AEAudioFormat &format, bool allowMixing, AudioDeviceID outputDevice);
  virtual bool  InitializePCMEncoded(ICoreAudioSource *pSource, AEAudioFormat &format, AudioDeviceID outputDevice);
  virtual bool  InitializeEncoded(AudioDeviceID outputDevice, AEAudioFormat &format);
  virtual bool  Initialize(ICoreAudioSource *ae, bool passThrough, AEAudioFormat &format, AEDataFormat rawDataFormat, std::string &device);
  virtual void  Deinitialize();
  virtual void  EnumerateOutputDevices(AEDeviceList &devices, bool passthrough);
  virtual void  SetDirectInput(ICoreAudioSource *pSource, AEAudioFormat &format);
  virtual void  Stop();
  virtual bool  Start();
  virtual double GetDelay();
  virtual void  SetVolume(float volume);
  virtual unsigned int GetBufferIndex();
  virtual CAUOutputDevice *DestroyUnit(CAUOutputDevice *outputUnit);
  virtual CAUOutputDevice *CreateUnit(ICoreAudioSource *pSource, AEAudioFormat &format);
  virtual bool  AllowMixing() { return m_allowMixing; }
};

#endif
#endif
