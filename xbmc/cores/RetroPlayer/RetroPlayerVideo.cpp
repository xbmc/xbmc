/*
 *      Copyright (C) 2012-2017 Team Kodi
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
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "RetroPlayerVideo.h"
#include "cores/RetroPlayer/process/RPProcessInfo.h"
#include "cores/RetroPlayer/rendering/RPRenderManager.h"
#include "utils/log.h"

using namespace KODI;
using namespace RETRO;

CRetroPlayerVideo::CRetroPlayerVideo(CRPRenderManager& renderManager, CRPProcessInfo& processInfo) :
  m_renderManager(renderManager),
  m_processInfo(processInfo)
{
  m_renderManager.Initialize();
}

CRetroPlayerVideo::~CRetroPlayerVideo()
{
  CloseStream();
  m_renderManager.Deinitialize();
}

bool CRetroPlayerVideo::OpenPixelStream(AVPixelFormat pixfmt, unsigned int width, unsigned int height, unsigned int orientationDeg)
{
  CLog::Log(LOGINFO, "RetroPlayerVideo: Creating video stream with pixel format: %i, %dx%d", pixfmt, width, height);

  m_processInfo.SetVideoPixelFormat(pixfmt);
  m_processInfo.SetVideoDimensions(width, height);

  return m_renderManager.Configure(pixfmt, width, height, orientationDeg);
}

bool CRetroPlayerVideo::OpenEncodedStream(AVCodecID codec)
{
  return false; //! @todo
}

void CRetroPlayerVideo::AddData(const uint8_t* data, unsigned int size)
{
  m_renderManager.AddFrame(data, size);
}

void CRetroPlayerVideo::CloseStream()
{
  m_renderManager.Flush();
}
