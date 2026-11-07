/*
 *  Copyright (C) 2010-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/AudioEngine/Utils/AERingBuffer.h"

#include <atomic>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <vector>

#include <pipewire/core.h>
#include <pipewire/stream.h>

struct spa_pod;

namespace KODI
{
namespace PIPEWIRE
{

class CPipewireCore;

class CPipewireStream
{
public:
  explicit CPipewireStream(CPipewireCore& core);
  CPipewireStream() = delete;
  ~CPipewireStream();

  pw_stream* Get() const { return m_stream.get(); }

  CPipewireCore& GetCore() const { return m_core; }

  bool Connect(uint32_t id,
               const pw_direction& direction,
               std::vector<const spa_pod*>& params,
               const pw_stream_flags& flags);

  pw_stream_state GetState();
  pw_stream_state GetState(const char** error);
  bool HasInitError() const { return m_initError; }
  bool HasStreamError() const { return m_streamError; }
  void SetActive(bool active);

  void Flush(bool drain);

  uint32_t GetNodeId();

  void UpdateProperties(spa_dict* dict);

  pw_time GetTime() const;

  using ParamChangedCallback = std::function<void(uint32_t id, const spa_pod* param)>;
  void SetParamChangedCallback(ParamChangedCallback callback);
  bool IsFormatNegotiated() const { return m_formatNegotiated; }
  int UpdateParams(const spa_pod** params, uint32_t n_params);

  // Ring buffer interface for RT_PROCESS data delivery
  void InitRingBuffer(uint32_t size, uint32_t frameSize);
  unsigned int Write(const uint8_t* data, unsigned int frames);
  bool WaitForSpace(std::chrono::milliseconds timeout);
  unsigned int GetRingBufferReadSize() const;

private:
  static void StateChanged(void* userdata,
                           enum pw_stream_state old,
                           enum pw_stream_state state,
                           const char* error);
  static void ParamChanged(void* userdata, uint32_t id, const struct spa_pod* param);
  static void Process(void* userdata);
  static void Drained(void* userdata);
  void Stop();

  static pw_stream_events CreateStreamEvents();

  CPipewireCore& m_core;

  const pw_stream_events m_streamEvents;

  spa_hook m_streamListener;

  bool m_initError{false};
  bool m_formatNegotiated{false};
  std::atomic<bool> m_streamError{false};
  ParamChangedCallback m_paramChangedCallback;

  // Ring buffer for decoupling audio engine thread from PipeWire RT thread
  std::unique_ptr<AERingBuffer> m_ringBuffer;
  std::mutex m_mutex;
  std::condition_variable m_dataConsumed; // signaled when on_process drains data
  std::atomic<bool> m_running{false};
  uint32_t m_frameSize{0};

  struct PipewireStreamDeleter
  {
    void operator()(pw_stream* p) { pw_stream_destroy(p); }
  };

  std::unique_ptr<pw_stream, PipewireStreamDeleter> m_stream;
};

} // namespace PIPEWIRE
} // namespace KODI
