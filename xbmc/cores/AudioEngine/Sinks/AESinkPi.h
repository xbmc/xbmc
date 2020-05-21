/*
 *  Copyright (C) 2010-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/AudioEngine/Interfaces/AESink.h"
#include "cores/AudioEngine/Utils/AEDeviceInfo.h"
#include "utils/XTimeUtils.h"

#include "platform/linux/OMXCore.h"

class CAESinkPi : public IAESink
{
public:
  virtual const char *GetName() { return "SinkPi"; }

  CAESinkPi();
  virtual ~CAESinkPi();

  static void Register();
  static IAESink* Create(std::string &device, AEAudioFormat &desiredFormat);

  virtual bool Initialize(AEAudioFormat &format, std::string &device);
  virtual void Deinitialize();
  virtual bool IsCompatible(const AEAudioFormat &format, const std::string &device);

  virtual void         GetDelay        (AEDelayStatus& status);
  virtual double       GetCacheTotal   ();
  virtual unsigned int AddPackets      (uint8_t **data, unsigned int frames, unsigned int offset);
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
  COMXCoreComponent   *m_omx_output;
  COMXCoreComponent    m_omx_splitter;
  COMXCoreComponent    m_omx_render;
  COMXCoreComponent    m_omx_render_slave;
  bool                 m_passthrough;
  COMXCoreTunnel       m_omx_tunnel_splitter;
  COMXCoreTunnel       m_omx_tunnel_splitter_slave;
  enum { AESINKPI_UNKNOWN, AESINKPI_HDMI, AESINKPI_ANALOGUE, AESINKPI_BOTH } m_output;
};
