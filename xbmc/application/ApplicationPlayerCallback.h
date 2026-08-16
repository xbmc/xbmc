/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/IPlayerCallback.h"
#include "threads/Event.h"
#include "threads/Timer.h"

#include <memory>

class CApplicationStackHelper;
class CFileItem;

class CApplicationPlayerCallback : public IPlayerCallback
{
public:
  CApplicationPlayerCallback() = default;

  void OnPlayBackEnded() override;
  void OnPlayBackStarted(const CFileItem& file) override;
  void OnPlayerCloseFile(const CFileItem& file, const CBookmark& bookmark) override;
  void OnPlayBackPaused() override;
  void OnPlayBackResumed() override;
  void OnPlayBackStopped() override;
  void OnPlayBackError() override;
  void OnQueueNextItem() override;
  void OnPlayBackSeek(int64_t iTime, int64_t seekOffset) override;
  void OnPlayBackSeekChapter(int iChapter) override;
  void OnPlayBackSpeedChanged(int iSpeed) override;
  void OnAVChange() override;
  void OnAVStarted(const CFileItem& file) override;
  void RequestVideoSettings(const CFileItem& fileItem) override;
  void StoreVideoSettings(const CFileItem& fileItem, const CVideoSettings& vs) override;

private:
  // Periodic progress save timer
  void OnProgressSaveTimer();
  void StartProgressSaveTimer(const CFileItem& file);
  void StopProgressSaveTimer();

  std::unique_ptr<CTimer> m_progressSaveTimer;
  std::shared_ptr<CFileItem> m_currentFileItem;
};
