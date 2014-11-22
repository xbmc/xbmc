#pragma once
/*
 *      Copyright (C) 2010-2013 Team XBMC
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

#include "cores/AudioEngine/Interfaces/AESink.h"
#include "cores/AudioEngine/AEFactory.h"
#include "Utils/AEDeviceInfo.h"
#include "Utils/AEUtil.h"
#include <pulse/pulseaudio.h>
#include "threads/CriticalSection.h"

class CAESinkPULSE : public IAESink
{
public:
  virtual const char *GetName() { return "PULSE"; }

  CAESinkPULSE();
  virtual ~CAESinkPULSE();

  virtual bool Initialize(AEAudioFormat &format, std::string &device);
  virtual void Deinitialize();

  virtual double       GetDelay        () { return 0.0; }
  virtual void         GetDelay        (AEDelayStatus& status);
  virtual double       GetCacheTotal   ();
  virtual unsigned int AddPackets      (uint8_t **data, unsigned int frames, unsigned int offset);
  virtual void         Drain           ();

  virtual bool HasVolume() { return true; };
  virtual void SetVolume(float volume);

  static void EnumerateDevicesEx(AEDeviceInfoList &list, bool force = false);
  bool IsInitialized();
  void UpdateInternalVolume(const pa_cvolume* nVol);
  pa_stream* GetInternalStream();
  CCriticalSection m_sec;
private:
  void Pause(bool pause);
  static inline bool WaitForOperation(pa_operation *op, pa_threaded_mainloop *mainloop, const char *LogEntry);
  static bool SetupContext(const char *host, pa_context **context, pa_threaded_mainloop **mainloop);

  bool m_IsAllocated;
  bool m_passthrough;
  bool m_IsStreamPaused;

  AEAudioFormat m_format;
  unsigned int m_BytesPerSecond;
  unsigned int m_BufferSize;
  unsigned int m_Channels;

  pa_stream *m_Stream; 
  pa_cvolume m_Volume;
  bool m_volume_needs_update;

  pa_context *m_Context;
  pa_threaded_mainloop *m_MainLoop;
};
