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
#include <iterator>
#include <limits>
#include <type_traits>

#include <CoreMedia/CMSampleBuffer.h>
#include <CoreVideo/CVImageBuffer.h>
#include <CoreVideo/CVPixelBuffer.h>
#import <Foundation/NSData.h>

namespace
{
template<typename T>
void PutBigEndian(uint8_t* bytes, size_t offset, double value)
{
  static_assert(std::is_same_v<T, uint16_t> || std::is_same_v<T, uint32_t>);
  const T host = static_cast<T>(std::llround(std::clamp(
      std::isfinite(value) ? value : 0.0, 0.0,
      static_cast<double>(std::numeric_limits<T>::max()))));
  T bigEndian;
  if constexpr (std::is_same_v<T, uint16_t>)
    bigEndian = CFSwapInt16HostToBig(host);
  else
    bigEndian = CFSwapInt32HostToBig(host);
  memcpy(bytes + offset, &bigEndian, sizeof(bigEndian));
}

VTB::CVideoBufferVTB* GetVTBBuffer(CVideoBuffer* buffer)
{
  auto* vtb = dynamic_cast<VTB::CVideoBufferVTB*>(buffer);
  return vtb && vtb->GetPB() ? vtb : nullptr;
}

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
    // Mastering display colour volume SEI uses 24 big-endian bytes in G, B, R,
    // white point, max/min luminance order (ISO/IEC 23008-2 D.2.28).
    // Chromaticity coordinates use 1/50000 units; luminance uses 1/10000 cd/m^2.
    constexpr double chromaticityScale = 50000.0;
    constexpr double luminanceScale = 10000.0;
    uint8_t bytes[24]{};
    size_t offset = 0;
    auto put16 = [&bytes, &offset](double value)
    {
      PutBigEndian<uint16_t>(bytes, offset, value);
      offset += sizeof(uint16_t);
    };
    auto put32 = [&bytes, &offset](double value)
    {
      PutBigEndian<uint32_t>(bytes, offset, value);
      offset += sizeof(uint32_t);
    };
    const auto& metadata = picture.displayMetadata;
    for (size_t i = 0; i < std::size(metadata.display_primaries); ++i)
    {
      const size_t primary = (i + 1) % std::size(metadata.display_primaries);
      put16(av_q2d(metadata.display_primaries[primary][0]) * chromaticityScale);
      put16(av_q2d(metadata.display_primaries[primary][1]) * chromaticityScale);
    }
    put16(av_q2d(metadata.white_point[0]) * chromaticityScale);
    put16(av_q2d(metadata.white_point[1]) * chromaticityScale);
    put32(av_q2d(metadata.max_luminance) * luminanceScale);
    put32(av_q2d(metadata.min_luminance) * luminanceScale);
    NSData* data = [NSData dataWithBytes:bytes length:sizeof(bytes)];
    CVBufferSetAttachment(pixelBuffer, kCVImageBufferMasteringDisplayColorVolumeKey,
                          (__bridge CFDataRef)data,
                          kCVAttachmentMode_ShouldPropagate);
  }
  else
    CVBufferRemoveAttachment(pixelBuffer, kCVImageBufferMasteringDisplayColorVolumeKey);

  if (picture.hasLightMetadata)
  {
    const unsigned maxContentLight = std::numeric_limits<uint16_t>::max();
    uint16_t values[2] = {
        CFSwapInt16HostToBig(
            static_cast<uint16_t>(std::min(picture.lightMetadata.MaxCLL, maxContentLight))),
        CFSwapInt16HostToBig(
            static_cast<uint16_t>(std::min(picture.lightMetadata.MaxFALL, maxContentLight)))};
    NSData* data = [NSData dataWithBytes:values length:sizeof(values)];
    CVBufferSetAttachment(pixelBuffer, kCVImageBufferContentLightLevelInfoKey,
                          (__bridge CFDataRef)data,
                          kCVAttachmentMode_ShouldPropagate);
  }
  else
    CVBufferRemoveAttachment(pixelBuffer, kCVImageBufferContentLightLevelInfoKey);
}
} // namespace

CBaseRenderer* CRendererVTBDisplayLayer::Create(CVideoBuffer* buffer)
{
  auto* vtb = GetVTBBuffer(buffer);
  if (!vtb ||
      CVPixelBufferGetPixelFormatType(vtb->GetPB()) !=
          kCVPixelFormatType_420YpCbCr10BiPlanarVideoRange)
    return nullptr;
  return new CRendererVTBDisplayLayer;
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
  auto* vtb = GetVTBBuffer(picture.videoBuffer);
  if (!vtb)
  {
    CLog::Log(LOGERROR, "CRendererVTBDisplayLayer::Configure: missing VideoToolbox pixel buffer");
    return false;
  }
  if (![g_xbmcController enableVideoLayer])
  {
    CLog::Log(LOGERROR, "CRendererVTBDisplayLayer::Configure: unable to enable video layer");
    return false;
  }

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
  auto* vtb = GetVTBBuffer(picture.videoBuffer);
  if (!vtb)
  {
    CLog::Log(LOGERROR, "CRendererVTBDisplayLayer::AddVideoPicture: missing VideoToolbox pixel buffer");
    return;
  }
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
    const CGSize boundsSize = g_xbmcController.glView.bounds.size;
    const CGFloat sx = boundsSize.width / gfx.GetWidth();
    const CGFloat sy = boundsSize.height / gfx.GetHeight();
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
  auto* vtb = GetVTBBuffer(m_buffers[index]);
  if (!vtb)
  {
    CLog::Log(LOGERROR, "CRendererVTBDisplayLayer::RenderUpdate: missing VideoToolbox pixel buffer");
    return;
  }

  CMVideoFormatDescriptionRef format = nullptr;
  const OSStatus formatStatus =
      CMVideoFormatDescriptionCreateForImageBuffer(kCFAllocatorDefault, vtb->GetPB(), &format);
  if (formatStatus != noErr)
  {
    CLog::Log(LOGERROR, "CRendererVTBDisplayLayer::RenderUpdate: format creation failed ({})",
              formatStatus);
    return;
  }
  CMSampleBufferRef sample = nullptr;
  const OSStatus status =
      CMSampleBufferCreateForImageBuffer(kCFAllocatorDefault, vtb->GetPB(), true, nullptr, nullptr,
                                         format, &kCMTimingInfoInvalid, &sample);
  CFRelease(format);
  if (status != noErr || !sample)
  {
    CLog::Log(LOGERROR, "CRendererVTBDisplayLayer::RenderUpdate: sample creation failed ({})",
              status);
    if (sample)
      CFRelease(sample);
    return;
  }

  CFArrayRef attachments = CMSampleBufferGetSampleAttachmentsArray(sample, true);
  if (!attachments || CFArrayGetCount(attachments) != 1)
  {
    CLog::Log(LOGERROR, "CRendererVTBDisplayLayer::RenderUpdate: missing sample attachments");
    CFRelease(sample);
    return;
  }
  // CMSampleBufferCreateForImageBuffer creates one sample with one mutable dictionary.
  auto dictionary = static_cast<CFMutableDictionaryRef>(
      const_cast<void*>(CFArrayGetValueAtIndex(attachments, 0)));
  CFDictionarySetValue(dictionary, kCMSampleAttachmentKey_DisplayImmediately, kCFBooleanTrue);
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
