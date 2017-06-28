#pragma once
/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "DVDVideoCodec.h"
#include "DVDStreamInfo.h"
#include "threads/CriticalSection.h"
#include "cores/VideoPlayer/Process/VideoBuffer.h"

#include <set>
#include <atomic>

class CAMLCodec;
struct frame_queue;
struct mpeg2_sequence;
class CBitstreamParser;
class CBitstreamConverter;

class CDVDVideoCodecAmlogic;

class CDVDAmlogicVideoBuffer : public CVideoBuffer
{
public:
  CDVDAmlogicVideoBuffer(int id) : CVideoBuffer(id) = default;

  CDVDVideoCodecAmlogic* m_codec;
  std::shared_ptr<CAMLCodec> m_amlCodec;
  int m_omxPts, m_amlDuration;
  uint32_t m_bufferIndex;
};

class CAMLPool : public CAMLVideoBufferPool
{
public:
  virtual ~CAMLVideoBufferPool();

  virtual CVideoBuffer* Get() override;
  virtual void Return(int id) override;

private:
  std::vector<CDVDAmlogicVideoBuffer*> m_videoBuffers;
  std::vector<int> m_freeBuffers;
};

class CDVDVideoCodecAmlogic : public CDVDVideoCodec
{
  friend class CDVDAmlogicInfo;

public:
  CDVDVideoCodecAmlogic(CProcessInfo &processInfo);
  virtual ~CDVDVideoCodecAmlogic();

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
  void            FrameQueuePop(void);
  void            FrameQueuePush(double dts, double pts);
  void            FrameRateTracking(uint8_t *pData, int iSize, double dts, double pts);
  void            RemoveInfo(CDVDAmlogicInfo* info);

  std::shared_ptr<CAMLCodec> m_Codec;

  const char     *m_pFormatName;
  VideoPicture m_videobuffer;
  bool            m_opened;
  int             m_codecControlFlags;
  CDVDStreamInfo  m_hints;
  double          m_last_pts;
  frame_queue    *m_frame_queue;
  int32_t         m_queue_depth;
  pthread_mutex_t m_queue_mutex;
  double          m_framerate;
  int             m_video_rate;
  float           m_aspect_ratio;
  mpeg2_sequence *m_mpeg2_sequence;
  double          m_mpeg2_sequence_pts;
  bool            m_has_keyframe;

  CBitstreamParser *m_bitparser;
  CBitstreamConverter *m_bitstream;
private:
  CAMLPool m_bufferPool;

  CCriticalSection m_secure;
  static std::atomic<bool> m_InstanceGuard;
};
