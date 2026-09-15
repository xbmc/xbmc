/*
 *  Copyright (C) 2026 Team Kodi
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "WasmVsync.h"

#include <emscripten/em_asm.h>
#include <emscripten/threading.h>

namespace
{
volatile uint32_t g_tick = 0;
alignas(8) volatile int64_t g_tickTimeNs = 0;
volatile int32_t g_refreshRateMilliHz = 0;
bool g_pumpInstalled = false;
constexpr double DEFAULT_REFRESH_RATE = 60.0;
} // namespace

namespace KODI::WINDOWING::WASM::VSYNC
{
// CurrentHostCounter() is emscripten_get_now() in nanoseconds, which under
// pthreads is performance.timeOrigin + performance.now(); the rAF timestamp is
// relative to that same origin. Microseconds still fit a double exactly, so the
// conversion happens there and the last scaling step in BigInt.
void InstallPump()
{
  if (g_pumpInstalled)
    return;
  g_pumpInstalled = true;

  MAIN_THREAD_EM_ASM(
      {
        const tickIdx = $0 >> 2;
        const timeIdx = $1 >> 3;
        const rateIdx = $2 >> 2;
        let lastTimestamp = 0.0;
        let measuredRefreshRate = 0.0;
        const tick = (timestamp) =>
        {
          if (lastTimestamp > 0.0)
          {
            const intervalMs = timestamp - lastTimestamp;
            if (intervalMs > 0.0)
            {
              const hz = 1000.0 / intervalMs;
              if (hz >= 20.0 && hz <= 240.0)
              {
                measuredRefreshRate =
                    measuredRefreshRate > 0.0 ? measuredRefreshRate * 0.9 + hz * 0.1 : hz;
                Atomics.store(HEAP32, rateIdx, Math.round(measuredRefreshRate * 1000.0));
              }
            }
          }
          lastTimestamp = timestamp;
          Atomics.store(HEAP64, timeIdx,
                        BigInt(Math.round((performance.timeOrigin + timestamp) * 1000.0)) * 1000n);
          Atomics.add(HEAP32, tickIdx, 1);
          Atomics.notify(HEAP32, tickIdx);
          requestAnimationFrame(tick);
        };
        requestAnimationFrame(tick);
      },
      &g_tick, &g_tickTimeNs, &g_refreshRateMilliHz);
}

uint32_t Tick()
{
  return __atomic_load_n(&g_tick, __ATOMIC_ACQUIRE);
}

uint32_t WaitForTick(uint32_t seen, double timeoutMs)
{
  if (Tick() == seen)
    emscripten_futex_wait(&g_tick, seen, timeoutMs);
  return Tick();
}

int64_t LastTickHostTime()
{
  return __atomic_load_n(&g_tickTimeNs, __ATOMIC_ACQUIRE);
}

double RefreshRate()
{
  const int32_t milliHz = __atomic_load_n(&g_refreshRateMilliHz, __ATOMIC_RELAXED);
  return milliHz > 0 ? milliHz / 1000.0 : DEFAULT_REFRESH_RATE;
}
} // namespace KODI::WINDOWING::WASM::VSYNC
