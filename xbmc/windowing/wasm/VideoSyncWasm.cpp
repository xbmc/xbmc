/*
 *  Copyright (C) 2026 Team Kodi
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "VideoSyncWasm.h"

#include "ServiceBroker.h"
#include "WasmVsync.h"
#include "cores/VideoPlayer/VideoReferenceClock.h"
#include "utils/MathUtils.h"
#include "utils/TimeUtils.h"
#include "windowing/GraphicContext.h"
#include "windowing/WinSystem.h"

using namespace KODI::WINDOWING::WASM;

namespace
{
// Bounds how long a stop request waits while a hidden tab produces no ticks.
constexpr double TICK_WAIT_TIMEOUT_MS = 100.0;
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
  while (!stop.Signaled())
  {
    const uint32_t tick = VSYNC::WaitForTick(seen, TICK_WAIT_TIMEOUT_MS);
    if (tick == seen)
      continue;
    seen = tick;

    const int64_t tickTime = VSYNC::LastTickHostTime();
    const double elapsed = static_cast<double>(tickTime - m_lastVBlankTime) /
                           static_cast<double>(CurrentHostFrequency());
    m_lastVBlankTime = tickTime;
    m_refClock->UpdateClock(MathUtils::round_int(elapsed * static_cast<double>(m_fps)), tickTime);
  }
}

float CVideoSyncWasm::GetFps()
{
  m_fps = CServiceBroker::GetWinSystem()->GetGfxContext().GetFPS();
  return m_fps;
}
