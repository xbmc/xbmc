/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ReversiblePlayback.h"

#include "ServiceBroker.h"
#include "XBDateTime.h"
#include "addons/AddonVersion.h"
#include "cores/RetroPlayer/guibridge/GUIGameMessenger.h"
#include "cores/RetroPlayer/rendering/RPRenderManager.h"
#include "cores/RetroPlayer/savestates/ISavestate.h"
#include "cores/RetroPlayer/savestates/SavestateDatabase.h"
#include "cores/RetroPlayer/streams/memory/DeltaPairMemoryStream.h"
#include "filesystem/File.h"
#include "games/AchievementRuntime.h"
#include "games/GameServices.h"
#include "games/GameSettings.h"
#include "games/addons/GameClient.h"
#include "games/addons/disc/GameClientDiscModel.h"
#include "games/addons/disc/GameClientDiscs.h"
#include "utils/MathUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

#include <algorithm>
#include <cstring>
#include <memory>
#include <mutex>
#include <vector>

using namespace KODI;
using namespace RETRO;
using GAME::RestoreResult;

#define REWIND_FACTOR 0.25 // Rewind at 25% of gameplay speed

CReversiblePlayback::CReversiblePlayback(GAME::CGameClient* gameClient,
                                         CRPRenderManager& renderManager,
                                         CGUIGameMessenger& guiMessenger,
                                         double fps,
                                         size_t serializeSize)
  : m_gameClient(gameClient),
    m_renderManager(renderManager),
    m_guiMessenger(guiMessenger),
    m_gameLoop(this, fps),
    m_savestateDatabase(new CSavestateDatabase)
{
  UpdateMemoryStream();

  GAME::CGameSettings& gameSettings = CServiceBroker::GetGameServices().GameSettings();
  gameSettings.RegisterObserver(this);
}

CReversiblePlayback::~CReversiblePlayback()
{
  GAME::CGameSettings& gameSettings = CServiceBroker::GetGameServices().GameSettings();
  gameSettings.UnregisterObserver(this);

  Deinitialize();
}

void CReversiblePlayback::Initialize()
{
  UpdateMemoryStream();
  m_gameLoop.Start();
}

void CReversiblePlayback::Deinitialize()
{
  // Wait for autosave tasks
  for (std::future<void>& task : m_savestateThreads)
    task.wait();
  m_savestateThreads.clear();

  m_gameLoop.Stop();

  std::unique_lock lock(m_mutex);
  m_memoryStream.reset();
  m_discStateHistory.Clear();
}

void CReversiblePlayback::SeekTimeMs(unsigned int timeMs)
{
  std::unique_lock lock(m_mutex);
  if (m_restoreFailed)
    return;

  const double previousSpeed = m_gameLoop.GetSpeed();
  const int offsetTimeMs = timeMs - GetTimeMs();
  const int offsetFrames = MathUtils::round_int(offsetTimeMs / 1000.0 * m_gameLoop.FPS());

  if (offsetFrames > 0)
  {
    const uint64_t frames = std::min(static_cast<uint64_t>(offsetFrames), m_futureFrameCount);
    if (frames > 0)
    {
      m_gameLoop.SetSpeed(0.0);
      if (AdvanceFrames(frames) != RestoreResult::StateUncertain)
        m_gameLoop.SetSpeed(previousSpeed);
    }
  }
  else if (offsetFrames < 0)
  {
    const uint64_t frames = std::min(static_cast<uint64_t>(-offsetFrames), m_pastFrameCount);
    if (frames > 0)
    {
      m_gameLoop.SetSpeed(0.0);
      if (RewindFrames(frames) != RestoreResult::StateUncertain)
        m_gameLoop.SetSpeed(previousSpeed);
    }
  }
}

double CReversiblePlayback::GetSpeed() const
{
  return m_gameLoop.GetSpeed();
}

void CReversiblePlayback::SetSpeed(double speedFactor)
{
  std::unique_lock lock(m_mutex);
  if (speedFactor != 0.0)
  {
    if (m_restoreFailed)
      return;
  }

  if (speedFactor >= 0.0)
    m_gameLoop.SetSpeed(speedFactor);
  else
    m_gameLoop.SetSpeed(speedFactor * REWIND_FACTOR);
}

void CReversiblePlayback::PauseAsync()
{
  m_gameLoop.PauseAsync();
}

std::string CReversiblePlayback::CreateSavestate(bool autosave,
                                                 const std::string& savestatePath /* = "" */)
{
  {
    std::unique_lock lock(m_mutex);
    if (m_restoreFailed)
      return "";
  }

  const size_t memorySize = m_gameClient->SerializeSize();

  // Game client must support serialization
  if (memorySize == 0)
    return "";

  //! @todo Handle savestates for standalone game clients
  if (m_gameClient->GetGamePath().empty())
  {
    return "";
  }

  // Take a timestamp of the system clock
  const CDateTime nowUTC = CDateTime::GetUTCDateTime();

  // Get the savestate path
  std::string savePath(savestatePath);
  {
    std::unique_lock lock(m_savestateMutex);

    if (autosave && savePath.empty())
      savePath = m_autosavePath;

    // Clear autosave path so the next autosave is created in a new slot and
    // does not overwrite the newly-created manual save
    if (!autosave && savePath == m_autosavePath)
      m_autosavePath.clear();

    // If path is still unknown, calculate it now
    if (savePath.empty())
      savePath = CSavestateDatabase::MakeSavestatePath(m_gameClient->GetGamePath(), nowUTC);

    // Update autosave path
    if (autosave)
      m_autosavePath = savePath;
  }

  // Capture the current video frame
  m_renderManager.CacheVideoFrame(savePath);

  {
    std::unique_lock lock(m_savestateMutex);

    // Prune any finished autosave threads
    m_savestateThreads.erase(std::remove_if(m_savestateThreads.begin(), m_savestateThreads.end(),
                                            [](std::future<void>& task) {
                                              return task.wait_for(std::chrono::seconds(0)) ==
                                                     std::future_status::ready;
                                            }),
                             m_savestateThreads.end());

    // Save async to not block game loop
    std::future<void> task = std::async(std::launch::async, [this, autosave, savePath, nowUTC]()
                                        { CommitSavestate(autosave, savePath, nowUTC); });

    m_savestateThreads.emplace_back(std::move(task));
  }

  return savePath;
}

void CReversiblePlayback::CommitSavestate(bool autosave,
                                          const std::string& savePath,
                                          const CDateTime& nowUTC)
{
  std::unique_ptr<ISavestate> savestate = CSavestateDatabase::AllocateSavestate();
  std::unique_ptr<ISavestate> loadedSavestate;

  const size_t memorySize = m_gameClient->SerializeSize();
  uint8_t* const memoryData = savestate->GetMemoryBuffer(memorySize);

  // Separate from the emulator's memory; see savestate.fbs
  std::vector<uint8_t> achievementState;

  uint64_t timestampFrames;
  // Lock order: playback, then client. The client lock also guards entire disc-model mutations.
  {
    std::unique_lock lock(m_mutex);
    std::unique_lock clientLock = m_gameClient->LockForSnapshot();

    if (m_restoreFailed)
      return;

    std::optional<GAME::GameClientDiscState> discState;
    if (m_rewindFrameRendered)
    {
      // Rewind runs a preview frame; save the cursor's machine, media and time together.
      if (!m_memoryStream || !m_memoryStream->CurrentFrame() ||
          m_memoryStream->FrameSize() != memorySize)
      {
        CLog::Log(LOGERROR, "RetroPlayer[SAVE]: Rewind frame is unavailable for capture");
        return;
      }
      const uint32_t discStateId = m_memoryStream->GetDiscStateID();
      const auto* discModel = m_discStateHistory.Get(discStateId);
      if (discStateId != 0 && !discModel)
      {
        CLog::Log(LOGERROR, "RetroPlayer[SAVE]: Missing rewind disc state {}", discStateId);
        return;
      }
      if (discModel)
      {
        if (!(m_gameClient->Discs().GetDiscsForSnapshot() == *discModel))
        {
          CLog::Log(LOGERROR,
                    "RetroPlayer[SAVE]: Rewind cursor media changed before capture; save rejected");
          return;
        }
        discState = discModel->GetState();
      }
      std::memcpy(memoryData, m_memoryStream->CurrentFrame(), memorySize);
      timestampFrames = m_memoryStream->GetFrameCounter();
      // Rewind history has no matching achievement snapshot; loading will reset that runtime.
    }
    else
    {
      if (!m_gameClient->Serialize(memoryData, memorySize))
        return;
      if (m_gameClient->SupportsDiscControl())
      {
        m_gameClient->Discs().RefreshDiscStateLive();
        discState = m_gameClient->Discs().GetDiscsForSnapshot().GetState();
      }
      m_gameClient->SerializeAchievementState(achievementState);
      timestampFrames = m_totalFrameCount;
    }
    if (discState)
    {
      savestate->SetDiscState(*discState);
      CLog::Log(LOGDEBUG, "RetroPlayer[SAVE]: Captured disc state: slots={} selected={} ejected={}",
                discState->slots.size(), discState->selectedSlot, discState->trayEjected);
    }
  }

  if (!achievementState.empty())
  {
    if (uint8_t* const achievementData = savestate->GetAchievementBuffer(achievementState.size()))
      std::memcpy(achievementData, achievementState.data(), achievementState.size());
  }

  // Attempt to get existing properties
  {
    std::unique_lock lock(m_savestateMutex);
    if (!savePath.empty() && XFILE::CFile::Exists(savePath))
    {
      loadedSavestate = CSavestateDatabase::AllocateSavestate();
      if (!m_savestateDatabase->GetSavestate(savePath, *loadedSavestate))
        loadedSavestate.reset();
    }
  }

  const std::string caption =
      CServiceBroker::GetGameServices().AchievementRuntime().GetRichPresence();
  const std::string gameFileName = URIUtils::GetFileName(m_gameClient->GetGamePath());
  const double timestampWallClock =
      (timestampFrames /
       m_gameClient->GetFrameRate()); //! @todo Accumulate playtime instead of deriving it
  const std::string gameClientId = m_gameClient->ID();
  const std::string gameClientVersion = m_gameClient->Version().asString();

  savestate->SetType(autosave ? SAVE_TYPE::AUTO : SAVE_TYPE::MANUAL);
  savestate->SetLabel(loadedSavestate ? loadedSavestate->Label() : "");
  savestate->SetCaption(caption);
  savestate->SetCreated(nowUTC);
  savestate->SetGameFileName(gameFileName);
  savestate->SetTimestampFrames(timestampFrames);
  savestate->SetTimestampWallClock(timestampWallClock);
  savestate->SetGameClientID(gameClientId);
  savestate->SetGameClientVersion(gameClientVersion);

  m_renderManager.SaveVideoFrame(savePath, *savestate);

  savestate->Finalize();

  bool success;
  {
    std::unique_lock lock(m_savestateMutex);
    success = m_savestateDatabase->AddSavestate(savePath, m_gameClient->GetGamePath(), *savestate);
  }

  if (success)
  {
    std::string thumbnailPath = CSavestateDatabase::MakeThumbnailPath(savePath);
    m_renderManager.SaveThumbnail(thumbnailPath);
  }

  // Notify the GUI that the metadata for this savestate should be refreshed
  m_guiMessenger.RefreshSavestates(savePath, savestate.get());
}

bool CReversiblePlayback::LoadSavestate(const std::string& savestatePath)
{
  const size_t memorySize = m_gameClient->SerializeSize();

  // Game client must support serialization
  if (memorySize == 0)
    return false;

  bool bSuccess = false;

  std::unique_ptr<ISavestate> savestate = CSavestateDatabase::AllocateSavestate();
  if (m_savestateDatabase->GetSavestate(savestatePath, *savestate))
  {
    if (!savestate->PrepareMemoryData(memorySize))
    {
      CLog::Log(LOGERROR, "RetroPlayer[SAVE]: Failed to prepare memory data");
    }
    else if (savestate->GetMemorySize() != memorySize)
    {
      CLog::Log(LOGERROR, "Invalid memory size, got {}, expected {}", savestate->GetMemorySize(),
                memorySize);
    }
    else
    {
      std::unique_lock lock(m_mutex);
      std::unique_lock clientLock = m_gameClient->LockForSnapshot();
      std::optional<GAME::CGameClientDiscModel> discModel;
      if (const auto discState = savestate->GetDiscState())
      {
        discModel.emplace();
        if (!m_gameClient->SupportsDiscControl() ||
            !m_gameClient->Discs().GetDiscsForSnapshot().ResolveState(*discState, *discModel))
        {
          CLog::Log(LOGERROR, "RetroPlayer[SAVE]: Failed to resolve saved disc state");
          return false;
        }
        if (!m_gameClient->Discs().IsMediaSupported(*discModel))
        {
          CLog::Log(LOGERROR, "RetroPlayer[SAVE]: Saved media is unsupported by the active core");
          return false;
        }
        CLog::Log(LOGDEBUG,
                  "RetroPlayer[SAVE]: Restoring disc state: slots={} selected={} ejected={}",
                  discState->slots.size(), discState->selectedSlot, discState->trayEjected);
      }

      const RestoreResult result = m_gameClient->Deserialize(savestate->GetMemoryData(), memorySize,
                                                             discModel ? &*discModel : nullptr);
      if (result == RestoreResult::Restored)
      {
        // After the emulator, so the runtime matches its machine state, and
        // unconditionally: a savestate written before this existed, or while
        // signed out, carries none, but the client still has to be told the
        // machine state jumped. Left untold, the progress it holds for the
        // timeline being abandoned would survive the restore.
        const uint8_t* const achievementData = savestate->GetAchievementData();
        const size_t achievementSize = savestate->GetAchievementSize();

        if (!m_gameClient->DeserializeAchievements(achievementData, achievementSize) &&
            achievementData != nullptr && achievementSize != 0)
        {
          // State the runtime would not take, from another runtime version or
          // a damaged file. Ask for a reset rather than leaving it: what it
          // still holds describes the timeline just abandoned, and carrying
          // that forward is how an achievement gets awarded unearned. The
          // savestate itself is fine, so the load is not failed for it.
          CLog::Log(LOGWARNING, "RetroPlayer[SAVE]: Achievement state refused, resetting runtime");

          m_gameClient->DeserializeAchievements(nullptr, 0);
        }

        if (m_memoryStream)
        {
          const uint64_t maxFrames = m_memoryStream->MaxFrameCount();
          m_memoryStream->Init(memorySize, maxFrames);
          m_discStateHistory.Clear();
          std::memcpy(m_memoryStream->BeginFrame(), savestate->GetMemoryData(), memorySize);
          const uint32_t discStateId =
              m_gameClient->SupportsDiscControl()
                  ? m_discStateHistory.Intern(m_gameClient->Discs().GetDiscsForSnapshot())
                  : 0;
          m_memoryStream->SubmitFrame(discStateId, savestate->TimestampFrames());
          UpdatePlaybackStats();
        }
        m_totalFrameCount = savestate->TimestampFrames();
        m_restoreFailed = false;
        m_rewindFrameRendered = false;
        bSuccess = true;
        if (savestate->Type() == SAVE_TYPE::AUTO)
        {
          std::unique_lock savestateLock(m_savestateMutex);
          m_autosavePath = savestatePath;
        }
      }
      else if (result == RestoreResult::StateUncertain)
      {
        LatchRestoreFailure();
      }
    }
  }

  return bSuccess;
}

void CReversiblePlayback::FrameEvent()
{
  std::unique_lock lock(m_mutex);
  if (!m_restoreFailed && !m_rewindFrameRendered)
  {
    // Input scanning calls back into the client from another thread.
    lock.unlock();
    m_gameClient->PollInput();
    lock.lock();
  }
  std::unique_lock clientLock = m_gameClient->LockForSnapshot();

  if (m_restoreFailed)
    return;

  // The rewind preview has already run and updated the frame rate.
  if (!m_rewindFrameRendered)
  {
    m_gameClient->RunFrame(false);
    UpdateFrameRate();
  }

  AddFrame();
}

void CReversiblePlayback::RewindEvent()
{
  m_gameClient->PollInput();

  std::unique_lock lock(m_mutex);
  std::unique_lock clientLock = m_gameClient->LockForSnapshot();

  if (m_restoreFailed || RewindFrames(1) != RestoreResult::Restored)
    return;

  m_gameClient->RunFrame(false);
  m_rewindFrameRendered = true;
  UpdateFrameRate();
}

void CReversiblePlayback::EndEvent()
{
  m_renderManager.DestroyContext();
}

void CReversiblePlayback::AddFrame()
{
  std::unique_lock lock(m_mutex);

  if (m_memoryStream)
  {
    if (m_gameClient->Serialize(m_memoryStream->BeginFrame(), m_memoryStream->FrameSize()))
    {
      uint32_t discStateId = 0;
      if (m_gameClient->SupportsDiscControl())
      {
        m_gameClient->Discs().RefreshDiscStateLive();
        discStateId = m_discStateHistory.Intern(m_gameClient->Discs().GetDiscsForSnapshot());
      }
      m_memoryStream->SubmitFrame(discStateId, m_totalFrameCount + 1);
      UpdatePlaybackStats();
    }
  }

  m_totalFrameCount++;
  m_rewindFrameRendered = false;
}

void CReversiblePlayback::UpdateFrameRate()
{
  const double previousFrameRate = m_gameLoop.FPS();
  m_gameLoop.SetFrameRate(m_gameClient->GetFrameRate());

  if (m_gameLoop.FPS() != previousFrameRate)
    UpdateMemoryStream();
}

RestoreResult CReversiblePlayback::RewindFrames(uint64_t frames)
{
  std::unique_lock lock(m_mutex);
  std::unique_lock clientLock = m_gameClient->LockForSnapshot();

  RestoreResult result = RestoreResult::Rejected;
  if (m_memoryStream)
  {
    const uint64_t rewound = m_memoryStream->RewindFrames(frames);
    if (rewound > 0)
    {
      const RestoreResult targetResult = RestoreFrame();
      if (targetResult == RestoreResult::Restored)
      {
        m_totalFrameCount = m_memoryStream->GetFrameCounter();
        UpdatePlaybackStats();
        return RestoreResult::Restored;
      }
      else
      {
        const uint64_t rolledBack = m_memoryStream->AdvanceFrames(rewound);
        if (rolledBack != rewound || (targetResult == RestoreResult::StateUncertain &&
                                      RestoreFrame() != RestoreResult::Restored))
        {
          result = RestoreResult::StateUncertain;
          LatchRestoreFailure();
        }
      }
    }
    UpdatePlaybackStats();
  }

  return result;
}

RestoreResult CReversiblePlayback::AdvanceFrames(uint64_t frames)
{
  std::unique_lock lock(m_mutex);
  std::unique_lock clientLock = m_gameClient->LockForSnapshot();

  RestoreResult result = RestoreResult::Rejected;
  if (m_memoryStream)
  {
    const uint64_t advanced = m_memoryStream->AdvanceFrames(frames);
    if (advanced > 0)
    {
      const RestoreResult targetResult = RestoreFrame();
      if (targetResult == RestoreResult::Restored)
      {
        m_totalFrameCount = m_memoryStream->GetFrameCounter();
        UpdatePlaybackStats();
        return RestoreResult::Restored;
      }
      else
      {
        const uint64_t rolledBack = m_memoryStream->RewindFrames(advanced);
        if (rolledBack != advanced || (targetResult == RestoreResult::StateUncertain &&
                                       RestoreFrame() != RestoreResult::Restored))
        {
          result = RestoreResult::StateUncertain;
          LatchRestoreFailure();
        }
      }
    }
    UpdatePlaybackStats();
  }

  return result;
}

RestoreResult CReversiblePlayback::RestoreFrame()
{
  const uint32_t discStateId = m_memoryStream->GetDiscStateID();
  const auto* discModel = m_discStateHistory.Get(discStateId);
  if (discStateId != 0 && !discModel)
  {
    CLog::Log(LOGERROR, "RetroPlayer[DISC]: Missing rewind disc state {}", discStateId);
    return RestoreResult::Rejected;
  }
  if (discModel && !(m_gameClient->Discs().GetDiscsForSnapshot() == *discModel))
  {
    const auto selected = discModel->GetSelectedDiscIndex();
    CLog::Log(LOGDEBUG,
              "RetroPlayer[DISC]: Restoring rewind disc state {}: slots={} selected={} ejected={}",
              discStateId, discModel->Size(), selected ? static_cast<int64_t>(*selected) : -1,
              discModel->IsEjected());
  }
  const RestoreResult result = m_gameClient->Deserialize(m_memoryStream->CurrentFrame(),
                                                         m_memoryStream->FrameSize(), discModel);
  if (result == RestoreResult::Restored)
    m_rewindFrameRendered = false;
  return result;
}

void CReversiblePlayback::LatchRestoreFailure()
{
  if (!m_restoreFailed)
    CLog::Log(LOGERROR, "RetroPlayer[SAVE]: Machine state restore failed, pausing playback");
  m_restoreFailed = true;
  m_gameLoop.PauseAsync();
}

void CReversiblePlayback::UpdatePlaybackStats()
{
  m_pastFrameCount = m_memoryStream->PastFramesAvailable();
  m_futureFrameCount = m_memoryStream->FutureFramesAvailable();

  const uint64_t played = m_pastFrameCount + (m_memoryStream->CurrentFrame() ? 1 : 0);
  const uint64_t total = m_memoryStream->MaxFrameCount();
  const uint64_t cached = m_futureFrameCount;

  m_playTimeMs = MathUtils::round_int(1000.0 * played / m_gameLoop.FPS());
  m_totalTimeMs = MathUtils::round_int(1000.0 * total / m_gameLoop.FPS());
  m_cacheTimeMs = MathUtils::round_int(1000.0 * cached / m_gameLoop.FPS());
}

void CReversiblePlayback::Notify(const Observable& obs, const ObservableMessage msg)
{
  switch (msg)
  {
    case ObservableMessageSettingsChanged:
      UpdateMemoryStream();
      break;
    default:
      break;
  }
}

void CReversiblePlayback::UpdateMemoryStream()
{
  std::unique_lock lock(m_mutex);

  bool bRewindEnabled = false;

  GAME::CGameSettings& gameSettings = CServiceBroker::GetGameServices().GameSettings();

  if (m_gameClient->SerializeSize() > 0)
    bRewindEnabled = gameSettings.RewindEnabled();

  if (bRewindEnabled)
  {
    unsigned int rewindBufferSec = gameSettings.MaxRewindTimeSec();
    if (rewindBufferSec < 10)
      rewindBufferSec = 10; // Sanity check

    unsigned int frameCount = MathUtils::round_int(rewindBufferSec * m_gameLoop.FPS());

    if (!m_memoryStream)
    {
      const size_t memorySize = m_gameClient->SerializeSize();

      // Ceiling, not the real cost: the buffer keeps xor deltas of changed
      // words only. Worth logging because a large state and a long window put
      // that ceiling in the gigabytes.
      CLog::Log(LOGINFO,
                "RetroPlayer[SAVE]: Rewind buffer: {} frames of up to {} bytes ({:.1f} MB "
                "worst case) for {} seconds at {:.2f} fps",
                frameCount, memorySize,
                static_cast<double>(memorySize) * frameCount / (1024.0 * 1024.0), rewindBufferSec,
                m_gameLoop.FPS());

      m_memoryStream = std::make_unique<CDeltaPairMemoryStream>();
      m_memoryStream->Init(memorySize, frameCount);
    }

    if (m_memoryStream->MaxFrameCount() != frameCount)
    {
      m_memoryStream->SetMaxFrameCount(frameCount);
    }
  }
  else
  {
    m_memoryStream.reset();
    m_discStateHistory.Clear();

    // Reset playback stats
    m_pastFrameCount = 0;
    m_futureFrameCount = 0;
    m_playTimeMs = 0;
    m_totalTimeMs = 0;
    m_cacheTimeMs = 0;
  }
}
