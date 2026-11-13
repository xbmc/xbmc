/*
 *  Copyright (C) 2024 Team Kodi
 *  Copyright (C) 2010-2021 Hendrik Leppkes
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 *
 *  The TrueHD seamless-branch handling (output-timing discontinuity detection
 *  and padding carry-forward) is derived from the TrueHD MAT packer in LAV
 *  Filters by Hendrik Leppkes (Nevcairiel), decoder/LAVAudio/BitstreamMAT.cpp.
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
  //! @brief Pack an access unit and preserve both timestamp timelines on completed MAT frames.
  bool PackTrueHD(const uint8_t* data,
                  int size,
                  double pts,
                  double originalPts,
                  double duration,
                  bool hasPts,
                  bool hasOriginalPts);
  std::vector<uint8_t> GetOutputFrame();
  //! @brief Retrieve the next MAT frame with its corrected and original-media timestamps.
  std::vector<uint8_t> GetOutputFrame(double& pts,
                                      bool& hasPts,
                                      double& originalPts,
                                      bool& hasOriginalPts);
  //! @brief Return whether a complete MAT frame is waiting to be retrieved.
  bool HasOutputFrame() const { return !m_outputQueue.empty(); }
  //! @brief Discard buffered MAT data and reset its timestamp state.
  void Reset();

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

    // output timing parsed from the TrueHD major sync restart header (when
    // present) or inferred by advancing a counter. Used to detect a seamless
    // branch point: the value jumps when the stream branches.
    uint16_t outputTiming;
    bool outputTimingValid;

    // input timing of previous audio unit used to calculate padding bytes
    uint16_t prevFrametime;
    bool prevFrametimeValid;

    uint32_t matFramesize; // size in bytes of current MAT frame
    uint32_t prevMatFramesize; // size in bytes of previous MAT frame

    uint32_t padding; // padding bytes pending to write

    // frame-time to output-time offset, used to size the padding carry-forward
    // on the next branch point.
    int32_t nOutputTimeOffset;
  };

  void WriteHeader();
  void WritePadding();
  void AppendData(const uint8_t* data, int size, Type type);
  uint32_t GetCount() const { return m_bufferCount; }
  int FillDataBuffer(const uint8_t* data, int size, Type type);
  void FlushPacket();

  struct OutputFrame
  {
    std::vector<uint8_t> data;
    double pts{0.0};
    double originalPts{0.0};
    bool hasPts{false};
    bool hasOriginalPts{false};
  };

  MATState m_state{};

  uint32_t m_bufferCount{0};
  std::vector<uint8_t> m_buffer;
  std::deque<OutputFrame> m_outputQueue;
  double m_currentOutputPts{0.0};
  double m_currentOutputOriginalPts{0.0};
  double m_outputDuration{0.0};
  bool m_hasCurrentOutputPts{false};
  bool m_hasCurrentOutputOriginalPts{false};
};
