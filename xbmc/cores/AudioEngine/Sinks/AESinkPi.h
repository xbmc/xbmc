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

#if defined(TARGET_RASPBERRY_PI)

#include "cores/AudioEngine/Interfaces/AESink.h"
#include "cores/AudioEngine/Utils/AEDeviceInfo.h"

#include "cores/omxplayer/OMXAudio.h"

class CAESinkPi : public IAESink
{
public:
  virtual const char *GetName() { return "SinkPi"; }

  CAESinkPi();
  virtual ~CAESinkPi();

  virtual bool Initialize(AEAudioFormat &format, std::string &device);
  virtual void Deinitialize();
  virtual bool IsCompatible(const AEAudioFormat &format, const std::string &device);

  virtual double       GetDelay        ();
  virtual double       GetCacheTime    ();
  virtual double       GetCacheTotal   ();
  virtual unsigned int AddPackets      (uint8_t *data, unsigned int frames, bool hasAudio, bool blocking = false);
  virtual void         Drain           ();

  static void          EnumerateDevicesEx(AEDeviceInfoList &list, bool force = false);
private:
  void                 SetAudioDest();

  std::string          m_initDevice;
  AEAudioFormat        m_initFormat;
  AEAudioFormat        m_format;
  double               m_sinkbuffer_sec_per_byte;
  static CAEDeviceInfo m_info;
  bool                 m_Initialized;
  uint32_t             m_submitted;
  OMX_AUDIO_PARAM_PCMMODETYPE m_pcm_input;
  COMXCoreComponent    m_omx_render;
  bool                 m_passthrough;
};

#endif
