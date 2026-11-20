/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <cstdint>

// Web Audio output for CWasmAudioWorkletManager; implemented in audio_worklet.js.

// Values audio_worklet.js stores in the node state.
inline constexpr uint32_t WASM_AUDIO_NODE_PENDING = 0;
inline constexpr uint32_t WASM_AUDIO_NODE_STARTED = 1;
inline constexpr uint32_t WASM_AUDIO_NODE_FAILED = 2;

extern "C"
{
  int wasm_audio_open(int sampleRate, void* runningFlag, int* sampleRateOut, int* quantumSizeOut);
  // Stores WASM_AUDIO_NODE_STARTED in state once the processor has been
  // constructed, and WASM_AUDIO_NODE_FAILED when the node cannot start or its
  // processor fails later.
  void wasm_audio_start(int channels,
                        void* ring,
                        int capacityFrames,
                        int prebufferFrames,
                        void* ready,
                        void* prebufferComplete,
                        void* activeCallbacks,
                        void* generation,
                        void* readFrame,
                        void* writeFrame,
                        void* underrunFrames,
                        void* state);
  void wasm_audio_close();
  int wasm_audio_latency_us();
  int wasm_audio_max_output_channels();
}
