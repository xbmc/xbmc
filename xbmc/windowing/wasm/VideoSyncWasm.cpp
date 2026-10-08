/*
 *  Copyright (C) 2026 Team Kodi
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "VideoSyncWasm.h"

#include "WasmVsync.h"
#include "cores/VideoPlayer/VideoReferenceClock.h"
#include "utils/MathUtils.h"
#include "utils/TimeUtils.h"

#include <cmath>

using namespace KODI::WINDOWING::WASM;

namespace
{
// Bounds how long a stop request waits while a hidden tab produces no ticks.
constexpr double TICK_WAIT_TIMEOUT_MS = 100.0;
// Display rates are at least 20% apart; frame timing jitter stays well below this.
constexpr double RATE_CHANGE_TOLERANCE = 0.1;
// Ticks the pump's moving average needs to come within ~2% of a new rate.
constexpr int RATE_CHANGE_TICKS = 40;
} // namespace

bool CVideoSyncWasm::Setup()
{
  const int64_t lastTick = VSYNC::LastTickHostTime();
  m_lastVBlankTime = lastTick != 0 ? lastTick : CurrentHostCounter();
  return true;
}

// The browser skips requestAnimationFrame callbacks while its main thread is
// busy, so vblanks are counted from the tick timestamps rather than the ticks.
void CVideoSyncWasm::Run(CEvent& stop)
{
  uint32_t seen = VSYNC::Tick();
  int deviatingTicks = 0;
  while (!stop.Signaled())
  {
    // A tab started in the background has no measurement yet. Returning makes
    // the reference clock set up again and pick up the measured rate.
    if (!m_rateSettled && VSYNC::RefreshRateSettled())
      return;

    const uint32_t tick = VSYNC::WaitForTick(seen, TICK_WAIT_TIMEOUT_MS);
    if (tick == seen)
      continue;
    seen = tick;

    // The rate changes when the window moves to another display. Waiting for the
    // measurement to settle sets the reference clock up again only once.
    if (std::abs(VSYNC::RefreshRate() - m_fps) > m_fps * RATE_CHANGE_TOLERANCE)
    {
      if (++deviatingTicks >= RATE_CHANGE_TICKS)
        return;
    }
    else
      deviatingTicks = 0;

    // Counting from the last counted vblank rather than the last tick keeps the
    // clock advancing while m_fps differs from the tick rate.
    const int64_t tickTime = VSYNC::LastTickHostTime();
    const double frequency = static_cast<double>(CurrentHostFrequency());
    const double elapsed = static_cast<double>(tickTime - m_lastVBlankTime) / frequency;
    const int vblanks = MathUtils::round_int(elapsed * static_cast<double>(m_fps));
    if (vblanks <= 0)
      continue;
    m_lastVBlankTime += std::llround(vblanks * frequency / static_cast<double>(m_fps));
    m_refClock->UpdateClock(vblanks, tickTime);
  }
}

float CVideoSyncWasm::GetFps()
{
  m_rateSettled = VSYNC::RefreshRateSettled();
  m_fps = static_cast<float>(VSYNC::RefreshRate());
  return m_fps;
}
