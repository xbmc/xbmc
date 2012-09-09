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

#if defined(TARGET_DARWIN_OSX)

#include "ICoreAudioAEHAL.h"
#include "ICoreAudioSource.h"

#include <CoreAudio/CoreAudio.h>

// Forward declarations
class CCoreAudioAE;
class CCoreAudioGraph;
class CCoreAudioDevice;
class CCoreAudioStream;

class CAUOutputDevice;

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
  float             m_initVolume;
public:
  unsigned int      m_NumLatencyFrames;
  unsigned int      m_OutputBufferIndex;
  CCoreAudioAE     *m_ae;

  CCoreAudioAEHALOSX();
  virtual ~CCoreAudioAEHALOSX();

  virtual bool  InitializePCM(ICoreAudioSource *pSource, AEAudioFormat &format, bool allowMixing, AudioDeviceID outputDevice);
  virtual bool  InitializePCMEncoded(ICoreAudioSource *pSource, AEAudioFormat &format, AudioDeviceID outputDevice);
  virtual bool  InitializeEncoded(AudioDeviceID outputDevice, AEAudioFormat &format);
  virtual bool  Initialize(ICoreAudioSource *ae, bool passThrough, AEAudioFormat &format, AEDataFormat rawDataFormat, std::string &device, float initVolume);
  virtual void  Deinitialize();
  virtual void  EnumerateOutputDevices(AEDeviceList &devices, bool passthrough);
  virtual void  SetDirectInput(ICoreAudioSource *pSource, AEAudioFormat &format);
  virtual void  Stop();
  virtual bool  Start();
  virtual double GetDelay();
  virtual void  SetVolume(float volume);
  virtual unsigned int GetBufferIndex();
  virtual CAUOutputDevice* DestroyUnit(CAUOutputDevice *outputUnit);
  virtual CAUOutputDevice* CreateUnit(ICoreAudioSource *pSource, AEAudioFormat &format);
  virtual bool  AllowMixing() { return m_allowMixing; }
};

#endif
