/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "DVDVideoCodec.h"
#include "DVDStreamInfo.h"
#include "settings/lib/ISettingCallback.h"
#include "threads/CriticalSection.h"
#include "cores/VideoPlayer/Buffers/VideoBuffer.h"
#include "utils/BitstreamConverter.h"

#include <set>
#include <atomic>
#include <memory>

class CAMLCodec;
struct mpeg2_sequence;
struct h264_sequence;
class CBitstreamParser;
class CBitstreamConverter;
class CDataCacheCore;
class CSetting;

class CDVDVideoCodecAmlogic;

typedef std::tuple<uint8_t*, uint32_t, bool, double> DLDemuxPacket;

class CAMLVideoBuffer : public CVideoBuffer
{
public:
  CAMLVideoBuffer(int id) : CVideoBuffer(id) {};
  void Set(CDVDVideoCodecAmlogic *codec, std::shared_ptr<CAMLCodec> amlcodec, uint64_t omxPts, int amlDuration, uint32_t bufferIndex)
  {
    m_codec = codec;
    m_amlCodec = amlcodec;
    m_omxPts = omxPts;
    m_amlDuration = amlDuration;
    m_bufferIndex = bufferIndex;
  }

  CDVDVideoCodecAmlogic* m_codec;
  std::shared_ptr<CAMLCodec> m_amlCodec;
  uint64_t m_omxPts;
  int m_amlDuration;
  uint32_t m_bufferIndex;
};

class CAMLVideoBufferPool : public IVideoBufferPool
{
public:
  virtual ~CAMLVideoBufferPool();

  virtual CVideoBuffer* Get() override;
  virtual void Return(int id) override;

private:
  CCriticalSection m_criticalSection;;
  std::vector<CAMLVideoBuffer*> m_videoBuffers;
  std::vector<int> m_freeBuffers;
};

class CDVDVideoCodecAmlogic : public CDVDVideoCodec, public ISettingCallback
{
public:
  CDVDVideoCodecAmlogic(CProcessInfo &processInfo);
  virtual ~CDVDVideoCodecAmlogic();

  static std::unique_ptr<CDVDVideoCodec> Create(CProcessInfo& processInfo);
  static bool Register();

  // Required overrides
  virtual bool Open(CDVDStreamInfo &hints, CDVDCodecOptions &options) override;
  virtual bool AddData(const DemuxPacket &packet) override;
  virtual void Reset() override;
  virtual VCReturn GetPicture(VideoPicture* pVideoPicture) override;
  virtual void SetSpeed(int iSpeed) override;
  virtual void SetCodecControl(int flags) override;
  virtual const char* GetName(void) override { return (const char*)m_pFormatName; }
  virtual bool SupportsExtention() { return true; }

  void OnSettingChanged(const std::shared_ptr<const CSetting>& setting) override;

protected:
  void Close(void);
  void FrameRateTracking(uint8_t *pData, int iSize, double dts, double pts);

  std::shared_ptr<CAMLCodec> m_Codec;

  const char     *m_pFormatName;
  VideoPicture    m_videobuffer;
  bool            m_opened;
  int             m_codecControlFlags;
  CDVDStreamInfo  m_hints;
  double          m_framerate;
  int             m_video_rate;
  float           m_aspect_ratio;
  double          m_mpeg2_sequence_pts;
  double          m_h264_sequence_pts;
  bool            m_has_keyframe;

  std::unique_ptr<mpeg2_sequence> m_mpeg2_sequence;
  std::unique_ptr<h264_sequence>  m_h264_sequence;

  std::unique_ptr<CBitstreamParser>    m_bitparser;
  std::unique_ptr<CBitstreamConverter> m_bitstream;
private:
  bool DualLayerConvert(uint8_t *pData, uint32_t iSize, const DemuxPacket &packet);
  bool SingleLayerConvert(uint8_t *pData, uint32_t iSize, const DemuxPacket &packet) const;
  void ClearBitstreamCommon(void);
  void UpdateAppendCMv40SettingCache();
  void ApplyDynamicDoViSettings();

  std::shared_ptr<CAMLVideoBufferPool> m_videoBufferPool;
  static std::atomic<bool> m_InstanceGuard;

  std::list<DLDemuxPacket> m_packages;

  bool      m_last_added = true;
  uint8_t  *m_last_pData = nullptr;
  uint32_t  m_last_iSize = 0;

  std::atomic<int> m_appendCMv40ModeSetting{static_cast<int>(DOVICMv40Mode::CMV40_NONE)};
  DOVICMv40Mode m_appendCMv40ModeApplied{DOVICMv40Mode::CMV40_NONE};
  bool m_settingsCallbackRegistered{false};

};
