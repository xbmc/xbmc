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
  memset(&m_pTarget, 0, sizeof(VideoPicture));
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

  if (m_pTarget.videoBuffer != nullptr)
  {
    m_pTarget.videoBuffer->Release();
    m_pTarget.videoBuffer = nullptr;
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

    m_pContext = pp_get_context(m_pSource->iWidth, m_pSource->iHeight, PPCPUFlags() | PP_FORMAT_420);

    m_iInitWidth = m_pSource->iWidth;
    m_iInitHeight = m_pSource->iHeight;

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

bool CDVDVideoPPFFmpeg::Process(VideoPicture* pPicture)
{
  m_pSource = pPicture;

  if (m_pSource->videoBuffer->GetFormat() != AV_PIX_FMT_YUV420P)
    return false;

  if (!CheckInit(m_pSource->iWidth, m_pSource->iHeight))
  {
    CLog::Log(LOGERROR, "Initialization of ffmpeg postprocessing failed");
    return false;
  }

  if (m_pTarget.videoBuffer)
  {
    m_pTarget.videoBuffer->Release();
    m_pTarget.videoBuffer = nullptr;
  }

  m_pTarget.videoBuffer = m_processInfo.GetVideoBufferManager().Get(AV_PIX_FMT_YUV420P, pPicture->iWidth * pPicture->iHeight * 3 / 2);
  if (!m_pTarget.videoBuffer)
  {
    return false;
  }

  int pict_type = (m_pSource->qscale_type != DVP_QSCALE_MPEG1) ?
                   PP_PICT_TYPE_QP2 : 0;

  uint8_t* srcPlanes[YuvImage::MAX_PLANES], *dstPlanes[YuvImage::MAX_PLANES];
  int srcStrides[YuvImage::MAX_PLANES], dstStrides[YuvImage::MAX_PLANES];
  m_pSource->videoBuffer->GetPlanes(srcPlanes);
  m_pSource->videoBuffer->GetStrides(srcStrides);
  m_pTarget.videoBuffer->GetPlanes(dstPlanes);
  m_pTarget.videoBuffer->GetStrides(dstStrides);
  pp_postprocess((const uint8_t**)srcPlanes, srcStrides,
                 dstPlanes, dstStrides,
                 m_pSource->iWidth, m_pSource->iHeight,
                 m_pSource->qp_table, m_pSource->qstride,
                 m_pMode, m_pContext,
                 pict_type); //m_pSource->iFrameType);

  //Copy frame information over to target, but make sure it is set as allocated should decoder have forgotten
  if (m_deinterlace)
    m_pTarget.iFlags &= ~DVP_FLAG_INTERLACED;
  m_pTarget.iFrameType = m_pSource->iFrameType;
  m_pTarget.iRepeatPicture = m_pSource->iRepeatPicture;
  m_pTarget.iDuration = m_pSource->iDuration;
  m_pTarget.qp_table = m_pSource->qp_table;
  m_pTarget.qstride = m_pSource->qstride;
  m_pTarget.qscale_type = m_pSource->qscale_type;
  m_pTarget.iDisplayHeight = m_pSource->iDisplayHeight;
  m_pTarget.iDisplayWidth = m_pSource->iDisplayWidth;
  m_pTarget.pts = m_pSource->pts;
  return true;
}

bool CDVDVideoPPFFmpeg::GetPicture(VideoPicture* pPicture)
{
  if (m_pTarget.videoBuffer)
  {
    pPicture = &m_pTarget;
    m_pTarget.videoBuffer = nullptr;
    return true;
  }
  return false;
}

