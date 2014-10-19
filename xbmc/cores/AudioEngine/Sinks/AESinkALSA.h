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
#ifdef HAS_ALSA

#include "cores/AudioEngine/Interfaces/AESink.h"
#include "cores/AudioEngine/Utils/AEDeviceInfo.h"
#include "cores/AudioEngine/Sinks/alsa/ALSADeviceMonitor.h"
#include "cores/AudioEngine/Sinks/alsa/ALSAHControlMonitor.h"
#include <stdint.h>

#define ALSA_PCM_NEW_HW_PARAMS_API
#include <alsa/asoundlib.h>

#include "threads/CriticalSection.h"

// ARGH... this is apparently needed to avoid FDEventMonitor
// being destructed before CALSA*Monitor below.
#include "linux/FDEventMonitor.h"

class CAESinkALSA : public IAESink
{
public:
  virtual const char *GetName() { return "ALSA"; }

  CAESinkALSA();
  virtual ~CAESinkALSA();

  virtual bool Initialize  (AEAudioFormat &format, std::string &device);
  virtual void Deinitialize();

  virtual void         Stop            ();
  virtual void         GetDelay        (AEDelayStatus& status);
  virtual double       GetCacheTotal   ();
  virtual unsigned int AddPackets      (uint8_t **data, unsigned int frames, unsigned int offset);
  virtual void         Drain           ();

  static void EnumerateDevicesEx(AEDeviceInfoList &list, bool force = false);
private:
  CAEChannelInfo GetChannelLayoutRaw(AEDataFormat dataFormat);
  CAEChannelInfo GetChannelLayoutLegacy(const AEAudioFormat& format, unsigned int minChannels, unsigned int maxChannels);
  CAEChannelInfo GetChannelLayout(const AEAudioFormat& format, unsigned int channels);

#ifdef SND_CHMAP_API_VERSION
  static bool AllowALSAMaps();
  static AEChannel ALSAChannelToAEChannel(unsigned int alsaChannel);
  static unsigned int AEChannelToALSAChannel(AEChannel aeChannel);
  static CAEChannelInfo ALSAchmapToAEChannelMap(snd_pcm_chmap_t* alsaMap);
  static snd_pcm_chmap_t* AEChannelMapToALSAchmap(const CAEChannelInfo& info);
  static snd_pcm_chmap_t* CopyALSAchmap(snd_pcm_chmap_t* alsaMap);
  static std::string ALSAchmapToString(snd_pcm_chmap_t* alsaMap);
  static CAEChannelInfo GetAlternateLayoutForm(const CAEChannelInfo& info);
  snd_pcm_chmap_t* SelectALSAChannelMap(const CAEChannelInfo& info);
#endif

  void           GetAESParams(const AEAudioFormat& format, std::string& params);
  void           HandleError(const char* name, int err);

  std::string       m_initDevice;
  AEAudioFormat     m_initFormat;
  AEAudioFormat     m_format;
  unsigned int      m_bufferSize;
  double            m_formatSampleRateMul;
  bool              m_passthrough;
  std::string       m_device;
  snd_pcm_t        *m_pcm;
  int               m_timeout;
  // support fragmentation, e.g. looping in the sink to get a certain amount of data onto the device
  bool              m_fragmented;
  unsigned int      m_originalPeriodSize;

#if HAVE_LIBUDEV
  static CALSADeviceMonitor m_deviceMonitor;
#endif
  static CALSAHControlMonitor m_controlMonitor;

  struct ALSAConfig
  {
    unsigned int sampleRate;
    unsigned int periodSize;
    unsigned int frameSize;
    unsigned int channels;
    AEDataFormat format;
  };

  static snd_pcm_format_t AEFormatToALSAFormat(const enum AEDataFormat format);

  bool InitializeHW(const ALSAConfig &inconfig, ALSAConfig &outconfig);
  bool InitializeSW(const ALSAConfig &inconfig);

  static void AppendParams(std::string &device, const std::string &params);
  static bool TryDevice(const std::string &name, snd_pcm_t **pcmp, snd_config_t *lconf);
  static bool TryDeviceWithParams(const std::string &name, const std::string &params, snd_pcm_t **pcmp, snd_config_t *lconf);
  static bool OpenPCMDevice(const std::string &name, const std::string &params, int channels, snd_pcm_t **pcmp, snd_config_t *lconf);

  static AEDeviceType AEDeviceTypeFromName(const std::string &name);
  static std::string GetParamFromName(const std::string &name, const std::string &param);
  static void EnumerateDevice(AEDeviceInfoList &list, const std::string &device, const std::string &description, snd_config_t *config);
  static bool SoundDeviceExists(const std::string& device);
  static bool GetELD(snd_hctl_t *hctl, int device, CAEDeviceInfo& info, bool& badHDMI);

  static void sndLibErrorHandler(const char *file, int line, const char *function, int err, const char *fmt, ...);
};
#endif

