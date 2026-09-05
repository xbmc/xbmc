/*
 *  Copyright (C) 2026 Team Kodi
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "WasmAudioWorkletManager.h"

#include "utils/log.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <limits>

#include <emscripten.h>
#include <emscripten/em_asm.h>
#include <emscripten/threading.h>
#include <emscripten/webaudio.h>

namespace
{
constexpr char AUDIO_PROCESSOR_NAME[] = "kodi-audio-worklet";
constexpr unsigned int BUFFER_TARGET_MS = 100;
constexpr unsigned int MIN_BUFFER_QUANTA = 2;
constexpr unsigned int PREBUFFER_TARGET_MS = 40;
constexpr unsigned int WORKLET_STALL_LIMIT_MS = 500;
constexpr auto ASYNC_TIMEOUT = std::chrono::seconds(5);

int CreateAudioContextOnMain(int requestedSampleRate)
{
  EmscriptenWebAudioCreateAttributes attrs{};
  attrs.latencyHint = "interactive";
  attrs.sampleRate = static_cast<uint32_t>(std::max(requestedSampleRate, 0));
  attrs.renderSizeHint = AUDIO_CONTEXT_RENDER_SIZE_DEFAULT;
  return emscripten_create_audio_context(&attrs);
}

int GetAudioContextSampleRateOnMain(int audioContext)
{
  return emscripten_audio_context_sample_rate(audioContext);
}

int GetAudioContextQuantumSizeOnMain(int audioContext)
{
  return emscripten_audio_context_quantum_size(audioContext);
}

int GetAudioContextLatencyUsOnMain(int audioContext)
{
  const double totalSeconds = EM_ASM_DOUBLE(
      {
        const ctx = emscriptenGetAudioObject($0);
        if (!ctx)
          return 0.0;
        let total = 0.0;
        if (typeof ctx.baseLatency === "number" && isFinite(ctx.baseLatency))
          total += ctx.baseLatency;
        if (typeof ctx.outputLatency === "number" && isFinite(ctx.outputLatency))
          total += ctx.outputLatency;
        return total;
      },
      audioContext);

  if (!(totalSeconds > 0.0))
    return 0;
  const double us = totalSeconds * 1'000'000.0;
  if (us > static_cast<double>(std::numeric_limits<int>::max()))
    return std::numeric_limits<int>::max();
  return static_cast<int>(us);
}

void StartWorkletThreadOnMain(int audioContext,
                              uintptr_t stackBase,
                              int stackSize,
                              uintptr_t callback,
                              uintptr_t userData)
{
  emscripten_start_wasm_audio_worklet_thread_async(
      audioContext, reinterpret_cast<void*>(stackBase), static_cast<uint32_t>(stackSize),
      reinterpret_cast<EmscriptenStartWebAudioWorkletCallback>(callback),
      reinterpret_cast<void*>(userData));
}

void CreateProcessorOnMain(int audioContext,
                           uintptr_t options,
                           uintptr_t callback,
                           uintptr_t userData)
{
  emscripten_create_wasm_audio_worklet_processor_async(
      audioContext, reinterpret_cast<const WebAudioWorkletProcessorCreateOptions*>(options),
      reinterpret_cast<EmscriptenWorkletProcessorCreatedCallback>(callback),
      reinterpret_cast<void*>(userData));
}

int CreateNodeOnMain(int audioContext,
                     uintptr_t name,
                     uintptr_t options,
                     uintptr_t processCallback,
                     uintptr_t userData)
{
  return emscripten_create_wasm_audio_worklet_node(
      audioContext, reinterpret_cast<const char*>(name),
      reinterpret_cast<const EmscriptenAudioWorkletNodeCreateOptions*>(options),
      reinterpret_cast<EmscriptenWorkletNodeProcessCallback>(processCallback),
      reinterpret_cast<void*>(userData));
}

void ConnectAudioNodeOnMain(int source, int destination, int outputIndex, int inputIndex)
{
  emscripten_audio_node_connect(source, destination, outputIndex, inputIndex);
}

void DestroyAudioContextOnMain(int audioContext)
{
  emscripten_destroy_audio_context(audioContext);
}

int QueryMaxOutputChannelsOnMain(int audioContext)
{
  return EM_ASM_INT(
      {
        let ctx = $0 ? emscriptenGetAudioObject($0) : null;
        const temporary = !ctx;
        try
        {
          if (!ctx)
            ctx = new AudioContext();
          return ctx.destination.maxChannelCount | 0;
        }
        catch (e)
        {
          return 0;
        }
        finally
        {
          if (temporary && ctx)
            ctx.close().catch(() => {});
        }
      },
      audioContext);
}

// AudioDestinationNode.channelCount defaults to 2 regardless of the hardware, which
// would fold a 5.1 node to stereo.
void SetDestinationChannelCountOnMain(int audioContext, int channels)
{
  EM_ASM(
      {
        const ctx = emscriptenGetAudioObject($0);
        if (!ctx)
          return;
        const dest = ctx.destination;
        try
        {
          dest.channelCount = Math.min($1, dest.maxChannelCount);
        }
        catch (e)
        {
          console.warn("WASM AudioWorklet: cannot set destination channelCount", e);
        }
      },
      audioContext, channels);
}

void DestroyAudioNodeOnMain(int audioNode)
{
  emscripten_destroy_web_audio_node(audioNode);
}

void ClearResumeHooksOnMain(int audioContext)
{
  EM_ASM(
      {
        const ctx = emscriptenGetAudioObject($0);
        if (!ctx || !ctx.__kodiResumeHandler)
          return;
        const handler = ctx.__kodiResumeHandler;
        window.removeEventListener("pointerdown", handler, true);
        window.removeEventListener("keydown", handler, true);
        window.removeEventListener("touchstart", handler, true);
        document.removeEventListener("visibilitychange", handler, true);
        ctx.__kodiResumeHandler = null;
        ctx.__kodiResumeHooksInstalled = false;
      },
      audioContext);
}

} // namespace

namespace KODI::PLATFORM::WASM
{
CWasmAudioWorkletManager& CWasmAudioWorkletManager::Instance()
{
  static CWasmAudioWorkletManager instance;
  return instance;
}

bool CWasmAudioWorkletManager::Initialize(unsigned int channels, unsigned int requestedSampleRate)
{
  if (channels == 0 || channels > kMaxChannels)
  {
    CLog::Log(LOGERROR, "WASM AudioWorklet: unsupported channel count {}", channels);
    return false;
  }

  m_ready.store(false, std::memory_order_release);

  if (!EnsureContext(requestedSampleRate))
  {
    Shutdown();
    return false;
  }

  if (!EnsureWorkletProcessor())
  {
    Shutdown();
    return false;
  }

  if (!ConfigureNode(channels))
  {
    Shutdown();
    return false;
  }

  EnsureBufferAllocated();
  ResetBuffer();
  InstallResumeHooks();
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
  const uint64_t currentWrite = m_writeFrame.load(std::memory_order_acquire);
  m_readFrame.store(currentWrite, std::memory_order_release);
  m_underrunFrames.store(0, std::memory_order_relaxed);
  m_prebufferComplete.store(false, std::memory_order_release);
}

void CWasmAudioWorkletManager::Shutdown()
{
  WaitForWorkletIdle();

  if (m_workletNode != 0)
  {
    emscripten_sync_run_in_main_runtime_thread(EM_FUNC_SIG_VI, DestroyAudioNodeOnMain,
                                               m_workletNode);
    m_workletNode = 0;
  }
  if (m_audioContext != 0)
  {
    ClearResumeHooks();
    emscripten_sync_run_in_main_runtime_thread(EM_FUNC_SIG_VI, DestroyAudioContextOnMain,
                                               m_audioContext);
    m_audioContext = 0;
  }

  m_workletThreadCreated.store(false, std::memory_order_release);
  m_processorCreated.store(false, std::memory_order_release);
  m_bufferCapacityFrames = 0;
  m_bufferChannels = 0;
  m_bufferSampleRate = 0;
  m_bufferQuantumSize = 0;
  m_prebufferFrames = 0;
  m_prebufferComplete.store(false, std::memory_order_release);
  m_ringBuffer.clear();
  m_ringBuffer.shrink_to_fit();
  m_readFrame.store(0, std::memory_order_release);
  m_writeFrame.store(0, std::memory_order_release);
  m_pipelineLatencyUs.store(0, std::memory_order_relaxed);
  m_underrunFrames.store(0, std::memory_order_relaxed);
}

unsigned int CWasmAudioWorkletManager::WritePlanar(const float* const* planes,
                                                   unsigned int channels,
                                                   unsigned int frames,
                                                   unsigned int offsetFrames)
{

  if (!planes || frames == 0 || !IsReady())
  {
    return 0;
  }

  const unsigned int configuredChannels = m_channels.load(std::memory_order_relaxed);
  if (configuredChannels == 0 || configuredChannels > kMaxChannels)
    return 0;

  const unsigned int copyChannels = std::min(channels, configuredChannels);
  if (copyChannels == 0)
    return 0;

  const int quantumMs = QuantumSleepMs();

  unsigned int writtenFrames = 0;
  unsigned int stalledMs = 0;
  while (writtenFrames < frames)
  {
    if (!IsReady())
      return writtenFrames;

    const unsigned int capacityFrames = m_bufferCapacityFrames;
    if (capacityFrames == 0 || m_ringBuffer.empty())
      return writtenFrames;

    const uint64_t readFrame = m_readFrame.load(std::memory_order_acquire);
    const uint64_t writeFrame = m_writeFrame.load(std::memory_order_relaxed);
    const uint64_t usedFrames = writeFrame - readFrame;
    if (usedFrames >= capacityFrames)
    {
      stalledMs += static_cast<unsigned int>(quantumMs);
      if (stalledMs >= WORKLET_STALL_LIMIT_MS)
        return writtenFrames;
      emscripten_thread_sleep(quantumMs);
      continue;
    }
    stalledMs = 0;

    const uint64_t freeFrames = capacityFrames - usedFrames;
    const uint64_t toWrite = std::min<uint64_t>(frames - writtenFrames, freeFrames);
    const uint64_t dstStart = writeFrame % capacityFrames;
    const uint64_t firstChunk = std::min<uint64_t>(toWrite, capacityFrames - dstStart);
    const uint64_t secondChunk = toWrite - firstChunk;

    for (unsigned int ch = 0; ch < copyChannels; ++ch)
    {
      if (!planes[ch])
        return writtenFrames;
      float* const dstPlane = &m_ringBuffer[static_cast<size_t>(ch) * capacityFrames];
      const float* const srcPlane = planes[ch] + offsetFrames + writtenFrames;
      std::memcpy(dstPlane + dstStart, srcPlane, firstChunk * sizeof(float));
      if (secondChunk > 0)
        std::memcpy(dstPlane, srcPlane + firstChunk, secondChunk * sizeof(float));
    }
    // Zero any ring planes the writer did not fill (e.g. upmix headroom) so
    // stale data never leaks into the browser output.
    for (unsigned int ch = copyChannels; ch < configuredChannels; ++ch)
    {
      float* const dstPlane = &m_ringBuffer[static_cast<size_t>(ch) * capacityFrames];
      std::memset(dstPlane + dstStart, 0, firstChunk * sizeof(float));
      if (secondChunk > 0)
        std::memset(dstPlane, 0, secondChunk * sizeof(float));
    }

    m_writeFrame.store(writeFrame + toWrite, std::memory_order_release);
    writtenFrames += static_cast<unsigned int>(toWrite);
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
  const int quantumMs = QuantumSleepMs();
  uint64_t lastReadFrame = m_readFrame.load(std::memory_order_acquire);
  unsigned int stalledMs = 0;

  while (IsReady() && GetBufferedSeconds() > 0.0)
  {
    emscripten_thread_sleep(quantumMs);

    const uint64_t readFrame = m_readFrame.load(std::memory_order_acquire);
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
      return;
  }
}

double CWasmAudioWorkletManager::GetBufferedSeconds() const
{
  const unsigned int rate = m_sampleRate.load(std::memory_order_relaxed);
  if (rate == 0)
    return 0.0;

  const uint64_t readFrame = m_readFrame.load(std::memory_order_acquire);
  const uint64_t writeFrame = m_writeFrame.load(std::memory_order_relaxed);
  const uint64_t queuedFrames = writeFrame - readFrame;
  return static_cast<double>(queuedFrames) / static_cast<double>(rate);
}

double CWasmAudioWorkletManager::GetBufferCapacitySeconds() const
{
  const unsigned int rate = m_sampleRate.load(std::memory_order_relaxed);
  if (rate == 0 || m_bufferCapacityFrames == 0)
    return 0.0;

  return static_cast<double>(m_bufferCapacityFrames) / static_cast<double>(rate);
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
  const int channels = emscripten_sync_run_in_main_runtime_thread(
      EM_FUNC_SIG_II, QueryMaxOutputChannelsOnMain, m_audioContext);
  return channels > 0 ? static_cast<unsigned int>(channels) : 2U;
}

// seq_cst pairs with ProcessAudio(), which increments m_activeCallbacks before loading
// m_ready: either the worklet sees the flag or we see the callback and wait for it.
void CWasmAudioWorkletManager::WaitForWorkletIdle()
{
  m_ready.store(false, std::memory_order_seq_cst);
  for (unsigned int i = 0; i < 100 && m_activeCallbacks.load(std::memory_order_seq_cst) > 0; ++i)
    emscripten_thread_sleep(1);
}

bool CWasmAudioWorkletManager::IsReady() const
{
  return m_ready.load(std::memory_order_acquire);
}

bool CWasmAudioWorkletManager::EnsureContext(unsigned int requestedSampleRate)
{
  if (m_audioContext != 0)
    return true;

  m_audioContext = emscripten_sync_run_in_main_runtime_thread(
      EM_FUNC_SIG_II, CreateAudioContextOnMain, static_cast<int>(requestedSampleRate));

  if (m_audioContext == 0)
  {
    CLog::Log(LOGERROR, "WASM AudioWorklet: failed to create AudioContext");
    return false;
  }

  const int sampleRate = emscripten_sync_run_in_main_runtime_thread(
      EM_FUNC_SIG_II, GetAudioContextSampleRateOnMain, m_audioContext);
  const int quantumSize = emscripten_sync_run_in_main_runtime_thread(
      EM_FUNC_SIG_II, GetAudioContextQuantumSizeOnMain, m_audioContext);

  m_sampleRate.store(static_cast<unsigned int>(sampleRate), std::memory_order_relaxed);
  m_quantumSize.store(static_cast<unsigned int>(quantumSize), std::memory_order_relaxed);

  return true;
}

bool CWasmAudioWorkletManager::EnsureWorkletProcessor()
{
  if (m_audioContext == 0)
    return false;

  alignas(16) static std::array<uint8_t, 16384> workletStack{};

  if (!m_workletThreadCreated.load(std::memory_order_acquire))
  {
    m_asyncDone.store(false, std::memory_order_release);
    m_asyncResult.store(false, std::memory_order_release);

    emscripten_sync_run_in_main_runtime_thread(
        EM_FUNC_SIG_VIIIII, StartWorkletThreadOnMain, m_audioContext,
        reinterpret_cast<uintptr_t>(workletStack.data()), static_cast<int>(workletStack.size()),
        reinterpret_cast<uintptr_t>(&CWasmAudioWorkletManager::OnWorkletThreadStarted),
        reinterpret_cast<uintptr_t>(this));

    if (!WaitForState(m_asyncDone, "thread"))
      return false;

    if (!m_asyncResult.load(std::memory_order_acquire))
    {
      CLog::Log(LOGERROR, "WASM AudioWorklet: failed to create worklet thread");
      return false;
    }
    m_workletThreadCreated.store(true, std::memory_order_release);
  }

  if (!m_processorCreated.load(std::memory_order_acquire))
  {
    WebAudioWorkletProcessorCreateOptions opts{};
    opts.name = AUDIO_PROCESSOR_NAME;

    m_asyncDone.store(false, std::memory_order_release);
    m_asyncResult.store(false, std::memory_order_release);
    emscripten_sync_run_in_main_runtime_thread(
        EM_FUNC_SIG_VIIII, CreateProcessorOnMain, m_audioContext,
        reinterpret_cast<uintptr_t>(&opts),
        reinterpret_cast<uintptr_t>(&CWasmAudioWorkletManager::OnProcessorCreated),
        reinterpret_cast<uintptr_t>(this));

    if (!WaitForState(m_asyncDone, "processor"))
      return false;

    if (!m_asyncResult.load(std::memory_order_acquire))
    {
      CLog::Log(LOGERROR, "WASM AudioWorklet: failed to create processor");
      return false;
    }
    m_processorCreated.store(true, std::memory_order_release);
  }

  return true;
}

bool CWasmAudioWorkletManager::ConfigureNode(unsigned int channels)
{
  m_channels.store(channels, std::memory_order_release);

  if (m_workletNode != 0)
  {
    emscripten_sync_run_in_main_runtime_thread(EM_FUNC_SIG_VI, DestroyAudioNodeOnMain,
                                               m_workletNode);
    m_workletNode = 0;
  }

  int outputChannelCounts[1] = {static_cast<int>(channels)};
  EmscriptenAudioWorkletNodeCreateOptions options{};
  options.numberOfInputs = 0;
  options.numberOfOutputs = 1;
  options.outputChannelCounts = outputChannelCounts;
  options.channelCount = channels;
  options.channelCountMode = WEBAUDIO_CHANNEL_COUNT_MODE_EXPLICIT;
  options.channelInterpretation = WEBAUDIO_CHANNEL_INTERPRETATION_SPEAKERS;

  m_workletNode = emscripten_sync_run_in_main_runtime_thread(
      EM_FUNC_SIG_IIIIII, CreateNodeOnMain, m_audioContext,
      reinterpret_cast<uintptr_t>(AUDIO_PROCESSOR_NAME), reinterpret_cast<uintptr_t>(&options),
      reinterpret_cast<uintptr_t>(reinterpret_cast<EmscriptenWorkletNodeProcessCallback>(
          &CWasmAudioWorkletManager::ProcessAudio)),
      reinterpret_cast<uintptr_t>(this));
  if (m_workletNode == 0)
  {
    CLog::Log(LOGERROR, "WASM AudioWorklet: failed to create node");
    return false;
  }

  emscripten_sync_run_in_main_runtime_thread(EM_FUNC_SIG_VII, SetDestinationChannelCountOnMain,
                                             m_audioContext, static_cast<int>(channels));
  emscripten_sync_run_in_main_runtime_thread(EM_FUNC_SIG_VIIII, ConnectAudioNodeOnMain,
                                             m_workletNode, m_audioContext, 0, 0);
  RefreshPipelineLatency();
  return true;
}

void CWasmAudioWorkletManager::RefreshPipelineLatency()
{
  if (m_audioContext == 0)
    return;

  const int latencyUs = emscripten_sync_run_in_main_runtime_thread(
      EM_FUNC_SIG_II, GetAudioContextLatencyUsOnMain, m_audioContext);
  if (latencyUs < 0)
    return;

  m_pipelineLatencyUs.store(static_cast<uint32_t>(latencyUs), std::memory_order_relaxed);
}

void CWasmAudioWorkletManager::InstallResumeHooks() const
{
  MAIN_THREAD_ASYNC_EM_ASM(
      ({
        const ctx = emscriptenGetAudioObject($0);
        if (!ctx)
          return;
        if (ctx.state === "running")
          return;
        if (ctx.__kodiResumeHooksInstalled)
          return;

        ctx.__kodiResumeHooksInstalled = true;
        const tryResume = () => {
          if (ctx.state !== "running")
            ctx.resume().catch(() => {});
          if (ctx.state === "running")
          {
            window.removeEventListener("pointerdown", tryResume, true);
            window.removeEventListener("keydown", tryResume, true);
            window.removeEventListener("touchstart", tryResume, true);
            document.removeEventListener("visibilitychange", tryResume, true);
            ctx.__kodiResumeHooksInstalled = false;
            ctx.__kodiResumeHandler = null;
          }
        };
        ctx.__kodiResumeHandler = tryResume;

        window.addEventListener("pointerdown", tryResume, true);
        window.addEventListener("keydown", tryResume, true);
        window.addEventListener("touchstart", tryResume, true);
        document.addEventListener("visibilitychange", tryResume, true);
      }),
      m_audioContext);
}

void CWasmAudioWorkletManager::ClearResumeHooks() const
{
  if (m_audioContext == 0)
    return;

  emscripten_sync_run_in_main_runtime_thread(EM_FUNC_SIG_VI, ClearResumeHooksOnMain,
                                             m_audioContext);
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
  if (m_bufferCapacityFrames == capacityFrames && m_bufferChannels == channels &&
      m_bufferSampleRate == sampleRate && m_bufferQuantumSize == quantumSize)
    return;

  WaitForWorkletIdle();
  m_bufferCapacityFrames = capacityFrames;
  m_bufferChannels = channels;
  m_bufferSampleRate = sampleRate;
  m_bufferQuantumSize = quantumSize;
  const size_t sampleCount = static_cast<size_t>(capacityFrames) * channels;
  m_ringBuffer.assign(sampleCount, 0.0f);

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

bool CWasmAudioWorkletManager::WaitForState(std::atomic<bool>& readyFlag, const char* stageName)
{
  std::unique_lock<std::mutex> lock(m_stateMutex);
  const bool completed =
      m_stateCv.wait_for(lock, ASYNC_TIMEOUT, [&readyFlag] { return readyFlag.load(); });
  if (!completed)
  {
    CLog::Log(LOGERROR, "WASM AudioWorklet: timeout waiting for {}", stageName);
    return false;
  }
  return true;
}

void CWasmAudioWorkletManager::OnWorkletThreadStarted(int audioContext,
                                                      bool success,
                                                      void* userData)
{
  auto* self = static_cast<CWasmAudioWorkletManager*>(userData);
  if (!self)
    return;

  self->m_asyncResult.store(success && audioContext != 0, std::memory_order_release);
  self->m_asyncDone.store(true, std::memory_order_release);
  self->m_stateCv.notify_all();
}

void CWasmAudioWorkletManager::OnProcessorCreated(int, bool success, void* userData)
{
  auto* self = static_cast<CWasmAudioWorkletManager*>(userData);
  if (!self)
    return;

  self->m_asyncResult.store(success, std::memory_order_release);
  self->m_asyncDone.store(true, std::memory_order_release);
  self->m_stateCv.notify_all();
}

bool CWasmAudioWorkletManager::ProcessAudio(
    int, const void*, int numOutputs, void* outputs, int, const void*, void* userData)
{
  auto* self = static_cast<CWasmAudioWorkletManager*>(userData);
  if (!self)
    return false;

  self->m_activeCallbacks.fetch_add(1, std::memory_order_seq_cst);
  const bool result = self->ProcessAudioImpl(numOutputs, outputs);
  self->m_activeCallbacks.fetch_sub(1, std::memory_order_seq_cst);
  return result;
}

bool CWasmAudioWorkletManager::ProcessAudioImpl(int numOutputs, void* outputsRaw)
{

  auto* outputs = static_cast<AudioSampleFrame*>(outputsRaw);
  if (numOutputs <= 0 || !outputs)
  {
    return true;
  }

  const int samplesPerChannel = outputs[0].samplesPerChannel;
  if (samplesPerChannel <= 0 || !outputs[0].data)
    return true;

  const int outputChannels = outputs[0].numberOfChannels;
  if (outputChannels <= 0)
    return true;

  float* outputData = outputs[0].data;

  auto zeroOutput = [&]()
  {
    const int totalSamples = outputChannels * samplesPerChannel;
    std::fill(outputData, outputData + totalSamples, 0.0f);
  };

  if (!m_ready.load(std::memory_order_seq_cst))
  {
    zeroOutput();
    return true;
  }

  const unsigned int channels = m_channels.load(std::memory_order_relaxed);
  const unsigned int capacityFrames = m_bufferCapacityFrames;
  if (channels == 0 || capacityFrames == 0 || m_ringBuffer.empty())
  {
    zeroOutput();
    return true;
  }

  const unsigned int copyChannels = std::min<unsigned int>(channels, outputChannels);

  const uint64_t readFrame = m_readFrame.load(std::memory_order_relaxed);
  const uint64_t writeFrame = m_writeFrame.load(std::memory_order_acquire);
  const uint64_t availableFrames = writeFrame - readFrame;
  const uint64_t wantedFrames = static_cast<uint64_t>(samplesPerChannel);

  if (!m_prebufferComplete.load(std::memory_order_acquire))
  {
    if (m_prebufferFrames == 0 || availableFrames >= m_prebufferFrames)
      m_prebufferComplete.store(true, std::memory_order_release);
    else
    {
      zeroOutput();
      return true;
    }
  }

  const uint64_t toRead = std::min<uint64_t>(availableFrames, wantedFrames);
  if (toRead < wantedFrames)
    m_underrunFrames.fetch_add(wantedFrames - toRead, std::memory_order_relaxed);
  if (toRead == 0)
  {
    zeroOutput();
    return true;
  }

  // Planar memcpy per channel: ring buffer is laid out as
  // [ch0 capacityFrames | ch1 capacityFrames | ...]. The browser's output
  // buffer follows the same planar convention (outputs[0].data[ch*N + frame]).
  // No per-sample reshuffling.
  const uint64_t srcStart = readFrame % capacityFrames;
  const uint64_t firstChunk = std::min<uint64_t>(toRead, capacityFrames - srcStart);
  const uint64_t secondChunk = toRead - firstChunk;
  for (unsigned int ch = 0; ch < copyChannels; ++ch)
  {
    const float* const srcPlane = &m_ringBuffer[static_cast<size_t>(ch) * capacityFrames];
    float* const dstPlane = outputData + static_cast<size_t>(ch) * samplesPerChannel;
    std::memcpy(dstPlane, srcPlane + srcStart, firstChunk * sizeof(float));
    if (secondChunk > 0)
      std::memcpy(dstPlane + firstChunk, srcPlane, secondChunk * sizeof(float));

    if (toRead < wantedFrames)
      std::fill(dstPlane + toRead, dstPlane + wantedFrames, 0.0f);
  }

  for (unsigned int ch = copyChannels; ch < static_cast<unsigned int>(outputChannels); ++ch)
  {
    float* const dstPlane = outputData + static_cast<size_t>(ch) * samplesPerChannel;
    std::fill(dstPlane, dstPlane + wantedFrames, 0.0f);
  }

  m_readFrame.store(readFrame + toRead, std::memory_order_release);
  return true;
}
} // namespace KODI::PLATFORM::WASM
