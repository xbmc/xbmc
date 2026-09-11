/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "AutosaveCapture.h"
#include "GameLoop.h"
#include "IPlayback.h"
#include "SavestateWorker.h"
#include "XBDateTime.h"
#include "cores/RetroPlayer/rendering/RPRenderManager.h"
#include "threads/CriticalSection.h"
#include "utils/Observer.h"

#include <atomic>
#include <memory>
#include <stddef.h>
#include <stdint.h>
#include <vector>

namespace KODI
{
namespace GAME
{
class CGameClient;
}

namespace RETRO
{
class CGUIGameMessenger;
class CSavestateDatabase;
class IMemoryStream;

class CReversiblePlayback : public IPlayback, public IGameLoopCallback, public Observer
{
public:
  CReversiblePlayback(GAME::CGameClient* gameClient,
                      CRPRenderManager& renderManager,
                      CGUIGameMessenger& guiMessenger,
                      double fps,
                      size_t serializeSize);

  ~CReversiblePlayback() override;

  // implementation of IPlayback
  void Initialize() override;
  void Quiesce() override;
  void Deinitialize() override;
  bool WaitForSavestates() override;
  bool CanPause() const override { return true; }
  bool CanSeek() const override { return true; }
  unsigned int GetTimeMs() const override { return m_playTimeMs; }
  unsigned int GetTotalTimeMs() const override { return m_totalTimeMs; }
  unsigned int GetCacheTimeMs() const override { return m_cacheTimeMs; }
  void SeekTimeMs(unsigned int timeMs) override;
  double GetSpeed() const override;
  void SetSpeed(double speedFactor) override;
  void PauseAsync() override;
  void RequestAutosave() override { m_autosaveCapture.Request(); }
  std::string CreateSavestate(bool autosave, const std::string& savestatePath = "") override;
  bool LoadSavestate(const std::string& savestatePath) override;

  // implementation of IGameLoopCallback
  void FrameEvent() override;
  void RewindEvent() override;
  void EndEvent() override;

  // implementation of Observer
  void Notify(const Observable& obs, const ObservableMessage msg) override;

private:
  void AddFrame();
  void UpdateFrameRate();
  void RewindFrames(uint64_t frames);
  void AdvanceFrames(uint64_t frames);
  void UpdatePlaybackStats();
  void UpdateMemoryStream();
  struct Snapshot
  {
    std::unique_ptr<uint32_t[]> memory;
    std::vector<uint8_t> achievements;
    CRPRenderManager::VideoFrame video;
    CDateTime created;
    uint64_t frames{0};
    double wallClock{0.0};
    std::string path;
    bool autosave{true};
    bool rewind{false};
    bool discarded{false};
    int64_t serializeUs{0};
    int64_t captureUs{0};
  };

  void CaptureMetadata(Snapshot& snapshot);
  void ProcessAutosave(bool serialized, int64_t serializeUs);
  void CancelAutosave();
  void InvalidateAutosave();
  bool CommitSavestate(const Snapshot& snapshot);
  std::string GetSavestatePath(bool autosave, const std::string& path, const CDateTime& created);

  // Construction parameter
  GAME::CGameClient* const m_gameClient;
  CRPRenderManager& m_renderManager;
  CGUIGameMessenger& m_guiMessenger;

  // Gameplay functionality
  CGameLoop m_gameLoop;
  std::unique_ptr<IMemoryStream> m_memoryStream;
  CCriticalSection m_mutex;

  // Savestate functionality
  CAutosaveCapture m_autosaveCapture;
  std::unique_ptr<CSavestateDatabase> m_savestateDatabase;
  std::string m_autosavePath{};
  const size_t m_memorySize;
  const std::string m_gamePath;
  const std::string m_gameClientId;
  const std::string m_gameClientVersion;
  std::unique_ptr<CSavestateWorker<Snapshot>> m_saveWorker;
  std::unique_ptr<Snapshot> m_pendingSnapshot;
  bool m_snapshotReady{false};
  std::atomic<bool> m_saveSucceeded{true};
  CCriticalSection m_savestateMutex;

  // Playback stats
  uint64_t m_totalFrameCount = 0;
  uint64_t m_pastFrameCount = 0;
  uint64_t m_futureFrameCount = 0;
  unsigned int m_playTimeMs = 0;
  unsigned int m_totalTimeMs = 0;
  unsigned int m_cacheTimeMs = 0;
};
} // namespace RETRO
} // namespace KODI
