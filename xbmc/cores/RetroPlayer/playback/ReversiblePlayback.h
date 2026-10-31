/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "AutosaveCapture.h"
#include "DiscStateHistory.h"
#include "GameLoop.h"
#include "IPlayback.h"
#include "SavestateWorker.h"
#include "XBDateTime.h"
#include "cores/RetroPlayer/rendering/RPRenderManager.h"
#include "games/addons/GameClientRestoreResult.h"
#include "games/addons/disc/GameClientDiscState.h"
#include "threads/CriticalSection.h"
#include "utils/Observer.h"

#include <atomic>
#include <memory>
#include <optional>
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
class CRPStreamManager;
class CSavestateDatabase;
class CDeltaPairMemoryStream;

class CReversiblePlayback : public IPlayback, public IGameLoopCallback, public Observer
{
public:
  CReversiblePlayback(GAME::CGameClient* gameClient,
                      CRPRenderManager& renderManager,
                      CRPStreamManager& streamManager,
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
  /*!
   * \brief Decide how many frames to run ahead for the coming frame, or 0 for
   *        none
   *
   * Asked each frame, because a client may not know its state size until it
   * has run. Also applies a settings change, lets go of the state buffer when
   * run-ahead is off, and logs each reason it holds off once.
   */
  unsigned int PrepareRunahead();

  /*!
   * \brief Run the frame that is really happening, then show one from further on
   *
   * The client is run past the current frame with the same input, the picture
   * and sound of the last of those frames are presented, and the client is
   * put back. Input then takes effect on screen that many frames sooner.
   *
   * \return The state the client was put back to, or nullptr if run-ahead
   *         failed, in which case it is switched off, and playback is paused
   *         if the client couldn't be put back
   */
  const uint8_t* RunaheadFrame(unsigned int frames);

  /*!
   * \brief Run a frame as it is while the client refuses a state for run-ahead,
   * then ask it again
   */
  void RunHeldFrame();

  void UpdateRunahead();

  //! \param runaheadState The client's current state if run-ahead took it
  void AddFrame(const uint8_t* runaheadState = nullptr);
  void UpdateFrameRate();
  GAME::RestoreResult RewindFrames(uint64_t frames);
  GAME::RestoreResult AdvanceFrames(uint64_t frames);
  GAME::RestoreResult RestoreFrame();
  void LatchRestoreFailure();
  void UpdatePlaybackStats();
  void UpdateMemoryStream();
  struct Snapshot
  {
    std::unique_ptr<uint32_t[]> memory;
    std::vector<uint8_t> achievements;
    std::optional<GAME::GameClientDiscState> discState;
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

  void InitializeSaveWorker();
  void CaptureMetadata(Snapshot& snapshot);
  void ProcessAutosave(bool serialized, int64_t serializeUs);
  void CancelAutosave();
  void InvalidateAutosave();
  bool CommitSavestate(const Snapshot& snapshot);
  std::string GetSavestatePath(bool autosave, const std::string& path, const CDateTime& created);

  // Construction parameter
  GAME::CGameClient* const m_gameClient;
  CRPRenderManager& m_renderManager;
  CRPStreamManager& m_streamManager;
  CGUIGameMessenger& m_guiMessenger;

  // Gameplay functionality
  CGameLoop m_gameLoop;
  std::unique_ptr<CDeltaPairMemoryStream> m_memoryStream;
  CDiscStateHistory m_discStateHistory;
  CCriticalSection m_mutex;
  bool m_restoreFailed{false};
  bool m_rewindFrameRendered{false};

  //! Retry after each frame until serialization becomes available, or rewind is disabled.
  bool m_memoryStreamSized{false};

  // Run-ahead functionality. The settings observer only writes the atomics;
  // the buffers belong to the game loop.
  std::atomic<unsigned int> m_runaheadFrames{0};
  std::atomic<bool> m_runaheadReset{false};
  enum class RunaheadStatus
  {
    Ready,
    Failed,
    Unsupported,
    NotDeterministic,
    AudioOffFrameThread,
  };
  RunaheadStatus m_runaheadStatus{RunaheadStatus::Ready};
  //! Whether the client has loaded back a state it saved, which some can't
  bool m_runaheadRestoreChecked{false};
  //! Whether the client refused its last state for run-ahead
  bool m_runaheadHeld{false};
  std::vector<uint8_t> m_runaheadState;

  // Savestate functionality
  CAutosaveCapture m_autosaveCapture;
  std::unique_ptr<CSavestateDatabase> m_savestateDatabase;
  std::string m_autosavePath{};
  size_t m_memorySize;
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
