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
#include "DVDPlayerAudio.h"
#include "DVDCodecs/Audio/DVDAudioCodec.h"
#include "DVDCodecs/DVDCodecs.h"
#include "DVDCodecs/DVDFactoryCodec.h"
#include "DVDPerformanceCounter.h"
#include "../../Util.h"
#include "Settings.h"
#include <sstream>
#include <iomanip>
#include <iostream> //remove later, just for debugging
using namespace std;

CPTSOutputQueue::CPTSOutputQueue()
{
  Flush();
}

void CPTSOutputQueue::Add(double pts, double delay, double duration)
{
  CSingleLock lock(m_sync);
  
  TPTSItem item;
  item.pts = pts;
  item.timestamp = CDVDClock::GetAbsoluteClock() + delay;
  item.duration = duration;

  // first one is applied directly
  if(m_queue.empty() && m_current.pts == DVD_NOPTS_VALUE)
    m_current = item;
  else
    m_queue.push(item);

  // call function to make sure the queue 
  // doesn't grow should nobody call it
  Current();
}
void CPTSOutputQueue::Flush()
{
  CSingleLock lock(m_sync);
  
  while( !m_queue.empty() ) m_queue.pop();
  m_current.pts = DVD_NOPTS_VALUE;
  m_current.timestamp = 0.0;
  m_current.duration = 0.0;
}

double CPTSOutputQueue::Current()
{
  CSingleLock lock(m_sync);

  if(!m_queue.empty() && m_current.pts == DVD_NOPTS_VALUE)
  {
    m_current = m_queue.front();
    m_queue.pop();
  }

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
  CSingleLock lock(m_sync);
  
  m_list.insert(m_list.begin(), make_pair(bytes, pts));
}

void CPTSInputQueue::Flush()
{
  CSingleLock lock(m_sync);
  
  m_list.clear();
}
double CPTSInputQueue::Get(__int64 bytes, bool consume)
{
  CSingleLock lock(m_sync);
  
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
, m_messageQueue("audio")
, m_dvdAudio((bool&)m_bStop)
{
  m_pClock = pClock;
  m_pAudioCodec = NULL;  
  m_audioClock = 0;
  m_droptime = 0;
  m_speed = DVD_PLAYSPEED_NORMAL;
  m_stalled = false;
  m_started = false;

  InitializeCriticalSection(&m_critCodecSection);
  m_messageQueue.SetMaxDataSize(30 * 16 * 1024);
  g_dvdPerformanceCounter.EnableAudioQueue(&m_messageQueue);
  
  PCMSynctype = SYNC_DISCON;
  AC3DTSSynctype = SYNC_DISCON;
  ErrorCount = 0;
  SamplesMeasured = 0;
  AverageError = 0.0;
  SkipDupCount = 0;
  PrevSkipped = false;
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
  m_started = false;

  CLog::Log(LOGNOTICE, "Creating audio thread");
  Create();
  
  PCMSynctype = g_guiSettings.GetInt("audiooutput.analogsynctype");
  SyncToVideoClock = g_guiSettings.GetBool("videoplayer.synctodisplay");
  
  Offset = 1.0;
  
  if (!SyncToVideoClock)
  {
    PCMSynctype = SYNC_DISCON;
    AC3DTSSynctype = SYNC_DISCON;
  }
  else
  {
    AC3DTSSynctype = SYNC_SKIPDUP;
  }
  
  return true;
}

void CDVDPlayerAudio::CloseStream(bool bWaitForBuffers)
{
  // wait until buffers are empty
  if (bWaitForBuffers && m_speed > 0) m_messageQueue.WaitUntilEmpty();

  // send abort message to the audio queue
  m_messageQueue.Abort();

  CLog::Log(LOGNOTICE, "Waiting for audio thread to exit");

  // shut down the adio_decode thread and wait for it
  StopThread(); // will set this->m_bStop to true

  // destroy audio device
  CLog::Log(LOGNOTICE, "Closing audio device");
  if (bWaitForBuffers && m_speed > 0)
  {
    m_bStop = false;
    m_dvdAudio.Drain();
    m_bStop = true;
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

      if (audioframe.size <= 0) 
        continue;

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

    if (m_messageQueue.RecievedAbortRequest()) return DECODE_FLAG_ABORT;

    CDVDMsg* pMsg;
    int iPriority = (m_speed == DVD_PLAYSPEED_PAUSE) ? 1 : 0;
    // read next packet and return -1 on error
    LeaveCriticalSection(&m_critCodecSection); //Leave here as this might stall a while
    MsgQueueReturnCode ret = m_messageQueue.Get(&pMsg, datatimeout, iPriority);
    EnterCriticalSection(&m_critCodecSection);

    if (ret == MSGQ_TIMEOUT) 
      return DECODE_FLAG_TIMEOUT;

    if (MSGQ_IS_ERROR(ret) || ret == MSGQ_ABORT) 
      return DECODE_FLAG_ABORT;

    if (pMsg->IsType(CDVDMsg::DEMUXER_PACKET))
    {
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
      ((CDVDMsgGeneralSynchronize*)pMsg)->Wait( &m_bStop, SYNCSOURCE_AUDIO );
      CLog::Log(LOGDEBUG, "CDVDPlayerAudio - CDVDMsg::GENERAL_SYNCHRONIZE");
    } 
    else if (pMsg->IsType(CDVDMsg::GENERAL_RESYNC))
    { //player asked us to set internal clock
      CDVDMsgGeneralResync* pMsgGeneralResync = (CDVDMsgGeneralResync*)pMsg;
      
      if (pMsgGeneralResync->m_timestamp != DVD_NOPTS_VALUE)
        m_audioClock = pMsgGeneralResync->m_timestamp;

      m_ptsOutput.Add(m_audioClock, m_dvdAudio.GetDelay(), 0);
      if (pMsgGeneralResync->m_clock)
      {
        CLog::Log(LOGDEBUG, "CDVDPlayerAudio - CDVDMsg::GENERAL_RESYNC(%f, 1)", m_audioClock);
        m_pClock->Discontinuity(CLOCK_DISC_NORMAL, m_ptsOutput.Current(), 0);
      }
      else
        CLog::Log(LOGDEBUG, "CDVDPlayerAudio - CDVDMsg::GENERAL_RESYNC(%f, 0)", m_audioClock);
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

      if (m_speed == DVD_PLAYSPEED_PAUSE)
      {
        m_ptsOutput.Flush();
        m_dvdAudio.Pause();
      }
      else 
        m_dvdAudio.Resume();
    }
    pMsg->Release();
  }
  return 0;
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

  double clock, error;
  
  while (!m_bStop)
  {
    //Don't let anybody mess with our global variables
    EnterCriticalSection(&m_critCodecSection);
    result = DecodeFrame(audioframe, m_speed != DVD_PLAYSPEED_NORMAL); // blocks if no audio is available, but leaves critical section before doing so
    LeaveCriticalSection(&m_critCodecSection);

    if( result & DECODE_FLAG_ERROR ) 
    { 
      CLog::Log(LOGDEBUG, "CDVDPlayerAudio::Process - Decode Error");
      continue;
    }

    if( result & DECODE_FLAG_TIMEOUT ) 
    {
      if(m_started)
        m_stalled = true;
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

    m_stalled = false;
    m_started = true;

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
      if(m_speed > 0)
        m_droptime += audioframe.duration * DVD_PLAYSPEED_NORMAL / m_speed;
      while( !m_bStop && m_droptime > m_pClock->GetAbsoluteClock() ) Sleep(1);
    } 
    else
    {
      bool new_error = false;
      m_droptime = 0.0;
      
      //check if measured for at least one second
      if (SamplesMeasured > audioframe.sample_rate)
      {
        AverageError /= (double)(ErrorCount);
        new_error = true;
      }

      if ((AC3DTSSynctype == SYNC_DISCON && audioframe.passthrough) || (PCMSynctype == SYNC_DISCON && !audioframe.passthrough))
      {
        if (new_error && fabs(AverageError) > DVD_MSEC_TO_TIME(10))
        {
          clock = m_pClock->GetClock();
          m_pClock->Discontinuity(CLOCK_DISC_NORMAL, clock + AverageError, 0);
          if(m_speed == DVD_PLAYSPEED_NORMAL)
            CLog::Log(LOGDEBUG, "CDVDPlayerAudio:: Discontinuty - was:%f, should be:%f, error:%f", clock, clock + AverageError, AverageError);
        }
        m_dvdAudio.AddPackets(audioframe);
        SkipDupCount = 0;
      }
      else if ((AC3DTSSynctype == SYNC_SKIPDUP && audioframe.passthrough) || (PCMSynctype == SYNC_SKIPDUP && !audioframe.passthrough))
      {
        //skip/duplicate required number of frames
        if (SkipDupCount > 0)
        {
          m_dvdAudio.AddPackets(audioframe);
          m_dvdAudio.AddPackets(audioframe);
          SkipDupCount--;
        }
        else if (SkipDupCount < 0)
        {
          if ((PrevSkipped = !PrevSkipped))  m_dvdAudio.AddPackets(audioframe);
          else SkipDupCount++;
        }
        else
        {
          m_dvdAudio.AddPackets(audioframe);
          
          if (new_error)
          {
            //check how many packets to skip/duplicate
            SkipDupCount = (int)(AverageError / audioframe.duration);
            //if less than one frame off, see if it's more than two thirds of a frame, so we can get better in sync*/
            if (SkipDupCount == 0 && fabs(AverageError) > audioframe.duration / 3 * 2)
              SkipDupCount = (int)(AverageError / (audioframe.duration / 3 * 2));
            
            if (SkipDupCount > 0) CLog::Log(LOGDEBUG, "CDVDPlayerAudio:: Duplicating %i packet(s)", SkipDupCount);
            else if (SkipDupCount < 0) CLog::Log(LOGDEBUG, "CDVDPlayerAudio:: Skipping %i packet(s)", SkipDupCount * -1);
          }
        }
      }
      else if (PCMSynctype == SYNC_RESAMPLE && !audioframe.passthrough)
      {
        if(new_error)
        {
          if (fabs(AverageError) < DVD_MSEC_TO_TIME(100))
          {
            Offset += AverageError / DVD_TIME_BASE / 50.0;
          }
#ifndef _WIN32
          //for making pretty graphs
          static int count = 0;
          cerr << count++ << " " << AverageError << " " << Offset << "\n";
#endif
        }

        if (fabs(AverageError) < DVD_MSEC_TO_TIME(100))
          Resampler.SetRatio(Offset);
        else
          Resampler.SetRatio(Offset + error / DVD_TIME_BASE);
        
        Resampler.Add(audioframe);
        while(Resampler.Retreive(audioframe)) m_dvdAudio.AddPackets(audioframe);
        
        SkipDupCount = 0;
      }
      if (new_error) ResetErrorCounter();
    }
    

    // store the delay for this pts value so we can calculate the current playing
    if(m_speed != DVD_PLAYSPEED_PAUSE)
      m_ptsOutput.Add(audioframe.pts, m_dvdAudio.GetDelay() - audioframe.duration, audioframe.duration);

    if (m_ptsOutput.Current() == DVD_NOPTS_VALUE)
      continue;
    
    if( m_speed != DVD_PLAYSPEED_NORMAL)
      continue;
    
    clock = m_pClock->GetClock();
    error = m_ptsOutput.Current() - clock;
    
    if (fabs(error) > DVD_MSEC_TO_TIME(200) && !SyncToVideoClock)
    {
      m_pClock->Discontinuity(CLOCK_DISC_NORMAL, clock + error, 0);
      if(m_speed == DVD_PLAYSPEED_NORMAL)
        CLog::Log(LOGDEBUG, "CDVDPlayerAudio:: Discontinuty - was:%f, should be:%f, error:%f", clock, clock + error, error);
      ResetErrorCounter();
    }
    
    //only add to average when not skipping
    if (SkipDupCount == 0)
    {
      AverageError += error;
      ErrorCount++;
      SamplesMeasured += audioframe.size / audioframe.channels / (audioframe.bits_per_sample / 8);
    }
  }
}

void CDVDPlayerAudio::ResetErrorCounter()
{
  AverageError = 0.0;
  ErrorCount = 0.0;
  SamplesMeasured = 0;
}

void CDVDPlayerAudio::OnExit()
{
  g_dvdPerformanceCounter.DisableAudioDecodePerformance();

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
  m_messageQueue.Put( new CDVDMsg(CDVDMsg::GENERAL_FLUSH));
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
  s << "aq:" << std::setw(3) << min(99,100 * m_messageQueue.GetDataSize() / m_messageQueue.GetMaxDataSize()) << "%";
  s << ", ";
  s << "cpu: " << (int)(100 * CThread::GetRelativeUsage()) << "%, ";
  s << "bitrate: " << std::setprecision(6) << (double)GetAudioBitrate() / 1024.0 << " KBit/s";
  return s.str();
}

int CDVDPlayerAudio::GetAudioBitrate()
{
  return (int)m_audioStats.GetBitrate();
}

CDVDPlayerResampler::CDVDPlayerResampler()
{
  NrChannels = -1;
  Converter = NULL;
  ConverterData.data_in = NULL;
  ConverterData.data_out = NULL;
  ConverterData.end_of_input = 0;
  ConverterData.src_ratio = 1.0;
  RingBufferPos = 0;
  RingBufferFill = 0;
  RingBuffer = NULL;
}
  
CDVDPlayerResampler::~CDVDPlayerResampler()
{
  if (Converter) src_delete(Converter);
  if (ConverterData.data_in) delete ConverterData.data_in;
  if (ConverterData.data_out) delete ConverterData.data_out;
  if (RingBuffer) delete RingBuffer;
}

void CDVDPlayerResampler::Add(DVDAudioFrame audioframe)
{
  int Value, Bytes = audioframe.bits_per_sample / 8;
  int AddPos;
  CheckResampleBuffers(audioframe.channels);
  
  int NrSamples = audioframe.size / audioframe.channels / (audioframe.bits_per_sample / 8);
  if (NrSamples > MAXCONVSAMPLES) NrSamples = MAXCONVSAMPLES;
  
  for (int i = 0; i < NrSamples; i++)
  {
    for (int j = 0; j < NrChannels; j++)
    {
      Value = 0;
      for (int k = Bytes - 1; k >= 0; k--)
      {
        Value <<= 8;
        Value |= audioframe.data[i * NrChannels * Bytes + j * Bytes + k];
      }
      //check sign bit
      if (Value & (1 << (Bytes * 8 - 1)))
      {
        Value |= ~((1 << (Bytes * 8)) - 1);
      }
      //add to resampler buffer
      ConverterData.data_in[i * NrChannels + j] = (float)Value / (float)(unsigned int)(1 << (Bytes * 8 - 1));
    }
  }
  
  //resample
  ConverterData.input_frames = NrSamples;
  src_process(Converter, &ConverterData);
  
  //add samples to ringbuffer
  AddPos = RingBufferPos + RingBufferFill;
  if (AddPos >= RINGSIZE) AddPos -= RINGSIZE;
  
  for (int i = 0; i < ConverterData.output_frames_gen; i++)
  {
    for (int j = 0; j < NrChannels; j++)
    {
      RingBuffer[AddPos * NrChannels + j] = ConverterData.data_out[i * NrChannels + j];
    }
    RingBufferFill++;
    AddPos++;
    if (AddPos >= RINGSIZE) AddPos -= RINGSIZE;
  }
}

bool CDVDPlayerResampler::Retreive(DVDAudioFrame audioframe)
{
  int i, NrSamples = audioframe.size / audioframe.channels / (audioframe.bits_per_sample / 8);
  int Value, GetPos, Bytes = audioframe.bits_per_sample / 8;
  float Multiply = (float)(1 << (audioframe.bits_per_sample - 1)) - 1.0;
  
  CheckResampleBuffers(audioframe.channels);
  
  if (NrSamples > RingBufferFill)
  {
    return false;
  }
  
  //add from ringbuffer to audioframe
  for (i = 0; i < NrSamples; i++)
  {
    GetPos = RingBufferPos + i;
    if (GetPos >= RINGSIZE) GetPos -= RINGSIZE;
    
    for (int j = 0; j < NrChannels; j++)
    {
      Value = (int)Clamp(RingBuffer[GetPos * NrChannels + j] * Multiply, Multiply * -1.0, Multiply);
      for (int k = 0; k < Bytes; k++)
      {
        audioframe.data[i * NrChannels * Bytes + j * Bytes + k] = (BYTE)((Value >> (k * 8)) & 0xFF);
      }
    }
    RingBufferFill--;
  }
  RingBufferPos += i;
  if (RingBufferPos >= RINGSIZE) RingBufferPos -= RINGSIZE;
  
  return true;
}

void CDVDPlayerResampler::CheckResampleBuffers(int channels)
{
  int error;
  if (channels != NrChannels)
  {
    NrChannels = channels;
    if (Converter) src_delete(Converter);
    if (ConverterData.data_in) delete ConverterData.data_in;
    if (ConverterData.data_out) delete ConverterData.data_out;
    if (RingBuffer) delete RingBuffer;

    Converter = src_new(SRC_LINEAR, NrChannels, &error);
    ConverterData.data_in = new float[MAXCONVSAMPLES * NrChannels];
    ConverterData.data_out = new float[MAXCONVSAMPLES * NrChannels * 3];
    ConverterData.output_frames = MAXCONVSAMPLES * 3;
    RingBuffer = new float[RINGSIZE * NrChannels];
    RingBufferPos = 0;
    RingBufferFill = 0;
  }
}

void CDVDPlayerResampler::SetRatio(float ratio)
{
  if (ratio > 2.0) ratio = 2.0;
  else if (ratio < 0.5) ratio = 0.5;
  
  ConverterData.src_ratio = ratio;
  //IMPORTANT: libsamplerate adjust the ratio slowly based on ConverterData.output_frames
}