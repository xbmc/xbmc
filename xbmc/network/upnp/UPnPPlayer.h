/*
 *  Copyright (c) 2006 elupus (Joakim Plate)
 *  Copyright (C) 2006-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/IPlayer.h"
#include "threads/SystemClock.h"
#include "threads/Thread.h"
#include "utils/logtypes.h"

#include <memory>
#include <string>
#include <string_view>
#include <utility>

class PLT_MediaController;

namespace UPNP
{

class CUPnPPlayerController;

//! \brief Tracks whether a transport state reported by a renderer ends the file Kodi asked it for.
//!
//! Starting a file on a renderer that is already playing stops it first, so a state of STOPPED is
//! the end of playback only once it is known to belong to the file being watched.
class CPlaybackState
{
public:
  //! \brief An open is in flight, so the renderer still reports states belonging to the last file.
  void Opening() { m_started = false; }

  //! \brief The renderer is playing the file that was opened.
  void Started() { m_started = true; }

  //! \brief Stops watching, returning whether playback was being watched.
  bool Finish() { return std::exchange(m_started, false); }

  bool IsStarted() const { return m_started; }

  //! \brief Whether \a transportState ends the file being watched.
  bool HasEnded(std::string_view transportState) const
  {
    return m_started && transportState == "STOPPED";
  }

private:
  bool m_started{false};
};

class CUPnPPlayer : public IPlayer, public CThread
{
public:
  CUPnPPlayer(IPlayerCallback& callback, const char* uuid);
  ~CUPnPPlayer() override;

  bool OpenFile(const CFileItem& file, const CPlayerOptions& options) override;
  bool QueueNextFile(const CFileItem &file) override;
  bool CloseFile(bool reopen = false) override;
  bool IsPlaying() const override;
  void Pause() override;
  bool HasVideo() const override { return m_hasVideo; }
  bool HasAudio() const override { return m_hasAudio; }
  void Seek(bool bPlus, bool bLargeStep, bool bChapterOverride) override;
  void SeekPercentage(float fPercent = 0) override;
  void SetVolume(float volume) override;

  int SeekChapter(int iChapter) override { return -1; }

  void SeekTime(int64_t iTime = 0) override;
  void SetSpeed(float speed = 0) override;

  bool IsCaching() const override { return false; }
  int GetCacheLevel() const override { return -1; }
  bool OnAction(const CAction &action) override;

  int PlayFile(const CFileItem& file,
               const CPlayerOptions& options,
               XbmcThreads::EndTime<>& timeout);

private:
  bool IsPaused() const;
  int64_t GetTime();
  int64_t GetTotalTime();
  float GetPercentage();

  // implementation of CThread
  void Process() override;
  void OnExit() override;

  PLT_MediaController* m_control = nullptr;
  std::unique_ptr<CUPnPPlayerController> m_delegate;
  CPlaybackState m_playback;
  bool m_stopremote = false;
  bool m_hasVideo{false};
  bool m_hasAudio{false};
  XbmcThreads::EndTime<> m_updateTimer;

  Logger m_logger;
};

} /* namespace UPNP */
