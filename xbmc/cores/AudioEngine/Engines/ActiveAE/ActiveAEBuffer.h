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

#include "cores/AudioEngine/Utils/AEAudioFormat.h"
#include "cores/AudioEngine/Interfaces/AE.h"
#include <deque>

extern "C" {
#include "libavutil/avutil.h"
#include "libswresample/swresample.h"
}

namespace ActiveAE
{

struct SampleConfig
{
  AVSampleFormat fmt;
  uint64_t channel_layout;
  int channels;
  int sample_rate;
  int bits_per_sample;
  int dither_bits;
};

/**
 * the variables here follow ffmpeg naming
 */
class CSoundPacket
{
public:
  CSoundPacket(SampleConfig conf, int samples);
  ~CSoundPacket();
  uint8_t **data;                        // array with pointers to planes of data
  SampleConfig config;
  int bytes_per_sample;                  // bytes per sample and per channel
  int linesize;                          // see ffmpeg, required for planar formats
  int planes;                            // 1 for non planar formats, #channels for planar
  int nb_samples;                        // number of frames used
  int max_nb_samples;                    // max number of frames this packet can hold
};

class CActiveAEBufferPool;

class CSampleBuffer
{
public:
  CSampleBuffer();
  ~CSampleBuffer();
  CSampleBuffer *Acquire();
  void Return();
  CSoundPacket *pkt;
  CActiveAEBufferPool *pool;
  int64_t timestamp;
  int clockId;
  int pkt_start_offset;
  int refCount;
};

class CActiveAEBufferPool
{
public:
  CActiveAEBufferPool(AEAudioFormat format);
  virtual ~CActiveAEBufferPool();
  virtual bool Create(unsigned int totaltime);
  CSampleBuffer *GetFreeBuffer();
  void ReturnBuffer(CSampleBuffer *buffer);
  AEAudioFormat m_format;
  std::deque<CSampleBuffer*> m_allSamples;
  std::deque<CSampleBuffer*> m_freeSamples;
};

class IAEResample;

class CActiveAEBufferPoolResample : public CActiveAEBufferPool
{
public:
  CActiveAEBufferPoolResample(AEAudioFormat inputFormat, AEAudioFormat outputFormat, AEQuality quality);
  virtual ~CActiveAEBufferPoolResample();
  virtual bool Create(unsigned int totaltime, bool remap, bool upmix, bool normalize = true);
  void ChangeResampler();
  bool ResampleBuffers(int64_t timestamp = 0);
  float GetDelay();
  void Flush();
  AEAudioFormat m_inputFormat;
  std::deque<CSampleBuffer*> m_inputSamples;
  std::deque<CSampleBuffer*> m_outputSamples;
  CSampleBuffer *m_procSample;
  IAEResample *m_resampler;
  uint8_t *m_planes[16];
  bool m_fillPackets;
  bool m_drain;
  bool m_empty;
  bool m_changeResampler;
  bool m_forceResampler;
  double m_resampleRatio;
  AEQuality m_resampleQuality;
  bool m_stereoUpmix;
  bool m_normalize;
  bool m_remap;
  int64_t m_lastSamplePts;
};

}
