#pragma once
/*
 *      Copyright (C) 2010-2012 Team XBMC
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

#include "Interfaces/AESink.h"
#include "Utils/AEDeviceInfo.h"

class AERingBuffer;

class CAESinkAUDIOTRACK : public CThread, public IAESink
{
public:
  virtual const char *GetName() { return "AUDIOTRACK"; }

  CAESinkAUDIOTRACK();
  virtual ~CAESinkAUDIOTRACK();

  virtual bool Initialize(AEAudioFormat &format, std::string &device);
  virtual void Deinitialize();
  virtual bool IsCompatible(const AEAudioFormat format, const std::string device);

  virtual double       GetDelay        ();
  virtual double       GetCacheTime    ();
  virtual double       GetCacheTotal   ();
  virtual unsigned int AddPackets      (uint8_t *data, unsigned int frames, bool hasAudio);
  virtual void         Drain           ();
  virtual bool         HasVolume       ();
  virtual void         SetVolume       (float volume);
  static void          EnumerateDevicesEx(AEDeviceInfoList &list);

private:
  virtual void Process();

  static CAEDeviceInfo m_info;
  AEAudioFormat      m_format;
  double             m_volume;
  bool               m_volume_changed;
  volatile int       m_min_frames;
  int16_t           *m_alignedS16LE;
  AERingBuffer      *m_sinkbuffer;
  unsigned int       m_sink_frameSize;
  double             m_sinkbuffer_sec;
  double             m_sinkbuffer_sec_per_byte;

  CEvent             m_wake;
  CEvent             m_inited;
  volatile bool      m_draining;
  double             m_audiotrackbuffer_sec;
  double             m_audiotrack_empty_sec;
};
