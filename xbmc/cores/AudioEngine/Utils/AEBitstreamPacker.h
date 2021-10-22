/*
 *  Copyright (C) 2010-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "AEChannelInfo.h"
#include "AEPackIEC61937.h"

#include <list>
#include <stdint.h>
#include <vector>

#define MAT_CODE(position, data) \
  { \
    position, data, sizeof(data) \
  }

namespace
{
constexpr auto BURST_HEADER_SIZE = 8;
constexpr auto EAC3_MAX_BURST_PAYLOAD_SIZE = 24576 - BURST_HEADER_SIZE;

constexpr auto MAT_PKT_OFFSET = 61440;
constexpr auto MAT_FRAME_SIZE = 61424;

/* magic MAT format values, meaning is unknown at this point */
const uint8_t mat_start_code[20] = {
    0x07, 0x9E, 0x00, 0x03, 0x84, 0x01, 0x01, 0x01, 0x80, 0x00,
    0x56, 0xA5, 0x3B, 0xF4, 0x81, 0x83, 0x49, 0x80, 0x77, 0xE0,
};
const uint8_t mat_middle_code[12] = {
    0xC3, 0xC1, 0x42, 0x49, 0x3B, 0xFA, 0x82, 0x83, 0x49, 0x80, 0x77, 0xE0,
};
const uint8_t mat_end_code[16] = {
    0xC3, 0xC2, 0xC0, 0xC4, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x97, 0x11,
};

const struct
{
  int pos;
  const uint8_t* code;
  unsigned int len;
} MatCodes[] = {MAT_CODE(0, mat_start_code), MAT_CODE(30708, mat_middle_code),
                MAT_CODE(MAT_FRAME_SIZE - sizeof(mat_end_code), mat_end_code)};

struct TrueHD
{
  int prevFrameSize;
  int samplesPerFrame;
  int bufferFilled;
  int bufferIndex;
  uint16_t prevFrameTime;
  uint8_t* outputBuffer;
};
} // unnamed namespace

class CAEStreamInfo;

class CAEBitstreamPacker
{
public:
  CAEBitstreamPacker();
  ~CAEBitstreamPacker();

  void Pack(CAEStreamInfo &info, uint8_t* data, int size);
  bool PackPause(CAEStreamInfo &info, unsigned int millis, bool iecBursts);
  void Reset();
  uint8_t* GetBuffer();
  unsigned int GetSize() const;
  static unsigned int GetOutputRate(CAEStreamInfo &info);
  static CAEChannelInfo GetOutputChannelMap(CAEStreamInfo &info);

private:
  void PackTrueHD(CAEStreamInfo &info, uint8_t* data, int size);
  void PackDTSHD(CAEStreamInfo &info, uint8_t* data, int size);
  void PackEAC3(CAEStreamInfo &info, uint8_t* data, int size);

  /* we keep the trueHD and dtsHD buffers separate so that we can handle a fast stream switch */
  std::vector<uint8_t> m_trueHD[2];
  TrueHD m_thd{};

  std::vector<uint8_t> m_dtsHD;
  unsigned int m_dtsHDSize = 0;

  std::vector<uint8_t> m_eac3;
  unsigned int m_eac3Size = 0;
  unsigned int m_eac3FramesCount = 0;
  unsigned int m_eac3FramesPerBurst = 0;

  unsigned int  m_dataSize = 0;
  uint8_t       m_packedBuffer[MAX_IEC61937_PACKET];
  unsigned int m_pauseDuration = 0;
};

