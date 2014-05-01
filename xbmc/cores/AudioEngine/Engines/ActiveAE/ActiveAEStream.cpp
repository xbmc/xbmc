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

#include "system.h"
#include "threads/SingleLock.h"
#include "utils/log.h"
#include "utils/MathUtils.h"

#include "cores/AudioEngine/AEFactory.h"
#include "cores/AudioEngine/Utils/AEUtil.h"

#include "ActiveAE.h"
#include "ActiveAEStream.h"

using namespace ActiveAE;

/* typecast AE to CActiveAE */
#define AE (*((CActiveAE*)CAEFactory::GetEngine()))


CActiveAEStream::CActiveAEStream(AEAudioFormat *format)
{
  m_format = *format;
  m_bufferedTime = 0;
  m_currentBuffer = NULL;
  m_drain = false;
  m_paused = false;
  m_rgain = 1.0;
  m_volume = 1.0;
  m_amplify = 1.0;
  m_streamSpace = m_format.m_frameSize * m_format.m_frames;
  m_streamDraining = false;
  m_streamDrained = false;
  m_streamFading = false;
  m_streamFreeBuffers = 0;
  m_streamIsBuffering = true;
  m_streamSlave = NULL;
  m_convertFn = NULL;
  m_leftoverBuffer = new uint8_t[m_format.m_frameSize];
  m_leftoverBytes = 0;
  m_forceResampler = false;
  m_remapper = NULL;
  m_remapBuffer = NULL;
  m_streamResampleRatio = 1.0;
}

CActiveAEStream::~CActiveAEStream()
{
  delete [] m_leftoverBuffer;
  delete m_remapper;
  delete m_remapBuffer;
}

void CActiveAEStream::IncFreeBuffers()
{
  CSingleLock lock(m_streamLock);
  m_streamFreeBuffers++;
}

void CActiveAEStream::DecFreeBuffers()
{
  CSingleLock lock(m_streamLock);
  m_streamFreeBuffers--;
}

void CActiveAEStream::ResetFreeBuffers()
{
  CSingleLock lock(m_streamLock);
  m_streamFreeBuffers = 0;
}

void CActiveAEStream::InitRemapper()
{
  // check if input format follows ffmpeg channel mask
  bool needRemap = false;
  unsigned int avLast, avCur = 0;
  for(unsigned int i=0; i<m_format.m_channelLayout.Count(); i++)
  {
    avLast = avCur;
    avCur = CActiveAEResample::GetAVChannel(m_format.m_channelLayout[i]);
    if(avCur < avLast)
    {
      needRemap = true;
      break;
    }
  }

  if(needRemap)
  {
    CLog::Log(LOGDEBUG, "CActiveAEStream::%s - initialize remapper", __FUNCTION__);

    m_remapper = new CActiveAEResample();
    uint64_t avLayout = CActiveAEResample::GetAVChannelLayout(m_format.m_channelLayout);

    // build layout according to ffmpeg channel order
    // we need this for reference
    CAEChannelInfo ffmpegLayout;
    ffmpegLayout.Reset();
    int idx = 0;
    for(unsigned int i=0; i<m_format.m_channelLayout.Count(); i++)
    {
      for(unsigned int j=0; j<m_format.m_channelLayout.Count(); j++)
      {
        idx = m_remapper->GetAVChannelIndex(m_format.m_channelLayout[j], avLayout);
        if (idx == (int)i)
        {
          ffmpegLayout += m_format.m_channelLayout[j];
          break;
        }
      }
    }

    // build remap layout we can pass to resampler as destination layout
    CAEChannelInfo remapLayout;
    remapLayout.Reset();
    for(unsigned int i=0; i<m_format.m_channelLayout.Count(); i++)
    {
      for(unsigned int j=0; j<m_format.m_channelLayout.Count(); j++)
      {
        idx = m_remapper->GetAVChannelIndex(m_format.m_channelLayout[j], avLayout);
        if (idx == (int)i)
        {
          remapLayout += ffmpegLayout[j];
          break;
        }
      }
    }

    // initialize resampler for only doing remapping
    m_remapper->Init(avLayout,
                     m_format.m_channelLayout.Count(),
                     m_format.m_sampleRate,
                     CActiveAEResample::GetAVSampleFormat(m_format.m_dataFormat),
                     CAEUtil::DataFormatToUsedBits(m_format.m_dataFormat),
                     avLayout,
                     m_format.m_channelLayout.Count(),
                     m_format.m_sampleRate,
                     CActiveAEResample::GetAVSampleFormat(m_format.m_dataFormat),
                     CAEUtil::DataFormatToUsedBits(m_format.m_dataFormat),
                     false,
                     false,
                     &remapLayout,
                     AE_QUALITY_LOW); // not used for remapping

    // extra sound packet, we can't resample to the same buffer
    m_remapBuffer = new CSoundPacket(m_inputBuffers->m_allSamples[0]->pkt->config, m_inputBuffers->m_allSamples[0]->pkt->max_nb_samples);
  }
}

void CActiveAEStream::RemapBuffer()
{
  if(m_remapper)
  {
    int samples = m_remapper->Resample(m_remapBuffer->data, m_remapBuffer->max_nb_samples,
                                       m_currentBuffer->pkt->data, m_currentBuffer->pkt->nb_samples,
                                       1.0);

    if (samples != m_currentBuffer->pkt->nb_samples)
    {
      CLog::Log(LOGERROR, "CActiveAEStream::%s - error remapping", __FUNCTION__);
    }

    // swap sound packets
    CSoundPacket *tmp = m_remapBuffer;
    tmp = m_currentBuffer->pkt;
    m_currentBuffer->pkt = m_remapBuffer;
    m_remapBuffer = tmp;
  }
}

unsigned int CActiveAEStream::GetSpace()
{
  CSingleLock lock(m_streamLock);
  return m_streamFreeBuffers * m_streamSpace;
}

unsigned int CActiveAEStream::AddData(void *data, unsigned int size)
{
  Message *msg;
  unsigned int copied = 0;
  unsigned int bytesToCopy = size;
  uint8_t *buf = (uint8_t*)data;

  while(copied < size)
  {
    buf = ((uint8_t*)data) + copied;
    bytesToCopy = size - copied;

    if (m_currentBuffer)
    {
      // fill leftover buffer and copy it first
      if (m_leftoverBytes && bytesToCopy >= (m_format.m_frameSize - m_leftoverBytes))
      {
        int fillbytes = m_format.m_frameSize - m_leftoverBytes;
        memcpy(m_leftoverBuffer+m_leftoverBytes, buf, fillbytes);
        copied += fillbytes;
        // leftover buffer will be copied on next cycle
        buf = m_leftoverBuffer;
        bytesToCopy = m_format.m_frameSize;
        m_leftoverBytes = 0;
      }

      int start = m_currentBuffer->pkt->nb_samples *
                  m_currentBuffer->pkt->bytes_per_sample *
                  m_currentBuffer->pkt->config.channels /
                  m_currentBuffer->pkt->planes;

      int freeSamples = m_currentBuffer->pkt->max_nb_samples - m_currentBuffer->pkt->nb_samples;
      int availableSamples = bytesToCopy / m_format.m_frameSize;

      // if we don't have a full frame, copy to leftover buffer
      if (!availableSamples && bytesToCopy)
      {
        memcpy(m_leftoverBuffer+m_leftoverBytes, buf, bytesToCopy);
        m_leftoverBytes = bytesToCopy;
        copied += bytesToCopy;
      }

      int samples = std::min(freeSamples, availableSamples);
      int bytes = samples * m_format.m_frameSize;

      //TODO: handle planar formats
      if (m_convertFn)
        m_convertFn(buf, samples*m_currentBuffer->pkt->config.channels, (float*)(m_currentBuffer->pkt->data[0] + start));
      else
        memcpy(m_currentBuffer->pkt->data[0] + start, buf, bytes);
      {
        CSingleLock lock(*m_statsLock);
        m_currentBuffer->pkt->nb_samples += samples;
        m_bufferedTime += (double)samples / m_currentBuffer->pkt->config.sample_rate;
      }
      if (buf != m_leftoverBuffer)
        copied += bytes;
      if (m_currentBuffer->pkt->nb_samples == m_currentBuffer->pkt->max_nb_samples)
      {
        MsgStreamSample msgData;
        msgData.buffer = m_currentBuffer;
        msgData.stream = this;
        RemapBuffer();
        m_streamPort->SendOutMessage(CActiveAEDataProtocol::STREAMSAMPLE, &msgData, sizeof(MsgStreamSample));
        m_currentBuffer = NULL;
      }
      continue;
    }
    else if (m_streamPort->ReceiveInMessage(&msg))
    {
      if (msg->signal == CActiveAEDataProtocol::STREAMBUFFER)
      {
        m_currentBuffer = *((CSampleBuffer**)msg->data);
        msg->Release();
        DecFreeBuffers();
        continue;
      }
      else
      {
        CLog::Log(LOGERROR, "CActiveAEStream::AddData - unknown signal");
        msg->Release();
        break;
      }
    }
    if (!m_inMsgEvent.WaitMSec(200))
      break;
  }
  return copied;
}

double CActiveAEStream::GetDelay()
{
  return AE.GetDelay(this);
}

bool CActiveAEStream::IsBuffering()
{
  CSingleLock lock(m_streamLock);
  return m_streamIsBuffering;
}

double CActiveAEStream::GetCacheTime()
{
  return AE.GetCacheTime(this);
}

double CActiveAEStream::GetCacheTotal()
{
  return AE.GetCacheTotal(this);
}

void CActiveAEStream::Pause()
{
  AE.PauseStream(this, true);
}

void CActiveAEStream::Resume()
{
  AE.PauseStream(this, false);
}

void CActiveAEStream::Drain(bool wait)
{
  Message *msg;
  CActiveAEStream *stream = this;

  m_streamDraining = true;
  m_streamDrained = false;

  Message *reply;
  if (m_streamPort->SendOutMessageSync(CActiveAEDataProtocol::DRAINSTREAM,
                                       &reply,2000,
                                       &stream, sizeof(CActiveAEStream*)))
  {
    bool success = reply->signal == CActiveAEDataProtocol::ACC ? true : false;
    reply->Release();
    if (!success)
    {
      CLog::Log(LOGERROR, "CActiveAEStream::Drain - no acc");
    }
  }

  if (m_currentBuffer)
  {
    MsgStreamSample msgData;
    msgData.buffer = m_currentBuffer;
    msgData.stream = this;
    RemapBuffer();
    m_streamPort->SendOutMessage(CActiveAEDataProtocol::STREAMSAMPLE, &msgData, sizeof(MsgStreamSample));
    m_currentBuffer = NULL;
  }

  XbmcThreads::EndTime timer(2000);
  while (!timer.IsTimePast())
  {
    if (m_streamPort->ReceiveInMessage(&msg))
    {
      if (msg->signal == CActiveAEDataProtocol::STREAMBUFFER)
      {
        MsgStreamSample msgData;
        msgData.stream = this;
        msgData.buffer = *((CSampleBuffer**)msg->data);
        msg->Reply(CActiveAEDataProtocol::STREAMSAMPLE, &msgData, sizeof(MsgStreamSample));
        DecFreeBuffers();
        continue;
      }
      else if (msg->signal == CActiveAEDataProtocol::STREAMDRAINED)
      {
        msg->Release();
        return;
      }
    }
    else if (!wait)
      return;

    m_inMsgEvent.WaitMSec(timer.MillisLeft());
  }
  CLog::Log(LOGERROR, "CActiveAEStream::Drain - timeout out");
}

bool CActiveAEStream::IsDraining()
{
  CSingleLock lock(m_streamLock);
  return m_streamDraining;
}

bool CActiveAEStream::IsDrained()
{
  CSingleLock lock(m_streamLock);
  return m_streamDrained;
}

void CActiveAEStream::Flush()
{
  m_currentBuffer = NULL;
  m_leftoverBytes = 0;
  AE.FlushStream(this);
  ResetFreeBuffers();
}

float CActiveAEStream::GetAmplification()
{
  return m_streamAmplify;
}

void CActiveAEStream::SetAmplification(float amplify)
{
  m_streamAmplify = amplify;
  AE.SetStreamAmplification(this, m_streamAmplify);
}

float CActiveAEStream::GetReplayGain()
{
  return m_streamRgain;
}

void CActiveAEStream::SetReplayGain(float factor)
{
  m_streamRgain = std::max( 0.0f, factor);
  AE.SetStreamReplaygain(this, m_streamRgain);
}

float CActiveAEStream::GetVolume()
{
  return m_streamVolume;
}

void CActiveAEStream::SetVolume(float volume)
{
  m_streamVolume = std::max( 0.0f, std::min(1.0f, volume));
  AE.SetStreamVolume(this, m_streamVolume);
}

double CActiveAEStream::GetResampleRatio()
{
  return m_streamResampleRatio;
}

bool CActiveAEStream::SetResampleRatio(double ratio)
{
  m_streamResampleRatio = ratio;
  AE.SetStreamResampleRatio(this, m_streamResampleRatio);
  return true;
}

void CActiveAEStream::FadeVolume(float from, float target, unsigned int time)
{
  if (time == 0 || AE_IS_RAW(m_format.m_dataFormat))
    return;

  m_streamFading = true;
  AE.SetStreamFade(this, from, target, time);
}

bool CActiveAEStream::IsFading()
{
  CSingleLock lock(m_streamLock);
  return m_streamFading;
}

const unsigned int CActiveAEStream::GetFrameSize() const
{
  return m_format.m_frameSize;
}

const unsigned int CActiveAEStream::GetChannelCount() const
{
  return m_format.m_channelLayout.Count();
}

const unsigned int CActiveAEStream::GetSampleRate() const
{
  return m_format.m_sampleRate;
}

const unsigned int CActiveAEStream::GetEncodedSampleRate() const
{
  return m_format.m_encodedRate;
}

const enum AEDataFormat CActiveAEStream::GetDataFormat() const
{
  return m_format.m_dataFormat;
}

void CActiveAEStream::RegisterAudioCallback(IAudioCallback* pCallback)
{
}

void CActiveAEStream::UnRegisterAudioCallback()
{
}

void CActiveAEStream::RegisterSlave(IAEStream *slave)
{
  CSingleLock lock(m_streamLock);
  m_streamSlave = slave;
}

