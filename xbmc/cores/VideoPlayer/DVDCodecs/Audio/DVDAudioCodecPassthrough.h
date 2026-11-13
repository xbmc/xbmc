/*
 *  Copyright (C) 2010-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "DVDAudioCodec.h"
#include "cores/AudioEngine/Utils/AEAudioFormat.h"
#include "cores/AudioEngine/Utils/AEBitstreamPacker.h"
#include "cores/AudioEngine/Utils/AEStreamInfo.h"

#include <deque>
#include <memory>
#include <stdint.h>
#include <vector>

class CProcessInfo;
class CPackerMAT;

class CDVDAudioCodecPassthrough : public CDVDAudioCodec
{
public:
  CDVDAudioCodecPassthrough(CProcessInfo &processInfo, CAEStreamInfo::DataType streamType);
  ~CDVDAudioCodecPassthrough() override;

  bool Open(CDVDStreamInfo &hints, CDVDCodecOptions &options) override;
  void Dispose() override;
  bool AddData(const DemuxPacket &packet) override;
  void GetData(DVDAudioFrame &frame) override;
  void Reset() override;
  AEAudioFormat GetFormat() override { return m_format; }
  bool NeedPassthrough() override { return true; }
  std::string GetName() override { return m_codecName; }
  int GetBufferSize() override;

private:
  struct PacketTimestampSpan
  {
    uint64_t start;
    uint64_t end;
    double pts;
    double originalPts;
  };

  int GetData(uint8_t** dst);
  unsigned int PackTrueHD();
  //! @brief Resolve an access unit timestamp from the demux span owning its first byte.
  double GetPacketTimestampForAccessUnit(bool original);
  CAEStreamParser m_parser;
  uint8_t* m_buffer = nullptr;
  unsigned int m_bufferSize = 0;
  unsigned int m_dataSize = 0;
  AEAudioFormat m_format;
  uint8_t *m_backlogBuffer = nullptr;
  unsigned int m_backlogBufferSize = 0;
  unsigned int m_backlogSize = 0;
  double m_currentPts = DVD_NOPTS_VALUE;
  double m_currentOriginalPts = DVD_NOPTS_VALUE;
  std::deque<PacketTimestampSpan> m_packetTimestampSpans;
  uint64_t m_nextInputOffset{0};
  uint64_t m_lastPtsSpanStart{UINT64_MAX};
  uint64_t m_lastOriginalPtsSpanStart{UINT64_MAX};
  double m_lastAccessUnitPts{DVD_NOPTS_VALUE};
  double m_lastAccessUnitOriginalPts{DVD_NOPTS_VALUE};
  uint16_t m_lastPtsFrameTime{0};
  uint16_t m_lastOriginalPtsFrameTime{0};
  bool m_haveLastPtsFrameTime{false};
  bool m_haveLastOriginalPtsFrameTime{false};
  std::string m_codecName;

  // TrueHD specifics
  std::unique_ptr<CPackerMAT> m_packerMAT;
  std::vector<uint8_t> m_trueHDBuffer;
  unsigned int m_trueHDoffset = 0;
  unsigned int m_trueHDframes = 0;
  double m_trueHDCurrentPts{DVD_NOPTS_VALUE};
  double m_trueHDCurrentOriginalPts{DVD_NOPTS_VALUE};
  bool m_deviceIsRAW{false};
};
