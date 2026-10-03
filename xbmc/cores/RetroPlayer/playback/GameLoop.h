/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/RetroPlayer/process/DisplayPacing.h"
#include "threads/Event.h"
#include "threads/Thread.h"

#include <atomic>
#include <chrono>

namespace KODI
{
namespace RETRO
{
/*!
 * \brief Interface for the game loop callback
 *
 * The IGameLoopCallback interface provides the necessary methods for handling
 * frame events within the game loop.
 *
 * Implementers must define how the next frame (FrameEvent) and previous frame
 * (RewindEvent) are processed.
 */
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

  /*!
   * \brief Called when the game loop has ended
   *
   * This gives implementers a chance to release resources or execute final
   * actions once the thread finishes processing frames.
   */
  virtual void EndEvent() = 0;
};

/*!
 * \brief The game loop class
 *
 * CGameLoop is responsible for managing the timing and execution of frame
 * updates in a game or emulator.
 *
 * Inside the Process method, the loop checks for changes in the speed factor,
 * updates frame timing, and uses precise sleep intervals to maintain a
 * consistent frame rate while invoking the appropriate callbacks for frame
 * events.
 */
class CGameLoop : protected CThread
{
public:
  CGameLoop(IGameLoopCallback* callback, double fps, CDisplayPacing* displayPacing = nullptr);

  ~CGameLoop() override;

  void Start();
  // Park at a frame boundary, preserving the game thread and its render context.
  void Quiesce();
  void Stop();

  double FPS() const { return m_fps.load(); }
  void SetFrameRate(double fps);

  double GetSpeed() const { return m_speedFactor.load(); }
  void SetSpeed(double speedFactor);
  void PauseAsync();

protected:
  // implementation of CThread
  void Process() override;

private:
  /*!
   * \brief Run the next frame in step with the screen, if it can be
   *
   * \return False if the frame should be timed by the game's own rate instead
   */
  bool PaceToDisplay();
  void StopPacing();
  void RunFrame();

  std::chrono::microseconds FrameTimeUs() const;
  std::chrono::microseconds NowUs() const;

  IGameLoopCallback* const m_callback;
  CDisplayPacing* const m_displayPacing;
  std::atomic<double> m_fps;
  std::atomic<double> m_speedFactor{0.0};
  double m_loopSpeedFactor{0.0};
  std::chrono::microseconds m_lastFrameUs{std::chrono::microseconds::zero()};
  CEvent m_sleepEvent;
  CDisplayPacing::Clock::time_point m_lastPacedTake{};
  CDisplayPacing::Clock::duration m_frameCost{};
  std::atomic<bool> m_quiesceRequested{false};
  CEvent m_quiescedEvent{true};
};
} // namespace RETRO
} // namespace KODI
