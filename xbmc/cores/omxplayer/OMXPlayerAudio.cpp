/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#if (defined HAVE_CONFIG_H) && (!defined TARGET_WINDOWS)
  #include "config.h"
#elif defined(TARGET_WINDOWS)
#include "system.h"
#endif

#include "OMXPlayerAudio.h"

#include <stdio.h>
#include <unistd.h>
#include <iomanip>

#include "linux/XMemUtils.h"
#include "utils/BitstreamStats.h"

#include "DVDDemuxers/DVDDemuxUtils.h"
#include "cores/AudioEngine/Utils/AEUtil.h"
#include "utils/MathUtils.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "utils/TimeUtils.h"

#include "OMXPlayer.h"
#include "linux/RBP.h"
#include "cores/AudioEngine/AEFactory.h"

#include <iostream>
#include <sstream>

class COMXMsgAudioCodecChange : public CDVDMsg
{
public:
  COMXMsgAudioCodecChange(const CDVDStreamInfo &hints, COMXAudioCodecOMX* codec)
    : CDVDMsg(GENERAL_STREAMCHANGE)
    , m_codec(codec)
    , m_hints(hints)
  {}
 ~COMXMsgAudioCodecChange()
  {
    delete m_codec;
  }
  COMXAudioCodecOMX   *m_codec;
  CDVDStreamInfo      m_hints;
};

OMXPlayerAudio::OMXPlayerAudio(OMXClock *av_clock, CDVDMessageQueue& parent)
: CThread("OMXPlayerAudio")
, m_messageQueue("audio")
, m_messageParent(parent)
{
  m_av_clock      = av_clock;
  m_pAudioCodec   = NULL;
  m_speed         = DVD_PLAYSPEED_NORMAL;
  m_started       = false;
  m_stalled       = false;
  m_audioClock    = DVD_NOPTS_VALUE;
  m_buffer_empty  = false;
  m_nChannels     = 0;
  m_DecoderOpen   = false;
  m_bad_state     = false;
  m_hints_current.Clear();

  bool small_mem = g_RBP.GetArmMem() < 256;
  m_messageQueue.SetMaxDataSize((small_mem ? 3:6) * 1024 * 1024);

  m_messageQueue.SetMaxTimeSize(8.0);
  m_passthrough = false;
  m_use_hw_decode = false;
  m_hw_decode = false;
  m_silence = false;
  m_flush = false;  
}


OMXPlayerAudio::~OMXPlayerAudio()
{
  CloseStream(false);
}

bool OMXPlayerAudio::OpenStream(CDVDStreamInfo &hints)
{
  m_bad_state = false;

  COMXAudioCodecOMX *codec = new COMXAudioCodecOMX();

  if(!codec || !codec->Open(hints))
  {
    CLog::Log(LOGERROR, "Unsupported audio codec");
    delete codec; codec = NULL;
    return false;
  }

  if(m_messageQueue.IsInited())
    m_messageQueue.Put(new COMXMsgAudioCodecChange(hints, codec), 0);
  else
  {
    OpenStream(hints, codec);
    m_messageQueue.Init();
    CLog::Log(LOGNOTICE, "Creating audio thread");
    Create();
  }

  return true;
}

void OMXPlayerAudio::OpenStream(CDVDStreamInfo &hints, COMXAudioCodecOMX *codec)
{
  SAFE_DELETE(m_pAudioCodec);

  m_hints           = hints;
  m_pAudioCodec     = codec;

  if(m_hints.bitspersample == 0)
    m_hints.bitspersample = 16;

  m_speed           = DVD_PLAYSPEED_NORMAL;
  m_audioClock      = DVD_NOPTS_VALUE;
  m_hw_decode       = false;
  m_silence         = false;
  m_started         = false;
  m_flush           = false;
  m_nChannels       = 0;
  m_stalled         = m_messageQueue.GetPacketCount(CDVDMsg::DEMUXER_PACKET) == 0;
  m_use_hw_decode   = g_advancedSettings.m_omxHWAudioDecode;
  m_format.m_dataFormat    = GetDataFormat(m_hints);
  m_format.m_sampleRate    = 0;
  m_format.m_channelLayout = 0;
}

bool OMXPlayerAudio::CloseStream(bool bWaitForBuffers)
{
  // wait until buffers are empty
  if (bWaitForBuffers && m_speed > 0) m_messageQueue.WaitUntilEmpty();

  m_messageQueue.Abort();

  if(IsRunning())
    StopThread();

  m_messageQueue.End();

  if (m_pAudioCodec)
  {
    m_pAudioCodec->Dispose();
    delete m_pAudioCodec;
    m_pAudioCodec = NULL;
  }

  CloseDecoder();

  m_speed         = DVD_PLAYSPEED_NORMAL;
  m_started       = false;

  return true;
}

void OMXPlayerAudio::OnStartup()
{
}

void OMXPlayerAudio::OnExit()
{
  CLog::Log(LOGNOTICE, "thread end: OMXPlayerAudio::OnExit()");
}

bool OMXPlayerAudio::CodecChange()
{
  unsigned int old_bitrate = m_hints.bitrate;
  unsigned int new_bitrate = m_hints_current.bitrate;

  if(m_pAudioCodec)
  {
    m_hints.channels = m_pAudioCodec->GetChannels();
    m_hints.samplerate = m_pAudioCodec->GetSampleRate();
  }

  /* only check bitrate changes on AV_CODEC_ID_DTS, AV_CODEC_ID_AC3, AV_CODEC_ID_EAC3 */
  if(m_hints.codec != AV_CODEC_ID_DTS && m_hints.codec != AV_CODEC_ID_AC3 && m_hints.codec != AV_CODEC_ID_EAC3)
    new_bitrate = old_bitrate = 0;

  // for passthrough we only care about the codec and the samplerate
  bool minor_change = m_hints_current.channels       != m_hints.channels ||
                      m_hints_current.bitspersample  != m_hints.bitspersample ||
                      old_bitrate                    != new_bitrate;

  if(m_hints_current.codec          != m_hints.codec ||
     m_hints_current.samplerate     != m_hints.samplerate ||
     (!m_passthrough && minor_change) || !m_DecoderOpen)
  {
    m_hints_current = m_hints;
    return true;
  }

  return false;
}

bool OMXPlayerAudio::Decode(DemuxPacket *pkt, bool bDropPacket)
{
  if(!pkt || m_bad_state || !m_pAudioCodec)
    return false;

  if(pkt->dts != DVD_NOPTS_VALUE)
    m_audioClock = pkt->dts;

  const uint8_t *data_dec = pkt->pData;
  int            data_len = pkt->iSize;

  if(!OMX_IS_RAW(m_format.m_dataFormat) && !bDropPacket)
  {
    while(!m_bStop && data_len > 0)
    {
      int len = m_pAudioCodec->Decode((BYTE *)data_dec, data_len);
      if( (len < 0) || (len >  data_len) )
      {
        m_pAudioCodec->Reset();
        break;
      }

      data_dec+= len;
      data_len -= len;

      uint8_t *decoded;
      int decoded_size = m_pAudioCodec->GetData(&decoded);

      if(decoded_size <=0)
        continue;

      int ret = 0;

      m_audioStats.AddSampleBytes(decoded_size);

      if(CodecChange())
      {
        m_DecoderOpen = OpenDecoder();
        if(!m_DecoderOpen)
          return false;
      }

      while(!m_bStop)
      {
        // discard if flushing as clocks may be stopped and we'll never submit it
        if(m_flush)
          break;

        if(m_omxAudio.GetSpace() < (unsigned int)decoded_size)
        {
          Sleep(10);
          continue;
        }
        
        if(!bDropPacket)
        {
          // Zero out the frame data if we are supposed to silence the audio
          if(m_silence)
            memset(decoded, 0x0, decoded_size);

          ret = m_omxAudio.AddPackets(decoded, decoded_size, m_audioClock, m_audioClock);

          if(ret != decoded_size)
          {
            CLog::Log(LOGERROR, "error ret %d decoded_size %d\n", ret, decoded_size);
          }
        }

        break;

      }
    }
  }
  else if(!bDropPacket)
  {
    if(CodecChange())
    {
      m_DecoderOpen = OpenDecoder();
      if(!m_DecoderOpen)
        return false;
    }

    while(!m_bStop)
    {
      if(m_flush)
        break;

      if(m_omxAudio.GetSpace() < (unsigned int)pkt->iSize)
      {
        Sleep(10);
        continue;
      }
        
      if(!bDropPacket)
      {
        if(m_silence)
          memset(pkt->pData, 0x0, pkt->iSize);

        m_omxAudio.AddPackets(pkt->pData, pkt->iSize, m_audioClock, m_audioClock);
      }

      m_audioStats.AddSampleBytes(pkt->iSize);

      break;
    }
  }

  if(bDropPacket)
    m_stalled = false;

  // signal to our parent that we have initialized
  if(m_started == false)
  {
    m_started = true;
    m_messageParent.Put(new CDVDMsgInt(CDVDMsg::PLAYER_STARTED, DVDPLAYER_AUDIO));
  }

  return true;
}

void OMXPlayerAudio::Process()
{
  m_audioStats.Start();

  while(!m_bStop)
  {
    CDVDMsg* pMsg;
    int priority = (m_speed == DVD_PLAYSPEED_PAUSE && m_started) ? 1 : 0;
    int timeout = 1000;

    MsgQueueReturnCode ret = m_messageQueue.Get(&pMsg, timeout, priority);

    if (ret == MSGQ_TIMEOUT)
    {
      Sleep(10);
      continue;
    }

    if (MSGQ_IS_ERROR(ret) || ret == MSGQ_ABORT)
    {
      Sleep(10);
      continue;
    }

    if (pMsg->IsType(CDVDMsg::DEMUXER_PACKET))
    {
      DemuxPacket* pPacket = ((CDVDMsgDemuxerPacket*)pMsg)->GetPacket();
      bool bPacketDrop     = ((CDVDMsgDemuxerPacket*)pMsg)->GetPacketDrop();

      #ifdef _DEBUG
      CLog::Log(LOGINFO, "Audio: dts:%.0f pts:%.0f size:%d (s:%d f:%d d:%d l:%d) s:%d %d/%d late:%d,%d", pPacket->dts, pPacket->pts,
           (int)pPacket->iSize, m_started, m_flush, bPacketDrop, m_stalled, m_speed, 0, 0, (int)m_omxAudio.GetAudioRenderingLatency(), (int)m_hints_current.samplerate);
      #endif
      if(Decode(pPacket, m_speed > DVD_PLAYSPEED_NORMAL || m_speed < 0 || bPacketDrop))
      {
        // we are not running until something is cached in output device
        if(m_stalled && m_omxAudio.GetCacheTime() > 0.0)
        {
          CLog::Log(LOGINFO, "COMXPlayerAudio - Switching to normal playback");
          m_stalled = false;
        }
      }
    }
    else if (pMsg->IsType(CDVDMsg::GENERAL_SYNCHRONIZE))
    {
      if(((CDVDMsgGeneralSynchronize*)pMsg)->Wait( 100, SYNCSOURCE_AUDIO ))
        CLog::Log(LOGDEBUG, "COMXPlayerAudio - CDVDMsg::GENERAL_SYNCHRONIZE");
      else
        m_messageQueue.Put(pMsg->Acquire(), 1); /* push back as prio message, to process other prio messages */
    }
    else if (pMsg->IsType(CDVDMsg::GENERAL_RESYNC))
    { //player asked us to set internal clock
      CDVDMsgGeneralResync* pMsgGeneralResync = (CDVDMsgGeneralResync*)pMsg;

      if (pMsgGeneralResync->m_clock && pMsgGeneralResync->m_timestamp != DVD_NOPTS_VALUE)
      {
        CLog::Log(LOGDEBUG, "CDVDPlayerAudio - CDVDMsg::GENERAL_RESYNC(%f, %f, 1)", m_audioClock, pMsgGeneralResync->m_timestamp);
        m_av_clock->Discontinuity(pMsgGeneralResync->m_timestamp);
      }
      else
        CLog::Log(LOGDEBUG, "CDVDPlayerAudio - CDVDMsg::GENERAL_RESYNC(%f, 0)", m_audioClock);

      m_flush = false;
      m_audioClock = DVD_NOPTS_VALUE;
    }
    else if (pMsg->IsType(CDVDMsg::GENERAL_RESET))
    {
      CLog::Log(LOGDEBUG, "COMXPlayerAudio - CDVDMsg::GENERAL_RESET");
      if (m_pAudioCodec)
        m_pAudioCodec->Reset();
      m_omxAudio.Flush();
      m_started = false;
      m_audioClock = DVD_NOPTS_VALUE;
    }
    else if (pMsg->IsType(CDVDMsg::GENERAL_FLUSH))
    {
      CLog::Log(LOGDEBUG, "COMXPlayerAudio - CDVDMsg::GENERAL_FLUSH");
      m_omxAudio.Flush();
      m_stalled   = true;
      m_started   = false;

      if (m_pAudioCodec)
        m_pAudioCodec->Reset();
      m_audioClock = DVD_NOPTS_VALUE;
    }
    else if (pMsg->IsType(CDVDMsg::PLAYER_STARTED))
    {
      CLog::Log(LOGDEBUG, "COMXPlayerAudio - CDVDMsg::PLAYER_STARTED %d", m_started);
      if(m_started)
        m_messageParent.Put(new CDVDMsgInt(CDVDMsg::PLAYER_STARTED, DVDPLAYER_AUDIO));
    }
    else if (pMsg->IsType(CDVDMsg::PLAYER_DISPLAYTIME))
    {
      COMXPlayer::SPlayerState& state = ((CDVDMsgType<COMXPlayer::SPlayerState>*)pMsg)->m_value;
      double pts = m_audioClock;
      double stamp = m_av_clock->OMXMediaTime();

      if(state.time_src == COMXPlayer::ETIMESOURCE_CLOCK)
        state.time      = stamp == 0.0 ? state.time : DVD_TIME_TO_MSEC(stamp + state.time_offset);
      else
        state.time      = stamp == 0.0 || pts == DVD_NOPTS_VALUE ? state.time : state.time + DVD_TIME_TO_MSEC(stamp - pts);
      state.timestamp = m_av_clock->GetAbsoluteClock();
      state.player    = DVDPLAYER_AUDIO;
      m_messageParent.Put(pMsg->Acquire());
    }
    else if (pMsg->IsType(CDVDMsg::GENERAL_EOF))
    {
      CLog::Log(LOGDEBUG, "COMXPlayerAudio - CDVDMsg::GENERAL_EOF");
      SubmitEOS();
    }
    else if (pMsg->IsType(CDVDMsg::GENERAL_DELAY))
    {
      double timeout = static_cast<CDVDMsgDouble*>(pMsg)->m_value;
      CLog::Log(LOGDEBUG, "COMXPlayerAudio - CDVDMsg::GENERAL_DELAY(%f)", timeout);
    }
    else if (pMsg->IsType(CDVDMsg::PLAYER_SETSPEED))
    {
      if (m_speed != static_cast<CDVDMsgInt*>(pMsg)->m_value)
      {
        m_speed = static_cast<CDVDMsgInt*>(pMsg)->m_value;
        CLog::Log(LOGDEBUG, "COMXPlayerAudio - CDVDMsg::PLAYER_SETSPEED %d", m_speed);
      }
    }
    else if (pMsg->IsType(CDVDMsg::AUDIO_SILENCE))
    {
      m_silence = static_cast<CDVDMsgBool*>(pMsg)->m_value;
      if (m_silence)
        CLog::Log(LOGDEBUG, "COMXPlayerAudio - CDVDMsg::AUDIO_SILENCE(%f, 1)", m_audioClock);
      else
        CLog::Log(LOGDEBUG, "COMXPlayerAudio - CDVDMsg::AUDIO_SILENCE(%f, 0)", m_audioClock);
    }
    else if (pMsg->IsType(CDVDMsg::GENERAL_STREAMCHANGE))
    {
      COMXMsgAudioCodecChange* msg(static_cast<COMXMsgAudioCodecChange*>(pMsg));
      CLog::Log(LOGDEBUG, "COMXPlayerAudio - CDVDMsg::GENERAL_STREAMCHANGE");
      OpenStream(msg->m_hints, msg->m_codec);
      msg->m_codec = NULL;
    }

    pMsg->Release();
  }
}

void OMXPlayerAudio::Flush()
{
  m_flush = true;
  m_messageQueue.Flush();
  m_messageQueue.Put( new CDVDMsg(CDVDMsg::GENERAL_FLUSH), 1);
}

void OMXPlayerAudio::WaitForBuffers()
{
  // make sure there are no more packets available
  m_messageQueue.WaitUntilEmpty();

  // make sure almost all has been rendered
  // leave 500ms to avound buffer underruns
  double delay = GetCacheTime();
  if(delay > 0.5)
    Sleep((int)(1000 * (delay - 0.5)));
}

bool OMXPlayerAudio::Passthrough() const
{
  return m_passthrough;
}

AEDataFormat OMXPlayerAudio::GetDataFormat(CDVDStreamInfo hints)
{
  AEDataFormat dataFormat = AE_FMT_S16NE;

  m_passthrough = false;
  m_hw_decode   = false;

  /* check our audio capabilties */

  /* pathrought is overriding hw decode*/
  if(hints.codec == AV_CODEC_ID_AC3 && CAEFactory::SupportsRaw(AE_FMT_AC3))
  {
    dataFormat = AE_FMT_AC3;
    m_passthrough = true;
  }
  if(hints.codec == AV_CODEC_ID_DTS && CAEFactory::SupportsRaw(AE_FMT_DTS))
  {
    dataFormat = AE_FMT_DTS;
    m_passthrough = true;
  }

  /* hw decode */
  if(m_use_hw_decode && !m_passthrough)
  {
    if(hints.codec == AV_CODEC_ID_AC3 && COMXAudio::CanHWDecode(m_hints.codec))
    {
      dataFormat = AE_FMT_AC3;
      m_hw_decode = true;
    }
    if(hints.codec == AV_CODEC_ID_DTS && COMXAudio::CanHWDecode(m_hints.codec))
    {
      dataFormat = AE_FMT_DTS;
      m_hw_decode = true;
    }
  }

  /* software path */
  if(!m_passthrough && !m_hw_decode)
  {
    if (m_pAudioCodec && m_pAudioCodec->GetBitsPerSample() == 16)
      dataFormat = AE_FMT_S16NE;
    else
      dataFormat = AE_FMT_FLOAT;
  }

  return dataFormat;
}

bool OMXPlayerAudio::OpenDecoder()
{
  m_nChannels   = m_hints.channels;
  m_passthrough = false;
  m_hw_decode   = false;

  if(m_DecoderOpen)
  {
    m_omxAudio.Deinitialize();
    m_DecoderOpen = false;
  }

  /* setup audi format for audio render */
  m_format.m_sampleRate    = m_hints.samplerate;
  /* GetDataFormat is setting up evrything */
  m_format.m_dataFormat = GetDataFormat(m_hints);

  m_format.m_channelLayout.Reset();
  if (m_pAudioCodec && !m_passthrough)
    m_format.m_channelLayout = m_pAudioCodec->GetChannelMap();
  else if (m_passthrough)
  {
    // we just want to get the channel count right to stop OMXAudio.cpp rejecting stream
    // the actual layout is not used
    if (m_nChannels > 0 ) m_format.m_channelLayout += AE_CH_FL;
    if (m_nChannels > 1 ) m_format.m_channelLayout += AE_CH_FR;
    if (m_nChannels > 2 ) m_format.m_channelLayout += AE_CH_FC;
    if (m_nChannels > 3 ) m_format.m_channelLayout += AE_CH_LFE;
    if (m_nChannels > 4 ) m_format.m_channelLayout += AE_CH_BL;
    if (m_nChannels > 5 ) m_format.m_channelLayout += AE_CH_BR;
  }
  bool bAudioRenderOpen = m_omxAudio.Initialize(m_format, m_av_clock, m_hints, m_passthrough, m_hw_decode);

  m_codec_name = "";
  m_bad_state  = !bAudioRenderOpen;
  
  if(!bAudioRenderOpen)
  {
    CLog::Log(LOGERROR, "OMXPlayerAudio : Error open audio output");
    m_omxAudio.Deinitialize();
  }
  else
  {
    CLog::Log(LOGINFO, "Audio codec %s channels %d samplerate %d bitspersample %d\n",
      m_codec_name.c_str(), m_nChannels, m_hints.samplerate, m_hints.bitspersample);
  }

  m_started = false;

  return bAudioRenderOpen;
}

void OMXPlayerAudio::CloseDecoder()
{
  m_omxAudio.Deinitialize();
  m_DecoderOpen = false;
}

double OMXPlayerAudio::GetDelay()
{
  return m_omxAudio.GetDelay();
}

double OMXPlayerAudio::GetCacheTime()
{
  return m_omxAudio.GetCacheTime();
}

double OMXPlayerAudio::GetCacheTotal()
{
  return m_omxAudio.GetCacheTotal();
}

void OMXPlayerAudio::SubmitEOS()
{
  if(!m_bad_state)
    m_omxAudio.SubmitEOS();
}

bool OMXPlayerAudio::IsEOS()
{
  return m_bad_state || m_omxAudio.IsEOS();
}

void OMXPlayerAudio::WaitCompletion()
{
  unsigned int nTimeOut = AUDIO_BUFFER_SECONDS * 1000;
  while(nTimeOut)
  {
    if(IsEOS())
    {
      CLog::Log(LOGDEBUG, "%s::%s - got eos\n", CLASSNAME, __func__);
      break;
    }

    if(nTimeOut == 0)
    {
      CLog::Log(LOGERROR, "%s::%s - wait for eos timed out\n", CLASSNAME, __func__);
      break;
    }
    Sleep(50);
    nTimeOut -= 50;
  }
}

void OMXPlayerAudio::SetSpeed(int speed)
{
  if(m_messageQueue.IsInited())
    m_messageQueue.Put( new CDVDMsgInt(CDVDMsg::PLAYER_SETSPEED, speed), 1 );
  else
    m_speed = speed;
}

int OMXPlayerAudio::GetAudioBitrate()
{
  return (int)m_audioStats.GetBitrate();
}

std::string OMXPlayerAudio::GetPlayerInfo()
{
  std::ostringstream s;
  s << "aq:"     << setw(2) << min(99,m_messageQueue.GetLevel() + MathUtils::round_int(100.0/8.0*GetCacheTime())) << "%";
  s << ", Kb/s:" << fixed << setprecision(2) << (double)GetAudioBitrate() / 1024.0;

  return s.str();
}
