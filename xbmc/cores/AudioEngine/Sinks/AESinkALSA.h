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

private:
  CAEChannelInfo GetChannelLayoutRaw(const AEAudioFormat& format);
  CAEChannelInfo GetChannelLayoutLegacy(const AEAudioFormat& format, unsigned int minChannels, unsigned int maxChannels);
  CAEChannelInfo GetChannelLayout(const AEAudioFormat& format, unsigned int channels);

  static AEChannel ALSAChannelToAEChannel(unsigned int alsaChannel);
  static unsigned int AEChannelToALSAChannel(AEChannel aeChannel);
  static CAEChannelInfo ALSAchmapToAEChannelMap(snd_pcm_chmap_t* alsaMap);
  static snd_pcm_chmap_t* AEChannelMapToALSAchmap(const CAEChannelInfo& info);
  static snd_pcm_chmap_t* CopyALSAchmap(snd_pcm_chmap_t* alsaMap);
  static std::string ALSAchmapToString(snd_pcm_chmap_t* alsaMap);
  static CAEChannelInfo GetAlternateLayoutForm(const CAEChannelInfo& info);
  snd_pcm_chmap_t* SelectALSAChannelMap(const CAEChannelInfo& info);

  void GetAESParams(const AEAudioFormat& format, std::string& params);
  void HandleError(const char* name, int err);

  std::string m_initDevice;
  AEAudioFormat m_initFormat;
  AEAudioFormat m_format;
  unsigned int m_bufferSize = 0;
  double m_formatSampleRateMul = 0.0;
  bool m_passthrough = false;
  std::string m_device;
  snd_pcm_t *m_pcm;
  int m_timeout = 0;
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

