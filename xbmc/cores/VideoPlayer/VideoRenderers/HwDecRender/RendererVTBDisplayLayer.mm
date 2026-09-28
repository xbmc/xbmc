/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RendererVTBDisplayLayer.h"

#include "../RenderFactory.h"
#include "ServiceBroker.h"
#include "cores/VideoPlayer/DVDCodecs/Video/VTB.h"
#include "utils/log.h"
#include "windowing/GraphicContext.h"
#include "windowing/tvos/WinSystemTVOS.h"

#include "platform/darwin/tvos/TVOSEAGLView.h"
#include "platform/darwin/tvos/XBMCController.h"

#include <algorithm>
#include <cmath>
#include <cstring>

#include <CoreMedia/CMSampleBuffer.h>
#include <CoreVideo/CVImageBuffer.h>
#include <CoreVideo/CVPixelBuffer.h>

namespace
{
void SetColorAttachments(CVPixelBufferRef pixelBuffer, const VideoPicture& picture)
{
  if (picture.hdrType != StreamHdrType::HDR_TYPE_HDR10 &&
      picture.hdrType != StreamHdrType::HDR_TYPE_HLG)
    return;

  CVBufferSetAttachment(pixelBuffer, kCVImageBufferColorPrimariesKey,
                        kCVImageBufferColorPrimaries_ITU_R_2020, kCVAttachmentMode_ShouldPropagate);
  CVBufferSetAttachment(pixelBuffer, kCVImageBufferYCbCrMatrixKey,
                        kCVImageBufferYCbCrMatrix_ITU_R_2020, kCVAttachmentMode_ShouldPropagate);
  CVBufferSetAttachment(pixelBuffer, kCVImageBufferTransferFunctionKey,
                        picture.hdrType == StreamHdrType::HDR_TYPE_HLG
                            ? kCVImageBufferTransferFunction_ITU_R_2100_HLG
                            : kCVImageBufferTransferFunction_SMPTE_ST_2084_PQ,
                        kCVAttachmentMode_ShouldPropagate);

  if (picture.hdrType != StreamHdrType::HDR_TYPE_HDR10)
  {
    CVBufferRemoveAttachment(pixelBuffer, kCVImageBufferMasteringDisplayColorVolumeKey);
    CVBufferRemoveAttachment(pixelBuffer, kCVImageBufferContentLightLevelInfoKey);
    return;
  }

  if (picture.hasDisplayMetadata && picture.displayMetadata.has_primaries &&
      picture.displayMetadata.has_luminance)
  {
    // Mastering display colour volume SEI: G, B, R, white point, max/min
    // luminance, all in big-endian order (ISO/IEC 23008-2 D.2.28).
    uint8_t bytes[24]{};
    auto put16 = [&bytes](int offset, double value)
    {
      const uint16_t be = CFSwapInt16HostToBig(static_cast<uint16_t>(
          std::lround(std::clamp(std::isfinite(value) ? value : 0.0, 0.0, 65535.0))));
      memcpy(bytes + offset, &be, sizeof(be));
    };
    auto put32 = [&bytes](int offset, double value)
    {
      const uint32_t be = CFSwapInt32HostToBig(static_cast<uint32_t>(
          std::llround(std::clamp(std::isfinite(value) ? value : 0.0, 0.0, 4294967295.0))));
      memcpy(bytes + offset, &be, sizeof(be));
    };
    const auto& metadata = picture.displayMetadata;
    for (int i = 0; i < 3; ++i)
    {
      const int primary = (i + 1) % 3;
      put16(i * 4, av_q2d(metadata.display_primaries[primary][0]) * 50000.0);
      put16(i * 4 + 2, av_q2d(metadata.display_primaries[primary][1]) * 50000.0);
    }
    put16(12, av_q2d(metadata.white_point[0]) * 50000.0);
    put16(14, av_q2d(metadata.white_point[1]) * 50000.0);
    put32(16, av_q2d(metadata.max_luminance) * 10000.0);
    put32(20, av_q2d(metadata.min_luminance) * 10000.0);
    CFDataRef data = CFDataCreate(kCFAllocatorDefault, bytes, sizeof(bytes));
    CVBufferSetAttachment(pixelBuffer, kCVImageBufferMasteringDisplayColorVolumeKey, data,
                          kCVAttachmentMode_ShouldPropagate);
    CFRelease(data);
  }
  else
    CVBufferRemoveAttachment(pixelBuffer, kCVImageBufferMasteringDisplayColorVolumeKey);

  if (picture.hasLightMetadata)
  {
    uint16_t values[2] = {
        CFSwapInt16HostToBig(static_cast<uint16_t>(picture.lightMetadata.MaxCLL)),
        CFSwapInt16HostToBig(static_cast<uint16_t>(picture.lightMetadata.MaxFALL))};
    CFDataRef data =
        CFDataCreate(kCFAllocatorDefault, reinterpret_cast<const UInt8*>(values), sizeof(values));
    CVBufferSetAttachment(pixelBuffer, kCVImageBufferContentLightLevelInfoKey, data,
                          kCVAttachmentMode_ShouldPropagate);
    CFRelease(data);
  }
  else
    CVBufferRemoveAttachment(pixelBuffer, kCVImageBufferContentLightLevelInfoKey);
}
} // namespace

CBaseRenderer* CRendererVTBDisplayLayer::Create(CVideoBuffer* buffer)
{
  auto* vtb = dynamic_cast<VTB::CVideoBufferVTB*>(buffer);
  if (!vtb || !vtb->GetPB() ||
      CVPixelBufferGetPixelFormatType(vtb->GetPB()) !=
          kCVPixelFormatType_420YpCbCr10BiPlanarVideoRange)
    return nullptr;
  return new CRendererVTBDisplayLayer();
}

bool CRendererVTBDisplayLayer::Register()
{
  VIDEOPLAYER::CRendererFactory::RegisterRenderer("vtbav", Create);
  return true;
}

CRendererVTBDisplayLayer::~CRendererVTBDisplayLayer()
{
  UnInit();
}

bool CRendererVTBDisplayLayer::Configure(const VideoPicture& picture,
                                         float fps,
                                         unsigned int orientation)
{
  auto* vtb = dynamic_cast<VTB::CVideoBufferVTB*>(picture.videoBuffer);
  if (!vtb || !vtb->GetPB() || ![g_xbmcController enableVideoLayer])
    return false;

  m_sourceWidth = picture.iWidth;
  m_sourceHeight = picture.iHeight;
  m_renderOrientation = orientation;
  m_fps = fps;
  m_hdrType = picture.hdrType;
  CalculateFrameAspectRatio(picture.iDisplayWidth, picture.iDisplayHeight);
  SetViewMode(m_videoSettings.m_ViewMode);
  ManageRenderArea();

  SetColorAttachments(vtb->GetPB(), picture);
  const bool hdrRequested = CServiceBroker::GetWinSystem()->SetHDR(&picture);
  CLog::Log(LOGINFO, "CRendererVTBDisplayLayer::Configure: P010 video layer, HDR request {}",
            hdrRequested ? "accepted" : "rejected");
  m_configured = true;
  return true;
}

bool CRendererVTBDisplayLayer::ConfigChanged(const VideoPicture& picture)
{
  return picture.hdrType != m_hdrType;
}

void CRendererVTBDisplayLayer::AddVideoPicture(const VideoPicture& picture, int index)
{
  ReleaseBuffer(index);
  if (index == m_lastIndex)
    m_lastIndex = -1;
  auto* vtb = dynamic_cast<VTB::CVideoBufferVTB*>(picture.videoBuffer);
  if (!vtb || !vtb->GetPB())
    return;
  SetColorAttachments(vtb->GetPB(), picture);
  m_buffers[index] = picture.videoBuffer;
  m_buffers[index]->Acquire();
}

void CRendererVTBDisplayLayer::ReleaseBuffer(int index)
{
  if (m_buffers[index])
  {
    m_buffers[index]->Release();
    m_buffers[index] = nullptr;
  }
}

void CRendererVTBDisplayLayer::UnInit()
{
  if (!m_configured)
    return;
  for (int i = 0; i < NUM_BUFFERS; ++i)
    ReleaseBuffer(i);
  CServiceBroker::GetWinSystem()->SetHDR(nullptr);
  [g_xbmcController disableVideoLayer];
  m_lastIndex = -1;
  m_configured = false;
}

bool CRendererVTBDisplayLayer::Flush(bool saveBuffers)
{
  [g_xbmcController flushVideoLayer];
  m_lastIndex = -1;
  if (!saveBuffers)
  {
    for (int i = 0; i < NUM_BUFFERS; ++i)
      ReleaseBuffer(i);
  }
  return true;
}

void CRendererVTBDisplayLayer::Update()
{
  if (!m_configured)
    return;
  ManageRenderArea();
  if (m_destRect != m_lastRect)
  {
    const auto& gfx = CServiceBroker::GetWinSystem()->GetGfxContext();
    const CGSize bounds = g_xbmcController.glView.bounds.size;
    const CGFloat sx = bounds.width / gfx.GetWidth();
    const CGFloat sy = bounds.height / gfx.GetHeight();
    [g_xbmcController
        setVideoLayerFrame:CGRectMake(m_destRect.x1 * sx, m_destRect.y1 * sy,
                                      m_destRect.Width() * sx, m_destRect.Height() * sy)];
    m_lastRect = m_destRect;
  }
}

void CRendererVTBDisplayLayer::RenderUpdate(
    int index, int index2, bool clear, unsigned int flags, unsigned int alpha)
{
  if (!m_configured || index == m_lastIndex)
    return;
  Update();
  auto* vtb = dynamic_cast<VTB::CVideoBufferVTB*>(m_buffers[index]);
  if (!vtb || !vtb->GetPB())
    return;

  CMVideoFormatDescriptionRef format = nullptr;
  if (CMVideoFormatDescriptionCreateForImageBuffer(kCFAllocatorDefault, vtb->GetPB(), &format) !=
      noErr)
    return;
  CMSampleBufferRef sample = nullptr;
  const OSStatus status =
      CMSampleBufferCreateForImageBuffer(kCFAllocatorDefault, vtb->GetPB(), true, nullptr, nullptr,
                                         format, &kCMTimingInfoInvalid, &sample);
  CFRelease(format);
  if (status != noErr || !sample)
    return;

  CFArrayRef attachments = CMSampleBufferGetSampleAttachmentsArray(sample, true);
  if (attachments && CFArrayGetCount(attachments) > 0)
  {
    auto* dictionary = static_cast<CFMutableDictionaryRef>(
        const_cast<void*>(CFArrayGetValueAtIndex(attachments, 0)));
    CFDictionarySetValue(dictionary, kCMSampleAttachmentKey_DisplayImmediately, kCFBooleanTrue);
  }
  [g_xbmcController enqueueVideoSampleBuffer:sample];
  CFRelease(sample);
  m_lastIndex = index;
}

CRenderInfo CRendererVTBDisplayLayer::GetRenderInfo()
{
  CRenderInfo info;
  info.max_buffer_size = NUM_BUFFERS;
  return info;
}
