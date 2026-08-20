/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PAPlayer.h"

#include "FileItem.h"
#include "ICodec.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "Util.h"
#include "cores/AudioEngine/Interfaces/AE.h"
#include "cores/AudioEngine/Interfaces/AEStream.h"
#include "cores/AudioEngine/Utils/AEStreamData.h"
#include "cores/AudioEngine/Utils/AEUtil.h"
#include "cores/DataCacheCore.h"
#include "cores/VideoPlayer/Process/ProcessInfo.h"
#include "jobs/JobManager.h"
#include "messaging/ApplicationMessenger.h"
#include "music/MusicFileItemClassify.h"
#include "music/tags/MusicInfoTag.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "threads/SystemClock.h"
#include "utils/log.h"
#include "video/Bookmark.h"

#include <memory>
#include <mutex>

using namespace KODI;
using namespace std::chrono_literals;

#define TIME_TO_CACHE_NEXT_FILE 5000 /* 5 seconds before end of song, start caching the next song */
#define FAST_XFADE_TIME           80 /* 80 milliseconds */
#define MAX_SKIP_XFADE_TIME     2000 /* max 2 seconds crossfade on track skip */

// PAP: Psycho-acoustic Audio Player
// Supporting all open  audio codec standards.
// First one being nullsoft's nsv audio decoder format

PAPlayer::PAPlayer(IPlayerCallback& callback)
  : IPlayer(callback), CThread("PAPlayer"), m_playbackSpeed(1), m_audioCallback(NULL)
{
  memset(&m_playerGUIData, 0, sizeof(m_playerGUIData));
  m_processInfo.reset(CProcessInfo::CreateInstance());
  m_processInfo->SetDataCache(&CServiceBroker::GetDataCacheCore());
}

PAPlayer::~PAPlayer()
{
  CloseFile();
}

void PAPlayer::SoftStart(bool wait/* = false */)
{
  std::unique_lock lock(m_streamsLock);
  for(StreamList::iterator itt = m_streams.begin(); itt != m_streams.end(); ++itt)
  {
    StreamInfo* si = *itt;
    if (si->m_fadeOutTriggered)
      continue;

    si->m_stream->Resume();
    si->m_stream->FadeVolume(0.0f, 1.0f, FAST_XFADE_TIME);
  }

  if (wait)
  {
    /* wait for them to fade in */
    lock.unlock();
    CThread::Sleep(std::chrono::milliseconds(FAST_XFADE_TIME));
    lock.lock();

    /* be sure they have faded in */
    while(wait)
    {
      wait = false;
      for(StreamList::iterator itt = m_streams.begin(); itt != m_streams.end(); ++itt)
      {
        StreamInfo* si = *itt;
        if (si->m_stream->IsFading())
        {
          lock.unlock();
          wait = true;
          CThread::Sleep(1ms);
          lock.lock();
          break;
        }
      }
    }
  }
}

void PAPlayer::SoftStop(bool wait/* = false */, bool close/* = true */)
{
  /* fade all the streams out fast for a nice soft stop */
  std::unique_lock lock(m_streamsLock);
  for(StreamList::iterator itt = m_streams.begin(); itt != m_streams.end(); ++itt)
  {
    StreamInfo* si = *itt;
    if (si->m_stream)
      si->m_stream->FadeVolume(1.0f, 0.0f, FAST_XFADE_TIME);

    if (close)
    {
      si->m_prepareTriggered  = true;
      si->m_playNextTriggered = true;
      si->m_fadeOutTriggered  = true;
    }
  }

  /* if we are going to wait for them to finish fading */
  if(wait)
  {
    // fail safe timer, do not wait longer than 1000ms
    XbmcThreads::EndTime<> timer(1000ms);

    /* wait for them to fade out */
    lock.unlock();
    CThread::Sleep(std::chrono::milliseconds(FAST_XFADE_TIME));
    lock.lock();

    /* be sure they have faded out */
    while(wait && !CServiceBroker::GetActiveAE()->IsSuspended() && !timer.IsTimePast())
    {
      wait = false;
      for(StreamList::iterator itt = m_streams.begin(); itt != m_streams.end(); ++itt)
      {
        StreamInfo* si = *itt;
        if (si->m_stream && si->m_stream->IsFading())
        {
          lock.unlock();
          wait = true;
          CThread::Sleep(1ms);
          lock.lock();
          break;
        }
      }
    }

    /* if we are not closing the streams, pause them */
    if (!close)
    {
      for(StreamList::iterator itt = m_streams.begin(); itt != m_streams.end(); ++itt)
      {
        StreamInfo* si = *itt;
        si->m_stream->Pause();
      }
    }
  }
}

void PAPlayer::CloseAllStreams(bool fade/* = true */)
{
  if (!fade)
  {
    std::unique_lock lock(m_streamsLock);
    while (!m_streams.empty())
    {
      StreamInfo* si = m_streams.front();
      m_streams.pop_front();

      if (si->m_stream)
      {
        CloseFileCB(*si);
        si->m_stream.reset();
      }

      si->m_decoder.Destroy();
      delete si;
    }

    while (!m_finishing.empty())
    {
      StreamInfo* si = m_finishing.front();
      m_finishing.pop_front();

      if (si->m_stream)
      {
        CloseFileCB(*si);
        si->m_stream.reset();
      }

      si->m_decoder.Destroy();
      delete si;
    }
    m_currentStream = nullptr;
  }
  else
  {
    SoftStop(false, true);
    std::unique_lock lock(m_streamsLock);
    m_currentStream = NULL;
  }
}

bool PAPlayer::OpenFile(const CFileItem& file, const CPlayerOptions &options)
{
  m_defaultCrossfadeMS = CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(CSettings::SETTING_MUSICPLAYER_CROSSFADE) * 1000;
  m_fullScreen = options.fullscreen;

  // a request left over from the previous file does not apply to this one
  m_requestedAudioStream = -1;

  if (m_streams.size() > 1 || !m_defaultCrossfadeMS || m_isPaused)
  {
    CloseAllStreams(!m_isPaused);
    StopThread();
    m_isPaused = false; // Make sure to reset the pause state
  }

  {
    std::unique_lock lock(m_streamsLock);
    // A new playback request; the queue is no longer exhausted. Nothing else clears this, so
    // a reused player would otherwise carry it into the next song.
    m_isFinished = false;
    m_jobCounter++;
  }
  CServiceBroker::GetJobManager()->Submit([=, this]() { QueueNextFileEx(file, false); }, this,
                                          CJob::PRIORITY_NORMAL);

  std::unique_lock lock(m_streamsLock);
  if (m_streams.size() == 2)
  {
    //do a short crossfade on trackskip, set to max 2 seconds for these prev/next transitions
    m_upcomingCrossfadeMS = std::min(m_defaultCrossfadeMS, (unsigned int)MAX_SKIP_XFADE_TIME);

    //start transition to next track
    StreamInfo* si = m_streams.front();
    si->m_playNextAtFrame  = si->m_framesSent; //start next track at current frame
    si->m_prepareTriggered = true; //next track is ready to go
  }
  lock.unlock();

  if (!IsRunning())
    Create();

  /* trigger playback start */
  m_isPlaying = true;
  m_startEvent.Set();

  // OnPlayBackStarted to be made only once. Callback processing may be slower than player process
  // so clear signal flag first otherwise async stream processing could also make callback
  m_signalStarted = false;
  m_callback.OnPlayBackStarted(file);

  return true;
}

void PAPlayer::UpdateCrossfadeTime(const CFileItem& file)
{
  // we explicitly disable crossfading for audio cds
  if (MUSIC::IsCDDA(file))
    m_upcomingCrossfadeMS = 0;
  else
    m_upcomingCrossfadeMS = m_defaultCrossfadeMS = CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(CSettings::SETTING_MUSICPLAYER_CROSSFADE) * 1000;

  if (m_upcomingCrossfadeMS)
  {
    if (!m_currentStream ||
        (file.HasMusicInfoTag() &&
         !CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
             CSettings::SETTING_MUSICPLAYER_CROSSFADEALBUMTRACKS) &&
         m_currentStream->m_fileItem->HasMusicInfoTag() &&
         (!m_currentStream->m_fileItem->GetMusicInfoTag()->GetAlbum().empty()) &&
         (m_currentStream->m_fileItem->GetMusicInfoTag()->GetAlbum() ==
          file.GetMusicInfoTag()->GetAlbum()) &&
         (m_currentStream->m_fileItem->GetMusicInfoTag()->GetDiscNumber() ==
          file.GetMusicInfoTag()->GetDiscNumber()) &&
         (m_currentStream->m_fileItem->GetMusicInfoTag()->GetTrackNumber() ==
          file.GetMusicInfoTag()->GetTrackNumber() - 1)))
    {
      //do not crossfade when playing consecutive albumtracks
      m_upcomingCrossfadeMS = 0;
    }
  }
}

bool PAPlayer::QueueNextFile(const CFileItem &file)
{
  {
    std::unique_lock lock(m_streamsLock);
    m_jobCounter++;
  }
  CServiceBroker::GetJobManager()->Submit([this, file]() { QueueNextFileEx(file, true); }, this,
                                          CJob::PRIORITY_NORMAL);

  return true;
}

bool PAPlayer::QueueNextFileEx(const CFileItem &file, bool fadeIn)
{
  if (m_currentStream)
  {
    // check if we advance a track of a CUE sheet
    // if this is the case we don't need to open a new stream
    std::string newURL = file.GetDynURL().GetFileName();
    std::string oldURL = m_currentStream->m_fileItem->GetDynURL().GetFileName();
    if (newURL.compare(oldURL) == 0 && file.GetStartOffset() &&
        file.GetStartOffset() == m_currentStream->m_fileItem->GetEndOffset() && m_currentStream &&
        m_currentStream->m_prepareTriggered)
    {
      m_currentStream->m_nextFileItem = std::make_unique<CFileItem>(file);
      m_upcomingCrossfadeMS = 0;
      return true;
    }
    m_currentStream->m_nextFileItem.reset();
  }

  StreamInfo *si = new StreamInfo();
  si->m_fileItem = std::make_unique<CFileItem>(file);

  // Start stream at zero offset
  si->m_startOffset = 0;
  //File item start offset defines where in song to resume
  double starttime = CUtil::ConvertMilliSecsToSecs(si->m_fileItem->GetStartOffset());

  // Music from cuesheet => "item_start" and offset match
  // Start offset defines where this song starts in file of multiple songs
  if (si->m_fileItem->HasProperty("item_start") &&
      (si->m_fileItem->GetProperty("item_start").asInteger() == si->m_fileItem->GetStartOffset()))
  {
    // Start stream at offset from cuesheet
    si->m_startOffset = si->m_fileItem->GetStartOffset();
    starttime = 0; // No resume point
  }

  // Carry the listener's choice into the next track - with continuous playback through an album
  // whose files are laid out alike, dropping back to the first stream every track is not what was
  // asked for. A track without that stream falls back to its first one.
  if (!si->m_decoder.Create(file, si->m_startOffset, m_preferredAudioStream))
  {
    CLog::Log(LOGWARNING, "PAPlayer::QueueNextFileEx - Failed to create the decoder");

    // advance playlist
    AdvancePlaylistOnError(*si->m_fileItem);
    m_callback.OnQueueNextItem();

    delete si;
    return false;
  }

  /* decode until there is data-available */
  si->m_decoder.Start();
  while (si->m_decoder.GetDataSize(true) == 0)
  {
    int status = si->m_decoder.GetStatus();
    if (status == STATUS_ENDED   ||
        status == STATUS_NO_FILE ||
        si->m_decoder.ReadSamples(PACKET_SIZE) == RET_ERROR)
    {
      CLog::Log(LOGINFO, "PAPlayer::QueueNextFileEx - Error reading samples");

      si->m_decoder.Destroy();
      // advance playlist
      AdvancePlaylistOnError(*si->m_fileItem);
      m_callback.OnQueueNextItem();
      delete si;
      return false;
    }

    /* yield our time so that the main PAP thread doesn't stall */
    CThread::Sleep(1ms);
  }

  // set m_upcomingCrossfadeMS depending on type of file and user settings
  UpdateCrossfadeTime(*si->m_fileItem);

  /* init the streaminfo struct */
  si->m_audioStream = si->m_decoder.GetStreamIndex();
  si->m_audioFormat = si->m_decoder.GetFormat();
  // si->m_startOffset already initialized
  si->m_endOffset = file.GetEndOffset();
  si->m_bytesPerSample = CAEUtil::DataFormatToBits(si->m_audioFormat.m_dataFormat) >> 3;
  si->m_bytesPerFrame = si->m_bytesPerSample * si->m_audioFormat.m_channelLayout.Count();
  si->m_started = false;
  si->m_finishing = false;
  si->m_framesSent = 0;
  si->m_seekNextAtFrame = 0;
  si->m_seekFrame = -1;
  si->m_stream = NULL;
  si->m_volume = (fadeIn && m_upcomingCrossfadeMS) ? 0.0f : 1.0f;
  si->m_fadeOutTriggered = false;
  si->m_isSlaved = false;
  si->m_slavedTo = nullptr;

  si->m_decoderTotal = si->m_decoder.TotalTime();
  int64_t streamTotalTime = si->m_decoderTotal;
  if (si->m_endOffset)
    streamTotalTime = si->m_endOffset - si->m_startOffset;

  // Seek to a resume point
  if (si->m_fileItem->HasProperty("StartPercent") &&
      (si->m_fileItem->GetProperty("StartPercent").asDouble() > 0) &&
      (si->m_fileItem->GetProperty("StartPercent").asDouble() <= 100))
  {
    si->m_seekFrame =
        si->m_audioFormat.m_sampleRate *
        CUtil::ConvertMilliSecsToSecs(static_cast<int>(+(static_cast<double>(
            streamTotalTime * (si->m_fileItem->GetProperty("StartPercent").asDouble() / 100.0)))));
  }
  else if (starttime > 0)
    si->m_seekFrame = si->m_audioFormat.m_sampleRate * starttime;
  else if (si->m_fileItem->HasProperty("audiobook_bookmark"))
    si->m_seekFrame = si->m_audioFormat.m_sampleRate *
                      CUtil::ConvertMilliSecsToSecs(
                          si->m_fileItem->GetProperty("audiobook_bookmark").asInteger());

  si->m_prepareNextAtFrame = 0;
  // cd drives don't really like it to be crossfaded or prepared
  if (!MUSIC::IsCDDA(file))
  {
    if (streamTotalTime >= TIME_TO_CACHE_NEXT_FILE + m_defaultCrossfadeMS)
      si->m_prepareNextAtFrame = (int)((streamTotalTime - TIME_TO_CACHE_NEXT_FILE - m_defaultCrossfadeMS) * si->m_audioFormat.m_sampleRate / 1000.0f);
  }

  if (m_currentStream && ((m_currentStream->m_audioFormat.m_dataFormat == AE_FMT_RAW) || (si->m_audioFormat.m_dataFormat == AE_FMT_RAW)))
  {
    m_currentStream->m_prepareTriggered = false;
    m_currentStream->m_waitOnDrain = true;
    m_currentStream->m_prepareNextAtFrame = 0;
    si->m_decoder.Destroy();
    delete si;
    return false;
  }

  si->m_prepareTriggered = false;
  si->m_playNextAtFrame = 0;
  si->m_playNextTriggered = false;
  si->m_waitOnDrain = false;

  if (!PrepareStream(si))
  {
    CLog::Log(LOGINFO, "PAPlayer::QueueNextFileEx - Error preparing stream");

    si->m_decoder.Destroy();
    // advance playlist
    AdvancePlaylistOnError(*si->m_fileItem);
    m_callback.OnQueueNextItem();
    delete si;
    return false;
  }

  /* add the stream to the list */
  std::unique_lock lock(m_streamsLock);
  m_streams.push_back(si);
  //update the current stream to start playing the next track at the correct frame.
  UpdateStreamInfoPlayNextAtFrame(m_currentStream, m_upcomingCrossfadeMS);

  return true;
}

void PAPlayer::UpdateStreamInfoPlayNextAtFrame(StreamInfo *si, unsigned int crossFadingTime)
{
  // if no crossfading or cue sheet, wait for eof
  if (si && (crossFadingTime || si->m_endOffset))
  {
    int64_t streamTotalTime = si->m_decoder.TotalTime();
    if (si->m_endOffset)
      streamTotalTime = si->m_endOffset - si->m_startOffset;
    if (streamTotalTime < crossFadingTime)
      si->m_playNextAtFrame = (int)((streamTotalTime / 2) * si->m_audioFormat.m_sampleRate / 1000.0f);
    else
      si->m_playNextAtFrame = (int)((streamTotalTime - crossFadingTime) * si->m_audioFormat.m_sampleRate / 1000.0f);
  }
}

inline bool PAPlayer::PrepareStream(StreamInfo *si)
{
  /* if we have a stream we are already prepared */
  if (si->m_stream)
    return true;

  /* get a paused stream */
  AEAudioFormat format = si->m_audioFormat;
  si->m_stream = CServiceBroker::GetActiveAE()->MakeStream(
    format,
    AESTREAM_PAUSED
  );

  if (!si->m_stream)
  {
    CLog::Log(LOGDEBUG, "PAPlayer::PrepareStream - Failed to get IAEStream");
    return false;
  }

  si->m_stream->SetVolume(si->m_volume);
  float peak = 1.0;
  float gain = si->m_decoder.GetReplayGain(peak);
  if (peak * gain <= 1.0f)
    // No clipping protection needed
    si->m_stream->SetReplayGain(gain);
  else if (CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_MUSICPLAYER_REPLAYGAINAVOIDCLIPPING))
    // Normalise volume reducing replaygain to avoid needing clipping protection, plays file at lower level
    si->m_stream->SetReplayGain(1.0f / fabs(peak));
  else
    // Clipping protection (when enabled in AE) by audio limiting, applied just where needed
    si->m_stream->SetAmplification(gain);

  /* if its not the first stream and crossfade is not enabled */
  if (m_currentStream && m_currentStream != si && !m_upcomingCrossfadeMS)
  {
    /* slave the stream for gapless */
    si->m_isSlaved = true;
    si->m_slavedTo = m_currentStream;
    m_currentStream->m_stream->RegisterSlave(si->m_stream.get());
  }

  /* fill the stream's buffer */
  while(si->m_stream->IsBuffering())
  {
    int status = si->m_decoder.GetStatus();
    if (status == STATUS_ENDED   ||
        status == STATUS_NO_FILE ||
        si->m_decoder.ReadSamples(PACKET_SIZE) == RET_ERROR)
    {
      CLog::Log(LOGINFO, "PAPlayer::PrepareStream - Stream Finished");
      break;
    }

    if (!QueueData(si))
      break;

    /* yield our time so that the main PAP thread doesn't stall */
    CThread::Sleep(1ms);
  }

  CLog::Log(LOGINFO, "PAPlayer::PrepareStream - Ready");

  return true;
}

bool PAPlayer::CloseFile(bool reopen)
{
  if (reopen)
    CServiceBroker::GetActiveAE()->KeepConfiguration(3000);

  if (!m_isPaused)
    SoftStop(true, true);
  CloseAllStreams(false);

  /* wait for the thread to terminate */
  StopThread(true);//true - wait for end of thread

  // wait for any pending jobs to complete
  {
    std::unique_lock lock(m_streamsLock);
    while (m_jobCounter > 0)
    {
      lock.unlock();
      m_jobEvent.Wait(100ms);
      lock.lock();
    }
  }
  CServiceBroker::GetDataCacheCore().Reset();
  return true;
}

void PAPlayer::Process()
{
  if (!m_startEvent.Wait(100ms))
  {
    CLog::Log(LOGDEBUG, "PAPlayer::Process - Failed to receive start event");
    return;
  }

  CLog::Log(LOGDEBUG, "PAPlayer::Process - Playback started");
  while(m_isPlaying && !m_bStop)
  {
    /* this needs to happen outside of any locks to prevent deadlocks */
    if (m_signalSpeedChange)
    {
      m_callback.OnPlayBackSpeedChanged(m_playbackSpeed);
      m_signalSpeedChange = false;
    }

    double freeBufferTime = 0.0;
    ProcessStreams(freeBufferTime);

    // if none of our streams wants at least 10ms of data, we sleep
    if (freeBufferTime < 0.01)
    {
      CThread::Sleep(10ms);
    }

    if (m_newForcedPlayerTime != -1)
    {
      if (SetTimeInternal(m_newForcedPlayerTime))
      {
        m_newForcedPlayerTime = -1;
      }
    }

    if (m_newForcedTotalTime != -1)
    {
      if (SetTotalTimeInternal(m_newForcedTotalTime))
      {
        m_newForcedTotalTime = -1;
      }
    }

    GetTimeInternal(); //update for GUI
  }
  m_isPlaying = false;
}

inline void PAPlayer::ProcessStreams(double &freeBufferTime)
{
  std::unique_lock sharedLock(m_streamsLock);
  if (m_isFinished && m_streams.empty() && m_finishing.empty())
  {
    m_isPlaying = false;
    freeBufferTime = 1.0;
    return;
  }

  /* destroy any drained streams */
  for (auto itt = m_finishing.begin(); itt != m_finishing.end();)
  {
    StreamInfo* si = *itt;
    if (si->m_stream->IsDrained())
    {
      itt = m_finishing.erase(itt);
      CloseFileCB(*si);

      // Draining is what hands a stream slaved to this one over: AE has resumed it and dropped the
      // pointer it held, so it drives itself from here and may be rebuilt like any other stream.
      // Nothing else clears this, and while it is set the stream is left alone - both as something
      // to switch and as the slave to re-register on a rebuilt master.
      for (StreamInfo* remaining : m_streams)
      {
        if (remaining->m_slavedTo == si)
        {
          remaining->m_isSlaved = false;
          remaining->m_slavedTo = nullptr;
        }
      }

      delete si;

      CLog::Log(LOGDEBUG, "PAPlayer::ProcessStreams - Stream Freed");
    }
    else
      ++itt;
  }

  sharedLock.unlock();
  std::unique_lock lock(m_streamsLock);

  /* apply any pending audio stream change before we touch the streams again */
  ProcessAudioStreamChange();

  for(StreamList::iterator itt = m_streams.begin(); itt != m_streams.end(); ++itt)
  {
    StreamInfo* si = *itt;
    if (!m_currentStream && !si->m_started)
    {
      m_currentStream = si;
      UpdateGUIData(si); //update for GUI
    }
    /* if the stream is finishing */
    if ((si->m_playNextTriggered && si->m_stream && !si->m_stream->IsFading()) || !ProcessStream(si, freeBufferTime))
    {
      if (!si->m_prepareTriggered)
      {
        if (si->m_waitOnDrain)
        {
          si->m_stream->Drain(true);
          si->m_waitOnDrain = false;
        }
        si->m_prepareTriggered = true;
        m_callback.OnQueueNextItem();
      }

      /* remove the stream */
      itt = m_streams.erase(itt);
      /* if its the current stream */
      if (si == m_currentStream)
      {
        /* if it was the last stream */
        if (itt == m_streams.end())
        {
          /* if it didn't trigger the next queue item */
          if (!si->m_prepareTriggered)
          {
            if (si->m_waitOnDrain)
            {
              si->m_stream->Drain(true);
              si->m_waitOnDrain = false;
            }
            m_callback.OnQueueNextItem();
            si->m_prepareTriggered = true;
          }
          m_currentStream = NULL;
        }
        else
        {
          m_currentStream = *itt;
          UpdateGUIData(*itt); //update for GUI
        }
      }

      /* unregister the audio callback */
      si->m_stream->UnRegisterAudioCallback();
      si->m_decoder.Destroy();
      si->m_stream->Drain(false);
      m_finishing.push_back(si);
      return;
    }

    if (!si->m_started)
      continue;

    // is it time to prepare the next stream?
    if (si->m_prepareNextAtFrame > 0 && !si->m_prepareTriggered && si->m_framesSent >= si->m_prepareNextAtFrame)
    {
      si->m_prepareTriggered = true;
      m_callback.OnQueueNextItem();
    }

    // it is time to start playing the next stream?
    if (si->m_playNextAtFrame > 0 && !si->m_playNextTriggered && !si->m_nextFileItem && si->m_framesSent >= si->m_playNextAtFrame)
    {
      if (!si->m_prepareTriggered)
      {
        si->m_prepareTriggered = true;
        m_callback.OnQueueNextItem();
      }

      if (!m_isFinished)
      {
        if (m_upcomingCrossfadeMS)
        {
          si->m_stream->FadeVolume(1.0f, 0.0f, m_upcomingCrossfadeMS);
          si->m_fadeOutTriggered = true;
        }
        m_currentStream = NULL;

        /* unregister the audio callback */
        si->m_stream->UnRegisterAudioCallback();
      }

      si->m_playNextTriggered = true;
    }
  }
}

inline bool PAPlayer::ProcessStream(StreamInfo *si, double &freeBufferTime)
{
  /* if playback needs to start on this stream, do it */
  if (si == m_currentStream && !si->m_started)
  {
    si->m_started = true;
    si->m_stream->RegisterAudioCallback(m_audioCallback);
    if (!si->m_isSlaved)
      si->m_stream->Resume();
    si->m_stream->FadeVolume(0.0f, 1.0f, m_upcomingCrossfadeMS);
    if (m_signalStarted)
      m_callback.OnPlayBackStarted(*si->m_fileItem);
    m_signalStarted = true;
    if (m_fullScreen)
    {
      CServiceBroker::GetAppMessenger()->PostMsg(TMSG_SWITCHTOFULLSCREEN);
      m_fullScreen = false;
    }
    m_callback.OnAVStarted(*si->m_fileItem);
  }

  /* if we have not started yet and the stream has been primed */
  unsigned int space = si->m_stream->GetSpace();
  if (!si->m_started && !space)
    return true;

  if (!m_playbackSpeed)
    return true;

  /* see if it is time yet to FF/RW or a direct seek */
  if (!si->m_playNextTriggered && ((m_playbackSpeed != 1 && si->m_framesSent >= si->m_seekNextAtFrame) || si->m_seekFrame > -1))
  {
    int64_t time = (int64_t)0;
    /* if its a direct seek */
    if (si->m_seekFrame > -1)
    {
      time = (int64_t)((float)si->m_seekFrame / (float)si->m_audioFormat.m_sampleRate * 1000.0f);
      si->m_framesSent = (int)(si->m_seekFrame - ((float)si->m_startOffset * (float)si->m_audioFormat.m_sampleRate) / 1000.0f);
      si->m_seekFrame  = -1;
      m_playerGUIData.m_time = time; //update for GUI
      si->m_seekNextAtFrame = 0;
      CDataCacheCore::GetInstance().SetPlayTimes(0, time, 0, m_playerGUIData.m_totalTime);
    }
    /* if its FF/RW */
    else
    {
      si->m_framesSent      += si->m_audioFormat.m_sampleRate * (m_playbackSpeed  - 1);
      si->m_seekNextAtFrame  = si->m_framesSent + si->m_audioFormat.m_sampleRate / 2;
      time = (int64_t)(((float)si->m_framesSent / (float)si->m_audioFormat.m_sampleRate * 1000.0f) + (float)si->m_startOffset);
    }

    /* if we are seeking back before the start of the track start normal playback */
    if (time < si->m_startOffset || si->m_framesSent < 0)
    {
      time = si->m_startOffset;
      si->m_framesSent      = 0;
      si->m_seekNextAtFrame = 0;
      SetSpeed(1);
    }

    si->m_decoder.Seek(time);
  }

  int status = si->m_decoder.GetStatus();
  if (status == STATUS_ENDED || status == STATUS_NO_FILE ||
      si->m_decoder.ReadSamples(PACKET_SIZE) == RET_ERROR ||
      ((si->m_endOffset) && (si->m_framesSent >= (si->m_endOffset - si->m_startOffset) *
                                                     si->m_audioFormat.m_sampleRate / 1000)))
  {
    if (si == m_currentStream && si->m_nextFileItem)
    {
      CloseFileCB(*si);

      // update current stream with info of next track
      si->m_startOffset = si->m_nextFileItem->GetStartOffset();
      if (si->m_nextFileItem->GetEndOffset())
        si->m_endOffset = si->m_nextFileItem->GetEndOffset();
      else
        si->m_endOffset = 0;
      si->m_framesSent = 0;

      *si->m_fileItem = *si->m_nextFileItem;
      si->m_nextFileItem.reset();

      int64_t streamTotalTime = si->m_decoder.TotalTime() - si->m_startOffset;
      if (si->m_endOffset)
        streamTotalTime = si->m_endOffset - si->m_startOffset;

      // calculate time when to prepare next stream
      si->m_prepareNextAtFrame = 0;
      if (streamTotalTime >= TIME_TO_CACHE_NEXT_FILE + m_defaultCrossfadeMS)
        si->m_prepareNextAtFrame = (int)((streamTotalTime - TIME_TO_CACHE_NEXT_FILE - m_defaultCrossfadeMS) * si->m_audioFormat.m_sampleRate / 1000.0f);

      si->m_prepareTriggered = false;
      si->m_playNextAtFrame = 0;
      si->m_playNextTriggered = false;
      si->m_seekNextAtFrame = 0;

      //update the current stream to start playing the next track at the correct frame.
      UpdateStreamInfoPlayNextAtFrame(m_currentStream, m_upcomingCrossfadeMS);

      UpdateGUIData(si);
      if (m_signalStarted)
        m_callback.OnPlayBackStarted(*si->m_fileItem);
      m_signalStarted = true;
      m_callback.OnAVStarted(*si->m_fileItem);
    }
    else
    {
      CLog::Log(LOGINFO, "PAPlayer::ProcessStream - Stream Finished");
      return false;
    }
  }

  if (!QueueData(si))
    return false;

  /* update free buffer time if we are running */
  if (si->m_started)
  {
    if (si->m_stream->IsBuffering())
      freeBufferTime = 1.0;
    else
    {
      double free_space;
      if (si->m_audioFormat.m_dataFormat != AE_FMT_RAW)
        free_space = (double)(si->m_stream->GetSpace() / si->m_bytesPerSample) / si->m_audioFormat.m_sampleRate;
      else
      {
        free_space = static_cast<double>(si->m_stream->GetSpace()) *
                     si->m_audioFormat.m_streamInfo.GetDuration() / 1000;
      }

      freeBufferTime = std::max(freeBufferTime , free_space);
    }
  }

  return true;
}

bool PAPlayer::QueueData(StreamInfo *si)
{
  unsigned int space = si->m_stream->GetSpace();

  if (si->m_audioFormat.m_dataFormat != AE_FMT_RAW)
  {
    unsigned int samples = std::min(si->m_decoder.GetDataSize(false), space / si->m_bytesPerSample);
    if (!samples)
      return true;

    // we want complete frames
    samples -= samples % si->m_audioFormat.m_channelLayout.Count();

    uint8_t* data = (uint8_t*)si->m_decoder.GetData(samples);
    if (!data)
    {
      CLog::Log(LOGERROR, "PAPlayer::QueueData - Failed to get data from the decoder");
      return false;
    }

    unsigned int frames = samples/si->m_audioFormat.m_channelLayout.Count();
    unsigned int added = si->m_stream->AddData(&data, 0, frames, nullptr);
    si->m_framesSent += added;
  }
  else
  {
    if (!space)
      return true;

    int size;
    uint8_t *data = si->m_decoder.GetRawData(size);
    if (data && size)
    {
      int added = si->m_stream->AddData(&data, 0, size, nullptr);
      if (added != size)
      {
        CLog::Log(LOGERROR, "PAPlayer::QueueData - unknown error");
        return false;
      }

      si->m_framesSent += si->m_audioFormat.m_streamInfo.GetDuration() / 1000 *
                          si->m_audioFormat.m_streamInfo.m_sampleRate;
    }
  }

  const ICodec* codec = si->m_decoder.GetCodec();
  m_playerGUIData.m_cacheLevel = codec ? codec->GetCacheLevel() : 0; //update for GUI

  return true;
}

void PAPlayer::OnExit()
{
  //@todo signal OnPlayBackError if there was an error on last stream
  if (m_isFinished && !m_bStop)
    m_callback.OnPlayBackEnded();
  else
    m_callback.OnPlayBackStopped();
}

void PAPlayer::OnNothingToQueueNotify()
{
  m_isFinished = true;
}

bool PAPlayer::IsPlaying() const
{
  return m_isPlaying;
}

void PAPlayer::Pause()
{
  if (m_isPaused)
  {
    SetSpeed(1);
  }
  else
  {
    SetSpeed(0);
  }
}

void PAPlayer::SetVolume(float volume)
{

}

void PAPlayer::SetDynamicRangeCompression(long drc)
{

}

void PAPlayer::SetSpeed(float speed)
{
  m_playbackSpeed = static_cast<int>(speed);
  CDataCacheCore::GetInstance().SetSpeed(1.0, speed);
  if (m_playbackSpeed != 0 && m_isPaused)
  {
    m_isPaused = false;
    SoftStart();
    m_callback.OnPlayBackResumed();
  }
  else if (m_playbackSpeed == 0 && !m_isPaused)
  {
    m_isPaused = true;
    SoftStop(true, false);
    m_callback.OnPlayBackPaused();
  }
  m_signalSpeedChange = true;
}

int64_t PAPlayer::GetTimeInternal()
{
  std::unique_lock lock(m_streamsLock);
  if (!m_currentStream)
    return 0;

  double time = ((double)m_currentStream->m_framesSent / (double)m_currentStream->m_audioFormat.m_sampleRate);
  if (m_currentStream->m_stream)
    time -= m_currentStream->m_stream->GetDelay();
  time = time * 1000.0;

  m_playerGUIData.m_time = (int64_t)time; //update for GUI
  CDataCacheCore::GetInstance().SetPlayTimes(0, time, 0, m_playerGUIData.m_totalTime);

  return (int64_t)time;
}

bool PAPlayer::SetTotalTimeInternal(int64_t time)
{
  std::unique_lock lock(m_streamsLock);
  if (!m_currentStream)
  {
    return false;
  }

  m_currentStream->m_decoder.SetTotalTime(time);
  UpdateGUIData(m_currentStream);

  return true;
}

bool PAPlayer::SetTimeInternal(int64_t time)
{
  std::unique_lock lock(m_streamsLock);
  if (!m_currentStream)
    return false;

  m_currentStream->m_framesSent = time / 1000 * m_currentStream->m_audioFormat.m_sampleRate;

  if (m_currentStream->m_stream)
    m_currentStream->m_framesSent += m_currentStream->m_stream->GetDelay() * m_currentStream->m_audioFormat.m_sampleRate;

  return true;
}

void PAPlayer::SetTime(int64_t time)
{
  m_newForcedPlayerTime = time;
}

int64_t PAPlayer::GetTotalTime64()
{
  std::unique_lock lock(m_streamsLock);
  if (!m_currentStream)
    return 0;

  int64_t total = m_currentStream->m_decoder.TotalTime();
  if (m_currentStream->m_endOffset)
    total = m_currentStream->m_endOffset;
  total -= m_currentStream->m_startOffset;
  return total;
}

void PAPlayer::SetTotalTime(int64_t time)
{
  m_newForcedTotalTime = time;
}

int PAPlayer::GetCacheLevel() const
{
  return m_playerGUIData.m_cacheLevel;
}

void PAPlayer::GetAudioStreamInfo(int index, AudioStreamInfo& info) const
{
  std::unique_lock lock(m_streamsLock);

  const StreamInfo* si = PlayingStream();
  if (!si)
    return;

  if (index == CURRENT_STREAM)
    index = si->m_audioStream;

  // Language, name and codec description are only known to the demuxer, so get from decoder
  si->m_decoder.GetStreamInfo(index, info);

  if (index != si->m_audioStream)
    return;

  // For the stream being decoded prefer the values the codec reports
  info.valid = true;
  info.bitrate = m_playerGUIData.m_audioBitrate;
  info.channels = m_playerGUIData.m_channelCount;
  info.codecName = m_playerGUIData.m_codec;
  info.samplerate = m_playerGUIData.m_sampleRate;
  info.bitspersample = m_playerGUIData.m_bitsPerSample;
}

bool PAPlayer::CanSeek() const
{
  return m_playerGUIData.m_canSeek;
}

void PAPlayer::Seek(bool bPlus, bool bLargeStep, bool bChapterOverride)
{
  if (!CanSeek()) return;

  long long seek;
  const std::shared_ptr<CAdvancedSettings> advancedSettings = CServiceBroker::GetSettingsComponent()->GetAdvancedSettings();
  if (advancedSettings->m_musicUseTimeSeeking && m_playerGUIData.m_totalTime > 2 * advancedSettings->m_musicTimeSeekForwardBig)
  {
    if (bLargeStep)
      seek = bPlus ? advancedSettings->m_musicTimeSeekForwardBig : advancedSettings->m_musicTimeSeekBackwardBig;
    else
      seek = bPlus ? advancedSettings->m_musicTimeSeekForward : advancedSettings->m_musicTimeSeekBackward;
    seek *= 1000;
    seek += m_playerGUIData.m_time;
  }
  else
  {
    float percent;
    if (bLargeStep)
      percent = bPlus ? static_cast<float>(advancedSettings->m_musicPercentSeekForwardBig) : static_cast<float>(advancedSettings->m_musicPercentSeekBackwardBig);
    else
      percent = bPlus ? static_cast<float>(advancedSettings->m_musicPercentSeekForward) : static_cast<float>(advancedSettings->m_musicPercentSeekBackward);
    seek = static_cast<long long>(GetTotalTime64() * (GetPercentage() + percent) / 100);
  }

  SeekTime(seek);
}

void PAPlayer::SeekTime(int64_t iTime /*=0*/)
{
  if (!CanSeek()) return;

  std::unique_lock lock(m_streamsLock);
  if (!m_currentStream)
    return;

  int64_t seekOffset = iTime - GetTimeInternal();

  if (m_playbackSpeed != 1)
    SetSpeed(1);

  m_currentStream->m_seekFrame = (int)((float)m_currentStream->m_audioFormat.m_sampleRate * ((float)iTime + (float)m_currentStream->m_startOffset) / 1000.0f);
  m_callback.OnPlayBackSeek(iTime, seekOffset);
}

void PAPlayer::SeekPercentage(float fPercent /*=0*/)
{
  if (fPercent < 0.0f  ) fPercent = 0.0f;
  if (fPercent > 100.0f) fPercent = 100.0f;
  SeekTime((int64_t)(fPercent * 0.01f * (float)GetTotalTime64()));
}

float PAPlayer::GetPercentage()
{
  if (m_playerGUIData.m_totalTime > 0)
    return m_playerGUIData.m_time * 100.0f / m_playerGUIData.m_totalTime;

  return 0.0f;
}

void PAPlayer::UpdateGUIData(StreamInfo *si)
{
  /* Store data need by external threads in member
   * structure to prevent locking conflicts when
   * data required by GUI and main application
   */
  std::unique_lock lock(m_streamsLock);

  m_playerGUIData.m_sampleRate    = si->m_audioFormat.m_sampleRate;
  m_playerGUIData.m_channelCount  = si->m_audioFormat.m_channelLayout.Count();
  m_playerGUIData.m_canSeek       = si->m_decoder.CanSeek();

  const ICodec* codec = si->m_decoder.GetCodec();

  m_playerGUIData.m_audioBitrate = codec ? codec->m_bitRate : 0;
  strncpy(m_playerGUIData.m_codec,codec ? codec->m_CodecName.c_str() : "",20);
  m_playerGUIData.m_cacheLevel   = codec ? codec->GetCacheLevel() : 0;
  m_playerGUIData.m_bitsPerSample = (codec && codec->m_bitsPerCodedSample) ? codec->m_bitsPerCodedSample : si->m_bytesPerSample << 3;

  int64_t total = si->m_decoder.TotalTime();
  if (si->m_endOffset)
    total = m_currentStream->m_endOffset;
  total -= m_currentStream->m_startOffset;
  m_playerGUIData.m_totalTime = total;

  CServiceBroker::GetDataCacheCore().SignalAudioInfoChange();
}

void PAPlayer::OnJobComplete(unsigned int jobID, bool success, CJob *job)
{
  std::unique_lock lock(m_streamsLock);
  m_jobCounter--;
  m_jobEvent.Set();
}

void PAPlayer::CloseFileCB(StreamInfo &si)
{
  IPlayerCallback *cb = &m_callback;
  CFileItem fileItem(*si.m_fileItem);
  CBookmark bookmark;
  double total = si.m_decoderTotal;
  if (si.m_endOffset)
    total = si.m_endOffset;
  total -= si.m_startOffset;
  bookmark.totalTimeInSeconds = total / 1000;
  bookmark.timeInSeconds = (static_cast<double>(si.m_framesSent) /
                            static_cast<double>(si.m_audioFormat.m_sampleRate));

  // A stream that has been given up on has no sink left to account for
  if (si.m_stream)
    bookmark.timeInSeconds -= si.m_stream->GetDelay();
  bookmark.player = m_name;
  bookmark.playerState = GetPlayerState();
  CServiceBroker::GetJobManager()->Submit([=]() { cb->OnPlayerCloseFile(fileItem, bookmark); },
                                          CJob::PRIORITY_NORMAL);
}

void PAPlayer::AdvancePlaylistOnError(CFileItem &fileItem)
{
  if (m_signalStarted)
    m_callback.OnPlayBackStarted(fileItem);
  m_signalStarted = true;
  m_callback.OnAVStarted(fileItem);
}

PAPlayer::StreamInfo* PAPlayer::PlayingStream() const
{
  if (m_currentStream)
    return m_currentStream;

  // Handing over from one track to the next there is no current stream for a moment. The stream
  // taking over is queued by then, and answering from it beats reporting that nothing is playing -
  // which had a request made at that moment rejected as being for a stream that does not exist.
  return m_streams.empty() ? nullptr : m_streams.front();
}

int PAPlayer::GetAudioStreamCount() const
{
  std::unique_lock lock(m_streamsLock);
  const StreamInfo* si = PlayingStream();
  return si ? si->m_decoder.GetStreamCount() : 0;
}

int PAPlayer::GetAudioStream()
{
  std::unique_lock lock(m_streamsLock);
  const StreamInfo* si = PlayingStream();
  return si ? si->m_audioStream : 0;
}

inline bool PAPlayer::RecreateStream(StreamInfo* si)
{
  if (!si)
    return false;

  const bool wasStarted = si->m_started;

  // Anything slaved to si is chained off the IAEStream we are about to throw away, so the link
  // has to be rebuilt against the replacement. The slave is always the stream queued directly
  // after si.
  StreamInfo* slave = nullptr;
  for (auto itt = m_streams.begin(); itt != m_streams.end(); ++itt)
  {
    if (*itt == si)
    {
      if (++itt != m_streams.end() && (*itt)->m_isSlaved && (*itt)->m_stream)
        slave = *itt;
      break;
    }
  }

  // The old stream has to be gone before the replacement is made: AE picks the sink format from
  // the streams it holds, so with both registered it configures for the one being replaced and
  // every switch ends up playing through the previous stream's format. Which leaves si without a
  // stream if MakeStream() then fails, so the caller has to drop it rather than carry on with it.
  if (si->m_stream)
  {
    si->m_stream->UnRegisterAudioCallback();
    si->m_stream.reset();
  }

  AEAudioFormat format = si->m_audioFormat;
  si->m_stream = CServiceBroker::GetActiveAE()->MakeStream(format, AESTREAM_PAUSED);
  if (!si->m_stream)
  {
    CLog::Log(LOGDEBUG, "PAPlayer::RecreateStream - Failed to get IAEStream");
    return false;
  }

  // A stream that is already playing has finished fading in, so m_volume (the level to start
  // at) no longer describes it - it is at full volume.
  si->m_stream->SetVolume(wasStarted ? 1.0f : si->m_volume);

  float peak = 1.0f;
  float gain = si->m_decoder.GetReplayGain(peak);
  if (peak * gain <= 1.0f)
    si->m_stream->SetReplayGain(gain);
  else if (CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
               CSettings::SETTING_MUSICPLAYER_REPLAYGAINAVOIDCLIPPING))
    si->m_stream->SetReplayGain(1.0f / fabs(peak));
  else
    si->m_stream->SetAmplification(gain);

  if (slave)
    si->m_stream->RegisterSlave(slave->m_stream.get());

  // unlike PrepareStream() this runs on the player thread, so it must not outlive a stop
  while (si->m_stream->IsBuffering() && !m_bStop)
  {
    int status = si->m_decoder.GetStatus();
    if (status == STATUS_ENDED || status == STATUS_NO_FILE ||
        si->m_decoder.ReadSamples(PACKET_SIZE) == RET_ERROR)
    {
      CLog::Log(LOGINFO, "PAPlayer::RecreateStream - Stream Finished");
      break;
    }

    if (!QueueData(si))
    {
      // The replacement is in place but was never started, so hand it back to ProcessStream()
      // to take from the top rather than leaving it paused and unregistered.
      si->m_started = false;
      return false;
    }

    CThread::Sleep(1ms);
  }

  if (wasStarted)
  {
    si->m_stream->RegisterAudioCallback(m_audioCallback);

    // The master that would have resumed si is gone along with the old IAEStream, so si now
    // has to drive itself.
    si->m_isSlaved = false;
    si->m_slavedTo = nullptr;

    // Paused playback stays paused - the replacement is made paused, and SoftStart() resumes every
    // stream when playback is resumed. Resuming here would have it playing with the player, and
    // everything it tells the rest of Kodi, still saying it is paused.
    if (!m_isPaused)
      si->m_stream->Resume();
  }

  CLog::Log(LOGINFO, "PAPlayer::RecreateStream - Ready");
  return true;
}

void PAPlayer::SetAudioStream(int iStream)
{
  {
    std::unique_lock lock(m_streamsLock);
    const StreamInfo* si = PlayingStream();
    if (!si || iStream < 0 || iStream >= si->m_decoder.GetStreamCount())
    {
      CLog::Log(LOGERROR, "PAPlayer::SetAudioStream - Invalid audio stream {}", iStream);
      return;
    }

    if (iStream == si->m_audioStream)
      return;
  }

  // The switch tears down and rebuilds the StreamInfo's decoder and IAEStream, both of which
  // the player thread is using continuously. Rather than reach in from here, hand the request
  // over and let ProcessStreams() perform it between its own uses of them.
  m_requestedAudioStream = iStream;
}

void PAPlayer::DropStream(StreamInfo* si)
{
  // Nothing can be done with a StreamInfo that has no IAEStream - playing it and finishing it both
  // go through the stream - so it cannot be left in the list for the player thread to walk into.
  m_streams.remove(si);
  if (m_currentStream == si)
    m_currentStream = nullptr;

  if (!si->m_prepareTriggered)
  {
    si->m_prepareTriggered = true;
    m_callback.OnQueueNextItem();
  }

  // The file is being closed like any other, so it still gets to record where it had reached
  CloseFileCB(*si);

  // Nothing can be waiting on a stream that is going away
  for (StreamInfo* remaining : m_streams)
  {
    if (remaining->m_slavedTo == si)
    {
      remaining->m_isSlaved = false;
      remaining->m_slavedTo = nullptr;
    }
  }

  si->m_decoder.Destroy();
  delete si;
}

void PAPlayer::ProcessAudioStreamChange()
{
  if (m_requestedAudioStream < 0)
  {
    m_audioStreamDeferred = false;
    return;
  }

  // leave the request pending rather than dropping it if there is nothing to apply it to yet
  StreamInfo* si = m_currentStream;
  if (!si)
    return;

  const int iStream = m_requestedAudioStream;
  if (iStream < 0 || iStream == si->m_audioStream)
  {
    m_requestedAudioStream = -1;
    m_audioStreamDeferred = false;
    return;
  }

  if (iStream >= si->m_decoder.GetStreamCount())
  {
    // A request carried over from a track with more streams than this one has
    CLog::Log(LOGDEBUG,
              "PAPlayer::ProcessAudioStreamChange - Dropping request for audio stream "
              "{}, this track has {}",
              iStream, si->m_decoder.GetStreamCount());
    m_requestedAudioStream = -1;
    m_audioStreamDeferred = false;
    return;
  }

  // The end of the track is the same situation: the format probe needs to decode frames from the
  // stream being switched to, and there are none left to read.
  const int status = si->m_decoder.GetStatus();
  if (status == STATUS_ENDING || status == STATUS_ENDED || status == STATUS_NO_FILE)
  {
    if (!m_audioStreamDeferred)
    {
      CLog::Log(LOGINFO,
                "PAPlayer::ProcessAudioStreamChange - Deferring switch to audio stream {}, track "
                "has run out",
                iStream);
      m_audioStreamDeferred = true;
    }
    return;
  }

  // Mid-handover the stream is fading out and its successor is already primed and possibly
  // slaved. Rebuilding it here would leave the crossfade half connected. A stream that is itself
  // still slaved is the other half of that: the predecessor holds its IAEStream as a raw pointer
  // and is what resumes it, so replacing it now would leave that pointer dangling.
  //
  // A stream still fading in is the same handover seen from the other end. The fade belongs to the
  // IAEStream we would be throwing away, so the replacement would come back at full volume against
  // a predecessor that is still fading out - and switching what cannot be heard yet is not what
  // was asked for anyway. It becomes audible once the fade finishes, which is when this applies.
  if (si->m_playNextTriggered || si->m_fadeOutTriggered || si->m_isSlaved ||
      (si->m_stream && si->m_stream->IsFading()))
  {
    // Leave the request pending. The track handing over cannot take it, but the one taking over
    // can - either it was opened on that stream already or this runs again once it is current - so
    // the listener ends up with what they asked for and the notification claimed. This runs on
    // every pass of the player loop, so say so once rather than for as long as the fade lasts.
    if (!m_audioStreamDeferred)
    {
      CLog::Log(LOGINFO,
                "PAPlayer::ProcessAudioStreamChange - Deferring switch to audio stream {}, waiting "
                "for the handover to finish",
                iStream);
      m_audioStreamDeferred = true;
    }
    return;
  }

  m_requestedAudioStream = -1;
  m_audioStreamDeferred = false;

  // Capture what the listener is currently hearing, before changing stream format/state. Like
  // the seeks in ProcessStream() this is in the decoder's time domain, which for a cue sheet
  // track is offset from the frame counter by m_startOffset.
  int64_t currentTime = si->m_startOffset;
  if (si->m_audioFormat.m_sampleRate > 0)
  {
    double time =
        static_cast<double>(si->m_framesSent) / static_cast<double>(si->m_audioFormat.m_sampleRate);
    if (si->m_stream)
      time -= si->m_stream->GetDelay();

    currentTime += static_cast<int64_t>(time * 1000.0);
    currentTime = std::max(currentTime, si->m_startOffset);
  }

  if (!si->m_decoder.SetStream(iStream))
  {
    // A failed switch either left the stream that was playing restored, in which case the track
    // carries on as it was, or left the codec with nothing it can decode - it has given up the old
    // stream and has no format for the new one. Only the latter is fatal, and ending the track
    // through the same path as running out of data beats feeding on an unusable decoder.
    if (si->m_decoder.IsUsable())
    {
      // Probing the stream we asked for, and again the one put back in its place, has moved the
      // demuxer on from where playback is. Seek back so the track carries on where it left off
      // instead of jumping forward.
      si->m_decoder.Seek(currentTime);

      CLog::Log(LOGWARNING,
                "PAPlayer::ProcessAudioStreamChange - Failed to switch to stream {}, carrying on "
                "with stream {}",
                iStream, si->m_audioStream);
      return;
    }

    CLog::Log(LOGERROR,
              "PAPlayer::ProcessAudioStreamChange - Failed to switch to stream {}, ending the "
              "track as its decoder can no longer be used",
              iStream);
    si->m_decoder.Destroy();
    return;
  }

  si->m_audioStream = iStream;
  m_preferredAudioStream = iStream;

  AEAudioFormat newFormat = si->m_decoder.GetFormat();
  si->m_audioFormat = newFormat;
  si->m_bytesPerSample = CAEUtil::DataFormatToBits(newFormat.m_dataFormat) >> 3;
  si->m_bytesPerFrame = si->m_bytesPerSample * newFormat.m_channelLayout.Count();

  // Seek using the captured pre-switch playback time.
  si->m_decoder.Seek(currentTime);

  // Rebase the frame counter onto the new sample rate before priming the new sink. It counts
  // from the start of the track, not the start of the file.
  si->m_framesSent = static_cast<int>(
      ((currentTime - si->m_startOffset) * static_cast<int64_t>(si->m_audioFormat.m_sampleRate)) /
      1000);
  si->m_seekFrame = -1;
  si->m_seekNextAtFrame = 0;

  // Recalculate trigger points for the new sample rate.
  si->m_decoderTotal = si->m_decoder.TotalTime();
  int64_t streamTotalTime = si->m_decoderTotal;
  if (si->m_endOffset)
    streamTotalTime = si->m_endOffset - si->m_startOffset;

  si->m_prepareNextAtFrame = 0;
  if (streamTotalTime >= TIME_TO_CACHE_NEXT_FILE + m_defaultCrossfadeMS)
  {
    si->m_prepareNextAtFrame =
        static_cast<int>((streamTotalTime - TIME_TO_CACHE_NEXT_FILE - m_defaultCrossfadeMS) *
                         si->m_audioFormat.m_sampleRate / 1000.0f);
  }

  UpdateStreamInfoPlayNextAtFrame(si, m_upcomingCrossfadeMS);

  if (!RecreateStream(si) || !si->m_stream)
  {
    CLog::Log(LOGERROR, "PAPlayer::ProcessAudioStreamChange - Failed to recreate IAEStream");
    if (!si->m_stream)
      DropStream(si);
    return;
  }

  UpdateGUIData(si);
  CLog::Log(LOGINFO, "PAPlayer::ProcessAudioStreamChange - Switched to audio stream {}", iStream);
}
