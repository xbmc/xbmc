/*
 *      Copyright (C) 2005-2016 Team XBMC
 *      http://xbmc.org
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
#include "system.h"
#include "platform/darwin/osx/CocoaInterface.h"
#include "platform/darwin/DarwinUtils.h"
#include "cores/VideoPlayer/Process/ProcessInfo.h"
#include "DVDVideoCodec.h"
#include "DVDCodecs/DVDCodecUtils.h"
#include "utils/log.h"
#include "VTB.h"
#include "utils/BitstreamConverter.h"
#include "utils/BitstreamReader.h"
#include "settings/Settings.h"
#include "threads/SingleLock.h"
#include "ServiceBroker.h"

extern "C" {
#include "libavcodec/videotoolbox.h"
}

using namespace VTB;

//------------------------------------------------------------------------------
// Video Buffers
//------------------------------------------------------------------------------

CVideoBufferVTB::CVideoBufferVTB(IVideoBufferPool &pool, int id)
: CVideoBuffer(id)
{
  m_pFrame = av_frame_alloc();
}

CVideoBufferVTB::~CVideoBufferVTB()
{
  av_frame_free(&m_pFrame);
}

void CVideoBufferVTB::SetRef(AVFrame *frame)
{
  av_frame_unref(m_pFrame);
  av_frame_ref(m_pFrame, frame);
  m_pbRef = (CVPixelBufferRef)m_pFrame->data[3];
}

void CVideoBufferVTB::Unref()
{
  av_frame_unref(m_pFrame);
}

CVPixelBufferRef CVideoBufferVTB::GetPB()
{
  return m_pbRef;
}

//------------------------------------------------------------------------------

class VTB::CVideoBufferPoolVTB : public IVideoBufferPool
{
public:
  virtual ~CVideoBufferPoolVTB();
  virtual void Return(int id) override;
  virtual CVideoBuffer* Get() override;

protected:
  CCriticalSection m_critSection;
  std::vector<CVideoBufferVTB*> m_all;
  std::deque<int> m_used;
  std::deque<int> m_free;
};

CVideoBufferPoolVTB::~CVideoBufferPoolVTB()
{
  for (auto buf : m_all)
  {
    delete buf;
  }
}

CVideoBuffer* CVideoBufferPoolVTB::Get()
{
  CSingleLock lock(m_critSection);

  CVideoBufferVTB *buf = nullptr;
  if (!m_free.empty())
  {
    int idx = m_free.front();
    m_free.pop_front();
    m_used.push_back(idx);
    buf = m_all[idx];
  }
  else
  {
    int id = m_all.size();
    buf = new CVideoBufferVTB(*this, id);
    m_all.push_back(buf);
    m_used.push_back(id);
  }

  buf->Acquire(GetPtr());
  return buf;
}

void CVideoBufferPoolVTB::Return(int id)
{
  CSingleLock lock(m_critSection);

  m_all[id]->Unref();
  auto it = m_used.begin();
  while (it != m_used.end())
  {
    if (*it == id)
    {
      m_used.erase(it);
      break;
    }
    else
      ++it;
  }
  m_free.push_back(id);
}

//------------------------------------------------------------------------------
// main class
//------------------------------------------------------------------------------

CDecoder::CDecoder(CProcessInfo& processInfo) : m_processInfo(processInfo)
{
  m_avctx = nullptr;
  m_videoBufferPool = std::make_shared<CVideoBufferPoolVTB>();
}

CDecoder::~CDecoder()
{
  if (m_renderBuffer)
    m_renderBuffer->Release();
  Close();
}

void CDecoder::Close()
{
  if (m_avctx)
  {
    av_videotoolbox_default_free(m_avctx);
    m_avctx = nullptr;
  }
}

bool CDecoder::Open(AVCodecContext *avctx, AVCodecContext* mainctx, enum AVPixelFormat fmt)
{
  if (!CServiceBroker::GetSettings().GetBool(CSettings::SETTING_VIDEOPLAYER_USEVTB))
    return false;

  if (avctx->codec_id == AV_CODEC_ID_H264)
  {
    CBitstreamConverter bs;
    if (!bs.Open(avctx->codec_id, (uint8_t*)avctx->extradata, avctx->extradata_size, false))
    {
      return false;
    }
    CFDataRef avcCData = CFDataCreate(kCFAllocatorDefault,
                            (const uint8_t*)bs.GetExtraData(), bs.GetExtraSize());
    bool interlaced = true;
    int max_ref_frames;
    uint8_t *spc = (uint8_t*)CFDataGetBytePtr(avcCData) + 6;
    uint32_t sps_size = BS_RB16(spc);
    if (sps_size)
      bs.parseh264_sps(spc+3, sps_size-1, &interlaced, &max_ref_frames);
    CFRelease(avcCData);
    if (interlaced)
    {
      CLog::Log(LOGNOTICE, "%s - possible interlaced content.", __FUNCTION__);
      return false;
    }
  }

  if (av_videotoolbox_default_init(avctx) < 0)
    return false;

  m_avctx = avctx;

  mainctx->pix_fmt = fmt;
  mainctx->hwaccel_context = avctx->hwaccel_context;

  m_processInfo.SetVideoDeintMethod("none");

  std::list<EINTERLACEMETHOD> deintMethods;
  deintMethods.push_back(EINTERLACEMETHOD::VS_INTERLACEMETHOD_NONE);
  m_processInfo.UpdateDeinterlacingMethods(deintMethods);

  return true;
}

CDVDVideoCodec::VCReturn CDecoder::Decode(AVCodecContext* avctx, AVFrame* frame)
{
  CDVDVideoCodec::VCReturn status = Check(avctx);
  if(status)
    return status;

  if(frame)
  {
    if (m_renderBuffer)
      m_renderBuffer->Release();
    m_renderBuffer = dynamic_cast<CVideoBufferVTB*>(m_videoBufferPool->Get());
    m_renderBuffer->SetRef(frame);
    return CDVDVideoCodec::VC_PICTURE;
  }
  else
    return CDVDVideoCodec::VC_BUFFER;
}

bool CDecoder::GetPicture(AVCodecContext* avctx, VideoPicture* picture)
{
  ((ICallbackHWAccel*)avctx->opaque)->GetPictureCommon(picture);

  if (picture->videoBuffer)
    picture->videoBuffer->Release();

  picture->videoBuffer = m_renderBuffer;
  picture->videoBuffer->Acquire();
  return true;
}

CDVDVideoCodec::VCReturn CDecoder::Check(AVCodecContext* avctx)
{
  return CDVDVideoCodec::VC_NONE;
}

unsigned CDecoder::GetAllowedReferences()
{
  return 5;
}
