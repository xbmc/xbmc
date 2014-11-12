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

#include "config.h"

#if defined(TARGET_DARWIN_OSX)
#include "system_gl.h"
#include "DVDVideoCodecVDA.h"

extern "C" {
#include "libswscale/swscale.h"
}

#include "DVDClock.h"
#include "DVDStreamInfo.h"
#include "cores/dvdplayer/DVDCodecs/DVDCodecUtils.h"
#include "cores/FFmpeg.h"
#include "osx/CocoaInterface.h"
#include "windowing/WindowingFactory.h"
#include "utils/BitstreamConverter.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/TimeUtils.h"
#include "settings/Settings.h"
#include "settings/AdvancedSettings.h"

#include <CoreFoundation/CoreFoundation.h>
#include <VideoDecodeAcceleration/VDADecoder.h>

// extra flags not defined in VDADecoder.h
enum {
  kVDADecodeInfo_Asynchronous = 1UL << 0,
  kVDADecodeInfo_FrameDropped = 1UL << 1
};

/*
 how to detect the interlacing used on an existing stream:
- progressive is signalled by setting
   frame_mbs_only_flag: 1 in the SPS.
- interlaced is signalled by setting
   frame_mbs_only_flag: 0 in the SPS and
   field_pic_flag: 1 on all frames.
- paff is signalled by setting
   frame_mbs_only_flag: 0 in the SPS and
   field_pic_flag: 1 on all frames that get interlaced and
   field_pic_flag: 0 on all frames that get progressive.
- mbaff is signalled by setting
   frame_mbs_only_flag: 0 in the SPS and
   mb_adaptive_frame_field_flag: 1 in the SPS and
   field_pic_flag: 0 on the frames,
   (field_pic_flag: 1 would indicate a normal interlaced frame).
*/

// tracks a frame in and output queue in display order
typedef struct frame_queue {
  double              dts;
  double              pts;
  int64_t             sort_time;
  FourCharCode        pixel_buffer_format;
  CVBufferRef         pixel_buffer_ref;
  struct frame_queue  *nextframe;
} frame_queue;

////////////////////////////////////////////////////////////////////////////////////////////
// helper function that wraps dts/pts into a dictionary
static CFDictionaryRef CreateDictionaryWithDisplayTime(int64_t time, double dts, double pts)
{
  CFStringRef key[3] = {
    CFSTR("VideoDisplay_TIME"),
    CFSTR("VideoDisplay_DTS"),
    CFSTR("VideoDisplay_PTS")};
  CFNumberRef value[3];
  CFDictionaryRef display_time;

  value[0] = CFNumberCreate(kCFAllocatorDefault, kCFNumberLongLongType, &time);
  value[1] = CFNumberCreate(kCFAllocatorDefault, kCFNumberDoubleType, &dts);
  value[2] = CFNumberCreate(kCFAllocatorDefault, kCFNumberDoubleType, &pts);

  display_time = CFDictionaryCreate(
    kCFAllocatorDefault, (const void **)&key, (const void **)&value, 3,
    &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
  
  CFRelease(value[0]);
  CFRelease(value[1]);
  CFRelease(value[2]);

  return display_time;
}

// helper function to extract dts/pts from a dictionary
static void GetFrameDisplayTimeFromDictionary(
  CFDictionaryRef inFrameInfoDictionary, frame_queue *frame)
{
  // default to DVD_NOPTS_VALUE
  frame->sort_time = -1;
  frame->dts = DVD_NOPTS_VALUE;
  frame->pts = DVD_NOPTS_VALUE;
  if (inFrameInfoDictionary == NULL)
    return;

  CFNumberRef value[3];
  //
  value[0] = (CFNumberRef)CFDictionaryGetValue(inFrameInfoDictionary, CFSTR("VideoDisplay_TIME"));
  if (value[0])
    CFNumberGetValue(value[0], kCFNumberLongLongType, &frame->sort_time);
  value[1] = (CFNumberRef)CFDictionaryGetValue(inFrameInfoDictionary, CFSTR("VideoDisplay_DTS"));
  if (value[1])
    CFNumberGetValue(value[1], kCFNumberDoubleType, &frame->dts);
  value[2] = (CFNumberRef)CFDictionaryGetValue(inFrameInfoDictionary, CFSTR("VideoDisplay_PTS"));
  if (value[2])
    CFNumberGetValue(value[2], kCFNumberDoubleType, &frame->pts);

  return;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////
CDVDVideoCodecVDA::CDVDVideoCodecVDA() : CDVDVideoCodec()
{
  m_vda_decoder = NULL;
  m_pFormatName = "vda-";

  m_queue_depth = 0;
  m_display_queue = NULL;
  pthread_mutex_init(&m_queue_mutex, NULL);

  m_bitstream = NULL;
  memset(&m_videobuffer, 0, sizeof(DVDVideoPicture));
  m_DropPictures = false;
  m_decode_async = false;
  m_sort_time = 0;
  m_use_cvBufferRef = false;
}

CDVDVideoCodecVDA::~CDVDVideoCodecVDA()
{
  Dispose();
  pthread_mutex_destroy(&m_queue_mutex);
}

bool CDVDVideoCodecVDA::Open(CDVDStreamInfo &hints, CDVDCodecOptions &options)
{
  if (CSettings::Get().GetBool("videoplayer.usevda") && !hints.software)
  {
    CCocoaAutoPool pool;

    //
    int width  = hints.width;
    int height = hints.height;
    int level  = hints.level;
    int profile = hints.profile;
    
    switch(profile)
    {
      case FF_PROFILE_H264_HIGH_10:
      case FF_PROFILE_H264_HIGH_10_INTRA:
      case FF_PROFILE_H264_HIGH_422:
      case FF_PROFILE_H264_HIGH_422_INTRA:
      case FF_PROFILE_H264_HIGH_444_PREDICTIVE:
      case FF_PROFILE_H264_HIGH_444_INTRA:
      case FF_PROFILE_H264_CAVLC_444:
        CLog::Log(LOGNOTICE, "%s - unsupported h264 profile(%d)", __FUNCTION__, hints.profile);
        return false;
        break;
    }

    if (width <= 0 || height <= 0)
    {
      CLog::Log(LOGNOTICE, "%s - bailing with bogus hints, width(%d), height(%d)",
        __FUNCTION__, width, height);
      return false;
    }
    
    if (Cocoa_GPUForDisplayIsNvidiaPureVideo3() && !CDVDCodecUtils::IsVP3CompatibleWidth(width))
    {
      CLog::Log(LOGNOTICE, "%s - Nvidia 9400 GPU hardware limitation, cannot decode a width of %d", __FUNCTION__, width);
      return false;
    }

    CFDataRef avcCData;
    switch (hints.codec)
    {
      case AV_CODEC_ID_H264:
        m_bitstream = new CBitstreamConverter;
        if (!m_bitstream->Open(hints.codec, (uint8_t*)hints.extradata, hints.extrasize, false))
          return false;

        avcCData = CFDataCreate(kCFAllocatorDefault,
          (const uint8_t*)m_bitstream->GetExtraData(), m_bitstream->GetExtraSize());

        m_format = 'avc1';
        m_pFormatName = "vda-h264";
      break;
      default:
        return false;
      break;
    }

    // check the avcC atom's sps for number of reference frames and
    // bail if interlaced, VDA does not handle interlaced h264.
    uint32_t avcc_len = CFDataGetLength(avcCData);
    if (avcc_len < 8)
    {
      // avcc atoms with length less than 8 are borked.
      CFRelease(avcCData);
      delete m_bitstream, m_bitstream = NULL;
      return false;
    }
    else
    {
      bool interlaced = true;
      uint8_t *spc = (uint8_t*)CFDataGetBytePtr(avcCData) + 6;
      uint32_t sps_size = BS_RB16(spc);
      if (sps_size)
        m_bitstream->parseh264_sps(spc+3, sps_size-1, &interlaced, &m_max_ref_frames);
      if (interlaced)
      {
        CLog::Log(LOGNOTICE, "%s - possible interlaced content.", __FUNCTION__);
        CFRelease(avcCData);
        return false;
      }
    }

    if (profile == FF_PROFILE_H264_MAIN && level == 32 && m_max_ref_frames > 4)
    {
      // Main@L3.2, VDA cannot handle greater than 4 reference frames
      CLog::Log(LOGNOTICE, "%s - Main@L3.2 detected, VDA cannot decode.", __FUNCTION__);
      CFRelease(avcCData);
      return false;
    }

    std::string rendervendor = g_Windowing.GetRenderVendor();
    StringUtils::ToLower(rendervendor);
    if (rendervendor.find("nvidia") != std::string::npos)
    {
      // Nvidia gpu's are all powerful and work the way god intended
      m_decode_async = true;
      // The gods are liars, ignore the sirens for now.
      m_use_cvBufferRef = false;
    }
    else if (rendervendor.find("intel") != std::string::npos)
    {
      // Intel gpu are borked when using cvBufferRef
      m_decode_async = true;
      m_use_cvBufferRef = false;
    }
    else
    {
      // ATI gpu's are borked when using async decode
      m_decode_async = false;
      // They lie here too.
      m_use_cvBufferRef = false;
    }

    if (!m_use_cvBufferRef)
    {
      // allocate a YV12 DVDVideoPicture buffer.
      // first make sure all properties are reset.
      memset(&m_videobuffer, 0, sizeof(DVDVideoPicture));
      unsigned int iPixels = width * height;
      unsigned int iChromaPixels = iPixels/4;

      m_videobuffer.dts = DVD_NOPTS_VALUE;
      m_videobuffer.pts = DVD_NOPTS_VALUE;
      m_videobuffer.iFlags = DVP_FLAG_ALLOCATED;
      m_videobuffer.format = RENDER_FMT_YUV420P;
      m_videobuffer.color_range  = 0;
      m_videobuffer.color_matrix = 4;
      m_videobuffer.iWidth  = width;
      m_videobuffer.iHeight = height;
      m_videobuffer.iDisplayWidth  = width;
      m_videobuffer.iDisplayHeight = height;

      m_videobuffer.iLineSize[0] = width;   //Y
      m_videobuffer.iLineSize[1] = width/2; //U
      m_videobuffer.iLineSize[2] = width/2; //V
      m_videobuffer.iLineSize[3] = 0;

      m_videobuffer.data[0] = (uint8_t*)malloc(16 + iPixels);
      m_videobuffer.data[1] = (uint8_t*)malloc(16 + iChromaPixels);
      m_videobuffer.data[2] = (uint8_t*)malloc(16 + iChromaPixels);
      m_videobuffer.data[3] = NULL;

      // set all data to 0 for less artifacts.. hmm.. what is black in YUV??
      memset(m_videobuffer.data[0], 0, iPixels);
      memset(m_videobuffer.data[1], 0, iChromaPixels);
      memset(m_videobuffer.data[2], 0, iChromaPixels);
    }

    // setup the decoder configuration dict
    CFMutableDictionaryRef decoderConfiguration = CFDictionaryCreateMutable(
      kCFAllocatorDefault, 4, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);

    CFNumberRef avcWidth  = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &width);
    CFNumberRef avcHeight = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &height);
    CFNumberRef avcFormat = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &m_format);

    CFDictionarySetValue(decoderConfiguration, kVDADecoderConfiguration_Height, avcHeight);
    CFDictionarySetValue(decoderConfiguration, kVDADecoderConfiguration_Width,  avcWidth);
    CFDictionarySetValue(decoderConfiguration, kVDADecoderConfiguration_SourceFormat, avcFormat);
    CFDictionarySetValue(decoderConfiguration, kVDADecoderConfiguration_avcCData, avcCData);

    // release the retained object refs, decoderConfiguration owns them now
    CFRelease(avcWidth);
    CFRelease(avcHeight);
    CFRelease(avcFormat);
    CFRelease(avcCData);

    // setup the destination image buffer dict, vda will output this pict format
    CFMutableDictionaryRef destinationImageBufferAttributes = CFDictionaryCreateMutable(
      kCFAllocatorDefault, 2, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    OSType cvPixelFormatType = kCVPixelFormatType_422YpCbCr8;
    CFNumberRef pixelFormat  = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &cvPixelFormatType);

    // an IOSurface properties dictionary
    CFDictionaryRef iosurfaceDictionary = CFDictionaryCreate(kCFAllocatorDefault,
      NULL, NULL, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);

    CFDictionarySetValue(destinationImageBufferAttributes,
      kCVPixelBufferPixelFormatTypeKey, pixelFormat);
    CFDictionarySetValue(destinationImageBufferAttributes,
      kCVPixelBufferIOSurfacePropertiesKey, iosurfaceDictionary);
    // release the retained object refs, destinationImageBufferAttributes owns them now
    CFRelease(pixelFormat);
    CFRelease(iosurfaceDictionary);

    // create the VDADecoder object
    OSStatus status;
    try
    {
      status = VDADecoderCreate(decoderConfiguration, destinationImageBufferAttributes,
        (VDADecoderOutputCallback*)VDADecoderCallback, this, (VDADecoder*)&m_vda_decoder);
    }
    catch (...)
    {
      CLog::Log(LOGERROR, "%s - exception",__FUNCTION__);
      status = kVDADecoderDecoderFailedErr;
    }
    CFRelease(decoderConfiguration);
    CFRelease(destinationImageBufferAttributes);
    if (status != kVDADecoderNoErr)
    {
      if (status == kVDADecoderDecoderFailedErr)
          CLog::Log(LOGNOTICE, "%s - VDADecoder Codec failed to open, currently in use by another process",
            __FUNCTION__);
      else
          CLog::Log(LOGNOTICE, "%s - VDADecoder Codec failed to open, status(%d), profile(%d), level(%d)",
            __FUNCTION__, (int)status, profile, level);
      return false;
    }

    m_DropPictures = false;
    m_max_ref_frames = std::max(m_max_ref_frames + 1, 5);
    m_sort_time = 0;
    return true;
  }

  return false;
}

void CDVDVideoCodecVDA::Dispose()
{
  CCocoaAutoPool pool;
  if (m_vda_decoder)
    VDADecoderDestroy((VDADecoder)m_vda_decoder), m_vda_decoder = NULL;

  if (!m_use_cvBufferRef && m_videobuffer.iFlags & DVP_FLAG_ALLOCATED)
  {
    free(m_videobuffer.data[0]), m_videobuffer.data[0] = NULL;
    free(m_videobuffer.data[1]), m_videobuffer.data[1] = NULL;
    free(m_videobuffer.data[2]), m_videobuffer.data[2] = NULL;
    m_videobuffer.iFlags = 0;
  }

  if (m_bitstream)
    delete m_bitstream, m_bitstream = NULL;
}

void CDVDVideoCodecVDA::SetDropState(bool bDrop)
{
  m_DropPictures = bDrop;
}

int CDVDVideoCodecVDA::Decode(uint8_t* pData, int iSize, double dts, double pts)
{
  CCocoaAutoPool pool;
  //
  if (pData)
  {
    m_bitstream->Convert(pData, iSize);
    CFDataRef avc_demux = CFDataCreate(kCFAllocatorDefault,
      m_bitstream->GetConvertBuffer(), m_bitstream->GetConvertSize());

    CFDictionaryRef avc_time = CreateDictionaryWithDisplayTime(m_sort_time++, dts, pts);

    uint32_t avc_flags = 0;
    if (m_DropPictures)
      avc_flags = kVDADecoderDecodeFlags_DontEmitFrame;

    OSStatus status = VDADecoderDecode((VDADecoder)m_vda_decoder, avc_flags, avc_demux, avc_time);
    CFRelease(avc_time);
    CFRelease(avc_demux);
    if (status != kVDADecoderNoErr)
    {
      CLog::Log(LOGNOTICE, "%s - VDADecoderDecode failed, status(%d)", __FUNCTION__, (int)status);
      return VC_ERROR;
    }
  }

  if (!m_decode_async)
  {
    // force synchronous decode to fix issues with ATI GPUs,
    // we still have to sort returned frames by pts to handle out-of-order demuxer packets. 
    VDADecoderFlush((VDADecoder)m_vda_decoder, kVDADecoderFlush_EmitFrames);
  }

  if (m_queue_depth < m_max_ref_frames)
    return VC_BUFFER;

  return VC_PICTURE;
}

void CDVDVideoCodecVDA::Reset(void)
{
  CCocoaAutoPool pool;
  VDADecoderFlush((VDADecoder)m_vda_decoder, 0);

  while (m_queue_depth)
    DisplayQueuePop();

  m_sort_time = 0;
}

bool CDVDVideoCodecVDA::GetPicture(DVDVideoPicture* pDvdVideoPicture)
{
  // get the top yuv frame, we risk getting the wrong frame if the frame queue
  // depth is less than the number of encoded reference frames. If queue depth
  // is greater than the number of encoded reference frames, then the top frame
  // will never change and we can just grab a ref to the top frame.
  if (m_use_cvBufferRef)
  {
    pthread_mutex_lock(&m_queue_mutex);
    pDvdVideoPicture->dts             = m_display_queue->dts;
    pDvdVideoPicture->pts             = m_display_queue->pts;
    pDvdVideoPicture->cvBufferRef     = m_display_queue->pixel_buffer_ref;
    m_display_queue->pixel_buffer_ref = NULL;
    pthread_mutex_unlock(&m_queue_mutex);

    pDvdVideoPicture->format          = RENDER_FMT_CVBREF;
    pDvdVideoPicture->iFlags          = DVP_FLAG_ALLOCATED;
    pDvdVideoPicture->color_range     = 0;
    pDvdVideoPicture->color_matrix    = 4;
    pDvdVideoPicture->iWidth          = CVPixelBufferGetWidth(pDvdVideoPicture->cvBufferRef);
    pDvdVideoPicture->iHeight         = CVPixelBufferGetHeight(pDvdVideoPicture->cvBufferRef);
    pDvdVideoPicture->iDisplayWidth   = pDvdVideoPicture->iWidth;
    pDvdVideoPicture->iDisplayHeight  = pDvdVideoPicture->iHeight;
  }
  else
  {
    FourCharCode pixel_buffer_format;
    CVPixelBufferRef picture_buffer_ref;

    // clone the video picture buffer settings.
    *pDvdVideoPicture = m_videobuffer;

    // get the top yuv frame, we risk getting the wrong frame if the frame queue
    // depth is less than the number of encoded reference frames. If queue depth
    // is greater than the number of encoded reference frames, then the top frame
    // will never change and we can just grab a ref to the top frame. This way
    // we don't lockout the vdadecoder while doing color format convert.
    pthread_mutex_lock(&m_queue_mutex);
    picture_buffer_ref = m_display_queue->pixel_buffer_ref;
    pixel_buffer_format = m_display_queue->pixel_buffer_format;
    pDvdVideoPicture->dts = m_display_queue->dts;
    pDvdVideoPicture->pts = m_display_queue->pts;
    pthread_mutex_unlock(&m_queue_mutex);

    // lock the CVPixelBuffer down
    CVPixelBufferLockBaseAddress(picture_buffer_ref, 0);
    int row_stride = CVPixelBufferGetBytesPerRowOfPlane(picture_buffer_ref, 0);
    uint8_t *base_ptr = (uint8_t*)CVPixelBufferGetBaseAddressOfPlane(picture_buffer_ref, 0);
    if (base_ptr)
    {
      if (pixel_buffer_format == kCVPixelFormatType_422YpCbCr8)
        UYVY422_to_YUV420P(base_ptr, row_stride, pDvdVideoPicture);
      else if (pixel_buffer_format == kCVPixelFormatType_32BGRA)
        BGRA_to_YUV420P(base_ptr, row_stride, pDvdVideoPicture);
    }
    // unlock the CVPixelBuffer
    CVPixelBufferUnlockBaseAddress(picture_buffer_ref, 0);
  }

  // now we can pop the top frame.
  DisplayQueuePop();
  //CLog::Log(LOGNOTICE, "%s - VDADecoderDecode dts(%f), pts(%f)", __FUNCTION__,
  //  pDvdVideoPicture->dts, pDvdVideoPicture->pts);

  return true;
}

bool CDVDVideoCodecVDA::ClearPicture(DVDVideoPicture* pDvdVideoPicture)
{
  // release any previous retained image buffer ref that
  // has not been passed up to renderer (ie. dropped frames, etc).
  if (m_use_cvBufferRef && pDvdVideoPicture->cvBufferRef)
    CVBufferRelease(pDvdVideoPicture->cvBufferRef);

  return CDVDVideoCodec::ClearPicture(pDvdVideoPicture);
}

void CDVDVideoCodecVDA::DisplayQueuePop(void)
{
  CCocoaAutoPool pool;
  if (!m_display_queue || m_queue_depth == 0)
    return;

  // pop the top frame off the queue
  pthread_mutex_lock(&m_queue_mutex);
  frame_queue *top_frame = m_display_queue;
  m_display_queue = m_display_queue->nextframe;
  m_queue_depth--;
  pthread_mutex_unlock(&m_queue_mutex);

  // and release it
  if (top_frame->pixel_buffer_ref)
    CVBufferRelease(top_frame->pixel_buffer_ref);
  free(top_frame);
}

void CDVDVideoCodecVDA::UYVY422_to_YUV420P(uint8_t *yuv422_ptr, int yuv422_stride, DVDVideoPicture *picture)
{
  // convert PIX_FMT_UYVY422 to PIX_FMT_YUV420P.
  struct SwsContext *swcontext = sws_getContext(
    m_videobuffer.iWidth, m_videobuffer.iHeight, PIX_FMT_UYVY422, 
    m_videobuffer.iWidth, m_videobuffer.iHeight, PIX_FMT_YUV420P, 
    SWS_FAST_BILINEAR | SwScaleCPUFlags(), NULL, NULL, NULL);
  if (swcontext)
  {
    uint8_t  *src[] = { yuv422_ptr, 0, 0, 0 };
    int srcStride[] = { yuv422_stride, 0, 0, 0 };

    uint8_t  *dst[] = { picture->data[0], picture->data[1], picture->data[2], 0 };
    int dstStride[] = { picture->iLineSize[0], picture->iLineSize[1], picture->iLineSize[2], 0 };

    sws_scale(swcontext, src, srcStride, 0, picture->iHeight, dst, dstStride);
    sws_freeContext(swcontext);
  }
}

void CDVDVideoCodecVDA::BGRA_to_YUV420P(uint8_t *bgra_ptr, int bgra_stride, DVDVideoPicture *picture)
{
  // convert PIX_FMT_BGRA to PIX_FMT_YUV420P.
  struct SwsContext *swcontext = sws_getContext(
    m_videobuffer.iWidth, m_videobuffer.iHeight, PIX_FMT_BGRA, 
    m_videobuffer.iWidth, m_videobuffer.iHeight, PIX_FMT_YUV420P, 
    SWS_FAST_BILINEAR | SwScaleCPUFlags(), NULL, NULL, NULL);
  if (swcontext)
  {
    uint8_t  *src[] = { bgra_ptr, 0, 0, 0 };
    int srcStride[] = { bgra_stride, 0, 0, 0 };

    uint8_t  *dst[] = { picture->data[0], picture->data[1], picture->data[2], 0 };
    int dstStride[] = { picture->iLineSize[0], picture->iLineSize[1], picture->iLineSize[2], 0 };

    sws_scale(swcontext, src, srcStride, 0, picture->iHeight, dst, dstStride);
    sws_freeContext(swcontext);
  }
}

void CDVDVideoCodecVDA::VDADecoderCallback(
  void              *decompressionOutputRefCon,
  CFDictionaryRef    frameInfo,
  OSStatus           status,
  uint32_t           infoFlags,
  CVImageBufferRef   imageBuffer)
{
  CCocoaAutoPool pool;
  // Warning, this is an async callback. There can be multiple frames in flight.
  CDVDVideoCodecVDA *ctx = (CDVDVideoCodecVDA*)decompressionOutputRefCon;

  if (imageBuffer == NULL)
  {
    //CLog::Log(LOGDEBUG, "%s - imageBuffer is NULL", __FUNCTION__);
    return;
  }
  OSType format_type = CVPixelBufferGetPixelFormatType(imageBuffer);
  if ((format_type != kCVPixelFormatType_422YpCbCr8) &&
      (format_type != kCVPixelFormatType_32BGRA) )
  {
    CLog::Log(LOGERROR, "%s - imageBuffer format is not '2vuy' or 'BGRA',is reporting 0x%x",
      __FUNCTION__, (unsigned int)format_type);
    return;
  }
  if (kVDADecodeInfo_FrameDropped & infoFlags)
  {
    if (g_advancedSettings.CanLogComponent(LOGVIDEO))
      CLog::Log(LOGDEBUG, "%s - frame dropped", __FUNCTION__);
    return;
  }

  // allocate a new frame and populate it with some information.
  // this pointer to a frame_queue type keeps track of the newest decompressed frame
  // and is then inserted into a linked list of frame pointers depending on the display time
  // parsed out of the bitstream and stored in the frameInfo dictionary by the client
  frame_queue *newFrame = (frame_queue*)calloc(sizeof(frame_queue), 1);
  newFrame->nextframe = NULL;
  newFrame->pixel_buffer_format = format_type;
  newFrame->pixel_buffer_ref = CVBufferRetain(imageBuffer);
  GetFrameDisplayTimeFromDictionary(frameInfo, newFrame);

  // since the frames we get may be in decode order rather than presentation order
  // our hypothetical callback places them in a queue of frames which will
  // hold them in display order for display on another thread
  pthread_mutex_lock(&ctx->m_queue_mutex);

  frame_queue base;
  base.nextframe = ctx->m_display_queue;
  frame_queue *ptr = &base;
  for(; ptr->nextframe; ptr = ptr->nextframe)
  {
    if(ptr->nextframe->pts == DVD_NOPTS_VALUE
    || newFrame->pts       == DVD_NOPTS_VALUE)
      continue;
    if(ptr->nextframe->pts > newFrame->pts)
      break;
  }
  /* insert after ptr */
  newFrame->nextframe = ptr->nextframe;
  ptr->nextframe = newFrame;

  /* update anchor if needed */
  if(newFrame->nextframe == ctx->m_display_queue)
    ctx->m_display_queue = newFrame;

  ctx->m_queue_depth++;
  //
  pthread_mutex_unlock(&ctx->m_queue_mutex);	
}

unsigned CDVDVideoCodecVDA::GetAllowedReferences()
{
  if (m_use_cvBufferRef)
    return 3;
  else
    return 0;
}

#endif
