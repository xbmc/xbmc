#pragma once
/*
 *      Copyright (C) 2011-2013 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "system.h"

#if defined(TARGET_DARWIN_OSX)

#include <list>
#include <string>
#include <vector>

#include "cores/AudioEngine/Sinks/osx/CoreAudioStream.h"

#include <CoreAudio/CoreAudio.h>

typedef std::vector<UInt32> CoreAudioDataSourceList;
typedef std::list<AudioDeviceID> CoreAudioDeviceList;

class CCoreAudioChannelLayout;

class CCoreAudioDevice
{
public:
  CCoreAudioDevice();
  CCoreAudioDevice(AudioDeviceID deviceId);
  virtual ~CCoreAudioDevice();
  
  bool          Open(AudioDeviceID deviceId);
  void          Close();
  
  void          Start();
  void          Stop();
  void          RemoveObjectListenerProc(AudioObjectPropertyListenerProc callback, void *pClientData);
  bool          SetObjectListenerProc(AudioObjectPropertyListenerProc callback, void *pClientData);
  
  AudioDeviceID GetId() {return m_DeviceId;}
  std::string   GetName() const;
  bool          IsDigital() const;
  UInt32        GetTransportType() const;
  UInt32        GetTotalOutputChannels() const;
  UInt32        GetNumChannelsOfStream(UInt32 streamIdx) const;
  bool          GetStreams(AudioStreamIdList *pList);
  bool          IsRunning();
  bool          SetHogStatus(bool hog);
  pid_t         GetHogStatus();
  bool          SetMixingSupport(UInt32 mix);
  bool          GetMixingSupport();
  bool          SetCurrentVolume(Float32 vol);
  bool          GetPreferredChannelLayout(CCoreAudioChannelLayout &layout) const;
  bool          GetPreferredChannelLayoutForStereo(CCoreAudioChannelLayout &layout) const;
  bool          GetDataSources(CoreAudioDataSourceList *pList) const;
  bool          GetDataSource(UInt32 &dataSourceId) const;
  bool          SetDataSource(UInt32 &dataSourceId);
  std::string   GetDataSourceName(UInt32 dataSourceId) const;
  std::string   GetCurrentDataSourceName() const;
  Float64       GetNominalSampleRate();
  bool          SetNominalSampleRate(Float64 sampleRate);
  UInt32        GetNumLatencyFrames();
  UInt32        GetBufferSize();
  bool          SetBufferSize(UInt32 size);
    
  static void   RegisterDeviceChangedCB(bool bRegister, AudioObjectPropertyListenerProc callback,  void *ref);
  static void   RegisterDefaultOutputDeviceChangedCB(bool bRegister, AudioObjectPropertyListenerProc callback, void *ref);
  // suppresses the default output device changed callback for given time in ms
  static void   SuppressDefaultOutputDeviceCB(unsigned int suppressTimeMs){ m_callbackSuppressTimer.Set(suppressTimeMs); }

  bool          AddIOProc(AudioDeviceIOProc ioProc, void* pCallbackData);
  bool          RemoveIOProc();
protected:

  bool              m_Started;
  AudioDeviceID     m_DeviceId;
  int               m_MixerRestore;
  AudioDeviceIOProc m_IoProc;
  AudioObjectPropertyListenerProc m_ObjectListenerProc;
  
  Float64           m_SampleRateRestore;
  pid_t             m_HogPid;
  unsigned int      m_frameSize;
  unsigned int      m_OutputBufferIndex;
  unsigned int      m_BufferSizeRestore;

  static XbmcThreads::EndTime m_callbackSuppressTimer;
  static AudioObjectPropertyListenerProc m_defaultOutputDeviceChangedCB;
  static OSStatus defaultOutputDeviceChanged(AudioObjectID                       inObjectID,
                                             UInt32                              inNumberAddresses,
                                             const AudioObjectPropertyAddress    inAddresses[],
                                             void*                               inClientData);

};

#endif
