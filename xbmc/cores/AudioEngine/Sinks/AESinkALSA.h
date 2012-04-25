#pragma once
/*
 *      Copyright (C) 2005-2010 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
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
  virtual unsigned int AddPackets      (uint8_t *data, unsigned int frames);
  virtual void         Drain           ();

  static void EnumerateDevicesEx(AEDeviceInfoList &list);
private:
  CAEChannelInfo GetChannelLayout(AEAudioFormat format);
  void           GetPassthroughDevice(const AEAudioFormat format, std::string& device);
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

  static bool SoundDeviceExists(const std::string& device);
  static bool GetELD(snd_hctl_t *hctl, int device, CAEDeviceInfo& info, bool& badHDMI);
};
#endif

