/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "../../AddonBase.h"
#include "../../c-api/addon-instance/pvr/pvr_stream.h"

#ifdef __cplusplus

namespace kodi
{
namespace addon
{

class PVRCodec : public CStructHdl<PVRCodec, PVR_CODEC>
{
public:
  PVRCodec()
  {
    m_cStructure->codec_type = PVR_CODEC_TYPE_UNKNOWN;
    m_cStructure->codec_id = PVR_INVALID_CODEC_ID;
  }
  PVRCodec(const PVRCodec& type) : CStructHdl(type) {}
  PVRCodec(const PVR_CODEC& type) : CStructHdl(&type) {}
  PVRCodec(const PVR_CODEC* type) : CStructHdl(type) {}
  PVRCodec(PVR_CODEC* type) : CStructHdl(type) {}

  void SetCodecType(PVR_CODEC_TYPE codecType) { m_cStructure->codec_type = codecType; }
  PVR_CODEC_TYPE GetCodecType() const { return m_cStructure->codec_type; }

  void SetCodecId(unsigned int codecId) { m_cStructure->codec_id = codecId; }
  unsigned int GetCodecId() const { return m_cStructure->codec_id; }
};

class PVRStreamProperties
  : public CStructHdl<PVRStreamProperties, PVR_STREAM_PROPERTIES::PVR_STREAM>
{
public:
  PVRStreamProperties() { memset(m_cStructure, 0, sizeof(PVR_STREAM_PROPERTIES::PVR_STREAM)); }
  PVRStreamProperties(const PVRStreamProperties& type) : CStructHdl(type) {}
  PVRStreamProperties(const PVR_STREAM_PROPERTIES::PVR_STREAM* type) : CStructHdl(type) {}
  PVRStreamProperties(PVR_STREAM_PROPERTIES::PVR_STREAM* type) : CStructHdl(type) {}

  void SetPID(unsigned int pid) { m_cStructure->iPID = pid; }
  unsigned int GetPID() const { return m_cStructure->iPID; }

  void SetCodecType(PVR_CODEC_TYPE codecType) { m_cStructure->iCodecType = codecType; }
  PVR_CODEC_TYPE GetCodecType() const { return m_cStructure->iCodecType; }

  void SetCodecId(unsigned int codecId) { m_cStructure->iCodecId = codecId; }
  unsigned int GetCodecId() const { return m_cStructure->iCodecId; }

  void SetLanguage(const std::string& language)
  {
    if (language.size() > 3)
    {
      kodi::Log(ADDON_LOG_ERROR,
                "PVRStreamProperties::%s: Language string size '%li' higher as needed 3", __func__,
                language.size());
      return;
    }
    m_cStructure->strLanguage[0] = language[0];
    m_cStructure->strLanguage[1] = language[1];
    m_cStructure->strLanguage[2] = language[2];
    m_cStructure->strLanguage[2] = 0;
  }
  std::string GetLanguage() const { return m_cStructure->strLanguage; }

  void SetSubtitleInfo(int subtitleInfo) { m_cStructure->iSubtitleInfo = subtitleInfo; }
  int GetSubtitleInfo() const { return m_cStructure->iSubtitleInfo; }

  void SetFPSScale(int fpsScale) { m_cStructure->iFPSScale = fpsScale; }
  int GetFPSScale() const { return m_cStructure->iFPSScale; }

  void SetFPSRate(int fpsRate) { m_cStructure->iFPSRate = fpsRate; }
  int GetFPSRate() const { return m_cStructure->iFPSRate; }

  void SetHeight(int height) { m_cStructure->iHeight = height; }
  int GetHeight() const { return m_cStructure->iHeight; }

  void SetWidth(int width) { m_cStructure->iWidth = width; }
  int GetWidth() const { return m_cStructure->iWidth; }

  void SetAspect(float aspect) { m_cStructure->fAspect = aspect; }
  float GetAspect() const { return m_cStructure->fAspect; }

  void SetChannels(int channels) { m_cStructure->iChannels = channels; }
  int GetChannels() const { return m_cStructure->iChannels; }

  void SetSampleRate(int sampleRate) { m_cStructure->iSampleRate = sampleRate; }
  int GetSampleRate() const { return m_cStructure->iSampleRate; }

  void SetBlockAlign(int blockAlign) { m_cStructure->iBlockAlign = blockAlign; }
  int GetBlockAlign() const { return m_cStructure->iBlockAlign; }

  void SetBitRate(int bitRate) { m_cStructure->iBitRate = bitRate; }
  int GetBitRate() const { return m_cStructure->iBitRate; }

  void SetBitsPerSample(int bitsPerSample) { m_cStructure->iBitsPerSample = bitsPerSample; }
  int GetBitsPerSample() const { return m_cStructure->iBitsPerSample; }
};

class PVRStreamTimes : public CStructHdl<PVRStreamTimes, PVR_STREAM_TIMES>
{
public:
  PVRStreamTimes() { memset(m_cStructure, 0, sizeof(PVR_STREAM_TIMES)); }
  PVRStreamTimes(const PVRStreamTimes& type) : CStructHdl(type) {}
  PVRStreamTimes(const PVR_STREAM_TIMES* type) : CStructHdl(type) {}
  PVRStreamTimes(PVR_STREAM_TIMES* type) : CStructHdl(type) {}

  void SetStartTime(time_t startTime) { m_cStructure->startTime = startTime; }
  time_t GetStartTime() const { return m_cStructure->startTime; }

  void SetPTSStart(int64_t ptsStart) { m_cStructure->ptsStart = ptsStart; }
  int64_t GetPTSStart() const { return m_cStructure->ptsStart; }

  void SetPTSBegin(int64_t ptsBegin) { m_cStructure->ptsBegin = ptsBegin; }
  int64_t GetPTSBegin() const { return m_cStructure->ptsBegin; }

  void SetPTSEnd(int64_t ptsEnd) { m_cStructure->ptsEnd = ptsEnd; }
  int64_t GetPTSEnd() const { return m_cStructure->ptsEnd; }
};

} /* namespace addon */
} /* namespace kodi */

#endif /* __cplusplus */
