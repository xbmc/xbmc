/*
 *  Copyright (C) 2010-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/AudioEngine/Interfaces/AEResample.h"
#include "platform/linux/OMXCore.h"

namespace ActiveAE
{

class CActiveAEResamplePi : public IAEResample
{
public:
  const char *GetName() { return "ActiveAEResamplePi"; }
  CActiveAEResamplePi();
  virtual ~CActiveAEResamplePi();
  bool Init(SampleConfig dstConfig, SampleConfig srcConfig, bool upmix, bool normalize, double centerMix,
            CAEChannelInfo *remapLayout, AEQuality quality, bool force_resample);
  int Resample(uint8_t **dst_buffer, int dst_samples, uint8_t **src_buffer, int src_samples, double ratio);
  int64_t GetDelay(int64_t base);
  int GetBufferedSamples();
  bool WantsNewSamples(int samples) { return GetBufferedSamples() <= samples; }
  int CalcDstSampleCount(int src_samples, int dst_rate, int src_rate);
  int GetSrcBufferSize(int samples);
  int GetDstBufferSize(int samples);

protected:
  void DeInit();
  uint64_t m_src_chan_layout, m_dst_chan_layout;
  int m_src_rate, m_dst_rate;
  int m_src_channels, m_dst_channels;
  AVSampleFormat m_src_fmt, m_dst_fmt;
  int m_src_bits, m_dst_bits;
  int m_src_pitch, m_dst_pitch;
  int m_src_dither_bits, m_dst_dither_bits;

  OMX_AUDIO_PARAM_PCMMODETYPE m_pcm_input;
  OMX_AUDIO_PARAM_PCMMODETYPE m_pcm_output;
  COMXCoreComponent    m_omx_mixer;
  bool                 m_Initialized;
  bool                 m_force_resample;
  OMX_BUFFERHEADERTYPE *m_encoded_buffer;
  unsigned int         m_offset;
  double               m_ratio;
};

}
