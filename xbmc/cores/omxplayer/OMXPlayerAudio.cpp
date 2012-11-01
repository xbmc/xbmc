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

#if (defined HAVE_CONFIG_H) && (!defined WIN32)
  #include "config.h"
#elif defined(_WIN32)
#include "system.h"
#endif

#include "OMXPlayerAudio.h"

#include <stdio.h>
#include <unistd.h>
#include <iomanip>

#include "linux/XMemUtils.h"
#include "utils/BitstreamStats.h"
#include "settings/GUISettings.h"
#include "settings/Settings.h"

#include "DVDDemuxers/DVDDemuxUtils.h"
#include "utils/MathUtils.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "utils/TimeUtils.h"

#include "OMXPlayer.h"

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

OMXPlayerAudio::OMXPlayerAudio(OMXClock *av_clock,
                               CDVDMessageQueue& parent)
: CThread("COMXPlayerAudio")
, m_messageQueue("audio")
, m_messageParent(parent)
{
  m_av_clock      = av_clock;
  m_pAudioCodec   = NULL;
  m_speed         = DVD_PLAYSPEED_NORMAL;
  m_started       = false;
  m_stalled       = false;
  m_audioClock    = 0;
  m_buffer_empty  = false;
  m_nChannels     = 0;
  m_DecoderOpen   = false;
  m_freq          = CurrentHostFrequency();
  m_send_eos      = false;
  m_hints_current.Clear();

  m_av_clock->SetMasterClock(false);

  m_messageQueue.SetMaxDataSize(3 * 1024 * 1024);
  m_messageQueue.SetMaxTimeSize(8.0);
}


OMXPlayerAudio::~OMXPlayerAudio()
{
  CloseStream(false);

  m_DllBcmHost.Unload();
}

bool OMXPlayerAudio::OpenStream(CDVDStreamInfo &hints)
{
  if(!m_DllBcmHost.Load())
    return false;

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
  m_audioClock      = 0;
  m_error           = 0;
  m_errorbuff       = 0;
  m_errorcount      = 0;
  m_integral        = 0;
  m_skipdupcount    = 0;
  m_prevskipped     = false;
  m_syncclock       = true;
  m_hw_decode       = false;
  m_errortime       = CurrentHostCounter();
  m_silence         = false;
  m_started         = false;
  m_flush           = false;
  m_nChannels       = 0;
  m_synctype        = SYNC_DISCON;
  m_stalled         = m_messageQueue.GetPacketCount(CDVDMsg::DEMUXER_PACKET) == 0;
  m_use_passthrough = (g_guiSettings.GetInt("audiooutput.mode") == AUDIO_HDMI) ? true : false ;
  m_use_hw_decode   = g_advancedSettings.m_omxHWAudioDecode;
  m_send_eos        = false;
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



void OMXPlayerAudio::HandleSyncError(double duration)
{
  double clock = m_av_clock->GetClock();
  double error = m_audioClock - clock;
  int64_t now;

  if( fabs(error) > DVD_MSEC_TO_TIME(100) || m_syncclock )
  {
    m_av_clock->Discontinuity(clock+error);
    /*
    if(m_speed == DVD_PLAYSPEED_NORMAL)
    CLog::Log(LOGDEBUG, "OMXPlayerAudio:: Discontinuity - was:%f, should be:%f, error:%f\n", clock, clock+error, error);
    */

    m_errorbuff = 0;
    m_errorcount = 0;
    m_skipdupcount = 0;
    m_error = 0;
    m_syncclock = false;
    m_errortime = m_av_clock->CurrentHostCounter();

    return;
  }

  if (m_speed != DVD_PLAYSPEED_NORMAL)
  {
    m_errorbuff = 0;
    m_errorcount = 0;
    m_integral = 0;
    m_skipdupcount = 0;
    m_error = 0;
    m_errortime = m_av_clock->CurrentHostCounter();
    return;
  }

  //check if measured error for 1 second
  now = m_av_clock->CurrentHostCounter();
  if ((now - m_errortime) >= m_freq)
  {
    m_errortime = now;
    m_error = m_errorbuff / m_errorcount;

    m_errorbuff = 0;
    m_errorcount = 0;

    if (m_synctype == SYNC_DISCON)
    {
      double limit, error;

      if (m_av_clock->GetRefreshRate(&limit) > 0)
      {
        //when the videoreferenceclock is running, the discontinuity limit is one vblank period
        limit *= DVD_TIME_BASE;

        //make error a multiple of limit, rounded towards zero,
        //so it won't interfere with the sync methods in CXBMCRenderManager::WaitPresentTime
        if (m_error > 0.0)
          error = limit * floor(m_error / limit);
        else
          error = limit * ceil(m_error / limit);
      }
      else
      {
        limit = DVD_MSEC_TO_TIME(10);
        error = m_error;
      }

      /*
      limit = DVD_MSEC_TO_TIME(10);
      error = m_error;
      */

      if (fabs(error) > limit - 0.001)
      {
        m_av_clock->Discontinuity(clock+error);
        /*
        if(m_speed == DVD_PLAYSPEED_NORMAL)
          CLog::Log(LOGDEBUG, "COMXPlayerAudio:: Discontinuity - was:%f, should be:%f, error:%f", clock, clock+error, error);
        */
      }
    }
    /*
    else if (m_synctype == SYNC_SKIPDUP && m_skipdupcount == 0 && fabs(m_error) > DVD_MSEC_TO_TIME(10))
    if (m_skipdupcount == 0 && fabs(m_error) > DVD_MSEC_TO_TIME(10))
    {
      //check how many packets to skip/duplicate
      m_skipdupcount = (int)(m_error / duration);
      //if less than one frame off, see if it's more than two thirds of a frame, so we can get better in sync
      if (m_skipdupcount == 0 && fabs(m_error) > duration / 3 * 2)
        m_skipdupcount = (int)(m_error / (duration / 3 * 2));

      if (m_skipdupcount > 0)
        CLog::Log(LOGDEBUG, "OMXPlayerAudio:: Duplicating %i packet(s) of %.2f ms duration",
                  m_skipdupcount, duration / DVD_TIME_BASE * 1000.0);
      else if (m_skipdupcount < 0)
        CLog::Log(LOGDEBUG, "OMXPlayerAudio:: Skipping %i packet(s) of %.2f ms duration ",
                  m_skipdupcount * -1,  duration / DVD_TIME_BASE * 1000.0);
    }
    */
  }
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

  /* only check bitrate changes on CODEC_ID_DTS, CODEC_ID_AC3, CODEC_ID_EAC3 */
  if(m_hints.codec != CODEC_ID_DTS && m_hints.codec != CODEC_ID_AC3 && m_hints.codec != CODEC_ID_EAC3)
    new_bitrate = old_bitrate = 0;
    
  if(m_hints_current.codec          != m_hints.codec ||
     m_hints_current.channels       != m_hints.channels ||
     m_hints_current.samplerate     != m_hints.samplerate ||
     m_hints_current.bitspersample  != m_hints.bitspersample ||
     old_bitrate                    != new_bitrate ||
     !m_DecoderOpen)
  {
    m_hints_current = m_hints;
    return true;
  }

  return false;
}

bool OMXPlayerAudio::Decode(DemuxPacket *pkt, bool bDropPacket)
{
  if(!pkt)
    return false;

  /* last decoder reinit went wrong */
  if(!m_pAudioCodec)
    return true;

  if(pkt->dts != DVD_NOPTS_VALUE)
    m_audioClock = pkt->dts;

  const uint8_t *data_dec = pkt->pData;
  int            data_len = pkt->iSize;

  if(!OMX_IS_RAW(m_format.m_dataFormat))
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
        CloseDecoder();

        m_DecoderOpen = OpenDecoder();
        if(!m_DecoderOpen)
          return false;
      }

      while(!m_bStop)
      {
        if(m_flush)
        {
          m_flush = false;
          break;
        }

        if(m_omxAudio.GetSpace() < (unsigned int)pkt->iSize)
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

        int n = (m_nChannels * m_hints.bitspersample * m_hints.samplerate)>>3;
        if (n > 0)
          m_audioClock += ((double)decoded_size * DVD_TIME_BASE) / n;

        if(m_speed == DVD_PLAYSPEED_NORMAL)
          HandleSyncError((((double)decoded_size * DVD_TIME_BASE) / n));
        break;

      }
    }
  }
  else
  {
    if(CodecChange())
    {
      CloseDecoder();

      m_DecoderOpen = OpenDecoder();
      if(!m_DecoderOpen)
        return false;
    }

    while(!m_bStop)
    {
      if(m_flush)
      {
        m_flush = false;
        break;
      }

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

      if(m_speed == DVD_PLAYSPEED_NORMAL)
        HandleSyncError(0);

      m_audioStats.AddSampleBytes(pkt->iSize);

      break;
    }
  }

  if(bDropPacket)
    m_stalled = false;

  if(m_omxAudio.GetDelay() < 0.1)
    m_stalled = true;

  // signal to our parent that we have initialized
  if(m_started == false)
  {
    m_started = true;
    m_messageParent.Put(new CDVDMsgInt(CDVDMsg::PLAYER_STARTED, DVDPLAYER_AUDIO));
  }

  if(!bDropPacket && m_speed == DVD_PLAYSPEED_NORMAL && m_av_clock->HasVideo())
  {
    if(GetDelay() < 0.1f && !m_av_clock->OMXAudioBuffer())
    {
      clock_gettime(CLOCK_REALTIME, &m_starttime);
      m_av_clock->OMXAudioBufferStart();
    }
    else if(GetDelay() > (AUDIO_BUFFER_SECONDS * 0.75f) && m_av_clock->OMXAudioBuffer())
    {
      m_av_clock->OMXAudioBufferStop();
    }
    else if(m_av_clock->OMXAudioBuffer())
    {
      clock_gettime(CLOCK_REALTIME, &m_endtime);
      if((m_endtime.tv_sec - m_starttime.tv_sec) > 1)
      {
        m_av_clock->OMXAudioBufferStop();
      }
    }
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

      if(Decode(pPacket, m_speed > DVD_PLAYSPEED_NORMAL || m_speed < 0 || bPacketDrop))
      {
        if (m_stalled && (m_omxAudio.GetDelay() > (AUDIO_BUFFER_SECONDS * 0.75f)))
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

      if (pMsgGeneralResync->m_timestamp != DVD_NOPTS_VALUE)
        m_audioClock = pMsgGeneralResync->m_timestamp;

      if (pMsgGeneralResync->m_clock)
      {
        CLog::Log(LOGDEBUG, "COMXPlayerAudio - CDVDMsg::GENERAL_RESYNC(%f, 1)", m_audioClock);
        m_av_clock->Discontinuity(m_audioClock);
        //m_av_clock->OMXUpdateClock(m_audioClock);
      }
      else
        CLog::Log(LOGDEBUG, "COMXPlayerAudio - CDVDMsg::GENERAL_RESYNC(%f, 0)", m_audioClock);
    }
    else if (pMsg->IsType(CDVDMsg::GENERAL_RESET))
    {
      if (m_pAudioCodec)
        m_pAudioCodec->Reset();
      m_started = false;
    }
    else if (pMsg->IsType(CDVDMsg::GENERAL_FLUSH))
    {
      CLog::Log(LOGDEBUG, "COMXPlayerAudio - CDVDMsg::GENERAL_FLUSH");
      m_av_clock->Lock();
      m_av_clock->OMXStop(false);
      m_omxAudio.Flush();
      m_av_clock->OMXReset(false);
      m_av_clock->UnLock();
      m_syncclock = true;
      m_stalled   = true;
      m_started   = false;

      if (m_pAudioCodec)
        m_pAudioCodec->Reset();
    }
    else if (pMsg->IsType(CDVDMsg::PLAYER_STARTED))
    {
      if(m_started)
        m_messageParent.Put(new CDVDMsgInt(CDVDMsg::PLAYER_STARTED, DVDPLAYER_AUDIO));
    }
    else if (pMsg->IsType(CDVDMsg::GENERAL_EOF))
    {
      CLog::Log(LOGDEBUG, "COMXPlayerAudio - CDVDMsg::GENERAL_EOF");
      WaitCompletion();
    }
    else if (pMsg->IsType(CDVDMsg::GENERAL_DELAY))
    {
      if (m_speed != DVD_PLAYSPEED_PAUSE)
      {
        double timeout = static_cast<CDVDMsgDouble*>(pMsg)->m_value;

        CLog::Log(LOGDEBUG, "COMXPlayerAudio - CDVDMsg::GENERAL_DELAY(%f)", timeout);

        timeout *= (double)DVD_PLAYSPEED_NORMAL / abs(m_speed);
        timeout += m_av_clock->GetAbsoluteClock();

        while(!m_bStop && m_av_clock->GetAbsoluteClock() < timeout)
          Sleep(1);
      }
    }
    else if (pMsg->IsType(CDVDMsg::PLAYER_SETSPEED))
    {
      CLog::Log(LOGDEBUG, "COMXPlayerAudio - CDVDMsg::PLAYER_SETSPEED");
      m_speed = static_cast<CDVDMsgInt*>(pMsg)->m_value;
      if (m_speed != DVD_PLAYSPEED_NORMAL)
      {
        m_syncclock = true;
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
  bool hdmi_passthrough_dts = false;
  bool hdmi_passthrough_ac3 = false;

  if (m_DllBcmHost.vc_tv_hdmi_audio_supported(EDID_AudioFormat_eAC3, 2, EDID_AudioSampleRate_e44KHz, EDID_AudioSampleSize_16bit ) == 0)
    hdmi_passthrough_ac3 = true;
  if (m_DllBcmHost.vc_tv_hdmi_audio_supported(EDID_AudioFormat_eDTS, 2, EDID_AudioSampleRate_e44KHz, EDID_AudioSampleSize_16bit ) == 0)
    hdmi_passthrough_dts = true;
  //printf("Audio support AC3=%d, DTS=%d\n", hdmi_passthrough_ac3, hdmi_passthrough_dts);

  m_passthrough = false;
  m_hw_decode   = false;

  /* check our audio capabilties */

  /* pathrought is overriding hw decode*/
  if(AUDIO_IS_BITSTREAM(g_guiSettings.GetInt("audiooutput.mode")) && m_use_passthrough)
  {
    if(hints.codec == CODEC_ID_AC3 && g_guiSettings.GetBool("audiooutput.ac3passthrough") && hdmi_passthrough_ac3)
    {
      dataFormat = AE_FMT_AC3;
      m_passthrough = true;
    }
    if(hints.codec == CODEC_ID_DTS && g_guiSettings.GetBool("audiooutput.dtspassthrough") && hdmi_passthrough_dts)
    {
      dataFormat = AE_FMT_DTS;
      m_passthrough = true;
    }
  }

  /* hw decode */
  if(m_use_hw_decode && !m_passthrough)
  {
    if(hints.codec == CODEC_ID_AC3 && COMXAudio::CanHWDecode(m_hints.codec))
    {
      dataFormat = AE_FMT_AC3;
      m_hw_decode = true;
    }
    if(hints.codec == CODEC_ID_DTS && COMXAudio::CanHWDecode(m_hints.codec))
    {
      dataFormat = AE_FMT_DTS;
      m_hw_decode = true;
    }
  }

  /* software path */
  if(!m_passthrough && !m_hw_decode)
  {
    /* 6 channel have to be mapped to 8 for PCM */
    if(m_nChannels > 4)
      m_nChannels = 8;
    dataFormat = AE_FMT_S16NE;
  }

  return dataFormat;
}

bool OMXPlayerAudio::OpenDecoder()
{
  m_nChannels   = m_hints.channels;
  m_passthrough = false;
  m_hw_decode   = false;

  m_omxAudio.SetClock(m_av_clock);

  m_av_clock->Lock();
  m_av_clock->OMXStop(false);

  /* setup audi format for audio render */
  m_format.m_sampleRate    = m_hints.samplerate;
  m_format.m_channelLayout = m_pAudioCodec->GetChannelMap(); 
  /* GetDataFormat is setting up evrything */
  m_format.m_dataFormat = GetDataFormat(m_hints);

  std::string device = "";
  
  if(g_guiSettings.GetInt("audiooutput.mode") == AUDIO_HDMI)
    device = "hdmi";
  else
    device = "local";

  bool bAudioRenderOpen = m_omxAudio.Initialize(m_format, device, m_av_clock, m_hints, m_passthrough, m_hw_decode);

  m_codec_name = "";
  
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

  m_av_clock->HasAudio(bAudioRenderOpen);
  m_av_clock->OMXReset(false);
  m_av_clock->UnLock();

  return bAudioRenderOpen;
}

void OMXPlayerAudio::CloseDecoder()
{
  m_av_clock->Lock();
  m_av_clock->OMXStop(false);
  m_av_clock->HasAudio(false);
  m_omxAudio.Deinitialize();
  m_av_clock->OMXReset(false);
  m_av_clock->UnLock();

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

void OMXPlayerAudio::WaitCompletion()
{
  if(!m_send_eos)
    m_omxAudio.WaitCompletion();
  m_send_eos = true;
}

void OMXPlayerAudio::RegisterAudioCallback(IAudioCallback *pCallback)
{
  m_omxAudio.RegisterAudioCallback(pCallback);
}

void OMXPlayerAudio::UnRegisterAudioCallback()
{
  m_omxAudio.UnRegisterAudioCallback();
}

void OMXPlayerAudio::SetCurrentVolume(float fVolume)
{
  m_omxAudio.SetCurrentVolume(fVolume);
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
