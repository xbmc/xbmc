/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <deque>
#include <stdint.h>
#include <vector>

struct TrueHDMajorSyncInfo
{
  int ratebits{0};
  uint16_t outputTiming{0};
  bool outputTimingPresent{false};
  bool valid{false};
};

enum class Type
{
  PADDING,
  DATA,
};

class CPackerMAT
{
public:
  CPackerMAT();
  ~CPackerMAT() = default;

  bool PackTrueHD(const uint8_t* data, int size);
  std::vector<uint8_t> GetOutputFrame();
  
  //============================================================================
  // LAV seamless branching support (conditionally enabled)
  // These methods are only meaningful when m_lavStyleEnabled = true
  // Developer: Wire m_lavStyleEnabled to your settings system
  //============================================================================
  int GetSamplesOffset() const { return m_lavStyleEnabled ? m_lastOutputSamplesOffset : 0; }
  bool HadDiscontinuity() const { return m_lavStyleEnabled ? m_lastOutputHadDiscontinuity : false; }
  void Reset();
  
  // Toggle LAV seamless branching support
  void SetLavStyleEnabled(bool enabled) { m_lavStyleEnabled = enabled; }
  bool IsLavStyleEnabled() const { return m_lavStyleEnabled; }

private:
  struct MATState
  {
    bool init; // differentiates the first header

    // audio_sampling_frequency:
    //  0 -> 48 kHz
    //  1 -> 96 kHz
    //  2 -> 192 kHz
    //  8 -> 44.1 kHz
    //  9 -> 88.2 kHz
    // 10 -> 176.4 kHz
    int ratebits;

    // Output timing obtained parsing TrueHD major sync headers (when available) or
    // inferred increasing a counter the rest of the time.
    uint16_t outputTiming;
    bool outputTimingValid;

    // Input timing of audio unit (obtained of each audio unit) and used to calculate padding
    // bytes. On the contrary of outputTiming, frametime is present in all audio units.
    uint16_t prevFrametime;
    bool prevFrametimeValid;

    uint32_t matFramesize; // size in bytes of current MAT frame
    uint32_t prevMatFramesize; // size in bytes of previous MAT frame

    int32_t padding; // padding bytes pending to write (signed: can go negative during LAV branch handling)
    uint32_t samples; // number of samples accumulated in current MAT frame
    int32_t numberOfSamplesOffset; // offset respect number of samples in a standard MAT frame (40 * 24)
    int32_t nOutputTimeOffset; // offset of frame time to output time (LAV: for discontinuity padding calculation)
  };

  void WriteHeader();
  void WritePadding();
  void AppendData(const uint8_t* data, int size, Type type);
  uint32_t GetCount() const { return m_bufferCount; }
  int FillDataBuffer(const uint8_t* data, int size, Type type);
  void FlushPacket();
  TrueHDMajorSyncInfo ParseTrueHDMajorSyncHeaders(const uint8_t* p, int buffsize) const;

  MATState m_state{};
  bool m_lavStyleEnabled{false}; // Toggle LAV seamless branching support (OFF by default)
  int m_lastOutputSamplesOffset{0}; // samples offset returned by last GetOutputFrame() call (LAV only)
  bool m_lastOutputHadDiscontinuity{false}; // discontinuity flag for last GetOutputFrame() (LAV only)

  uint32_t m_bufferCount{0};
  std::vector<uint8_t> m_buffer;
  std::deque<std::vector<uint8_t>> m_outputQueue;
  
  // LAV tracking (only used when m_lavStyleEnabled = true)
  std::deque<int> m_offsetQueue; // samples offset for each queued MAT frame
  std::deque<bool> m_discontinuityQueue; // discontinuity flag for each queued MAT frame
  bool m_pendingDiscontinuity{false}; // set when discontinuity detected, cleared when MAT flushed
};

class CBitStream
{
public:
  // opens an existing byte array as bitstream
  CBitStream(const uint8_t* bytes, int _size)
  {
    data = bytes;
    size = _size;
  }

  // reads bits from bitstream
  int ReadBits(int bits)
  {
    int dat = 0;
    for (int i = index; i < index + bits; i++)
    {
      dat = dat * 2 + getbit(data[i / 8], i % 8);
    }
    index += bits;
    return dat;
  }

  // skip bits from bitstream
  void SkipBits(int bits) { index += bits; }

private:
  uint8_t getbit(uint8_t x, int y) { return (x >> (7 - y)) & 1; }

  const uint8_t* data{nullptr};
  int size{0};
  int index{0};
};
