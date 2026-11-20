/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "WasmAudioRing.h"

#include <atomic>
#include <cstdint>

namespace KODI::PLATFORM::WASM
{
class CWasmAudioWorkletManager
{
public:
  static constexpr unsigned int kMaxChannels = 8;

  static CWasmAudioWorkletManager& GetInstance();

  // Initialize() starts a node on the context OpenContext() created.
  bool OpenContext(unsigned int requestedSampleRate);
  bool Initialize(unsigned int channels);
  void Shutdown();

  unsigned int WritePlanar(const float* const* planes,
                           unsigned int channels,
                           unsigned int frames,
                           unsigned int offsetFrames);
  void Drain();

  double GetBufferedSeconds() const;
  double GetBufferCapacitySeconds() const;
  double GetPipelineLatencySeconds() const;
  double GetTotalDelaySeconds() const;
  void RefreshPipelineLatency();

  unsigned int GetSampleRate() const;
  unsigned int GetQuantumSize() const;
  unsigned int GetChannels() const;
  unsigned int GetMaxOutputChannels() const;
  bool IsReady() const;

  // Returns and resets the number of underrun frames.
  uint64_t ConsumeUnderrunFrames();

private:
  CWasmAudioWorkletManager() = default;
  ~CWasmAudioWorkletManager() = default;
  CWasmAudioWorkletManager(const CWasmAudioWorkletManager&) = delete;
  CWasmAudioWorkletManager& operator=(const CWasmAudioWorkletManager&) = delete;

  bool StartNode();
  void EnsureBufferAllocated();
  void ResetBuffer();
  void WaitForWorkletIdle();
  int QuantumSleepMs() const;

  bool m_contextOpen{false};

  std::atomic<bool> m_ready{false};
  // One of the WASM_AUDIO_NODE_* values, written by audio_worklet.js.
  std::atomic<uint32_t> m_nodeState{0};

  std::atomic<unsigned int> m_channels{2};
  std::atomic<unsigned int> m_sampleRate{48000};
  std::atomic<unsigned int> m_quantumSize{128};

  // Allocated, freed and reset by the sink thread, only after WaitForWorkletIdle().
  CWasmAudioRing m_ring;
  // Identifies the node allowed to read the ring.
  std::atomic<uint32_t> m_nodeGeneration{0};

  // Written by the main thread through Atomics.store().
  std::atomic<bool> m_contextRunning{false};

  std::atomic<uint32_t> m_pipelineLatencyUs{0};

  std::atomic<uint32_t> m_underrunFrames{0};
  std::atomic<unsigned int> m_activeCallbacks{0};

  // Prebuffer watermark
  unsigned int m_prebufferFrames{0};
  std::atomic<bool> m_prebufferComplete{false};
};
} // namespace KODI::PLATFORM::WASM
