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
#include "rendering/capture/CaptureConvert.h"
#include "rendering/capture/CaptureTypes.h"
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
#include <memory>
#include <mutex>
#include <new>
#include <type_traits>
#include <vector>

#include <CoreImage/CoreImage.h>
#include <CoreMedia/CMSampleBuffer.h>
#include <CoreVideo/CVImageBuffer.h>
#include <CoreVideo/CVPixelBuffer.h>
#import <Foundation/NSData.h>
#include <ImageIO/CGImageProperties.h>

namespace
{
using namespace KODI::RENDERING::CAPTURE;

class CCompositeCapturePixels : public ICapturePixels
{
public:
  CCompositeCapturePixels(CaptureResult gui, CaptureResult video, unsigned int x, unsigned int y)
    : m_gui(std::move(gui)),
      m_video(std::move(video)),
      m_x(x),
      m_y(y)
  {
  }

  const uint8_t* Lock() override
  {
    // Both inputs own their pixels; defer tonemapping and blending to the consumer.
    std::call_once(
        m_once,
        [this]
        {
          auto pixels =
              std::make_unique<uint8_t[]>(static_cast<size_t>(m_gui.width) * m_gui.height * 4);
          std::vector<uint8_t> video(static_cast<size_t>(m_video.width) * m_video.height * 4);
          if (!CaptureCopyBGRA8(m_gui, m_gui.width, m_gui.height, pixels.get()) ||
              !CaptureToBGRA(m_video, m_video.width, m_video.height, video.data()))
            return;

          for (unsigned int y = 0; y < m_gui.height; ++y)
          {
            for (unsigned int x = 0; x < m_gui.width; ++x)
            {
              uint8_t* gui = pixels.get() + (static_cast<size_t>(y) * m_gui.width + x) * 4;
              if (x >= m_x && x - m_x < m_video.width && y >= m_y && y - m_y < m_video.height)
              {
                const uint8_t* src =
                    video.data() + (static_cast<size_t>(y - m_y) * m_video.width + x - m_x) * 4;
                for (unsigned int channel = 0; channel < 3; ++channel)
                {
                  // The GLES GUI is already premultiplied by its coverage alpha.
                  gui[channel] = static_cast<uint8_t>(std::min(
                      255U, gui[channel] + (src[channel] * (255U - gui[3]) + 127U) / 255U));
                }
              }
              gui[3] = 255;
            }
          }
          m_pixels = std::move(pixels);
        });
    return m_pixels.get();
  }

private:
  CaptureResult m_gui;
  CaptureResult m_video;
  unsigned int m_x;
  unsigned int m_y;
  std::once_flag m_once;
  std::unique_ptr<uint8_t[]> m_pixels;
};

template<typename T>
void PutBigEndian(uint8_t* bytes, size_t offset, double value)
{
  static_assert(std::is_same_v<T, uint16_t> || std::is_same_v<T, uint32_t>);
  const T host =
      static_cast<T>(std::llround(std::clamp(std::isfinite(value) ? value : 0.0, 0.0,
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

CGRect ToCGRect(const CRect& rect)
{
  return CGRectMake(static_cast<CGFloat>(rect.x1), static_cast<CGFloat>(rect.y1),
                    static_cast<CGFloat>(rect.Width()), static_cast<CGFloat>(rect.Height()));
}

void SetColorAttachments(CVPixelBufferRef pixelBuffer, const VideoPicture& picture)
{
  if (picture.hdrType != StreamHdrType::HDR_TYPE_HDR10 &&
      picture.hdrType != StreamHdrType::HDR_TYPE_HDR10PLUS &&
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

  if (picture.hdrType != StreamHdrType::HDR_TYPE_HDR10 &&
      picture.hdrType != StreamHdrType::HDR_TYPE_HDR10PLUS)
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
                          (__bridge CFDataRef)data, kCVAttachmentMode_ShouldPropagate);
  }
  else
    CVBufferRemoveAttachment(pixelBuffer, kCVImageBufferMasteringDisplayColorVolumeKey);

  if (picture.hasLightMetadata)
  {
    const unsigned maxContentLight = std::numeric_limits<uint16_t>::max();
    uint16_t values[2] = {CFSwapInt16HostToBig(static_cast<uint16_t>(
                              std::min(picture.lightMetadata.MaxCLL, maxContentLight))),
                          CFSwapInt16HostToBig(static_cast<uint16_t>(
                              std::min(picture.lightMetadata.MaxFALL, maxContentLight)))};
    NSData* data = [NSData dataWithBytes:values length:sizeof(values)];
    CVBufferSetAttachment(pixelBuffer, kCVImageBufferContentLightLevelInfoKey,
                          (__bridge CFDataRef)data, kCVAttachmentMode_ShouldPropagate);
  }
  else
    CVBufferRemoveAttachment(pixelBuffer, kCVImageBufferContentLightLevelInfoKey);
}
} // namespace

CBaseRenderer* CRendererVTBDisplayLayer::Create(CVideoBuffer* buffer)
{
  auto* vtb = GetVTBBuffer(buffer);
  if (!vtb)
    return nullptr;
  const OSType format = CVPixelBufferGetPixelFormatType(vtb->GetPB());
  if (format != kCVPixelFormatType_420YpCbCr10BiPlanarVideoRange &&
      format != kCVPixelFormatType_420YpCbCr10BiPlanarFullRange)
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
  m_lastRect = {};
  m_lastSourceRect = {};
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
  auto* vtb = GetVTBBuffer(picture.videoBuffer);
  if (!vtb)
  {
    CLog::Log(LOGERROR,
              "CRendererVTBDisplayLayer::AddVideoPicture: missing VideoToolbox pixel buffer");
    return;
  }
  SetColorAttachments(vtb->GetPB(), picture);
  m_buffers[index] = picture.videoBuffer;
  m_buffers[index]->Acquire();
  m_capturePictures[index].SetParams(picture);
}

void CRendererVTBDisplayLayer::ReleaseBuffer(int index)
{
  // Recycling a slot must not invalidate a newer presented frame.
  int expectedIndex = index;
  m_lastIndex.compare_exchange_strong(expectedIndex, -1);
  if (m_buffers[index])
  {
    m_buffers[index]->Release();
    m_buffers[index] = nullptr;
  }
  m_capturePictures[index].Reset();
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
  if (m_destRect != m_lastRect || m_sourceRect != m_lastSourceRect)
  {
    const auto& gfx = CServiceBroker::GetWinSystem()->GetGfxContext();
    const CGSize boundsSize = g_xbmcController.glView.bounds.size;
    const CGFloat sx = boundsSize.width / gfx.GetWidth();
    const CGFloat sy = boundsSize.height / gfx.GetHeight();
    // UIKit uses points; the source crop is normalized in the unrotated surface.
    [g_xbmcController
        setVideoLayerFrame:CGRectApplyAffineTransform(ToCGRect(m_destRect),
                                                      CGAffineTransformMakeScale(sx, sy))
                sourceRect:CGRectApplyAffineTransform(
                               ToCGRect(m_sourceRect),
                               CGAffineTransformMakeScale(1.0 / m_sourceWidth,
                                                          1.0 / m_sourceHeight))
               orientation:m_renderOrientation];
    m_lastRect = m_destRect;
    m_lastSourceRect = m_sourceRect;
  }
}

void CRendererVTBDisplayLayer::RenderUpdate(
    int index, int index2, bool clear, unsigned int flags, unsigned int alpha)
{
  if (!m_configured)
    return;
  Update();
  if (index == m_lastIndex.load())
    return;
  auto* vtb = GetVTBBuffer(m_buffers[index]);
  if (!vtb)
  {
    CLog::Log(LOGERROR,
              "CRendererVTBDisplayLayer::RenderUpdate: missing VideoToolbox pixel buffer");
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

// Native video bypasses the framebuffer. Convert P010 to packed RGB while
// preserving HDR coding and metadata for the capture consumer.
bool CRendererVTBDisplayLayer::CaptureVideoFrame(const KODI::RENDERING::CAPTURE::CaptureSpec& spec,
                                                 KODI::RENDERING::CAPTURE::CaptureResult& result)
{
  using namespace KODI::RENDERING::CAPTURE;

  const int index = m_lastIndex.load();
  if (!m_configured || index < 0)
    return false;
  auto* vtb = GetVTBBuffer(m_buffers[index]);
  if (!vtb)
    return false;

  const auto& gfx = CServiceBroker::GetWinSystem()->GetGfxContext();
  CRect visible = m_destRect;
  visible.Intersect(CRect(0, 0, gfx.GetWidth(), gfx.GetHeight()));
  if (visible.IsEmpty())
    return false;

  const unsigned int width = spec.width ? spec.width : static_cast<unsigned int>(visible.Width());
  const unsigned int height =
      spec.height ? spec.height : static_cast<unsigned int>(visible.Height());
  const bool native = spec.format == CaptureFormat::NATIVE;
  const unsigned int bytesPerPixel = native ? 8 : 4;
  // bytesPerPixel scales the int row stride, not the height. The total
  // allocation is computed in size_t on 64-bit tvOS.
  if (!width || !height || width > std::numeric_limits<int>::max() / bytesPerPixel ||
      height > std::numeric_limits<int>::max())
    return false;
  const int stride = width * bytesPerPixel;
  auto pixels =
      std::unique_ptr<uint8_t[]>(new (std::nothrow) uint8_t[static_cast<size_t>(stride) * height]);
  if (!pixels)
    return false;

  @autoreleasepool
  {
    static CIContext* context = [[CIContext alloc] initWithOptions:@{
      kCIContextWorkingColorSpace : [NSNull null],
      kCIContextOutputColorSpace : [NSNull null],
      kCIContextCacheIntermediates : @NO
    }];
    CIImage* image = [CIImage imageWithCVPixelBuffer:vtb->GetPB()
                                             options:@{kCIImageColorSpace : [NSNull null]}];
    if (!context || !image)
      return false;

    const CGImagePropertyOrientation orientation = [this]
    {
      switch (m_renderOrientation)
      {
        case 90:
          return kCGImagePropertyOrientationRight;
        case 180:
          return kCGImagePropertyOrientationDown;
        case 270:
          return kCGImagePropertyOrientationLeft;
        default:
          return kCGImagePropertyOrientationUp;
      }
    }();
    const CGRect extent = image.extent;
    // Convert decoder pixels from a top-left origin to Core Image's bottom-left origin.
    const CGAffineTransform sourceToImageScale = CGAffineTransformMakeScale(
        extent.size.width / m_sourceWidth, -extent.size.height / m_sourceHeight);
    const CGRect scaledSource =
        CGRectApplyAffineTransform(ToCGRect(m_sourceRect), sourceToImageScale);
    const CGRect imageSource = CGRectOffset(scaledSource, extent.origin.x, CGRectGetMaxY(extent));
    const CGRect rotatedSource =
        CGRectApplyAffineTransform(imageSource, [image imageTransformForCGOrientation:orientation]);
    image = [image imageByApplyingCGOrientation:orientation];
    // The destination is in GUI pixels. Map its visible portion into the
    // already-oriented source crop above.
    const CGRect destination = ToCGRect(m_destRect);
    const CGRect visibleInDestination =
        CGRectOffset(ToCGRect(visible), -destination.origin.x, -destination.origin.y);
    const CGAffineTransform destinationToSourceScale =
        CGAffineTransformMakeScale(rotatedSource.size.width / destination.size.width,
                                   -rotatedSource.size.height / destination.size.height);
    const CGRect scaledVisible =
        CGRectApplyAffineTransform(visibleInDestination, destinationToSourceScale);
    const CGRect crop =
        CGRectOffset(scaledVisible, rotatedSource.origin.x, CGRectGetMaxY(rotatedSource));
    // Clamp the full image to avoid transparent edges at fractional crop boundaries.
    image = [image imageByClampingToExtent];
    image = [image
        imageByApplyingTransform:CGAffineTransformMakeTranslation(-crop.origin.x, -crop.origin.y)];
    image = [image imageByApplyingTransform:CGAffineTransformMakeScale(width / crop.size.width,
                                                                       height / crop.size.height)];
    [context render:image
           toBitmap:pixels.get()
           rowBytes:stride
             bounds:CGRectMake(0, 0, width, height)
             format:native ? kCIFormatRGBA16 : kCIFormatBGRA8
         colorSpace:nullptr];
  }

  const auto& picture = m_capturePictures[index];
  result.pixels = std::make_shared<CHeapCapturePixels>(std::move(pixels));
  result.width = width;
  result.height = height;
  result.stride = stride;
  result.format = native ? AV_PIX_FMT_RGBA64LE : AV_PIX_FMT_BGRA;
  result.color.primaries = AVCOL_PRI_BT2020;
  result.color.transfer =
      picture.hdrType == StreamHdrType::HDR_TYPE_HLG ? AVCOL_TRC_ARIB_STD_B67 : AVCOL_TRC_SMPTE2084;
  result.color.range = AVCOL_RANGE_JPEG;
  result.content = CaptureContent::VIDEO;
  result.hasDisplayMetadata = picture.hasDisplayMetadata;
  result.displayMetadata = picture.displayMetadata;
  result.hasLightMetadata = picture.hasLightMetadata;
  result.lightMetadata = picture.lightMetadata;
  return true;
}

bool CRendererVTBDisplayLayer::CompositeVideoCapture(
    KODI::RENDERING::CAPTURE::CaptureResult& result)
{
  using namespace KODI::RENDERING::CAPTURE;

  if (!m_configured)
    return true;
  if (!result.pixels || !result.width || !result.height ||
      result.width > std::numeric_limits<int>::max() / 4)
    return false;

  const auto& gfx = CServiceBroker::GetWinSystem()->GetGfxContext();
  CRect visible = m_destRect;
  visible.Intersect(CRect(0, 0, gfx.GetWidth(), gfx.GetHeight()));
  if (visible.IsEmpty())
    return true;

  const double sx = static_cast<double>(result.width) / gfx.GetWidth();
  const double sy = static_cast<double>(result.height) / gfx.GetHeight();
  const auto x = static_cast<unsigned int>(std::lround(static_cast<double>(visible.x1) * sx));
  const auto y = static_cast<unsigned int>(std::lround(static_cast<double>(visible.y1) * sy));
  CaptureSpec spec;
  spec.format = CaptureFormat::NATIVE;
  spec.width = static_cast<unsigned int>(std::lround(static_cast<double>(visible.x2) * sx)) - x;
  spec.height = static_cast<unsigned int>(std::lround(static_cast<double>(visible.y2) * sy)) - y;
  if (!spec.width || !spec.height)
    return true;

  CaptureResult video;
  if (!CaptureVideoFrame(spec, video))
    return false;
  result.pixels = std::make_shared<CCompositeCapturePixels>(result, std::move(video), x, y);
  result.stride = static_cast<int>(result.width * 4);
  result.format = AV_PIX_FMT_BGRA;
  result.color = {AVCOL_PRI_BT709, AVCOL_TRC_BT709, AVCOL_RANGE_JPEG};
  result.content = CaptureContent::COMPOSITE;
  result.hasDisplayMetadata = false;
  result.displayMetadata = {};
  result.hasLightMetadata = false;
  result.lightMetadata = {};
  return true;
}

CRenderInfo CRendererVTBDisplayLayer::GetRenderInfo()
{
  CRenderInfo info;
  info.max_buffer_size = NUM_BUFFERS;
  return info;
}
