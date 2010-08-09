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

#include "PAPlayer.h"
#include "CodecFactory.h"
#include "../../utils/GUIInfoManager.h"
#include "AudioContext.h"
#include "../../FileSystem/FileShoutcast.h"
#include "../../Application.h"
#include "FileItem.h"
#include "AdvancedSettings.h"
#include "GUISettings.h"
#include "Settings.h"
#include "MusicInfoTag.h"
#include "../AudioRenderers/AudioRendererFactory.h"
#include "../../utils/TimeUtils.h"
#include "utils/log.h"

#include "utils/SingleLock.h"

#define TIME_TO_CACHE_NEXT_FILE 5000L         // 5 seconds
#define TIME_TO_CROSS_FADE      10000L        // 10 seconds

using namespace std;
extern XFILE::CFileShoutcast* m_pShoutCastRipper;

// PAP: Psycho-acoustic Audio Player
// Supporting all open  audio codec standards.
// First one being nullsoft's nsv audio decoder format

PAPlayer::PAPlayer(IPlayerCallback& callback) :
  IPlayer        (callback),
  m_audioCallback(NULL    ),
  m_isPaused     (false   )
{
  m_crossFade = g_guiSettings.GetInt("musicplayer.crossfade");

#if 0
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

  for (int i=0; i<2; i++)
  {
    m_channelCount[i]   = 0;
    m_channelMap[i]     = NULL;
    m_sampleRate[i]     = 0;
    m_bitsPerSample[i]  = 0;

    m_pAudioStream[i] = NULL;
    m_pcmBuffer[i] = NULL;
    m_bufferPos[i] = 0;
    m_Chunklen[i]  = PACKET_SIZE;
  }

  m_currentStream = 0;
  m_packet[0][0].packet = NULL;
  m_packet[1][0].packet = NULL;

  m_bytesSentOut = 0;
  m_BytesPerSecond = 0;

  m_visBufferLength = 0;
  m_pCallback = NULL;

  m_forceFadeToNext = false;
  m_CacheLevel = 0;
  m_LastCacheLevelCheck = 0;

  m_currentFile = new CFileItem;
  m_nextFile = new CFileItem;
#endif
}

PAPlayer::~PAPlayer()
{
  CSingleLock lock(m_critSection);
  while(!m_streams.empty())
    FreeStreamInfo(m_streams.front());

#if 0
  CloseFileInternal(true);
  delete m_currentFile;
  delete m_nextFile;
#endif
}

void PAPlayer::OnExit()
{

}

void PAPlayer::FreeStreamInfo(StreamInfo *si)
{
  m_streams.remove(si);
  delete si->m_stream;
  si->m_decoder.Destroy();
  delete si;
}

bool PAPlayer::OpenFile(const CFileItem& file, const CPlayerOptions &options)
{
  return QueueNextFile(file);
}

void PAPlayer::StaticStreamOnData(CAEStream *sender, void *arg)
{
  StreamInfo *si = (StreamInfo*)arg;
  CSingleLock lock(si->m_player->m_critSection);

  while(si->m_decoder.GetDataSize() == 0)
  {
    int status = si->m_decoder.GetStatus();
    if (status == STATUS_ENDED || status == STATUS_NO_FILE || si->m_decoder.ReadSamples(PACKET_SIZE) == RET_ERROR)
    {
      if (!si->m_triggered)
        si->m_player->m_callback.OnQueueNextItem();
      si->m_stream->Drain();
      return;
    }
  }

  unsigned int frames = std::min(si->m_decoder.GetDataSize(), sender->GetChannelCount());
  void *data = si->m_decoder.GetData(frames);
  si->m_stream->AddData(data, frames * sizeof(float));
  si->m_sent += frames;

  if (si->m_change > 0 && !si->m_triggered && si->m_sent >= si->m_change)
  {
    si->m_player->m_callback.OnQueueNextItem();
    si->m_triggered = true;
  }
}

void PAPlayer::StaticStreamOnDrain(CAEStream *sender, void *arg)
{
  StreamInfo *si = (StreamInfo*)arg;
  CSingleLock lock(si->m_player->m_critSection);

  /* we dont delete the stream as it is flagged to be deleted on drain completion */
  si->m_player->m_streams.remove(si);
  si->m_decoder.Destroy();
  delete si;
}

void PAPlayer::StaticFadeOnDone(CAEPPAnimationFade *sender, void *arg)
{
  StreamInfo *si = (StreamInfo*)arg;
  CSingleLock lock(si->m_player->m_critSection);
  si->m_stream->Drain();
}

#if 0
  if (m_currentlyCrossFading) CloseFileInternal(false); //user seems to be in a hurry

  UpdateCrossFadingTime(file);
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

  if (!m_decoder[m_currentDecoder].Create(file, (__int64)(options.starttime * 1000)))
    return false;

  m_iSpeed = 1;
  m_bPaused = false;
  m_bStopPlaying = false;
  m_bytesSentOut = 0;

  CLog::Log(LOGINFO, "PAPlayer: Playing %s", file.m_strPath.c_str());

  m_timeOffset = (__int64)(options.starttime * 1000);

  unsigned int channel, sampleRate, bitsPerSample;
  m_decoder[m_currentDecoder].GetDataFormat(&channel, &sampleRate, &bitsPerSample);

  if (!CreateStream(m_currentStream, channel, sampleRate, bitsPerSample))
  {
    m_decoder[m_currentDecoder].Destroy();
    CLog::Log(LOGERROR, "PAPlayer::Unable to create audio stream");
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
#endif

#if 0
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
#endif

void PAPlayer::OnNothingToQueueNotify()
{
/*
  //nothing to queue, stop playing
  m_bQueueFailed = true;
*/
}

bool PAPlayer::QueueNextFile(const CFileItem &file)
{
  StreamInfo *si = new StreamInfo();
  if (!si->m_decoder.Create(file, (file.m_lStartOffset * 1000) / 75))
  {
    delete si;
    return false;
  }

  unsigned int channels, sampleRate, bitsPerSample;
  si->m_decoder.GetDataFormat(&channels, &sampleRate, &bitsPerSample);

  si->m_player    = this;
  si->m_fadeIn    = NULL;
  si->m_fadeOut   = NULL;
  si->m_sent      = 0;
  si->m_change    = 0;
  si->m_triggered = false;

  si->m_stream = AE.GetStream(
    AE_FMT_FLOAT,
    sampleRate,
    channels,
    NULL, /* FIXME: channelLayout */
    true, /* free on drain */
    true  /* owns post-proc filters */
  );

  if (!si->m_stream)
    FreeStreamInfo(si);

  /* pause the stream and set the callbacks */
  si->m_stream->Pause();
  si->m_stream->SetDataCallback (StaticStreamOnData , si);
  si->m_stream->SetDrainCallback(StaticStreamOnDrain, si);

  m_isPaused  = false;

  CSingleLock lock(m_critSection);

  list<StreamInfo*>::iterator itt;
  for(itt = m_streams.begin(); itt != m_streams.end(); ++itt)
  {
    StreamInfo *i = *itt;
    if (!m_crossFade)
    {
      i->m_stream->Flush();
      i->m_stream->Drain();
    }
    else
    {
      if (!i->m_fadeOut)
      {
        i->m_fadeOut = new CAEPPAnimationFade(1.0f, 0.0f, m_crossFade * 1000L);
        i->m_fadeOut->SetDoneCallback(StaticFadeOnDone, i);
        i->m_fadeOut->SetPosition(1.0f);
        i->m_stream->PrependPostProc(i->m_fadeOut);
        i->m_fadeOut->Run();
      }
    }
  }

  if (m_crossFade && !m_streams.empty())
  {
    si->m_fadeIn = new CAEPPAnimationFade(0.0f, 1.0f, m_crossFade * 1000L);
    si->m_fadeIn->SetPosition(0.0f);
    si->m_stream->PrependPostProc(si->m_fadeIn);
    si->m_fadeIn->Run();
  }


  si->m_decoder.Start();
  si->m_change = (si->m_decoder.TotalTime() - (m_crossFade * 1000)) * (sampleRate * channels) / 1000.0f;

  m_streams.push_front(si);
  si->m_stream->Resume();
  m_callback.OnPlayBackStarted();
  return true;
}

#if 0
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
  int64_t seekOffset = (file.m_lStartOffset * 1000) / 75;
  if (!m_decoder[decoder].Create(file, seekOffset))
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

  // crossfading - need to create a new stream
  if (m_crossFading && !CreateStream(1 - m_currentStream, channels, samplerate, bitspersample))
  {
    m_decoder[decoder].Destroy();
    CLog::Log(LOGERROR, "PAPlayer::Unable to create audio stream");
  }

  *m_nextFile = file;

  return true;
}
#endif

bool PAPlayer::CloseFileInternal(bool bAudioDevice /*= true*/)
{
  return false;
#if 0
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
    if (bAudioDevice)
      FreeStream(i);
  }

  m_currentFile->Reset();
  m_nextFile->Reset();

  if(bAudioDevice)
    g_audioContext.SetActiveDevice(CAudioContext::DEFAULT_DEVICE);
  else
    FlushStreams();

  return true;
#endif
}

void PAPlayer::Pause()
{
#if 0
  CLog::Log(LOGDEBUG,"PAPlayer: pause m_bplaying: %d", m_bIsPlaying);
  if (!m_bIsPlaying || !m_pAudioStream)
  return ;

  m_bPaused = !m_bPaused;

  if (m_bPaused)
  {
    if (m_pAudioStream[m_currentStream])
      m_pAudioStream[m_currentStream]->Pause();

    if (m_currentlyCrossFading && m_pAudioStream[1 - m_currentStream])
      m_pAudioStream[1 - m_currentStream]->Pause();

    m_callback.OnPlayBackPaused();
    CLog::Log(LOGDEBUG, "PAPlayer: Playback paused");
  }
  else
  {
    if (m_pAudioStream[m_currentStream])
      m_pAudioStream[m_currentStream]->Resume();

    if (m_currentlyCrossFading && m_pAudioStream[1 - m_currentStream])
      m_pAudioStream[1 - m_currentStream]->Resume();

    m_callback.OnPlayBackResumed();
    CLog::Log(LOGDEBUG, "PAPlayer: Playback resumed");
  }
#endif
}

void PAPlayer::SetVolume(float volume)
{
#if 0
  if (m_pAudioStream[m_currentStream])
    m_pAudioStream[m_currentStream]->SetVolume(volume);
#endif
}

void PAPlayer::SetDynamicRangeCompression(long drc)
{
  // TODO: Add volume amplification
  CLog::Log(LOGDEBUG,"PAPlayer::SetDynamicRangeCompression - drc: %lu", drc);
}

void PAPlayer::Process()
{
  while(1)
  {
    sleep(100);
  }
#if 0
  CLog::Log(LOGDEBUG, "PAPlayer: Thread started");
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

    CLog::Log(LOGINFO, "PAPlayer: End of playback reached");
    m_bIsPlaying = false;
    if (!m_bStopPlaying && !m_bStop)
      m_callback.OnPlayBackEnded();
    else
      m_callback.OnPlayBackStopped();
  }
  CLog::Log(LOGDEBUG, "PAPlayer: Thread end");
#endif
}

void PAPlayer::ToFFRW(int iSpeed)
{
#if 0
  m_iSpeed = iSpeed;
  m_callback.OnPlayBackSpeedChanged(iSpeed);
#endif
}

#if 0
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
        if (m_decoder[1 - m_currentDecoder].GetStatus() == STATUS_QUEUED && m_pAudioStream[1 - m_currentStream])
        {
          m_currentlyCrossFading = true;
          m_currentDecoder = 1 - m_currentDecoder;
          m_decoder[m_currentDecoder].Start();
          m_currentStream = 1 - m_currentStream;
          CLog::Log(LOGDEBUG, "Starting Crossfade - resuming stream %i", m_currentStream);

          m_pAudioStream[m_currentStream]->Resume();

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
            if (channels != channels2 || samplerate != samplerate2 || bitspersample != bitspersample2)
            {
              CLog::Log(LOGINFO, "PAPlayer: Stream properties have changed, restarting stream");
              FreeStream(m_currentStream);
              if (!CreateStream(m_currentStream, channels2, samplerate2, bitspersample2))
              {
                CLog::Log(LOGERROR, "PAPlayer: Error creating stream!");
                return false;
              }
              m_pAudioStream[m_currentStream]->Resume();
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

    if (!m_bPaused)
    {

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

      // add packets as necessary
      if (AddPacketsToStream(m_currentStream, m_decoder[m_currentDecoder]))
        retVal = RET_SUCCESS;

      if (retVal == RET_SLEEP && retVal2 == RET_SLEEP)
      {
        float maximumSleepTime = 0; /* m_pAudioStream[m_currentStream]->GetCacheTime(); FIXME */
        
        if (m_pAudioStream[1 - m_currentStream])
          maximumSleepTime = std::min(maximumSleepTime, 0.0f); /*m_pAudioStream[1 - m_currentStream]->GetCacheTime()); FIXME*/

        int sleep = std::max((int)((maximumSleepTime / 2.0f) * 1000.0f), 1);

        Sleep(std::min(sleep, 15));
      }
    }
    else
      Sleep(100);
  }
  return true;
}
#endif

void PAPlayer::ResetTime()
{
  if (m_streams.empty()) return;
  StreamInfo *si = m_streams.front();
  si->m_sent = 0;
}

__int64 PAPlayer::GetTime()
{
  if (m_streams.empty()) return 0;
  StreamInfo *si = m_streams.front();
  return (float)si->m_sent / (float)(si->m_stream->GetSampleRate() * si->m_stream->GetChannelCount()) * 1000.0f;
}

#if 0
__int64 PAPlayer::GetTotalTime64()
{
  __int64 total = m_decoder[m_currentDecoder].TotalTime();
  if (m_currentFile->m_lEndOffset)
    total = m_currentFile->m_lEndOffset * 1000 / 75;
  if (m_currentFile->m_lStartOffset)
    total -= m_currentFile->m_lStartOffset * 1000 / 75;
  return total;
}
#endif

int PAPlayer::GetTotalTime()
{
  if (m_streams.empty()) return 0;
  return m_streams.front()->m_decoder.TotalTime();
}

int PAPlayer::GetCacheLevel() const
{
  if (m_streams.empty()) return 0;
  const ICodec* codec = m_streams.front()->m_decoder.GetCodec();
  if (codec)
    return codec->GetCacheLevel();

  return -1;
}

int PAPlayer::GetChannels()
{
  if (m_streams.empty()) return 0;
  const ICodec* codec = m_streams.front()->m_decoder.GetCodec();
  if (codec)
    return codec->m_Channels;

  return 0;
}

int PAPlayer::GetBitsPerSample()
{
  if (m_streams.empty()) return 0;
  const ICodec* codec = m_streams.front()->m_decoder.GetCodec();
  if (codec)
    return codec->m_BitsPerSample;

  return 0;
}

int PAPlayer::GetSampleRate()
{
  if (m_streams.empty()) return 0;
  const ICodec* codec = m_streams.front()->m_decoder.GetCodec();
  if (codec)
    return (codec->m_SampleRate / 1000) + 0.5;

  return 0;
}

CStdString PAPlayer::GetAudioCodecName()
{
  if (m_streams.empty()) return "";
  const ICodec* codec = m_streams.front()->m_decoder.GetCodec();
  if (codec)
    return codec->m_CodecName;

  return "";
}

int PAPlayer::GetAudioBitrate()
{
  if (m_streams.empty()) return 0;
  const ICodec* codec = m_streams.front()->m_decoder.GetCodec();
  if (codec)
    return (codec->m_Bitrate / 1000) + 0.5;

  return 0;
}

bool PAPlayer::CanSeek()
{
#if 0
  return ((m_decoder[m_currentDecoder].TotalTime() > 0) && m_decoder[m_currentDecoder].CanSeek());
#endif
  return false;
}

void PAPlayer::Seek(bool bPlus, bool bLargeStep)
{
#if 0
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
#endif
}

void PAPlayer::SeekTime(__int64 iTime /*=0*/)
{
#if 0
  if (!CanSeek()) return;
  int seekOffset = (int)(iTime - GetTime());
  if (m_currentFile->m_lStartOffset)
    iTime += m_currentFile->m_lStartOffset * 1000 / 75;
  m_SeekTime = iTime;
  m_callback.OnPlayBackSeek((int)m_SeekTime, seekOffset);
  CLog::Log(LOGDEBUG, "PAPlayer::Seeking to time %f", 0.001f * m_SeekTime);
#endif
}

void PAPlayer::SeekPercentage(float fPercent /*=0*/)
{
#if 0
  if (fPercent < 0.0f) fPercent = 0.0f;
  if (fPercent > 100.0f) fPercent = 100.0f;
  SeekTime((__int64)(fPercent * 0.01f * (float)GetTotalTime64()));
#endif
}

float PAPlayer::GetPercentage()
{
  float percent = (float)GetTime() * 100.0f / GetTotalTime();
  return percent;
}

#if 0
void PAPlayer::HandleSeeking()
{
  if (m_SeekTime != -1)
  {
    DWORD time = CTimeUtils::GetTimeMS();
    m_timeOffset = m_decoder[m_currentDecoder].Seek(m_SeekTime);
    CLog::Log(LOGDEBUG, "Seek to time %f took %i ms", 0.001f * m_SeekTime, CTimeUtils::GetTimeMS() - time);
    FlushStreams();
    m_SeekTime = -1;
  }
  g_infoManager.m_performingSeek = false;
}
#endif

#if 0
void PAPlayer::FlushStreams()
{
  m_bytesSentOut = 0;
  for (int stream = 0; stream < 2; stream++)
  {
    if (m_pAudioStream[stream] && m_packet[stream])
    {
      m_pAudioStream[stream]->Flush();
      m_bufferPos[stream] = 0;
    }
  }
}
#endif

#if 0
bool PAPlayer::HandleFFwdRewd()
{
  if (!m_IsFFwdRewding && m_iSpeed == 1)
    return true;  // nothing to do
  if (m_IsFFwdRewding && m_iSpeed == 1)
  { // stop ffwd/rewd
    m_IsFFwdRewding = false;
    SetVolume(g_settings.m_fVolumeLevel);
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
      FlushStreams();
      SetVolume(g_settings.m_fVolumeLevel - VOLUME_FFWD_MUTE); // override xbmc mute
    }
    else if (time < 0)
    { // ...disable seeking and start the track again
      time = m_currentFile->m_lStartOffset * 1000 / 75;
      m_timeOffset = m_decoder[m_currentDecoder].Seek(time);
      FlushStreams();
      m_iSpeed = 1;
      SetVolume(g_settings.m_fVolumeLevel); // override xbmc mute
    } // is our next position greater then the end sector...
    else //if (time > m_codec->m_TotalTime)
    {
      // restore volume level so the next track isn't muted
      SetVolume(g_settings.m_fVolumeLevel);
      CLog::Log(LOGDEBUG, "PAPlayer: End of track reached while seeking");
      return false;
    }
  }
  return true;
}
#endif

#if 0
void PAPlayer::SetStreamVolume(int stream, long nVolume)
{
  m_pAudioStream[stream]->SetVolume(nVolume);
}
#endif

#if 0
bool PAPlayer::AddPacketsToStream(int stream, CAudioDecoder &dec)
{
  if (!m_pAudioStream[stream] || dec.GetStatus() == STATUS_NO_FILE)
    return false;

  int amount = std::min((unsigned int)OUTPUT_SAMPLES, dec.GetDataSize()); //m_pAudioStream[stream]->GetFrameSamples();
  if (amount == 0) return false;
  //if (amount > dec.GetDataSize()) return false;
  
  void *data = dec.GetData(amount);
  amount *= sizeof(float);
  while(amount > 0)
  {
    int wrote = m_pAudioStream[stream]->AddData(data, amount);

    /* FIXME, this should wait on a signal */
    if (wrote == 0)
    {
      sleep(1);
      continue;
    }

    data    = (uint8_t*)data + wrote;
    amount -= wrote;
  }

  return true;
}
#endif

void PAPlayer::RegisterAudioCallback(IAudioCallback *pCallback)
{
  m_audioCallback = pCallback;
//  if (m_audioCallback)
  //  m_audioCallback->OnInitialize(m_channelCount[m_currentStream], m_sampleRate[m_currentStream], m_bitsPerSample[m_currentStream]);
}

void PAPlayer::UnRegisterAudioCallback()
{
  m_audioCallback = NULL;
}

void PAPlayer::DoAudioWork()
{
#if 0
  if (m_pCallback && m_visBufferLength)
  {
    m_pCallback->OnAudioData((BYTE*)m_visBuffer, m_visBufferLength);
    m_visBufferLength = 0;
  }
#endif
}

#if 0
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
#endif

void CALLBACK StaticStreamCallback( VOID* pStreamContext, VOID* pPacketContext, DWORD dwStatus )
{
#if 0
  PAPlayer* pPlayer = (PAPlayer*)pStreamContext;
  pPlayer->StreamCallback(pPacketContext);
#endif
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
#if 0
  if (m_decoder[m_currentDecoder].GetCodec() && m_decoder[m_currentDecoder].GetCodec()->SkipNext())
  {
    return true;
  }
  return false;
#endif
  return false;
}

bool PAPlayer::CanRecord()
{
#if 0
  if (!m_pShoutCastRipper) return false;
  return m_pShoutCastRipper->CanRecord();
#endif
  return false;
}

bool PAPlayer::IsRecording()
{
#if 0
  if (!m_pShoutCastRipper) return false;
  return m_pShoutCastRipper->IsRecording();
#endif
  return false;
}

bool PAPlayer::Record(bool bOnOff)
{
#if 0
  if (!m_pShoutCastRipper) return false;
  if (bOnOff && IsRecording()) return true;
  if (bOnOff == false && IsRecording() == false) return true;
  if (bOnOff)
    return m_pShoutCastRipper->Record();

  m_pShoutCastRipper->StopRecording();
  return true;
#endif
  return false;
}

#if 0
void PAPlayer::WaitForStream()
{
  // should we wait for our other stream as well?
  // currently we don't.
  if (m_pAudioStream[m_currentStream])
  {
    m_pAudioStream[m_currentStream]->Drain();
  }
}
#endif
