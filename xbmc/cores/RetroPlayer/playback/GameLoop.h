/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "threads/Event.h"
#include "threads/Thread.h"

#include <atomic>
#include <chrono>

namespace KODI
{
namespace RETRO
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

  ~CGameLoop() override;

  void Start();
  void Stop();

  double FPS() const { return m_fps; }

  double GetSpeed() const { return m_speedFactor; }
  void SetSpeed(double speedFactor);
  void PauseAsync();

protected:
  // implementation of CThread
  void Process() override;

private:
  std::chrono::microseconds FrameTimeUs() const;
  std::chrono::microseconds NowUs() const;

  IGameLoopCallback* const m_callback;
  const double m_fps;
  std::atomic<double> m_speedFactor;
  double m_loopSpeedFactor{0};
  std::chrono::microseconds m_lastFrameUs{std::chrono::microseconds::zero()};
  CEvent m_sleepEvent;
};
} // namespace RETRO
} // namespace KODI
