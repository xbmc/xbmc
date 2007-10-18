#include "../../stdafx.h"
#include "paplayer.h"
#include "CodecFactory.h"
#include "../../utils/GUIInfoManager.h"
#include "AudioContext.h"
#include "../../FileSystem/FileShoutcast.h"
#include "../../Application.h"
#ifdef HAS_KARAOKE
#include "../../CdgParser.h"
#endif

#ifdef _LINUX
#define XBMC_SAMPLE_RATE 44100
#else
#define XBMC_SAMPLE_RATE 48000
#endif

#define CHECK_ALSA(l,s,e) if ((e)<0) CLog::Log(l,"%s - %s, alsa error: %s",__FUNCTION__,s,snd_strerror(e));
#define CHECK_ALSA_RETURN(l,s,e) CHECK_ALSA((l),(s),(e)); if ((e)<0) return false;

#define VOLUME_FFWD_MUTE 900 // 9dB

#define FADE_TIME 2 * 2048.0f / XBMC_SAMPLE_RATE.0f      // 2 packets

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

  // periods will contain the amount of data that can be played with each call to alsa.
  // the unit is "Frames". for 2 channels 16 bit its 4.
  // we initialize with packet_size which is probably too big. later the alsa calls will set these
  // values correctly.
  m_periods[0] = PACKET_SIZE / 4;
  m_periods[1] = PACKET_SIZE / 4;

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
}

PAPlayer::~PAPlayer()
{
  CloseFileInternal(true);
}

void PAPlayer::OnExit()
{

}

bool PAPlayer::OpenFile(const CFileItem& file, const CPlayerOptions &options)
{
  if (m_currentlyCrossFading) CloseFileInternal(false); //user seems to be in a hurry

  m_crossFading = g_guiSettings.GetInt("musicplayer.crossfade");
  //no crossfading for cdda, cd-reading goes mad and no crossfading for last.fm doesn't like two connections
  if (file.IsCDDA() || file.IsLastFM()) m_crossFading = 0;
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

  CLog::Log(LOGINFO, "PAP Player: Playing %s", file.m_strPath.c_str());

  m_timeOffset = (__int64)(options.starttime * 1000);

  m_decoder[m_currentDecoder].GetDataFormat(&m_Channels, &m_SampleRate, &m_BitsPerSample);

  if (!CreateStream(m_currentStream, m_Channels, m_SampleRate, m_BitsPerSample))
  {
    m_decoder[m_currentDecoder].Destroy();
    CLog::Log(LOGERROR, "PAPlayer::Unable to create audio stream");
  }

  m_currentFile = file;

  if (ThreadHandle() == NULL)
    Create();

  m_startEvent.Set();

  m_bIsPlaying = true;
  m_cachingNextFile = false;
  m_currentlyCrossFading = false;
  m_forceFadeToNext = false;
  m_bQueueFailed = false;

  m_decoder[m_currentDecoder].Start();  // start playback

  if (m_pStream[m_currentStream])
     snd_pcm_reset(m_pStream[m_currentStream]);

  if (m_pStream[m_currentStream])
     snd_pcm_pause(m_pStream[m_currentStream], 0);

  return true;
}

void PAPlayer::UpdateCrossFadingTime(const CFileItem& file)
{
  if ((m_crossFading = g_guiSettings.GetInt("musicplayer.crossfade")))
  {
    if (
      m_crossFading &&
      (
        file.IsCDDA() ||
        file.IsLastFM() ||
        (
          file.HasMusicInfoTag() && !g_guiSettings.GetBool("musicplayer.crossfadealbumtracks") &&
          (m_currentFile.GetMusicInfoTag()->GetAlbum() != "") &&
          (m_currentFile.GetMusicInfoTag()->GetAlbum() == file.GetMusicInfoTag()->GetAlbum()) &&
          (m_currentFile.GetMusicInfoTag()->GetDiscNumber() == file.GetMusicInfoTag()->GetDiscNumber()) &&
          (m_currentFile.GetMusicInfoTag()->GetTrackNumber() == file.GetMusicInfoTag()->GetTrackNumber() - 1)
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

  if (file.m_strPath == m_currentFile.m_strPath &&
      file.m_lStartOffset > 0 && 
      file.m_lStartOffset == m_currentFile.m_lEndOffset)
  { // continuing on a .cue sheet item - return true to say we'll handle the transistion
    m_nextFile = file;
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
  CLog::Log(LOGINFO, "PAP Player: Queuing next file %s", file.m_strPath.c_str());

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

  m_nextFile = file;

  return true;
}



bool PAPlayer::CloseFileInternal(bool bAudioDevice /*= true*/)
{
  if (IsPaused())
    Pause();

  m_bStopPlaying = true;
  m_bStop = true;

  m_visBufferLength = 0;
  StopThread();

  // kill both our streams if we need to
  for (int i = 0; i < 2; i++)
  {
    m_decoder[i].Destroy();
    FreeStream(i);
  }

  m_currentFile.Reset();
  m_nextFile.Reset();

  if(bAudioDevice)
    g_audioContext.SetActiveDevice(CAudioContext::DEFAULT_DEVICE);

  return true;
}

void PAPlayer::FreeStream(int stream)
{
  if (m_pStream[stream])
  {
    snd_pcm_drain(m_pStream[stream]);
    snd_pcm_close(m_pStream[stream]);
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

bool PAPlayer::CreateStream(int num, int channels, int samplerate, int bitspersample, CStdString codec)
{
  snd_pcm_hw_params_t *hw_params=NULL;

  FreeStream(num);

  m_packet[num][0].packet = (BYTE*)malloc(PACKET_SIZE * PACKET_COUNT);
  for (int i = 1; i < PACKET_COUNT ; i++)
    m_packet[num][i].packet = m_packet[num][i - 1].packet + PACKET_SIZE;

  m_SampleRateOutput = channels>2?samplerate:XBMC_SAMPLE_RATE;
  m_BitsPerSampleOutput = 16;

  m_BytesPerSecond = (m_BitsPerSampleOutput / 8)*m_SampleRateOutput*channels;

	/* Open the device */
	int nErr = snd_pcm_open(&m_pStream[num], g_guiSettings.GetString("audiooutput.audiodevice"), SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK);
    CHECK_ALSA_RETURN(LOGERROR,"pcm_open",nErr);

	/* Allocate Hardware Parameters structures and fills it with config space for PCM */
	snd_pcm_hw_params_alloca(&hw_params);

	nErr = snd_pcm_hw_params_any(m_pStream[num], hw_params);
    CHECK_ALSA_RETURN(LOGERROR,"hw_params_any",nErr);

	nErr = snd_pcm_hw_params_set_access(m_pStream[num], hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
    CHECK_ALSA_RETURN(LOGERROR,"hw_params_set_access",nErr);
	
	// always use 16 bit samples
	nErr = snd_pcm_hw_params_set_format(m_pStream[num], hw_params, SND_PCM_FORMAT_S16_LE);
    CHECK_ALSA_RETURN(LOGERROR,"hw_params_set_format",nErr);

	nErr = snd_pcm_hw_params_set_rate_near(m_pStream[num], hw_params, &m_SampleRateOutput, NULL);
    CHECK_ALSA_RETURN(LOGERROR,"hw_params_set_rate",nErr);

	nErr = snd_pcm_hw_params_set_channels(m_pStream[num], hw_params, channels);
    CHECK_ALSA_RETURN(LOGERROR,"hw_params_set_channels",nErr);
	
    m_periods[num] = PACKET_SIZE / 4;
    nErr = snd_pcm_hw_params_set_period_size_near(m_pStream[num], hw_params, &m_periods[num], 0);
    CHECK_ALSA_RETURN(LOGERROR,"hw_params_set_period_size",nErr);

	snd_pcm_uframes_t buffer_size = PACKET_SIZE*20; // big enough buffer
    nErr = snd_pcm_hw_params_set_buffer_size_near(m_pStream[num], hw_params, &buffer_size);
    CHECK_ALSA_RETURN(LOGERROR,"hw_params_set_buffer_size",nErr);

	unsigned int periodDuration = 0;
	nErr = snd_pcm_hw_params_get_period_time(hw_params,&periodDuration, 0);
    CHECK_ALSA(LOGERROR,"hw_params_get_period_time",nErr);

    CLog::Log(LOGDEBUG,"PAPlayer::CreateStream - initialized. "
			   "sample rate: %d, channels: %d, period size: %d, buffer size: %d "
			   "period duration: %d", 
               m_SampleRateOutput, 
 			   channels,
			   (int) m_periods[num], 
			   (int) buffer_size,
			   periodDuration); 

	
  	/* Assign them to the playback handle and free the parameters structure */
	nErr = snd_pcm_hw_params(m_pStream[num], hw_params);
    CHECK_ALSA_RETURN(LOGERROR,"snd_pcm_hw_params",nErr);

	/* disable underrun reporting and play silence */
	nErr = snd_pcm_prepare (m_pStream[num]);
    CHECK_ALSA(LOGERROR,"snd_pcm_prepare",nErr);

    // create our resampler  // upsample to XBMC_SAMPLE_RATE, only do this for sources with 1 or 2 channels
    m_resampler[num].InitConverter(samplerate, bitspersample, channels, m_SampleRateOutput, m_BitsPerSampleOutput, PACKET_SIZE);

    // set initial volume
    SetStreamVolume(num, g_stSettings.m_nVolumeLevel);
  
  	// TODO: How do we best handle the callback, given that our samplerate etc. may be
  	// changing at this point?

  	// fire off our init to our callback
  	if (m_pCallback)
    	m_pCallback->OnInitialize(channels, m_SampleRateOutput, m_BitsPerSampleOutput);

  return true;
}

void PAPlayer::Pause()
{
  CLog::Log(LOGDEBUG,"PAPlayer: pause m_bplaying: %d", m_bIsPlaying);
  if (!m_bIsPlaying || !m_pStream) 
	return ;

  m_bPaused = !m_bPaused;

  if (m_bPaused)
  { 
	// pause both streams if we're crossfading
    if (m_pStream[m_currentStream]) 
		snd_pcm_pause(m_pStream[m_currentStream], 1);
    
	if (m_currentlyCrossFading && m_pStream[1 - m_currentStream])
		snd_pcm_pause(m_pStream[1 - m_currentStream], 1);
    
	CLog::Log(LOGDEBUG, "PAP Player: Playback paused");
  }
  else
  {
    if (m_pStream[m_currentStream]) {
		snd_pcm_pause(m_pStream[m_currentStream], 0);
	}
    
	if (m_currentlyCrossFading && m_pStream[1 - m_currentStream])
		snd_pcm_pause(m_pStream[1 - m_currentStream], 0);

	FlushStreams();

    CLog::Log(LOGDEBUG, "PAP Player: Playback resumed");
  }
}

void PAPlayer::SetVolume(long nVolume)
{
   // Todo: Grrrr.... no volume level in alsa. We'll need to do the math ourselves.
  m_amp[m_currentStream].SetVolume(nVolume);

}

void PAPlayer::SetDynamicRangeCompression(long drc)
{
  // TODO: Add volume amplification
  CLog::Log(LOGDEBUG,"PAPlayer::SetDynamicRangeCompression - drc: %lu", drc);
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
    {
      m_callback.OnPlayBackEnded();
    }
  }
  CLog::Log(LOGDEBUG, "PAPlayer: Thread end");
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
    if (m_currentFile.m_lEndOffset && GetTime() >= GetTotalTime64())
    {
      CLog::Log(LOGINFO, "PAPlayer: Passed end of track in a .cue sheet item");
      m_decoder[m_currentDecoder].SetStatus(STATUS_ENDED);
    }

    // check whether we need to send off our callbacks etc.
    int status = m_decoder[m_currentDecoder].GetStatus();
    if (status == STATUS_NO_FILE)
      return false;

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

          snd_pcm_pause(m_pStream[m_currentStream], 0);

          m_callback.OnPlayBackStarted();
          m_timeOffset = m_nextFile.m_lStartOffset * 1000 / 75;
          m_bytesSentOut = 0;
          m_currentFile = m_nextFile;
          m_nextFile.Reset();
          m_cachingNextFile = false;
        }
      }
    }

    // Check for EOF and queue the next track if applicable
    if (m_decoder[m_currentDecoder].GetStatus() == STATUS_ENDED)
    { // time to swap tracks
      if (m_nextFile.m_strPath != m_currentFile.m_strPath ||
          !m_nextFile.m_lStartOffset ||
          m_nextFile.m_lStartOffset != m_currentFile.m_lEndOffset)
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
              if (!CreateStream(m_currentStream, channels2, samplerate2, bitspersample2))
              {
                CLog::Log(LOGERROR, "PAPlayer: Error creating stream!");
                return false;
              }
              snd_pcm_pause(m_pStream[m_currentStream], 0);
            }
            else if (samplerate != samplerate2 || bitspersample != bitspersample2)
            {
              CLog::Log(LOGINFO, "PAPlayer: Restarting resampler due to a change in data format");
              m_resampler[m_currentStream].DeInitialize();
              if (!m_resampler[m_currentStream].InitConverter(samplerate2, bitspersample2, channels2, XBMC_SAMPLE_RATE, 16, PACKET_SIZE))
              {
                CLog::Log(LOGERROR, "PAPlayer: Error initializing resampler!");
                return false;
              }
            }
            CLog::Log(LOGINFO, "PAPlayer: Starting new track");

            m_decoder[m_currentDecoder].Destroy();
            m_decoder[1 - m_currentDecoder].Start();
            m_callback.OnPlayBackStarted();
            m_timeOffset = m_nextFile.m_lStartOffset * 1000 / 75;
            m_bytesSentOut = 0;
            m_currentFile = m_nextFile;
            m_nextFile.Reset();
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
        m_timeOffset = m_nextFile.m_lStartOffset * 1000 / 75;
        m_bytesSentOut = 0;
        m_currentFile = m_nextFile;
        m_nextFile.Reset();
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

    if (!m_bPaused) {

		// Let our decoding stream(s) do their thing
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
    }
    else
        Sleep(100);
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
  return m_timeOffset + timeplus - m_currentFile.m_lStartOffset * 1000 / 75;
}

__int64 PAPlayer::GetTotalTime64()
{
  __int64 total = m_decoder[m_currentDecoder].TotalTime();
  if (m_currentFile.m_lEndOffset)
    total = m_currentFile.m_lEndOffset * 1000 / 75;
  if (m_currentFile.m_lStartOffset)
    total -= m_currentFile.m_lStartOffset * 1000 / 75;
  return total;
}

int PAPlayer::GetTotalTime()
{
  return (int)(GetTotalTime64()/1000);
}

bool PAPlayer::IsCaching() const
{
  const ICodec* codec = m_decoder[m_currentDecoder].GetCodec();
  if (codec)
    return codec->IsCaching();

  return false;
}

int PAPlayer::GetCacheLevel() const
{
  const ICodec* codec = m_decoder[m_currentDecoder].GetCodec();
  if (codec)
    return codec->GetCacheLevel();

  return -1;
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

int PAPlayer::GetBitrate()
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

void PAPlayer::SeekTime(__int64 iTime /*=0*/)
{
  if (!CanSeek()) return;
  if (m_currentFile.m_lStartOffset)
    iTime += m_currentFile.m_lStartOffset * 1000 / 75;
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
    CLog::Log(LOGDEBUG, "Seek to time %f took %lu ms", 0.001f * m_SeekTime, timeGetTime() - time);
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
      int nErr = snd_pcm_drain(m_pStream[stream]);
      CHECK_ALSA(LOGERROR,"flush-drain",nErr); 
      nErr = snd_pcm_prepare(m_pStream[stream]);
      CHECK_ALSA(LOGERROR,"flush-prepare",nErr); 
      nErr = snd_pcm_start(m_pStream[stream]);
      CHECK_ALSA(LOGERROR,"flush-start",nErr); 
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
      time += m_currentFile.m_lStartOffset * 1000 / 75;
      m_timeOffset = m_decoder[m_currentDecoder].Seek(time);
      m_bytesSentOut = 0;
      FlushStreams();
      SetVolume(g_stSettings.m_nVolumeLevel - VOLUME_FFWD_MUTE); // override xbmc mute 
    }
    else if (time < 0)
    { // ...disable seeking and start the track again
      time = m_currentFile.m_lStartOffset * 1000 / 75;
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
      CLog::Log(LOGDEBUG, "PAP Player: End of track reached while seeking");
      return false;
    }
  }
  return true;
}

void PAPlayer::SetStreamVolume(int stream, long nVolume)
{
  CLog::Log(LOGDEBUG,"PAPlayer::SetStreamVolume - stream %d, volume: %lu", stream, nVolume);
  m_amp[stream].SetVolume(nVolume);
}

bool PAPlayer::AddPacketsToStream(int stream, CAudioDecoder &dec)
{

  if (!m_pStream[stream] || dec.GetStatus() == STATUS_NO_FILE)
    return false;

    bool ret = false;

    int nAvail = snd_pcm_frames_to_bytes(m_pStream[stream], snd_pcm_avail_update(m_pStream[stream]));
    if (nAvail < PACKET_SIZE) {
        return false;
    }

    if (m_resampler[stream].GetData(m_packet[stream][0].packet))
    {
      	// got some data from our resampler - construct audio packet
      	m_packet[stream][0].length = PACKET_SIZE;
      	m_packet[stream][0].stream = stream;

	unsigned char *pcmPtr = m_packet[stream][0].packet;
	
	// handle volume de-amp 
	m_amp[stream].DeAmplify((short *)pcmPtr, m_packet[stream][0].length / 2);
	
    StreamCallback(&m_packet[stream][0]);
   
	while ( pcmPtr < m_packet[stream][0].packet + m_packet[stream][0].length) {
		int nPeriodSize = snd_pcm_frames_to_bytes(m_pStream[stream],m_periods[stream]);
		if ( pcmPtr + nPeriodSize >  m_packet[stream][0].packet + m_packet[stream][0].length) {
			nPeriodSize = m_packet[stream][0].packet + m_packet[stream][0].length - pcmPtr; 
		}
			
		int framesToWrite = snd_pcm_bytes_to_frames(m_pStream[stream],nPeriodSize);
		int writeResult = snd_pcm_writei(m_pStream[stream], pcmPtr, framesToWrite);
		if (  writeResult == -EPIPE  ) {
			CLog::Log(LOGDEBUG, "PAPlayer::AddPacketsToStream - buffer underun (tried to write %d frames)", 
			framesToWrite);
			int err = snd_pcm_prepare(m_pStream[stream]);
				CHECK_ALSA(LOGERROR,"prepare after EPIPE", err); 
		}
		else if (writeResult != framesToWrite) { 
			CLog::Log(LOGERROR, "PAPlayer::AddPacketsToStream - failed to write %d frames. "
			"bad write (err: %d) - %s", 
				framesToWrite, writeResult, snd_strerror(writeResult));				
			break;
		}
		//else
      	//		m_bytesSentOut += nPeriodSize; 

		pcmPtr += nPeriodSize;  	
	}
		
      // something done
      ret = true;
    }
    else
    { // resampler wants more data - let's feed it
      int amount = m_resampler[stream].GetInputSamples();
      if (amount > 0 && amount <= (int)dec.GetDataSize())
      {
        // needs some data - let's feed it
        m_resampler[stream].PutFloatData((float *)dec.GetData(amount), amount);
        ret = true;
      }
    }

  return ret;
}

bool PAPlayer::FindFreePacket( int stream, DWORD* pdwPacket )
{
  return true;
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
    m_pCallback->OnAudioData((BYTE*)m_visBuffer, m_visBufferLength);
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
    memcpy((short*)m_visBuffer, pkt->packet, pkt->length);
    m_visBufferLength = pkt->length;
  }
}

void CALLBACK StaticStreamCallback( VOID* pStreamContext, VOID* pPacketContext, DWORD dwStatus )
{
  PAPlayer* pPlayer = (PAPlayer*)pStreamContext;
  pPlayer->StreamCallback(pPacketContext);
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
    snd_pcm_wait(m_pStream[m_currentStream], -1);
  }
}

