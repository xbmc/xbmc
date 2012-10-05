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

#include "system.h"
#ifdef HAS_ALSA

#include "Interfaces/AESink.h"
#include "Utils/AEDeviceInfo.h"
#include <stdint.h>

#define ALSA_PCM_NEW_HW_PARAMS_API
#include <alsa/asoundlib.h>

#include "threads/CriticalSection.h"

class CAESinkALSA : public IAESink
{
public:
  virtual const char *GetName() { return "ALSA"; }

  CAESinkALSA();
  virtual ~CAESinkALSA();

  virtual bool Initialize  (AEAudioFormat &format, std::string &device);
  virtual void Deinitialize();
  virtual bool IsCompatible(const AEAudioFormat format, const std::string device);

  virtual void         Stop            ();
  virtual double       GetDelay        ();
  virtual double       GetCacheTime    ();
  virtual double       GetCacheTotal   ();
  virtual unsigned int AddPackets      (uint8_t *data, unsigned int frames, bool hasAudio);
  virtual void         Drain           ();

  static void EnumerateDevicesEx(AEDeviceInfoList &list);
private:
  CAEChannelInfo GetChannelLayout(AEAudioFormat format);
  void           GetAESParams(const AEAudioFormat format, std::string& params);
  void           HandleError(const char* name, int err);

  std::string       m_initDevice;
  AEAudioFormat     m_initFormat;
  AEAudioFormat     m_format;
  unsigned int      m_bufferSize;
  double            m_formatSampleRateMul;
  bool              m_passthrough;
  CAEChannelInfo    m_channelLayout;
  std::string       m_device;
  snd_pcm_t        *m_pcm;
  int               m_timeout;

  static snd_pcm_format_t AEFormatToALSAFormat(const enum AEDataFormat format);

  bool InitializeHW(AEAudioFormat &format);
  bool InitializeSW(AEAudioFormat &format);

  static void AppendParams(std::string &device, const std::string &params);
  static bool TryDevice(const std::string &name, snd_pcm_t **pcmp, snd_config_t *lconf);
  static bool TryDeviceWithParams(const std::string &name, const std::string &params, snd_pcm_t **pcmp, snd_config_t *lconf);
  static bool OpenPCMDevice(const std::string &name, const std::string &params, int channels, snd_pcm_t **pcmp, snd_config_t *lconf, bool preferDmixStereo = false);

  static AEDeviceType AEDeviceTypeFromName(const std::string &name);
  static std::string GetParamFromName(const std::string &name, const std::string &param);
  static void EnumerateDevice(AEDeviceInfoList &list, const std::string &device, const std::string &description, snd_config_t *config);
  static bool SoundDeviceExists(const std::string& device);
  static bool GetELD(snd_hctl_t *hctl, int device, CAEDeviceInfo& info, bool& badHDMI);

  static void sndLibErrorHandler(const char *file, int line, const char *function, int err, const char *fmt, ...);
};
#endif

