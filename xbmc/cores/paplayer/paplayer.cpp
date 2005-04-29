#include "../../stdafx.h"
#include "paplayer.h"
#include "../../application.h"
#include "../../util.h"
#include "MP3Codec.h"
#include "APECodec.h"

#define VOLUME_FFWD_MUTE 900 // 9dB

#define INTERNAL_BUFFER_LENGTH  2*4*44100     // 2 seconds
#define OUTPUT_BUFFER_LENGTH    48000*4/768   // 1 second

// PAP: Psycho-acoustic Audio Player
// Supporting all open  audio codec standards.
// First one being nullsoft's nsv audio decoder format

static IAudioCallback* m_pCallback = NULL;

PAPlayer::PAPlayer(IPlayerCallback& callback) : IPlayer(callback)
{
  m_bIsPlaying = false;
  m_bPaused = false;
  m_dwAudioBufferMin = 0;
  m_pAudioDevice = NULL;
  m_dwAudioBufferSize = 0;
  m_iSpeed = 1;
  m_PcmSize = 0;  
  m_PcmPos = 0;
  m_dwAudioMaxSize = INTERNAL_BUFFER_LENGTH;
  m_pPcm = new BYTE[m_dwAudioMaxSize];
  m_BufferingPcm = true;
  m_dwBytesSentOut = 0;
  m_SeekTime=-1;
  m_IsFFwdRewding = false;
  m_eof = false;
  m_startOffset = 0;
  m_codec = NULL;
  m_BytesPerSecond = 0;
}

PAPlayer::~PAPlayer()
{
  CloseFile();

  KillAudioDevice();

  if ( m_pPcm )
    delete m_pPcm;
  m_pPcm = NULL;

  if ( m_codec )
    delete m_codec;
  m_codec = NULL;
}

void PAPlayer::OnExit()
{

}

bool PAPlayer::OpenFile(const CFileItem& file, __int64 iStartTime)
{
  CloseFile();

  if (!m_eof)
  { // we started a new file whilst still playing the old one - let's free up our buffers and so on.
    CLog::Log(LOGDEBUG, "PAPlayer: Starting next track whilst already playing - killing our output buffers");
    KillAudioDevice();
    if (m_codec)
      delete m_codec;
    m_codec = NULL;
    m_PcmPos = 0;
    m_PcmSize = 0;
  }

  m_iSpeed = 1;
  m_bPaused = false;
  m_bStopPlaying = false;
  m_eof = false;
  m_dwBytesSentOut = 0;
  m_startOffset = 0;
  m_BufferingPcm = true;  // buffer as much info as we can to begin with

  // determine which type of codec we should use.
  CURL url(file.m_strPath);
  if (m_codec && !m_codec->HandlesType(url.GetFileType().c_str()))
  { // we're opening a different type of file - close the current decoder
    delete m_codec;
    m_codec = NULL;
  }
  if (!m_codec)
  {
    if (url.GetFileType().Equals("mp3"))
      m_codec = new MP3Codec;
    else if (url.GetFileType().Equals("ape") || url.GetFileType().Equals("mac"))
      m_codec = new APECodec;
  }

  if (!m_codec || !m_codec->Init(file.m_strPath))
  {
    CLog::Log(LOGERROR, "PAP Player: Unable to Init Codec!");
    return false;
  }
  
  CLog::Log(LOGINFO, "PAP Player: Playing %s", file.m_strPath.c_str());

  if (!m_pAudioDevice)
    CreateAudioDevice();

  // Seek to appropriate start position
  if ( iStartTime > 0 && iStartTime < m_codec->m_TotalTime )
    m_startOffset = m_codec->Seek(iStartTime);

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

  // Close our decoder
  if (m_codec)
  {
    delete m_codec;
    m_codec = NULL;
  }
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
  CLog::Log(LOGDEBUG, "PAPlayer: Thread started");
  if (m_startEvent.WaitMSec(100))
  {
    m_startEvent.Reset();

    m_callback.OnPlayBackStarted();

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

    m_codec->DeInit();

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
  m_iSpeed = iSpeed;
}

bool PAPlayer::ProcessPAP()
{
  /*
   * Here's what we should be doing in each player loop:
   *
   * 1.  Run DoWork() on our audio device to actually output audio.
   *
   * 2.  Pass our current buffer to the audio device to see if it wants anything,
   *     and if so, reduce our buffer size accordingly.
   *
   * 3.  Check whether we have space in our buffer for more data, and if so,
   *     read some more in.
   *
   * 4.  Check for end of file and return false if we reach it.
   *
   * 5.  Perform any seeking and ffwd/rewding as necessary.
   *
   * 6.  If we don't do anything in 2...5, we can take a breather and break out for sleeping.
   */
  if (!m_pAudioDevice)
    return false;

  while (true && !m_bStop) // Just go once... see if that helps the system more.. vs sticking around so much
  {
    m_pAudioDevice->DoWork();

    // Check to see if we are finished decoding
    if ( (m_PcmSize < m_dwAudioBufferMin && m_eof) || m_bStop )
    {
      // Looks like we're done, get out.. 
      // anything left thats under min buffersize will be lost, but this
      // is normally very insubstantial (currently in the order of 200 samples)
      return false;
    }
    // Current Track ended?
    if ( m_eof )
    {
      // This is the point where we could request the next track from our GUI.
      // We don't at this stage as our system is very much a push-based one - the GUI
      // pushes us the tracks and we just play 'em.  Once we have everything incorporated
      // into the output stream system, we'd do it either here (or earlier) and allow 2
      // streams to exist for crossfade and the like.
    }
    // Move PCM information into our output device
    DWORD dwActual = 0;
    if ( !m_BufferingPcm && m_PcmSize )
      dwActual = m_pAudioDevice->AddPackets(m_pPcm + m_PcmPos, m_PcmSize);
    if (dwActual)
    { // they've taken a packet of size dwSize to the audio buffers
//          CLog::Log(LOGERROR, "Taken %i bytes of data", dwActual);
      m_dwBytesSentOut += dwActual;
      m_PcmPos += dwActual;
      m_PcmSize -= dwActual;
      if ( m_PcmSize == 0 ) 
        m_PcmPos = 0; // empty pcm buffer, go get some more quick
      // copy down any remaining audio if we're more than half way through the buffer
      if (m_PcmPos > m_dwAudioMaxSize / 2) 
      {
        memmove(m_pPcm, m_pPcm+m_PcmPos,m_PcmSize);
        m_PcmPos = 0;
      }
    }
    if (m_SeekTime != -1)
    {
      // Do any seeking as necessary.  We reset the audio buffers at this point as
      // they typically hold around 4 seconds of audio in advance that needs to be
      // cleared out.
      m_startOffset = m_codec->Seek(m_SeekTime);
      m_dwBytesSentOut = 0;
      m_pAudioDevice->ResetBytesInBuffer();
      m_PcmSize = 0;
      m_PcmPos = 0;
      m_SeekTime = -1;
      m_pAudioDevice->Stop();
      break;
    }
    if (m_IsFFwdRewding || m_iSpeed != 1)
    {
      // Do ffwd and rewind.
      // The technique used is to play a snippet of audio (a fraction of a second)
      // and then skip forward a suitable amount so that we effectively move through
      // the audio at the speed specified.
      // Audio is muted by a set amount, and levels are reset once we are back at
      // normal playback speed.
      if ( m_IsFFwdRewding && m_iSpeed == 1 ) 
      { // reset ffwd/rewind
        m_IsFFwdRewding = false;
        m_pAudioDevice->Stop();
        SetVolume(g_stSettings.m_nVolumeLevel);
        break;
      }
      // we're definitely fastforwarding or rewinding
      int snippet = m_BytesPerSecond / 4;
      if ( m_dwBytesSentOut >= snippet ) 
      {
        // Calculate offset to seek if we do FF/RW
        __int64 time = GetTime();
        if (m_IsFFwdRewding) snippet = (int)m_dwBytesSentOut;
        time += (__int64)((double)snippet * (m_iSpeed - 1.0) / m_BytesPerSecond * 1000.0);
        m_PcmSize = 0;
        m_PcmPos = 0;
        // Is our offset inside the track range?
        if (time >= 0 && time <= m_codec->m_TotalTime)
        { // just set next position to read
          m_IsFFwdRewding = true;  
          m_startOffset = m_codec->Seek(time);
          m_dwBytesSentOut = 0;
          m_pAudioDevice->ResetBytesInBuffer();
          SetVolume(g_stSettings.m_nVolumeLevel - VOLUME_FFWD_MUTE); // override xbmc mute 
        }
        else if (time < 0)
        { // ...disable seeking and start the track again
          m_startOffset = m_codec->Seek(0);
          m_dwBytesSentOut = 0;
          m_pAudioDevice->ResetBytesInBuffer();
          m_iSpeed = 1;
          m_BufferingPcm = true;  // Rebuffer to avoid stutter
          SetVolume(g_stSettings.m_nVolumeLevel); // override xbmc mute 
          CLog::Log(LOGINFO, "PAP Player: Start of track reached while seeking");
        } // is our next position greater then the end sector...
        else //if (time > m_codec->m_TotalTime)
        { // ...disable seeking and quit player
          m_iSpeed = 1;
          // HACK: restore volume level to the value before seeking was started
          // else next track will be muted
          m_eof = true;
          SetVolume(g_stSettings.m_nVolumeLevel);
          CLog::Log(LOGINFO, "PAP Player: End of track reached while seeking");
          return false;
        }
      }
    }
    // Read in more data
    int result = -1;
    int sendsize = m_BytesPerSecond / 2;  // 1/2 a second.
    if ( m_PcmPos + m_PcmSize + sendsize > m_dwAudioMaxSize ) 
      sendsize = m_dwAudioMaxSize - m_PcmSize - m_PcmPos;
    if ( sendsize )
    {
      int actualdatasent = 0;
//       CLog::Log(LOGERROR, "Getting %i more data, TIME = %i", sendsize * m_nBytesPerBlock, timeGetTime());
      int result = m_codec->ReadPCM(m_pPcm + m_PcmPos + m_PcmSize, sendsize, &actualdatasent);
      if ( result != READ_ERROR && actualdatasent ) 
      {
        // Apply replaygain.
        if (g_guiSettings.m_replayGain.iType)
          ApplyReplayGain(m_pPcm+m_PcmPos+m_PcmSize, actualdatasent);

        m_PcmSize += actualdatasent;
        if ( m_BufferingPcm ) 
        {
          if ( m_PcmSize >  m_dwAudioMaxSize / 2 )
            m_BufferingPcm = false;
        }
        if (result == READ_EOF)
        { // EOF reached
          m_eof = true;
        }
      }
      else if (result == READ_ERROR)
      {
        // error decoding, lets finish up and get out
        CLog::Log(LOGINFO, "PAP Player: Error while decoding %i", result);
        return false;
      }
      else if (result == READ_EOF)
        m_eof = true;
      else
      { // nothing to do - let's sleep and take a break
        Sleep(10);
        break;
      }
    }
  }
  return true;
}

int PAPlayer::CreateAudioDevice()
{
  // TODO: This routine should query our audio stream for it's current parameters and
  // reload it completely if they need changing.  This will currently cause problems
  // when changing from stereo -> mono and/or 16bit -> 8bit and vice-versa.
  // Check we don't first need to kill our buffers
  KillAudioDevice();
  m_BytesPerSecond = m_codec->m_Channels * m_codec->m_SampleRate * m_codec->m_BitsPerSample / 8;
  m_pAudioDevice = new CASyncDirectSound(m_pCallback, m_codec->m_Channels, m_codec->m_SampleRate, m_codec->m_BitsPerSample, false, OUTPUT_BUFFER_LENGTH); // use 64k of buffers
  m_dwAudioBufferMin = m_pAudioDevice->GetChunkLen();
  CLog::Log(LOGINFO, "PAP Player: New AudioDevice created. Chunklen %d",m_dwAudioBufferMin);
  return 0;
}

void PAPlayer::KillAudioDevice()
{
  if (m_pAudioDevice)
  {
    delete m_pAudioDevice;
    m_pAudioDevice = NULL;
  }

}

__int64 PAPlayer::GetTime()
{
  __int64  timeplus = 0;
  if (m_pAudioDevice)
    timeplus = m_BytesPerSecond ? (__int64)(((float)(m_dwBytesSentOut - m_pAudioDevice->GetBytesInBuffer())) / (float)m_BytesPerSecond * 1000.0) : 0;
  else
    timeplus = m_BytesPerSecond ? (__int64)(((float) m_dwBytesSentOut / (float)m_BytesPerSecond ) * 1000.0) : 0;
  return  m_startOffset + timeplus;
}

int PAPlayer::GetTotalTime()
{
  return (int)(m_codec->m_TotalTime / 1000);
}

void PAPlayer::SeekTime(__int64 iTime /*=0*/)
{
  m_SeekTime = iTime;
}

void PAPlayer::ApplyReplayGain(void *pData, int size)
{
  // Ideally, we'd keep everything in floats until we
  // get to the output stage.  Unfortunately, AddPackets() doesn't support this
  // yet.  It'd be reasonably easy to add, however.
  // This will currently fail rather badly on non-16bit audio.
#define REPLAY_GAIN_DEFAULT_LEVEL 89.0f
#define MAX_SHORT_VALUE 32767
#define MIN_SHORT_VALUE -32768  // this hardclipping is necessary due to the above.
  // Compute amount of gain
  float replaydB, peak;
  if (m_codec->m_replayGain.iHasGainInfo & REPLAY_GAIN_HAS_TRACK_INFO)
  {
    replaydB = (float)g_guiSettings.m_replayGain.iPreAmp + (float)m_codec->m_replayGain.iTrackGain / 100.0f;
    peak = m_codec->m_replayGain.fTrackPeak;
  }
  else if (m_codec->m_replayGain.iHasGainInfo & REPLAY_GAIN_HAS_ALBUM_INFO)
  {
    replaydB = (float)g_guiSettings.m_replayGain.iPreAmp + (float)m_codec->m_replayGain.iAlbumGain / 100.0f;
    peak = m_codec->m_replayGain.fAlbumPeak;
  }
  else
  {
    replaydB = (float)g_guiSettings.m_replayGain.iNoGainPreAmp;
    peak = 0.0f;
  }
  // convert to a gain type
  float replaygain = pow(10.0f, (replaydB - REPLAY_GAIN_DEFAULT_LEVEL)* 0.05f);
  // check peaks
  if (g_guiSettings.m_replayGain.bAvoidClipping)
  {
    if (fabs(peak * replaygain) > 1.0f)
      replaygain = 1.0f / fabs(peak);
  }
  // apply gain
  short *pShortData = (short *)pData;
  for (int i = size; i; i-=2)
  {
    float result = *pShortData * replaygain;
    if (result > MAX_SHORT_VALUE) result = MAX_SHORT_VALUE;
    if (result < MIN_SHORT_VALUE) result = MIN_SHORT_VALUE;
    *pShortData++ = (short)result;
  }
}
