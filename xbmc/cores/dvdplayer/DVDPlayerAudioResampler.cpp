/*
 *      Copyright (C) 2005-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "DVDPlayerAudioResampler.h"
#include "DVDPlayerAudio.h"
#include "utils/log.h"
#include "utils/MathUtils.h"

#ifdef _WIN32
#pragma comment(lib, "libsamplerate-0.lib")
#endif

CDVDPlayerResampler::CDVDPlayerResampler()
{
  m_nrchannels = -1;
  m_converter = NULL;
  
  memset(&m_converterdata, 0, sizeof(m_converterdata));
  m_converterdata.src_ratio = 1.0;
  
  m_quality = SRC_LINEAR;
  m_ratio = 1.0;

  m_buffer = NULL;
  m_ptsbuffer = NULL;
  m_buffersize = 0;
  m_bufferfill = 0;
}

CDVDPlayerResampler::~CDVDPlayerResampler()
{
  Clean();
}

void CDVDPlayerResampler::Add(DVDAudioFrame &audioframe, double pts)
{
  //check if nr of channels changed so we can allocate new buffers if necessary
  CheckResampleBuffers(audioframe.channel_count);

  //value to divide samples by to get them into -1.0:1.0 range
  float scale = (float)(1 << (audioframe.bits_per_sample - 1));
  int   nrframes = audioframe.size / audioframe.channel_count / (audioframe.bits_per_sample / 8);

  //resize sample buffer if necessary
  //we want the buffer to be large enough to hold the current frames in it,
  //the number of frames needed for libsamplerate's input
  //and the maximum number of frames libsamplerate might generate, times 2 for safety
  ResizeSampleBuffer(m_bufferfill + nrframes + nrframes * MathUtils::round_int(m_ratio + 0.5) * 2);

  //assign samplebuffers
  m_converterdata.input_frames = nrframes;
  m_converterdata.output_frames = m_buffersize - m_bufferfill - m_converterdata.input_frames;
  //output buffer starts at the place where the buffer doesn't hold samples
  m_converterdata.data_out = m_buffer + m_bufferfill * m_nrchannels;
  //intput buffer is a block of data at the end of the buffer
  m_converterdata.data_in = m_converterdata.data_out + m_converterdata.output_frames * m_nrchannels;

  //add samples to the resample input buffer
  int16_t* inputptr  = (int16_t*)audioframe.data;
  float*   outputptr = m_converterdata.data_in;

  for (int i = 0; i < nrframes * m_nrchannels; i++)
    *outputptr++ = (float)*inputptr++ / scale;

  //resample
  m_converterdata.src_ratio = m_ratio;
  src_set_ratio(m_converter, m_ratio);
  src_process(m_converter, &m_converterdata);

  //calculate a pts for each sample
  for (int i = 0; i < m_converterdata.output_frames_gen; i++)
  {
    m_ptsbuffer[m_bufferfill] = pts + i * (audioframe.duration / (double)m_converterdata.output_frames_gen);
    m_bufferfill++;
  }
}

bool CDVDPlayerResampler::Retrieve(DVDAudioFrame &audioframe, double &pts)
{
  //check if nr of channels changed so we can allocate new buffers if necessary
  CheckResampleBuffers(audioframe.channel_count);

  //value to divide samples by to get them into -1.0:1.0 range
  float scale = (float)(1 << (audioframe.bits_per_sample - 1));
  int   nrframes = audioframe.size / audioframe.channel_count / (audioframe.bits_per_sample / 8);

  //if we don't have enough in the samplebuffer, return false
  if (nrframes > m_bufferfill)
  {
    return false;
  }

  //use the pts of the first fresh value in the samplebuffer
  pts = m_ptsbuffer[0];

  //add from samplebuffer to audioframe
  float*   inputptr  = m_buffer;
  int16_t* outputptr = (int16_t*)audioframe.data;

  for (int i = 0; i < nrframes * m_nrchannels; i++)
    *outputptr++ = MathUtils::round_int(Clamp(*inputptr++ * scale, scale * -1.0f, scale - 1.0f));

  m_bufferfill -= nrframes;

  //shift old data to the beginning of the buffer
  memmove(m_buffer, m_buffer + (nrframes * m_nrchannels), m_bufferfill * m_nrchannels * sizeof(float));
  memmove(m_ptsbuffer, m_ptsbuffer + nrframes, m_bufferfill * sizeof(double));

  return true;
}

void CDVDPlayerResampler::CheckResampleBuffers(int channels)
{
  int error;
  if (channels != m_nrchannels)
  {
    Clean();

    m_nrchannels = channels;
    m_converter = src_new(m_quality, m_nrchannels, &error);
  }
}

void CDVDPlayerResampler::ResizeSampleBuffer(int nrframes)
{
  if (m_buffersize < nrframes)
  {
    m_buffersize = nrframes * 2;
    m_buffer = (float*)realloc(m_buffer, m_buffersize * m_nrchannels * sizeof(float));
    m_ptsbuffer = (double*)realloc(m_ptsbuffer, m_buffersize * sizeof(double));
    CLog::Log(LOGDEBUG, "CDVDPlayerResampler: resized buffers to hold %i frames", m_buffersize);
  }
}

void CDVDPlayerResampler::SetRatio(double ratio)
{
  m_ratio = Clamp(ratio, 1.0 / (double)MAXRATIO, (double)MAXRATIO);
}

void CDVDPlayerResampler::Flush()
{
  m_bufferfill = 0;
}

void CDVDPlayerResampler::SetQuality(int quality)
{
  int qualitylookup[] = {SRC_LINEAR, SRC_SINC_FASTEST, SRC_SINC_MEDIUM_QUALITY, SRC_SINC_BEST_QUALITY};
  m_quality = qualitylookup[Clamp(quality, 0, 3)];
  Clean();
}

void CDVDPlayerResampler::Clean()
{
  if (m_converter) src_delete(m_converter);
  m_converter = NULL;

  free(m_buffer);
  m_buffer = NULL;
  free(m_ptsbuffer);
  m_ptsbuffer = NULL;

  m_bufferfill = 0;
  m_buffersize = 0;

  m_nrchannels = -1;
  m_converterdata.end_of_input = 0;
  m_converterdata.src_ratio = 1.0;
  m_ratio = 1.0;
}
