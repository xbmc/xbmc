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
#include "guilib/DispResource.h"
#include "threads/SystemClock.h"
#include "utils/logtypes.h"

#include <string>

class PLT_MediaController;
class CGUIDialogBusy;

namespace XbmcThreads { class EndTime; }


namespace UPNP
{

class CUPnPPlayerController;

class CUPnPPlayer
  : public IPlayer, public IRenderLoop
{
public:
  CUPnPPlayer(IPlayerCallback& callback, const char* uuid);
  ~CUPnPPlayer() override;

  bool OpenFile(const CFileItem& file, const CPlayerOptions& options) override;
  bool QueueNextFile(const CFileItem &file) override;
  bool CloseFile(bool reopen = false) override;
  bool IsPlaying() const override;
  void Pause() override;
  bool HasVideo() const override { return false; }
  bool HasAudio() const override { return false; }
  void Seek(bool bPlus, bool bLargeStep, bool bChapterOverride) override;
  void SeekPercentage(float fPercent = 0) override;
  void SetVolume(float volume) override;

  int GetChapterCount() override { return 0; }
  int GetChapter() override { return -1; }
  void GetChapterName(std::string& strChapterName, int chapterIdx = -1) override { }
  int SeekChapter(int iChapter) override { return -1; }

  void SeekTime(int64_t iTime = 0) override;
  void SetSpeed(float speed = 0) override;

  bool IsCaching() const override { return false; }
  int GetCacheLevel() const override { return -1; }
  void DoAudioWork() override;
  bool OnAction(const CAction &action) override;

  void FrameMove() override;

  int PlayFile(const CFileItem& file, const CPlayerOptions& options, CGUIDialogBusy*& dialog, XbmcThreads::EndTime& timeout);

private:
  bool IsPaused() const;
  int64_t GetTime();
  int64_t GetTotalTime();
  float GetPercentage();

  PLT_MediaController* m_control;
  CUPnPPlayerController* m_delegate;
  std::string m_current_uri;
  std::string m_current_meta;
  bool m_started;
  bool m_stopremote;
  XbmcThreads::EndTime m_updateTimer;

  Logger m_logger;
};

} /* namespace UPNP */
