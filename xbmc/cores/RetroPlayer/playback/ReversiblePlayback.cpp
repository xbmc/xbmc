/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ReversiblePlayback.h"

#include "SavestateCapture.h"
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
    m_savestateDatabase(new CSavestateDatabase),
    m_memorySize(serializeSize),
    m_gamePath(gameClient->GetGamePath()),
    m_gameClientId(gameClient->ID()),
    m_gameClientVersion(gameClient->Version().asString())
{
  if (m_memorySize != 0 && !m_gamePath.empty())
  {
    auto snapshot = std::make_unique<Snapshot>();
    snapshot->memory =
        std::make_unique<uint32_t[]>((m_memorySize + sizeof(uint32_t) - 1) / sizeof(uint32_t));
    m_saveWorker = std::make_unique<CSavestateWorker<Snapshot>>(
        std::move(snapshot),
        [this](Snapshot& captured)
        {
          try
          {
            if (!captured.discarded)
            {
              m_saveSucceeded.store(false);
              m_saveSucceeded.store(CommitSavestate(captured));
            }
          }
          catch (const std::exception& e)
          {
            CLog::Log(LOGERROR, "RetroPlayer[SAVE]: Save failed: {}", e.what());
          }
          catch (...)
          {
            CLog::Log(LOGERROR, "RetroPlayer[SAVE]: Save failed");
          }
          captured.video.clear();
        });
  }
  UpdateMemoryStream();

  GAME::CGameSettings& gameSettings = CServiceBroker::GetGameServices().GameSettings();
  gameSettings.RegisterObserver(this);
}

CReversiblePlayback::~CReversiblePlayback()
{
  GAME::CGameSettings& gameSettings = CServiceBroker::GetGameServices().GameSettings();
  gameSettings.UnregisterObserver(this);

  Deinitialize();
  m_saveWorker.reset();
}

void CReversiblePlayback::Initialize()
{
  m_gameLoop.Start();
}

void CReversiblePlayback::Quiesce()
{
  m_gameLoop.Quiesce();

  std::unique_lock lock(m_mutex);
  CancelAutosave();
  m_autosaveCapture.Reset();
  if (m_saveWorker)
    m_saveWorker->Drain();
}

void CReversiblePlayback::Deinitialize()
{
  Quiesce();
  m_gameLoop.Stop();
}

void CReversiblePlayback::SeekTimeMs(unsigned int timeMs)
{
  const int offsetTimeMs = timeMs - GetTimeMs();
  const int offsetFrames = MathUtils::round_int(offsetTimeMs / 1000.0 * m_gameLoop.FPS());

  if (offsetFrames > 0)
  {
    const uint64_t frames = std::min(static_cast<uint64_t>(offsetFrames), m_futureFrameCount);
    if (frames > 0)
    {
      m_gameLoop.SetSpeed(0.0);
      AdvanceFrames(frames);
      m_gameLoop.SetSpeed(1.0);
    }
  }
  else if (offsetFrames < 0)
  {
    const uint64_t frames = std::min(static_cast<uint64_t>(-offsetFrames), m_pastFrameCount);
    if (frames > 0)
    {
      m_gameLoop.SetSpeed(0.0);
      RewindFrames(frames);
      m_gameLoop.SetSpeed(1.0);
    }
  }
}

double CReversiblePlayback::GetSpeed() const
{
  return m_gameLoop.GetSpeed();
}

void CReversiblePlayback::SetSpeed(double speedFactor)
{
  if (speedFactor >= 0.0)
    m_gameLoop.SetSpeed(speedFactor);
  else
    m_gameLoop.SetSpeed(speedFactor * REWIND_FACTOR);
}

void CReversiblePlayback::PauseAsync()
{
  m_gameLoop.PauseAsync();
}

std::string CReversiblePlayback::GetSavestatePath(bool autosave,
                                                  const std::string& path,
                                                  const CDateTime& created)
{
  std::unique_lock lock(m_savestateMutex);
  std::string savePath = path;
  if (autosave && savePath.empty())
    savePath = m_autosavePath;
  if (!autosave && savePath == m_autosavePath)
    m_autosavePath.clear();
  if (savePath.empty())
    savePath = CSavestateDatabase::MakeSavestatePath(m_gamePath, created);
  if (autosave)
    m_autosavePath = savePath;
  return savePath;
}

std::string CReversiblePlayback::CreateSavestate(bool autosave, const std::string& savestatePath)
{
  if (!m_saveWorker)
  {
    m_saveSucceeded.store(false);
    return "";
  }

  // Explicit saves may wait for storage; the periodic path only uses try-locks.
  std::unique_lock lock(m_mutex);
  CancelAutosave();
  auto snapshot = CaptureSavestate(*m_saveWorker, *m_gameClient, m_memorySize,
                                   [this](Snapshot& captured) { CaptureMetadata(captured); });
  m_saveSucceeded.store(false);
  if (!snapshot)
    return "";
  snapshot->autosave = autosave;
  snapshot->rewind = false;
  snapshot->serializeUs = 0;
  snapshot->captureUs = 0;
  snapshot->path = GetSavestatePath(autosave, savestatePath, snapshot->created);
  const std::string savePath = snapshot->path;
  m_renderManager.CacheVideoFrame(savePath);
  m_saveWorker->Submit(snapshot);
  return savePath;
}

void CReversiblePlayback::CaptureMetadata(Snapshot& snapshot)
{
  snapshot.created = CDateTime::GetUTCDateTime();
  snapshot.frames = m_totalFrameCount;
  snapshot.wallClock = snapshot.frames / m_gameLoop.FPS();
  snapshot.path.clear();
  snapshot.autosave = true;
  snapshot.discarded = false;
  m_gameClient->SerializeAchievementState(snapshot.achievements);
  m_renderManager.TryCaptureVideoFrame(snapshot.video);
}

void CReversiblePlayback::InvalidateAutosave()
{
  m_autosaveCapture.Cancel(m_pendingSnapshot, m_snapshotReady);
}

void CReversiblePlayback::CancelAutosave()
{
  InvalidateAutosave();
  if (m_pendingSnapshot)
    m_saveWorker->Submit(m_pendingSnapshot);
  m_snapshotReady = false;
}

bool CReversiblePlayback::WaitForSavestates()
{
  std::unique_lock lock(m_mutex);
  CancelAutosave();
  if (m_saveWorker && m_saveWorker->Drain())
  {
    m_saveSucceeded.store(false);
    CLog::Log(LOGERROR, "RetroPlayer[SAVE]: Save worker failed");
    return false;
  }
  return m_saveSucceeded.load();
}

bool CReversiblePlayback::CommitSavestate(const Snapshot& snapshot)
{
  const auto started = std::chrono::steady_clock::now();
  const std::string savePath = snapshot.path.empty()
                                   ? GetSavestatePath(snapshot.autosave, "", snapshot.created)
                                   : snapshot.path;
  auto savestate = CSavestateDatabase::AllocateSavestate();
  std::unique_ptr<ISavestate> loadedSavestate;
  uint8_t* memoryData = savestate->GetMemoryBuffer(m_memorySize);
  std::memcpy(memoryData, snapshot.memory.get(), m_memorySize);
  if (!snapshot.achievements.empty())
  {
    if (uint8_t* data = savestate->GetAchievementBuffer(snapshot.achievements.size()))
      std::memcpy(data, snapshot.achievements.data(), snapshot.achievements.size());
  }

  {
    std::unique_lock lock(m_savestateMutex);
    if (XFILE::CFile::Exists(savePath))
    {
      loadedSavestate = CSavestateDatabase::AllocateSavestate();
      if (!m_savestateDatabase->GetSavestate(savePath, *loadedSavestate))
        loadedSavestate.reset();
    }
  }
  savestate->SetType(snapshot.autosave ? SAVE_TYPE::AUTO : SAVE_TYPE::MANUAL);
  savestate->SetLabel(loadedSavestate ? loadedSavestate->Label() : "");
  savestate->SetCaption(CServiceBroker::GetGameServices().AchievementRuntime().GetRichPresence());
  savestate->SetCreated(snapshot.created);
  savestate->SetGameFileName(URIUtils::GetFileName(m_gamePath));
  savestate->SetTimestampFrames(snapshot.frames);
  savestate->SetTimestampWallClock(snapshot.wallClock);
  savestate->SetGameClientID(m_gameClientId);
  savestate->SetGameClientVersion(m_gameClientVersion);

  if (!snapshot.video.empty())
    m_renderManager.CacheVideoFrame(savePath, snapshot.video);
  m_renderManager.SaveVideoFrame(savePath, *savestate, snapshot.video);
  savestate->Finalize();
  bool success;
  {
    std::unique_lock lock(m_savestateMutex);
    success = m_savestateDatabase->AddSavestate(savePath, m_gamePath, *savestate);
  }
  if (success)
    m_renderManager.SaveThumbnail(CSavestateDatabase::MakeThumbnailPath(savePath));
  else
    CLog::Log(LOGERROR, "RetroPlayer[SAVE]: Failed to write savestate");
  m_guiMessenger.RefreshSavestates(savePath, savestate.get());
  CLog::Log(LOGDEBUG,
            "RetroPlayer[SAVE]: Frame {}: reused rewind {}, core {} us, capture/handoff {} us, "
            "background commit {} ms, success {}",
            snapshot.frames, snapshot.rewind, snapshot.serializeUs, snapshot.captureUs,
            std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() -
                                                                  started)
                .count(),
            success);
  return success;
}

bool CReversiblePlayback::LoadSavestate(const std::string& savestatePath)
{
  const size_t memorySize = m_gameClient->SerializeSize();

  // Game client must support serialization
  if (memorySize == 0)
    return false;

  std::unique_lock playbackLock(m_mutex);
  CancelAutosave();
  if (m_saveWorker)
    m_saveWorker->Drain();
  auto clientLock = m_gameClient->LockForSnapshot();
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
      {
        std::unique_lock lock(m_mutex);
        if (m_memoryStream)
        {
          m_memoryStream->SetFrameCounter(savestate->TimestampFrames());
          std::memcpy(m_memoryStream->BeginFrame(), savestate->GetMemoryData(), memorySize);
          m_memoryStream->SubmitFrame();
        }
      }

      if (m_gameClient->Deserialize(savestate->GetMemoryData(), memorySize))
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

        m_totalFrameCount = savestate->TimestampFrames();
        bSuccess = true;
        if (savestate->Type() == SAVE_TYPE::AUTO)
          m_autosavePath = savestatePath;
      }
    }
  }

  return bSuccess;
}

void CReversiblePlayback::FrameEvent()
{
  std::unique_lock lock(m_mutex);
  m_gameClient->RunFrame();
  UpdateFrameRate();

  AddFrame();
}

void CReversiblePlayback::RewindEvent()
{
  std::unique_lock lock(m_mutex);
  InvalidateAutosave();
  RewindFrames(1);

  m_gameClient->RunFrame();
  UpdateFrameRate();
}

void CReversiblePlayback::EndEvent()
{
  m_renderManager.DestroyContext();
}

void CReversiblePlayback::AddFrame()
{
  // Playback lock precedes the client lock for every snapshot and timeline change.
  auto clientLock = m_gameClient->LockForSnapshot();
  int64_t serializeUs = 0;
  bool serialized = false;
  if (m_memoryStream)
  {
    const bool measure = m_autosaveCapture.IsPending();
    const auto started =
        measure ? std::chrono::steady_clock::now() : std::chrono::steady_clock::time_point{};
    serialized = m_gameClient->Serialize(m_memoryStream->BeginFrame(), m_memoryStream->FrameSize());
    if (measure)
      serializeUs = std::chrono::duration_cast<std::chrono::microseconds>(
                        std::chrono::steady_clock::now() - started)
                        .count();
    if (serialized)
    {
      m_memoryStream->SubmitFrame();
      UpdatePlaybackStats();
    }
  }
  ++m_totalFrameCount;
  ProcessAutosave(serialized, serializeUs);
}

void CReversiblePlayback::ProcessAutosave(bool serialized, int64_t serializeUs)
{
  if (!m_saveWorker)
    return;

  if (m_snapshotReady)
  {
    if (m_saveWorker->TrySubmit(m_pendingSnapshot))
      m_snapshotReady = false;
    return;
  }
  if (!m_autosaveCapture.IsPending())
    return;
  if (!m_pendingSnapshot)
    m_pendingSnapshot = m_saveWorker->TryAcquire();
  if (!m_pendingSnapshot)
    return;

  auto& snapshot = *m_pendingSnapshot;
  const auto started = std::chrono::steady_clock::now();
  m_snapshotReady = m_autosaveCapture.CaptureFrame(
      m_memoryStream.get(), serialized, snapshot.memory,
      [this, &snapshot]
      {
        const auto coreStarted = std::chrono::steady_clock::now();
        const bool success = m_gameClient->Serialize(
            reinterpret_cast<uint8_t*>(snapshot.memory.get()), m_memorySize);
        snapshot.serializeUs = std::chrono::duration_cast<std::chrono::microseconds>(
                                   std::chrono::steady_clock::now() - coreStarted)
                                   .count();
        return success;
      },
      [this, &snapshot, serialized, serializeUs]
      {
        CaptureMetadata(snapshot);
        snapshot.rewind = serialized;
        if (serialized)
          snapshot.serializeUs = serializeUs;
        snapshot.captureUs = 0;
      });
  snapshot.captureUs += std::chrono::duration_cast<std::chrono::microseconds>(
                            std::chrono::steady_clock::now() - started)
                            .count();
  if (m_snapshotReady)
  {
    if (m_saveWorker->TrySubmit(m_pendingSnapshot))
      m_snapshotReady = false;
  }
}

void CReversiblePlayback::UpdateFrameRate()
{
  const double previousFrameRate = m_gameLoop.FPS();
  m_gameLoop.SetFrameRate(m_gameClient->GetFrameRate());

  if (m_gameLoop.FPS() != previousFrameRate)
    UpdateMemoryStream();
}

void CReversiblePlayback::RewindFrames(uint64_t frames)
{
  std::unique_lock lock(m_mutex);
  InvalidateAutosave();

  if (m_memoryStream)
  {
    m_memoryStream->RewindFrames(frames);
    m_gameClient->Deserialize(m_memoryStream->CurrentFrame(), m_memoryStream->FrameSize());
    UpdatePlaybackStats();
  }

  m_totalFrameCount -= std::min(m_totalFrameCount, frames);
}

void CReversiblePlayback::AdvanceFrames(uint64_t frames)
{
  std::unique_lock lock(m_mutex);
  InvalidateAutosave();

  if (m_memoryStream)
  {
    m_memoryStream->AdvanceFrames(frames);
    m_gameClient->Deserialize(m_memoryStream->CurrentFrame(), m_memoryStream->FrameSize());
    UpdatePlaybackStats();
  }

  m_totalFrameCount += frames;
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
    InvalidateAutosave();
    m_memoryStream.reset();

    // Reset playback stats
    m_pastFrameCount = 0;
    m_futureFrameCount = 0;
    m_playTimeMs = 0;
    m_totalTimeMs = 0;
    m_cacheTimeMs = 0;
  }
}
