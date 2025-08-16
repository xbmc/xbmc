/*
 *  Copyright (C) 2010-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/AudioEngine/Utils/AEChannelInfo.h"
#include "cores/AudioEngine/Interfaces/AE.h"
#include "cores/AudioEngine/Interfaces/AEResample.h"

extern "C" {
#include <libavutil/samplefmt.h>
}

struct SwrContext;

namespace ActiveAE
{

class CActiveAEResampleFFMPEG : public IAEResample
{
public:
  const char *GetName() override { return "ActiveAEResampleFFMPEG"; }
  CActiveAEResampleFFMPEG();
  ~CActiveAEResampleFFMPEG() override;
  bool Init(SampleConfig dstConfig,
            SampleConfig srcConfig,
            bool upmix,
            bool normalize,
            double centerMix,
            CAEChannelInfo* remapLayout,
            AEQuality quality,
            bool force_resample,
            float sublevel) override;
  int Resample(uint8_t **dst_buffer, int dst_samples, uint8_t **src_buffer, int src_samples, double ratio) override;
  int64_t GetDelay(int64_t base) override;
  int GetBufferedSamples() override;
  bool WantsNewSamples(int samples) override { return GetBufferedSamples() <= samples * 2; }
  int CalcDstSampleCount(int src_samples, int dst_rate, int src_rate) override;
  int GetSrcBufferSize(int samples) override;
  int GetDstBufferSize(int samples) override;

protected:
  bool m_loaded;
  bool m_doesResample;
  uint64_t m_src_chan_layout, m_dst_chan_layout;
  int m_src_rate, m_dst_rate;
  int m_src_channels, m_dst_channels;
  AVSampleFormat m_src_fmt, m_dst_fmt;
  int m_src_bits, m_dst_bits;
  int m_src_dither_bits, m_dst_dither_bits;
  SwrContext *m_pContext;
  double m_rematrix[AE_CH_MAX][AE_CH_MAX];
};

}
