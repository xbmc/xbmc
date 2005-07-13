
#include "../../stdafx.h"
#include "DVDPlayerAudio.h"
#include "DVDPlayer.h"
#include "DVDCodecs\DVDAudioCodec.h"
#include "DVDCodecs\DVDFactoryCodec.h"
#include "DVDCodecs\dvdaudiocodecpassthrough.h"

#include "..\..\util.h"
#include "DVDClock.h"

#define EMULATE_INTTYPES
#include "ffmpeg\avcodec.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define ROUNDED_DIV(a,b) (((a)>0 ? (a) + ((b)>>1) : (a) - ((b)>>1))/(b))
#define ABS(a) ((a) >= 0 ? (a) : (-(a)))

#define FFMAX(a,b) ((a) > (b) ? (a) : (b))
#define FFMIN(a,b) ((a) > (b) ? (b) : (a))

CDVDPlayerAudio::CDVDPlayerAudio(CDVDClock* pClock) : CThread()
{
  m_pClock = pClock;
  m_pAudioCodec = NULL;
  m_bInitializedOutputDevice = false;
  m_iSourceChannels = 0;
  m_audioClock = 0;

  InitializeCriticalSection(&m_critCodecSection);
  m_packetQueue.SetMaxSize(10 * 16 * 1024);
}

CDVDPlayerAudio::~CDVDPlayerAudio()
{
  // close the stream, and don't wait for the audio to be finished
  // CloseStream(true);
  DeleteCriticalSection(&m_critCodecSection);
}

void CDVDPlayerAudio::RegisterAudioCallback(IAudioCallback* pCallback)
{
  m_dvdAudio.RegisterAudioCallback(pCallback);
}

void CDVDPlayerAudio::UnRegisterAudioCallback()
{
  m_dvdAudio.UnRegisterAudioCallback();
}

bool CDVDPlayerAudio::OpenStream(CodecID codecID, int iChannels, int iSampleRate)
{
  // should alway's be NULL!!!!, it will probably crash anyway when deleting m_pAudioCodec here.
  if (m_pAudioCodec)
  {
    CLog::Log(LOGFATAL, "CDVDPlayerAudio::OpenStream() m_pAudioCodec != NULL");
    return false;
  }

  int iWantedChannels = iChannels;
  if (iWantedChannels == 5) iWantedChannels = 6;

  //Let codec downmix for us int the case of ac3
  if( codecID == CODEC_ID_AC3 )
    iWantedChannels = 2;


  CLog::Log(LOGNOTICE, "Opening passtrough codec for: %i", codecID);
  m_pAudioCodec = new CDVDAudioCodecPassthrough();
  if (!m_pAudioCodec->Open(codecID, iChannels, iSampleRate, 16))
  {
    CLog::Log(LOGNOTICE, "Failed to open passtrough codec for: %i", codecID);
    m_pAudioCodec->Dispose();
    delete m_pAudioCodec;
    m_pAudioCodec = NULL;

    //Okey now try a normal codec
    CLog::Log(LOGNOTICE, "Creating audio codec with codec id: %i", codecID);
    m_pAudioCodec = CDVDFactoryCodec::CreateAudioCodec(codecID);

    if( !m_pAudioCodec )
    {
      CLog::Log(LOGERROR, "Unsupported audio codec");
      return false;
    }


    CLog::Log(LOGNOTICE, "Opening audio codec id: %i, channels: %i, sample rate: %i, bits: %i",
              codecID, iWantedChannels, iSampleRate, 16);
    if (m_pAudioCodec && !m_pAudioCodec->Open(codecID, iWantedChannels, iSampleRate, 16))
    {
      CLog::Log(LOGERROR, "Error opening audio codec");
      m_pAudioCodec->Dispose();
      delete m_pAudioCodec;
      m_pAudioCodec = NULL;
      return false;
    }
  }

  m_codec = codecID;
  m_iSourceChannels = iChannels;

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
int CDVDPlayerAudio::DecodeFrame(BYTE** pAudioBuffer)
{
  CDVDDemux::DemuxPacket* pPacket = pAudioPacket;
  int n=48000*2*16/8, len, data_size;

  //Store amount left at this point, and what last pts was
  unsigned __int64 first_pkt_pts = 0;
  int first_pkt_size = 0; 
  int first_pkt_used = 0;
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

      // get decoded data and the size of it
      data_size = m_pAudioCodec->GetData(pAudioBuffer);

      audio_pkt_data += len;
      audio_pkt_size -= len;

      if (data_size <= 0) continue;
      
      // compute pts.
      n = m_pAudioCodec->GetChannels() * m_pAudioCodec->GetBitsPerSample() / 8 * m_pAudioCodec->GetSampleRate();
      m_audioClock += ((__int64)data_size * (__int64)DVD_TIME_BASE) / (__int64)n;
      return data_size;
    }
    /* free the current packet */
    if (pPacket)
    {
      CDVDDemuxUtils::FreeDemuxPacket(pPacket);
      pPacket = NULL;
      pAudioPacket = NULL;
    }

    if (m_packetQueue.RecievedAbortRequest()) return -1;
    
    // read next packet and return -1 on error
    int dvdstate=0, packstate=0;
    LeaveCriticalSection(&m_critCodecSection); //Leave here as this might stall a while
    packstate = m_packetQueue.Get(&pPacket, 1, (void**)&dvdstate);
    EnterCriticalSection(&m_critCodecSection);
    if (packstate < 0) return -1;

    pAudioPacket = pPacket;
    audio_pkt_data = pPacket->pData;
    audio_pkt_size = pPacket->iSize;

    // if update the audio clock with the pts
    if (dvdstate == DVDSTATE_RESYNC || first_pkt_pts > pPacket->pts)
    {
      //Okey first packet in this continous stream,
      //Setup clock, and then exit so we player can sync clock
      m_audioClock = pPacket->pts;
      m_pAudioCodec->Reset();
      return 0;
    }
    else if (pPacket->pts != DVD_NOPTS_VALUE)
    {
      if (first_pkt_size == 0) 
      {
        m_audioClock = pPacket->pts;
        continue;
      }

      if((unsigned __int64)m_audioClock < first_pkt_pts || (unsigned __int64)m_audioClock > pPacket->pts)
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
  pAudioPacket = NULL;
  m_audioClock = 0;
  audio_pkt_data = NULL;
  audio_pkt_size = 0;
}

void CDVDPlayerAudio::Process()
{
  CLog::Log(LOGNOTICE, "running thread: audio_thread");

  int len;

  // silence data
  BYTE silence[1024];
  memset(silence, 0, 1024);

  BYTE* pAudioBuffer;
  __int64 iClockDiff=0;
  int iDiffCount=0;;
  while (!m_bStop)
  {
    //Don't let anybody mess with our global variables
    EnterCriticalSection(&m_critCodecSection);
    len = DecodeFrame(&pAudioBuffer); // blocks if no audio is available, but leaves critical section before doing so
    LeaveCriticalSection(&m_critCodecSection);

    if (len < 0)
    {
      len = 1024;
      pAudioBuffer = silence;
    }

    // we have succesfully decoded an audio frame, openup the audio device if not already done
    if (!m_bInitializedOutputDevice)
    {
      m_bInitializedOutputDevice = InitializeOutputDevice();
    }

    //Add any packets decode
    if( len > 0 ) m_dvdAudio.AddPackets(pAudioBuffer, len);

    //Clock should be calculated after packets have been addet
    const __int64 iCurrDiff = (m_audioClock - m_dvdAudio.GetDelay()) - m_pClock->GetClock();
    const __int64 iAvDiff = (iClockDiff + iCurrDiff)/2;

    //Check for discontinuity in the stream, use an average of last two diffs to 
    //eliminate highfreq fluctuations of large packet sizes
    if (ABS(iAvDiff) > 5000 || len == 0) // sync clock if average diff is bigger than 5 msec 
    {
      //len == 0 is an expected discontinuity
      //Wait untill only one packet is left in buffer to minimize choppyness in video
      m_dvdAudio.WaitForBuffer(1);
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
  // destroy audio device
  CLog::Log(LOGNOTICE, "Closing audio device");
  m_dvdAudio.Destroy();
  m_bInitializedOutputDevice = false;

  CLog::Log(LOGNOTICE, "thread end: audio_thread");
}

void CDVDPlayerAudio::Pause()
{
  m_dvdAudio.Pause();
}

void CDVDPlayerAudio::Resume()
{
  m_dvdAudio.Resume();
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

void CDVDPlayerAudio::DoWork()
{
  m_dvdAudio.DoWork();
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

void CDVDPlayerAudio::SetVolume(long nVolume)
{
  m_dvdAudio.SetVolume(nVolume);
}
