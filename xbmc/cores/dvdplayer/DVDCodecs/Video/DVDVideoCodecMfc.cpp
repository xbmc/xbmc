/*
 *      Copyright (C) 2005-2014 Team XBMC
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

CDVDVideoCodecMfc::CDVDVideoCodecMfc() : CDVDVideoCodec()
{
  m_decoder = new MfcDecoder();
  m_converter = NULL;

  m_iDecodedWidth     = 0;
  m_iDecodedHeight    = 0;
  m_iConvertedWidth   = 0;
  m_iConvertedHeight  = 0;

  m_bDropPictures = false;
  memset(&(m_videoBuffer), 0, sizeof (m_videoBuffer));
}

CDVDVideoCodecMfc::~CDVDVideoCodecMfc()
{
  Dispose();
  delete m_converter;
  delete m_decoder;
}

bool CDVDVideoCodecMfc::Open(CDVDStreamInfo &hints, CDVDCodecOptions &options)
{
  if (hints.software)
  {
    return false;
  }
  Dispose();

  if (!m_decoder->OpenDevice())
  {
    CLog::Log(LOGDEBUG, "%s::%s - MFC device not found", CLASSNAME, __func__);
    return false;
  }

  if (!m_decoder->HasNV12Support())
  {
    m_NV12Support = false;
    // FIMC color convertor required
    m_converter = new FimcConverter();
    if (!m_converter->OpenDevice())
    {
      CLog::Log(LOGDEBUG, "%s::%s - FIMC device not found", CLASSNAME, __func__);
      return false;
    }
  }
  else
  {
    m_NV12Support = true;
  }

  if (!m_decoder->SetupOutputFormat(hints))
  {
    return false;
  }

  if (!m_decoder->SetupOutputBuffers())
  {
    return false;
  }

  if (!m_decoder->QueueHeader(hints))
  {
    return false;
  }

  if (m_NV12Support)
  {
    if (!m_decoder->SetCaptureFormat())
    {
      return false;
    }
  }

  struct v4l2_format mfc_fmt = {};
  if (!m_decoder->GetCaptureFormat(&mfc_fmt))
  {
    return false;
  }

  if (!m_NV12Support)
  {
    // the output format of FIMC is the input format of MFC
    if (!m_converter->SetOutputFormat(&mfc_fmt))
    {
      return false;
    }
  }

  if (!m_decoder->RequestBuffers())
  {
    return false;
  }

  struct v4l2_crop mfc_crop = {};
  if (!m_decoder->GetCaptureCrop(&mfc_crop))
  {
    return false;
  }

  if (!m_NV12Support) {
    m_iDecodedWidth = mfc_crop.c.width;
    m_iDecodedHeight = mfc_crop.c.height;
  }
  else
  {
    m_iDecodedWidth = (mfc_crop.c.width + 15)&~15;  // Align width by 16. Required for NV12 to YUV420 converter
    m_iDecodedHeight = mfc_crop.c.height;
    // FIXME: maybe we can do this nicer...
    m_decoder->SetOutputBufferPlanes(m_iDecodedWidth, m_iDecodedHeight);
  }

  // With no FIMC scaling, resulting picture will be the same size as source
  int width       = m_iDecodedWidth;
  int height      = m_iDecodedHeight;

  if (!m_NV12Support)
  {
    // Set FIMC crop based on MFC crop
    if (!m_converter->SetOutputCrop(&mfc_crop))
    {
      return false;
    }
    // Calculate FIMC final picture size as scaled to fit screen
    RESOLUTION_INFO res_info = CDisplaySettings::Get().GetResolutionInfo(g_graphicsContext.GetVideoResolution());
    double ratio  = std::min((double)res_info.iScreenWidth / (double)m_iDecodedWidth, (double)res_info.iScreenHeight / (double)m_iDecodedHeight);
    width         = (int)((double)m_iDecodedWidth * ratio);
    height        = (int)((double)m_iDecodedHeight * ratio);
    if (width%2)  width--;
    if (height%2) height--;
  }

  if (!m_decoder->SetupCaptureBuffers())
  {
    return false;
  }

  // with no FIMC we use for converted picture the decoded width and height
  m_iConvertedWidth = width;
  m_iConvertedHeight = height;
  if (!m_NV12Support)
  {
    // with FIMC we override the converted picture width and height
    if (!m_converter->RequestBuffers(m_decoder->GetCaptureBuffersCount()))
    {
      return false;
    }

    struct v4l2_format fimc_fmt = {};
    fimc_fmt.fmt.pix_mp.width   = width;
    fimc_fmt.fmt.pix_mp.height  = height;
    if (!m_converter->SetCaptureFormat(&fimc_fmt))
    {
      return false;
    }
    m_iConvertedWidth   = fimc_fmt.fmt.pix_mp.width;
    m_iConvertedHeight  = fimc_fmt.fmt.pix_mp.height;

    struct v4l2_crop fimc_crop  = {};
    fimc_crop.c.width           = m_iConvertedWidth;
    fimc_crop.c.height          = m_iConvertedHeight;
    if (!m_converter->SetCaptureCrop(&fimc_crop))
    {
      return false;
    }

    if (!m_converter->SetupCaptureBuffers())
    {
      return false;
    }

    // we dequeue header only on MFC5 (when FIMC is present)
    // on MFC we just start streaming...
    if (!m_decoder->DequeueHeader())
    {
      return false;
    }
  }

  return true;
}


void CDVDVideoCodecMfc::Dispose()
{
  while (!m_pts.empty())
    m_pts.pop();
  while (!m_dts.empty())
    m_dts.pop();

  m_bDropPictures = false;
  memset(&(m_videoBuffer), 0, sizeof (m_videoBuffer));

  if (m_converter != NULL)
    m_converter->Dispose();
  if (m_decoder != NULL)
    m_decoder->Dispose();
}

void CDVDVideoCodecMfc::SetDropState(bool bDrop)
{
  m_bDropPictures = bDrop;
}

int CDVDVideoCodecMfc::Decode(BYTE* pData, int iSize, double dts, double pts)
{
  int ret       = -1;
  size_t index  = 0;

  if (pData)
  {
    // Find buffer ready to be filled
    for (index = 0; index < m_decoder->GetOutputBuffersCount(); index++)
    {
      if (m_decoder->IsOutputBufferEmpty(index))
        break;
    }

    if (index == m_decoder->GetOutputBuffersCount())
    {
      // all input buffers are busy, dequeue needed
      if (!m_decoder->DequeueOutputBuffer(&ret))
      {
        return ret;
      }
      index = ret;
    }

    if (!m_decoder->SendBuffer(index, pData, iSize))
    {
      return VC_ERROR;
    }
    m_pts.push(-pts);
    m_dts.push(-dts);
  }

  if (m_NV12Support)
  {
    int buf = 0;
    if (m_decoder->GetFirstDecodedCaptureBuffer(&buf))
    {
      if (!m_decoder->QueueOutputBuffer(buf))
      {
        m_videoBuffer.iFlags      |= DVP_FLAG_DROPPED;
        m_videoBuffer.iFlags      &= DVP_FLAG_ALLOCATED;
        return VC_ERROR;
      }
      m_decoder->RemoveFirstDecodedCaptureBuffer();
    }
  }

  if (!m_decoder->DequeueDecodedFrame(&ret))
  {
    return ret;
  }
  index = ret;

  if (m_NV12Support)
  {
    m_decoder->AddDecodedCaptureBuffer(index);
  }

  if (m_bDropPictures)
  {
    m_videoBuffer.iFlags |= DVP_FLAG_DROPPED;
    CLog::Log(LOGDEBUG, "%s::%s - Dropping frame with index %d", CLASSNAME, __func__, ret);
  }
  else
  {
    if (!m_NV12Support)
    {
      if (!m_converter->QueueCaptureBuffer())
      {
        return VC_ERROR;
      }

      if (!m_converter->QueueOutputBuffer(index, m_decoder->GetCaptureBuffer(index), &ret))
      {
        return VC_ERROR;
      }

      m_decoder->SetCaptureBufferBusy(ret);

      if (m_converter->StartConverter())
      {
        return VC_BUFFER; // Queue one more frame for double buffering on FIMC
      }

      if (!m_converter->DequeueOutputBuffer(&ret))
      {
        return VC_ERROR;
      }
      index = ret;

      m_decoder->SetCaptureBufferEmpty(ret);

      if (!m_converter->DequeueCaptureBuffer(&ret))
      {
        if (ret != 0)
          return VC_BUFFER;
        else
          return VC_ERROR;
      }
    }

    m_videoBuffer.iFlags          = DVP_FLAG_ALLOCATED;
    m_videoBuffer.color_range     = 0;
    m_videoBuffer.color_matrix    = 4;

    if (!m_NV12Support) {
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
      m_videoBuffer.data[0]         = (BYTE*) m_converter->GetCurrentCaptureBuffer()->cPlane[0];
      m_videoBuffer.data[1]         = (BYTE*) m_converter->GetCurrentCaptureBuffer()->cPlane[1];
      m_videoBuffer.data[2]         = (BYTE*) m_converter->GetCurrentCaptureBuffer()->cPlane[2];
    }
    else
    {
      m_videoBuffer.format          = RENDER_FMT_NV12;
      m_videoBuffer.iDisplayWidth   = m_iDecodedWidth;
      m_videoBuffer.iDisplayHeight  = m_iDecodedHeight;
      m_videoBuffer.iWidth          = m_iDecodedWidth;
      m_videoBuffer.iHeight         = m_iDecodedHeight;

      m_videoBuffer.iLineSize[0]    = m_iDecodedWidth;
      m_videoBuffer.iLineSize[1]    = m_iDecodedWidth;
      m_videoBuffer.iLineSize[2]    = 0;
      m_videoBuffer.iLineSize[3]    = 0;
      m_videoBuffer.data[0]         = (BYTE*) m_decoder->GetCaptureBuffer(index)->cPlane[0];
      m_videoBuffer.data[1]         = (BYTE*) m_decoder->GetCaptureBuffer(index)->cPlane[1];
    }
  }

  // Pop pts/dts only when picture is finally ready to be showed up or skipped
  if (m_pts.size())
  {
    m_videoBuffer.pts = -m_pts.top(); // MFC always return frames in order and assigning them their pts'es from the input
                                      // will lead to reshuffle. This will assign least pts in the queue to the frame dequeued.
    m_videoBuffer.dts = -m_dts.top();

    m_pts.pop();
    m_dts.pop();
  }
  else
  {
    CLog::Log(LOGERROR, "%s::%s - no pts value", CLASSNAME, __func__);
    m_videoBuffer.pts           = DVD_NOPTS_VALUE;
    m_videoBuffer.dts           = DVD_NOPTS_VALUE;
  }

  if (!m_NV12Support)
  {
    // Queue dequeued from FIMC OUPUT frame back to MFC CAPTURE
    if (!m_decoder->QueueOutputBuffer(index))
    {
      m_videoBuffer.iFlags      |= DVP_FLAG_DROPPED;
      m_videoBuffer.iFlags      &= DVP_FLAG_ALLOCATED;
      return VC_ERROR;
    }
  }

  return VC_PICTURE; // Picture is finally ready to be processed further
}

void CDVDVideoCodecMfc::Reset()
{
  // FIXME: implement Reset - currently skipping back in a stream results in a small stuttering
}

bool CDVDVideoCodecMfc::GetPicture(DVDVideoPicture* pDvdVideoPicture)
{
  *pDvdVideoPicture = m_videoBuffer;
  return true;
}

bool CDVDVideoCodecMfc::ClearPicture(DVDVideoPicture* pDvdVideoPicture)
{
  return CDVDVideoCodec::ClearPicture(pDvdVideoPicture);
}
