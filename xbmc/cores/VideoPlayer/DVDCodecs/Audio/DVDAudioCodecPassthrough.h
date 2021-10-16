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

#include <list>
#include <vector>

class CProcessInfo;

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
  int GetData(uint8_t** dst);
  CAEStreamParser m_parser;
  uint8_t* m_buffer = nullptr;
  unsigned int m_bufferSize = 0;
  unsigned int m_dataSize = 0;
  AEAudioFormat m_format;
  uint8_t *m_backlogBuffer = nullptr;
  unsigned int m_backlogBufferSize = 0;
  unsigned int m_backlogSize = 0;
  double m_currentPts = DVD_NOPTS_VALUE;
  double m_nextPts = DVD_NOPTS_VALUE;
  std::string m_codecName;

  // TrueHD specifics
  std::vector<uint8_t> m_trueHDBuffer;
  unsigned int m_trueHDoffset = 0;
  unsigned int m_trueHDframes = 0;
};

