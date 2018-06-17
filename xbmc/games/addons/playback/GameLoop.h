/*
 *      Copyright (C) 2016-2017 Team Kodi
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include <atomic>

#include "threads/Event.h"
#include "threads/Thread.h"

namespace KODI
{
namespace GAME
{
  class IGameLoopCallback
  {
  public:
    virtual ~IGameLoopCallback() = default;

    /*!
     * \brief The next frame is being shown
     */
    virtual void FrameEvent() = 0;

    /*!
     * \brief The prior frame is being shown
     */
    virtual void RewindEvent() = 0;
  };

  class CGameLoop : protected CThread
  {
  public:
    CGameLoop(IGameLoopCallback* callback, double fps);

    virtual ~CGameLoop();

    void Start();
    void Stop();

    double FPS() const { return m_fps; }

    double GetSpeed() const { return m_speedFactor; }
    void SetSpeed(double speedFactor);
    void PauseAsync();

  protected:
    // implementation of CThread
    virtual void Process() override;

  private:
    double FrameTimeMs() const;
    double SleepTimeMs() const;
    double NowMs() const;

    IGameLoopCallback* const m_callback;
    const double             m_fps;
    std::atomic<double>      m_speedFactor;
    double                   m_lastFrameMs;
    mutable double           m_adjustTime;
    CEvent                   m_sleepEvent;
  };
}
}
