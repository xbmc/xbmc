/*
 *      Copyright (C) 2005-2011 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include <math.h>

#include "DVDVideoCodecAmlogic.h"
#include "DVDClock.h"
#include "DVDStreamInfo.h"
#include "AMLCodec.h"
#include "video/VideoThumbLoader.h"
#include "utils/log.h"

#define __MODULE_NAME__ "DVDVideoCodecAmlogic"

typedef struct pts_queue {
  double            dts;
  double            pts;
  double            sort_time;
  struct pts_queue  *nextpts;
} pts_queue;

CDVDVideoCodecAmlogic::CDVDVideoCodecAmlogic() :
  m_Codec(NULL),
  m_pFormatName("amcodec"),
  m_last_pts(0.0),
  m_pts_queue(NULL),
  m_queue_depth(0),
  m_framerate(0.0),
  m_video_rate(0)
{
  pthread_mutex_init(&m_queue_mutex, NULL);
}

CDVDVideoCodecAmlogic::~CDVDVideoCodecAmlogic()
{
  Dispose();
  pthread_mutex_destroy(&m_queue_mutex);
}

bool CDVDVideoCodecAmlogic::Open(CDVDStreamInfo &hints, CDVDCodecOptions &options)
{
  switch(hints.codec)
  {
    case CODEC_ID_MJPEG:
      m_pFormatName = "am-mjpeg";
      break;
    case CODEC_ID_MPEG1VIDEO:
    case CODEC_ID_MPEG2VIDEO:
    case CODEC_ID_MPEG2VIDEO_XVMC:
      m_pFormatName = "am-mpeg2";
      break;
    case CODEC_ID_H264:
      m_pFormatName = "am-h264";
      break;
    case CODEC_ID_MPEG4:
    case CODEC_ID_MSMPEG4V2:
    case CODEC_ID_MSMPEG4V3:
      m_pFormatName = "am-mpeg4";
      break;
    case CODEC_ID_H263:
    case CODEC_ID_H263P:
    case CODEC_ID_H263I:
      m_pFormatName = "am-h263";
      break;
    case CODEC_ID_FLV1:
      m_pFormatName = "am-flv1";
      break;
    case CODEC_ID_RV10:
    case CODEC_ID_RV20:
    case CODEC_ID_RV30:
    case CODEC_ID_RV40:
      m_pFormatName = "am-rv";
      break;
    case CODEC_ID_VC1:
      m_pFormatName = "am-vc1";
      break;
    case CODEC_ID_WMV3:
      m_pFormatName = "am-wmv3";
      break;
    case CODEC_ID_AVS:
    case CODEC_ID_CAVS:
      m_pFormatName = "am-avs";
      break;
    default:
      CLog::Log(LOGDEBUG, "%s: Unknown hints.codec(%d", __MODULE_NAME__, hints.codec);
      return false;
      break;
  }

  m_hints = hints;
  m_Codec = new CAMLCodec();
  if (!m_Codec)
  {
    CLog::Log(LOGERROR, "%s: Failed to create Amlogic Codec", __MODULE_NAME__);
    return false;
  }
  m_opened = false;

  // allocate a dummy DVDVideoPicture buffer.
  // first make sure all properties are reset.
  memset(&m_videobuffer, 0, sizeof(DVDVideoPicture));

  m_videobuffer.dts = DVD_NOPTS_VALUE;
  m_videobuffer.pts = DVD_NOPTS_VALUE;
  m_videobuffer.format = RENDER_FMT_BYPASS;
  m_videobuffer.color_range  = 0;
  m_videobuffer.color_matrix = 4;
  m_videobuffer.iFlags  = DVP_FLAG_ALLOCATED;
  m_videobuffer.iWidth  = hints.width;
  m_videobuffer.iHeight = hints.height;

  m_videobuffer.iDisplayWidth  = m_videobuffer.iWidth;
  m_videobuffer.iDisplayHeight = m_videobuffer.iHeight;
  if (hints.aspect > 0.0 && !hints.forced_aspect)
  {
    m_videobuffer.iDisplayWidth  = ((int)lrint(m_videobuffer.iHeight * hints.aspect)) & -3;
    if (m_videobuffer.iDisplayWidth > m_videobuffer.iWidth)
    {
      m_videobuffer.iDisplayWidth  = m_videobuffer.iWidth;
      m_videobuffer.iDisplayHeight = ((int)lrint(m_videobuffer.iWidth / hints.aspect)) & -3;
    }
  }

  CJobManager::GetInstance().Pause(kJobTypeMediaFlags);
  if (CJobManager::GetInstance().IsProcessing(kJobTypeMediaFlags) > 0)
  {
    int timeout_ms = 5000;
    while (timeout_ms > 0)
    {
      if (CJobManager::GetInstance().IsProcessing(kJobTypeMediaFlags) > 0)
      {
        Sleep(100);
        timeout_ms -= 100;
      }
      else
        break;
    }
  }

  CLog::Log(LOGINFO, "%s: Opened Amlogic Codec", __MODULE_NAME__);
  return true;
}

void CDVDVideoCodecAmlogic::Dispose(void)
{
  if (m_Codec)
    m_Codec->CloseDecoder(), m_Codec = NULL;
  if (m_videobuffer.iFlags)
    m_videobuffer.iFlags = 0;
  while (m_queue_depth)
    PtsQueuePop();

  // let thumbgen jobs resume.
  CJobManager::GetInstance().UnPause(kJobTypeMediaFlags);
}

int CDVDVideoCodecAmlogic::Decode(BYTE *pData, int iSize, double dts, double pts)
{
  // Handle Input, add demuxer packet to input queue, we must accept it or
  // it will be discarded as DVDPlayerVideo has no concept of "try again".

  if (!m_opened)
  {
    if (m_Codec && !m_Codec->OpenDecoder(m_hints))
      CLog::Log(LOGERROR, "%s: Failed to open Amlogic Codec", __MODULE_NAME__);
    m_opened = true;
  }

  if (m_hints.ptsinvalid)
    pts = DVD_NOPTS_VALUE;

  if (pData)
    FrameRateTracking(dts, pts);

  return m_Codec->Decode(pData, iSize, dts, pts);
}

void CDVDVideoCodecAmlogic::Reset(void)
{
  while (m_queue_depth)
    PtsQueuePop();

  m_Codec->Reset();
}

bool CDVDVideoCodecAmlogic::GetPicture(DVDVideoPicture* pDvdVideoPicture)
{
  if (m_Codec)
    m_Codec->GetPicture(&m_videobuffer);
  *pDvdVideoPicture = m_videobuffer;

  return true;
}

void CDVDVideoCodecAmlogic::SetDropState(bool bDrop)
{
}

void CDVDVideoCodecAmlogic::SetSpeed(int iSpeed)
{
  if (m_Codec)
    m_Codec->SetSpeed(iSpeed);
}

int CDVDVideoCodecAmlogic::GetDataSize(void)
{
  if (m_Codec)
    return m_Codec->GetDataSize();
  else
    return 0;
}

double CDVDVideoCodecAmlogic::GetTimeSize(void)
{
  if (m_Codec)
    return m_Codec->GetTimeSize();
  else
    return 0;
}

void CDVDVideoCodecAmlogic::PtsQueuePop(void)
{
  if (!m_pts_queue || m_queue_depth == 0)
    return;

  pthread_mutex_lock(&m_queue_mutex);
  // pop the top frame off the queue
  pts_queue *top_pts = m_pts_queue;
  m_pts_queue = top_pts->nextpts;
  m_queue_depth--;
  pthread_mutex_unlock(&m_queue_mutex);

  // and release it
  free(top_pts);
}

void CDVDVideoCodecAmlogic::PtsQueuePush(double dts, double pts)
{
  pts_queue *newpts = (pts_queue*)calloc(sizeof(pts_queue), 1);
  newpts->dts = dts;
  newpts->pts = pts;
  // if both dts or pts are good we use those, else use decoder insert time for frame sort
  if ((newpts->pts != DVD_NOPTS_VALUE) || (newpts->dts != DVD_NOPTS_VALUE))
  {
    // if pts is borked (stupid avi's), use dts for frame sort
    if (newpts->pts == DVD_NOPTS_VALUE)
      newpts->sort_time = newpts->dts;
    else
      newpts->sort_time = newpts->pts;
  }

  pthread_mutex_lock(&m_queue_mutex);
  pts_queue *queueWalker = m_pts_queue;
  if (!queueWalker || (newpts->sort_time < queueWalker->sort_time))
  {
    // we have an empty queue, or this frame earlier than the current queue head.
    newpts->nextpts = queueWalker;
    m_pts_queue = newpts;
  } else {
    // walk the queue and insert this frame where it belongs in display order.
    bool ptrInserted = false;
    pts_queue *nextpts = NULL;
    //
    while (!ptrInserted)
    {
      nextpts = queueWalker->nextpts;
      if (!nextpts || (newpts->sort_time < nextpts->sort_time))
      {
        // if the next frame is the tail of the queue, or our new frame is earlier.
        newpts->nextpts = nextpts;
        queueWalker->nextpts = newpts;
        ptrInserted = true;
      }
      queueWalker = nextpts;
    }
  }
  m_queue_depth++;
  pthread_mutex_unlock(&m_queue_mutex);	
}

void CDVDVideoCodecAmlogic::FrameRateTracking(double dts, double pts)
{
  PtsQueuePush(dts, pts);

  // we might have out-of-order pts,
  // so make sure we wait for at least 16 values in sorted queue.
  if (m_queue_depth > 16)
  {
    pthread_mutex_lock(&m_queue_mutex);

    float cur_pts = m_pts_queue->pts;
    if (cur_pts == DVD_NOPTS_VALUE)
      cur_pts = m_pts_queue->dts;

    pthread_mutex_unlock(&m_queue_mutex);	

    float duration = cur_pts - m_last_pts;
    m_last_pts = cur_pts;

    // clamp duration to sensible range,
    // 66 fsp to 20 fsp
    if (duration >= 15000.0 && duration <= 50000.0)
    {
      double framerate;
      switch((int)(0.5 + duration))
      {
        // 59.940 (16683.333333)
        case 16000 ... 17000:
          framerate = 60000.0 / 1001.0;
          break;

        // 50.000 (20000.000000)
        case 20000:
          framerate = 50000.0 / 1000.0;
          break;

        // 49.950 (20020.000000)
        case 20020:
          framerate = 50000.0 / 1001.0;
          break;

        // 29.970 (33366.666656)
        case 32000 ... 35000:
          framerate = 30000.0 / 1001.0;
          break;

        // 25.000 (40000.000000)
        case 40000:
          framerate = 25000.0 / 1000.0;
          break;

        // 24.975 (40040.000000)
        case 40040:
          framerate = 25000.0 / 1001.0;
          break;

        /*
        // 24.000 (41666.666666)
        case 41667:
          framerate = 24000.0 / 1000.0;
          break;
        */

        // 23.976 (41708.33333)
        case 40200 ... 43200:
          // 23.976 seems to have the crappiest encodings :)
          framerate = 24000.0 / 1001.0;
          break;

        default:
          framerate = 0.0;
          CLog::Log(LOGDEBUG, "%s: unknown duration(%f), cur_pts(%f)",
            __MODULE_NAME__, duration, cur_pts);
          break;
      }

      if (framerate > 0.0 && (int)m_framerate != (int)framerate)
      {
        m_framerate = framerate;
        m_video_rate = (int)(0.5 + (96000.0 / framerate));
        CLog::Log(LOGDEBUG, "%s: detected new framerate(%f), video_rate(%d)",
          __MODULE_NAME__, m_framerate, m_video_rate);
      }
    }

    PtsQueuePop();
  }
}
