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
#include "VDA.h"

extern "C" {
  #include <libavcodec/vda.h>
}

using namespace std;
using namespace VDA;


static void RelBufferS(AVCodecContext *avctx, AVFrame *pic)
{ ((CDecoder*)((CDVDVideoCodecFFmpeg*)avctx->opaque)->GetHardware())->RelBuffer(avctx, pic); }

static int GetBufferS(AVCodecContext *avctx, AVFrame *pic) 
{  return ((CDecoder*)((CDVDVideoCodecFFmpeg*)avctx->opaque)->GetHardware())->GetBuffer(avctx, pic); }

CDecoder::CDecoder()
: m_renderbuffers_count(0)
{
  m_ctx = (vda_context*)calloc(1, sizeof(vda_context));
}

CDecoder::~CDecoder()
{
  Close();
  free(m_ctx);
}

void CDecoder::RelBuffer(AVCodecContext *avctx, AVFrame *pic)
{
  CVPixelBufferRef cv_buffer = (CVPixelBufferRef)pic->data[3];
  CVPixelBufferRelease(cv_buffer);

  for (int i = 0; i < 4; i++)
    pic->data[i] = NULL;
}

int CDecoder::GetBuffer(AVCodecContext *avctx, AVFrame *pic)
{
  pic->type = FF_BUFFER_TYPE_USER;
  pic->data[0] = (uint8_t *)1;
  pic->reordered_opaque = avctx->reordered_opaque;
  return 0;
}

static void vda_decoder_callback (void *vda_hw_ctx,
                                  CFDictionaryRef user_info,
                                  OSStatus status,
                                  uint32_t infoFlags,
                                  CVImageBufferRef image_buffer)
{
  struct vda_context *vda_ctx = (struct vda_context *)vda_hw_ctx;

  if (!image_buffer)
    return;

  if (vda_ctx->cv_pix_fmt_type != CVPixelBufferGetPixelFormatType(image_buffer))
    return;

  vda_ctx->cv_buffer = CVPixelBufferRetain(image_buffer);
}

bool CDecoder::Create(AVCodecContext *avctx)
{
  OSStatus status;
  CFNumberRef height;
  CFNumberRef width;
  CFNumberRef format;
  CFDataRef avc_data;
  CFMutableDictionaryRef config_info;
  CFMutableDictionaryRef buffer_attributes;
  CFMutableDictionaryRef io_surface_properties;
  CFNumberRef cv_pix_fmt;

  m_ctx->priv_bitstream = NULL;
  m_ctx->priv_allocated_size = 0;

  /* Each VCL NAL in the bitstream sent to the decoder
   * is preceded by a 4 bytes length header.
   * Change the avcC atom header if needed, to signal headers of 4 bytes. */
  if (avctx->extradata_size >= 4 && (avctx->extradata[4] & 0x03) != 0x03) {
    uint8_t *rw_extradata;

    if (!(rw_extradata = (uint8_t*)av_malloc(avctx->extradata_size)))
      return false;

    memcpy(rw_extradata, avctx->extradata, avctx->extradata_size);

    rw_extradata[4] |= 0x03;

    avc_data = CFDataCreate(kCFAllocatorDefault, rw_extradata, avctx->extradata_size);

    av_freep(&rw_extradata);
  } else {
    avc_data = CFDataCreate(kCFAllocatorDefault, avctx->extradata, avctx->extradata_size);
  }

  config_info = CFDictionaryCreateMutable(kCFAllocatorDefault,
                                          4,
                                          &kCFTypeDictionaryKeyCallBacks,
                                          &kCFTypeDictionaryValueCallBacks);

  height   = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &m_ctx->height);
  width    = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &m_ctx->width);
  format   = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &m_ctx->format);

  CFDictionarySetValue(config_info, kVDADecoderConfiguration_Height      , height);
  CFDictionarySetValue(config_info, kVDADecoderConfiguration_Width       , width);
  CFDictionarySetValue(config_info, kVDADecoderConfiguration_SourceFormat, format);
  CFDictionarySetValue(config_info, kVDADecoderConfiguration_avcCData    , avc_data);

  buffer_attributes = CFDictionaryCreateMutable(kCFAllocatorDefault,
                                                2,
                                                &kCFTypeDictionaryKeyCallBacks,
                                                &kCFTypeDictionaryValueCallBacks);
  io_surface_properties = CFDictionaryCreateMutable(kCFAllocatorDefault,
                                                    0,
                                                    &kCFTypeDictionaryKeyCallBacks,
                                                    &kCFTypeDictionaryValueCallBacks);
  cv_pix_fmt  = CFNumberCreate(kCFAllocatorDefault,
                               kCFNumberSInt32Type,
                               &m_ctx->cv_pix_fmt_type);
  CFDictionarySetValue(buffer_attributes,
                       kCVPixelBufferPixelFormatTypeKey,
                       cv_pix_fmt);
  CFDictionarySetValue(buffer_attributes,
                       kCVPixelBufferIOSurfacePropertiesKey,
                       io_surface_properties);

  status = VDADecoderCreate(config_info,
                            buffer_attributes,
                            (VDADecoderOutputCallback*)vda_decoder_callback,
                            m_ctx,
                            &m_ctx->decoder);

  CFRelease(height);
  CFRelease(width);
  CFRelease(format);
  CFRelease(avc_data);
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
  av_freep(&m_ctx->priv_bitstream);
}

bool CDecoder::Open(AVCodecContext *avctx, enum PixelFormat fmt, unsigned int surfaces)
{
  Close();

  if(fmt != AV_PIX_FMT_VDA_VLD)
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

  /* init vda */
  memset(m_ctx, 0, sizeof(struct vda_context));
  m_ctx->width  = avctx->width;
  m_ctx->height = avctx->height;
  m_ctx->format = 'avc1';
  m_ctx->use_sync_decoding = 1;
  m_ctx->cv_pix_fmt_type = kCVPixelFormatType_422YpCbCr8;

  if (!Create(avctx))
    return false;

  avctx->pix_fmt         = fmt;
  avctx->hwaccel_context = m_ctx;
  avctx->get_buffer      = GetBufferS;
  avctx->release_buffer  = RelBufferS;

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

  picture->format      = RENDER_FMT_CVBREF;
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
