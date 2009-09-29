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

CDVDPlayerResampler::CDVDPlayerResampler()
{
  m_nrchannels = -1;
  m_converter = NULL;
  m_converterdata.data_in = NULL;
  m_converterdata.data_out = NULL;
  m_converterdata.end_of_input = 0;
  m_converterdata.src_ratio = 1.0;
  m_ringbufferpos = 0;
  m_ringbufferfill = 0;
  m_ringbuffer = NULL;
  m_ptsringbuffer = NULL;
  m_quality = SRC_LINEAR;
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
  float scale = (float)(1LL << ((int64_t)bytes * 8LL)); //value to divide samples by to get them into -1.0:1.0 range

  //calculate how many samples the audioframe has, if it's more than can fit in the resamplebuffers, use less
  //this will sound horrible, but at least there's no segfault.
  //resample buffers are huge anyway
  int nrsamples = (audioframe.size / audioframe.channels / bytes) % (MAXCONVSAMPLES + 1);
  
  //add samples to the resample input buffer
  for (int i = 0; i < nrsamples; i++)
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
  m_converterdata.input_frames = nrsamples;
  src_process(m_converter, &m_converterdata);
  
  //add samples to ringbuffer
  int addpos = (m_ringbufferpos + m_ringbufferfill) % RINGSIZE;
  
  for (int i = 0; i < m_converterdata.output_frames_gen; i++)
  {
    for (int j = 0; j < m_nrchannels; j++)
    {
      m_ringbuffer[addpos * m_nrchannels + j] = m_converterdata.data_out[i * m_nrchannels + j];
    }
    
    //calculate a pts for each sample
    m_ptsringbuffer[addpos] = pts + i * (audioframe.duration / (double)m_converterdata.output_frames_gen);

    m_ringbufferfill++;
    addpos = (addpos + 1) % RINGSIZE;
  }
}

bool CDVDPlayerResampler::Retreive(DVDAudioFrame &audioframe, double &pts)
{
  //check if nr of channels changed so we can allocate new buffers if necessary
  CheckResampleBuffers(audioframe.channels);

  int   bytes = audioframe.bits_per_sample / 8;
  int   nrsamples = audioframe.size / audioframe.channels / bytes;
  float scale = (float)(1LL << ((int64_t)bytes * 8LL)); //value to multiply samples by
  
  //if we don't have enough in the ringbuffer, return false
  if (nrsamples > m_ringbufferfill)
  {
    return false;
  }
  
  //use the pts of the first fresh value in the ringbuffer
  pts = m_ptsringbuffer[m_ringbufferpos];
  
  //add from ringbuffer to audioframe
  for (int i = 0; i < nrsamples; i++)
  {
    int getpos = (m_ringbufferpos + i) % RINGSIZE;
    
    for (int j = 0; j < m_nrchannels; j++)
    {
      int value = (int)Clamp(m_ringbuffer[getpos * m_nrchannels + j] * scale, scale * -1.0f, scale - 1.0f);
      for (int k = 0; k < bytes; k++)
      {
        audioframe.data[i * m_nrchannels * bytes + j * bytes + k] = (BYTE)((value >> (k * 8)) & 0xFF);
      }
    }
    m_ringbufferfill--;
  }
  m_ringbufferpos = (m_ringbufferpos + nrsamples - 1) % RINGSIZE;
  
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
    m_converterdata.data_in = new float[MAXCONVSAMPLES * m_nrchannels];
    m_converterdata.data_out = new float[MAXCONVSAMPLES * m_nrchannels * 3];
    m_converterdata.output_frames = MAXCONVSAMPLES * 3;
    m_ringbuffer = new float[RINGSIZE * m_nrchannels];
    m_ringbufferpos = 0;
    m_ringbufferfill = 0;
    
    m_ptsringbuffer = new double[RINGSIZE];
  }
}

void CDVDPlayerResampler::SetRatio(double ratio)
{
  src_set_ratio(m_converter, Clamp(ratio, 0.3, 2.9));
}

void CDVDPlayerResampler::Flush()
{
  m_ringbufferpos = 0;
  m_ringbufferfill = 0;
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
  if (m_converterdata.data_in) delete m_converterdata.data_in;
  if (m_converterdata.data_out) delete m_converterdata.data_out;
  if (m_ringbuffer) delete m_ringbuffer;
  if (m_ptsringbuffer) delete m_ptsringbuffer;
  
  m_nrchannels = -1;
  m_converter = NULL;
  m_converterdata.data_in = NULL;
  m_converterdata.data_out = NULL;
  m_converterdata.end_of_input = 0;
  m_converterdata.src_ratio = 1.0;
  m_ringbufferpos = 0;
  m_ringbufferfill = 0;
  m_ringbuffer = NULL;
  m_ptsringbuffer = NULL;
}
