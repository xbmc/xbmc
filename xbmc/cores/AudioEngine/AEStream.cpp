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
#include "utils/log.h"

#if (defined USE_EXTERNAL_FFMPEG)
  #include <libavutil/avutil.h>
#else
  #include "cores/dvdplayer/Codecs/ffmpeg/libavutil/avutil.h"
#endif

#include "AEStream.h"
#include "AEUtil.h"

using namespace std;

CAEStream::CAEStream(enum AEDataFormat dataFormat, unsigned int sampleRate, unsigned int channelCount, AEChLayout channelLayout, bool freeOnDrain, bool ownsPostProc):
  m_convertBuffer  (NULL ),
  m_valid          (false),
  m_delete         (false),
  m_volume         (1.0f ),
  m_rgain          (1.0f ),
  m_convertFn      (NULL ),
  m_frameBuffer    (NULL ),
  m_frameBufferSize(0    ),
  m_ssrc           (NULL ),
  m_framesBuffered (0    ),
  m_paused         (false),
  m_draining       (false),
  m_cbDataFunc     (NULL ),
  m_cbDrainFunc    (NULL ),
  m_cbDataArg      (NULL ),
  m_cbDrainArg     (NULL )
{
  m_ssrcData.data_out = NULL;

  m_initDataFormat    = dataFormat;
  m_initSampleRate    = sampleRate;
  m_initChannelCount  = channelCount;
  m_initChannelLayout = channelLayout;
  m_freeOnDrain       = freeOnDrain;
  m_ownsPostProc      = ownsPostProc;
  Initialize();
}

void CAEStream::Initialize()
{
  CSingleLock lock(m_critSection);
  if (m_valid)
  {
    Flush();
    _aligned_free(m_newPacket.data);
    _aligned_free(m_frameBuffer);

    if (m_convert)
      _aligned_free(m_convertBuffer);

    if (m_resample)
    {
      _aligned_free(m_ssrcData.data_out);
      m_ssrcData.data_out = NULL;
    }
  }

  enum AEDataFormat useDataFormat = m_initDataFormat;
  if (m_initDataFormat == AE_FMT_RAW)
  {
    /* we are raw, which means we need to work in the output format */
    useDataFormat       = AE.GetSinkDataFormat();
    m_initChannelLayout = AE.GetSinkChLayout  ();
    m_initChannelCount  = AE.GetSinkChCount   ();
  }
  else
  {
    /* no channel layout provided, so guess */
    if (!m_initChannelLayout)
    {
      m_initChannelLayout = CAEUtil::GuessChLayout(m_initChannelCount);
      if (!m_initChannelLayout)
      {
        m_valid = false;
        return;
      }
    }
  }

  m_bytesPerSample  = (CAEUtil::DataFormatToBits(useDataFormat) >> 3);
  m_bytesPerFrame   = m_bytesPerSample * m_initChannelCount;

  m_aeChannelCount  = AE.GetChannelCount();
  m_aePacketSamples = AE.GetFrames() * m_aeChannelCount;
  m_waterLevel      = AE.GetSampleRate() >> 3;

  m_format.m_dataFormat    = useDataFormat;
  m_format.m_sampleRate    = m_initSampleRate;
  m_format.m_channelCount  = m_initChannelCount;
  m_format.m_channelLayout = m_initChannelLayout;
  m_format.m_frames        = AE.GetFrames();
  m_format.m_frameSamples  = m_format.m_frames * m_initChannelCount;
  m_format.m_frameSize     = m_bytesPerFrame;

  if (m_initDataFormat != AE_FMT_RAW && !m_remap.Initialize(m_initChannelLayout, AE.GetChannelLayout(), false))
  {
    m_valid = false;
    return;
  }

  if (m_initDataFormat == AE_FMT_RAW)
    m_newPacket.data = (uint8_t*)_aligned_malloc(m_format.m_frames * m_format.m_frameSize, 16);
  else
    m_newPacket.data = (uint8_t*)_aligned_malloc(m_format.m_frameSamples * sizeof(float), 16);

  m_newPacket.samples = 0;
  m_packet.samples    = 0;
  m_packet.data       = NULL;

  m_frameBuffer   = (uint8_t*)_aligned_malloc(m_format.m_frameSize, 16);
  m_resample      = m_initSampleRate != AE.GetSampleRate() && m_initDataFormat != AE_FMT_RAW;
  m_convert       = m_initDataFormat != AE_FMT_FLOAT       && m_initDataFormat != AE_FMT_RAW;

  /* if we need to convert, set it up */
  if (m_convert)
  {
    /* get the conversion function and allocate a buffer for the data */
    CLog::Log(LOGDEBUG, "CAEStream::CAEStream - Converting from %s to AE_FMT_FLOAT", CAEUtil::DataFormatToStr(m_initDataFormat));
    m_convertFn = CAEConvert::ToFloat(m_initDataFormat);
    if (m_convertFn) m_convertBuffer = (float*)_aligned_malloc(m_format.m_frameSamples * sizeof(float), 16);
    else             m_valid         = false;
  }
  else
    m_convertBuffer = (float*)m_frameBuffer;

  /* if we need to resample, set it up */
  if (m_resample)
  {
    int err;
    m_ssrc                   = src_new(SRC_SINC_MEDIUM_QUALITY, m_initChannelCount, &err);
    m_ssrcData.data_in       = m_convertBuffer;
    m_ssrcData.input_frames  = m_format.m_frames;
    m_ssrcData.data_out      = (float*)_aligned_malloc(m_format.m_frameSamples * 2 * sizeof(float), 16);
    m_ssrcData.output_frames = m_format.m_frames * 2;
    m_ssrcData.src_ratio     = (double)AE.GetSampleRate() / (double)m_initSampleRate;
    m_ssrcData.end_of_input  = 0;
  }

  /* re-initialize post-proc objects */
  if (m_initDataFormat != AE_FMT_RAW)
  {
    list<IAEPostProc*>::iterator pitt;
    for(pitt = m_postProc.begin(); pitt != m_postProc.end();)
    {
      IAEPostProc *pp = *pitt;
      if (!pp->Initialize(this))
      {
        CLog::Log(LOGERROR, "CAEStream::CAEStream - Failed to re-initialize post-proc filter: %s", pp->GetName());
        pitt = m_postProc.erase(pitt);
        continue;
      }

      ++pitt;
    }
  }

  m_valid = true;
}

void CAEStream::Destroy()
{
  CSingleLock lock(m_critSection);
  m_valid  = false;
  m_delete = true;
}

CAEStream::~CAEStream()
{
  /* ensure that destroy was called first */
  ASSERT(m_delete);

  AE.RemoveStream(this);
  CSingleLock lock(m_critSection);

  /* de-init/free post-proc objects */
  while(!m_postProc.empty())
  {
    IAEPostProc *pp = m_postProc.front();
    m_postProc.pop_front();
    pp->DeInitialize();
    if (m_ownsPostProc) delete pp;
  }

  InternalFlush();
  _aligned_free(m_frameBuffer);
  if (m_convert)
    _aligned_free(m_convertBuffer);

  if (m_resample)
  {
    _aligned_free(m_ssrcData.data_out);
    src_delete(m_ssrc);
    m_ssrc = NULL;
  }

  _aligned_free(m_newPacket.data);
  CLog::Log(LOGDEBUG, "CAEStream::~CAEStream - Destructed");
}

void CAEStream::SetDataCallback(AECBFunc *cbFunc, void *arg)
{
  CSingleLock lock(m_critSection);
  m_cbDataFunc = cbFunc;
  m_cbDataArg  = arg;
}

void CAEStream::SetDrainCallback(AECBFunc *cbFunc, void *arg)
{
  CSingleLock lock(m_critSection);
  m_cbDrainFunc = cbFunc;
  m_cbDrainArg  = arg;
}

unsigned int CAEStream::AddData(void *data, unsigned int size)
{
  CSingleLock lock(m_critSection);
  if (!m_valid || size == 0 || data == NULL || m_draining) return 0;  

  /* only buffer up to 2x the water level */
  if (m_framesBuffered >= m_waterLevel << 1)
    return 0;

  uint8_t *ptr = (uint8_t*)data;
  while(size)
  {
    size_t room = m_format.m_frameSize - m_frameBufferSize;
    size_t copy = std::min((size_t)size, room);
    if (copy == 0)
      return ptr - (uint8_t*)data;

    memcpy(m_frameBuffer + m_frameBufferSize, ptr, copy);
    size              -= copy;
    m_frameBufferSize += copy;
    ptr               += copy;

    unsigned int consumed = ProcessFrameBuffer();
    if (consumed)
    {
      m_frameBufferSize -= consumed;
      memmove(m_frameBuffer + consumed, m_frameBuffer, m_frameBufferSize);
    }
  }

  return ptr - (uint8_t*)data;
}

unsigned int CAEStream::ProcessFrameBuffer()
{
  uint8_t     *data;
  unsigned int frames, consumed;

  /* convert the data if we need to */
  if (m_convert)
    m_convertFn(m_frameBuffer, m_format.m_frameSamples, m_convertBuffer);

  /* resample it if we need to */
  if (m_resample)
  {
    m_ssrcData.input_frames = m_frameBufferSize / m_bytesPerFrame;
    if (src_process(m_ssrc, &m_ssrcData) != 0) return 0;
    data     = (uint8_t*)m_ssrcData.data_out;
    frames   = m_ssrcData.output_frames_gen;
    consumed = m_ssrcData.input_frames_used * m_bytesPerFrame;
    if (!frames)
      return consumed;
  }
  else
  {
    data     = (uint8_t*)m_convertBuffer;
    frames   = m_frameBufferSize / m_bytesPerFrame;
    consumed = m_frameBufferSize;
  }

  /* buffer the data */
  unsigned int samples = frames * m_format.m_channelCount;
  m_framesBuffered += frames;

  while(samples)
  {
    unsigned int room = m_format.m_frameSamples - m_newPacket.samples;
    unsigned int copy = std::min(room, samples);

    if (m_initDataFormat == AE_FMT_RAW)
    {
      unsigned int size = copy * m_bytesPerSample;
      memcpy(m_newPacket.data + (m_newPacket.samples * m_bytesPerSample), data, size);
      data += size;
    }
    else
    {
      unsigned int size = copy * sizeof(float);
      memcpy((float*)m_newPacket.data + m_newPacket.samples, data, size);
      data += size;
    }

    m_newPacket.samples += copy;
    samples             -= copy;

    /* if we have a full block of data */
    if (m_newPacket.samples == m_format.m_frameSamples)
    {
      if (m_initDataFormat == AE_FMT_RAW)
      {
        m_outBuffer.push_back(m_newPacket);
        m_newPacket.samples = 0;
        m_newPacket.data    = (uint8_t*)_aligned_malloc(m_format.m_frames * m_format.m_frameSize, 16);
      }
      else
      {
        /* post-process the packet */
        list<IAEPostProc*>::iterator pitt;
        for(pitt = m_postProc.begin(); pitt != m_postProc.end(); ++pitt)
          (*pitt)->Process((float*)m_newPacket.data, m_format.m_frames);

        /* downmix/remap the data */
        PPacket pkt;
        pkt.samples = m_aePacketSamples;
        pkt.data    = (uint8_t*)_aligned_malloc(m_aePacketSamples * sizeof(float), 16);
        m_remap.Remap((float*)m_newPacket.data, (float*)pkt.data, m_format.m_frames);

        /* add the packet to the output */
        m_outBuffer.push_back(pkt);
        m_newPacket.samples = 0;
      }
    }
  }

  return consumed;
}

uint8_t* CAEStream::GetFrame()
{
  CSingleLock lock(m_critSection);
  if (m_delete) return NULL;

  /* if the packet is empty, advance to the next one */
  if(!m_packet.samples)
  {
    _aligned_free(m_packet.data);
    m_packet.data = NULL;
    
    /* no more packets, return null */
    if (m_outBuffer.empty())
    {
      if (m_draining)
      {
        /* if we are draining trigger the callback function */
        if (m_cbDrainFunc)
        {
          lock.Leave();
          m_cbDrainFunc(this, m_cbDrainArg, 0);
          m_cbDrainFunc = NULL;
        }
        return NULL;
      }
      else
      {
        /* otherwise ask for more data */
        if (m_cbDataFunc)
        {
          lock.Leave();
          m_cbDataFunc(this, m_cbDataArg, (m_format.m_frameSize - m_frameBufferSize) / m_bytesPerFrame);
          if (m_outBuffer.empty())
            return NULL;
        }
        else
          return NULL;
      }
    }

    /* get the next packet */
    m_packet = m_outBuffer.front();
    m_outBuffer.pop_front();

    m_packetPos = m_packet.data;
  }
  
  /* fetch one frame of data */
  uint8_t *ret      = (uint8_t*)m_packetPos;
  m_packet.samples -= m_aeChannelCount;
  if (m_initDataFormat == AE_FMT_RAW)
    m_packetPos += m_bytesPerFrame;
  else
    m_packetPos += m_aeChannelCount * sizeof(float);

  --m_framesBuffered;

  /* if we are draining */
  if (m_draining)
  {
    /* if we have drained trigger the callback function */
    if (m_framesBuffered == 0 && m_frameBufferSize == 0 && m_cbDrainFunc)
    {
      lock.Leave();
      m_cbDrainFunc(this, m_cbDrainArg, 0);
      m_cbDrainFunc = NULL;
    }
  }
  else
  {
    /* if the buffer is low, fill up again */ 
    if(m_cbDataFunc && m_framesBuffered < m_waterLevel)
    {
      lock.Leave();
      m_cbDataFunc(this, m_cbDataArg, (m_format.m_frameSize - m_frameBufferSize) / m_bytesPerFrame);
    }
  }

  return ret;
}

float CAEStream::GetDelay()
{
  CSingleLock lock(m_critSection);
  if (m_delete) return 0.0f;
  float frames = m_framesBuffered;
  lock.Leave();

  return AE.GetDelay() + (frames / AE.GetSampleRate());
}

void CAEStream::Drain()
{
  CSingleLock lock(m_critSection);
  m_draining = true;
}

void CAEStream::Flush()
{
  CSingleLock lock(m_critSection);
  InternalFlush();
}

void CAEStream::InternalFlush()
{
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
    _aligned_free(p.data);
    p.data = NULL;    
  };

  /* flush any post-proc objects */
  list<IAEPostProc*>::iterator ppi;
  for(ppi = m_postProc.begin(); ppi != m_postProc.end(); ++ppi)
    (*ppi)->Flush();
  
  /* reset our counts */
  m_frameBufferSize = 0;
  m_framesBuffered  = 0;
}

void CAEStream::SetDynamicRangeCompression(int drc)
{
  //FIXME
}

void CAEStream::AppendPostProc(IAEPostProc *pp)
{
  if (pp->Initialize(this))
  {
    CSingleLock lock(m_critSection);
    m_postProc.push_back(pp);
  }
  else
    CLog::Log(LOGERROR, "Failed to initialize post-proc filter: %s", pp->GetName());
}

void CAEStream::PrependPostProc(IAEPostProc *pp)
{
  if (pp->Initialize(this))
  {
    CSingleLock lock(m_critSection);
    m_postProc.push_front(pp);
  }
  else
    CLog::Log(LOGERROR, "Failed to initialize post-proc filter: %s", pp->GetName());
}

void CAEStream::RemovePostProc(IAEPostProc *pp)
{
  CSingleLock lock(m_critSection);
  m_postProc.remove(pp);
}

