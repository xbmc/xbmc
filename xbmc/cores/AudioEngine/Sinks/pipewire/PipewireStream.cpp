/*
 *  Copyright (C) 2010-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PipewireStream.h"

#include "PipewireContext.h"
#include "PipewireCore.h"
#include "PipewireThreadLoop.h"
#include "utils/log.h"

#include <cstring>
#include <stdexcept>

#include <spa/param/param.h>
#include <spa/utils/result.h>

using namespace KODI;
using namespace PIPEWIRE;

CPipewireStream::CPipewireStream(CPipewireCore& core)
  : m_core(core), m_streamEvents(CreateStreamEvents())
{
  m_stream.reset(pw_stream_new(core.Get(), nullptr, pw_properties_new(nullptr, nullptr)));
  if (!m_stream)
  {
    CLog::Log(LOGERROR, "CPipewireStream: failed to create stream: {}", strerror(errno));
    throw std::runtime_error("CPipewireStream: failed to create stream");
  }

  pw_stream_add_listener(m_stream.get(), &m_streamListener, &m_streamEvents, this);
}

void CPipewireStream::Stop()
{
  m_running = false;
  m_dataConsumed.notify_all();
}

CPipewireStream::~CPipewireStream()
{
  spa_hook_remove(&m_streamListener);
  Stop();
}

bool CPipewireStream::Connect(uint32_t id,
                              const pw_direction& direction,
                              std::vector<const spa_pod*>& params,
                              const pw_stream_flags& flags)
{
  int ret = pw_stream_connect(m_stream.get(), direction, id, flags, params.data(), params.size());
  if (ret < 0)
  {
    CLog::Log(LOGERROR, "CPipewireStream: failed to connect stream: {}", spa_strerror(errno));
    return false;
  }

  return true;
}

pw_stream_state CPipewireStream::GetState()
{
  return pw_stream_get_state(m_stream.get(), nullptr);
}

pw_stream_state CPipewireStream::GetState(const char** error)
{
  return pw_stream_get_state(m_stream.get(), error);
}

void CPipewireStream::SetActive(bool active)
{
  if (active)
  {
    // Reset ring buffer under lock (as documented in AERingBuffer.h)
    {
      std::lock_guard<std::mutex> lock(m_mutex);
      if (m_ringBuffer)
        m_ringBuffer->Reset();
    }
    m_streamError = false;
    m_running = true;
  }
  else
  {
    Stop();
  }
  pw_stream_set_active(m_stream.get(), active);
}

void CPipewireStream::Flush(bool drain)
{
  Stop();
  pw_stream_flush(m_stream.get(), drain);
}

uint32_t CPipewireStream::GetNodeId()
{
  return pw_stream_get_node_id(m_stream.get());
}

void CPipewireStream::UpdateProperties(spa_dict* dict)
{
  pw_stream_update_properties(m_stream.get(), dict);
}

pw_time CPipewireStream::GetTime() const
{
  pw_time time;
  pw_stream_get_time_n(m_stream.get(), &time, sizeof(time));

  return time;
}

// Ring buffer methods

void CPipewireStream::InitRingBuffer(uint32_t size, uint32_t frameSize)
{
  m_frameSize = frameSize;
  m_ringBuffer = std::make_unique<AERingBuffer>(size);
}

unsigned int CPipewireStream::Write(const uint8_t* data, unsigned int frames)
{
  std::lock_guard<std::mutex> lock(m_mutex);
  if (!m_ringBuffer)
    return 0;

  uint32_t bytesToWrite = frames * m_frameSize;
  uint32_t space = m_ringBuffer->GetWriteSize();
  uint32_t toWrite = std::min(bytesToWrite, space);
  uint32_t writtenFrames = toWrite / m_frameSize;
  toWrite = writtenFrames * m_frameSize; // align to frame boundary

  if (writtenFrames > 0)
    m_ringBuffer->Write(const_cast<unsigned char*>(data), toWrite);

  return writtenFrames;
}

bool CPipewireStream::WaitForSpace(std::chrono::milliseconds timeout)
{
  std::unique_lock<std::mutex> lock(m_mutex);
  return m_dataConsumed.wait_for(lock, timeout, [this] {
    return !m_ringBuffer || m_ringBuffer->GetWriteSize() >= m_frameSize || !m_running ||
           m_streamError;
  });
}


unsigned int CPipewireStream::GetRingBufferReadSize() const
{
  if (!m_ringBuffer)
    return 0;
  return m_ringBuffer->GetReadSize();
}

// PipeWire callbacks

void CPipewireStream::StateChanged(void* userdata,
                                   enum pw_stream_state old,
                                   enum pw_stream_state state,
                                   const char* error)
{
  auto& stream = *reinterpret_cast<CPipewireStream*>(userdata);
  auto& loop = stream.GetCore().GetContext().GetThreadLoop();

  CLog::Log(LOGDEBUG, "CPipewireStream::{} - stream state changed {} -> {}", __FUNCTION__,
            pw_stream_state_as_string(old), pw_stream_state_as_string(state));

  if (state == PW_STREAM_STATE_STREAMING)
    CLog::Log(LOGDEBUG, "CPipewireStream::{} - stream node {}", __FUNCTION__, stream.GetNodeId());

  if (state == PW_STREAM_STATE_ERROR)
  {
    stream.m_initError = true;
    stream.m_streamError = true;
    CLog::Log(LOGWARNING, "CPipewireStream::{} - stream error: {}", __FUNCTION__,
              error ? error : "unknown");
    // Unblock WaitForSpace so AddPackets returns immediately
    stream.m_dataConsumed.notify_all();
  }

  if (state == PW_STREAM_STATE_UNCONNECTED && old != PW_STREAM_STATE_UNCONNECTED)
  {
    stream.m_streamError = true;
    CLog::Log(LOGWARNING, "CPipewireStream::{} - stream disconnected", __FUNCTION__);
    stream.m_dataConsumed.notify_all();
  }

  loop.Signal(false);
}

void CPipewireStream::ParamChanged(void* userdata, uint32_t id, const struct spa_pod* param)
{
  auto& stream = *reinterpret_cast<CPipewireStream*>(userdata);
  auto& loop = stream.GetCore().GetContext().GetThreadLoop();

  if (id == SPA_PARAM_Format && param != nullptr)
  {
    CLog::Log(LOGDEBUG, "CPipewireStream::{} - format negotiated", __FUNCTION__);

    if (stream.m_paramChangedCallback)
      stream.m_paramChangedCallback(id, param);

    stream.m_formatNegotiated = true;
  }

  // Latency param indicates the stream node is linked (mpv pattern)
  if (id == SPA_PARAM_Latency)
    loop.Signal(false);
}

void CPipewireStream::Process(void* userdata)
{
  auto& stream = *reinterpret_cast<CPipewireStream*>(userdata);

  pw_buffer* b = pw_stream_dequeue_buffer(stream.m_stream.get());
  if (!b)
    return;

  spa_buffer* buf = b->buffer;
  spa_data* d = &buf->datas[0];

  if (stream.m_frameSize == 0 || !d->data)
  {
    pw_stream_queue_buffer(stream.m_stream.get(), b);
    return;
  }

  uint32_t maxFrames = d->maxsize / stream.m_frameSize;
  uint32_t requested = b->requested ? b->requested : maxFrames;
  uint32_t target = std::min(requested, maxFrames);

  uint32_t toRead = 0;
  {
    std::lock_guard<std::mutex> lock(stream.m_mutex);
    if (stream.m_ringBuffer)
    {
      uint32_t available = stream.m_ringBuffer->GetReadSize() / stream.m_frameSize;
      toRead = std::min(target, available);

      if (toRead > 0)
        stream.m_ringBuffer->Read(static_cast<unsigned char*>(d->data),
                                  toRead * stream.m_frameSize);
    }
  }

  // Fill remainder with silence to prevent graph stall
  if (toRead < target)
  {
    std::memset(static_cast<uint8_t*>(d->data) + toRead * stream.m_frameSize, 0,
                (target - toRead) * stream.m_frameSize);
    toRead = target;
  }

  d->chunk->offset = 0;
  d->chunk->stride = stream.m_frameSize;
  d->chunk->size = toRead * stream.m_frameSize;
  b->size = toRead;

  pw_stream_queue_buffer(stream.m_stream.get(), b);

  // Signal AddPackets that space is available in the ring buffer
  stream.m_dataConsumed.notify_one();
}

void CPipewireStream::Drained(void* userdata)
{
  auto& stream = *reinterpret_cast<CPipewireStream*>(userdata);
  auto& loop = stream.GetCore().GetContext().GetThreadLoop();

  stream.SetActive(false);

  CLog::Log(LOGDEBUG, "CPipewireStream::{}", __FUNCTION__);

  loop.Signal(false);
}

void CPipewireStream::SetParamChangedCallback(ParamChangedCallback callback)
{
  m_paramChangedCallback = std::move(callback);
}

int CPipewireStream::UpdateParams(const spa_pod** params, uint32_t n_params)
{
  return pw_stream_update_params(m_stream.get(), params, n_params);
}

pw_stream_events CPipewireStream::CreateStreamEvents()
{
  pw_stream_events streamEvents = {};
  streamEvents.version = PW_VERSION_STREAM_EVENTS;
  streamEvents.state_changed = StateChanged;
  streamEvents.param_changed = ParamChanged;
  streamEvents.process = Process;
  streamEvents.drained = Drained;

  return streamEvents;
}
