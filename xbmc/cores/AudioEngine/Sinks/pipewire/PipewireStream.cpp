/*
 *  Copyright (C) 2010-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PipewireStream.h"

#include "cores/AudioEngine/Sinks/pipewire/Pipewire.h"
#include "cores/AudioEngine/Sinks/pipewire/PipewireThreadLoop.h"
#include "utils/log.h"

#include <stdexcept>

#include <spa/param/audio/format-utils.h>
#include <spa/pod/builder.h>
#include <spa/utils/result.h>

namespace AE
{
namespace SINK
{
namespace PIPEWIRE
{

CPipewireStream::CPipewireStream(pw_core* core) : m_streamEvents(CreateStreamEvents())
{
  m_stream.reset(pw_stream_new(core, nullptr, pw_properties_new(nullptr, nullptr)));
  if (!m_stream)
  {
    CLog::Log(LOGERROR, "CPipewireStream: failed to create stream: {}", strerror(errno));
    throw std::runtime_error("CPipewireStream: failed to create stream");
  }
}

CPipewireStream::~CPipewireStream()
{
  spa_hook_remove(&m_streamListener);
}

void CPipewireStream::AddListener(void* userdata)
{
  pw_stream_add_listener(m_stream.get(), &m_streamListener, &m_streamEvents, userdata);
}

bool CPipewireStream::Connect(uint32_t id, spa_audio_info_raw& info)
{
  std::array<uint8_t, 1024> buffer;
  auto builder = SPA_POD_BUILDER_INIT(buffer.data(), buffer.size());
  auto params = spa_format_audio_raw_build(&builder, SPA_PARAM_EnumFormat, &info);

  int ret = pw_stream_connect(m_stream.get(), PW_DIRECTION_OUTPUT, id,
                              static_cast<pw_stream_flags>(PW_STREAM_FLAG_AUTOCONNECT |
                                                           PW_STREAM_FLAG_INACTIVE |
                                                           PW_STREAM_FLAG_MAP_BUFFERS),
                              const_cast<const spa_pod**>(&params), 1);

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


void CPipewireStream::StateChanged(void* userdata,
                                   enum pw_stream_state old,
                                   enum pw_stream_state state,
                                   const char* error)
{
  auto pipewire = reinterpret_cast<CPipewire*>(userdata);
  auto loop = pipewire->GetThreadLoop();
  auto stream = pipewire->GetStream();

  CLog::Log(LOGDEBUG, "CPipewireStream::{} - stream state changed {} -> {}", __FUNCTION__,
            pw_stream_state_as_string(old), pw_stream_state_as_string(state));

  if (state == PW_STREAM_STATE_STREAMING)
    CLog::Log(LOGDEBUG, "CPipewireStream::{} - stream node {}", __FUNCTION__, stream->GetNodeId());

  if (state == PW_STREAM_STATE_ERROR)
    CLog::Log(LOGDEBUG, "CPipewireStream::{} - stream node {} error: {}", __FUNCTION__,
              stream->GetNodeId(), error);

  loop->Signal(false);
}

void CPipewireStream::Process(void* userdata)
{
  auto pipewire = reinterpret_cast<CPipewire*>(userdata);
  auto loop = pipewire->GetThreadLoop();

  loop->Signal(false);
}

void CPipewireStream::Drained(void* userdata)
{
  auto pipewire = reinterpret_cast<CPipewire*>(userdata);
  auto loop = pipewire->GetThreadLoop();
  auto stream = pipewire->GetStream();

  stream->SetActive(false);

  CLog::Log(LOGDEBUG, "CPipewireStream::{}", __FUNCTION__);

  loop->Signal(false);
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

} // namespace PIPEWIRE
} // namespace SINK
} // namespace AE
