/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/RetroPlayer/streams/memory/IMemoryStream.h"

#include <atomic>
#include <memory>

namespace KODI::RETRO
{
class CAutosaveCapture
{
public:
  void Request() { m_requested.store(true); }
  bool IsPending() const { return m_pinned || m_requested.load(); }

  // Cancel a mark before the timeline or stream changes. Retry on a new frame.
  void Cancel()
  {
    if (m_pinned)
      Request();
    m_pinned = false;
  }

  template<typename Snapshot>
  void Cancel(std::unique_ptr<Snapshot>& snapshot, bool& ready)
  {
    if (snapshot && ready && !snapshot->discarded)
      Request();
    Cancel();
    if (snapshot)
    {
      snapshot->discarded = true;
      ready = true;
    }
  }

  void Reset()
  {
    m_pinned = false;
    m_requested.store(false);
  }

  // Only Request() is called off the game thread (or without the playback lock).
  template<typename Serialize, typename CaptureMetadata>
  bool CaptureFrame(IMemoryStream* stream,
                    bool serialized,
                    std::unique_ptr<uint32_t[]>& buffer,
                    Serialize&& serialize,
                    CaptureMetadata&& captureMetadata)
  {
    if (!buffer || (stream && !serialized))
      return false;
    if (m_pinned)
    {
      // The new delta reconstructs the marked frame, so its full buffer can leave.
      if (!stream->ExchangeRetiredFrame(buffer))
        return false;
      m_pinned = false;
      return true;
    }
    if (!m_requested.exchange(false))
      return false;
    if (!stream && !serialize())
    {
      Request();
      return false;
    }
    captureMetadata();
    m_pinned = stream != nullptr;
    return !m_pinned;
  }

private:
  std::atomic<bool> m_requested{false};
  bool m_pinned{false};
};
} // namespace KODI::RETRO
