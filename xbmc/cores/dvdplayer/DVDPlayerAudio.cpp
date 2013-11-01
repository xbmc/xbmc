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
#include "DVDPlayerAudio.h"
#include "DVDPlayer.h"
#include "DVDCodecs/Audio/DVDAudioCodec.h"
#include "DVDCodecs/DVDCodecs.h"
#include "DVDCodecs/DVDFactoryCodec.h"
#include "DVDPerformanceCounter.h"
#include "settings/Settings.h"
#include "video/VideoReferenceClock.h"
#include "utils/log.h"
#include "utils/TimeUtils.h"
#include "utils/MathUtils.h"
#include "cores/AudioEngine/AEFactory.h"
#include "cores/AudioEngine/Utils/AEUtil.h"

#include <sstream>
#include <iomanip>

/* for sync-based resampling */
#define PROPORTIONAL 20.0
#define PROPREF       0.01
#define PROPDIVMIN    2.0
#define PROPDIVMAX   40.0
#define INTEGRAL    200.0

using namespace std;

void CPTSInputQueue::Add(int64_t bytes, double pts)
{
  CSingleLock lock(m_sync);

  m_list.insert(m_list.begin(), make_pair(bytes, pts));
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


CDVDPlayerAudio::CDVDPlayerAudio(CDVDClock* pClock, CDVDMessageQueue& parent)
: CThread("DVDPlayerAudio")
, m_messageQueue("audio")
, m_messageParent(parent)
, m_dvdAudio((bool&)m_bStop)
{
  m_pClock = pClock;
  m_pAudioCodec = NULL;
  m_audioClock = 0;
  m_droptime = 0;
  m_speed = DVD_PLAYSPEED_NORMAL;
  m_stalled = true;
  m_started = false;
  m_silence = false;
  m_duration = 0.0;
  m_resampleratio = 1.0;
  m_synctype = SYNC_DISCON;
  m_setsynctype = SYNC_DISCON;
  m_prevsynctype = -1;
  m_error = 0;
  m_errorbuff = 0;
  m_errorcount = 0;
  m_syncclock = true;
  m_integral = 0;
  m_skipdupcount = 0;
  m_prevskipped = false;
  m_maxspeedadjust = 0.0;

  m_errortime = 0;
  m_freq = CurrentHostFrequency();

  m_messageQueue.SetMaxDataSize(6 * 1024 * 1024);
  m_messageQueue.SetMaxTimeSize(8.0);
  g_dvdPerformanceCounter.EnableAudioQueue(&m_messageQueue);
}

CDVDPlayerAudio::~CDVDPlayerAudio()
{
  StopThread();
  g_dvdPerformanceCounter.DisableAudioQueue();

  // close the stream, and don't wait for the audio to be finished
  // CloseStream(true);
}

bool CDVDPlayerAudio::OpenStream( CDVDStreamInfo &hints )
{
  CLog::Log(LOGNOTICE, "Finding audio codec for: %i", hints.codec);
  CDVDAudioCodec* codec = CDVDFactoryCodec::CreateAudioCodec(hints);
  if( !codec )
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
    CLog::Log(LOGNOTICE, "Creating audio thread");
    Create();
  }
  return true;
}

void CDVDPlayerAudio::OpenStream( CDVDStreamInfo &hints, CDVDAudioCodec* codec )
{
  SAFE_DELETE(m_pAudioCodec);
  m_pAudioCodec = codec;

  /* store our stream hints */
  m_streaminfo = hints;

  /* update codec information from what codec gave out, if any */
  int channelsFromCodec = m_pAudioCodec->GetChannels();
  int samplerateFromCodec = m_pAudioCodec->GetEncodedSampleRate();

  if (channelsFromCodec > 0)
    m_streaminfo.channels = channelsFromCodec;
  if (samplerateFromCodec > 0)
    m_streaminfo.samplerate = samplerateFromCodec;

  /* check if we only just got sample rate, in which case the previous call
   * to CreateAudioCodec() couldn't have started passthrough */
  if (hints.samplerate != m_streaminfo.samplerate)
    SwitchCodecIfNeeded();

  m_droptime = 0;
  m_audioClock = 0;
  m_stalled = m_messageQueue.GetPacketCount(CDVDMsg::DEMUXER_PACKET) == 0;
  m_started = false;

  m_synctype = SYNC_DISCON;
  m_setsynctype = SYNC_DISCON;
  if (CSettings::Get().GetBool("videoplayer.usedisplayasclock"))
    m_setsynctype = CSettings::Get().GetInt("videoplayer.synctype");
  m_prevsynctype = -1;

  m_error = 0;
  m_errorbuff = 0;
  m_errorcount = 0;
  m_integral = 0;
  m_skipdupcount = 0;
  m_prevskipped = false;
  m_syncclock = true;
  m_errortime = CurrentHostCounter();
  m_silence = false;

  m_maxspeedadjust = CSettings::Get().GetNumber("videoplayer.maxspeedadjust");
}

void CDVDPlayerAudio::CloseStream(bool bWaitForBuffers)
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
int CDVDPlayerAudio::DecodeFrame(DVDAudioFrame &audioframe, bool bDropPacket)
{
  int result = 0;

  // make sure the sent frame is clean
  memset(&audioframe, 0, sizeof(DVDAudioFrame));

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
      m_audioStats.AddSampleBytes(m_decode.size);
      if (len < 0)
      {
        /* if error, we skip the packet */
        CLog::Log(LOGERROR, "CDVDPlayerAudio::DecodeFrame - Decode Error. Skipping audio packet");
        m_decode.Release();
        m_pAudioCodec->Reset();
        return DECODE_FLAG_ERROR;
      }

      // fix for fucked up decoders
      if( len > m_decode.size )
      {
        CLog::Log(LOGERROR, "CDVDPlayerAudio:DecodeFrame - Codec tried to consume more data than available. Potential memory corruption");
        m_decode.Release();
        m_pAudioCodec->Reset();
        return DECODE_FLAG_ERROR;
      }

      m_decode.data += len;
      m_decode.size -= len;


      // get decoded data and the size of it
      audioframe.size = m_pAudioCodec->GetData(&audioframe.data);
      audioframe.pts  = m_audioClock;

      if (audioframe.size == 0)
        continue;

      audioframe.channel_layout        = m_pAudioCodec->GetChannelMap();
      audioframe.channel_count         = m_pAudioCodec->GetChannels();
      audioframe.encoded_channel_count = m_pAudioCodec->GetEncodedChannels();
      audioframe.data_format           = m_pAudioCodec->GetDataFormat();
      audioframe.bits_per_sample       = CAEUtil::DataFormatToBits(audioframe.data_format);
      audioframe.sample_rate           = m_pAudioCodec->GetSampleRate();
      audioframe.encoded_sample_rate   = m_pAudioCodec->GetEncodedSampleRate();
      audioframe.passthrough           = m_pAudioCodec->NeedPassthrough();

      if (m_streaminfo.samplerate != audioframe.encoded_sample_rate)
      {
        // The sample rate has changed or we just got it for the first time
        // for this stream. See if we should enable/disable passthrough due
        // to it.
        m_streaminfo.samplerate = audioframe.encoded_sample_rate;
        if (!switched && SwitchCodecIfNeeded()) {
          // passthrough has been enabled/disabled, reprocess the packet
          m_decode.data -= len;
          m_decode.size += len;
          switched = true;
          continue;
        }
      }

      // compute duration.
      int n = (audioframe.channel_count * audioframe.bits_per_sample * audioframe.sample_rate)>>3;
      if (n > 0)
      {
        // safety check, if channels == 0, n will result in 0, and that will result in a nice devide exception
        audioframe.duration = ((double)audioframe.size * DVD_TIME_BASE) / n;

        // increase audioclock to after the packet
        m_audioClock += audioframe.duration;
      }

      if(audioframe.duration > 0)
        m_duration = audioframe.duration;

      // if demux source want's us to not display this, continue
      if(m_decode.msg->GetPacketDrop())
        continue;

      //If we are asked to drop this packet, return a size of zero. then it won't be played
      //we currently still decode the audio.. this is needed since we still need to know it's
      //duration to make sure clock is updated correctly.
      if( bDropPacket )
        result |= DECODE_FLAG_DROP;

      return result;
    }
    // free the current packet
    m_decode.Release();

    if (m_messageQueue.ReceivedAbortRequest()) return DECODE_FLAG_ABORT;

    CDVDMsg* pMsg;
    int priority = (m_speed == DVD_PLAYSPEED_PAUSE && m_started) ? 1 : 0;

    int timeout;
    if(m_duration > 0)
      timeout = (int)(1000 * (m_duration / DVD_TIME_BASE + m_dvdAudio.GetCacheTime()));
    else
      timeout = 1000;

    // read next packet and return -1 on error
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
        CLog::Log(LOGDEBUG, "CDVDPlayerAudio - CDVDMsg::GENERAL_SYNCHRONIZE");
      else
        m_messageQueue.Put(pMsg->Acquire(), 1); /* push back as prio message, to process other prio messages */
    }
    else if (pMsg->IsType(CDVDMsg::GENERAL_RESYNC))
    { //player asked us to set internal clock
      CDVDMsgGeneralResync* pMsgGeneralResync = (CDVDMsgGeneralResync*)pMsg;

      if (pMsgGeneralResync->m_timestamp != DVD_NOPTS_VALUE)
        m_audioClock = pMsgGeneralResync->m_timestamp;

      m_ptsInput.Flush();
      m_dvdAudio.SetPlayingPts(m_audioClock);
      if (pMsgGeneralResync->m_clock)
      {
        CLog::Log(LOGDEBUG, "CDVDPlayerAudio - CDVDMsg::GENERAL_RESYNC(%f, 1)", m_audioClock);
        m_pClock->Discontinuity(m_dvdAudio.GetPlayingPts());
      }
      else
        CLog::Log(LOGDEBUG, "CDVDPlayerAudio - CDVDMsg::GENERAL_RESYNC(%f, 0)", m_audioClock);
    }
    else if (pMsg->IsType(CDVDMsg::GENERAL_RESET))
    {
      if (m_pAudioCodec)
        m_pAudioCodec->Reset();
      m_decode.Release();
      m_started = false;
    }
    else if (pMsg->IsType(CDVDMsg::GENERAL_FLUSH))
    {
      m_dvdAudio.Flush();
      m_ptsInput.Flush();
      m_syncclock = true;
      m_stalled   = true;
      m_started   = false;

      if (m_pAudioCodec)
        m_pAudioCodec->Reset();

      m_decode.Release();
    }
    else if (pMsg->IsType(CDVDMsg::PLAYER_STARTED))
    {
      if(m_started)
        m_messageParent.Put(new CDVDMsgInt(CDVDMsg::PLAYER_STARTED, DVDPLAYER_AUDIO));
    }
    else if (pMsg->IsType(CDVDMsg::PLAYER_DISPLAYTIME))
    {
      CDVDPlayer::SPlayerState& state = ((CDVDMsgType<CDVDPlayer::SPlayerState>*)pMsg)->m_value;

      if(state.time_src == CDVDPlayer::ETIMESOURCE_CLOCK)
        state.time      = DVD_TIME_TO_MSEC(m_pClock->GetClock(state.timestamp) + state.time_offset);
      else
        state.timestamp = CDVDClock::GetAbsoluteClock();
      state.player    = DVDPLAYER_AUDIO;
      m_messageParent.Put(pMsg->Acquire());
    }
    else if (pMsg->IsType(CDVDMsg::GENERAL_EOF))
    {
      CLog::Log(LOGDEBUG, "CDVDPlayerAudio - CDVDMsg::GENERAL_EOF");
      m_dvdAudio.Finish();
    }
    else if (pMsg->IsType(CDVDMsg::GENERAL_DELAY))
    {
      if (m_speed != DVD_PLAYSPEED_PAUSE)
      {
        double timeout = static_cast<CDVDMsgDouble*>(pMsg)->m_value;

        CLog::Log(LOGDEBUG, "CDVDPlayerAudio - CDVDMsg::GENERAL_DELAY(%f)", timeout);

        timeout *= (double)DVD_PLAYSPEED_NORMAL / abs(m_speed);
        timeout += CDVDClock::GetAbsoluteClock();

        while(!m_bStop && CDVDClock::GetAbsoluteClock() < timeout)
          Sleep(1);
      }
    }
    else if (pMsg->IsType(CDVDMsg::PLAYER_SETSPEED))
    {
      m_speed = static_cast<CDVDMsgInt*>(pMsg)->m_value;

      if (m_speed == DVD_PLAYSPEED_NORMAL)
      {
        m_dvdAudio.Resume();
      }
      else
      {
        m_syncclock = true;
        if (m_speed != DVD_PLAYSPEED_PAUSE)
          m_dvdAudio.Flush();
        m_dvdAudio.Pause();
      }
    }
    else if (pMsg->IsType(CDVDMsg::AUDIO_SILENCE))
    {
      m_silence = static_cast<CDVDMsgBool*>(pMsg)->m_value;
      if (m_silence)
        CLog::Log(LOGDEBUG, "CDVDPlayerAudio - CDVDMsg::AUDIO_SILENCE(%f, 1)", m_audioClock);
      else
        CLog::Log(LOGDEBUG, "CDVDPlayerAudio - CDVDMsg::AUDIO_SILENCE(%f, 0)", m_audioClock);
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

void CDVDPlayerAudio::OnStartup()
{
  m_decode.Release();

  g_dvdPerformanceCounter.EnableAudioDecodePerformance(this);

#ifdef TARGET_WINDOWS
  CoInitializeEx(NULL, COINIT_MULTITHREADED);
#endif
}

void CDVDPlayerAudio::UpdatePlayerInfo()
{
  std::ostringstream s;
  s << "aq:"     << setw(2) << min(99,m_messageQueue.GetLevel() + MathUtils::round_int(100.0/8.0*m_dvdAudio.GetCacheTime())) << "%";
  s << ", Kb/s:" << fixed << setprecision(2) << (double)GetAudioBitrate() / 1024.0;

  //print the inverse of the resample ratio, since that makes more sense
  //if the resample ratio is 0.5, then we're playing twice as fast
  if (m_synctype == SYNC_RESAMPLE)
    s << ", rr:" << fixed << setprecision(5) << 1.0 / m_resampleratio;

  s << ", att:" << fixed << setprecision(1) << log(GetCurrentAttenuation()) * 20.0f << " dB";

  { CSingleLock lock(m_info_section);
    m_info = s.str();
  }
}

void CDVDPlayerAudio::Process()
{
  CLog::Log(LOGNOTICE, "running thread: CDVDPlayerAudio::Process()");

  bool packetadded(false);

  DVDAudioFrame audioframe;
  m_audioStats.Start();

  while (!m_bStop)
  {
    //Don't let anybody mess with our global variables
    int result = DecodeFrame(audioframe, m_speed > DVD_PLAYSPEED_NORMAL || m_speed < 0 ||
                         CAEFactory::IsSuspended()); // blocks if no audio is available, but leaves critical section before doing so

    UpdatePlayerInfo();

    if( result & DECODE_FLAG_ERROR )
    {
      CLog::Log(LOGDEBUG, "CDVDPlayerAudio::Process - Decode Error");
      continue;
    }

    if( result & DECODE_FLAG_TIMEOUT )
    {
      m_stalled = true;

      // Flush as the audio output may keep looping if we don't
      if(m_speed == DVD_PLAYSPEED_NORMAL)
      {
        m_dvdAudio.Drain();
        m_dvdAudio.Flush();
      }

      continue;
    }

    if( result & DECODE_FLAG_ABORT )
    {
      CLog::Log(LOGDEBUG, "CDVDPlayerAudio::Process - Abort received, exiting thread");
      break;
    }

#ifdef PROFILE /* during profiling we just drop all packets, after having decoded */
    m_pClock->Discontinuity(audioframe.pts);
    continue;
#endif

    if( audioframe.size == 0 )
      continue;

    packetadded = true;

    // we have succesfully decoded an audio frame, setup renderer to match
    if (!m_dvdAudio.IsValidFormat(audioframe))
    {
      if(m_speed)
        m_dvdAudio.Drain();

      m_dvdAudio.Destroy();

      if(m_speed)
        m_dvdAudio.Resume();
      else
        m_dvdAudio.Pause();

      if(!m_dvdAudio.Create(audioframe, m_streaminfo.codec, m_setsynctype == SYNC_RESAMPLE))
        CLog::Log(LOGERROR, "%s - failed to create audio renderer", __FUNCTION__);
    }

    // Zero out the frame data if we are supposed to silence the audio
    if (m_silence)
      memset(audioframe.data, 0, audioframe.size);

    if( result & DECODE_FLAG_DROP )
    {
      //frame should be dropped. Don't let audio move ahead of the current time thou
      //we need to be able to start playing at any time
      //when playing backwords, we try to keep as small buffers as possible

      if(m_droptime == 0.0)
        m_droptime = m_pClock->GetAbsoluteClock();
      if(m_speed > 0)
        m_droptime += audioframe.duration * DVD_PLAYSPEED_NORMAL / m_speed;
      while( !m_bStop && m_droptime > m_pClock->GetAbsoluteClock() ) Sleep(1);

      m_stalled = false;
    }
    else
    {
      m_droptime = 0.0;

      SetSyncType(audioframe.passthrough);

      // add any packets play
      packetadded = OutputPacket(audioframe);

      // we are not running until something is cached in output device
      if(m_stalled && m_dvdAudio.GetCacheTime() > 0.0)
        m_stalled = false;
    }

    // signal to our parent that we have initialized
    if(m_started == false)
    {
      m_started = true;
      m_messageParent.Put(new CDVDMsgInt(CDVDMsg::PLAYER_STARTED, DVDPLAYER_AUDIO));
    }

    if( m_dvdAudio.GetPlayingPts() == DVD_NOPTS_VALUE )
      continue;

    if( m_speed != DVD_PLAYSPEED_NORMAL )
      continue;

    if (packetadded)
      HandleSyncError(audioframe.duration);
  }
}

void CDVDPlayerAudio::SetSyncType(bool passthrough)
{
  //set the synctype from the gui
  //use skip/duplicate when resample is selected and passthrough is on
  m_synctype = m_setsynctype;
  if (passthrough && m_synctype == SYNC_RESAMPLE)
    m_synctype = SYNC_SKIPDUP;

  //tell dvdplayervideo how much it can change the speed
  //if SetMaxSpeedAdjust returns false, it means no video is played and we need to use clock feedback
  double maxspeedadjust = 0.0;
  if (m_synctype == SYNC_RESAMPLE)
    maxspeedadjust = m_maxspeedadjust;

  if (!m_pClock->SetMaxSpeedAdjust(maxspeedadjust))
    m_synctype = SYNC_DISCON;

  if (m_synctype != m_prevsynctype)
  {
    const char *synctypes[] = {"clock feedback", "skip/duplicate", "resample", "invalid"};
    int synctype = (m_synctype >= 0 && m_synctype <= 2) ? m_synctype : 3;
    CLog::Log(LOGDEBUG, "CDVDPlayerAudio:: synctype set to %i: %s", m_synctype, synctypes[synctype]);
    m_prevsynctype = m_synctype;
  }

  CDVDClock::SetMasterClock(false);
}

void CDVDPlayerAudio::HandleSyncError(double duration)
{
  double clock = m_pClock->GetClock();
  double error = m_dvdAudio.GetPlayingPts() - clock;
  int64_t now;

  if( fabs(error) > DVD_MSEC_TO_TIME(100) || m_syncclock )
  {
    m_pClock->Discontinuity(clock+error);
    if(m_speed == DVD_PLAYSPEED_NORMAL)
      CLog::Log(LOGDEBUG, "CDVDPlayerAudio:: Discontinuity1 - was:%f, should be:%f, error:%f", clock, clock+error, error);

    m_errorbuff = 0;
    m_errorcount = 0;
    m_skipdupcount = 0;
    m_error = 0;
    m_syncclock = false;
    m_errortime = CurrentHostCounter();

    return;
  }

  if (m_speed != DVD_PLAYSPEED_NORMAL)
  {
    m_errorbuff = 0;
    m_errorcount = 0;
    m_integral = 0;
    m_skipdupcount = 0;
    m_error = 0;
    m_errortime = CurrentHostCounter();
    return;
  }

  m_errorbuff += error;
  m_errorcount++;

  //check if measured error for 2 seconds
  now = CurrentHostCounter();
  if ((now - m_errortime) >= m_freq * 2)
  {
    m_errortime = now;
    m_error = m_errorbuff / m_errorcount;

    m_errorbuff = 0;
    m_errorcount = 0;

    if (m_synctype == SYNC_DISCON)
    {
      double limit, error;
      if (g_VideoReferenceClock.GetRefreshRate(&limit) > 0)
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

      if (fabs(error) > limit - 0.001)
      {
        m_pClock->Discontinuity(clock+error);
        if(m_speed == DVD_PLAYSPEED_NORMAL)
          CLog::Log(LOGDEBUG, "CDVDPlayerAudio:: Discontinuity2 - was:%f, should be:%f, error:%f", clock, clock+error, error);
      }
    }
    else if (m_synctype == SYNC_SKIPDUP && m_skipdupcount == 0 && fabs(m_error) > DVD_MSEC_TO_TIME(10))
    {
      //check how many packets to skip/duplicate
      m_skipdupcount = (int)(m_error / duration);
      //if less than one frame off, see if it's more than two thirds of a frame, so we can get better in sync
      if (m_skipdupcount == 0 && fabs(m_error) > duration / 3 * 2)
        m_skipdupcount = (int)(m_error / (duration / 3 * 2));

      if (m_skipdupcount > 0)
        CLog::Log(LOGDEBUG, "CDVDPlayerAudio:: Duplicating %i packet(s) of %.2f ms duration",
                  m_skipdupcount, duration / DVD_TIME_BASE * 1000.0);
      else if (m_skipdupcount < 0)
        CLog::Log(LOGDEBUG, "CDVDPlayerAudio:: Skipping %i packet(s) of %.2f ms duration ",
                  m_skipdupcount * -1,  duration / DVD_TIME_BASE * 1000.0);
    }
    else if (m_synctype == SYNC_RESAMPLE)
    {
      //reset the integral on big errors, failsafe
      if (fabs(m_error) > DVD_TIME_BASE)
        m_integral = 0;
      else if (fabs(m_error) > DVD_MSEC_TO_TIME(5))
        m_integral += m_error / DVD_TIME_BASE / INTEGRAL;
    }
  }
}

bool CDVDPlayerAudio::OutputPacket(DVDAudioFrame &audioframe)
{
  if (m_synctype == SYNC_DISCON)
  {
    m_dvdAudio.AddPackets(audioframe);
  }
  else if (m_synctype == SYNC_SKIPDUP)
  {
    if (m_skipdupcount < 0)
    {
      m_prevskipped = !m_prevskipped;
      if (!m_prevskipped)
      {
        m_dvdAudio.AddPackets(audioframe);
        m_skipdupcount++;
      }
    }
    else if (m_skipdupcount > 0)
    {
      m_dvdAudio.AddPackets(audioframe);
      m_dvdAudio.AddPackets(audioframe);
      m_skipdupcount--;
    }
    else if (m_skipdupcount == 0)
    {
      m_dvdAudio.AddPackets(audioframe);
    }
  }
  else if (m_synctype == SYNC_RESAMPLE)
  {
    double proportional = 0.0;

    //on big errors use more proportional
    if (fabs(m_error / DVD_TIME_BASE) > 0.0)
    {
      double proportionaldiv = PROPORTIONAL * (PROPREF / fabs(m_error / DVD_TIME_BASE));
      if (proportionaldiv < PROPDIVMIN) proportionaldiv = PROPDIVMIN;
      else if (proportionaldiv > PROPDIVMAX) proportionaldiv = PROPDIVMAX;

      proportional = m_error / DVD_TIME_BASE / proportionaldiv;
    }

    m_resampleratio = 1.0 / g_VideoReferenceClock.GetSpeed() + proportional + m_integral;
    m_dvdAudio.SetResampleRatio(m_resampleratio);
    m_dvdAudio.AddPackets(audioframe);
  }

  return true;
}

void CDVDPlayerAudio::OnExit()
{
  g_dvdPerformanceCounter.DisableAudioDecodePerformance();

#ifdef TARGET_WINDOWS
  CoUninitialize();
#endif

  CLog::Log(LOGNOTICE, "thread end: CDVDPlayerAudio::OnExit()");
}

void CDVDPlayerAudio::SetSpeed(int speed)
{
  if(m_messageQueue.IsInited())
    m_messageQueue.Put( new CDVDMsgInt(CDVDMsg::PLAYER_SETSPEED, speed), 1 );
  else
    m_speed = speed;
}

void CDVDPlayerAudio::Flush()
{
  m_messageQueue.Flush();
  m_messageQueue.Put( new CDVDMsg(CDVDMsg::GENERAL_FLUSH), 1);
}

void CDVDPlayerAudio::WaitForBuffers()
{
  // make sure there are no more packets available
  m_messageQueue.WaitUntilEmpty();

  // make sure almost all has been rendered
  // leave 500ms to avound buffer underruns
  double delay = m_dvdAudio.GetCacheTime();
  if(delay > 0.5)
    Sleep((int)(1000 * (delay - 0.5)));
}

bool CDVDPlayerAudio::SwitchCodecIfNeeded()
{
  CLog::Log(LOGDEBUG, "CDVDPlayerAudio: Sample rate changed, checking for passthrough");
  CDVDAudioCodec *codec = CDVDFactoryCodec::CreateAudioCodec(m_streaminfo);
  if (!codec || codec->NeedPassthrough() == m_pAudioCodec->NeedPassthrough()) {
    // passthrough state has not changed
    delete codec;
    return false;
  }

  delete m_pAudioCodec;
  m_pAudioCodec = codec;
  return true;
}

string CDVDPlayerAudio::GetPlayerInfo()
{
  CSingleLock lock(m_info_section);
  return m_info;
}

int CDVDPlayerAudio::GetAudioBitrate()
{
  return (int)m_audioStats.GetBitrate();
}

bool CDVDPlayerAudio::IsPassthrough() const
{
  return m_pAudioCodec && m_pAudioCodec->NeedPassthrough();
}
