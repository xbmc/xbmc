
#include "../../stdafx.h"
#include "DVDPlayerAudio.h"
#include "DVDCodecs\DVDAudioCodec.h"
#include "DVDCodecs\DVDFactoryCodec.h"
#include "..\..\util.h"
#include "DVDClock.h"

#define EMULATE_INTTYPES
#include "ffmpeg\avcodec.h"

CDVDPlayerAudio::CDVDPlayerAudio(CDVDClock* pClock) : CThread()
{
  m_pClock = pClock;
  m_pAudioCodec = NULL;
  m_bInitializedOutputDevice = false;
  m_iSourceChannels = 0;
  
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
        
  switch (codecID)
  {
    case CODEC_ID_AC3:
    {
      iWantedChannels = 2;
      break;
    }
    //case CODEC_ID_AAC:
    case CODEC_ID_MP2:
    case CODEC_ID_PCM_S16BE:
    case CODEC_ID_PCM_S16LE:
    case CODEC_ID_MP3:
    {
      break;
    }
    case CODEC_ID_DTS:
    {
      // dts stream
      // asyncaudiostream is unable to open dts streams, use ac97 for this
      CLog::Log(LOGWARNING, "CODEC_ID_DTS is currently not supported");
      return false;
    }
    default:
    {
      CLog::Log(LOGWARNING, "Unsupported audio codec");
      return false;
    }
  }

  CLog::Log(LOGNOTICE, "Creating audio codec with codec id: %i", codecID);
  m_pAudioCodec = CDVDFactoryCodec::CreateAudioCodec(codecID);
  
  CLog::Log(LOGNOTICE, "Opening audio codec id: %i, channels: %i, sample rate: %i, bits: %i",
      codecID, iWantedChannels, iSampleRate, 16);
  if (!m_pAudioCodec->Open(codecID, iWantedChannels, iSampleRate, 16))
  {
    CLog::Log(LOGERROR, "Error opening audio codec");
    m_pAudioCodec->Dispose();
    delete m_pAudioCodec;
    m_pAudioCodec = NULL;
    return false;
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
  int n, len, data_size;

  for(;;)
  {
    /* NOTE: the audio packet can contain several frames */
    while (audio_pkt_size > 0)
    {
      EnterCriticalSection(&m_critCodecSection);
      len = m_pAudioCodec->Decode(audio_pkt_data, audio_pkt_size);
      LeaveCriticalSection(&m_critCodecSection);
      if (len < 0)
      {
        /* if error, we skip the frame */
        audio_pkt_size = 0;
        break;
      }
      
      // get decoded data and the size of it
      data_size = m_pAudioCodec->GetData(pAudioBuffer);
      
      audio_pkt_data += len;
      audio_pkt_size -= len;
      
      if (data_size <= 0) continue;

      // if no pts, then compute it
      n = 2 * m_pAudioCodec->GetChannels();
      m_audioClock += ((__int64)data_size * DVD_TIME_BASE) / (n * m_pAudioCodec->GetSampleRate());

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
    if (m_packetQueue.Get(&pPacket, 1) < 0) return -1;

    pAudioPacket = pPacket;
    audio_pkt_data = pPacket->pData;
    audio_pkt_size = pPacket->iSize;
    
    // if update the audio clock with the pts
    if (pPacket->pts != DVD_NOPTS_VALUE)
    {
      m_audioClock = ((__int64)pPacket->pts * DVD_TIME_BASE) / AV_TIME_BASE;
    }
  }
}

void CDVDPlayerAudio::OnStartup()
{
  pAudioPacket = NULL;
  audio_pkt_data = NULL;
  audio_pkt_size = 0;
}

void CDVDPlayerAudio::Process()
{
  CLog::Log(LOGNOTICE, "running thread: audio_thread");
  
  int len;
  int iAudioBufferSize = 0;
  int iAudioBufferIndex = 0;
  
  // silence data
  BYTE silence[1024];
  memset(silence, 0, 1024);
  
  BYTE* pAudioBuffer;
  
  while(!m_bStop)
  {
    if (iAudioBufferIndex >= iAudioBufferSize)
    {
      iAudioBufferSize = DecodeFrame(&pAudioBuffer); // blocks if no audio is available
      if (iAudioBufferSize < 0)
      {
        iAudioBufferSize = 1024;
        pAudioBuffer = silence;
      }
      iAudioBufferIndex = 0;
    }
    len = iAudioBufferSize - iAudioBufferIndex;

    // we have succesfully decoded an audio frame, openup the audio device if not already done
    if (!m_bInitializedOutputDevice)
    {
      m_bInitializedOutputDevice = InitializeOutputDevice();
    }
    
    // while (pDVDPlayer->m_bIsDVD && !pDVDPlayer->m_bDrawedFrame) Sleep(5); // wait for first video picture
    __int64 iCurrentAudioClock = m_audioClock - m_dvdAudio.GetDelay();
    __int64 iClockDiff = (iCurrentAudioClock - m_pClock->GetClock());
    if (iClockDiff < 0) iClockDiff = 0 - iClockDiff;
    //if (/*diff < 25000/ && */diff > -25000)
    {
      
    }
    if (iClockDiff > 5000) // sync clock if diff is bigger than 5 msec
    {
      m_pClock->Discontinuity(CLOCK_DISC_NORMAL, iCurrentAudioClock);
      CLog::DebugLog("CDVDPlayer:: Detected Audio Discontinuity, syncing clock");
      //usleep(1000);
    }
    m_dvdAudio.AddPackets(pAudioBuffer + iAudioBufferIndex, len);
    iAudioBufferIndex += len;
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

  if (iChannels == 0 || iSampleRate == 0 || iBitsPerSample == 0)
  {
    CLog::Log(LOGERROR, "Unable to create audio device, (iChannels == 0 || iSampleRate == 0 || iBitsPerSample == 0)");
    return false;
  }

  CLog::Log(LOGNOTICE, "Creating audio device with codec id: %i, channels: %i, sample rate: %i", m_codec, iChannels, iSampleRate);
  if (m_dvdAudio.Create(iChannels, iSampleRate, iBitsPerSample)) // always 16 bit with ffmpeg ?
  {
    return true;
  }

  CLog::Log(LOGERROR, "Failed Creating audio device with codec id: %i, channels: %i, sample rate: %i", m_codec, iChannels, iSampleRate);
  return false;
}
