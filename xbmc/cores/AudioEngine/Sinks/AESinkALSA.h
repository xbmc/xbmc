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
#include "threads/CriticalSection.h"

#include <stdint.h>

#include <alsa/asoundlib.h>

#define AE_MIN_PERIODSIZE 256

// linux/amlogic/media/sound/spdif_info.h
enum IEC958_mode_codec {
  STEREO_PCM, DTS_RAW_MODE, DOLBY_DIGITAL, DTS, DD_PLUS, DTS_HD, MULTI_CHANNEL_LPCM, TRUEHD, _DTS_HD_MA, HIGH_SR_STEREO_LPCM,
  CODEC_CNT
};

// sound/soc/amlogic/auge/spdif_hw.h
enum spdif_id {
  HDMITX_SRC_SPDIF, HDMITX_SRC_SPDIF_B,
  HDMITX_SRC_NUM
};

class CAESinkALSA : public IAESink
{
public:
  const char *GetName() override { return "ALSA"; }

  CAESinkALSA();
  ~CAESinkALSA() override;

  static void Register();
  static std::unique_ptr<IAESink> Create(std::string& device, AEAudioFormat& desiredFormat);
  static void EnumerateDevicesEx(AEDeviceInfoList &list, bool force = false);
  static void Cleanup();

  bool Initialize(AEAudioFormat &format, std::string &device) override;
  void Deinitialize() override;

  virtual void Stop ();
  void GetDelay(AEDelayStatus& status) override;
  double GetCacheTotal() override;
  unsigned int AddPackets(uint8_t **data, unsigned int frames, unsigned int offset) override;
  void Drain() override;
  void Flush() override;

private:
  CAEChannelInfo GetChannelLayoutRaw(const AEAudioFormat& format) const;
  CAEChannelInfo GetChannelLayoutLegacy(const AEAudioFormat& format, unsigned int minChannels, unsigned int maxChannels) const;
  CAEChannelInfo GetChannelLayout(const AEAudioFormat& format, unsigned int channels);

  static AEChannel ALSAChannelToAEChannel(unsigned int alsaChannel);
  static unsigned int AEChannelToALSAChannel(AEChannel aeChannel);
  static CAEChannelInfo ALSAchmapToAEChannelMap(const snd_pcm_chmap_t* alsaMap);
  static snd_pcm_chmap_t* AEChannelMapToALSAchmap(const CAEChannelInfo& info);
  static snd_pcm_chmap_t* CopyALSAchmap(const snd_pcm_chmap_t* alsaMap);
  static std::string ALSAchmapToString(const snd_pcm_chmap_t* alsaMap);
  static unsigned int ALSAchmapActiveCount(const snd_pcm_chmap_t& chmap);
  static CAEChannelInfo GetAlternateLayoutForm(const CAEChannelInfo& info);
  snd_pcm_chmap_t* SelectALSAChannelMap(const CAEChannelInfo& info) const;

  void aml_configure_simple_control(std::string &device, const enum IEC958_mode_codec codec);

  void GetAESParams(const AEAudioFormat& format, std::string& params) const;
  void HandleError(const char* name, int err);

  AEAudioFormat m_format;
  unsigned int m_bufferSize = 0;
  double m_formatSampleRateMul = 0.0;
  bool m_passthrough = false;
  bool m_isAmlDevice = false;
  std::string m_device;
  snd_pcm_t *m_pcm;
  int m_timeout = 0;

  // cached SW parameters (used in ApplySwParams())
  unsigned int m_swSampleRate = 0;
  snd_pcm_uframes_t m_swAvailMin = 0;
  // support fragmentation, e.g. looping in the sink to get a certain amount of data onto the device
  bool m_fragmented = false;
  unsigned int m_originalPeriodSize = AE_MIN_PERIODSIZE;

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
  bool ApplySwParams();

  static void AppendParams(std::string &device, std::string_view params);
  static bool TryDevice(const std::string &name, snd_pcm_t **pcmp, snd_config_t *lconf);
  static bool TryDeviceWithParams(const std::string &name, const std::string &params, snd_pcm_t **pcmp, snd_config_t *lconf);
  static bool OpenPCMDevice(const std::string &name, const std::string &params, int channels, snd_pcm_t **pcmp, snd_config_t *lconf);

  static AEDeviceType AEDeviceTypeFromName(std::string_view name);
  static std::string GetParamFromName(const std::string &name, const std::string &param);
  static void EnumerateDevice(AEDeviceInfoList &list, const std::string &device, const std::string &description, snd_config_t *config);
  static bool GetELD(snd_hctl_t *hctl, int device, CAEDeviceInfo& info, bool& badHDMI);

  static void sndLibErrorHandler(const char *file, int line, const char *function, int err, const char *fmt, ...);
};

