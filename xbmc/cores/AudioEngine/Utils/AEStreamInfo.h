/*
 *  Copyright (C) 2010-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "AEPackIEC61937.h"
#include "AEChannelInfo.h"
#include <stdint.h>

/* ffmpeg re-defines this, so undef it to squash the warning */
#undef restrict

extern "C" {
#include <libavutil/crc.h>
}

class CAEStreamInfo
{
public:
  double GetDuration() const;
  bool operator==(const CAEStreamInfo& info) const;

  enum DataType
  {
    STREAM_TYPE_NULL,
    STREAM_TYPE_AC3,
    STREAM_TYPE_DTS_512,
    STREAM_TYPE_DTS_1024,
    STREAM_TYPE_DTS_2048,
    STREAM_TYPE_DTSHD,
    STREAM_TYPE_DTSHD_CORE,
    STREAM_TYPE_EAC3,
    STREAM_TYPE_MLP,
    STREAM_TYPE_TRUEHD,
    STREAM_TYPE_DTSHD_MA
  };
  DataType m_type = STREAM_TYPE_NULL;
  unsigned int m_sampleRate;
  unsigned int m_channels;
  bool m_dataIsLE = true;
  unsigned int m_dtsPeriod = 0;
  unsigned int m_repeat = 0;
  unsigned int m_frameSize = 0;
};

class CAEStreamParser
{
public:

  CAEStreamParser();
  ~CAEStreamParser() = default;

  int AddData(uint8_t *data, unsigned int size, uint8_t **buffer = NULL, unsigned int *bufferSize = 0);

  void SetCoreOnly(bool value) { m_coreOnly = value; }
  unsigned int IsValid() const { return m_hasSync; }
  unsigned int GetSampleRate() const { return m_info.m_sampleRate; }
  unsigned int GetChannels() const { return m_info.m_channels; }
  unsigned int GetFrameSize() const { return m_fsize; }
  // unsigned int GetDTSBlocks() const { return m_dtsBlocks; }
  unsigned int GetDTSPeriod() const { return m_info.m_dtsPeriod; }
  unsigned int GetEAC3BlocksDiv() const { return m_info.m_repeat; }
  enum CAEStreamInfo::DataType GetDataType() const { return m_info.m_type; }
  bool IsLittleEndian() const { return m_info.m_dataIsLE; }
  unsigned int GetBufferSize() const { return m_bufferSize; }
  CAEStreamInfo& GetStreamInfo() { return m_info; }
  void Reset();

private:
  uint8_t m_buffer[MAX_IEC61937_PACKET];
  unsigned int m_bufferSize = 0;
  unsigned int m_skipBytes = 0;

  typedef unsigned int (CAEStreamParser::*ParseFunc)(uint8_t *data, unsigned int size);

  CAEStreamInfo m_info;
  bool m_coreOnly = false;
  unsigned int m_needBytes = 0;
  ParseFunc m_syncFunc;
  bool m_hasSync = false;

  unsigned int m_coreSize = 0;         /* core size for dtsHD */
  unsigned int m_dtsBlocks = 0;
  unsigned int m_fsize = 0;
  int m_substreams = 0;       /* used for TrueHD  */
  AVCRC m_crcTrueHD[1024];  /* TrueHD crc table */

  void GetPacket(uint8_t **buffer, unsigned int *bufferSize);
  unsigned int DetectType(uint8_t *data, unsigned int size);
  bool TrySyncAC3(uint8_t *data, unsigned int size, bool resyncing, bool wantEAC3dependent);
  unsigned int SyncAC3(uint8_t *data, unsigned int size);
  unsigned int SyncDTS(uint8_t *data, unsigned int size);
  unsigned int SyncTrueHD(uint8_t *data, unsigned int size);

  static unsigned int GetTrueHDChannels(const uint16_t chanmap);
};

