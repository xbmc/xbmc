/*
 *  Copyright (C) 2010-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <memory>
#include <vector>

#include <pipewire/core.h>
#include <pipewire/stream.h>

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
  void SetActive(bool active);

  pw_buffer* DequeueBuffer();
  void QueueBuffer(pw_buffer* buffer);

  bool TriggerProcess() const;

  void Flush(bool drain);

  uint32_t GetNodeId();

  void UpdateProperties(spa_dict* dict);

  pw_time GetTime() const;

private:
  static void StateChanged(void* userdata,
                           enum pw_stream_state old,
                           enum pw_stream_state state,
                           const char* error);
  static void Process(void* userdata);
  static void Drained(void* userdata);

  static pw_stream_events CreateStreamEvents();

  CPipewireCore& m_core;

  const pw_stream_events m_streamEvents;

  spa_hook m_streamListener;

  struct PipewireStreamDeleter
  {
    void operator()(pw_stream* p) { pw_stream_destroy(p); }
  };

  std::unique_ptr<pw_stream, PipewireStreamDeleter> m_stream;
};

} // namespace PIPEWIRE
} // namespace KODI
