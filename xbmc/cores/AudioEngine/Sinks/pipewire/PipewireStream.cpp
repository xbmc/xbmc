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

#include <stdexcept>

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

CPipewireStream::~CPipewireStream()
{
  spa_hook_remove(&m_streamListener);
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

void CPipewireStream::SetActive(bool active)
{
  pw_stream_set_active(m_stream.get(), active);
}

pw_buffer* CPipewireStream::DequeueBuffer()
{
  return pw_stream_dequeue_buffer(m_stream.get());
}

void CPipewireStream::QueueBuffer(pw_buffer* buffer)
{
  pw_stream_queue_buffer(m_stream.get(), buffer);
}

bool CPipewireStream::TriggerProcess() const
{
  int ret = pw_stream_trigger_process(m_stream.get());
  if (ret < 0)
  {
    CLog::Log(LOGERROR, "CPipewireStream: failed to trigger process: {}", spa_strerror(errno));
    return false;
  }

  return true;
}

void CPipewireStream::Flush(bool drain)
{
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
    CLog::Log(LOGDEBUG, "CPipewireStream::{} - stream node {} error: {}", __FUNCTION__,
              stream.GetNodeId(), error);

  loop.Signal(false);
}

void CPipewireStream::Process(void* userdata)
{
  auto& stream = *reinterpret_cast<CPipewireStream*>(userdata);
  auto& loop = stream.GetCore().GetContext().GetThreadLoop();

  loop.Signal(false);
}

void CPipewireStream::Drained(void* userdata)
{
  auto& stream = *reinterpret_cast<CPipewireStream*>(userdata);
  auto& loop = stream.GetCore().GetContext().GetThreadLoop();

  stream.SetActive(false);

  CLog::Log(LOGDEBUG, "CPipewireStream::{}", __FUNCTION__);

  loop.Signal(false);
}

pw_stream_events CPipewireStream::CreateStreamEvents()
{
  pw_stream_events streamEvents = {};
  streamEvents.version = PW_VERSION_STREAM_EVENTS;
  streamEvents.state_changed = StateChanged;
  streamEvents.process = Process;
  streamEvents.drained = Drained;

  return streamEvents;
}
