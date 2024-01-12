/*
 *  Copyright (c) 2006 elupus (Joakim Plate)
 *  Copyright (C) 2006-2018 Team Kodi
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

class PLT_MediaController;

namespace UPNP
{

class CUPnPPlayerController;

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
  std::string m_current_uri;
  std::string m_current_meta;
  bool m_started = false;
  bool m_stopremote = false;
  bool m_hasVideo{false};
  bool m_hasAudio{false};
  XbmcThreads::EndTime<> m_updateTimer;

  Logger m_logger;
};

} /* namespace UPNP */
