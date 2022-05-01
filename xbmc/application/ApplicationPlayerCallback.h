/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "FileItem.h"
#include "cores/IPlayerCallback.h"
#include "threads/Event.h"

class CApplicationPlayer;
class CApplicationStackHelper;

class CApplicationPlayerCallback : public IPlayerCallback
{
public:
  CApplicationPlayerCallback(CApplicationPlayer& appPlayer, CApplicationStackHelper& stackHelper);

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
  void StoreVideoSettings(const CFileItem& fileItem, CVideoSettings vs) override;

protected:
  CApplicationPlayer& m_appPlayer; //!< Reference to application player
  CApplicationStackHelper& m_stackHelper; //!< Reference to application stack helper
  CFileItemPtr m_itemCurrentFile; //!< Currently playing file
  CEvent m_playerEvent;
};
