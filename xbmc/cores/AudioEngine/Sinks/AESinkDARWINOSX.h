#pragma once
/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include "cores/AudioEngine/Interfaces/AESink.h"
#include "cores/AudioEngine/Utils/AEDeviceInfo.h"
#include "cores/AudioEngine/Sinks/osx/CoreAudioDevice.h"

class AERingBuffer;
class AEDelayStatus;

class CAESinkDARWINOSX : public IAESink
{
public:
  virtual const char *GetName() { return "DARWINOSX"; }

  CAESinkDARWINOSX();
  virtual ~CAESinkDARWINOSX();

  virtual bool Initialize(AEAudioFormat &format, std::string &device);
  virtual void Deinitialize();

  virtual void         GetDelay(AEDelayStatus& status);
  virtual double       GetCacheTotal   ();
  virtual unsigned int AddPackets      (uint8_t **data, unsigned int frames, unsigned int offset);
  virtual void         Drain           ();
  static void          EnumerateDevicesEx(AEDeviceInfoList &list, bool force = false);

private:
  static OSStatus renderCallback(AudioDeviceID inDevice, const AudioTimeStamp* inNow, const AudioBufferList* inInputData, const AudioTimeStamp* inInputTime, AudioBufferList* outOutputData, const AudioTimeStamp* inOutputTime, void* inClientData);
  void SetHogMode(bool on);

  CAEDeviceInfo      m_info;

  CCoreAudioDevice   m_device;
  CCoreAudioStream   m_outputStream;
  unsigned int       m_latentFrames;
  unsigned int       m_outputBufferIndex;

  bool               m_outputBitstream;   ///< true if we're bistreaming into a LinearPCM stream rather than AC3 stream.
  unsigned int       m_planes;            ///< number of audio planes (1 if non-planar)
  unsigned int       m_frameSizePerPlane; ///< frame size (per plane) in bytes
  unsigned int       m_framesPerSecond;   ///< sample rate

  AERingBuffer      *m_buffer;
  volatile bool      m_started;     // set once we get a callback from CoreAudio, which can take a little while.

  CAESpinSection         m_render_locker;
  volatile int64_t       m_render_tick;
  volatile double        m_render_delay;
};
