/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
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

#include "DVDVideoCodecMfc.h"

#ifdef CLASSNAME
#undef CLASSNAME
#endif
#define CLASSNAME "CDVDVideoCodecMfc"

CDVDVideoCodecMfc::CDVDVideoCodecMfc() : CDVDVideoCodec() {

  decoder = new MfcDecoder();
  converter = NULL;

  m_iDecodedWidth = 0;
  m_iDecodedHeight = 0;
  m_iConvertedWidth = 0;
  m_iConvertedHeight = 0;

  m_bDropPictures = false;  
  memzero(m_videoBuffer);
}

CDVDVideoCodecMfc::~CDVDVideoCodecMfc() {
  Dispose();
  if (converter)
    delete converter;
  if (decoder)
    delete decoder;
}

bool CDVDVideoCodecMfc::Open(CDVDStreamInfo &hints, CDVDCodecOptions &options) {

  if (hints.software) {
    return false;
  }

  Dispose();

  if (!decoder->OpenDevice()) {
    CLog::Log(LOGDEBUG, "%s::%s - MFC device not found", CLASSNAME, __func__);
    return false;
  }

  if (!decoder->HasNV12Support()) {
    MFC5 = true;
    MFC6 = false;
    // MFC5 requires FIMC convertor
    if (!converter->OpenDevice()) {
      CLog::Log(LOGDEBUG, "%s::%s - FIMC device not found", CLASSNAME, __func__);
      return false;
    }
  } else {
    MFC5 = false;
    MFC6 = true;
  }

  if (!decoder->SetupOutputFormat(hints)) {
    return false;
  }
  
  if (!decoder->SetupOutputBuffers()) {
    return false;
  }

  if (!decoder->QueueHeader(hints)) {
    return false;
  }

if (MFC6) {
  if (!decoder->SetCaptureFormat()) {
    return false;
  }
}

  struct v4l2_format mfc_fmt = {};
  if (!decoder->GetCaptureFormat(&mfc_fmt)) {
    return false;
  }

if (MFC5) {
  // the output format of FIMC is the input format of MFC
  if (!converter->SetOutputFormat(&mfc_fmt)) {
    return false;
  }  
}

  if (!decoder->RequestBuffers()) {
    return false;
  }

  struct v4l2_crop mfc_crop = {};
  if (!decoder->GetCaptureCrop(&mfc_crop)) {
    return false;
  }

if (MFC5) {
  m_iDecodedWidth = mfc_crop.c.width;
  m_iDecodedHeight = mfc_crop.c.height;
}
if (MFC6) {
  m_iDecodedWidth = (mfc_crop.c.width + 15)&~15; // Align width by 16. Required for NV12 to YUV420 converter
  m_iDecodedHeight = mfc_crop.c.height;

  // FIXME: add this to mfc decoder MFC6
  decoder->SetOutputBufferPlanes(m_iDecodedWidth, m_iDecodedHeight);
  //m_v4l2OutputBuffer.cPlane[0] = new BYTE[m_iVideoWidth * m_iVideoHeight];
  //m_v4l2OutputBuffer.cPlane[1] = new BYTE[m_iVideoWidth * (m_iVideoHeight >> 1)];
}

  // With no FIMC scaling, resulting picture will be the same size as source
  int width = m_iDecodedWidth;
  int height = m_iDecodedHeight;

if (MFC5) {
  // Set FIMC crop based on MFC crop 
  if (!converter->SetOutputCrop(&mfc_crop)) {
    return false;
  }

  // Calculate FIMC final picture size as scaled to fit screen
  RESOLUTION_INFO res_info = CDisplaySettings::Get().GetResolutionInfo(g_graphicsContext.GetVideoResolution());
  double ratio = std::min((double)res_info.iScreenWidth / (double)m_iDecodedWidth, (double)res_info.iScreenHeight / (double)m_iDecodedHeight);
  width = (int)((double)m_iDecodedWidth * ratio);
  height = (int)((double)m_iDecodedHeight * ratio);
  if (width%2) width--;
  if (height%2) height--;
}
  
  if (!decoder->SetupCaptureBuffers()) {
    return false;
  }
  
  // with no FIMC we use for converted picture the decoded width and height
  m_iConvertedWidth = width;
  m_iConvertedHeight = height;
if (MFC5) {
  // with FIMC we override the converted picture width and height
  if (!converter->RequestBuffers(decoder->GetCaptureBuffersCount())) {
    return false;
  }

  struct v4l2_format fimc_fmt = {};
  fimc_fmt.fmt.pix_mp.width = width;
  fimc_fmt.fmt.pix_mp.height = height;
  if (!converter->SetCaptureFormat(&fimc_fmt)) {
    return false;
  }
  m_iConvertedWidth = fimc_fmt.fmt.pix_mp.width;
  m_iConvertedHeight = fimc_fmt.fmt.pix_mp.height;

  struct v4l2_crop fimc_crop = {};
  fimc_crop.c.width = m_iConvertedWidth;
  fimc_crop.c.height = m_iConvertedHeight;
  if (!converter->SetCaptureCrop(&fimc_crop)) {
    return false;
  }

  if (!converter->SetupCaptureBuffers()) {
    return false;
  }

  // we dequeue header only on MFC5 (when FIMC is present)
  // on MFC we just start streaming...
  if (!decoder->DequeueHeader()) {
    return false;
  }
}

  return true;
}


void CDVDVideoCodecMfc::Dispose() {
  while(!m_pts.empty())
    m_pts.pop();
  while(!m_dts.empty())
    m_dts.pop();

  m_bDropPictures = false;
  memzero(m_videoBuffer);

  if (converter)
    converter->Dispose();
  if (decoder)
    decoder->Dispose();
}

void CDVDVideoCodecMfc::SetDropState(bool bDrop) {

  m_bDropPictures = bDrop;

}

int CDVDVideoCodecMfc::Decode(BYTE* pData, int iSize, double dts, double pts) {

  int ret = -1;
  size_t index = 0;

  if(pData) {
    // Find buffer ready to be filled
    for (index = 0; index < decoder->GetOutputBuffersCount(); index++) {
      if (decoder->IsOutputBufferEmpty(index))
        break;
    }

    if (index == decoder->GetOutputBuffersCount()) { // all input buffers are busy, dequeue needed
      if (!decoder->DequeueOutputBuffer(&ret)) {
        return ret;
      }
      index = ret;
    }

    if (!decoder->SendBuffer(index, pData, iSize)) {
      return VC_ERROR;
    }
    m_pts.push(-pts);
    m_dts.push(-dts);
  }

if (MFC6) {
  int buf = 0;
  if (decoder->GetFirstDecodedCaptureBuffer(&buf)) {
    if (!decoder->QueueOutputBuffer(buf)) {
      m_videoBuffer.iFlags      |= DVP_FLAG_DROPPED;
      m_videoBuffer.iFlags      &= DVP_FLAG_ALLOCATED;
      return VC_ERROR;
    }
    decoder->RemoveFirstDecodedCaptureBuffer();
  }
}

  if (!decoder->DequeueDecodedFrame(&ret)) {
    return ret;
  }
  index = ret;

if (MFC6) { 
  decoder->AddDecodedCaptureBuffer(index);
}

  if (m_bDropPictures) {
    m_videoBuffer.iFlags |= DVP_FLAG_DROPPED;
    CLog::Log(LOGDEBUG, "%s::%s - Dropping frame with index %d", CLASSNAME, __func__, ret);
  } else {
if (MFC5) {
    if (!converter->QueueCaptureBuffer()) {
      return VC_ERROR;
    }
  
    if (!converter->QueueOutputBuffer(index, decoder->GetCaptureBuffer(index), &ret)) {
      return VC_ERROR;
    }
 
    decoder->SetCaptureBufferBusy(ret);

    if (converter->StartConverter()) {
      return VC_BUFFER; // Queue one more frame for double buffering on FIMC
    }

    if (!converter->DequeueOutputBuffer(&ret)) {
      return VC_ERROR;
    }
    index = ret;

    decoder->SetCaptureBufferEmpty(ret);
    
    if (!converter->DequeueCaptureBuffer(&ret)) {
      if (ret != 0)
        return VC_BUFFER;
      else
        return VC_ERROR;
    }
}

    m_videoBuffer.iFlags          = DVP_FLAG_ALLOCATED;
    m_videoBuffer.color_range     = 0;
    m_videoBuffer.color_matrix    = 4;

if (MFC5) {
    m_videoBuffer.iDisplayWidth   = m_iConvertedWidth;
    m_videoBuffer.iDisplayHeight  = m_iConvertedHeight;
    m_videoBuffer.iWidth          = m_iConvertedWidth;
    m_videoBuffer.iHeight         = m_iConvertedHeight;

    m_videoBuffer.data[0]         = 0;
    m_videoBuffer.data[1]         = 0;
    m_videoBuffer.data[2]         = 0;
    m_videoBuffer.data[3]         = 0;
    
    m_videoBuffer.format          = RENDER_FMT_YUV420P;
    m_videoBuffer.iLineSize[0]    = m_iConvertedWidth;
    m_videoBuffer.iLineSize[1]    = m_iConvertedWidth >> 1;
    m_videoBuffer.iLineSize[2]    = m_iConvertedWidth >> 1;
    m_videoBuffer.iLineSize[3]    = 0;
    m_videoBuffer.data[0]         = (BYTE*) converter->GetCurrentCaptureBuffer()->cPlane[0];
    m_videoBuffer.data[1]         = (BYTE*) converter->GetCurrentCaptureBuffer()->cPlane[1];
    m_videoBuffer.data[2]         = (BYTE*) converter->GetCurrentCaptureBuffer()->cPlane[2];
}
if (MFC6) {
    m_videoBuffer.format          = RENDER_FMT_NV12;
    m_videoBuffer.iDisplayWidth   = m_iDecodedWidth;
    m_videoBuffer.iDisplayHeight  = m_iDecodedHeight;
    m_videoBuffer.iWidth          = m_iDecodedWidth;
    m_videoBuffer.iHeight         = m_iDecodedHeight;

    m_videoBuffer.iLineSize[0]    = m_iDecodedWidth;
    m_videoBuffer.iLineSize[1]    = m_iDecodedWidth;
    m_videoBuffer.iLineSize[2]    = 0;
    m_videoBuffer.iLineSize[3]    = 0;
    m_videoBuffer.data[0]         = (BYTE*) decoder->GetCaptureBuffer(index)->cPlane[0];
    m_videoBuffer.data[1]         = (BYTE*) decoder->GetCaptureBuffer(index)->cPlane[1];
}
  }

  // Pop pts/dts only when picture is finally ready to be showed up or skipped
  if(m_pts.size()) {
    m_videoBuffer.pts = -m_pts.top(); // MFC always return frames in order and assigning them their pts'es from the input
                                      // will lead to reshuffle. This will assign least pts in the queue to the frame dequeued.
    m_videoBuffer.dts = -m_dts.top();
    
    m_pts.pop();
    m_dts.pop();
  } else {
    CLog::Log(LOGERROR, "%s::%s - no pts value", CLASSNAME, __func__);
    m_videoBuffer.pts           = DVD_NOPTS_VALUE;
    m_videoBuffer.dts           = DVD_NOPTS_VALUE;
  }

if (MFC5) {
  // Queue dequeued from FIMC OUPUT frame back to MFC CAPTURE
  if (!decoder->QueueOutputBuffer(index)) {
    m_videoBuffer.iFlags      |= DVP_FLAG_DROPPED;
    m_videoBuffer.iFlags      &= DVP_FLAG_ALLOCATED;
    return VC_ERROR;
  }
}
  return VC_PICTURE; // Picture is finally ready to be processed further

}

void CDVDVideoCodecMfc::Reset() {
}

bool CDVDVideoCodecMfc::GetPicture(DVDVideoPicture* pDvdVideoPicture) {

  *pDvdVideoPicture = m_videoBuffer;
  return true;
}

bool CDVDVideoCodecMfc::ClearPicture(DVDVideoPicture* pDvdVideoPicture)
{
  return CDVDVideoCodec::ClearPicture(pDvdVideoPicture);
}

