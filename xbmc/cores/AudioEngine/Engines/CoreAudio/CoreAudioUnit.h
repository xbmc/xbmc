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

#include "system.h"

#if defined(TARGET_DARWIN_OSX)

#define INVALID_BUS -1
#define kOutputBus 0
#define kInputBus 1

#include "ICoreAudioSource.h"
#include "CoreAudioChannelLayout.h"

#include <AudioToolbox/AUGraph.h>
#include <CoreAudio/CoreAudio.h>
#include <CoreServices/CoreServices.h>

class CCoreAudioUnit
{
public:
  CCoreAudioUnit();
  virtual ~CCoreAudioUnit();
  
  virtual bool      Open(AUGraph audioGraph, AudioComponentDescription desc);
  virtual bool      Open(AUGraph audioGraph, OSType type, OSType subType, OSType manufacturer);
  virtual void      Close();
  virtual bool      SetInputSource(ICoreAudioSource *pSource);
  virtual bool      IsInitialized() {return m_Initialized;}
  virtual bool      GetFormat(AudioStreamBasicDescription *pDesc, AudioUnitScope scope, AudioUnitElement bus);    
  virtual bool      SetFormat(AudioStreamBasicDescription *pDesc, AudioUnitScope scope, AudioUnitElement bus);
  virtual bool      SetMaxFramesPerSlice(UInt32 maxFrames);
  virtual bool      GetSupportedChannelLayouts(AudioChannelLayoutList* pLayouts);
  virtual void      GetFormatDesc(AEAudioFormat format, 
                      AudioStreamBasicDescription *streamDesc, AudioStreamBasicDescription *coreaudioDesc);
  virtual float     GetLatency();
  virtual bool      Stop();
  virtual bool      Start();
  virtual AudioUnit GetUnit  (){return m_audioUnit;}
  virtual AUGraph   GetGraph (){return m_audioGraph;}
  virtual AUNode    GetNode  (){return m_audioNode;}
  virtual int       GetBus   (){return m_busNumber;}
  virtual void      SetBus   (int busNumber){m_busNumber = busNumber;}
  
protected:
  bool SetRenderProc();
  bool RemoveRenderProc();
  static OSStatus RenderCallback(void *inRefCon, 
    AudioUnitRenderActionFlags *ioActionFlags,const AudioTimeStamp *inTimeStamp, 
    UInt32 inBusNumber, UInt32 inNumberFrames, AudioBufferList *ioData);

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

  bool            SetCurrentDevice(AudioDeviceID deviceId);
  bool            GetChannelMap(CoreAudioChannelList *pChannelMap);
  bool            SetChannelMap(CoreAudioChannelList *pChannelMap);
  UInt32          GetBufferFrameSize();
  
  Float32         GetCurrentVolume();
  bool            SetCurrentVolume(Float32 vol);
  bool            EnableInputOuput();
  virtual bool    GetPreferredChannelLayout(CCoreAudioChannelLayout &layout);

protected:
  AudioDeviceID   m_DeviceId;
};

class CAUMatrixMixer : public CAUOutputDevice
{
public:
  CAUMatrixMixer();
  virtual ~CAUMatrixMixer();

  bool            InitMatrixMixerVolumes();
  
  UInt32          GetInputBusCount();
  bool            SetInputBusFormat(UInt32 busCount, AudioStreamBasicDescription *pFormat);
  bool            SetInputBusCount(UInt32 busCount);
  UInt32          GetOutputBusCount();
  bool            SetOutputBusCount(UInt32 busCount);
  
  Float32         GetGlobalVolume();
  bool            SetGlobalVolume(Float32 vol);
  Float32         GetInputVolume(UInt32 element);
  bool            SetInputVolume(UInt32 element, Float32 vol);
  Float32         GetOutputVolume(UInt32 element);
  bool            SetOutputVolume(UInt32 element, Float32 vol);
};

#endif