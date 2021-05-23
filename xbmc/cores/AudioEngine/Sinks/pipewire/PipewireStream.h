/*
 *  Copyright (C) 2010-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <memory>

#include <pipewire/core.h>
#include <pipewire/stream.h>
#include <spa/param/audio/raw.h>

namespace AE
{
namespace SINK
{
namespace PIPEWIRE
{

class CPipewireStream
{
public:
  explicit CPipewireStream(pw_core* core);
  CPipewireStream() = delete;
  ~CPipewireStream();

  pw_stream* Get() const { return m_stream.get(); }

  void AddListener(void* userdata);
  bool Connect(uint32_t id, spa_audio_info_raw& info);

  pw_stream_state GetState();
  void SetActive(bool active);

  pw_buffer* DequeueBuffer();
  void QueueBuffer(pw_buffer* buffer);

  void Flush(bool drain);

  uint32_t GetNodeId();

  void UpdateProperties(spa_dict* dict);

private:
  static void StateChanged(void* userdata,
                           enum pw_stream_state old,
                           enum pw_stream_state state,
                           const char* error);
  static void Process(void* userdata);
  static void Drained(void* userdata);

  const pw_stream_events m_streamEvents = {.version = PW_VERSION_STREAM_EVENTS,
                                           .destroy = nullptr,
                                           .state_changed = StateChanged,
                                           .control_info = nullptr,
                                           .io_changed = nullptr,
                                           .param_changed = nullptr,
                                           .add_buffer = nullptr,
                                           .remove_buffer = nullptr,
                                           .process = Process,
                                           .drained = Drained};

  spa_hook m_streamListener;

  struct PipewireStreamDeleter
  {
    void operator()(pw_stream* p) { pw_stream_destroy(p); }
  };

  std::unique_ptr<pw_stream, PipewireStreamDeleter> m_stream;
};

} // namespace PIPEWIRE
} // namespace SINK
} // namespace AE
