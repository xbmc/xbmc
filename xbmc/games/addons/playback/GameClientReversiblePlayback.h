/*
 *      Copyright (C) 2016-present Team Kodi
 *      This file is part of Kodi - https://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
 *
 */
#pragma once

#include "IGameClientPlayback.h"
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
  class CSavestateReader;
  class CSavestateWriter;
  class IMemoryStream;

  class CGameClientReversiblePlayback : public IGameClientPlayback,
                                        public IGameLoopCallback,
                                        public Observer
  {
  public:
    CGameClientReversiblePlayback(CGameClient* gameClient, double fps, size_t serializeSize);

    virtual ~CGameClientReversiblePlayback();

    // implementation of IGameClientPlayback
    virtual bool CanPause() const override               { return true; }
    virtual bool CanSeek() const override                { return true; }
    virtual unsigned int GetTimeMs() const override      { return m_playTimeMs; }
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
    void RewindFrames(unsigned int frames);
    void AdvanceFrames(unsigned int frames);
    void UpdatePlaybackStats();
    void UpdateMemoryStream();

    // Construction parameter
    CGameClient* const m_gameClient;

    // Gameplay functionality
    CGameLoop                      m_gameLoop;
    std::unique_ptr<IMemoryStream> m_memoryStream;
    CCriticalSection               m_mutex;

    // Savestate functionality
    std::unique_ptr<CSavestateWriter> m_savestateWriter;
    std::unique_ptr<CSavestateReader> m_savestateReader;

    // Playback stats
    uint64_t     m_totalFrameCount;
    unsigned int m_pastFrameCount;
    unsigned int m_futureFrameCount;
    unsigned int m_playTimeMs;
    unsigned int m_totalTimeMs;
    unsigned int m_cacheTimeMs;
  };
}
}
