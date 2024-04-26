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
  bool HaveOutput() const { return !m_outputQueue.empty(); }
  std::vector<uint8_t> GetOutputFrame();

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

    uint32_t padding; // padding bytes pending to write
    uint32_t samples; // number of samples accumulated in current MAT frame
    int numberOfSamplesOffset; // offset respect number of samples in a standard MAT frame (40 * 24)
  };

  void WriteHeader();
  void WritePadding();
  void AppendData(const uint8_t* data, int size, Type type);
  uint32_t GetCount() const { return m_bufferCount; }
  int FillDataBuffer(const uint8_t* data, int size, Type type);
  void FlushPacket();
  TrueHDMajorSyncInfo ParseTrueHDMajorSyncHeaders(const uint8_t* p, int buffsize) const;

  MATState m_state{};

  bool m_logPadding{false};

  uint32_t m_bufferCount{0};
  std::vector<uint8_t> m_buffer;
  std::deque<std::vector<uint8_t>> m_outputQueue;
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
