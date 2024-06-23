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

    // input timing of previous audio unit used to calculate padding bytes
    uint16_t prevFrametime;
    bool prevFrametimeValid;

    uint32_t matFramesize; // size in bytes of current MAT frame
    uint32_t prevMatFramesize; // size in bytes of previous MAT frame

    uint32_t padding; // padding bytes pending to write
  };

  void WriteHeader();
  void WritePadding();
  void AppendData(const uint8_t* data, int size, Type type);
  uint32_t GetCount() const { return m_bufferCount; }
  int FillDataBuffer(const uint8_t* data, int size, Type type);
  void FlushPacket();

  MATState m_state{};

  uint32_t m_bufferCount{0};
  std::vector<uint8_t> m_buffer;
  std::deque<std::vector<uint8_t>> m_outputQueue;
};
