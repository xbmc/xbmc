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
  m_decoder   = new MfcDecoder();
  m_converter = NULL;

  m_bDropPictures                  = false;
  m_iDequeuedToPresentBufferNumber = -1;
  memset(&(m_videoBuffer), 0, sizeof (m_videoBuffer));
}

CDVDVideoCodecMfc::~CDVDVideoCodecMfc()
{
  Dispose();
  delete m_converter;
  delete m_decoder;
}

void CDVDVideoCodecMfc::Dispose()
{
  m_iDequeuedToPresentBufferNumber = -1;
  m_bDropPictures                  = false;
  memset(&(m_videoBuffer), 0, sizeof (m_videoBuffer));

  if (m_converter != NULL)
    m_converter->Dispose();
  if (m_decoder != NULL)
    m_decoder->Dispose();
}

bool CDVDVideoCodecMfc::Open(CDVDStreamInfo &hints, CDVDCodecOptions &options)
{
  int iDecodedWidth;
  int iDecodedHeight;

  m_hints = hints;

  if (m_hints.software)
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

  if (!m_decoder->SetupOutputFormat(m_hints))
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

  iDecodedWidth  = mfc_crop.c.width;
  iDecodedHeight = mfc_crop.c.height;

  if (!m_NV12Support)
  {
    // Set FIMC crop based on MFC crop
    if (!m_converter->SetOutputCrop(&mfc_crop))
    {
      return false;
    }
    // Calculate FIMC final picture size as scaled to fit screen
    RESOLUTION_INFO res_info = CDisplaySettings::Get().GetResolutionInfo(g_graphicsContext.GetVideoResolution());
    double ratio  = std::min((double)res_info.iScreenWidth / (double)iDecodedWidth, (double)res_info.iScreenHeight / (double)iDecodedHeight);
    iDecodedWidth         = (int)((double)iDecodedWidth * ratio);
    iDecodedHeight        = (int)((double)iDecodedHeight * ratio);
    if (iDecodedWidth%2)  iDecodedWidth--;
    if (iDecodedHeight%2) iDecodedHeight--;
  }

  if (!m_decoder->SetupCaptureBuffers())
  {
    return false;
  }

  if (!m_NV12Support)
  {
    if (!m_converter->RequestBuffers(m_decoder->GetCaptureBuffersCount()))
    {
      return false;
    }

    struct v4l2_format fimc_fmt = {};
    fimc_fmt.fmt.pix_mp.width   = iDecodedWidth;
    fimc_fmt.fmt.pix_mp.height  = iDecodedHeight;
    if (!m_converter->SetCaptureFormat(&fimc_fmt))
    {
      return false;
    }

    struct v4l2_crop fimc_crop  = {};
    fimc_crop.c.width           = iDecodedWidth;
    fimc_crop.c.height          = iDecodedHeight;
    if (!m_converter->SetCaptureCrop(&fimc_crop))
    {
      return false;
    }

    if (!m_converter->SetupCaptureBuffers())
    {
      return false;
    }
  }

  m_videoBuffer.iFlags          = DVP_FLAG_ALLOCATED;

  m_videoBuffer.color_range     = 0;
  m_videoBuffer.color_matrix    = 4;

  m_videoBuffer.iDisplayWidth   = iDecodedWidth;
  m_videoBuffer.iDisplayHeight  = iDecodedHeight;
  m_videoBuffer.iWidth          = iDecodedWidth;
  m_videoBuffer.iHeight         = iDecodedHeight;

  m_videoBuffer.data[0]         = NULL;
  m_videoBuffer.data[1]         = NULL;
  m_videoBuffer.data[2]         = NULL;
  m_videoBuffer.data[3]         = NULL;

  m_videoBuffer.format          = RENDER_FMT_NV12;
  m_videoBuffer.iLineSize[0]    = iDecodedWidth;
  m_videoBuffer.iLineSize[1]    = iDecodedWidth;
  m_videoBuffer.iLineSize[2]    = 0;
  m_videoBuffer.iLineSize[3]    = 0;
  m_videoBuffer.pts             = DVD_NOPTS_VALUE;
  m_videoBuffer.dts             = DVD_NOPTS_VALUE;

  return true;
}

int CDVDVideoCodecMfc::Decode(BYTE* pData, int iSize, double dts, double pts)
{
  int ret       = -1;
  size_t index  = 0;
  double dequeuedTimestamp;

  if (m_hints.ptsinvalid)
    pts = DVD_NOPTS_VALUE;

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
      if (!m_decoder->DequeueOutputBuffer(&ret, &dequeuedTimestamp))
      {
        return ret;
      }
      index = ret;
    }

    if (!m_decoder->SendBuffer(index, pData, iSize, pts))
    {
      return VC_FLUSHED;
    }
  }

  if (m_iDequeuedToPresentBufferNumber >= 0)
  {
    if (!m_NV12Support)
    {
      if (!m_converter->GetCaptureBuffer(m_iDequeuedToPresentBufferNumber)->bQueue)
      {
        if (!m_converter->QueueCaptureBuffer(m_iDequeuedToPresentBufferNumber))
          return VC_FLUSHED;
        m_iDequeuedToPresentBufferNumber = -1;
      }
    }
    else
    {
      if (!m_decoder->GetCaptureBuffer(m_iDequeuedToPresentBufferNumber)->bQueue)
      {
        if (!m_decoder->QueueCaptureBuffer(m_iDequeuedToPresentBufferNumber))
          return VC_FLUSHED;
        m_iDequeuedToPresentBufferNumber = -1;
      }
    }
  }

  if (!m_decoder->DequeueDecodedFrame(&ret, &dequeuedTimestamp))
  {
    return ret;
  }
  index = ret;

  if (m_bDropPictures)
  {
    CLog::Log(LOGDEBUG, "%s::%s - Dropping frame with index %d", CLASSNAME, __func__, ret);
    // Queue it back to MFC CAPTURE since the picture is dropped anyway
    if (!m_decoder->QueueCaptureBuffer(index))
    {
      return VC_FLUSHED;
    }
    return VC_BUFFER; // Continue, we have no picture to show
  }
  else
  {
    if (!m_NV12Support)
    {
      if (!m_converter->QueueOutputBuffer(index, m_decoder->GetCaptureBuffer(index), &ret))
        return VC_FLUSHED;

      if (!m_converter->DequeueCaptureBuffer(&ret, &dequeuedTimestamp))
      {
        if (ret != 0)
          return VC_BUFFER;
        else
          return VC_FLUSHED;
      }
      index = ret;

      if (!m_converter->DequeueOutputBuffer(&ret, &dequeuedTimestamp))
      {
        if (ret != 0)
          return VC_BUFFER;
        else
          return VC_FLUSHED;
      }
      // Queue it back to MFC CAPTURE
      if (!m_decoder->QueueCaptureBuffer(ret))
      {
        CLog::Log(LOGERROR, "%s::%s - MFC queue CAPTURE buffer", CLASSNAME, __func__);
        return VC_FLUSHED;
      }

      m_converter->GetCaptureBuffer(index)->bQueue = false;
      m_converter->GetCaptureBuffer(index)->timestamp = dequeuedTimestamp;
      m_videoBuffer.data[0] = (BYTE*) m_converter->GetCaptureBuffer(index)->cPlane[0];
      m_videoBuffer.data[1] = (BYTE*) m_converter->GetCaptureBuffer(index)->cPlane[1];
      m_videoBuffer.pts     = dequeuedTimestamp;
    }
    else
    {
      m_videoBuffer.data[0] = (BYTE*) m_decoder->GetCaptureBuffer(index)->cPlane[0];
      m_videoBuffer.data[1] = (BYTE*) m_decoder->GetCaptureBuffer(index)->cPlane[1];
      m_videoBuffer.pts     = m_decoder->GetCaptureBuffer(index)->timestamp;
    }

    m_iDequeuedToPresentBufferNumber = index;
  }

  return VC_PICTURE | VC_BUFFER; // Picture is finally ready to be processed further
}

void CDVDVideoCodecMfc::Reset()
{
  CDVDCodecOptions options;
  Open(m_hints, options);
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

void CDVDVideoCodecMfc::SetDropState(bool bDrop)
{
  m_bDropPictures = bDrop;
  if (m_bDropPictures)
    m_videoBuffer.iFlags |=  DVP_FLAG_DROPPED;
  else
    m_videoBuffer.iFlags &= ~DVP_FLAG_DROPPED;
}
