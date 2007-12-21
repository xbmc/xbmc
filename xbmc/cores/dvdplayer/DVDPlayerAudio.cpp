
#include "stdafx.h"
#include "DVDPlayerAudio.h"
#include "DVDCodecs/Audio/DVDAudioCodec.h"
#include "DVDCodecs/DVDCodecs.h"
#include "DVDCodecs/DVDFactoryCodec.h"
#include "DVDPerformanceCounter.h"
#include <sstream>

CPTSOutputQueue::CPTSOutputQueue()
{
  Flush();
}

void CPTSOutputQueue::Add(double pts, double delay, double duration)
{
  TPTSItem item;
  item.pts = pts;
  item.timestamp = CDVDClock::GetAbsoluteClock() + delay;
  item.duration = duration;
  m_queue.push(item);

  // call function to make sure the queue 
  // doesn't grow should nobody call it
  Current();
}
void CPTSOutputQueue::Flush()
{
  while( !m_queue.empty() ) m_queue.pop();
  m_current.pts = DVD_NOPTS_VALUE;
  m_current.timestamp = 0.0;
  m_current.duration = 0.0;
}

double CPTSOutputQueue::Current()
{   
  while( !m_queue.empty() && CDVDClock::GetAbsoluteClock() >= m_queue.front().timestamp )
  {
    m_current = m_queue.front();
    m_queue.pop();
  }

  if( m_current.timestamp == 0 ) return m_current.pts;

  return m_current.pts + min(m_current.duration, (CDVDClock::GetAbsoluteClock() - m_current.timestamp));
}  

void CPTSInputQueue::Add(__int64 bytes, double pts)
{
  m_list.insert(m_list.begin(), make_pair(bytes, pts));
}

void CPTSInputQueue::Flush()
{
  m_list.clear();
}
double CPTSInputQueue::Get(__int64 bytes, bool consume)
{
  IT it = m_list.begin();
  for(; it != m_list.end(); it++)
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

CDVDPlayerAudio::CDVDPlayerAudio(CDVDClock* pClock) 
: CThread()
, m_dvdAudio((bool&)m_bStop)
{
  m_pClock = pClock;
  m_pAudioCodec = NULL;  
  m_audioClock = 0;
  m_droptime = 0;
  m_speed = DVD_PLAYSPEED_NORMAL;
  m_stalled = false;

  InitializeCriticalSection(&m_critCodecSection);
  m_messageQueue.SetMaxDataSize(30 * 16 * 1024);
  g_dvdPerformanceCounter.EnableAudioQueue(&m_messageQueue);
}

CDVDPlayerAudio::~CDVDPlayerAudio()
{
  StopThread();
  g_dvdPerformanceCounter.DisableAudioQueue();

  // close the stream, and don't wait for the audio to be finished
  // CloseStream(true);
  DeleteCriticalSection(&m_critCodecSection);
}

bool CDVDPlayerAudio::OpenStream( CDVDStreamInfo &hints )                                 
{
  // should alway's be NULL!!!!, it will probably crash anyway when deleting m_pAudioCodec here.
  if (m_pAudioCodec)
  {
    CLog::Log(LOGFATAL, "CDVDPlayerAudio::OpenStream() m_pAudioCodec != NULL");
    return false;
  }

  /* try to open decoder without probing, we could actually allow us to continue here */
  if( !OpenDecoder(hints) ) return false;

  m_messageQueue.Init();

  m_droptime = 0;
  m_audioClock = 0;
  m_stalled = false;

  CLog::Log(LOGNOTICE, "Creating audio thread");
  Create();

  return true;
}

void CDVDPlayerAudio::CloseStream(bool bWaitForBuffers)
{
  // wait until buffers are empty
  if (bWaitForBuffers) m_messageQueue.WaitUntilEmpty();

  // send abort message to the audio queue
  m_messageQueue.Abort();

  CLog::Log(LOGNOTICE, "waiting for audio thread to exit");

  // shut down the adio_decode thread and wait for it
  StopThread(); // will set this->m_bStop to true
  this->WaitForThreadExit(INFINITE);

  // uninit queue
  m_messageQueue.End();

  CLog::Log(LOGNOTICE, "Deleting audio codec");
  if (m_pAudioCodec)
  {
    m_pAudioCodec->Dispose();
    delete m_pAudioCodec;
    m_pAudioCodec = NULL;
  }

  // flush any remaining pts values
  m_ptsOutput.Flush();
}

bool CDVDPlayerAudio::OpenDecoder(CDVDStreamInfo &hints, BYTE* buffer /* = NULL*/, unsigned int size /* = 0*/)
{
  EnterCriticalSection(&m_critCodecSection);

  /* close current audio codec */
  if( m_pAudioCodec )
  {
    CLog::Log(LOGNOTICE, "Deleting audio codec");
    m_pAudioCodec->Dispose();
    SAFE_DELETE(m_pAudioCodec);
  }

  /* store our stream hints */
  m_streaminfo = hints;

  CLog::Log(LOGNOTICE, "Finding audio codec for: %i", m_streaminfo.codec);
  m_pAudioCodec = CDVDFactoryCodec::CreateAudioCodec( m_streaminfo );
  if( !m_pAudioCodec )
  {
    CLog::Log(LOGERROR, "Unsupported audio codec");

    m_streaminfo.Clear();
    LeaveCriticalSection(&m_critCodecSection);
    return false;
  }

  /* update codec information from what codec gave ut */  
  m_streaminfo.channels = m_pAudioCodec->GetChannels();
  m_streaminfo.samplerate = m_pAudioCodec->GetSampleRate();
  
  LeaveCriticalSection(&m_critCodecSection);

  return true;
}

// decode one audio frame and returns its uncompressed size
int CDVDPlayerAudio::DecodeFrame(DVDAudioFrame &audioframe, bool bDropPacket)
{
  int result = 0;
  int datatimeout = 1000;

  // make sure the sent frame is clean
  memset(&audioframe, 0, sizeof(DVDAudioFrame));

  while (!m_bStop)
  {
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
        assert(0);
      }

      m_decode.data += len;
      m_decode.size -= len;

      // get decoded data and the size of it
      audioframe.size = m_pAudioCodec->GetData(&audioframe.data);
      audioframe.pts = m_audioClock;
      audioframe.channels = m_pAudioCodec->GetChannels();
      audioframe.bits_per_sample = m_pAudioCodec->GetBitsPerSample();
      audioframe.sample_rate = m_pAudioCodec->GetSampleRate();
      audioframe.passthrough = m_pAudioCodec->NeedPasstrough();

      if (audioframe.size <= 0) continue;

      // compute duration.
      int n = (audioframe.channels * audioframe.bits_per_sample * audioframe.sample_rate)>>3;
      if (n > 0)
      {
        // safety check, if channels == 0, n will result in 0, and that will result in a nice devide exception
        audioframe.duration = ((double)audioframe.size * DVD_TIME_BASE) / n;

        // increase audioclock to after the packet
        m_audioClock += audioframe.duration;
        datatimeout = (unsigned int)(audioframe.duration*2.0);
      }

      //If we are asked to drop this packet, return a size of zero. then it won't be played
      //we currently still decode the audio.. this is needed since we still need to know it's 
      //duration to make sure clock is updated correctly.
      if( bDropPacket )
      {
        result |= DECODE_FLAG_DROP;
      }
      return result;
    }
    // free the current packet
    m_decode.Release();

    if (m_messageQueue.RecievedAbortRequest()) return DECODE_FLAG_ABORT;
    
    // read next packet and return -1 on error
    LeaveCriticalSection(&m_critCodecSection); //Leave here as this might stall a while
    
    CDVDMsg* pMsg;
    MsgQueueReturnCode ret = m_messageQueue.Get(&pMsg, datatimeout);
    
    EnterCriticalSection(&m_critCodecSection);
    
    if (ret == MSGQ_TIMEOUT) 
    {
      m_stalled = true;
      continue;
    }

    if (MSGQ_IS_ERROR(ret) || ret == MSGQ_ABORT) 
      return DECODE_FLAG_ABORT;

    if (pMsg->IsType(CDVDMsg::DEMUXER_PACKET))
    {
      m_stalled = false;
      m_decode.Attach((CDVDMsgDemuxerPacket*)pMsg);
      m_ptsInput.Add( m_decode.size, m_decode.dts );
    }
    else if (pMsg->IsType(CDVDMsg::GENERAL_STREAMCHANGE))
    {
      CDVDMsgGeneralStreamChange* pMsgStreamChange = (CDVDMsgGeneralStreamChange*)pMsg;
      CDVDStreamInfo* hints = pMsgStreamChange->GetStreamInfo();

      /* recieved a stream change, reopen codec. */
      /* we should really not do this untill first packet arrives, to have a probe buffer */      

      /* try to open decoder, if none is found keep consuming packets */
      OpenDecoder( *hints );

    }
    else if (pMsg->IsType(CDVDMsg::GENERAL_SYNCHRONIZE))
    {
      CDVDMsgGeneralSynchronize* pMsgGeneralSynchronize = (CDVDMsgGeneralSynchronize*)pMsg;
      
      CLog::Log(LOGDEBUG, "CDVDPlayerAudio - CDVDMsg::GENERAL_SYNCHRONIZE");

      pMsgGeneralSynchronize->Wait( &m_bStop, SYNCSOURCE_AUDIO );
    } 
    else if (pMsg->IsType(CDVDMsg::GENERAL_RESYNC))
    { //player asked us to set internal clock
      CDVDMsgGeneralResync* pMsgGeneralResync = (CDVDMsgGeneralResync*)pMsg;                  

      if (pMsgGeneralResync->m_timestamp != DVD_NOPTS_VALUE)
        m_audioClock = pMsgGeneralResync->m_timestamp;
      
      if (pMsgGeneralResync->m_clock)
        result |= DECODE_FLAG_RESYNC;
    }
    else if (pMsg->IsType(CDVDMsg::GENERAL_FLUSH))
    {
      m_dvdAudio.Flush();
      m_ptsOutput.Flush();
      m_ptsInput.Flush();

      if (m_pAudioCodec)
        m_pAudioCodec->Reset();        

      m_decode.Release();
    }
    pMsg->Release();
  }
}

void CDVDPlayerAudio::OnStartup()
{
  CThread::SetName("CDVDPlayerAudio");

  m_decode.msg = NULL;
  m_decode.Release();

  g_dvdPerformanceCounter.EnableAudioDecodePerformance(ThreadHandle());
}

void CDVDPlayerAudio::Process()
{
  CLog::Log(LOGNOTICE, "running thread: CDVDPlayerAudio::Process()");

  int result;
 
  DVDAudioFrame audioframe;
  m_audioStats.Start();

  while (!m_bStop)
  {
    //make sure player doesn't keep processing data while paused
    while (!m_bStop && m_speed == DVD_PLAYSPEED_PAUSE && !m_messageQueue.RecievedAbortRequest()) Sleep(5);

    //Don't let anybody mess with our global variables
    EnterCriticalSection(&m_critCodecSection);
    result = DecodeFrame(audioframe, m_speed != DVD_PLAYSPEED_NORMAL); // blocks if no audio is available, but leaves critical section before doing so
    LeaveCriticalSection(&m_critCodecSection);

    if( result & DECODE_FLAG_ERROR ) 
    { 
      CLog::Log(LOGDEBUG, "CDVDPlayerAudio::Process - Decode Error");
      continue;
    }

    if( result & DECODE_FLAG_ABORT )
    {
      CLog::Log(LOGDEBUG, "CDVDPlayerAudio::Process - Abort recieved, exiting thread");
      break;
    }

#ifdef PROFILE /* during profiling we just drop all packets, after having decoded */
    m_pClock->Discontinuity(CLOCK_DISC_NORMAL, audioframe.pts, 0);
    continue;
#endif
    
    if( audioframe.size == 0 )
      continue;

    // we have succesfully decoded an audio frame, setup renderer to match
    if (!m_dvdAudio.IsValidFormat(audioframe))
    {
      m_dvdAudio.Destroy();
      if(!m_dvdAudio.Create(audioframe, m_streaminfo.codec))
        CLog::Log(LOGERROR, "%s - failed to create audio renderer", __FUNCTION__);
    }

    if( result & DECODE_FLAG_DROP )
    {
      //frame should be dropped. Don't let audio move ahead of the current time thou
      //we need to be able to start playing at any time
      //when playing backwords, we try to keep as small buffers as possible

      if(m_droptime == 0.0)
        m_droptime = m_pClock->GetAbsoluteClock();
      m_droptime += audioframe.duration * DVD_PLAYSPEED_NORMAL / m_speed;
      while( !m_bStop && m_droptime > m_pClock->GetAbsoluteClock() ) Sleep(1);
    } 
    else
    {
      m_droptime = 0.0;

      // add any packets play
      m_dvdAudio.AddPackets(audioframe);
    }

    // store the delay for this pts value so we can calculate the current playing
    if(m_speed != DVD_PLAYSPEED_PAUSE)
      m_ptsOutput.Add(audioframe.pts, m_dvdAudio.GetDelay() - audioframe.duration, audioframe.duration);

    // if we wanted to resync, we resync on this packet
    if( result & DECODE_FLAG_RESYNC )
    {
      m_pClock->Discontinuity(CLOCK_DISC_NORMAL, audioframe.pts, m_dvdAudio.GetDelay() - audioframe.duration);
      CLog::Log(LOGDEBUG, "CDVDPlayerAudio:: Resync - clock:%f, delay:%f", audioframe.pts, m_dvdAudio.GetDelay() - audioframe.duration);
    }
    
    // don't try to fix a desynced clock, until we played out the full audio buffer
    if( fabs(m_pClock->DistanceToDisc()) < m_dvdAudio.GetDelay() )
      continue;
    
    if( m_ptsOutput.Current() == DVD_NOPTS_VALUE )
      continue;

    if( m_speed != DVD_PLAYSPEED_NORMAL )
      continue;

    double clock = m_pClock->GetClock();
    double error = m_ptsOutput.Current() - clock;

    if( fabs(error) > DVD_MSEC_TO_TIME(10) )
    {
      m_pClock->Discontinuity(CLOCK_DISC_NORMAL, clock+error, 0);      
      if(m_speed == DVD_PLAYSPEED_NORMAL)
        CLog::Log(LOGDEBUG, "CDVDPlayerAudio:: Discontinuty - was:%f, should be:%f, error:%f", clock, clock+error, error);
    }
  }
}

void CDVDPlayerAudio::OnExit()
{
  g_dvdPerformanceCounter.DisableAudioDecodePerformance();
  
  // destroy audio device
  CLog::Log(LOGNOTICE, "Closing audio device");
  m_dvdAudio.Destroy();

  CLog::Log(LOGNOTICE, "thread end: CDVDPlayerAudio::OnExit()");
}

void CDVDPlayerAudio::SetSpeed(int speed)
{ 
  m_speed = speed;
  
  if (m_speed == DVD_PLAYSPEED_PAUSE)
  {
    m_ptsOutput.Flush();
    m_dvdAudio.Pause();
  }
  else 
    m_dvdAudio.Resume();
}

void CDVDPlayerAudio::Flush()
{
  m_messageQueue.Flush();
  m_messageQueue.Put( new CDVDMsgGeneralFlush() );
}

void CDVDPlayerAudio::WaitForBuffers()
{
  // make sure there are no more packets available
  m_messageQueue.WaitUntilEmpty();
  
  // make sure almost all has been rendered
  // leave 500ms to avound buffer underruns

  while( m_dvdAudio.GetDelay() > DVD_TIME_BASE/2 )
  {
    Sleep(5);
  }
}

string CDVDPlayerAudio::GetPlayerInfo()
{
  std::ostringstream s;
  s << "aq size:" << min(100,100 * m_messageQueue.GetDataSize() / m_messageQueue.GetMaxDataSize()) << "%";
  s << ", ";
  s << "cpu: " << (int)(100 * CThread::GetRelativeUsage()) << "%, ";
  s << "bitrate: " << (GetAudioBitrate() / 8) / 1024 << " KBytes/s";
  return s.str();
}

int CDVDPlayerAudio::GetAudioBitrate()
{
  return (int)m_audioStats.GetBitrate();
}
