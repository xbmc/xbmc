/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "jobs/JobQueue.h"

#include <memory>
#include <string>

namespace KODI
{
namespace RETRO
{
class CGUIGameMessenger;
class ISavestate;
struct SavestateThumbnailPayload;
struct SavestateWriteRequest;
struct RefreshState;
struct WriteTracker;

class CSavestateWriteQueue
{
public:
  explicit CSavestateWriteQueue(CGUIGameMessenger& guiMessenger);
  ~CSavestateWriteQueue();

  void QueueSavestateWrite(SavestateWriteRequest request);
  void Wait();

private:
  bool ArmWrite();
  bool QueueSavestateFileWrite(std::string savePath,
                               std::string gamePath,
                               std::unique_ptr<ISavestate> savestate,
                               bool compressSavedGame);
  bool QueueThumbnailWrite(SavestateThumbnailPayload payload);

  CGUIGameMessenger& m_guiMessenger;
  CJobQueue m_fileWriteQueue;
  CJobQueue m_thumbnailWriteQueue;
  std::shared_ptr<RefreshState> m_refreshState;
  std::shared_ptr<WriteTracker> m_tracker;
};
} // namespace RETRO
} // namespace KODI
