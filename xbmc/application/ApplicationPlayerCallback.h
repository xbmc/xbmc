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
  void OnContentGeometryChanged(const LiveGeometryUpdate& update) override;
  void RequestVideoSettings(const CFileItem& fileItem) override;
  void StoreVideoSettings(const CFileItem& fileItem, const CVideoSettings& vs) override;
};

namespace KODI::APPLICATION
{
/*!
 \brief Update the bookmark of a disc stack part closed by the player for the whole stack.
 \param bookmark The player's bookmark, relative to the part
 \param file The part closed
 \param stackHelper The stack being played
 \return false if nothing has been played yet, true otherwise
 */
bool UpdateDiscStackBookmark(CBookmark& bookmark,
                             const CFileItem& file,
                             const std::shared_ptr<CApplicationStackHelper>& stackHelper);

/*!
 \brief Check if a file is part of a disc stack with parts that have no time yet, so the stack's
 total time doesn't cover the whole stack.
 \param file The part
 \param stackHelper The stack being played
 \return true if the stack is partly timed, false otherwise
 */
bool IsPartlyTimedDiscStack(const CFileItem& file,
                            const std::shared_ptr<CApplicationStackHelper>& stackHelper);
} // namespace KODI::APPLICATION
