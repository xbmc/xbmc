/*
 *      Copyright (C) 2005-2008 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "DVDPlayerAudioResampler.h"
#include "DVDPlayerAudio.h"
#include "utils/log.h"
#include "MathUtils.h"

CDVDPlayerResampler::CDVDPlayerResampler()
{
  m_nrchannels = -1;
  m_converter = NULL;
  m_converterdata.end_of_input = 0;
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
  CheckResampleBuffers(audioframe.channels);
  
  int   bytes = audioframe.bits_per_sample / 8;
  float scale = (float)(1LL << ((int64_t)bytes * 8LL - 1)); //value to divide samples by to get them into -1.0:1.0 range

  int nrframes = (audioframe.size / audioframe.channels / bytes);
  
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
  for (int i = 0; i < nrframes; i++)
  {
    for (int j = 0; j < m_nrchannels; j++)
    {
      int value = 0;
      for (int k = bytes - 1; k >= 0; k--)
      {
        value <<= 8;
        value |= audioframe.data[i * m_nrchannels * bytes + j * bytes + k];
      }
      //check sign bit
      if (value & (1 << (bytes * 8 - 1)))
      {
        value |= ~((1 << (bytes * 8)) - 1);
      }
      //add to resampler buffer
      m_converterdata.data_in[i * m_nrchannels + j] = (float)value / scale;
    }
  }
  
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

bool CDVDPlayerResampler::Retreive(DVDAudioFrame &audioframe, double &pts)
{
  //check if nr of channels changed so we can allocate new buffers if necessary
  CheckResampleBuffers(audioframe.channels);

  int   bytes = audioframe.bits_per_sample / 8;
  int   nrframes = audioframe.size / audioframe.channels / bytes;
  float scale = (float)(1LL << ((int64_t)bytes * 8LL - 1)); //value to multiply samples by
  
  //if we don't have enough in the samplebuffer, return false
  if (nrframes > m_bufferfill)
  {
    return false;
  }
  
  //use the pts of the first fresh value in the samplebuffer
  pts = m_ptsbuffer[0];
  
  //add from ringbuffer to audioframe
  for (int i = 0; i < nrframes; i++)
  {
    for (int j = 0; j < m_nrchannels; j++)
    {
      double sample = Clamp(m_buffer[i * m_nrchannels + j] * scale, scale * -1.0f, scale - 1.0f);
      int    value = MathUtils::round_int(sample);
      for (int k = 0; k < bytes; k++)
      {
        audioframe.data[i * m_nrchannels * bytes + j * bytes + k] = (BYTE)((value >> (k * 8)) & 0xFF);
      }
    }
  }
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
