/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "GameLoop.h"
#include "IPlayback.h"
#include "threads/CriticalSection.h"
#include "utils/Observer.h"

#include <future>
#include <memory>
#include <stddef.h>
#include <stdint.h>

class CDateTime;

namespace KODI
{
namespace GAME
{
class CGameClient;
}

namespace RETRO
{
class CCheevos;
class CGUIGameMessenger;
class CRPRenderManager;
class CSavestateDatabase;
class IMemoryStream;

class CReversiblePlayback : public IPlayback, public IGameLoopCallback, public Observer
{
public:
  CReversiblePlayback(GAME::CGameClient* gameClient,
                      CRPRenderManager& renderManager,
                      CCheevos* cheevos,
                      CGUIGameMessenger& guiMessenger,
                      double fps,
                      size_t serializeSize);

  ~CReversiblePlayback() override;

  // implementation of IPlayback
  void Initialize() override;
  void Deinitialize() override;
  bool CanPause() const override { return true; }
  bool CanSeek() const override { return true; }
  unsigned int GetTimeMs() const override { return m_playTimeMs; }
  unsigned int GetTotalTimeMs() const override { return m_totalTimeMs; }
  unsigned int GetCacheTimeMs() const override { return m_cacheTimeMs; }
  void SeekTimeMs(unsigned int timeMs) override;
  double GetSpeed() const override;
  void SetSpeed(double speedFactor) override;
  void PauseAsync() override;
  std::string CreateSavestate(bool autosave, const std::string& savestatePath = "") override;
  bool LoadSavestate(const std::string& savestatePath) override;

  // implementation of IGameLoopCallback
  void FrameEvent() override;
  void RewindEvent() override;

  // implementation of Observer
  void Notify(const Observable& obs, const ObservableMessage msg) override;

private:
  void AddFrame();
  void RewindFrames(uint64_t frames);
  void AdvanceFrames(uint64_t frames);
  void UpdatePlaybackStats();
  void UpdateMemoryStream();
  void CommitSavestate(bool autosave,
                       const std::string& savePath,
                       const CDateTime& nowUTC,
                       uint64_t timestampFrames);

  // Construction parameter
  GAME::CGameClient* const m_gameClient;
  CRPRenderManager& m_renderManager;
  CCheevos* const m_cheevos;
  CGUIGameMessenger& m_guiMessenger;

  // Gameplay functionality
  CGameLoop m_gameLoop;
  std::unique_ptr<IMemoryStream> m_memoryStream;
  CCriticalSection m_mutex;

  // Savestate functionality
  std::unique_ptr<CSavestateDatabase> m_savestateDatabase;
  std::string m_autosavePath{};
  std::vector<std::future<void>> m_savestateThreads;
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
