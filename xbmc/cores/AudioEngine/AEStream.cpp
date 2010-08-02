/*
 *      Copyright (C) 2005-2010 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "utils/SingleLock.h"

#include "AEStream.h"
#include "AEUtil.h"

CAEStream::CAEStream(enum AEDataFormat dataFormat, unsigned int sampleRate, unsigned int channelCount, AEChLayout channelLayout):
  m_convertBuffer  (NULL),
  m_valid          (true),
  m_convertFn      (NULL),
  m_frameBuffer    (NULL),
  m_frameBufferSize(0   ),
  m_ssrc           (NULL),
  m_framesBuffered (0   ),
  m_paused         (0   )
{
  m_bytesPerFrame          = (CAEUtil::DataFormatToBits(dataFormat) >> 3) * channelCount;
  m_aeChannelCount         = AE.GetChannelCount();
  m_aePacketSamples        = AE.GetFrames() * m_aeChannelCount;

  /* if no layout provided, guess one */
  if (channelLayout == NULL) {
    static AEChannel guess[2][3] = {
      {AE_CH_FC, AE_CH_NULL},
      {AE_CH_FL, AE_CH_FR, AE_CH_NULL}
    };
    
    if (channelCount > 0 && channelCount <= 2)
      channelLayout = guess[channelCount-1];
    else {
      m_valid = false;
      return;
    }
  }
  
  m_format.m_dataFormat    = dataFormat;
  m_format.m_sampleRate    = sampleRate;
  m_format.m_channelCount  = channelCount;
  m_format.m_channelLayout = channelLayout;
  m_format.m_frames        = AE.GetFrames();
  m_format.m_frameSamples  = m_format.m_frames * channelCount;
  m_format.m_frameSize     = m_format.m_frames * m_bytesPerFrame;

  if (!m_remap.Initialize(channelLayout, AE.GetChannelLayout(), false))
  {
    m_valid = false;
    return;
  }

  m_newPacket.samples = 0;
  m_newPacket.data    = new float[m_format.m_frameSamples];
  m_packet.samples    = 0;
  m_packet.data       = NULL;

  m_frameBuffer   = new uint8_t[m_format.m_frameSize];
  m_resample      = sampleRate != AE.GetSampleRate();
  m_convert       = dataFormat != AE_FMT_FLOAT;

  /* if we need to convert, set it up */
  if (m_convert)
  {
    /* get the conversion function and allocate a buffer for the data */
    m_convertFn = CAEConvert::ToFloat(dataFormat);
    if (m_convertFn) m_convertBuffer = new float[m_format.m_frameSamples];
    else             m_valid         = false;
  }
  else
    m_convertBuffer = (float*)m_frameBuffer;

  /* if we need to resample, set it up */
  if (m_resample)
  {
    int err;
    m_ssrc                   = src_new(SRC_SINC_MEDIUM_QUALITY, channelCount, &err);
    m_ssrcData.data_in       = m_convertBuffer;
    m_ssrcData.input_frames  = m_format.m_frames;
    m_ssrcData.data_out      = new float[m_format.m_frameSamples * 2];
    m_ssrcData.output_frames = m_format.m_frames * 2;
    m_ssrcData.src_ratio     = (double)AE.GetSampleRate() / (double)sampleRate;
    m_ssrcData.end_of_input  = 0;
  }
}

CAEStream::~CAEStream()
{
  CSingleLock lock(m_critSection);
  m_valid = 0;
  lock.Leave();
  
  Flush();
  AE.RemoveStream(this);
  delete[] m_frameBuffer;
  if (m_convert)
    delete[] m_convertBuffer;

  if (m_resample)
  {
    delete[] m_ssrcData.data_out;
    src_delete(m_ssrc);
    m_ssrc = NULL;
  }

  delete[] m_newPacket.data;
}

unsigned int CAEStream::AddData(void *data, unsigned int size)
{
  CSingleLock lock(m_critSection);
  if (!m_valid || size == 0 || data == NULL) return 0;  

  /* only buffer up to 1 second of data */
  if (m_framesBuffered >= AE.GetSampleRate())
    return 0;

  uint8_t *ptr = (uint8_t*)data;
  while(size)
  {
    size_t room = m_format.m_frameSize - m_frameBufferSize;
    size_t copy = size > room ? room : size;
    if (copy == 0)
      return ptr - (uint8_t*)data;

    memcpy(&m_frameBuffer[m_frameBufferSize], ptr, copy);
    size              -= copy;
    m_frameBufferSize += copy;
    ptr               += copy;

    unsigned int consumed = ProcessFrameBuffer();
    if (consumed)
    {
      memmove(&m_frameBuffer[consumed], m_frameBuffer, m_frameBufferSize - consumed);
      m_frameBufferSize -= consumed;
    }
  }

  return ptr - (uint8_t*)data;
}

unsigned int CAEStream::ProcessFrameBuffer()
{
  float	      *data;
  unsigned int frames, samples, consumed;

  /* convert the data if we need to */
  if (m_convert)
    m_convertFn(m_frameBuffer, m_format.m_frameSamples, m_convertBuffer);

  /* resample it if we need to */
  if (m_resample) {
    m_ssrcData.input_frames = m_frameBufferSize / m_bytesPerFrame;
    if (src_process(m_ssrc, &m_ssrcData) != 0) return 0;
    data     = m_ssrcData.data_out;
    frames   = m_ssrcData.output_frames_gen;
    consumed = m_ssrcData.input_frames_used * m_bytesPerFrame;
    if (!frames)
      return consumed;
  }
  else
  {
    data     = m_convertBuffer;
    frames   = m_frameBufferSize / m_bytesPerFrame;
    consumed = m_frameBufferSize;
  }

  /* buffer the data */
  samples = frames * m_format.m_channelCount;
  m_framesBuffered += frames;

  while(samples)
  {
    unsigned int room = m_format.m_frameSamples - m_newPacket.samples;
    unsigned int copy = room > samples ? samples : room;

    memcpy(&m_newPacket.data[m_newPacket.samples], data, copy * sizeof(float));
    data                += copy;
    m_newPacket.samples += copy;
    samples             -= copy;

    /* if we have a full block of data */
    if (m_newPacket.samples == m_format.m_frameSamples)
    {
      PPacket pkt;
      pkt.samples = m_aePacketSamples;
      pkt.data    = new float[m_aePacketSamples];

      /* downmix/remap the data */
      m_remap.Remap(m_newPacket.data, pkt.data, m_format.m_frames);

      m_outBuffer.push_back(pkt);
      m_newPacket.samples = 0;
    }
  }

  return consumed;
}

float* CAEStream::GetFrame()
{
  CSingleLock lock(m_critSection);

  /* if the packet is empty, advance to the next one */
  if(!m_packet.samples)
  {
    delete[] m_packet.data;
    m_packet.data = NULL;
    
    /* no more packets, return null */
    if (m_outBuffer.empty())
      return NULL;

    /* get the next packet */
    m_packet = m_outBuffer.front();
    m_outBuffer.pop_front();

    m_packetPos = m_packet.data;
  }
  
  /* fetch one frame of data */
  float *ret        = m_packetPos;
  m_packet.samples -= m_aeChannelCount;
  m_packetPos      += m_aeChannelCount;

  --m_framesBuffered;
  return ret;
}

float CAEStream::GetDelay()
{
  float frames;
  /* TODO: make me thread safe */
  CSingleLock lock(m_critSection);
  frames = m_framesBuffered;
  lock.Leave();

  return AE.GetDelay() + (frames / AE.GetSampleRate());
}

void CAEStream::Drain()
{
  /* FIXME */
}

void CAEStream::Flush()
{
  CSingleLock lock(m_critSection);
  /* reset the resampler */
  if (m_resample) {
    m_ssrcData.end_of_input = 0;
    src_reset(m_ssrc);
  }
  
  /* invalidate any incoming samples */
  m_newPacket.samples = 0;
  
  /*
    clear the current buffered packet we cant delete the data as it may be
    in use by the AE thread, so we just set the packet count to 0, it will
    get freed by the next call to GetFrame or destruction
  */
  m_packet.samples    = 0;

  /* clear any other buffered packets */
  while(!m_outBuffer.empty()) {    
    PPacket p = m_outBuffer.front();
    m_outBuffer.pop_front();    
    delete[] p.data;
    p.data = NULL;    
  };
  
  /* reset our counts */
  m_frameBufferSize = 0;
  m_framesBuffered  = 0;
}

void CAEStream::SetVolume(int volume)
{
  //FIXME
}

void CAEStream::SetDynamicRangeCompression(int drc)
{
  //FIXME
}
