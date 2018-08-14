/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IPlayback.h"
#include "GameLoop.h"
#include "threads/CriticalSection.h"
#include "utils/Observer.h"

#include <memory>
#include <stddef.h>
#include <stdint.h>

namespace KODI
{
namespace GAME
{
  class CGameClient;
}

namespace RETRO
{
  class CSavestateDatabase;
  class IMemoryStream;

  class CReversiblePlayback : public IPlayback,
                              public IGameLoopCallback,
                              public Observer
  {
  public:
    CReversiblePlayback(GAME::CGameClient* gameClient, double fps, size_t serializeSize);

    virtual ~CReversiblePlayback();

    // implementation of IPlayback
    virtual void Initialize() override;
    virtual void Deinitialize() override;
    virtual bool CanPause() const override { return true; }
    virtual bool CanSeek() const override { return true; }
    virtual unsigned int GetTimeMs() const override { return m_playTimeMs; }
    virtual unsigned int GetTotalTimeMs() const override { return m_totalTimeMs; }
    virtual unsigned int GetCacheTimeMs() const override { return m_cacheTimeMs; }
    virtual void SeekTimeMs(unsigned int timeMs) override;
    virtual double GetSpeed() const override;
    virtual void SetSpeed(double speedFactor) override;
    virtual void PauseAsync() override;
    virtual std::string CreateSavestate() override;
    virtual bool LoadSavestate(const std::string& path) override;

    // implementation of IGameLoopCallback
    virtual void FrameEvent() override;
    virtual void RewindEvent() override;

    // implementation of Observer
    virtual void Notify(const Observable &obs, const ObservableMessage msg) override;

  private:
    void AddFrame();
    void RewindFrames(uint64_t frames);
    void AdvanceFrames(uint64_t frames);
    void UpdatePlaybackStats();
    void UpdateMemoryStream();

    // Construction parameter
    GAME::CGameClient* const m_gameClient;

    // Gameplay functionality
    CGameLoop m_gameLoop;
    std::unique_ptr<IMemoryStream> m_memoryStream;
    CCriticalSection m_mutex;

    // Savestate functionality
    std::unique_ptr<CSavestateDatabase> m_savestateDatabase;

    // Playback stats
    uint64_t m_totalFrameCount;
    uint64_t m_pastFrameCount;
    uint64_t m_futureFrameCount;
    unsigned int m_playTimeMs;
    unsigned int m_totalTimeMs;
    unsigned int m_cacheTimeMs;
  };
}
}
