/*
 *      Copyright (C) 2005-2013 Team XBMC
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
#ifdef TARGET_DARWIN_OSX
#include "osx/CocoaInterface.h"
#include "DVDVideoCodec.h"
#include "DVDCodecs/DVDCodecUtils.h"
#include "utils/log.h"
#include "VDA.h"
#include "utils/BitstreamConverter.h"

extern "C" {
  #include "libavcodec/vda.h"
}

using namespace std;
using namespace VDA;


CDecoder::CDecoder()
: m_renderbuffers_count(3)
{
  m_ctx = av_vda_alloc_context();
  m_bitstream = NULL;
}

CDecoder::~CDecoder()
{
  Close();
  av_free(m_ctx);
}

bool CDecoder::Create(AVCodecContext *avctx)
{
  OSStatus status;
  CFNumberRef height;
  CFNumberRef width;
  CFNumberRef format;
  CFMutableDictionaryRef config_info;
  CFMutableDictionaryRef buffer_attributes;
  CFDictionaryRef io_surface_properties;
  CFNumberRef cv_pix_fmt;
  CFDataRef avcCData;
  int32_t fmt = 'avc1';
  int32_t pix_fmt = kCVPixelFormatType_422YpCbCr8;

  switch (avctx->codec_id)
  {
    case AV_CODEC_ID_H264:
      m_bitstream = new CBitstreamConverter;
      if (!m_bitstream->Open(avctx->codec_id, (uint8_t*)avctx->extradata, avctx->extradata_size, false))
      {
        return false;
      }
      break;

    default:
      return false;
      break;
  }

  avcCData = CFDataCreate(kCFAllocatorDefault,
                          (const uint8_t*)m_bitstream->GetExtraData(), m_bitstream->GetExtraSize());

  // check the avcC atom's sps for number of reference frames and
  // bail if interlaced, VDA does not handle interlaced h264.
  uint32_t avcc_len = CFDataGetLength(avcCData);
  if (avcc_len < 8)
  {
    // avcc atoms with length less than 8 are borked.
    CFRelease(avcCData);
    return false;
  }
  else
  {
    bool interlaced = true;
    int max_ref_frames;
    uint8_t *spc = (uint8_t*)CFDataGetBytePtr(avcCData) + 6;
    uint32_t sps_size = BS_RB16(spc);
    if (sps_size)
      m_bitstream->parseh264_sps(spc+3, sps_size-1, &interlaced, &max_ref_frames);
    if (interlaced)
    {
      CLog::Log(LOGNOTICE, "%s - possible interlaced content.", __FUNCTION__);
      CFRelease(avcCData);
      return false;
    }

    if (((uint8_t*)avctx->extradata)[4] == 0xFE)
    {
      // video content is from so silly encoder that think 3 byte NAL sizes are valid
      CLog::Log(LOGNOTICE, "%s - 3 byte nal length not supported", __FUNCTION__);
      CFRelease(avcCData);
      return false;
    }
  }


  config_info = CFDictionaryCreateMutable(kCFAllocatorDefault,
                                          4,
                                          &kCFTypeDictionaryKeyCallBacks,
                                          &kCFTypeDictionaryValueCallBacks);

  height = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &avctx->height);
  width = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &avctx->width);
  format = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &fmt);

  CFDictionarySetValue(config_info, kVDADecoderConfiguration_Height, height);
  CFDictionarySetValue(config_info, kVDADecoderConfiguration_Width, width);
  CFDictionarySetValue(config_info, kVDADecoderConfiguration_SourceFormat, format);
  CFDictionarySetValue(config_info, kVDADecoderConfiguration_avcCData, avcCData);

  buffer_attributes = CFDictionaryCreateMutable(kCFAllocatorDefault,
                                                2,
                                                &kCFTypeDictionaryKeyCallBacks,
                                                &kCFTypeDictionaryValueCallBacks);

  io_surface_properties = CFDictionaryCreate(kCFAllocatorDefault,
                                             NULL, NULL, 0,
                                             &kCFTypeDictionaryKeyCallBacks,
                                             &kCFTypeDictionaryValueCallBacks);

  cv_pix_fmt  = CFNumberCreate(kCFAllocatorDefault,
                               kCFNumberSInt32Type,
                               &pix_fmt);

  CFDictionarySetValue(buffer_attributes,
                       kCVPixelBufferPixelFormatTypeKey,
                       cv_pix_fmt);

  CFDictionarySetValue(buffer_attributes,
                       kCVPixelBufferIOSurfacePropertiesKey,
                       io_surface_properties);

  status = VDADecoderCreate(config_info,
                            buffer_attributes,
                            (VDADecoderOutputCallback*)m_ctx->output_callback,
                            avctx,
                            &m_ctx->decoder);

  CFRelease(height);
  CFRelease(width);
  CFRelease(format);
  CFRelease(avcCData);
  CFRelease(config_info);
  CFRelease(io_surface_properties);
  CFRelease(cv_pix_fmt);
  CFRelease(buffer_attributes);

  if(status != kVDADecoderNoErr)
  {
    CLog::Log(LOGERROR, "VDA::CDecoder - Failed to init VDA decoder: %d", status);
    return false;
  }
  return true;
}

void CDecoder::Close()
{
  OSStatus status = kVDADecoderNoErr;

  if (m_ctx->decoder)
    status = VDADecoderDestroy(m_ctx->decoder);
  m_ctx->decoder = NULL;

  delete m_bitstream;
  m_bitstream = NULL;
}

bool CDecoder::Open(AVCodecContext *avctx, AVCodecContext* mainctx, enum PixelFormat fmt, unsigned int surfaces)
{
  Close();

  if(fmt != AV_PIX_FMT_VDA)
    return false;

  if(avctx->codec_id != AV_CODEC_ID_H264)
    return false;

  switch(avctx->profile)
  {
    case FF_PROFILE_H264_HIGH_10:
    case FF_PROFILE_H264_HIGH_10_INTRA:
    case FF_PROFILE_H264_HIGH_422:
    case FF_PROFILE_H264_HIGH_422_INTRA:
    case FF_PROFILE_H264_HIGH_444_PREDICTIVE:
    case FF_PROFILE_H264_HIGH_444_INTRA:
    case FF_PROFILE_H264_CAVLC_444:
      return false;
    default:
      break;
  }

  if (Cocoa_GPUForDisplayIsNvidiaPureVideo3() && !CDVDCodecUtils::IsVP3CompatibleWidth(avctx->width))
  {
    CLog::Log(LOGNOTICE, "%s - Nvidia 9400 GPU hardware limitation, cannot decode a width of %d", __FUNCTION__, avctx->width);
    return false;
  }

  if (avctx->profile == FF_PROFILE_H264_MAIN && avctx->level == 32 && avctx->refs > 4)
  {
    // Main@L3.2, VDA cannot handle greater than 4 reference frames
    CLog::Log(LOGNOTICE, "%s - Main@L3.2 detected, VDA cannot decode.", __FUNCTION__);
    return false;
  }

  if (!Create(avctx))
    return false;

  avctx->pix_fmt = fmt;
  avctx->hwaccel_context = m_ctx;

  mainctx->pix_fmt = fmt;
  mainctx->hwaccel_context = m_ctx;

  return true;
}

int CDecoder::Decode(AVCodecContext* avctx, AVFrame* frame)
{
  int status = Check(avctx);
  if(status)
    return status;

  if(frame)
    return VC_BUFFER | VC_PICTURE;
  else
    return VC_BUFFER;
}

bool CDecoder::GetPicture(AVCodecContext* avctx, AVFrame* frame, DVDVideoPicture* picture)
{
  ((CDVDVideoCodecFFmpeg*)avctx->opaque)->GetPictureCommon(picture);

  picture->format = RENDER_FMT_CVBREF;
  picture->cvBufferRef = (CVPixelBufferRef)frame->data[3];
  return true;
}

int CDecoder::Check(AVCodecContext* avctx)
{
  return 0;
}

unsigned CDecoder::GetAllowedReferences()
{
  return m_renderbuffers_count;
}

#endif
