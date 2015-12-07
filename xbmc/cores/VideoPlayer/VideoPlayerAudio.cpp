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

#include "threads/SingleLock.h"
#include "VideoPlayerAudio.h"
#include "DVDCodecs/Audio/DVDAudioCodec.h"
#include "DVDCodecs/DVDFactoryCodec.h"
#include "settings/Settings.h"
#include "video/VideoReferenceClock.h"
#include "utils/log.h"
#include "utils/MathUtils.h"
#include "cores/AudioEngine/AEFactory.h"
#include "cores/AudioEngine/Utils/AEUtil.h"
#include "cores/DataCacheCore.h"

#include <sstream>
#include <iomanip>
#include <math.h>

// allow audio for slow and fast speeds (but not rewind/fastforward)
#define ALLOW_AUDIO(speed) ((speed) > 5*DVD_PLAYSPEED_NORMAL/10 && (speed) <= 15*DVD_PLAYSPEED_NORMAL/10)

void CPTSInputQueue::Add(int64_t bytes, double pts)
{
  CSingleLock lock(m_sync);

  m_list.insert(m_list.begin(), std::make_pair(bytes, pts));
}

void CPTSInputQueue::Flush()
{
  CSingleLock lock(m_sync);

  m_list.clear();
}
double CPTSInputQueue::Get(int64_t bytes, bool consume)
{
  CSingleLock lock(m_sync);

  IT it = m_list.begin();
  for(; it != m_list.end(); ++it)
  {
    if(bytes <= it->first)
    {
      double pts = it->second;
      if(consume)
      {
        it->second = DVD_NOPTS_VALUE;
        m_list.erase(++it, m_list.end());
      }
      return pts;
    }
    bytes -= it->first;
  }
  return DVD_NOPTS_VALUE;
}


class CDVDMsgAudioCodecChange : public CDVDMsg
{
public:
  CDVDMsgAudioCodecChange(const CDVDStreamInfo &hints, CDVDAudioCodec* codec)
    : CDVDMsg(GENERAL_STREAMCHANGE)
    , m_codec(codec)
    , m_hints(hints)
  {}
 ~CDVDMsgAudioCodecChange()
  {
    delete m_codec;
  }
  CDVDAudioCodec* m_codec;
  CDVDStreamInfo  m_hints;
};


CVideoPlayerAudio::CVideoPlayerAudio(CDVDClock* pClock, CDVDMessageQueue& parent)
: CThread("VideoPlayerAudio")
, m_messageQueue("audio")
, m_messageParent(parent)
, m_dvdAudio((bool&)m_bStop, pClock)
{
  m_pClock = pClock;
  m_pAudioCodec = NULL;
  m_audioClock = 0;
  m_speed = DVD_PLAYSPEED_NORMAL;
  m_stalled = true;
  m_syncState = IDVDStreamPlayer::SYNC_STARTING;
  m_silence = false;
  m_synctype = SYNC_DISCON;
  m_setsynctype = SYNC_DISCON;
  m_prevsynctype = -1;
  m_prevskipped = false;
  m_maxspeedadjust = 0.0;

  m_messageQueue.SetMaxDataSize(6 * 1024 * 1024);
  m_messageQueue.SetMaxTimeSize(8.0);
}

CVideoPlayerAudio::~CVideoPlayerAudio()
{
  StopThread();

  // close the stream, and don't wait for the audio to be finished
  // CloseStream(true);
}

bool CVideoPlayerAudio::OpenStream(CDVDStreamInfo &hints)
{
  CLog::Log(LOGNOTICE, "Finding audio codec for: %i", hints.codec);
  bool allowpassthrough = !CSettings::GetInstance().GetBool(CSettings::SETTING_VIDEOPLAYER_USEDISPLAYASCLOCK);
  CDVDAudioCodec* codec = CDVDFactoryCodec::CreateAudioCodec(hints, allowpassthrough);
  if(!codec)
  {
    CLog::Log(LOGERROR, "Unsupported audio codec");
    return false;
  }

  if(m_messageQueue.IsInited())
    m_messageQueue.Put(new CDVDMsgAudioCodecChange(hints, codec), 0);
  else
  {
    OpenStream(hints, codec);
    m_messageQueue.Init();
    m_syncState = IDVDStreamPlayer::SYNC_STARTING;
    CLog::Log(LOGNOTICE, "Creating audio thread");
    Create();
  }
  return true;
}

void CVideoPlayerAudio::OpenStream(CDVDStreamInfo &hints, CDVDAudioCodec* codec)
{
  SAFE_DELETE(m_pAudioCodec);
  m_pAudioCodec = codec;

  /* store our stream hints */
  m_streaminfo = hints;

  /* update codec information from what codec gave out, if any */
  int channelsFromCodec   = m_pAudioCodec->GetFormat().m_channelLayout.Count();
  int samplerateFromCodec = m_pAudioCodec->GetFormat().m_sampleRate;

  if (channelsFromCodec > 0)
    m_streaminfo.channels = channelsFromCodec;
  if (samplerateFromCodec > 0)
    m_streaminfo.samplerate = samplerateFromCodec;

  /* check if we only just got sample rate, in which case the previous call
   * to CreateAudioCodec() couldn't have started passthrough */
  if (hints.samplerate != m_streaminfo.samplerate)
    SwitchCodecIfNeeded();

  m_audioClock = 0;
  m_stalled = m_messageQueue.GetPacketCount(CDVDMsg::DEMUXER_PACKET) == 0;

  m_synctype = SYNC_DISCON;
  m_setsynctype = SYNC_DISCON;
  if (CSettings::GetInstance().GetBool(CSettings::SETTING_VIDEOPLAYER_USEDISPLAYASCLOCK))
    m_setsynctype = SYNC_RESAMPLE;
  m_prevsynctype = -1;

  m_prevskipped = false;
  m_silence = false;

  m_maxspeedadjust = 5.0;

  g_dataCacheCore.SignalAudioInfoChange();
}

void CVideoPlayerAudio::CloseStream(bool bWaitForBuffers)
{
  bool bWait = bWaitForBuffers && m_speed > 0 && !CAEFactory::IsSuspended();

  // wait until buffers are empty
  if (bWait) m_messageQueue.WaitUntilEmpty();

  // send abort message to the audio queue
  m_messageQueue.Abort();

  CLog::Log(LOGNOTICE, "Waiting for audio thread to exit");

  // shut down the adio_decode thread and wait for it
  StopThread(); // will set this->m_bStop to true

  // destroy audio device
  CLog::Log(LOGNOTICE, "Closing audio device");
  if (bWait)
  {
    m_bStop = false;
    m_dvdAudio.Drain();
    m_bStop = true;
  }
  else
  {
    m_dvdAudio.Flush();
  }

  m_dvdAudio.Destroy();

  // uninit queue
  m_messageQueue.End();

  CLog::Log(LOGNOTICE, "Deleting audio codec");
  if (m_pAudioCodec)
  {
    m_pAudioCodec->Dispose();
    delete m_pAudioCodec;
    m_pAudioCodec = NULL;
  }
}

// decode one audio frame and returns its uncompressed size
int CVideoPlayerAudio::DecodeFrame(DVDAudioFrame &audioframe)
{
  int result = 0;

  // make sure the sent frame is clean
  audioframe.nb_frames = 0;

  while (!m_bStop)
  {
    bool switched = false;
    /* NOTE: the audio packet can contain several frames */
    while( !m_bStop && m_decode.size > 0 )
    {
      if( !m_pAudioCodec )
        return DECODE_FLAG_ERROR;

      /* the packet dts refers to the first audioframe that starts in the packet */
      double dts = m_ptsInput.Get(m_decode.size + m_pAudioCodec->GetBufferSize(), true);
      if (dts != DVD_NOPTS_VALUE)
        m_audioClock = dts;

      int len = m_pAudioCodec->Decode(m_decode.data, m_decode.size);
      if (len < 0 || len > m_decode.size)
      {
        /* if error, we skip the packet */
        CLog::Log(LOGERROR, "CVideoPlayerAudio::DecodeFrame - Decode Error. Skipping audio packet (%d)", len);
        m_decode.Release();
        m_pAudioCodec->Reset();
        return DECODE_FLAG_ERROR;
      }

      m_audioStats.AddSampleBytes(len);

      m_decode.data += len;
      m_decode.size -= len;

      // get decoded data and the size of it
      m_pAudioCodec->GetData(audioframe);

      if (audioframe.nb_frames == 0)
        continue;

      if (audioframe.pts == DVD_NOPTS_VALUE)
        audioframe.pts = m_audioClock;

      if (audioframe.format.m_sampleRate && m_streaminfo.samplerate != audioframe.format.m_sampleRate)
      {
        // The sample rate has changed or we just got it for the first time
        // for this stream. See if we should enable/disable passthrough due
        // to it.
        m_streaminfo.samplerate = audioframe.format.m_sampleRate;
        if (!switched && SwitchCodecIfNeeded())
        {
          // passthrough has been enabled/disabled, reprocess the packet
          m_decode.data -= len;
          m_decode.size += len;
          switched = true;
          continue;
        }
      }

      // increase audioclock to after the packet
      m_audioClock += audioframe.duration;

      // if demux source want's us to not display this, continue
      if(m_decode.msg->GetPacketDrop())
        result |= DECODE_FLAG_DROP;

      return result;
    }
    // free the current packet
    m_decode.Release();

    if (m_messageQueue.ReceivedAbortRequest()) return DECODE_FLAG_ABORT;

    CDVDMsg* pMsg;
    int timeout  = (int)(1000 * m_dvdAudio.GetCacheTime()) + 100;

    // read next packet and return -1 on error
    int priority = 1;
    //Do we want a new audio frame?
    if (m_syncState == IDVDStreamPlayer::SYNC_STARTING ||              /* when not started */
        ALLOW_AUDIO(m_speed) || /* when playing normally */
        m_speed <  DVD_PLAYSPEED_PAUSE  || /* when rewinding */
       (m_speed >  DVD_PLAYSPEED_NORMAL && m_audioClock < m_pClock->GetClock())) /* when behind clock in ff */
      priority = 0;

    if (m_syncState == IDVDStreamPlayer::SYNC_WAITSYNC)
      priority = 1;

    // consider stream stalled if queue is empty
    // we can't sync audio to clock with an empty queue
    if (ALLOW_AUDIO(m_speed))
    {
      timeout = 0;
    }

    MsgQueueReturnCode ret = m_messageQueue.Get(&pMsg, timeout, priority);

    if (ret == MSGQ_TIMEOUT)
      return DECODE_FLAG_TIMEOUT;

    if (MSGQ_IS_ERROR(ret))
      return DECODE_FLAG_ABORT;

    if (pMsg->IsType(CDVDMsg::DEMUXER_PACKET))
    {
      m_decode.Attach((CDVDMsgDemuxerPacket*)pMsg);
      m_ptsInput.Add( m_decode.size, m_decode.dts );
    }
    else if (pMsg->IsType(CDVDMsg::GENERAL_SYNCHRONIZE))
    {
      if(((CDVDMsgGeneralSynchronize*)pMsg)->Wait( 100, SYNCSOURCE_AUDIO ))
        CLog::Log(LOGDEBUG, "CVideoPlayerAudio - CDVDMsg::GENERAL_SYNCHRONIZE");
      else
        m_messageQueue.Put(pMsg->Acquire(), 1); /* push back as prio message, to process other prio messages */
    }
    else if (pMsg->IsType(CDVDMsg::GENERAL_RESYNC))
    { //player asked us to set internal clock
      double pts = static_cast<CDVDMsgDouble*>(pMsg)->m_value;
      CLog::Log(LOGDEBUG, "CVideoPlayerAudio - CDVDMsg::GENERAL_RESYNC(%f)", pts);

      m_audioClock = pts;
      m_ptsInput.Flush();
      if (m_speed != DVD_PLAYSPEED_PAUSE)
        m_dvdAudio.Resume();
      m_syncState = IDVDStreamPlayer::SYNC_INSYNC;
    }
    else if (pMsg->IsType(CDVDMsg::GENERAL_RESET))
    {
      if (m_pAudioCodec)
        m_pAudioCodec->Reset();
      m_decode.Release();
    }
    else if (pMsg->IsType(CDVDMsg::GENERAL_FLUSH))
    {
      bool sync = static_cast<CDVDMsgBool*>(pMsg)->m_value;
      m_dvdAudio.Flush();
      m_ptsInput.Flush();
      m_stalled = true;

      if (sync)
      {
        m_syncState = IDVDStreamPlayer::SYNC_STARTING;
        m_dvdAudio.Pause();
      }

      if (m_pAudioCodec)
        m_pAudioCodec->Reset();

      m_decode.Release();
    }
    else if (pMsg->IsType(CDVDMsg::PLAYER_DISPLAYTIME))
    {
      SPlayerState& state = ((CDVDMsgType<SPlayerState>*)pMsg)->m_value;

      if(state.time_src == ETIMESOURCE_CLOCK)
        state.time = DVD_TIME_TO_MSEC(m_pClock->GetClock(state.timestamp) + state.time_offset);
      else
        state.timestamp = CDVDClock::GetAbsoluteClock();
      state.player = VideoPlayer_AUDIO;
      m_messageParent.Put(pMsg->Acquire());
    }
    else if (pMsg->IsType(CDVDMsg::GENERAL_EOF))
    {
      CLog::Log(LOGDEBUG, "CVideoPlayerAudio - CDVDMsg::GENERAL_EOF");
    }
    else if (pMsg->IsType(CDVDMsg::PLAYER_SETSPEED))
    {
      double speed = static_cast<CDVDMsgInt*>(pMsg)->m_value;

      if (ALLOW_AUDIO(speed))
      {
        if (speed != m_speed)
        {
          if (m_syncState == IDVDStreamPlayer::SYNC_INSYNC)
            m_dvdAudio.Resume();
        }
      }
      else
      {
        m_dvdAudio.Pause();
      }
      m_speed = speed;
    }
    else if (pMsg->IsType(CDVDMsg::AUDIO_SILENCE))
    {
      m_silence = static_cast<CDVDMsgBool*>(pMsg)->m_value;
      CLog::Log(LOGDEBUG, "CVideoPlayerAudio - CDVDMsg::AUDIO_SILENCE(%f, %d)"
                        , m_audioClock, m_silence);
    }
    else if (pMsg->IsType(CDVDMsg::GENERAL_STREAMCHANGE))
    {
      CDVDMsgAudioCodecChange* msg(static_cast<CDVDMsgAudioCodecChange*>(pMsg));
      OpenStream(msg->m_hints, msg->m_codec);
      msg->m_codec = NULL;
    }

    pMsg->Release();
  }
  return 0;
}

void CVideoPlayerAudio::OnStartup()
{
  m_decode.Release();

#ifdef TARGET_WINDOWS
  CoInitializeEx(NULL, COINIT_MULTITHREADED);
#endif
}

void CVideoPlayerAudio::UpdatePlayerInfo()
{
  std::ostringstream s;
  s << "aq:"     << std::setw(2) << std::min(99,m_messageQueue.GetLevel() + MathUtils::round_int(100.0/8.0*m_dvdAudio.GetCacheTime())) << "%";
  s << ", Kb/s:" << std::fixed << std::setprecision(2) << (double)GetAudioBitrate() / 1024.0;

  //print the inverse of the resample ratio, since that makes more sense
  //if the resample ratio is 0.5, then we're playing twice as fast
  if (m_synctype == SYNC_RESAMPLE)
    s << ", rr:" << std::fixed << std::setprecision(5) << 1.0 / m_dvdAudio.GetResampleRatio();

  s << ", att:" << std::fixed << std::setprecision(1) << log(GetCurrentAttenuation()) * 20.0f << " dB";

  SInfo info;
  info.info        = s.str();
  info.pts         = m_dvdAudio.GetPlayingPts();
  info.passthrough = m_pAudioCodec && m_pAudioCodec->NeedPassthrough();

  { CSingleLock lock(m_info_section);
    m_info = info;
  }
}

void CVideoPlayerAudio::Process()
{
  CLog::Log(LOGNOTICE, "running thread: CVideoPlayerAudio::Process()");

  DVDAudioFrame audioframe;
  m_audioStats.Start();

  while (!m_bStop)
  {
    int result = DecodeFrame(audioframe);

    //Drop when not playing normally
    if (!ALLOW_AUDIO(m_speed) && m_syncState == IDVDStreamPlayer::SYNC_INSYNC)
    {
      result |= DECODE_FLAG_DROP;
    }

    UpdatePlayerInfo();

    if (result & DECODE_FLAG_ERROR)
    {
      CLog::Log(LOGDEBUG, "CVideoPlayerAudio::Process - Decode Error");
      continue;
    }

    if (result & DECODE_FLAG_TIMEOUT)
    {
      // Flush as the audio output may keep looping if we don't
      if (ALLOW_AUDIO(m_speed) && !m_stalled && m_syncState == IDVDStreamPlayer::SYNC_INSYNC)
      {
        m_stalled = true;
      }

      continue;
    }

    if (result & DECODE_FLAG_ABORT)
    {
      CLog::Log(LOGDEBUG, "CVideoPlayerAudio::Process - Abort received, exiting thread");
      break;
    }

    if (audioframe.nb_frames == 0)
      continue;

    // demuxer reads metatags that influence channel layout
    if (m_streaminfo.codec == AV_CODEC_ID_FLAC && m_streaminfo.channellayout)
      audioframe.format.m_channelLayout = CAEUtil::GetAEChannelLayout(m_streaminfo.channellayout);
    
    // we have succesfully decoded an audio frame, setup renderer to match
    if (!m_dvdAudio.IsValidFormat(audioframe))
    {
      if(m_speed)
        m_dvdAudio.Drain();

      m_dvdAudio.Destroy();

      if(!m_dvdAudio.Create(audioframe, m_streaminfo.codec, m_setsynctype == SYNC_RESAMPLE))
        CLog::Log(LOGERROR, "%s - failed to create audio renderer", __FUNCTION__);

      if (m_syncState == IDVDStreamPlayer::SYNC_INSYNC)
        m_dvdAudio.Resume();

      m_streaminfo.channels = audioframe.format.m_channelLayout.Count();

      g_dataCacheCore.SignalAudioInfoChange();
    }

    // Zero out the frame data if we are supposed to silence the audio
    if (m_silence)
    {
      int size = audioframe.nb_frames * audioframe.framesize / audioframe.planes;
      for (unsigned int i=0; i<audioframe.planes; i++)
        memset(audioframe.data[i], 0, size);
    }

    if(!(result & DECODE_FLAG_DROP))
    {
      SetSyncType(audioframe.passthrough);

      OutputPacket(audioframe);
    }

    // signal to our parent that we have initialized
    if(m_syncState == IDVDStreamPlayer::SYNC_STARTING && !(result & DECODE_FLAG_DROP))
    {
      double cachetotal = DVD_SEC_TO_TIME(m_dvdAudio.GetCacheTotal());
      double cachetime = m_dvdAudio.GetDelay();
      if (cachetime >= cachetotal * 0.5)
      {
        m_syncState = IDVDStreamPlayer::SYNC_WAITSYNC;
        m_stalled = false;
        SStartMsg msg;
        msg.player = VideoPlayer_AUDIO;
        msg.cachetotal = cachetotal;
        msg.cachetime = cachetime;
        msg.timestamp = m_audioClock;
        m_messageParent.Put(new CDVDMsgType<SStartMsg>(CDVDMsg::PLAYER_STARTED, msg));
      }
    }
  }
}

void CVideoPlayerAudio::SetSyncType(bool passthrough)
{
  //set the synctype from the gui
  m_synctype = m_setsynctype;
  if (passthrough && m_synctype == SYNC_RESAMPLE)
    m_synctype = SYNC_DISCON;

  //if SetMaxSpeedAdjust returns false, it means no video is played and we need to use clock feedback
  double maxspeedadjust = 0.0;
  if (m_synctype == SYNC_RESAMPLE)
    maxspeedadjust = m_maxspeedadjust;

  m_pClock->SetMaxSpeedAdjust(maxspeedadjust);

  if (m_synctype != m_prevsynctype)
  {
    const char *synctypes[] = {"clock feedback", "resample", "invalid"};
    int synctype = (m_synctype >= 0 && m_synctype <= 1) ? m_synctype : 2;
    CLog::Log(LOGDEBUG, "CVideoPlayerAudio:: synctype set to %i: %s", m_synctype, synctypes[synctype]);
    m_prevsynctype = m_synctype;
    if (m_synctype == SYNC_RESAMPLE)
      m_dvdAudio.SetResampleMode(1);
    else
      m_dvdAudio.SetResampleMode(0);
  }
}

bool CVideoPlayerAudio::OutputPacket(DVDAudioFrame &audioframe)
{
  double syncerror = m_dvdAudio.GetSyncError();

  if (m_synctype == SYNC_DISCON)
  {
    double limit, error;
    limit = DVD_MSEC_TO_TIME(10);
    error = syncerror;

    double absolute;
    double clock = m_pClock->GetClock(absolute);
    if (m_pClock->Update(clock + error, absolute, limit - 0.001, "CVideoPlayerAudio::OutputPacket"))
    {
      m_dvdAudio.SetSyncErrorCorrection(-error);
    }
  }
  m_dvdAudio.AddPackets(audioframe);

  return true;
}

void CVideoPlayerAudio::OnExit()
{
#ifdef TARGET_WINDOWS
  CoUninitialize();
#endif

  CLog::Log(LOGNOTICE, "thread end: CVideoPlayerAudio::OnExit()");
}

void CVideoPlayerAudio::SetSpeed(int speed)
{
  if(m_messageQueue.IsInited())
    m_messageQueue.Put( new CDVDMsgInt(CDVDMsg::PLAYER_SETSPEED, speed), 1 );
  else
    m_speed = speed;
}

void CVideoPlayerAudio::Flush(bool sync)
{
  m_messageQueue.Flush();
  m_messageQueue.Put( new CDVDMsgBool(CDVDMsg::GENERAL_FLUSH, sync), 1);
}

void CVideoPlayerAudio::WaitForBuffers()
{
  // make sure there are no more packets available
  m_messageQueue.WaitUntilEmpty();

  // make sure almost all has been rendered
  // leave 500ms to avound buffer underruns
  double delay = m_dvdAudio.GetCacheTime();
  if(delay > 0.5)
    Sleep((int)(1000 * (delay - 0.5)));
}

bool CVideoPlayerAudio::SwitchCodecIfNeeded()
{
  CLog::Log(LOGDEBUG, "CVideoPlayerAudio: Sample rate changed, checking for passthrough");
  bool allowpassthrough = !CSettings::GetInstance().GetBool(CSettings::SETTING_VIDEOPLAYER_USEDISPLAYASCLOCK);
  CDVDAudioCodec *codec = CDVDFactoryCodec::CreateAudioCodec(m_streaminfo, allowpassthrough);
  if (!codec || codec->NeedPassthrough() == m_pAudioCodec->NeedPassthrough()) {
    // passthrough state has not changed
    delete codec;
    return false;
  }

  delete m_pAudioCodec;
  m_pAudioCodec = codec;

  return true;
}

std::string CVideoPlayerAudio::GetPlayerInfo()
{
  CSingleLock lock(m_info_section);
  return m_info.info;
}

int CVideoPlayerAudio::GetAudioBitrate()
{
  return (int)m_audioStats.GetBitrate();
}

int CVideoPlayerAudio::GetAudioChannels()
{
  return m_streaminfo.channels;
}

bool CVideoPlayerAudio::IsPassthrough() const
{
  CSingleLock lock(m_info_section);
  return m_info.passthrough;
}
