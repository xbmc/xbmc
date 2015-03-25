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

#include "cores/AudioEngine/Interfaces/AEResample.h"
#include "linux/OMXCore.h"

namespace ActiveAE
{

class CActiveAEResamplePi : public IAEResample
{
public:
  const char *GetName() { return "ActiveAEResamplePi"; }
  CActiveAEResamplePi();
  virtual ~CActiveAEResamplePi();
  bool Init(uint64_t dst_chan_layout, int dst_channels, int dst_rate, AVSampleFormat dst_fmt, int dst_bits, int dst_dither, uint64_t src_chan_layout, int src_channels, int src_rate, AVSampleFormat src_fmt, int src_bits, int src_dither, bool upmix, bool normalize, CAEChannelInfo *remapLayout, AEQuality quality, bool force_resample);
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
