/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *  See LICENSES/README.md for more information.
 */

#include "VTB.h"

#include "DVDCodecs/DVDCodecUtils.h"
#include "DVDCodecs/DVDFactoryCodec.h"
#include "DVDVideoCodec.h"
#include "ServiceBroker.h"
#include "cores/VideoPlayer/Process/ProcessInfo.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"

#include <mutex>

extern "C" {
#include <libavcodec/videotoolbox.h>
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
  ~CVideoBufferPoolVTB() override;
  void Return(int id) override;
  CVideoBuffer* Get() override;

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
  std::unique_lock<CCriticalSection> lock(m_critSection);

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
  std::unique_lock<CCriticalSection> lock(m_critSection);

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

IHardwareDecoder* CDecoder::Create(CDVDStreamInfo &hint, CProcessInfo &processInfo, AVPixelFormat fmt)
{
#if defined(TARGET_DARWIN_EMBEDDED)
  // force disable HW acceleration for live streams
  // to avoid absent image issue on interlaced videos
  if (processInfo.IsRealtimeStream())
    return nullptr;
#endif

  if (fmt == AV_PIX_FMT_VIDEOTOOLBOX && CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_VIDEOPLAYER_USEVTB))
    return new VTB::CDecoder(processInfo);

  return nullptr;
}

bool CDecoder::Register()
{
  CDVDFactoryCodec::RegisterHWAccel("vtb", CDecoder::Create);
  return true;
}

CDecoder::CDecoder(CProcessInfo& processInfo)
  : m_processInfo(processInfo), m_videoBufferPool(std::make_shared<CVideoBufferPoolVTB>())
{
  m_avctx = nullptr;
}

CDecoder::~CDecoder()
{
  if (m_renderBuffer)
    m_renderBuffer->Release();
  Close();
}

void CDecoder::Close()
{

}

bool CDecoder::Open(AVCodecContext *avctx, AVCodecContext* mainctx, enum AVPixelFormat fmt)
{
  if (!CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_VIDEOPLAYER_USEVTB))
    return false;

  AVBufferRef *deviceRef =  av_hwdevice_ctx_alloc(AV_HWDEVICE_TYPE_VIDEOTOOLBOX);
  AVBufferRef *framesRef = av_hwframe_ctx_alloc(deviceRef);
  AVHWFramesContext *framesCtx = (AVHWFramesContext*)framesRef->data;
  framesCtx->format = AV_PIX_FMT_VIDEOTOOLBOX;
  framesCtx->sw_format = AV_PIX_FMT_NV12;
  avctx->hw_frames_ctx = framesRef;
  m_avctx = avctx;

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
    if (frame->interlaced_frame)
      return CDVDVideoCodec::VC_FATAL;

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
