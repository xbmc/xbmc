/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SavestateWriteQueue.h"

#include "ISavestate.h"
#include "SavestateDatabase.h"
#include "SavestateWriteRequest.h"
#include "ServiceBroker.h"
#include "cores/RetroPlayer/guibridge/GUIGameMessenger.h"
#include "jobs/Job.h"
#include "jobs/JobQueue.h"
#include "messaging/ApplicationMessenger.h"
#include "messaging/ThreadMessage.h"
#include "utils/log.h"

#include <condition_variable>
#include <memory>
#include <mutex>
#include <utility>

using namespace KODI;
using namespace RETRO;

namespace KODI::RETRO
{
struct WriteTracker
{
  std::mutex mutex;
  std::condition_variable condition;
  unsigned int pendingWrites{0};
  bool acceptingWrites{true};
};

// Refresh callbacks are posted asynchronously. Wait() disables the shared
// refresh state before draining workers so callbacks posted before shutdown
// either run safely on the app thread or no-op after shutdown begins.
struct RefreshState
{
  std::mutex mutex;
  CGUIGameMessenger* guiMessenger{nullptr};
  bool acceptingRefreshes{true};
};
} // namespace KODI::RETRO

namespace
{
struct SavestateFileWriteRequest
{
  std::string savePath;
  std::string gamePath;
  std::unique_ptr<ISavestate> savestate;
  bool compressSavedGame{true};
};

class CWriteCompletionGuard
{
public:
  explicit CWriteCompletionGuard(std::shared_ptr<WriteTracker> tracker)
    : m_tracker(std::move(tracker))
  {
  }

  CWriteCompletionGuard(const CWriteCompletionGuard&) = delete;
  CWriteCompletionGuard& operator=(const CWriteCompletionGuard&) = delete;

  ~CWriteCompletionGuard()
  {
    if (!m_tracker)
      return;

    std::unique_lock lock(m_tracker->mutex);
    if (m_tracker->pendingWrites > 0)
      --m_tracker->pendingWrites;
    lock.unlock();
    m_tracker->condition.notify_all();
  }

private:
  std::shared_ptr<WriteTracker> m_tracker;
};

void RefreshSavestatesOnApplicationThread(void* userptr);

struct RefreshSavestatesCallback : KODI::MESSAGING::ThreadMessageCallback
{
  std::shared_ptr<RefreshState> refreshState;
  std::string savePath;
};

void RefreshSavestatesOnApplicationThread(void* userptr)
{
  std::unique_ptr<RefreshSavestatesCallback> callback(
      static_cast<RefreshSavestatesCallback*>(userptr));

  CGUIGameMessenger* guiMessenger = nullptr;
  {
    std::unique_lock lock(callback->refreshState->mutex);
    if (!callback->refreshState->acceptingRefreshes)
      return;

    guiMessenger = callback->refreshState->guiMessenger;
    if (guiMessenger == nullptr)
      return;
  }

  guiMessenger->RefreshSavestates(callback->savePath);
}

void PostSavestateRefresh(std::shared_ptr<RefreshState> refreshState, const std::string& savePath)
{
  {
    std::unique_lock lock(refreshState->mutex);
    if (!refreshState->acceptingRefreshes || refreshState->guiMessenger == nullptr)
      return;
  }

  auto* callback = new RefreshSavestatesCallback;
  callback->callback = RefreshSavestatesOnApplicationThread;
  callback->userptr = callback;
  callback->refreshState = std::move(refreshState);
  callback->savePath = savePath;

  CServiceBroker::GetAppMessenger()->PostMsg(TMSG_CALLBACK, -1, -1, callback);
}

class CSavestateWriteJob : public CJob
{
public:
  CSavestateWriteJob(SavestateFileWriteRequest request,
                     std::shared_ptr<RefreshState> refreshState,
                     std::shared_ptr<WriteTracker> tracker)
    : m_request(std::move(request)),
      m_refreshState(std::move(refreshState)),
      m_completionGuard(std::move(tracker))
  {
  }

  bool DoWork() override
  {
    if (m_request.savePath.empty() || m_request.gamePath.empty() || !m_request.savestate)
      return false;

    m_request.savestate->Finalize(m_request.compressSavedGame);

    CSavestateDatabase database;
    const bool success =
        database.AddSavestate(m_request.savePath, m_request.gamePath, *m_request.savestate);

    if (success)
      PostSavestateRefresh(m_refreshState, m_request.savePath);

    return success;
  }

private:
  SavestateFileWriteRequest m_request;
  std::shared_ptr<RefreshState> m_refreshState;
  CWriteCompletionGuard m_completionGuard;
};

class CSavestateThumbnailWriteJob : public CJob
{
public:
  CSavestateThumbnailWriteJob(SavestateThumbnailPayload payload,
                              std::shared_ptr<WriteTracker> tracker)
    : m_payload(std::move(payload)),
      m_completionGuard(std::move(tracker))
  {
  }

  bool DoWork() override { return WriteSavestateThumbnailPayload(m_payload); }

private:
  SavestateThumbnailPayload m_payload;
  CWriteCompletionGuard m_completionGuard;
};
} // namespace

CSavestateWriteQueue::CSavestateWriteQueue(CGUIGameMessenger& guiMessenger)
  : m_guiMessenger(guiMessenger),
    m_fileWriteQueue(false, 1, CJob::PRIORITY_LOW),
    m_thumbnailWriteQueue(false, 1, CJob::PRIORITY_LOW),
    m_refreshState(std::make_shared<RefreshState>()),
    m_tracker(std::make_shared<WriteTracker>())
{
  m_refreshState->guiMessenger = &m_guiMessenger;
}

CSavestateWriteQueue::~CSavestateWriteQueue()
{
  Wait();
}

void CSavestateWriteQueue::QueueSavestateWrite(SavestateWriteRequest request)
{
  std::optional<SavestateThumbnailPayload> thumbnail = std::move(request.thumbnail);

  if (!QueueSavestateFileWrite(std::move(request.savePath), std::move(request.gamePath),
                               std::move(request.savestate), request.compressSavedGame))
    return;

  if (thumbnail)
  {
    // Thumbnail writes are best-effort. If queueing or writing the thumbnail
    // fails, keep the savestate file.
    (void)QueueThumbnailWrite(std::move(*thumbnail));
  }
}

void CSavestateWriteQueue::Wait()
{
  {
    std::unique_lock refreshLock(m_refreshState->mutex);
    m_refreshState->acceptingRefreshes = false;
    m_refreshState->guiMessenger = nullptr;
  }

  std::unique_lock lock(m_tracker->mutex);
  m_tracker->acceptingWrites = false;
  m_tracker->condition.wait(lock, [this] { return m_tracker->pendingWrites == 0; });
}

bool CSavestateWriteQueue::ArmWrite()
{
  std::unique_lock lock(m_tracker->mutex);
  if (!m_tracker->acceptingWrites)
    return false;

  ++m_tracker->pendingWrites;
  return true;
}

bool CSavestateWriteQueue::QueueSavestateFileWrite(std::string savePath,
                                                   std::string gamePath,
                                                   std::unique_ptr<ISavestate> savestate,
                                                   bool compressSavedGame)
{
  if (!ArmWrite())
    return false;

  SavestateFileWriteRequest request{
      std::move(savePath),
      std::move(gamePath),
      std::move(savestate),
      compressSavedGame,
  };

  auto* job = new CSavestateWriteJob(std::move(request), m_refreshState, m_tracker);

  if (!m_fileWriteQueue.AddJob(job))
  {
    CLog::Log(LOGERROR, "RetroPlayer[SAVE]: Failed to queue savestate write job");
    return false;
  }

  return true;
}

bool CSavestateWriteQueue::QueueThumbnailWrite(SavestateThumbnailPayload payload)
{
  if (!ArmWrite())
    return false;

  auto* job = new CSavestateThumbnailWriteJob(std::move(payload), m_tracker);

  if (!m_thumbnailWriteQueue.AddJob(job))
  {
    CLog::Log(LOGERROR, "RetroPlayer[SAVE]: Failed to queue savestate thumbnail write job");
    return false;
  }

  return true;
}
