#pragma once
/*
 *      Copyright (C) 2011-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#if defined(TARGET_DARWIN_IOS)

#include <list>
#include <vector>

#include "ICoreAudioAEHAL.h"
#include "CoreAudioAEHAL.h"
#include "utils/StdString.h"

#include <AudioUnit/AudioUnit.h>
#include <AudioUnit/AudioUnitProperties.h>
#include <AudioToolbox/AudioToolbox.h>
#include <AudioToolbox/AudioServices.h>
#include <CoreAudio/CoreAudioTypes.h>

#define kOutputBus 0
#define kInputBus 1
#define MAX_CONNECTION_LIMIT  8
#define INVALID_BUS -1

// Forward declarations
class CCoreAudioAE;
class CIOSCoreAudioConverter;

typedef std::list<AudioComponentInstance> IOSCoreAudioDeviceList;

// There is only one AudioSystemObject instance system-side.
// Therefore, all CIOSCoreAudioHardware methods are static
class CIOSCoreAudioHardware
{
public:
  static AudioComponentInstance FindAudioDevice(std::string deviceName);
  static AudioComponentInstance GetDefaultOutputDevice();
  static UInt32 GetOutputDevices(IOSCoreAudioDeviceList* pList);
};

class CCoreAudioUnit
{
public:
  CCoreAudioUnit();
  virtual ~CCoreAudioUnit();

  virtual bool Open(AUGraph audioGraph, AudioComponentDescription desc);
  virtual bool Open(AUGraph audioGraph, OSType type, OSType subType, OSType manufacturer);
  virtual void Close();
  virtual bool SetInputSource(ICoreAudioSource* pSource);
  virtual bool IsInitialized() {return m_Initialized;}
  virtual bool GetFormat(AudioStreamBasicDescription* pDesc, AudioUnitScope scope, AudioUnitElement bus);
  virtual bool SetFormat(AudioStreamBasicDescription* pDesc, AudioUnitScope scope, AudioUnitElement bus);
  virtual bool SetMaxFramesPerSlice(UInt32 maxFrames);
  virtual void GetFormatDesc(AEAudioFormat format, AudioStreamBasicDescription *streamDesc);
  virtual float GetLatency();
  virtual bool SetSampleRate(Float64 sampleRate, AudioUnitScope scope, AudioUnitElement bus);
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
  UInt32 GetBufferFrameSize();

  /*
  Float32 GetCurrentVolume();
  bool SetCurrentVolume(Float32 vol);
  */
  bool EnableInputOuput();
};

class CAUMultiChannelMixer : public CAUOutputDevice
{
public:
  CAUMultiChannelMixer();
  virtual ~CAUMultiChannelMixer();

  UInt32 GetInputBusCount();
  bool SetInputBusFormat(UInt32 busCount, AudioStreamBasicDescription *pFormat);
  bool SetInputBusCount(UInt32 busCount);
  UInt32 GetOutputBusCount();
  bool SetOutputBusCount(UInt32 busCount);

  Float32 GetCurrentVolume();
  bool SetCurrentVolume(Float32 vol);
};

class CCoreAudioGraph
{
private:
  AUGraph           m_audioGraph;

  CAUOutputDevice  *m_audioUnit;
  CAUMultiChannelMixer   *m_mixerUnit;
  CAUOutputDevice  *m_inputUnit;

  int m_reservedBusNumber[MAX_CONNECTION_LIMIT];
  bool              m_initialized;
  bool              m_allowMixing;

  typedef std::list<CAUOutputDevice*> AUUnitList;
  AUUnitList        m_auUnitList;

public:
  CCoreAudioGraph();
  ~CCoreAudioGraph();

  bool Open(ICoreAudioSource *pSource, AEAudioFormat &format, bool allowMixing, float initVolume);
  bool Close();
  bool Start();
  bool Stop();
  bool SetInputSource(ICoreAudioSource* pSource);
  bool SetCurrentVolume(Float32 vol);
  CAUOutputDevice *DestroyUnit(CAUOutputDevice *outputUnit);
  CAUOutputDevice *CreateUnit(AEAudioFormat &format);
  int GetFreeBus();
  void ReleaseBus(int busNumber);
  bool IsBusFree(int busNumber);
  int GetMixerChannelOffset(int busNumber);
  void ShowGraph();
  float GetLatency();
};

class CCoreAudioAEHALIOS : public ICoreAudioAEHAL
{
protected:
  CCoreAudioGraph  *m_audioGraph;
  bool              m_Initialized;
  bool              m_Passthrough;
  AEAudioFormat     m_initformat;
  bool              m_allowMixing;
  bool              m_encoded;
  AEDataFormat      m_rawDataFormat;
  float             m_initVolume;
public:
  unsigned int      m_NumLatencyFrames;
  unsigned int      m_OutputBufferIndex;
  CCoreAudioAE     *m_ae;

  CCoreAudioAEHALIOS();
  virtual ~CCoreAudioAEHALIOS();

  virtual bool   InitializePCM(ICoreAudioSource *pSource, AEAudioFormat &format, bool allowMixing);
  virtual bool   InitializePCMEncoded(ICoreAudioSource *pSource, AEAudioFormat &format);
  virtual bool   Initialize(ICoreAudioSource *ae, bool passThrough, AEAudioFormat &format, AEDataFormat rawDataFormat, std::string &device, float initVolume);
  virtual void   Deinitialize();
  virtual void   EnumerateOutputDevices(AEDeviceList &devices, bool passthrough);
  virtual void   SetDirectInput(ICoreAudioSource *pSource, AEAudioFormat &format);
  virtual void   Stop();
  virtual bool   Start();
  virtual double GetDelay();
  virtual void   SetVolume(float volume);
  virtual unsigned int GetBufferIndex();
  virtual CAUOutputDevice *DestroyUnit(CAUOutputDevice *outputUnit);
  virtual CAUOutputDevice *CreateUnit(ICoreAudioSource *pSource, AEAudioFormat &format);
  virtual bool  AllowMixing() { return m_allowMixing; }
};

#endif
