/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

namespace KODI::PLATFORM::WASM
{
// SPSC ring of planar float frames. The sink thread writes; the processor in
// audio_worklet_processor.js reads the planes and stores the read counter
// through raw wasm memory. The frame counters wrap, and each side keeps its
// own ring position.
class CWasmAudioRing
{
public:
  // The reader must be idle while the ring is allocated, freed or reset.
  void Allocate(unsigned int capacityFrames, unsigned int channels)
  {
    m_capacityFrames = capacityFrames;
    m_channels = channels;
    m_samples.assign(static_cast<size_t>(capacityFrames) * channels, 0.0f);
    Reset();
  }

  void Free()
  {
    m_capacityFrames = 0;
    m_channels = 0;
    m_samples.clear();
    m_samples.shrink_to_fit();
    Reset();
  }

  void Reset()
  {
    m_writeIndex = 0;
    m_readFrame.store(0, std::memory_order_release);
    m_writeFrame.store(0, std::memory_order_release);
  }

  // Copies as many of the frames as fit and returns their number. Planes the
  // caller does not provide are zeroed, so that stale data never plays.
  unsigned int Write(const float* const* planes,
                     unsigned int channels,
                     unsigned int frames,
                     unsigned int offsetFrames)
  {
    const uint32_t writeFrame = m_writeFrame.load(std::memory_order_relaxed);
    const uint32_t usedFrames = writeFrame - m_readFrame.load(std::memory_order_acquire);
    if (usedFrames >= m_capacityFrames)
      return 0;

    const unsigned int toWrite = std::min<unsigned int>(frames, m_capacityFrames - usedFrames);
    const unsigned int firstChunk = std::min(toWrite, m_capacityFrames - m_writeIndex);
    const unsigned int secondChunk = toWrite - firstChunk;
    const unsigned int copyChannels = std::min(channels, m_channels);

    for (unsigned int ch = 0; ch < m_channels; ++ch)
    {
      float* const dstPlane = &m_samples[static_cast<size_t>(ch) * m_capacityFrames];
      if (ch < copyChannels)
      {
        const float* const srcPlane = planes[ch] + offsetFrames;
        std::memcpy(dstPlane + m_writeIndex, srcPlane, firstChunk * sizeof(float));
        std::memcpy(dstPlane, srcPlane + firstChunk, secondChunk * sizeof(float));
      }
      else
      {
        std::memset(dstPlane + m_writeIndex, 0, firstChunk * sizeof(float));
        std::memset(dstPlane, 0, secondChunk * sizeof(float));
      }
    }

    m_writeIndex = (m_writeIndex + toWrite) % m_capacityFrames;
    m_writeFrame.store(writeFrame + toWrite, std::memory_order_release);
    return toWrite;
  }

  unsigned int GetCapacityFrames() const { return m_capacityFrames; }

  unsigned int GetBufferedFrames() const
  {
    return m_writeFrame.load(std::memory_order_relaxed) -
           m_readFrame.load(std::memory_order_acquire);
  }

  // What the reader works on.
  float* GetData() { return m_samples.data(); }
  std::atomic<uint32_t>& GetReadFrame() { return m_readFrame; }
  std::atomic<uint32_t>& GetWriteFrame() { return m_writeFrame; }

private:
  std::vector<float> m_samples;
  unsigned int m_capacityFrames{0};
  unsigned int m_channels{0};
  unsigned int m_writeIndex{0};
  std::atomic<uint32_t> m_readFrame{0};
  std::atomic<uint32_t> m_writeFrame{0};
};
} // namespace KODI::PLATFORM::WASM
