
#include "../../stdafx.h"
#include "paplayer.h"
#include "../../application.h"
//#include "../lib/libcdio/util.h"

#define VOLUME_FFWD_MUTE 900 // 9dB

#define DECODER_DELAY 529 // decoder delay in samples

// PAP: Psycho-acoustic Audio Player
// Supporting all open  audio codec standards.
// First one being nullsoft's nsv audio decoder format

static IAudioDecoder* (__cdecl* CreateAudioDecoder)(unsigned int type, IAudioOutput **output)=NULL;

static IAudioCallback* m_pCallback = NULL;

PAPlayer::PAPlayer(IPlayerCallback& callback) : IPlayer(callback)
{
  
  m_pDll = NULL;
  m_pPAP = NULL;
  m_bIsPlaying = false;
  m_bPaused = false;
  m_dwAudioBufferMin = 0;
  m_pAudioDevice = NULL;
  m_pInputBuffer = NULL;
  m_dwAudioBufferSize = 0;
  m_dwAudioBufferPos = 0;
  m_iSpeed = 1;
  m_iLastSpeed = 1;
  m_Channels = 0;
  m_SampleRate = 0;
  m_SampleSize = 0;
  m_InputBytesWanted = 65535;  // Change when reading from http  
  m_dwInputBufferSize = 0;
  m_Decoding = false;
  m_PcmSize = 0;  
  m_PcmPos = 0;
  m_pInputBuffer = new BYTE[m_InputBytesWanted];  
  m_dwAudioMaxSize = (16*44100); // 4 seconds for now
  m_pPcm = new BYTE[m_dwAudioMaxSize];
  m_BufferingPcm = true;
  m_AverageInputBytesPerSecond = 20000; // 160k , good place to start i guess
  // Initialize our buffer and output device later
  m_dwBytesReadIn = 0;
  m_dwBytesSentOut = 0;
  m_SeekTime=-1;
  m_cantSeek = 0;
  m_eof = false;
  m_startOffset = 0;
  m_bGuessByterate = false;
  m_lastByteOffset = 0;
  m_bDllLoaded = false;
}

PAPlayer::~PAPlayer()
{
  CloseFile();

  // kill our buffers etc.
  KillBuffer();
  if (m_pPAP )
    delete m_pPAP;
  m_pPAP = NULL;
  if (m_pDll)
    delete m_pDll;
  m_pDll = NULL;

  if ( m_pInputBuffer )
    delete m_pInputBuffer;
  m_pInputBuffer = NULL;
  if ( m_pPcm )
    delete m_pPcm;
  m_pPcm = NULL;
}

void PAPlayer::OnExit()
{

}

bool PAPlayer::LoadDLL()
{
  if (m_bDllLoaded)
    return true;
  CStdString strDll = "Q:\\system\\players\\paplayer\\in_mp3.dll"; 
  m_pDll = new DllLoader(strDll.c_str(), true);
  if (!m_pDll)
  {
    CLog::Log(LOGERROR, "PAP Player: Unable to load dll %s", strDll.c_str());
    return false;
  }
  if (!m_pDll->Parse())
  {
    // failed,
    CLog::Log(LOGERROR, "PAP Player: Unable to load dll %s", strDll.c_str());
    delete m_pDll;
    m_pDll = NULL;
    return false;
  }
  m_pDll->ResolveImports();

  // get handle to the functions in the dll
  m_pDll->ResolveExport("CreateAudioDecoder", (void**)&CreateAudioDecoder);
  
  if (!CreateAudioDecoder )
  {
    CLog::Log(LOGERROR, "PAP Player: Unable to find PAP data in dll %s", strDll.c_str());
    delete m_pDll;
    m_pDll = NULL;
    return false;
  }
  m_bDllLoaded = true;
  return true;
}

bool PAPlayer::OpenFile(const CFileItem& file, __int64 iStartTime)
{
  CloseFile();

  if (!m_eof)
  { // we started a new file whilst still playing the old one - let's free up our buffers and so on.
    KillBuffer();
    m_PcmPos = 0;
    m_PcmSize = 0;
    m_dwInputBufferSize = 0;
  }

  m_bPaused = false;
  m_bStopPlaying = false;
  m_eof = false;
  m_lastByteOffset = 0;
  m_dwBytesSentOut = 0;
  m_startOffset = 0;
  m_IgnoreFirst = true; // we want to be gapless
  m_pIgnoredFirstBytes = NULL;
  m_IgnoreLast = true;
  m_BufferingPcm = true;  // buffer as much info as we can to begin with

  if (!m_filePAP.Open(file.m_strPath))
    return false;

  LoadDLL();

  if ( !m_pDll ) return false;

  // TODO:  add file extension checking and HTTP/Icecast/Shoutcast reading
  if (m_pPAP)
  {
    delete m_pPAP;
    m_pPAP = NULL;
  }
  m_pPAP = CreateAudioDecoder(' 3PM',NULL);

  if ( m_pPAP )  CLog::Log(LOGINFO, "PAP Player: Loaded decoder at %p", m_pPAP);
  else  return false;
  
  CLog::Log(LOGINFO, "PAP Player: Playing %s", file.m_strPath.c_str());


  float TrackDuration = 0.0f; //file.m_musicInfoTag.GetDuration();
  __int64 length = m_filePAP.GetLength();

  // Guess Bitrate
  CMusicInfoTagLoaderMP3 mp3info;
  CMusicInfoTag tag;
  mp3info.Load(file.m_strPath.c_str(), tag);
  mp3info.GetSeekInfo(m_seekInfo);
  TrackDuration = m_seekInfo.GetDuration();
  if ( TrackDuration )
    m_AverageInputBytesPerSecond = (DWORD)(length / TrackDuration);
  else
    m_bGuessByterate = true;  // If we don't have the TrackDuration we'll have to guess later.

  if ( TrackDuration && iStartTime > 0 )
  {
    // calculate our seek offset accurately using the seekInfo from the file
    float fseektime = (float)m_seekInfo.GetByteOffset(iStartTime / 1000.0f);

    if ( (__int64) fseektime < length ) 
    {
      m_filePAP.Seek( (__int64)(fseektime) , SEEK_SET);
      m_lastByteOffset = m_filePAP.GetPosition();
      m_startOffset = iStartTime;
      CLog::Log(LOGINFO, "PAP Player: Starting at %d",(__int64) fseektime);
    }
    else
    {
      CLog::Log(LOGINFO, "PAP Player: Start Time is greater than file length");
      return false;
    }
  }


  if (ThreadHandle() == NULL)
    Create();

  m_startEvent.Set();

  m_bIsPlaying = true;

  return true;
}

bool PAPlayer::CloseFile()
{
  if (IsPaused())
    Pause();

  m_bStopPlaying = true;
  m_bStop = true;

  StopThread();

  m_filePAP.Close();

  return true;
}


void PAPlayer::Pause()
{
  if (!m_bIsPlaying || !m_pAudioDevice) return ;

  m_bPaused = !m_bPaused;

  if (m_bPaused)
  {
    m_pAudioDevice->Pause();
    
    CLog::Log(LOGINFO, "PAP Player: Playback paused");
  }
  else
  {
    m_pAudioDevice->Resume();
    CLog::Log(LOGINFO, "PAP Player: Playback resumed");
  }
}

void PAPlayer::SetVolume(long nVolume)
{
  if (m_pAudioDevice)
    m_pAudioDevice->SetCurrentVolume(nVolume);
}

void PAPlayer::Process()
{
  if (m_startEvent.WaitMSec(100))
  {
    m_startEvent.Reset();
    do
    {
      if (!m_bPaused)
      {
        if (!ProcessPAP())
          break;
      }
      else
      {
        Sleep(100);
      }
    }
    while (!m_bStopPlaying && m_bIsPlaying && !m_bStop);

    m_filePAP.Close();

    m_bIsPlaying = false;
    if (!m_bStopPlaying && !m_bStop)
    {
      m_callback.OnPlayBackEnded();
    }
  }
}

void PAPlayer::RegisterAudioCallback(IAudioCallback* pCallback)
{
  m_pCallback = pCallback;
  if (m_pAudioDevice)
    m_pAudioDevice->RegisterAudioCallback( m_pCallback);
}

void PAPlayer::UnRegisterAudioCallback()
{
  m_pCallback = NULL;
  if (m_pAudioDevice)
    m_pAudioDevice->UnRegisterAudioCallback();
}

void PAPlayer::OnInitialize(int iChannels, int iSamplesPerSec, int iBitsPerSample)
{
  if (!m_pCallback) return ;
  m_pCallback->OnInitialize(iChannels, iSamplesPerSec, iBitsPerSample);
}

void PAPlayer::OnAudioData(const unsigned char* pAudioData, int iAudioDataLength)
{
  if (!m_pCallback) return ;
  m_pCallback->OnAudioData(pAudioData, iAudioDataLength);
}

void PAPlayer::ToFFRW(int iSpeed)
{
  /*int iRate = 48000;
  if ( iSpeed != 1 )
  {
    iRate += ( iSpeed * 500 );
  } // When using the AsyncDirectSound, you can SetFrequency in Async's SetPlaySpeed() to iRate here to get pitch/tempo control
      // Async would just need to take iRate as the Freq and do no other math
  if ( m_pAudioDevice) m_pAudioDevice->SetPlaySpeed( iSpeed );*/
  m_iSpeed = iSpeed;
}

bool PAPlayer::ProcessPAP()
{
 
  while (true && !m_bStop) // Just go once... see if that helps the system more.. vs sticking around so much
  {
          
    if (m_pAudioDevice) m_pAudioDevice->DoWork();

    if ( (m_PcmSize < m_dwAudioBufferMin && !m_CallPAPAgain && m_pAudioDevice && m_eof &&  !m_dwInputBufferSize) || m_bStop )
    {
      // Looks like we're done, get out.. 
      // anything left thats under min buffersize will be lost...
      // Let directsound run itself empty before we call it quit
      // TODO I THINK...
      return false;
    }
    if ( m_PcmSize  && m_pAudioDevice )
    {
      // We have data to move into the output audio buffer

      // Check if we have packets free in the buffer
      if ( m_PcmSize >=  m_dwAudioBufferMin && !m_BufferingPcm)
      {
        DWORD dwSize = m_PcmSize;

        DWORD dwActual = m_pAudioDevice->AddPackets(m_pPcm + m_PcmPos, dwSize);
        if (dwActual)
        { // they've taken a packet of size dwSize to the audio buffers
          m_dwBytesSentOut += dwActual;
          m_PcmPos += dwActual;
          m_PcmSize -= dwActual;
          if ( m_PcmSize == 0 ) 
            m_PcmPos = 0; // empty pcm buffer, go get some more quick
//          else
//            break; // Exit out to see if we need to stop, and so if we can keep the soundcard busy
        }
        else
          Sleep(10);
      }
      else
      {
        if (m_PcmPos) 
        {
          memmove(m_pPcm, m_pPcm+m_PcmPos,m_PcmSize); // Copy down remainder, always less then min buffer size
          m_PcmPos = 0;
        }
      }
    }    


    // Get some more data, read 8192 if we can
    int m_InputBufferToRead = m_InputBytesWanted - m_dwInputBufferSize;
    if (m_InputBufferToRead && !m_CallPAPAgain && m_filePAP.GetPosition() != m_filePAP.GetLength() && !m_eof ) 
    {
      int fileLeft=(int)(m_filePAP.GetLength() - m_filePAP.GetPosition());
      if ( m_InputBufferToRead >  fileLeft ) m_InputBufferToRead = fileLeft;

      DWORD dwBytesRead = m_filePAP.Read(m_pInputBuffer+m_dwInputBufferSize , m_InputBufferToRead);
      if (!dwBytesRead)
        return false; 
      // add the size of read PAP data to the buffer size
      m_dwInputBufferSize = dwBytesRead;
      if ( m_filePAP.GetLength() == m_filePAP.GetPosition() )
        m_eof = true;
    }
    if (m_dwInputBufferSize || m_CallPAPAgain || m_PcmSize )
    {
      int result;
        
      m_Decoding = true;

      int sendsize=(m_dwAudioMaxSize-m_PcmSize)-m_PcmPos;
      if ( sendsize )
      {
        unsigned int formatdata[8];
        
        m_CallPAPAgain = false;
        
        result = m_pPAP->decode( m_pInputBuffer,m_dwInputBufferSize,m_pPcm+m_PcmPos+m_PcmSize,&sendsize, (unsigned int *)&formatdata);
        if ( result == 1 || result == 0) 
        {
          if (sendsize && m_PcmSize <= 1152*4*20)
          {
//            MemDump(m_pPcm + m_PcmPos + m_PcmSize, sendsize);
          }
          m_PcmSize += sendsize;
          if ( m_BufferingPcm ) 
          {
            if ( m_PcmSize >  m_dwAudioMaxSize / 2 )
              m_BufferingPcm = false;
          }
          if ( m_IgnoreFirst && sendsize )
          {
            // starting up - lets ignore the first (typically 576) samples
            // this code assumes that sendsize is at least as big as the number of ignored bytes.
            if (!m_pIgnoredFirstBytes)
              m_pIgnoredFirstBytes = m_pPcm + m_PcmPos + m_PcmSize - sendsize;
            int iDelay = DECODER_DELAY + m_seekInfo.GetFirstSample();  // decoder delay + encoder delay
            iDelay *= formatdata[2] * formatdata[3] / 8;            // sample size
            if (m_pPcm + m_PcmPos + m_PcmSize - m_pIgnoredFirstBytes >= iDelay)
            { // have enough data to ignore up to our first sample
              // dump samples as we have them...
              int iAmountToMove = m_pPcm + m_PcmPos + m_PcmSize - m_pIgnoredFirstBytes - iDelay;
              memmove(m_pIgnoredFirstBytes, m_pIgnoredFirstBytes + iDelay, iAmountToMove);
              m_PcmSize -= iDelay;
              m_IgnoreFirst = false;
            }
          }
          if ( result )
            m_CallPAPAgain = true; // Codec wants us to call it again with the same buffer
          else 
          {
            m_dwBytesReadIn+=m_dwInputBufferSize;
            m_dwInputBufferSize = 0; // Read more from the file
            if (m_eof && m_IgnoreLast)
            {
              // lets remove the last sample info from our buffer
              unsigned int samplestoremove = (m_seekInfo.GetLastSample() - DECODER_DELAY);
              samplestoremove *= m_Channels * m_SampleSize / 8;
              if (samplestoremove > m_PcmSize) samplestoremove = m_PcmSize;
              m_PcmSize -= samplestoremove;
              m_IgnoreLast = false;
            }
          }

          if ( sendsize &&  formatdata[4] && m_bGuessByterate ) m_AverageInputBytesPerSecond = formatdata[4] / 8;
          if ( sendsize && !m_BufferingPcm && (!m_pAudioDevice || m_Channels != formatdata[2] || m_SampleRate != formatdata[1] || m_SampleSize != formatdata[3]))
          {
            m_Channels    = formatdata[2];
            m_SampleRate  = formatdata[1];
            m_SampleSize  = formatdata[3];
            
            CLog::Log(LOGINFO, "PAP Player: Channels %d : Sample Rate %d",m_Channels,m_SampleRate);
            // Create the audio device
            CreateBuffer();
            m_callback.OnPlayBackStarted();
            break;
          }
        }
        else if ( result == -1 ) 
        {
          // error decoding, lets finish up and get out
          CLog::Log(LOGINFO, "PAP Player: Error while decoding");
          m_Decoding = false;
        }
      } 
    }
    if ( m_pAudioDevice ) // If we're playing we can seek, if not, dont bother
    {
      // Current Track ended?
      // And theres nothing left to decode
      if (m_eof && !m_Decoding )
      {
        CLog::Log(LOGINFO, "PAP Player: End of Track reached");
        return false;
      }
      if (m_SeekTime != -1)
      {
        // calculate our offset
        __int64 iOffset = (__int64)m_seekInfo.GetByteOffset(0.001f * m_SeekTime);
        m_filePAP.Seek(iOffset, SEEK_SET);
        m_startOffset = m_SeekTime;
        m_dwBytesSentOut = 0;
        m_dwInputBufferSize = 0;
        m_pAudioDevice->Stop();
        m_CallPAPAgain = false;
        m_PcmSize = 0;
        m_PcmPos = 0;
        m_SeekTime = -1;
        break;
      }
      if ( m_cantSeek ) 
      {
        m_cantSeek--;
        if ( m_iSpeed == 1 )
        {
          m_pAudioDevice->Stop();
          SetVolume(g_stSettings.m_nVolumeLevel);
        }
      }
      int inewSpeed = 8;
      int snippet; 
      if ( inewSpeed != 0 )
        snippet = (m_Channels*(m_SampleSize/8)*m_SampleRate)/abs(inewSpeed);  
      if (m_iSpeed != 1 && !m_cantSeek && m_dwBytesSentOut >= snippet && snippet ) 
      {
        // Calculate offset to seek if we do FF/RW
        int iOffset = (int)(m_AverageInputBytesPerSecond*((m_iSpeed - 1.0f) / inewSpeed));

        __int64  timeconstant = m_Channels*(m_SampleSize/8)*m_SampleRate;
        float timeout = (float) m_dwBytesSentOut / (float)timeconstant  ;
        
        m_lastByteOffset +=(__int64) ( (float) m_AverageInputBytesPerSecond * timeout );
    
        //m_pAudioDevice->Stop();
        m_pPAP->flush(); // Flush the decoder!
        m_dwInputBufferSize = 0;
        m_CallPAPAgain = false;
        m_PcmSize = 0;
        m_PcmPos = 0;
        m_dwBytesSentOut = 0;
        // Is our offset inside the track range?
        if (m_lastByteOffset + iOffset >= 0 && m_lastByteOffset + iOffset <= m_filePAP.GetLength())
        { // just set next position to read
          m_filePAP.Seek(m_lastByteOffset + iOffset, SEEK_SET);
          m_cantSeek = 1;  
          m_lastByteOffset = m_filePAP.GetPosition();
          m_startOffset = m_seekInfo.GetTimeOffset(m_lastByteOffset);
          m_dwBytesSentOut = 0;
          SetVolume(g_stSettings.m_nVolumeLevel - VOLUME_FFWD_MUTE); // override xbmc mute 
         } // Is our next position smaller then the start...
        else if (m_lastByteOffset + iOffset < 0)
        { // ...disable seeking and start the track again
          m_iSpeed = 1;
          m_filePAP.Seek(0, SEEK_SET);
          m_startOffset = 0;
          m_dwBytesSentOut = 0;
          m_lastByteOffset = 0;
          m_BufferingPcm = true;  // Rebuffer to avoid stutter
          SetVolume(g_stSettings.m_nVolumeLevel); // override xbmc mute 
          CLog::Log(LOGINFO, "PAP Player: Start of track reached while seeking");
        } // is our next position greater then the end sector...
        else if (m_lastByteOffset + iOffset > m_filePAP.GetLength())
        { // ...disable seeking and quit player
          m_iSpeed = 1;
          // HACK: restore volume level to the value before seeking was started
          // else next track will be muted
          SetVolume(g_stSettings.m_nVolumeLevel);
          CLog::Log(LOGINFO, "PAP Player: End of track reached while seeking");
          return false;
        }
       
        break;
        
      }
    }
  }
  return true;
}

int PAPlayer::CreateBuffer()
{
  // Check we don't first need to kill our buffers
  KillBuffer();
  m_pAudioDevice = new CASyncDirectSound(m_pCallback, m_Channels, m_SampleRate, m_SampleSize,false, 128); // use 64k of buffers
  m_dwAudioBufferMin = m_pAudioDevice->GetChunkLen();
  CLog::Log(LOGINFO, "PAP Player: Chunklen %d",m_dwAudioBufferMin);
  return 0;
}

void PAPlayer::KillBuffer()
{
  if (m_pAudioDevice)
  {
    delete m_pAudioDevice;
    m_pAudioDevice = NULL;
  }

}

__int64 PAPlayer::GetTime()
{

  __int64  timeconstant = m_Channels*(m_SampleSize/8)*m_SampleRate;
  __int64  timeplus = timeconstant ? (__int64)(((float) m_dwBytesSentOut / (float)timeconstant ) * 1000.0) : 0;
  return  m_startOffset + timeplus;
}

int PAPlayer::GetTotalTime()
{
  return (int)(m_filePAP.GetLength() / m_AverageInputBytesPerSecond);
}

void PAPlayer::SeekTime(__int64 iTime /*=0*/)
{
  m_SeekTime = iTime;
}

// Application.cpp
//else if (url.GetFileType() == "mp3")
//{
//    if (CUtil::FileExists("Q:\\system\\players\\paplayer\\in_mp3.dll"))  strNewPlayer = "paplayer";
//  }

// PlayerCoreFactory.cpp
 //if (strCoreLower == "paplayer" ) return new PAPlayer(callback);
