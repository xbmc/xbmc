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

#include "cores/AudioEngine/Interfaces/AESink.h"
#include "cores/AudioEngine/Utils/AEDeviceInfo.h"
#include "threads/CriticalSection.h"
#include <set>

class AERingBuffer;
namespace jni
{
class CJNIAudioTrack;
};

class CAESinkAUDIOTRACK : public IAESink
{
public:
  virtual const char *GetName() { return "AUDIOTRACK"; }

  CAESinkAUDIOTRACK();
  virtual ~CAESinkAUDIOTRACK();

  virtual bool Initialize(AEAudioFormat &format, std::string &device);
  virtual void Deinitialize();
  bool IsInitialized();

  virtual void         GetDelay        (AEDelayStatus& status);
  virtual double       GetLatency      ();
  virtual double       GetCacheTotal   ();
  virtual unsigned int AddPackets      (uint8_t **data, unsigned int frames, unsigned int offset);
  virtual void         Drain           ();
  virtual bool         WantsIEC61937   ();
  static void          EnumerateDevicesEx(AEDeviceInfoList &list, bool force = false);

protected:
  static bool IsSupported(int sampleRateInHz, int channelConfig, int audioFormat);

private:
  jni::CJNIAudioTrack  *m_at_jni;
  double                m_duration_written;
  double                m_last_duration_written;

  static CAEDeviceInfo m_info;
  static std::set<unsigned int>       m_sink_sampleRates;
  std::vector<double>                 m_smoothedDelayVec;
  int                                 m_smoothedDelayCount;
  AEAudioFormat      m_format;
  double             m_volume;
  unsigned int       m_sink_frameSize;
  unsigned int       m_sink_sampleRate;
  bool               m_passthrough;
  double             m_audiotrackbuffer_sec;
  int                m_encoding;
  unsigned int       m_buffer_size;
  double             m_sink_delay;
};
