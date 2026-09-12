/*
 *  Copyright (C) 2026 Team Kodi
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <cstdint>

// Display refresh ticks from the browser main thread's requestAnimationFrame
// loop, shared with every Kodi thread that paces itself to the display.
namespace KODI::WINDOWING::WASM::VSYNC
{
// Installs the requestAnimationFrame pump on the browser main thread. Idempotent.
void InstallPump();

// Number of display frames the pump has seen.
uint32_t Tick();

// Blocks until the tick count differs from `seen` or `timeoutMs` elapses, and
// returns the current count.
uint32_t WaitForTick(uint32_t seen, double timeoutMs);

// Time of the most recent tick in CurrentHostCounter() units, 0 before the first.
int64_t LastTickHostTime();

// Refresh rate measured by the pump; 60 until enough frames have been seen.
double RefreshRate();
} // namespace KODI::WINDOWING::WASM::VSYNC
