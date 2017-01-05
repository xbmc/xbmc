/*
 *      Copyright (C) 2012-2016 Team Kodi
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
#include "RetroPlayerDefines.h"
#include "PixelConverter.h"
#include "PixelConverterRBP.h"
#include "cores/VideoPlayer/DVDCodecs/DVDCodecUtils.h"
#include "cores/VideoPlayer/VideoRenderers/RenderFlags.h"
#include "cores/VideoPlayer/VideoRenderers/RenderManager.h"
#include "utils/log.h"

#include <atomic> //! @todo

using namespace GAME;

CRetroPlayerVideo::CRetroPlayerVideo(CRenderManager& renderManager, CProcessInfo& processInfo) :
  //CThread("RetroPlayerVideo"),
  m_renderManager(renderManager),
  m_processInfo(processInfo),
  m_framerate(0.0),
  m_orientation(0),
  m_bConfigured(false),
  m_droppedFrames(0)
{
  m_renderManager.PreInit();
}

CRetroPlayerVideo::~CRetroPlayerVideo()
{
  CloseStream();
  m_renderManager.UnInit();
}

bool CRetroPlayerVideo::OpenPixelStream(AVPixelFormat pixfmt, unsigned int width, unsigned int height, double framerate, unsigned int orientationDeg)
{
  CLog::Log(LOGINFO, "RetroPlayerVideo: Creating video stream with pixel format: %i, %dx%d", pixfmt, width, height);

  m_framerate = framerate;
  m_orientation = orientationDeg;
  m_bConfigured = false;
  m_droppedFrames = 0;

#ifdef TARGET_RASPBERRY_PI
  m_pixelConverter.reset(new CPixelConverterRBP);
#else
  m_pixelConverter.reset(new CPixelConverter);
#endif

  if (m_pixelConverter->Open(pixfmt, AV_PIX_FMT_YUV420P, width, height))
  {
    //! @todo
    //m_processInfo.SetVideoPixelFormat(CDVDVideoCodecFFmpeg::GetPixelFormatName(pixfmt));
    m_processInfo.SetVideoDimensions(width, height);
    m_processInfo.SetVideoFps(static_cast<float>(framerate));
    return true;
  }

  m_pixelConverter.reset();

  return false;
}

bool CRetroPlayerVideo::OpenEncodedStream(AVCodecID codec)
{
  /*
  CDemuxStreamVideo videoStream;

  // Stream
  videoStream.uniqueId = GAME_STREAM_VIDEO_ID;
  videoStream.codec = codec;
  videoStream.type = STREAM_VIDEO;
  videoStream.source = STREAM_SOURCE_DEMUX;
  videoStream.realtime = true;
  */

  // Video
  //! @todo Needed?
  /*
  videoStream.iFpsScale = 1000;
  videoStream.iFpsRate = static_cast<int>(framerate * 1000);
  videoStream.iHeight = height;
  videoStream.iWidth = width;
  videoStream.fAspect = static_cast<float>(width) / static_cast<float>(height);
  videoStream.iOrientation = orientationDeg;
  */

  //! @todo

  return false;
}

void CRetroPlayerVideo::AddData(const uint8_t* data, unsigned int size)
{
  DVDVideoPicture picture = { };

  if (GetPicture(data, size, picture))
  {
    if (!Configure(picture))
    {
      CLog::Log(LOGERROR, "RetroPlayerVideo: Failed to configure renderer");
      CloseStream();
    }
    else
    {
      SendPicture(picture);
    }
  }
}

void CRetroPlayerVideo::CloseStream()
{
  m_renderManager.Flush();
  m_pixelConverter.reset();
}

bool CRetroPlayerVideo::Configure(DVDVideoPicture& picture)
{
  if (!m_bConfigured)
  {
    // Determine RenderManager flags
    unsigned int flags = CONF_FLAGS_YUVCOEF_BT601 | // color_matrix = 4
                         CONF_FLAGS_FULLSCREEN;     // Allow fullscreen

    const int buffers = 1; //! @todo

    m_bConfigured = m_renderManager.Configure(picture, static_cast<float>(m_framerate), flags, m_orientation, buffers);

    if (m_bConfigured)
    {
      // Update process info
      AVPixelFormat pixfmt = static_cast<AVPixelFormat>(CDVDCodecUtils::PixfmtFromEFormat(picture.format));
      if (pixfmt != AV_PIX_FMT_NONE)
      {
        //! @todo
        //m_processInfo.SetVideoPixelFormat(CDVDVideoCodecFFmpeg::GetPixelFormatName(pixfmt));
      }
      m_processInfo.SetVideoDimensions(picture.iWidth, picture.iHeight);
      m_processInfo.SetVideoFps(static_cast<float>(m_framerate));
    }
  }

  return m_bConfigured;
}

bool CRetroPlayerVideo::GetPicture(const uint8_t* data, unsigned int size, DVDVideoPicture& picture)
{
  bool bHasPicture = false;

  if (m_pixelConverter)
  {
    int lateframes;
    double renderPts;
    int queued, discard;
    m_renderManager.GetStats(lateframes, renderPts, queued, discard);

    // Drop frame if another is queued
    const bool bDropped = (queued > 0);

    if (!bDropped)
    {
      if (m_pixelConverter->Decode(data, size))
      {
        m_pixelConverter->GetPicture(picture);
        bHasPicture = true;
      }
    }
  }

  return bHasPicture;
}

void CRetroPlayerVideo::SendPicture(DVDVideoPicture& picture)
{
  std::atomic_bool bAbortOutput(false); //! @todo

  int index = m_renderManager.AddVideoPicture(picture);
  if (index < 0)
  {
    // Video device might not be done yet, drop the frame
    m_droppedFrames++;
  }
  else
  {
    m_renderManager.FlipPage(bAbortOutput, 0.0, VS_INTERLACEMETHOD_NONE, FS_NONE, false);
  }
}
