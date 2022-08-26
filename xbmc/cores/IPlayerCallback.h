/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "VideoSettings.h"

#include <stdint.h>

class CFileItem;
class CBookmark;

class IPlayerCallback
{
public:
  virtual ~IPlayerCallback() = default;
  virtual void OnPlayBackEnded() = 0;
  virtual void OnPlayBackStarted(const CFileItem &file) = 0;
  virtual void OnPlayerCloseFile(const CFileItem& file, const CBookmark& bookmark) {}
  virtual void OnPlayBackPaused() {}
  virtual void OnPlayBackResumed() {}
  virtual void OnPlayBackStopped() = 0;
  virtual void OnPlayBackError() = 0;
  virtual void OnQueueNextItem() = 0;
  virtual void OnPlayBackSeek(int64_t iTime, int64_t seekOffset) {}
  virtual void OnPlayBackSeekChapter(int iChapter) {}
  virtual void OnPlayBackSpeedChanged(int iSpeed) {}
  virtual void OnAVChange() {}
  virtual void OnAVStarted(const CFileItem& file) {}
  virtual void RequestVideoSettings(const CFileItem& fileItem) {}
  virtual void StoreVideoSettings(const CFileItem& fileItem, const CVideoSettings& vs) {}
};
