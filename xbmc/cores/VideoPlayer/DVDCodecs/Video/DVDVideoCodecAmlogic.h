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
#include "threads/CriticalSection.h"
#include "cores/VideoPlayer/Process/VideoBuffer.h"

#include <set>
#include <atomic>

class CAMLCodec;
struct mpeg2_sequence;
class CBitstreamParser;
class CBitstreamConverter;

class CDVDVideoCodecAmlogic;

class CAMLVideoBuffer : public CVideoBuffer
{
public:
  CAMLVideoBuffer(int id) : CVideoBuffer(id) {};
  void Set(CDVDVideoCodecAmlogic *codec, std::shared_ptr<CAMLCodec> amlcodec, int omxPts, int amlDuration, uint32_t bufferIndex)
  {
    m_codec = codec;
    m_amlCodec = amlcodec;
    m_omxPts = omxPts;
    m_amlDuration = amlDuration;
    m_bufferIndex = bufferIndex;
  }

  CDVDVideoCodecAmlogic* m_codec;
  std::shared_ptr<CAMLCodec> m_amlCodec;
  int m_omxPts, m_amlDuration;
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

class CDVDVideoCodecAmlogic : public CDVDVideoCodec
{
public:
  CDVDVideoCodecAmlogic(CProcessInfo &processInfo);
  virtual ~CDVDVideoCodecAmlogic();

  static CDVDVideoCodec* Create(CProcessInfo &processInfo);
  static bool Register();

  // Required overrides
  virtual bool Open(CDVDStreamInfo &hints, CDVDCodecOptions &options) override;
  virtual bool AddData(const DemuxPacket &packet) override;
  virtual void Reset() override;
  virtual VCReturn GetPicture(VideoPicture* pVideoPicture) override;
  virtual void SetSpeed(int iSpeed) override;
  virtual void SetCodecControl(int flags) override;
  virtual const char* GetName(void) override { return (const char*)m_pFormatName; }

protected:
  void            Dispose(void);
  void            FrameRateTracking(uint8_t *pData, int iSize, double dts, double pts);
  //void            RemoveInfo(CDVDAmlogicInfo* info);

  std::shared_ptr<CAMLCodec> m_Codec;

  const char     *m_pFormatName;
  VideoPicture m_videobuffer;
  bool            m_opened;
  int             m_codecControlFlags;
  CDVDStreamInfo  m_hints;
  double          m_framerate;
  int             m_video_rate;
  float           m_aspect_ratio;
  mpeg2_sequence *m_mpeg2_sequence;
  double          m_mpeg2_sequence_pts;
  bool            m_has_keyframe;

  CBitstreamParser *m_bitparser;
  CBitstreamConverter *m_bitstream;
private:
  std::shared_ptr<CAMLVideoBufferPool> m_videoBufferPool;
  static std::atomic<bool> m_InstanceGuard;
};
