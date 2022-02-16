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
  static unsigned int GetOutputRate(const CAEStreamInfo& info);
  static CAEChannelInfo GetOutputChannelMap(const CAEStreamInfo& info);

private:
  void PackTrueHD(CAEStreamInfo &info, uint8_t* data, int size);
  void PackDTSHD(CAEStreamInfo &info, uint8_t* data, int size);
  void PackEAC3(CAEStreamInfo &info, uint8_t* data, int size);

  /* we keep the trueHD and dtsHD buffers separate so that we can handle a fast stream switch */
  std::vector<uint8_t> m_trueHD[2];

  struct TrueHD
  {
    int prevFrameSize;
    int samplesPerFrame;
    int bufferFilled;
    int bufferIndex;
    uint16_t prevFrameTime;
    uint8_t* outputBuffer;
  };

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

