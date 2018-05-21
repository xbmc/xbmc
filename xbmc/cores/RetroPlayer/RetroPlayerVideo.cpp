/*
 *      Copyright (C) 2012-present Team Kodi
 *      http://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "RetroPlayerVideo.h"
#include "cores/RetroPlayer/process/RPProcessInfo.h"
#include "cores/RetroPlayer/rendering/RenderTranslator.h"
#include "cores/RetroPlayer/rendering/RPRenderManager.h"
#include "utils/log.h"

using namespace KODI;
using namespace RETRO;

CRetroPlayerVideo::CRetroPlayerVideo(CRPRenderManager& renderManager, CRPProcessInfo& processInfo) :
  m_renderManager(renderManager),
  m_processInfo(processInfo)
{
  CLog::Log(LOGDEBUG, "RetroPlayer[VIDEO]: Initializing video");

  m_renderManager.Initialize();
}

CRetroPlayerVideo::~CRetroPlayerVideo()
{
  CLog::Log(LOGDEBUG, "RetroPlayer[VIDEO]: Deinitializing video");

  CloseStream();
  m_renderManager.Deinitialize();
}

bool CRetroPlayerVideo::OpenPixelStream(AVPixelFormat pixfmt, unsigned int width, unsigned int height, unsigned int orientationDeg)
{
  CLog::Log(LOGDEBUG, "RetroPlayer[VIDEO]: Creating video stream - format %s, %ux%u, %u deg",
      CRenderTranslator::TranslatePixelFormat(pixfmt),
      width,
      height,
      orientationDeg);

  m_processInfo.SetVideoPixelFormat(pixfmt);
  m_processInfo.SetVideoDimensions(width, height);

  return m_renderManager.Configure(pixfmt, width, height, orientationDeg);
}

bool CRetroPlayerVideo::OpenEncodedStream(AVCodecID codec)
{
  CLog::Log(LOGERROR, "RetroPlayer[VIDEO]: Encoded video stream not supported");

  return false; //! @todo
}

void CRetroPlayerVideo::AddData(const uint8_t* data, unsigned int size)
{
  m_renderManager.AddFrame(data, size);
}

void CRetroPlayerVideo::CloseStream()
{
  CLog::Log(LOGDEBUG, "RetroPlayer[VIDEO]: Closing video stream");

  m_renderManager.Flush();
}
