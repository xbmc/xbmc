
#include "../../stdafx.h"
#include "DVDPlayerAudio.h"
#include "DVDPlayer.h"
#include "DVDCodecs\Audio\DVDAudioCodec.h"
#include "DVDCodecs\DVDFactoryCodec.h"
#include "DVDCodecs\Audio\dvdaudiocodecpassthrough.h"
#include "DVDPerformanceCounter.h"

#include "..\..\util.h"

#define EMULATE_INTTYPES
#include "ffmpeg\avcodec.h"

#define ABS(a) ((a) >= 0 ? (a) : (-(a)))


CDVDPlayerAudio::CDVDPlayerAudio(CDVDClock* pClock) : CThread()
{
  m_pClock = pClock;
  m_pAudioCodec = NULL;
  m_bInitializedOutputDevice = false;
  m_iSourceChannels = 0;
  m_audioClock = 0;

  m_iSpeed = 1;
  
  InitializeCriticalSection(&m_critCodecSection);
  m_packetQueue.SetMaxSize(10 * 16 * 1024);
  g_dvdPerformanceCounter.EnableAudioQueue(&m_packetQueue);
}

CDVDPlayerAudio::~CDVDPlayerAudio()
{
  g_dvdPerformanceCounter.DisableAudioQueue();

  // close the stream, and don't wait for the audio to be finished
  // CloseStream(true);
  DeleteCriticalSection(&m_critCodecSection);
}

bool CDVDPlayerAudio::OpenStream( CDemuxStreamAudio *pDemuxStream )                                 
{
  // should alway's be NULL!!!!, it will probably crash anyway when deleting m_pAudioCodec here.
  if (m_pAudioCodec)
  {
    CLog::Log(LOGFATAL, "CDVDPlayerAudio::OpenStream() m_pAudioCodec != NULL");
    return false;
  }

  CodecID codecID = pDemuxStream->codec;

  int iWantedChannels = pDemuxStream->iChannels;
  if (iWantedChannels == 5) iWantedChannels = 6;

  //Let codec downmix for us int the case of ac3
  if (pDemuxStream->codec == CODEC_ID_AC3 || pDemuxStream->codec == CODEC_ID_DTS)
  {
    iWantedChannels = 2;
  }

  CLog::Log(LOGNOTICE, "Finding audio codec for: %i", codecID);
  m_pAudioCodec = CDVDFactoryCodec::CreateAudioCodec( pDemuxStream );
  if( !m_pAudioCodec )
  {
    CLog::Log(LOGERROR, "Unsupported audio codec");
    return false;
  }

  m_codec = pDemuxStream->codec;
  m_iSourceChannels = pDemuxStream->iChannels;

  m_packetQueue.Init();

  CLog::Log(LOGNOTICE, "Creating audio thread");
  Create();

  return true;
}

void CDVDPlayerAudio::CloseStream(bool bWaitForBuffers)
{
  // wait until buffers are empty
  if (bWaitForBuffers) m_packetQueue.WaitUntilEmpty();

  // send abort message to the audio queue
  m_packetQueue.Abort();

  CLog::Log(LOGNOTICE, "waiting for audio thread to exit");

  // shut down the adio_decode thread and wait for it
  StopThread(); // will set this->m_bStop to true
  this->WaitForThreadExit(INFINITE);

  // uninit queue
  m_packetQueue.End();

  CLog::Log(LOGNOTICE, "Deleting audio codec");
  if (m_pAudioCodec)
  {
    m_pAudioCodec->Dispose();
    delete m_pAudioCodec;
    m_pAudioCodec = NULL;
  }

  pAudioPacket = NULL;
}
// decode one audio frame and returns its uncompressed size
int CDVDPlayerAudio::DecodeFrame(BYTE** pAudioBuffer, int *data_size,bool bDropPacket)
{
  CDVDDemux::DemuxPacket* pPacket = pAudioPacket;
  int n=48000*2*16/8, len;

  //Store amount left at this point, and what last pts was
  unsigned __int64 first_pkt_pts = 0;
  int first_pkt_size = 0; 
  int first_pkt_used = 0;
  int result = 0;
  if (pPacket)
  {
    first_pkt_pts = pPacket->pts;
    first_pkt_size = pPacket->iSize;
    first_pkt_used = first_pkt_size - audio_pkt_size;
  }

  for (;;)
  {
    /* NOTE: the audio packet can contain several frames */
    while (audio_pkt_size > 0)
    {
      len = m_pAudioCodec->Decode(audio_pkt_data, audio_pkt_size);
      if (len < 0)
      {
        /* if error, we skip the frame */
        audio_pkt_size=0;
        m_pAudioCodec->Reset();
        break;
      }

      // fix for fucked up decoders
      if( len > audio_pkt_size )
      {        
        CLog::Log(LOGERROR, "CDVDPlayerAudio:DecodeFrame - Codec tried to consume more data than available. Potential memory corruption");        
        audio_pkt_size=0;
        m_pAudioCodec->Reset();
        assert(0);
      }

      // get decoded data and the size of it
      *data_size = m_pAudioCodec->GetData(pAudioBuffer);

      audio_pkt_data += len;
      audio_pkt_size -= len;

      if (*data_size <= 0) continue;
      
      // compute pts.
      n = m_pAudioCodec->GetChannels() * m_pAudioCodec->GetBitsPerSample() / 8 * m_pAudioCodec->GetSampleRate();
      if (n > 0)
      {
        // safety check, if channels == 0, n will result in 0, and that will result in a nice devide exception
        m_audioClock += ((__int64)(*data_size) * (__int64)DVD_TIME_BASE) / (__int64)n;
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
    if (pPacket)
    {
      CDVDDemuxUtils::FreeDemuxPacket(pPacket);
      pPacket = NULL;
      pAudioPacket = NULL;
    }

    if (m_packetQueue.RecievedAbortRequest()) return DECODE_FLAG_ABORT;
    
    // read next packet and return -1 on error
    int dvdstate=0, packstate=0;
    LeaveCriticalSection(&m_critCodecSection); //Leave here as this might stall a while
    packstate = m_packetQueue.Get(&pPacket, INFINITE, (void**)&dvdstate);
    EnterCriticalSection(&m_critCodecSection);
    if (packstate < 0) return DECODE_FLAG_ABORT;

    pAudioPacket = pPacket;
    audio_pkt_data = pPacket->pData;
    audio_pkt_size = pPacket->iSize;

    // if update the audio clock with the pts
    if (pPacket->pts != DVD_NOPTS_VALUE)
    {
      if (first_pkt_size == 0) 
      { //first package
        m_audioClock = pPacket->pts;        
      }
      else if( dvdstate & DVDPACKET_MESSAGE_RESYNC )
      { //player asked us to sync on this package
        result |= DECODE_FLAG_RESYNC;
        m_audioClock = pPacket->pts;
      }
      else if( first_pkt_pts > pPacket->pts )
      { //okey first packet in this continous stream, make sure we use the time here        
        m_audioClock = pPacket->pts;        
      }
      else if((unsigned __int64)m_audioClock < first_pkt_pts || (unsigned __int64)m_audioClock > pPacket->pts)
      {
        //crap, moved outsided correct pts
        //Use pts from current packet, untill we find a better value for it.
        //Should be ok after a couple of frames, as soon as it starts clean on a packet
        m_audioClock = pPacket->pts;
      }
      else if(first_pkt_size == first_pkt_used)
      { //Nice starting up freshly on the start of a packet, use pts from it
        m_audioClock = pPacket->pts;
      }
    }
  }
}

void CDVDPlayerAudio::OnStartup()
{
  CThread::SetName("CDVDPlayerAudio");
  pAudioPacket = NULL;
  m_audioClock = 0;
  audio_pkt_data = NULL;
  audio_pkt_size = 0;
  
  g_dvdPerformanceCounter.EnableAudioDecodePerformance(ThreadHandle());
}

void CDVDPlayerAudio::Process()
{
  CLog::Log(LOGNOTICE, "running thread: CDVDPlayerAudio::Process()");

  int len, result;

  // silence data
  BYTE silence[1024];
  memset(silence, 0, 1024);

  BYTE* pAudioBuffer;
  __int64 iClockDiff=0;
  while (!m_bStop)
  {
    //Don't let anybody mess with our global variables
    EnterCriticalSection(&m_critCodecSection);
    result = DecodeFrame(&pAudioBuffer, &len, m_iSpeed != 1); // blocks if no audio is available, but leaves critical section before doing so
    LeaveCriticalSection(&m_critCodecSection);

    if( result & DECODE_FLAG_ERROR ) 
    {      
      CLog::Log(LOGERROR, "CDVDPlayerAudio::Process - Decode Error. Skipping audio frame");
      continue;
    }

    if( result & DECODE_FLAG_ABORT )
    {
      CLog::Log(LOGDEBUG, "CDVDPlayerAudio::Process - Abort recieved, exiting thread");
      break;
    }

    if( result & DECODE_FLAG_RESYNC )
    {
      CLog::Log(LOGDEBUG, "CDVDPlayerAudio::Process - Resync recieved.");
      m_pClock->Discontinuity(CLOCK_DISC_NORMAL, m_audioClock - m_dvdAudio.GetDelay());
    }

    if( result & DECODE_FLAG_DROP )
    {
      //frame should be dropped. Don't let audio move ahead of the current time thou
      //we need to be able to start playing at any time
      //when playing backwords, we try to keep as small buffers as possible

      if( m_iSpeed > 0 )
      { 
        while( !m_bStop && m_audioClock - m_dvdAudio.GetDelay() > m_pClock->GetClock() ) Sleep(1);
      }

      continue;
    }
    
    if( len > 0 )
    {
      // we have succesfully decoded an audio frame, openup the audio device if not already done
      if (!m_bInitializedOutputDevice)
      {
        m_bInitializedOutputDevice = InitializeOutputDevice();
      }

      //Add any packets play
      m_dvdAudio.AddPackets(pAudioBuffer, len );      
    }

    //Clock should be calculated after packets have been added as m_audioClock points to the 
    //time after they have been played

    const __int64 iCurrDiff = (m_audioClock - m_dvdAudio.GetDelay()) - m_pClock->GetClock();
    const __int64 iAvDiff = (iClockDiff + iCurrDiff)/2;

    //Check for discontinuity in the stream, use a moving average to
    //eliminate highfreq fluctuations of large packet sizes
    if( ABS(iAvDiff) > 5000 ) // sync clock if average diff is bigger than 5 msec 
    {
      //Wait untill only the new audio frame wich triggered the discontinuity is left
      //then set disc state
      while (!m_bStop && m_dvdAudio.GetBytesInBuffer() > len ) Sleep(5);

      m_pClock->Discontinuity(CLOCK_DISC_NORMAL, m_audioClock - m_dvdAudio.GetDelay());
      CLog::DebugLog("CDVDPlayer:: Detected Audio Discontinuity, syncing clock. diff was: %I64d, %I64d, av: %I64d", iClockDiff, iCurrDiff, iAvDiff);
      iClockDiff = 0;
    }
    else
    {      
      //Do gradual adjustments (not working yet)
      //m_pClock->AdjustSpeedToMatch(iClock + iAvDiff);
      iClockDiff = iCurrDiff;
    }
  }
}

void CDVDPlayerAudio::OnExit()
{
  g_dvdPerformanceCounter.DisableAudioDecodePerformance();
  
  // destroy audio device
  CLog::Log(LOGNOTICE, "Closing audio device");
  m_dvdAudio.Destroy();
  m_bInitializedOutputDevice = false;

  CLog::Log(LOGNOTICE, "thread end: CDVDPlayerAudio::OnExit()");
}

void CDVDPlayerAudio::Flush()
{
  m_packetQueue.Flush();
  m_dvdAudio.Flush();

  if (m_pAudioCodec)
  {
    EnterCriticalSection(&m_critCodecSection);
    audio_pkt_size = 0;
    audio_pkt_data = NULL;
    m_pAudioCodec->Reset();
    LeaveCriticalSection(&m_critCodecSection);
  }
}

bool CDVDPlayerAudio::InitializeOutputDevice()
{
  int iChannels = m_pAudioCodec->GetChannels();
  int iSampleRate = m_pAudioCodec->GetSampleRate();
  int iBitsPerSample = m_pAudioCodec->GetBitsPerSample();
  bool bPasstrough = m_pAudioCodec->NeedPasstrough();

  if (iChannels == 0 || iSampleRate == 0 || iBitsPerSample == 0)
  {
    CLog::Log(LOGERROR, "Unable to create audio device, (iChannels == 0 || iSampleRate == 0 || iBitsPerSample == 0)");
    return false;
  }

  CLog::Log(LOGNOTICE, "Creating audio device with codec id: %i, channels: %i, sample rate: %i", m_codec, iChannels, iSampleRate);
  if (m_dvdAudio.Create(iChannels, iSampleRate, iBitsPerSample, bPasstrough)) // always 16 bit with ffmpeg ?
  {
    return true;
  }

  CLog::Log(LOGERROR, "Failed Creating audio device with codec id: %i, channels: %i, sample rate: %i", m_codec, iChannels, iSampleRate);
  return false;
}
