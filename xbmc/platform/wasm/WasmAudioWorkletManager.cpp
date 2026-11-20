/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "WasmAudioWorkletManager.h"

#include "WasmAudioWorklet.h"
#include "utils/log.h"

#include <algorithm>
#include <chrono>

#include <emscripten/threading.h>

namespace
{
constexpr unsigned int BUFFER_TARGET_MS = 100;
constexpr unsigned int MIN_BUFFER_QUANTA = 2;
constexpr unsigned int PREBUFFER_TARGET_MS = 40;
constexpr unsigned int WORKLET_STALL_LIMIT_MS = 500;
constexpr auto ASYNC_TIMEOUT = std::chrono::seconds(5);

// The processor accesses these as plain shared memory with JS Atomics.
static_assert(std::atomic<bool>::is_always_lock_free && sizeof(std::atomic<bool>) == 1);
static_assert(std::atomic<uint32_t>::is_always_lock_free && sizeof(std::atomic<uint32_t>) == 4);
} // namespace

namespace KODI::PLATFORM::WASM
{
CWasmAudioWorkletManager& CWasmAudioWorkletManager::GetInstance()
{
  static CWasmAudioWorkletManager instance;
  return instance;
}

bool CWasmAudioWorkletManager::Initialize(unsigned int channels)
{
  if (!m_contextOpen)
    return false;

  if (channels == 0 || channels > kMaxChannels)
  {
    CLog::Log(LOGERROR, "WASM AudioWorklet: unsupported channel count {}", channels);
    Shutdown();
    return false;
  }

  m_channels.store(channels, std::memory_order_release);

  EnsureBufferAllocated();
  ResetBuffer();

  if (!StartNode())
  {
    Shutdown();
    return false;
  }

  RefreshPipelineLatency();

  m_ready.store(true, std::memory_order_release);
  const unsigned int sampleRateNow = std::max(m_sampleRate.load(std::memory_order_relaxed), 1U);
  const double prebufMs =
      static_cast<double>(m_prebufferFrames) * 1000.0 / static_cast<double>(sampleRateNow);
  CLog::Log(LOGINFO,
            "WASM AudioWorklet: initialized (channels={}, sampleRate={}, quantum={}, "
            "pipelineLatency={:.3f} ms, ringCapacity={:.3f} ms, prebuffer={:.1f} ms)",
            channels, m_sampleRate.load(std::memory_order_relaxed),
            m_quantumSize.load(std::memory_order_relaxed),
            GetPipelineLatencySeconds() * 1000.0, GetBufferCapacitySeconds() * 1000.0,
            prebufMs);
  return true;
}

void CWasmAudioWorkletManager::ResetBuffer()
{
  m_ring.Reset();
  m_underrunFrames.store(0, std::memory_order_relaxed);
  m_prebufferComplete.store(false, std::memory_order_release);
}

void CWasmAudioWorkletManager::Shutdown()
{
  WaitForWorkletIdle();

  if (m_contextOpen)
  {
    wasm_audio_close();
    m_contextOpen = false;
  }

  m_ring.Free();
  m_prebufferFrames = 0;
  m_prebufferComplete.store(false, std::memory_order_release);
  m_pipelineLatencyUs.store(0, std::memory_order_relaxed);
  m_underrunFrames.store(0, std::memory_order_relaxed);
}

unsigned int CWasmAudioWorkletManager::WritePlanar(const float* const* planes,
                                                   unsigned int channels,
                                                   unsigned int frames,
                                                   unsigned int offsetFrames)
{
  // audio_worklet.js can only report a processor failure through the node state.
  if (m_nodeState.load(std::memory_order_acquire) == WASM_AUDIO_NODE_FAILED &&
      m_ready.exchange(false, std::memory_order_acq_rel))
    CLog::Log(LOGERROR, "WASM AudioWorklet: the processor failed");

  if (!planes || frames == 0 || !IsReady())
    return 0;

  const unsigned int configuredChannels = m_channels.load(std::memory_order_relaxed);
  if (configuredChannels == 0 || configuredChannels > kMaxChannels)
    return 0;

  const unsigned int copyChannels = std::min(channels, configuredChannels);
  if (copyChannels == 0)
    return 0;
  for (unsigned int ch = 0; ch < copyChannels; ++ch)
  {
    if (!planes[ch])
      return 0;
  }

  const int quantumMs = QuantumSleepMs();

  unsigned int writtenFrames = 0;
  unsigned int stalledMs = 0;
  while (writtenFrames < frames)
  {
    if (!IsReady())
      return writtenFrames;

    if (m_ring.GetCapacityFrames() == 0)
      return writtenFrames;

    const unsigned int written =
        m_ring.Write(planes, copyChannels, frames - writtenFrames, offsetFrames + writtenFrames);
    if (written == 0)
    {
      // A context without a user gesture never drains the ring, and ActiveAE
      // treats repeated zero writes as a dead sink.
      if (!m_contextRunning.load(std::memory_order_acquire))
      {
        emscripten_thread_sleep(static_cast<double>(frames - writtenFrames) * 1000.0 /
                                static_cast<double>(std::max(m_sampleRate.load(), 1U)));
        return frames;
      }
      stalledMs += static_cast<unsigned int>(quantumMs);
      if (stalledMs >= WORKLET_STALL_LIMIT_MS)
        return writtenFrames;
      emscripten_thread_sleep(quantumMs);
      continue;
    }
    stalledMs = 0;
    writtenFrames += written;
  }

  return writtenFrames;
}

// Sleeping for one audio quantum wakes a waiting thread right when the worklet has
// consumed a block, rather than busy-looping on 1 ms sleeps.
int CWasmAudioWorkletManager::QuantumSleepMs() const
{
  const unsigned int sampleRate = std::max(m_sampleRate.load(std::memory_order_relaxed), 1U);
  const unsigned int quantum = std::max(m_quantumSize.load(std::memory_order_relaxed), 1U);
  return std::max(1,
                  static_cast<int>((static_cast<uint64_t>(quantum) * 1000ULL + sampleRate - 1ULL) /
                                   sampleRate));
}

void CWasmAudioWorkletManager::Drain()
{
  // Lets the processor play what is queued below the prebuffer watermark.
  m_prebufferComplete.store(true, std::memory_order_release);

  const int quantumMs = QuantumSleepMs();
  uint32_t lastReadFrame = m_ring.GetReadFrame().load(std::memory_order_acquire);
  unsigned int stalledMs = 0;

  while (IsReady() && GetBufferedSeconds() > 0.0)
  {
    emscripten_thread_sleep(quantumMs);

    const uint32_t readFrame = m_ring.GetReadFrame().load(std::memory_order_acquire);
    if (readFrame != lastReadFrame)
    {
      lastReadFrame = readFrame;
      stalledMs = 0;
      continue;
    }

    // An AudioContext that no user gesture has resumed yet never drains the ring,
    // so stop waiting rather than hold up the sink thread until a fixed deadline.
    stalledMs += static_cast<unsigned int>(quantumMs);
    if (stalledMs >= WORKLET_STALL_LIMIT_MS)
    {
      m_prebufferComplete.store(false, std::memory_order_release);
      return;
    }
  }

  // The ring stays empty until the sink writes again, which is not an underrun:
  // the processor counts nothing while it waits for the prebuffer.
  m_prebufferComplete.store(false, std::memory_order_release);

  // The browser still holds the pipeline latency's worth of audio after the ring empties.
  if (IsReady() && m_contextRunning.load(std::memory_order_acquire))
  {
    RefreshPipelineLatency();
    emscripten_thread_sleep(GetPipelineLatencySeconds() * 1000.0);
  }

  // Drops what the processor found missing as the ring ran out.
  m_underrunFrames.store(0, std::memory_order_relaxed);
}

double CWasmAudioWorkletManager::GetBufferedSeconds() const
{
  const unsigned int rate = m_sampleRate.load(std::memory_order_relaxed);
  if (rate == 0)
    return 0.0;

  return static_cast<double>(m_ring.GetBufferedFrames()) / static_cast<double>(rate);
}

double CWasmAudioWorkletManager::GetBufferCapacitySeconds() const
{
  const unsigned int rate = m_sampleRate.load(std::memory_order_relaxed);
  if (rate == 0)
    return 0.0;

  return static_cast<double>(m_ring.GetCapacityFrames()) / static_cast<double>(rate);
}

double CWasmAudioWorkletManager::GetPipelineLatencySeconds() const
{
  return static_cast<double>(m_pipelineLatencyUs.load(std::memory_order_relaxed)) / 1'000'000.0;
}

double CWasmAudioWorkletManager::GetTotalDelaySeconds() const
{
  return GetBufferedSeconds() + GetPipelineLatencySeconds();
}

uint64_t CWasmAudioWorkletManager::ConsumeUnderrunFrames()
{
  return m_underrunFrames.exchange(0, std::memory_order_relaxed);
}

unsigned int CWasmAudioWorkletManager::GetSampleRate() const
{
  return m_sampleRate.load(std::memory_order_relaxed);
}

unsigned int CWasmAudioWorkletManager::GetQuantumSize() const
{
  return m_quantumSize.load(std::memory_order_relaxed);
}

unsigned int CWasmAudioWorkletManager::GetChannels() const
{
  return m_channels.load(std::memory_order_relaxed);
}

unsigned int CWasmAudioWorkletManager::GetMaxOutputChannels() const
{
  const int channels = wasm_audio_max_output_channels();
  return channels > 0 ? static_cast<unsigned int>(channels) : 2U;
}

// seq_cst pairs with the processor, which increments m_activeCallbacks before loading
// m_ready: either the worklet sees the flag or we see the callback and wait for it.
void CWasmAudioWorkletManager::WaitForWorkletIdle()
{
  m_ready.store(false, std::memory_order_seq_cst);
  while (m_activeCallbacks.load(std::memory_order_seq_cst) > 0)
    emscripten_thread_sleep(1);
}

bool CWasmAudioWorkletManager::IsReady() const
{
  return m_ready.load(std::memory_order_acquire);
}

bool CWasmAudioWorkletManager::OpenContext(unsigned int requestedSampleRate)
{
  if (m_contextOpen)
    Shutdown();

  int sampleRate = 0;
  int quantumSize = 0;
  if (!wasm_audio_open(static_cast<int>(requestedSampleRate), &m_contextRunning, &sampleRate,
                       &quantumSize))
  {
    CLog::Log(LOGERROR, "WASM AudioWorklet: failed to create AudioContext");
    return false;
  }
  m_contextOpen = true;

  m_sampleRate.store(static_cast<unsigned int>(sampleRate), std::memory_order_relaxed);
  m_quantumSize.store(static_cast<unsigned int>(quantumSize), std::memory_order_relaxed);
  return true;
}

bool CWasmAudioWorkletManager::StartNode()
{
  m_nodeState.store(WASM_AUDIO_NODE_PENDING, std::memory_order_relaxed);
  m_nodeGeneration.fetch_add(1, std::memory_order_seq_cst);
  wasm_audio_start(static_cast<int>(m_channels.load(std::memory_order_relaxed)), m_ring.GetData(),
                   static_cast<int>(m_ring.GetCapacityFrames()),
                   static_cast<int>(m_prebufferFrames), &m_ready, &m_prebufferComplete,
                   &m_activeCallbacks, &m_nodeGeneration, &m_ring.GetReadFrame(),
                   &m_ring.GetWriteFrame(), &m_underrunFrames, &m_nodeState);

  const auto deadline = std::chrono::steady_clock::now() + ASYNC_TIMEOUT;
  uint32_t state;
  while ((state = m_nodeState.load(std::memory_order_acquire)) == WASM_AUDIO_NODE_PENDING)
  {
    const double remainingMs =
        std::chrono::duration<double, std::milli>(deadline - std::chrono::steady_clock::now())
            .count();
    if (remainingMs <= 0.0)
    {
      CLog::Log(LOGERROR, "WASM AudioWorklet: timeout waiting for the processor");
      return false;
    }
    emscripten_futex_wait(&m_nodeState, WASM_AUDIO_NODE_PENDING, remainingMs);
  }

  if (state != WASM_AUDIO_NODE_STARTED)
  {
    CLog::Log(LOGERROR, "WASM AudioWorklet: failed to create node");
    return false;
  }
  return true;
}

void CWasmAudioWorkletManager::RefreshPipelineLatency()
{
  if (!m_contextOpen)
    return;

  const int latencyUs = wasm_audio_latency_us();
  if (latencyUs < 0)
    return;

  m_pipelineLatencyUs.store(static_cast<uint32_t>(latencyUs), std::memory_order_relaxed);
}

void CWasmAudioWorkletManager::EnsureBufferAllocated()
{
  const unsigned int sampleRate = std::max(m_sampleRate.load(std::memory_order_relaxed), 1U);
  const unsigned int quantumSize = std::max(m_quantumSize.load(std::memory_order_relaxed), 1U);
  const unsigned int channels = std::max(m_channels.load(std::memory_order_relaxed), 1U);
  const uint64_t targetFrames =
      (static_cast<uint64_t>(sampleRate) * BUFFER_TARGET_MS + 999ULL) / 1000ULL;
  const uint64_t minFrames = static_cast<uint64_t>(quantumSize) * MIN_BUFFER_QUANTA;
  const unsigned int capacityFrames =
      static_cast<unsigned int>(std::max(targetFrames, minFrames));
  m_ring.Allocate(capacityFrames, channels);

  // Compute the prebuffer watermark. Clamp to half the ring so we always
  // have room for new writes before output resumes.
  const uint64_t prebuf =
      (static_cast<uint64_t>(sampleRate) * PREBUFFER_TARGET_MS + 999ULL) / 1000ULL;
  const uint64_t quantaInPrebuf = (prebuf + quantumSize - 1ULL) / quantumSize;
  const uint64_t prebufRounded = quantaInPrebuf * quantumSize;
  m_prebufferFrames =
      static_cast<unsigned int>(std::min<uint64_t>(prebufRounded, capacityFrames / 2));
  m_prebufferComplete.store(false, std::memory_order_release);
}

} // namespace KODI::PLATFORM::WASM
