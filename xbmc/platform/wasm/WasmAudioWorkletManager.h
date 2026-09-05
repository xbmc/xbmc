/*
 *  Copyright (C) 2026 Team Kodi
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <vector>

namespace KODI::PLATFORM::WASM
{
class CWasmAudioWorkletManager
{
public:
  static constexpr unsigned int kMaxChannels = 8;

  static CWasmAudioWorkletManager& Instance();

  bool Initialize(unsigned int channels, unsigned int requestedSampleRate);
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

  bool EnsureContext(unsigned int requestedSampleRate);
  bool EnsureWorkletProcessor();
  bool ConfigureNode(unsigned int channels);
  void InstallResumeHooks() const;
  void ClearResumeHooks() const;
  void EnsureBufferAllocated();
  void ResetBuffer();
  void WaitForWorkletIdle();
  int QuantumSleepMs() const;
  void RefreshPipelineLatency();

  bool WaitForState(std::atomic<bool>& readyFlag, const char* stageName);
  static void OnWorkletThreadStarted(int audioContext, bool success, void* userData);
  static void OnProcessorCreated(int audioContext, bool success, void* userData);
  static bool ProcessAudio(int numInputs,
                           const void* inputs,
                           int numOutputs,
                           void* outputs,
                           int numParams,
                           const void* params,
                           void* userData);
  bool ProcessAudioImpl(int numOutputs, void* outputs);

  mutable std::mutex m_stateMutex;
  std::condition_variable m_stateCv;

  int m_audioContext{0};
  int m_workletNode{0};

  std::atomic<bool> m_ready{false};
  std::atomic<bool> m_workletThreadCreated{false};
  std::atomic<bool> m_processorCreated{false};
  std::atomic<bool> m_asyncResult{false};
  std::atomic<bool> m_asyncDone{false};

  std::atomic<unsigned int> m_channels{2};
  std::atomic<unsigned int> m_sampleRate{48000};
  std::atomic<unsigned int> m_quantumSize{128};

  // SPSC ring: the sink thread writes m_writeFrame and (re)allocates, only after
  // WaitForWorkletIdle(); the worklet thread writes m_readFrame.
  std::vector<float> m_ringBuffer;
  unsigned int m_bufferCapacityFrames{0};
  unsigned int m_bufferChannels{0};
  unsigned int m_bufferSampleRate{0};
  unsigned int m_bufferQuantumSize{0};
  std::atomic<uint64_t> m_readFrame{0};
  std::atomic<uint64_t> m_writeFrame{0};

  std::atomic<uint32_t> m_pipelineLatencyUs{0};

  std::atomic<uint64_t> m_underrunFrames{0};
  std::atomic<unsigned int> m_activeCallbacks{0};

  // Prebuffer watermark
  unsigned int m_prebufferFrames{0};
  std::atomic<bool> m_prebufferComplete{false};
};
} // namespace KODI::PLATFORM::WASM
