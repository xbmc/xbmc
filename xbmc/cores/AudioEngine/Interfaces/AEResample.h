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
  IAEResample() {}
  virtual ~IAEResample() {}
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
