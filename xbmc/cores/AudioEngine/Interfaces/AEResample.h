/*
 *  Copyright (C) 2010-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/AudioEngine/Interfaces/AE.h"

extern "C" {
#include "libavutil/samplefmt.h"
}

namespace ActiveAE
{

class IAEResample
{
public:
  /* return the name of this sync for logging */
  virtual const char *GetName() = 0;
  IAEResample() = default;
  virtual ~IAEResample() = default;
  virtual bool Init(uint64_t dst_chan_layout, int dst_channels, int dst_rate, AVSampleFormat dst_fmt, int dst_bits, int dst_dither, uint64_t src_chan_layout, int src_channels, int src_rate, AVSampleFormat src_fmt, int src_bits, int src_dither, bool upmix, bool normalize, CAEChannelInfo *remapLayout, AEQuality quality, bool force_resample) = 0;
  virtual int Resample(uint8_t **dst_buffer, int dst_samples, uint8_t **src_buffer, int src_samples, double ratio) = 0;
  virtual int64_t GetDelay(int64_t base) = 0;
  virtual int GetBufferedSamples() = 0;
  virtual bool WantsNewSamples(int samples) = 0;
  virtual int CalcDstSampleCount(int src_samples, int dst_rate, int src_rate) = 0;
  virtual int GetSrcBufferSize(int samples) = 0;
  virtual int GetDstBufferSize(int samples) = 0;
};

}
