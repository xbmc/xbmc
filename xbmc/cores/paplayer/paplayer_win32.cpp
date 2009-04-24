

#include "stdafx.h"
#include "paplayer.h"
#include "CodecFactory.h"
#include "utils/GUIInfoManager.h"
#include "AudioContext.h"
#include "FileSystem/FileShoutcast.h"
#include "Application.h"
#include "Settings.h"
#include "FileItem.h"
#include "MusicInfoTag.h"

#ifdef HAS_AUDIO

#include "mmreg.h"

#define VOLUME_FFWD_MUTE 900 // 9dB

#define FADE_TIME 2 * 2048.0f / 48000.0f      // 2 packets

#define TIME_TO_CACHE_NEXT_FILE 5000L         // 5 seconds
#define TIME_TO_CROSS_FADE      10000L        // 10 seconds

extern XFILE::CFileShoutcast* m_pShoutCastRipper;

// PAP: Psycho-acoustic Audio Player
// Supporting all open  audio codec standards.
// First one being nullsoft's nsv audio decoder format

PAPlayer::PAPlayer(IPlayerCallback& callback) : IPlayer(callback)
{
  m_bIsPlaying = false;
  m_bPaused = false;
  m_cachingNextFile = false;
  m_currentlyCrossFading = false;
  m_bQueueFailed = false;

  m_currentDecoder = 0;

  m_iSpeed = 1;
  m_SeekTime=-1;
  m_IsFFwdRewding = false;
  m_timeOffset = 0;

  m_pStream[0] = NULL;
  m_pStream[1] = NULL;
  m_currentStream = 0;
  m_packet[0][0].packet = NULL;
  m_packet[1][0].packet = NULL;

  m_bytesSentOut = 0;

  m_BytesPerSecond = 0;
  m_SampleRate = 0;
  m_Channels = 0;
  m_BitsPerSample = 0;

  m_resampleAudio = true;

  m_visBufferLength = 0;
  m_pCallback = NULL;

  m_forceFadeToNext = false;
  m_CacheLevel = 0;
  m_LastCacheLevelCheck = 0;
  m_currentFile = new CFileItem;
  m_nextFile = new CFileItem;
}

PAPlayer::~PAPlayer()
{
  CloseFileInternal(true);
  delete m_currentFile;
  delete m_nextFile;
}

void PAPlayer::OnExit()
{

}

bool PAPlayer::OpenFile(const CFileItem& file, const CPlayerOptions &options)
{
  if (m_currentlyCrossFading) CloseFileInternal(false); //user seems to be in a hurry

  m_crossFading = g_guiSettings.GetInt("musicplayer.crossfade");
  //no crossfading for cdda, cd-reading goes mad and no crossfading for last.fm doesn't like two connections
  if (file.IsCDDA() || file.IsLastFM() || file.IsShoutCast()) m_crossFading = 0;
  if (m_crossFading && IsPlaying())
  {
    //do a short crossfade on trackskip
    //set to max 2 seconds for these prev/next transitions
    if (m_crossFading > 2) m_crossFading = 2;
    //queue for crossfading
    bool result = QueueNextFile(file, false);
    if (result)
    {
      //crossfading value may be update by QueueNextFile when nr of channels changed
      if (!m_crossFading) // swap to next track
        m_decoder[m_currentDecoder].SetStatus(STATUS_ENDED);
      else //force to fade to next track immediately
        m_forceFadeToNext = true;
    }
    return result;
  }

  // normal opening of file, nothing playing or crossfading not enabled
  // however no need to return to gui audio device
  CloseFileInternal(false);

  // always open the file using the current decoder
  m_currentDecoder = 0;

  if (!m_decoder[m_currentDecoder].Create(file, (__int64)(options.starttime * 1000), m_crossFading))
    return false;

  m_iSpeed = 1;
  m_bPaused = false;
  m_bStopPlaying = false;
  m_bytesSentOut = 0;

  CLog::Log(LOGINFO, "PAPlayer: Playing %s", file.m_strPath.c_str());

  m_timeOffset = (__int64)(options.starttime * 1000);

  m_decoder[m_currentDecoder].GetDataFormat(&m_Channels, &m_SampleRate, &m_BitsPerSample);

  SetupDirectSound(m_Channels);

  if (!CreateStream(m_currentStream, m_Channels, m_SampleRate, m_BitsPerSample))
  {
    m_decoder[m_currentDecoder].Destroy();
    CLog::Log(LOGERROR, "PAPlayer::Unable to create audio stream");
    return false;
  }

  *m_currentFile = file;

  if (ThreadHandle() == NULL)
    Create();

  m_startEvent.Set();

  m_bIsPlaying = true;
  m_cachingNextFile = false;
  m_currentlyCrossFading = false;
  m_forceFadeToNext = false;
  m_bQueueFailed = false;

  m_decoder[m_currentDecoder].Start();  // start playback

  return true;
}

void PAPlayer::UpdateCrossFadingTime(const CFileItem& file)
{
  if (m_crossFading = g_guiSettings.GetInt("musicplayer.crossfade"))
  {
    if (
      m_crossFading &&
      (
        file.IsCDDA() ||
        file.IsLastFM() ||
        file.IsShoutCast() ||
        (
          file.HasMusicInfoTag() && !g_guiSettings.GetBool("musicplayer.crossfadealbumtracks") &&
          (m_currentFile->GetMusicInfoTag()->GetAlbum() != "") &&
          (m_currentFile->GetMusicInfoTag()->GetAlbum() == file.GetMusicInfoTag()->GetAlbum()) &&
          (m_currentFile->GetMusicInfoTag()->GetDiscNumber() == file.GetMusicInfoTag()->GetDiscNumber()) &&
          (m_currentFile->GetMusicInfoTag()->GetTrackNumber() == file.GetMusicInfoTag()->GetTrackNumber() - 1)
        )
      )
    )
    {
      m_crossFading = 0;
    }
  }
}
void PAPlayer::OnNothingToQueueNotify()
{
  //nothing to queue, stop playing
  m_bQueueFailed = true;
}

bool PAPlayer::QueueNextFile(const CFileItem &file)
{
  return QueueNextFile(file, true);
}

bool PAPlayer::QueueNextFile(const CFileItem &file, bool checkCrossFading)
{
  if (IsPaused())
    Pause();

  if (file.m_strPath == m_currentFile->m_strPath &&
      file.m_lStartOffset > 0 &&
      file.m_lStartOffset == m_currentFile->m_lEndOffset)
  { // continuing on a .cue sheet item - return true to say we'll handle the transistion
    *m_nextFile = file;
    return true;
  }
  // check if we can handle this file at all
  int decoder = 1 - m_currentDecoder;
  __int64 seekOffset = (file.m_lStartOffset * 1000) / 75;
  if (!m_decoder[decoder].Create(file, seekOffset, m_crossFading))
  {
    m_bQueueFailed = true;
    return false;
  }
  // ok, we're good to go on queuing this one up
  CLog::Log(LOGINFO, "PAPlayer: Queuing next file %s", file.m_strPath.c_str());

  m_bQueueFailed = false;
  if (checkCrossFading)
  {
    UpdateCrossFadingTime(file);
  }

  unsigned int channels, samplerate, bitspersample;
  m_decoder[decoder].GetDataFormat(&channels, &samplerate, &bitspersample);

  // check the number of channels isn't changing (else we can't do crossfading)
  if (m_crossFading && m_decoder[m_currentDecoder].GetChannels() == channels)
  { // crossfading - need to create a new stream
    if (!CreateStream(1 - m_currentStream, channels, samplerate, bitspersample))
    {
      m_decoder[decoder].Destroy();
      CLog::Log(LOGERROR, "PAPlayer::Unable to create audio stream");
    }
  }
  else
  { // no crossfading if nr of channels is not the same
    m_crossFading = 0;
  }

  *m_nextFile = file;

  return true;
}



bool PAPlayer::CloseFileInternal(bool bAudioDevice /*= true*/)
{
  if (IsPaused())
    Pause();

  m_bStopPlaying = true;
  m_bStop = true;

  StopThread();

  // kill both our streams if we need to
  for (int i = 0; i < 2; i++)
  {
    m_decoder[i].Destroy();
    FreeStream(i);
  }

  m_currentFile->Reset();
  m_nextFile->Reset();

  if(bAudioDevice)
    g_audioContext.SetActiveDevice(CAudioContext::DEFAULT_DEVICE);

  return true;
}

void PAPlayer::FreeStream(int stream)
{
  if (m_pStream[stream])
  {
    m_pStream[stream]->Stop();
    m_pStream[stream]->Release();
  }
  m_pStream[stream] = NULL;

  if (m_packet[stream][0].packet)
    free(m_packet[stream][0].packet);
  for (int i = 0; i < PACKET_COUNT; i++)
  {
    m_packet[stream][i].packet = NULL;
  }

  m_resampler[stream].DeInitialize();
}

void PAPlayer::SetupDirectSound(int channels)
{
  bool bAudioOnAllSpeakers(false);
  g_audioContext.SetupSpeakerConfig(channels, bAudioOnAllSpeakers,true);
  g_audioContext.SetActiveDevice(CAudioContext::DIRECTSOUND_DEVICE);
  LPDIRECTSOUND pDSound=g_audioContext.GetDirectSoundDevice();
  if (!pDSound)
    return;
  // Set the default mixbins headroom to appropriate level as set in the settings file (to allow the maximum volume)
/*#ifdef HAS_XBOX_AUDIO
  for (DWORD i = 0; i < 8;i++)
    pDSound->SetMixBinHeadroom(i, DWORD(g_advancedSettings.m_audioHeadRoom / 6));
#endif*/
}

bool PAPlayer::CreateStream(int num, int channels, int samplerate, int bitspersample, CStdString codec)
{
  FreeStream(num);

  // Create our audio buffers
  // XphysicalAlloc has page (4k) granularity, so allocate all the buffers in one chunk
  m_packet[num][0].packet = (BYTE*)malloc(PACKET_SIZE * PACKET_COUNT);
  for (int i = 1; i < PACKET_COUNT ; i++)
    m_packet[num][i].packet = m_packet[num][i - 1].packet + PACKET_SIZE;

  if (channels <= 2 && g_advancedSettings.m_musicResample)
    m_SampleRateOutput = g_advancedSettings.m_musicResample;
  else
    m_SampleRateOutput = samplerate;

  m_BitsPerSampleOutput = 16;
  m_resampler[num].InitConverter(samplerate, bitspersample, channels, m_SampleRateOutput, m_BitsPerSampleOutput, PACKET_SIZE);

  // our wave format
  WAVEFORMATEX wfx    = {0};
  wfx.wFormatTag      = WAVE_FORMAT_PCM;
  wfx.nChannels       = channels;
  wfx.nSamplesPerSec  = m_SampleRateOutput;
  wfx.wBitsPerSample  = m_BitsPerSampleOutput;
  wfx.nBlockAlign     = wfx.nChannels * wfx.wBitsPerSample / 8;
  wfx.nAvgBytesPerSec = wfx.nBlockAlign * wfx.nSamplesPerSec;
  m_BytesPerSecond = wfx.nAvgBytesPerSec;

  WAVEFORMATEXTENSIBLE wfxex = {0};
  wfxex.Format = wfx;
  wfxex.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
  wfxex.Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX) ;
  wfxex.Samples.wReserved = 0;

  // setup the mixbins
  DWORD dwCMask;
/*#ifdef HAS_XBOX_AUDIO
  DSMIXBINS dsmb;
  DSMIXBINVOLUMEPAIR dsmbvp8[8];
  int iMixBinCount;

  if ((channels == 2) && (g_guiSettings.GetBool("musicplayer.outputtoallspeakers")))
    g_audioContext.GetMixBin(dsmbvp8, &iMixBinCount, &dwCMask, DSMIXBINTYPE_STEREOALL, channels);
  else
    g_audioContext.GetMixBin(dsmbvp8, &iMixBinCount, &dwCMask, DSMIXBINTYPE_STANDARD, channels);
  dsmb.dwMixBinCount = iMixBinCount;
  dsmb.lpMixBinVolumePairs = dsmbvp8;
#else*/
  dwCMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT;
  wfxex.dwChannelMask = dwCMask;

  // Set up the stream descriptor so we can create our streams
  DSBUFFERDESC dssd;
  memset(&dssd, 0, sizeof(dssd));

  dssd.dwSize = sizeof(DSBUFFERDESC);
  dssd.dwFlags = DSBCAPS_CTRLVOLUME | DSBCAPS_GLOBALFOCUS;
  dssd.dwBufferBytes = PACKET_SIZE * PACKET_COUNT;
  dssd.lpwfxFormat = &wfx;
  //dssd.guid3DAlgorithm = DS3DALG_DEFAULT;

  // Create the streams
  LPDIRECTSOUND pDSound=g_audioContext.GetDirectSoundDevice();
  if (!pDSound)
    return false;
  HRESULT hr = pDSound->CreateSoundBuffer( &dssd, &m_pStream[num], NULL);
  if( FAILED( hr ) )
    return false;

  m_pStream[num]->SetVolume(g_stSettings.m_nVolumeLevel);
  m_pStream[num]->Stop();

  // TODO: How do we best handle the callback, given that our samplerate etc. may be
  // changing at this point?

  m_nextPacket[num] = -1;
  // fire off our init to our callback
  if (m_pCallback)
    m_pCallback->OnInitialize(channels, m_SampleRateOutput, m_BitsPerSampleOutput);

  return true;
}

void PAPlayer::Pause()
{
  if (!m_bIsPlaying || !m_pStream) return ;

  m_bPaused = !m_bPaused;

  if (m_bPaused)
  { // pause both streams if we're crossfading
    if (m_pStream[m_currentStream]) m_pStream[m_currentStream]->Stop();
    if (m_currentlyCrossFading && m_pStream[1 - m_currentStream])
      m_pStream[1 - m_currentStream]->Stop();
    CLog::Log(LOGDEBUG, "PAPlayer: Playback paused");
  }
  else
  {
    if (m_pStream[m_currentStream]) m_pStream[m_currentStream]->Play(0, 0, DSBPLAY_LOOPING);
    if (m_currentlyCrossFading && m_pStream[1 - m_currentStream])
      m_pStream[1 - m_currentStream]->Play(0, 0, DSBPLAY_LOOPING);
    CLog::Log(LOGDEBUG, "PAPlayer: Playback resumed");
  }
}

void PAPlayer::SetVolume(long nVolume)
{
  if (m_pStream[m_currentStream])
    m_pStream[m_currentStream]->SetVolume(nVolume);
}

void PAPlayer::SetDynamicRangeCompression(long drc)
{
  // TODO: Add volume amplification
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

    CLog::Log(LOGINFO, "PAPlayer: End of playback reached");
    m_bIsPlaying = false;
    if (!m_bStopPlaying && !m_bStop)
      m_callback.OnPlayBackEnded();
    else
      m_callback.OnPlayBackStopped();
  }
}

void PAPlayer::ToFFRW(int iSpeed)
{
  m_iSpeed = iSpeed;
}

void PAPlayer::UpdateCacheLevel()
{
  //check cachelevel every .5 seconds
  if (m_LastCacheLevelCheck + 500 < GetTickCount())
  {
    ICodec* codec = m_decoder[m_currentDecoder].GetCodec();
    if (codec)
    {
      m_CacheLevel = codec->GetCacheLevel();
      m_LastCacheLevelCheck = GetTickCount();
      //CLog::Log(LOGDEBUG,"Cachelevel: %i%%", m_CacheLevel);
    }
  }
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
  while (true)
  {
    if (m_bStop) return false;

    // Check for .cue sheet item end
    if (m_currentFile->m_lEndOffset && GetTime() >= GetTotalTime64())
    {
      CLog::Log(LOGINFO, "PAPlayer: Passed end of track in a .cue sheet item");
      m_decoder[m_currentDecoder].SetStatus(STATUS_ENDED);
    }

    // check whether we need to send off our callbacks etc.
    int status = m_decoder[m_currentDecoder].GetStatus();
    if (status == STATUS_NO_FILE)
      return false;
    if (status == STATUS_PLAYING && !m_bPaused)
    {
      DWORD status;
      m_pStream[m_currentStream]->GetStatus(&status);
      if (!(status & DSBSTATUS_PLAYING))
        m_pStream[m_currentStream]->Play(0, 0, DSBPLAY_LOOPING);
    }

    UpdateCacheLevel();

    // check whether we should queue the next file up
    if ((GetTotalTime64() > 0) && GetTotalTime64() - GetTime() < TIME_TO_CACHE_NEXT_FILE + m_crossFading * 1000L && !m_cachingNextFile)
    { // request the next file from our application
      m_callback.OnQueueNextItem();
      m_cachingNextFile = true;
    }

    if (m_crossFading && m_decoder[0].GetChannels() == m_decoder[1].GetChannels())
    {
      if (((GetTotalTime64() - GetTime() < m_crossFading * 1000L) || (m_forceFadeToNext)) && !m_currentlyCrossFading)
      { // request the next file from our application
        if (m_decoder[1 - m_currentDecoder].GetStatus() == STATUS_QUEUED && m_pStream[1 - m_currentStream])
        {
          m_currentlyCrossFading = true;
          if (m_forceFadeToNext)
          {
            m_forceFadeToNext = false;
            m_crossFadeLength = m_crossFading * 1000L;
          }
          else
          {
            m_crossFadeLength = GetTotalTime64() - GetTime();
          }
          m_currentDecoder = 1 - m_currentDecoder;
          m_decoder[m_currentDecoder].Start();
          m_currentStream = 1 - m_currentStream;
          CLog::Log(LOGDEBUG, "Starting Crossfade - resuming stream %i", m_currentStream);

          m_pStream[m_currentStream]->Play(0, 0, DSBPLAY_LOOPING);

          m_callback.OnPlayBackStarted();
          m_timeOffset = m_nextFile->m_lStartOffset * 1000 / 75;
          m_bytesSentOut = 0;
          *m_currentFile = *m_nextFile;
          m_nextFile->Reset();
          m_cachingNextFile = false;
        }
      }
    }

    // Check for EOF and queue the next track if applicable
    if (m_decoder[m_currentDecoder].GetStatus() == STATUS_ENDED)
    { // time to swap tracks
      if (m_nextFile->m_strPath != m_currentFile->m_strPath ||
          !m_nextFile->m_lStartOffset ||
          m_nextFile->m_lStartOffset != m_currentFile->m_lEndOffset)
      { // don't have a .cue sheet item
        int nextstatus = m_decoder[1 - m_currentDecoder].GetStatus();
        if (nextstatus == STATUS_QUEUED || nextstatus == STATUS_QUEUING || nextstatus == STATUS_PLAYING)
        { // swap streams
          CLog::Log(LOGDEBUG, "PAPlayer: Swapping tracks %i to %i", m_currentDecoder, 1-m_currentDecoder);
          if (!m_crossFading || m_decoder[0].GetChannels() != m_decoder[1].GetChannels())
          { // playing gapless (we use only the 1 output stream in this case)
            int prefixAmount = m_decoder[m_currentDecoder].GetDataSize();
            CLog::Log(LOGDEBUG, "PAPlayer::Prefixing %i samples of old data to new track for gapless playback", prefixAmount);
            m_decoder[1 - m_currentDecoder].PrefixData(m_decoder[m_currentDecoder].GetData(prefixAmount), prefixAmount);
            // check if we need to change the resampler (due to format change)
            unsigned int channels, samplerate, bitspersample;
            m_decoder[m_currentDecoder].GetDataFormat(&channels, &samplerate, &bitspersample);
            unsigned int channels2, samplerate2, bitspersample2;
            m_decoder[1 - m_currentDecoder].GetDataFormat(&channels2, &samplerate2, &bitspersample2);
            // change of channels - reinitialize our speaker configuration
            if (channels != channels2)
            {
              CLog::Log(LOGWARNING, "PAPlayer: Channel number has changed - restarting direct sound");
              FreeStream(m_currentStream);
              SetupDirectSound(channels2);
              if (!CreateStream(m_currentStream, channels2, samplerate2, bitspersample2))
              {
                CLog::Log(LOGERROR, "PAPlayer: Error creating stream!");
                return false;
              }

              m_pStream[m_currentStream]->Play(0, 0, DSBPLAY_LOOPING);
            }
            else if (samplerate != samplerate2 || bitspersample != bitspersample2)
            {
              CLog::Log(LOGINFO, "PAPlayer: Restarting resampler due to a change in data format");
              m_resampler[m_currentStream].DeInitialize();
              if (!m_resampler[m_currentStream].InitConverter(samplerate2, bitspersample2, channels2, 48000, 16, PACKET_SIZE))
              {
                CLog::Log(LOGERROR, "PAPlayer: Error initializing resampler!");
                return false;
              }
            }
            CLog::Log(LOGINFO, "PAPlayer: Starting new track");

            m_decoder[m_currentDecoder].Destroy();
            m_decoder[1 - m_currentDecoder].Start();
            m_callback.OnPlayBackStarted();
            m_timeOffset = m_nextFile->m_lStartOffset * 1000 / 75;
            m_bytesSentOut = 0;
            *m_currentFile = *m_nextFile;
            m_nextFile->Reset();
            m_cachingNextFile = false;
            m_currentDecoder = 1 - m_currentDecoder;
          }
          else
          { // cross fading - shouldn't ever get here - if we do, return false
            if (!m_currentlyCrossFading)
            {
              CLog::Log(LOGERROR, "End of file Reached before crossfading kicked in!");
              return false;
            }
            else
            {
              CLog::Log(LOGINFO, "End of file reached before crossfading finished!");
              return false;
            }
          }
        }
        else
        {
          if (GetTotalTime64() <= 0 && !m_bQueueFailed)
          { //we did not know the duration so didn't queue the next song, try queueing it now
            if (!m_cachingNextFile)
            {// request the next file from our application
              m_callback.OnQueueNextItem();
              m_cachingNextFile = true;
            }
          }
          else
          {
            // no track queued - return and get another one once we are finished
            // with the current stream
            WaitForStream();
            return false;
          }
        }
      }
      else
      {
        // set the next track playing (.cue sheet)
        m_decoder[m_currentDecoder].SetStatus(STATUS_PLAYING);
        m_callback.OnPlayBackStarted();
        m_timeOffset = m_nextFile->m_lStartOffset * 1000 / 75;
        m_bytesSentOut = 0;
        *m_currentFile = *m_nextFile;
        m_nextFile->Reset();
        m_cachingNextFile = false;
      }
    }

    // handle seeking and ffwd/rewding.
    HandleSeeking();
    if (!HandleFFwdRewd())
    {
      // need to skip to the next track - let's see if we already have another one
      m_decoder[m_currentDecoder].SetStatus(STATUS_ENDED);
      continue; // loop around to start the next track
    }

    // Let our decoding stream(s) do their thing
    DWORD time = timeGetTime();
    int retVal = m_decoder[m_currentDecoder].ReadSamples(PACKET_SIZE);
    if (retVal == RET_ERROR)
    {
      m_decoder[m_currentDecoder].Destroy();
      return false;
    }
    int retVal2 = m_decoder[1 - m_currentDecoder].ReadSamples(PACKET_SIZE);
    if (retVal2 == RET_ERROR)
    {
      m_decoder[1 - m_currentDecoder].Destroy();
    }
    DWORD time2 = timeGetTime();

    // if we're cross-fading, then we do this for both streams, otherwise
    // we do it just for the one stream.
    if (m_currentlyCrossFading)
    {
      if (GetTime() >= m_crossFadeLength)  // finished
      {
        CLog::Log(LOGDEBUG, "Finished Crossfading");
        m_currentlyCrossFading = false;
        SetStreamVolume(m_currentStream, g_stSettings.m_nVolumeLevel);
        FreeStream(1 - m_currentStream);
        m_decoder[1 - m_currentDecoder].Destroy();
      }
      else
      {
        float fraction = (float)(m_crossFadeLength - GetTime()) / (float)m_crossFadeLength - 0.5f;
        // make sure we can take valid logs.
        if (fraction > 0.499f) fraction = 0.499f;
        if (fraction < -0.499f) fraction = -0.499f;
        float volumeCurrent = 2000.0f * log10(0.5f - fraction);
        float volumeNext = 2000.0f * log10(0.5f + fraction);
        SetStreamVolume(m_currentStream, g_stSettings.m_nVolumeLevel + (int)volumeCurrent);
        SetStreamVolume(1 - m_currentStream, g_stSettings.m_nVolumeLevel + (int)volumeNext);
        if (AddPacketsToStream(1 - m_currentStream, m_decoder[1 - m_currentDecoder]))
          retVal2 = RET_SUCCESS;
      }
    }
    // add packets as necessary
    if (AddPacketsToStream(m_currentStream, m_decoder[m_currentDecoder]))
      retVal = RET_SUCCESS;

    if (retVal == RET_SLEEP && retVal2 == RET_SLEEP)
      Sleep(1);
    DWORD time3 = timeGetTime();
//   CLog::Log(LOGINFO, "Time Decoding: %i, Time Resampling: %i, bytes processed %i, buffer 1 state %i, buffer 2 state %i", time2-time, time3-time2, dataToRead, m_decoder[m_currentDecoder].GetDataSize(), m_decoder[1 - m_currentDecoder].GetDataSize());
  }
  return true;
}

void PAPlayer::ResetTime()
{
  m_bytesSentOut = 0;
}

__int64 PAPlayer::GetTime()
{
  __int64  timeplus = m_BytesPerSecond ? (__int64)(((float) m_bytesSentOut / (float)m_BytesPerSecond ) * 1000.0) : 0;
  return m_timeOffset + timeplus - m_currentFile->m_lStartOffset * 1000 / 75;
}

__int64 PAPlayer::GetTotalTime64()
{
  __int64 total = m_decoder[m_currentDecoder].TotalTime();
  if (m_currentFile->m_lEndOffset)
    total = m_currentFile->m_lEndOffset * 1000 / 75;
  if (m_currentFile->m_lStartOffset)
    total -= m_currentFile->m_lStartOffset * 1000 / 75;
  return total;
}

int PAPlayer::GetTotalTime()
{
  return (int)(GetTotalTime64()/1000);
}

int PAPlayer::GetCacheLevel() const
{
  return m_CacheLevel;
}

int PAPlayer::GetChannels()
{
  ICodec* codec = m_decoder[m_currentDecoder].GetCodec();
  if (codec)
    return codec->m_Channels;
  return 0;
}

int PAPlayer::GetBitsPerSample()
{
  ICodec* codec = m_decoder[m_currentDecoder].GetCodec();
  if (codec)
    return codec->m_BitsPerSample;
  return 0;
}

int PAPlayer::GetSampleRate()
{
  ICodec* codec = m_decoder[m_currentDecoder].GetCodec();
  if (codec)
    return (int)((codec->m_SampleRate / 1000) + 0.5);
  return 0;
}

CStdString PAPlayer::GetCodecName()
{
  ICodec* codec = m_decoder[m_currentDecoder].GetCodec();
  if (codec)
    return codec->m_CodecName;
  return "";
}

int PAPlayer::GetAudioBitrate()
{
  ICodec* codec = m_decoder[m_currentDecoder].GetCodec();
  if (codec)
    return (int)((codec->m_Bitrate / 1000) + 0.5); // in kbits/s, rounded to the nearest int
  return 0;
}

bool PAPlayer::CanSeek()
{
  return ((m_decoder[m_currentDecoder].TotalTime() > 0) && m_decoder[m_currentDecoder].CanSeek());
}

void PAPlayer::Seek(bool bPlus, bool bLargeStep)
{
  __int64 seek;
  if (g_advancedSettings.m_musicUseTimeSeeking && GetTotalTime() > 2*g_advancedSettings.m_musicTimeSeekForwardBig)
  {
    if (bLargeStep)
      seek = bPlus ? g_advancedSettings.m_musicTimeSeekForwardBig : g_advancedSettings.m_musicTimeSeekBackwardBig;
    else
      seek = bPlus ? g_advancedSettings.m_musicTimeSeekForward : g_advancedSettings.m_musicTimeSeekBackward;
    seek *= 1000;
    seek += GetTime();
  }
  else
  {
    float percent;
    if (bLargeStep)
      percent = bPlus ? (float)g_advancedSettings.m_musicPercentSeekForwardBig : (float)g_advancedSettings.m_musicPercentSeekBackwardBig;
    else
      percent = bPlus ? (float)g_advancedSettings.m_musicPercentSeekForward : (float)g_advancedSettings.m_musicPercentSeekBackward;
    seek = (__int64)(GetTotalTime64()*(GetPercentage()+percent)/100);
  }

  SeekTime(seek);
}

void PAPlayer::SeekTime(__int64 iTime /*=0*/)
{
  if (!CanSeek()) return;
  if (m_currentFile->m_lStartOffset)
    iTime += m_currentFile->m_lStartOffset * 1000 / 75;
  m_SeekTime = iTime;
  CLog::Log(LOGDEBUG, "PAPlayer::Seeking to time %f", 0.001f * m_SeekTime);
}

void PAPlayer::SeekPercentage(float fPercent /*=0*/)
{
  if (fPercent < 0.0f) fPercent = 0.0f;
  if (fPercent > 100.0f) fPercent = 100.0f;
  SeekTime((__int64)(fPercent * 0.01f * (float)GetTotalTime64()));
}

float PAPlayer::GetPercentage()
{
  float percent = (float)GetTime() * 100.0f / GetTotalTime64();
  return percent;
}

void PAPlayer::HandleSeeking()
{
  if (m_SeekTime != -1)
  {
    DWORD time = timeGetTime();
    m_timeOffset = m_decoder[m_currentDecoder].Seek(m_SeekTime);
    CLog::Log(LOGDEBUG, "Seek to time %f took %i ms", 0.001f * m_SeekTime, timeGetTime() - time);
    FlushStreams();
    m_bytesSentOut = 0;
    m_SeekTime = -1;
  }
  g_infoManager.m_performingSeek = false;
}

void PAPlayer::FlushStreams()
{
  for (int stream = 0; stream < 2; stream++)
  {
    if (m_pStream[stream] && m_packet[stream])
    {
      DWORD status;
      m_pStream[stream]->GetStatus(&status);
/*#ifdef HAS_XBOX_AUDIO
      m_pStream[stream]->Flush();
      for (int i = PACKET_COUNT; i; i--)
        m_packet[stream][i].status = XMEDIAPACKET_STATUS_SUCCESS;
      // make sure it's still paused if it should be
      if (status == DSSTREAMSTATUS_PAUSED)
      {
        CLog::Log(LOGINFO, "Pausing stream %i after Flush()", stream);
        m_pStream[stream]->Pause(DSSTREAMPAUSE_PAUSE);
      }
#endif*/
    }
  }
}

bool PAPlayer::HandleFFwdRewd()
{
  if (!m_IsFFwdRewding && m_iSpeed == 1)
    return true;  // nothing to do
  if (m_IsFFwdRewding && m_iSpeed == 1)
  { // stop ffwd/rewd
    m_IsFFwdRewding = false;
    SetVolume(g_stSettings.m_nVolumeLevel);
    m_bytesSentOut = 0;
    FlushStreams();
    return true;
  }
  // we're definitely fastforwarding or rewinding
  int snippet = m_BytesPerSecond / 2;
  if ( m_bytesSentOut >= snippet )
  {
    // Calculate offset to seek if we do FF/RW
    __int64 time = GetTime();
    if (m_IsFFwdRewding) snippet = (int)m_bytesSentOut;
    time += (__int64)((double)snippet * (m_iSpeed - 1.0) / m_BytesPerSecond * 1000.0);

    // Is our offset inside the track range?
    if (time >= 0 && time <= m_decoder[m_currentDecoder].TotalTime())
    { // just set next position to read
      m_IsFFwdRewding = true;
      time += m_currentFile->m_lStartOffset * 1000 / 75;
      m_timeOffset = m_decoder[m_currentDecoder].Seek(time);
      m_bytesSentOut = 0;
      FlushStreams();
      SetVolume(g_stSettings.m_nVolumeLevel - VOLUME_FFWD_MUTE); // override xbmc mute
    }
    else if (time < 0)
    { // ...disable seeking and start the track again
      time = m_currentFile->m_lStartOffset * 1000 / 75;
      m_timeOffset = m_decoder[m_currentDecoder].Seek(time);
      m_bytesSentOut = 0;
      FlushStreams();
      m_iSpeed = 1;
      SetVolume(g_stSettings.m_nVolumeLevel); // override xbmc mute
    } // is our next position greater then the end sector...
    else //if (time > m_codec->m_TotalTime)
    {
      // restore volume level so the next track isn't muted
      SetVolume(g_stSettings.m_nVolumeLevel);
      CLog::Log(LOGDEBUG, "PAPlayer: End of track reached while seeking");
      return false;
    }
  }
  return true;
}

void PAPlayer::SetStreamVolume(int stream, long nVolume)
{
  if (nVolume > DSBVOLUME_MAX) nVolume = DSBVOLUME_MAX;
  if (nVolume < DSBVOLUME_MIN) nVolume = DSBVOLUME_MIN;
  if (m_pStream[stream])
    m_pStream[stream]->SetVolume(nVolume);
}

bool PAPlayer::AddPacketsToStream(int stream, CAudioDecoder &dec)
{
  if (!m_pStream[stream] || dec.GetStatus() == STATUS_NO_FILE)
    return false;

  bool ret = false;
  // find a free packet and fill it with the decoded data
  DWORD playCursor, writeCursor;
  if (SUCCEEDED(m_pStream[stream]->GetCurrentPosition(&playCursor, &writeCursor)))
  {
    // we may write from writeCursor to playCursor - do it in chunks
    // round up writecursor and down playcursor
    DWORD writePos = writeCursor / PACKET_SIZE + 1;
    DWORD playPos = playCursor / PACKET_SIZE;
    if (m_nextPacket[stream] == -1)
    {
      m_nextPacket[stream] = writePos;
    }
    DWORD nextPacket = m_nextPacket[stream];
    // we must write into nextPacket
    while ((playPos < writePos && (nextPacket >= writePos || nextPacket < playPos)) ||
           (playPos > writePos && (nextPacket >= writePos && nextPacket < playPos)))
    { // we have space from writePos to playPos
      if (m_resampler[stream].GetData(m_packet[stream][nextPacket].packet))
      {
        // got some data from our resampler - construct audio packet
        m_packet[stream][nextPacket].length = PACKET_SIZE;
        m_packet[stream][nextPacket].status = 1;//XMEDIAPACKET_STATUS_PENDING;
        m_packet[stream][nextPacket].stream = stream;

        //TODO: is this the right place to call back? maybe when it has played?
        StreamCallback(&m_packet[stream][nextPacket].packet);

        DWORD  offset = nextPacket * PACKET_SIZE;
        LPVOID start;
        DWORD  size;
        LPVOID startWrap;
        DWORD  sizeWrap;
        // copy data into our buffer
        if (FAILED(m_pStream[stream]->Lock(offset, PACKET_SIZE, &start, &size, &startWrap, &sizeWrap, 0)))
        { // bad news :(
          CLog::Log(LOGERROR, "Error adding packet %i to stream %i", nextPacket, stream);
          return false;
        }
        // copy the data over
        //CLog::Log(LOGDEBUG, "Adding packet %i at %p.  (play: %i, write: %i)", nextPacket, start, playCursor, writeCursor);
        memcpy(start, m_packet[stream][nextPacket].packet, size);
        if (startWrap)
          memcpy(startWrap, m_packet[stream][nextPacket].packet + size, sizeWrap);
        m_pStream[stream]->Unlock(start, size, startWrap, sizeWrap);
        // something done
        ret = true;
        m_nextPacket[stream] = (nextPacket + 1) % PACKET_COUNT;
        nextPacket = m_nextPacket[stream];
      }
      else
      { // resampler wants more data - let's feed it
        int amount = m_resampler[stream].GetInputSamples();
        if (amount <= 0 || amount > (int)dec.GetDataSize())
        { // doesn't need anything
          break;
        }
        // needs some data - let's feed it
        m_resampler[stream].PutFloatData((float *)dec.GetData(amount), amount);
        ret = true;
      }
    }
  }
  return ret;
}

bool PAPlayer::FindFreePacket( int stream, DWORD* pdwPacket )
{
  for( DWORD dwIndex = 0; dwIndex < PACKET_COUNT; dwIndex++ )
  {
    // The first EXTRA_PACKETS * 2 packets are reserved - odd packets
    // for stream 1, even packets for stream 2.  This is to ensure
    // that there are packets available during the crossfade
#ifdef HAS_XBOX_AUDIO
    if( XMEDIAPACKET_STATUS_PENDING != m_packet[stream][dwIndex].status )
    {
      (*pdwPacket) = dwIndex;
      return true;
    }
#endif
  }
  return false;
}

void PAPlayer::RegisterAudioCallback(IAudioCallback *pCallback)
{
  m_pCallback = pCallback;
  if (m_pCallback)
    m_pCallback->OnInitialize(m_Channels, m_SampleRateOutput, m_BitsPerSampleOutput);
}

void PAPlayer::UnRegisterAudioCallback()
{
  m_pCallback = NULL;
}

void PAPlayer::DoAudioWork()
{
  if (m_pCallback && m_visBufferLength)
  {
    m_pCallback->OnAudioData((const unsigned char *)m_visBuffer, m_visBufferLength);
    m_visBufferLength = 0;
  }
}

void PAPlayer::StreamCallback( LPVOID pPacketContext )
{
  AudioPacket *pkt = (AudioPacket *)pPacketContext;

  // only process from the current stream (if we're crossfading for instance)
  if (pkt->stream != m_currentStream)
    return;

  m_bytesSentOut += pkt->length;

  if (m_pCallback)
  { // copy into our visualisation buffer.
    // can't use a memcpy() here due to the context (will crash otherwise)
    memcpy(m_visBuffer, pkt->packet, pkt->length);
    m_visBufferLength = pkt->length;
  }
}

void CALLBACK StaticStreamCallback( VOID* pStreamContext, VOID* pPacketContext, DWORD dwStatus )
{
  PAPlayer* pPlayer = (PAPlayer*)pStreamContext;

#ifdef HAS_XBOX_AUDIO
  if( dwStatus == XMEDIAPACKET_STATUS_SUCCESS )
#endif
  {
    pPlayer->StreamCallback(pPacketContext);
  }
}

bool PAPlayer::HandlesType(const CStdString &type)
{
  ICodec* codec=CodecFactory::CreateCodec(type);

  if (codec && codec->CanInit())
  {
    delete codec;
    return true;
  }
  if (codec)
    delete codec;

  return false;
}

// Skip to next track/item inside the current media (if supported).
bool PAPlayer::SkipNext()
{
  if (m_decoder[m_currentDecoder].GetCodec() && m_decoder[m_currentDecoder].GetCodec()->SkipNext())
  {
    return true;
  }
  return false;
}

bool PAPlayer::CanRecord()
{
  if (!m_pShoutCastRipper) return false;
  return m_pShoutCastRipper->CanRecord();
}

bool PAPlayer::IsRecording()
{
  if (!m_pShoutCastRipper) return false;
  return m_pShoutCastRipper->IsRecording();
}

bool PAPlayer::Record(bool bOnOff)
{
  if (!m_pShoutCastRipper) return false;
  if (bOnOff && IsRecording()) return true;
  if (bOnOff == false && IsRecording() == false) return true;
  if (bOnOff)
    return m_pShoutCastRipper->Record();

  m_pShoutCastRipper->StopRecording();
  return true;
}

void PAPlayer::WaitForStream()
{
  // should we wait for our other stream as well?
  // currently we don't.
  if (!m_pStream[m_currentStream])
  {
    DWORD status;
    do
    {
      m_pStream[m_currentStream]->GetStatus(&status);
    }
    while (status & DSBSTATUS_PLAYING);
  }
}

#endif
