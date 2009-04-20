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

#include "stdafx.h"
#include "DVDPlayerAudioResampler.h"
#include "DVDPlayerAudio.h"

CDVDPlayerResampler::CDVDPlayerResampler()
{
  m_NrChannels = -1;
  m_Converter = NULL;
  m_ConverterData.data_in = NULL;
  m_ConverterData.data_out = NULL;
  m_ConverterData.end_of_input = 0;
  m_ConverterData.src_ratio = 1.0;
  m_RingBufferPos = 0;
  m_RingBufferFill = 0;
  m_RingBuffer = NULL;
  m_PtsRingBuffer = NULL;
  m_Quality = SRC_LINEAR;
}
  
CDVDPlayerResampler::~CDVDPlayerResampler()
{
  Clean();
}

void CDVDPlayerResampler::Add(DVDAudioFrame &audioframe, double pts)
{
  int Value, Bytes = audioframe.bits_per_sample / 8;
  int AddPos;
  //check if nr of channels changed so we can allocate new buffers if necessary
  CheckResampleBuffers(audioframe.channels);
  
  //calculate how many samples the audioframe has, if it's more than can fit in the resamplebuffers, use less
  //this will sound horrible, but at least there's no segfault.
  //resample buffers are huge anyway
  int NrSamples = audioframe.size / audioframe.channels / (audioframe.bits_per_sample / 8);
  if (NrSamples > MAXCONVSAMPLES) NrSamples = MAXCONVSAMPLES;
  
  //add samples to the resample input buffer
  for (int i = 0; i < NrSamples; i++)
  {
    for (int j = 0; j < m_NrChannels; j++)
    {
      Value = 0;
      for (int k = Bytes - 1; k >= 0; k--)
      {
        Value <<= 8;
        Value |= audioframe.data[i * m_NrChannels * Bytes + j * Bytes + k];
      }
      //check sign bit
      if (Value & (1 << (Bytes * 8 - 1)))
      {
        Value |= ~((1 << (Bytes * 8)) - 1);
      }
      //add to resampler buffer
      m_ConverterData.data_in[i * m_NrChannels + j] = (float)Value / (float)(unsigned int)(1 << (Bytes * 8 - 1));
    }
  }
  
  //resample
  m_ConverterData.input_frames = NrSamples;
  src_process(m_Converter, &m_ConverterData);
  
  //add samples to ringbuffer
  AddPos = m_RingBufferPos + m_RingBufferFill;
  if (AddPos >= RINGSIZE) AddPos -= RINGSIZE;
  
  for (int i = 0; i < m_ConverterData.output_frames_gen; i++)
  {
    for (int j = 0; j < m_NrChannels; j++)
    {
      m_RingBuffer[AddPos * m_NrChannels + j] = m_ConverterData.data_out[i * m_NrChannels + j];
    }
    
    //calculate a pts for each sample
    m_PtsRingBuffer[AddPos] = pts + i * (audioframe.duration / (double)m_ConverterData.output_frames_gen);

    m_RingBufferFill++;
    AddPos++;
    if (AddPos >= RINGSIZE) AddPos -= RINGSIZE;
  }
}

bool CDVDPlayerResampler::Retreive(DVDAudioFrame &audioframe, double &pts)
{
  int i, NrSamples = audioframe.size / audioframe.channels / (audioframe.bits_per_sample / 8);
  int Value, GetPos, Bytes = audioframe.bits_per_sample / 8;
  float Multiply = (float)(1 << (audioframe.bits_per_sample - 1)) - 1.0;
  
  CheckResampleBuffers(audioframe.channels);
  
  //if we don't have enough in the ringbuffer, return false
  if (NrSamples > m_RingBufferFill)
  {
    return false;
  }
  
  //use the pts of the first fresh value in the ringbuffer
  pts = m_PtsRingBuffer[m_RingBufferPos];
  
  //add from ringbuffer to audioframe
  for (i = 0; i < NrSamples; i++)
  {
    GetPos = m_RingBufferPos + i;
    if (GetPos >= RINGSIZE) GetPos -= RINGSIZE;
    
    for (int j = 0; j < m_NrChannels; j++)
    {
      Value = (int)Clamp(m_RingBuffer[GetPos * m_NrChannels + j] * Multiply, Multiply * -1, Multiply);
      for (int k = 0; k < Bytes; k++)
      {
        audioframe.data[i * m_NrChannels * Bytes + j * Bytes + k] = (BYTE)((Value >> (k * 8)) & 0xFF);
      }
    }
    m_RingBufferFill--;
  }
  m_RingBufferPos += i;
  if (m_RingBufferPos >= RINGSIZE) m_RingBufferPos -= RINGSIZE;
  
  return true;
}

void CDVDPlayerResampler::CheckResampleBuffers(int channels)
{
  int error;
  if (channels != m_NrChannels)
  {
    Clean();
    
    m_NrChannels = channels;
    m_Converter = src_new(m_Quality, m_NrChannels, &error);
    m_ConverterData.data_in = new float[MAXCONVSAMPLES * m_NrChannels];
    m_ConverterData.data_out = new float[MAXCONVSAMPLES * m_NrChannels * 3];
    m_ConverterData.output_frames = MAXCONVSAMPLES * 3;
    m_RingBuffer = new float[RINGSIZE * m_NrChannels];
    m_RingBufferPos = 0;
    m_RingBufferFill = 0;
    
    m_PtsRingBuffer = new double[RINGSIZE];
  }
}

void CDVDPlayerResampler::SetRatio(double ratio)
{
  src_set_ratio(m_Converter, Clamp(ratio, 0.3, 2.9));
}

void CDVDPlayerResampler::Flush()
{
  m_RingBufferPos = 0;
  m_RingBufferFill = 0;
}

void CDVDPlayerResampler::SetQuality(int Quality)
{
  int QualityLookup[] = {SRC_LINEAR, SRC_SINC_FASTEST, SRC_SINC_MEDIUM_QUALITY, SRC_SINC_BEST_QUALITY};
  m_Quality = QualityLookup[Clamp(Quality, 0, 3)];
  Clean();
}

void CDVDPlayerResampler::Clean()
{
  if (m_Converter) src_delete(m_Converter);
  if (m_ConverterData.data_in) delete m_ConverterData.data_in;
  if (m_ConverterData.data_out) delete m_ConverterData.data_out;
  if (m_RingBuffer) delete m_RingBuffer;
  if (m_PtsRingBuffer) delete m_PtsRingBuffer;
  
  m_NrChannels = -1;
  m_Converter = NULL;
  m_ConverterData.data_in = NULL;
  m_ConverterData.data_out = NULL;
  m_ConverterData.end_of_input = 0;
  m_ConverterData.src_ratio = 1.0;
  m_RingBufferPos = 0;
  m_RingBufferFill = 0;
  m_RingBuffer = NULL;
  m_PtsRingBuffer = NULL;
}
