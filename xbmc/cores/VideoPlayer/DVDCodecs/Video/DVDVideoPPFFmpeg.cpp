/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://kodi.tv
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

#include "DVDVideoPPFFmpeg.h"
#include "utils/log.h"
#include "cores/FFmpeg.h"
#include "cores/VideoPlayer/Process/ProcessInfo.h"

extern "C" {
#include "libavutil/mem.h"
}

CDVDVideoPPFFmpeg::CDVDVideoPPFFmpeg(CProcessInfo &processInfo):
  m_sType(""), m_processInfo(processInfo)
{
  m_pMode = m_pContext = NULL;
  m_iInitWidth = m_iInitHeight = 0;
  m_deinterlace = false;
}

CDVDVideoPPFFmpeg::~CDVDVideoPPFFmpeg()
{
  Dispose();
}

void CDVDVideoPPFFmpeg::Dispose()
{
  if (m_pMode)
  {
    pp_free_mode(m_pMode);
    m_pMode = NULL;
  }
  if(m_pContext)
  {
    pp_free_context(m_pContext);
    m_pContext = NULL;
  }

  m_iInitWidth = 0;
  m_iInitHeight = 0;
}

bool CDVDVideoPPFFmpeg::CheckInit(int iWidth, int iHeight)
{
  if (m_iInitWidth != iWidth || m_iInitHeight != iHeight)
  {
    if (m_pContext || m_pMode)
    {
      Dispose();
    }

    m_pContext = pp_get_context(iWidth, iHeight, PPCPUFlags() | PP_FORMAT_420);

    m_iInitWidth = iWidth;
    m_iInitHeight = iHeight;

    m_pMode = pp_get_mode_by_name_and_quality((char *)m_sType.c_str(), PP_QUALITY_MAX);
  }

  if (m_pMode)
    return true;
  else
    return false;
}

void CDVDVideoPPFFmpeg::SetType(const std::string& mType, bool deinterlace)
{
  m_deinterlace = deinterlace;

  if (mType == m_sType)
    return;

  m_sType = mType;

  if(m_pContext || m_pMode)
    Dispose();
}

void CDVDVideoPPFFmpeg::Process(VideoPicture* pPicture)
{
  VideoPicture* pSource = pPicture;
  CVideoBuffer *videoBuffer;

  if (pSource->videoBuffer->GetFormat() != AV_PIX_FMT_YUV420P)
    return;

  if (!CheckInit(pSource->iWidth, pSource->iHeight))
  {
    CLog::Log(LOGERROR, "Initialization of ffmpeg postprocessing failed");
    return;
  }

  uint8_t* srcPlanes[YuvImage::MAX_PLANES], *dstPlanes[YuvImage::MAX_PLANES];
  int srcStrides[YuvImage::MAX_PLANES];
  pSource->videoBuffer->GetPlanes(srcPlanes);
  pSource->videoBuffer->GetStrides(srcStrides);

  videoBuffer = m_processInfo.GetVideoBufferManager().Get(AV_PIX_FMT_YUV420P,
                                                          srcStrides[0] * pPicture->iHeight +
                                                          srcStrides[1] * pPicture->iHeight, nullptr);
  if (!videoBuffer)
  {
    return;
  }

  videoBuffer->SetDimensions(pPicture->iWidth, pPicture->iHeight, srcStrides);
  videoBuffer->GetPlanes(dstPlanes);
  pp_postprocess((const uint8_t **)srcPlanes, srcStrides,
                 dstPlanes, srcStrides,
                 pSource->iWidth, pSource->iHeight,
                 pSource->qp_table, pSource->qstride,
                 m_pMode, m_pContext,
                 pSource->pict_type | pSource->qscale_type ? PP_PICT_TYPE_QP2 : 0);


  pPicture->SetParams(*pSource);
  if (pPicture->videoBuffer)
    pPicture->videoBuffer->Release();
  pPicture->videoBuffer = videoBuffer;

  if (m_deinterlace)
    pPicture->iFlags &= ~DVP_FLAG_INTERLACED;
}

