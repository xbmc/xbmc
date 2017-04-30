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

#include <math.h>

#include "DVDVideoCodecAmlogic.h"
#include "TimingConstants.h"
#include "DVDStreamInfo.h"
#include "AMLCodec.h"
#include "ServiceBroker.h"
#include "utils/AMLUtils.h"
#include "utils/BitstreamConverter.h"
#include "utils/log.h"
#include "utils/SysfsUtils.h"
#include "settings/Settings.h"
#include "settings/AdvancedSettings.h"
#include "threads/Thread.h"

#define __MODULE_NAME__ "DVDVideoCodecAmlogic"

typedef struct frame_queue {
  double dts;
  double pts;
  double sort_time;
  struct frame_queue *nextframe;
} frame_queue;

CDVDVideoCodecAmlogic::CDVDVideoCodecAmlogic(CProcessInfo &processInfo) : CDVDVideoCodec(processInfo),
  m_Codec(NULL),
  m_pFormatName("amcodec"),
  m_opened(false),
  m_codecControlFlags(0),
  m_last_pts(0.0),
  m_frame_queue(NULL),
  m_queue_depth(0),
  m_framerate(0.0),
  m_video_rate(0),
  m_mpeg2_sequence(NULL),
  m_has_keyframe(false),
  m_bitparser(NULL),
  m_bitstream(NULL)
{
  pthread_mutex_init(&m_queue_mutex, NULL);
}

CDVDVideoCodecAmlogic::~CDVDVideoCodecAmlogic()
{
  Dispose();
  pthread_mutex_destroy(&m_queue_mutex);
}

std::atomic<bool> CDVDVideoCodecAmlogic::m_InstanceGuard(false);

bool CDVDVideoCodecAmlogic::Open(CDVDStreamInfo &hints, CDVDCodecOptions &options)
{
  if (!CServiceBroker::GetSettings().GetBool(CSettings::SETTING_VIDEOPLAYER_USEAMCODEC))
    return false;
  if (hints.stills || hints.width == 0)
    return false;

  if (!aml_permissions())
  {
    CLog::Log(LOGERROR, "AML: no proper permission, please contact the device vendor. Skipping codec...");
    return false;
  }

  m_opened = false;

  // allow only 1 instance here
  if (m_InstanceGuard.exchange(true))
  {
    CLog::Log(LOGERROR, "CDVDVideoCodecAmlogic::Open - InstanceGuard locked\n");
    return false;
  }

  m_hints = hints;

  CLog::Log(LOGDEBUG, "CDVDVideoCodecAmlogic::Opening: codec %d profile:%d extra_size:%d", m_hints.codec, hints.profile, hints.extrasize);

  switch(m_hints.codec)
  {
    case AV_CODEC_ID_MJPEG:
      m_pFormatName = "am-mjpeg";
      break;
    case AV_CODEC_ID_MPEG1VIDEO:
    case AV_CODEC_ID_MPEG2VIDEO:
    case AV_CODEC_ID_MPEG2VIDEO_XVMC:
      if (m_hints.width <= CServiceBroker::GetSettings().GetInt(CSettings::SETTING_VIDEOPLAYER_USEAMCODECMPEG2))
        return false;
      m_mpeg2_sequence_pts = 0;
      m_mpeg2_sequence = new mpeg2_sequence;
      m_mpeg2_sequence->width  = m_hints.width;
      m_mpeg2_sequence->height = m_hints.height;
      m_mpeg2_sequence->ratio  = m_hints.aspect;
      m_mpeg2_sequence->fps_rate  = m_hints.fpsrate;
      m_mpeg2_sequence->fps_scale  = m_hints.fpsscale;
      m_pFormatName = "am-mpeg2";
      break;
    case AV_CODEC_ID_H264:
      if (m_hints.width <= CServiceBroker::GetSettings().GetInt(CSettings::SETTING_VIDEOPLAYER_USEAMCODECH264))
      {
        CLog::Log(LOGDEBUG, "CDVDVideoCodecAmlogic::h264 size check failed %d",CServiceBroker::GetSettings().GetInt(CSettings::SETTING_VIDEOPLAYER_USEAMCODECH264));
        goto FAIL;
      }
      switch(hints.profile)
      {
        case FF_PROFILE_H264_HIGH_10:
        case FF_PROFILE_H264_HIGH_10_INTRA:
        case FF_PROFILE_H264_HIGH_422:
        case FF_PROFILE_H264_HIGH_422_INTRA:
        case FF_PROFILE_H264_HIGH_444_PREDICTIVE:
        case FF_PROFILE_H264_HIGH_444_INTRA:
        case FF_PROFILE_H264_CAVLC_444:
          goto FAIL;
      }
      if ((aml_support_h264_4k2k() == AML_NO_H264_4K2K) && ((m_hints.width > 1920) || (m_hints.height > 1088)))
      {
        // 4K is supported only on Amlogic S802/S812 chip
        goto FAIL;
      }
      m_pFormatName = "am-h264";
      // convert h264-avcC to h264-annex-b as h264-avcC
      // under streamers can have issues when seeking.
      if (m_hints.extradata && *(uint8_t*)m_hints.extradata == 1)
      {
        m_bitstream = new CBitstreamConverter;
        m_bitstream->Open(m_hints.codec, (uint8_t*)m_hints.extradata, m_hints.extrasize, true);
        m_bitstream->ResetStartDecode();
        // make sure we do not leak the existing m_hints.extradata
        free(m_hints.extradata);
        m_hints.extrasize = m_bitstream->GetExtraSize();
        m_hints.extradata = malloc(m_hints.extrasize);
        memcpy(m_hints.extradata, m_bitstream->GetExtraData(), m_hints.extrasize);
      }
      else
      {
        m_bitparser = new CBitstreamParser();
        m_bitparser->Open();
      }
      break;
    case AV_CODEC_ID_MPEG4:
    case AV_CODEC_ID_MSMPEG4V2:
    case AV_CODEC_ID_MSMPEG4V3:
      if (m_hints.width <= CServiceBroker::GetSettings().GetInt(CSettings::SETTING_VIDEOPLAYER_USEAMCODECMPEG4))
        goto FAIL;
      m_pFormatName = "am-mpeg4";
      break;
    case AV_CODEC_ID_H263:
    case AV_CODEC_ID_H263P:
    case AV_CODEC_ID_H263I:
      // amcodec can't handle h263
      goto FAIL;
//    case AV_CODEC_ID_FLV1:
//      m_pFormatName = "am-flv1";
//      break;
    case AV_CODEC_ID_RV10:
    case AV_CODEC_ID_RV20:
    case AV_CODEC_ID_RV30:
    case AV_CODEC_ID_RV40:
      // m_pFormatName = "am-rv";
      // rmvb is not handled well by amcodec
      goto FAIL;
    case AV_CODEC_ID_VC1:
      m_pFormatName = "am-vc1";
      break;
    case AV_CODEC_ID_WMV3:
      m_pFormatName = "am-wmv3";
      break;
    case AV_CODEC_ID_AVS:
    case AV_CODEC_ID_CAVS:
      m_pFormatName = "am-avs";
      break;
    case AV_CODEC_ID_HEVC:
      if (aml_support_hevc()) {
        if (!aml_support_hevc_4k2k() && ((m_hints.width > 1920) || (m_hints.height > 1088)))
        {
          // 4K HEVC is supported only on Amlogic S812 chip
          goto FAIL;
        }
      } else {
        // HEVC supported only on S805 and S812.
        goto FAIL;
      }
      if ((hints.profile == FF_PROFILE_HEVC_MAIN_10) && !aml_support_hevc_10bit())
      {
        goto FAIL;
      }
      m_pFormatName = "am-h265";
      m_bitstream = new CBitstreamConverter();
      m_bitstream->Open(m_hints.codec, (uint8_t*)m_hints.extradata, m_hints.extrasize, true);
      // make sure we do not leak the existing m_hints.extradata
      free(m_hints.extradata);
      m_hints.extrasize = m_bitstream->GetExtraSize();
      m_hints.extradata = malloc(m_hints.extrasize);
      memcpy(m_hints.extradata, m_bitstream->GetExtraData(), m_hints.extrasize);
      break;
    default:
      CLog::Log(LOGDEBUG, "%s: Unknown hints.codec(%d", __MODULE_NAME__, m_hints.codec);
      goto FAIL;
  }

  m_aspect_ratio = m_hints.aspect;
  m_Codec = new CAMLCodec();
  if (!m_Codec)
  {
    CLog::Log(LOGERROR, "%s: Failed to create Amlogic Codec", __MODULE_NAME__);
    goto FAIL;
  }

  // allocate a dummy VideoPicture buffer.
  // first make sure all properties are reset.
  memset(&m_videobuffer, 0, sizeof(VideoPicture));

  m_videobuffer.dts = DVD_NOPTS_VALUE;
  m_videobuffer.pts = DVD_NOPTS_VALUE;
  m_videobuffer.format = RENDER_FMT_AML;
  m_videobuffer.color_range  = 0;
  m_videobuffer.color_matrix = 4;
  m_videobuffer.iFlags  = DVP_FLAG_ALLOCATED;
  m_videobuffer.iWidth  = m_hints.width;
  m_videobuffer.iHeight = m_hints.height;
  m_videobuffer.hwPic = NULL;

  m_videobuffer.iDisplayWidth  = m_videobuffer.iWidth;
  m_videobuffer.iDisplayHeight = m_videobuffer.iHeight;
  if (m_hints.aspect > 0.0 && !m_hints.forced_aspect)
  {
    m_videobuffer.iDisplayWidth  = ((int)lrint(m_videobuffer.iHeight * m_hints.aspect)) & ~3;
    if (m_videobuffer.iDisplayWidth > m_videobuffer.iWidth)
    {
      m_videobuffer.iDisplayWidth  = m_videobuffer.iWidth;
      m_videobuffer.iDisplayHeight = ((int)lrint(m_videobuffer.iWidth / m_hints.aspect)) & ~3;
    }
  }

  m_processInfo.SetVideoDecoderName(m_pFormatName, true);
  m_processInfo.SetVideoDimensions(m_hints.width, m_hints.height);
  m_processInfo.SetVideoDeintMethod("hardware");
  m_processInfo.SetVideoDAR(m_hints.aspect);

  m_has_keyframe = false;

  CLog::Log(LOGINFO, "%s: Opened Amlogic Codec", __MODULE_NAME__);
  return true;
FAIL:
  m_InstanceGuard.exchange(false);
  Dispose();
}

void CDVDVideoCodecAmlogic::Dispose(void)
{
  {
    CSingleLock lock(m_secure);
    for (std::set<CDVDAmlogicInfo*>::iterator it = m_inflight.begin(); it != m_inflight.end(); ++it)
      (*it)->invalidate();
  }

  if (m_Codec)
    m_Codec->CloseDecoder(), delete m_Codec, m_Codec = NULL, m_InstanceGuard.exchange(false);

  if (m_opened)
    m_InstanceGuard.exchange(false);

  if (m_videobuffer.iFlags)
    m_videobuffer.iFlags = 0;
  if (m_mpeg2_sequence)
    delete m_mpeg2_sequence, m_mpeg2_sequence = NULL;

  if (m_bitstream)
    delete m_bitstream, m_bitstream = NULL;

  if (m_bitparser)
    delete m_bitparser, m_bitparser = NULL;

  while (m_queue_depth)
    FrameQueuePop();

  m_opened = false;
}

bool CDVDVideoCodecAmlogic::AddData(const DemuxPacket &packet)
{
  // Handle Input, add demuxer packet to input queue, we must accept it or
  // it will be discarded as VideoPlayerVideo has no concept of "try again".

  uint8_t *pData(packet.pData);
  int iSize(packet.iSize);

  if (pData)
  {
    if (m_bitstream)
    {
      if (!m_bitstream->Convert(pData, iSize))
        return true;

      if (!m_bitstream->CanStartDecode())
      {
        CLog::Log(LOGDEBUG, "%s::Decode waiting for keyframe (bitstream)", __MODULE_NAME__);
        return true;
      }
      pData = m_bitstream->GetConvertBuffer();
      iSize = m_bitstream->GetConvertSize();
    }
    else if (!m_has_keyframe && m_bitparser)
    {
      if (!m_bitparser->CanStartDecode(pData, iSize))
      {
        CLog::Log(LOGDEBUG, "%s::Decode waiting for keyframe (bitparser)", __MODULE_NAME__);
        return true;
      }
      else
        m_has_keyframe = true;
    }
    FrameRateTracking( pData, iSize, packet.dts, packet.pts);

    if (!m_opened)
    {
      if (packet.pts == DVD_NOPTS_VALUE)
        m_hints.ptsinvalid = true;

      if (m_Codec && !m_Codec->OpenDecoder(m_hints))
        CLog::Log(LOGERROR, "%s: Failed to open Amlogic Codec", __MODULE_NAME__);
      m_opened = true;
    }
  }

  return m_Codec->AddData(pData, iSize, packet.dts, m_hints.ptsinvalid ? DVD_NOPTS_VALUE : packet.pts);
}

void CDVDVideoCodecAmlogic::Reset(void)
{
  while (m_queue_depth)
    FrameQueuePop();

  m_Codec->Reset();
  m_mpeg2_sequence_pts = 0;
  m_has_keyframe = false;
  if (m_bitstream && m_hints.codec == AV_CODEC_ID_H264)
    m_bitstream->ResetStartDecode();
}

CDVDVideoCodec::VCReturn CDVDVideoCodecAmlogic::GetPicture(VideoPicture* pVideoPicture)
{
  if (!m_Codec)
    return VC_ERROR;

  VCReturn retVal = m_Codec->GetPicture(&m_videobuffer);

  *pVideoPicture = m_videobuffer;

  CDVDAmlogicInfo* info = new CDVDAmlogicInfo(this, m_Codec, 
   m_Codec->GetOMXPts(), m_Codec->GetAmlDuration(), m_Codec->GetBufferIndex());

  {
    CSingleLock lock(m_secure);
    m_inflight.insert(info);
  }

  pVideoPicture->hwPic = info->Retain();

  // check for mpeg2 aspect ratio changes
  if (m_mpeg2_sequence && pVideoPicture->pts >= m_mpeg2_sequence_pts)
    m_aspect_ratio = m_mpeg2_sequence->ratio;

  pVideoPicture->iDisplayWidth  = pVideoPicture->iWidth;
  pVideoPicture->iDisplayHeight = pVideoPicture->iHeight;
  if (m_aspect_ratio > 1.0 && !m_hints.forced_aspect)
  {
    pVideoPicture->iDisplayWidth  = ((int)lrint(pVideoPicture->iHeight * m_aspect_ratio)) & ~3;
    if (pVideoPicture->iDisplayWidth > pVideoPicture->iWidth)
    {
      pVideoPicture->iDisplayWidth  = pVideoPicture->iWidth;
      pVideoPicture->iDisplayHeight = ((int)lrint(pVideoPicture->iWidth / m_aspect_ratio)) & ~3;
    }
  }

  return retVal;
}

bool CDVDVideoCodecAmlogic::ClearPicture(VideoPicture *pVideoPicture)
{
  if (pVideoPicture->hwPic)
  {
    static_cast<CDVDAmlogicInfo*>(pVideoPicture->hwPic)->Release();
    pVideoPicture->hwPic = nullptr;
  }
  return true;
}

void CDVDVideoCodecAmlogic::SetCodecControl(int flags)
{
  if (m_codecControlFlags != flags)
  {
    if (g_advancedSettings.CanLogComponent(LOGVIDEO))
      CLog::Log(LOGDEBUG, "%s %x->%x",  __func__, m_codecControlFlags, flags);
    m_codecControlFlags = flags;

    if (flags & DVD_CODEC_CTRL_DROP)
      m_videobuffer.iFlags |= DVP_FLAG_DROPPED;
    else
      m_videobuffer.iFlags &= ~DVP_FLAG_DROPPED;

    if (m_Codec)
      m_Codec->SetDrain((flags & DVD_CODEC_CTRL_DRAIN) != 0);
  }
}

void CDVDVideoCodecAmlogic::SetSpeed(int iSpeed)
{
  if (m_Codec)
    m_Codec->SetSpeed(iSpeed);
}

void CDVDVideoCodecAmlogic::FrameQueuePop(void)
{
  if (!m_frame_queue || m_queue_depth == 0)
    return;

  pthread_mutex_lock(&m_queue_mutex);
  // pop the top frame off the queue
  frame_queue *top = m_frame_queue;
  m_frame_queue = top->nextframe;
  m_queue_depth--;
  pthread_mutex_unlock(&m_queue_mutex);

  // and release it
  free(top);
}

void CDVDVideoCodecAmlogic::FrameQueuePush(double dts, double pts)
{
  frame_queue *newframe = (frame_queue*)calloc(sizeof(frame_queue), 1);
  newframe->dts = dts;
  newframe->pts = pts;
  // if both dts or pts are good we use those, else use decoder insert time for frame sort
  if ((newframe->pts != DVD_NOPTS_VALUE) || (newframe->dts != DVD_NOPTS_VALUE))
  {
    // if pts is borked (stupid avi's), use dts for frame sort
    if (newframe->pts == DVD_NOPTS_VALUE)
      newframe->sort_time = newframe->dts;
    else
      newframe->sort_time = newframe->pts;
  }

  pthread_mutex_lock(&m_queue_mutex);
  frame_queue *queueWalker = m_frame_queue;
  if (!queueWalker || (newframe->sort_time < queueWalker->sort_time))
  {
    // we have an empty queue, or this frame earlier than the current queue head.
    newframe->nextframe = queueWalker;
    m_frame_queue = newframe;
  }
  else
  {
    // walk the queue and insert this frame where it belongs in display order.
    bool ptrInserted = false;
    frame_queue *nextframe = NULL;
    //
    while (!ptrInserted)
    {
      nextframe = queueWalker->nextframe;
      if (!nextframe || (newframe->sort_time < nextframe->sort_time))
      {
        // if the next frame is the tail of the queue, or our new frame is earlier.
        newframe->nextframe = nextframe;
        queueWalker->nextframe = newframe;
        ptrInserted = true;
      }
      queueWalker = nextframe;
    }
  }
  m_queue_depth++;
  pthread_mutex_unlock(&m_queue_mutex);	
}

void CDVDVideoCodecAmlogic::FrameRateTracking(uint8_t *pData, int iSize, double dts, double pts)
{
  // mpeg2 handling
  if (m_mpeg2_sequence)
  {
    // probe demux for sequence_header_code NAL and
    // decode aspect ratio and frame rate.
    if (CBitstreamConverter::mpeg2_sequence_header(pData, iSize, m_mpeg2_sequence))
    {
      m_mpeg2_sequence_pts = pts;
      if (m_mpeg2_sequence_pts == DVD_NOPTS_VALUE)
        m_mpeg2_sequence_pts = dts;

      m_hints.fpsrate = m_mpeg2_sequence->fps_rate;
      m_hints.fpsscale = m_mpeg2_sequence->fps_scale;
      m_framerate = static_cast<float>(m_mpeg2_sequence->fps_rate) / m_mpeg2_sequence->fps_scale;
      m_video_rate = (int)(0.5 + (96000.0 / m_framerate));

      m_hints.width    = m_mpeg2_sequence->width;
      m_hints.height   = m_mpeg2_sequence->height;
      m_hints.aspect   = m_mpeg2_sequence->ratio;

      m_processInfo.SetVideoFps(m_framerate);
      m_processInfo.SetVideoDAR(m_hints.aspect);
    }
    return;
  }

  // everything else
  FrameQueuePush(dts, pts);

  // we might have out-of-order pts,
  // so make sure we wait for at least 8 values in sorted queue.
  if (m_queue_depth > 16)
  {
    pthread_mutex_lock(&m_queue_mutex);

    float cur_pts = m_frame_queue->pts;
    if (cur_pts == DVD_NOPTS_VALUE)
      cur_pts = m_frame_queue->dts;

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
        case 39900 ... 40100:
          framerate = 25000.0 / 1000.0;
          break;

        // 23.976 (41708.33333)
        case 40200 ... 43200:
          // 23.976 seems to have the crappiest encodings :)
          framerate = 24000.0 / 1001.0;
          break;

        default:
          framerate = 0.0;
          //CLog::Log(LOGDEBUG, "%s: unknown duration(%f), cur_pts(%f)",
          //  __MODULE_NAME__, duration, cur_pts);
          break;
      }

      if (framerate > 0.0 && (int)m_framerate != (int)framerate)
      {
        m_framerate = framerate;
        m_video_rate = (int)(0.5 + (96000.0 / framerate));

        if (m_Codec)
          m_Codec->SetVideoRate(m_video_rate);

        m_processInfo.SetVideoFps(m_framerate);

        CLog::Log(LOGDEBUG, "%s: detected new framerate(%f), video_rate(%d)",
          __MODULE_NAME__, m_framerate, m_video_rate);
      }
    }

    FrameQueuePop();
  }
}

void CDVDVideoCodecAmlogic::RemoveInfo(CDVDAmlogicInfo *info)
{
  CSingleLock lock(m_secure);
  m_inflight.erase(m_inflight.find(info));
}

CDVDAmlogicInfo::CDVDAmlogicInfo(CDVDVideoCodecAmlogic *codec, CAMLCodec *amlcodec, int omxPts, int amlDuration, uint32_t bufferIndex)
  : m_refs(0)
  , m_codec(codec)
  , m_amlCodec(amlcodec)
  , m_omxPts(omxPts)
  , m_amlDuration(amlDuration)
  , m_bufferIndex(bufferIndex)
  , m_rendered(false)
{
}

CDVDAmlogicInfo *CDVDAmlogicInfo::Retain()
{
  ++m_refs;
  return this;
}

long CDVDAmlogicInfo::Release()
{
  long count = --m_refs;
  if (count == 0)
  {
    if (m_codec)
      m_codec->RemoveInfo(this);
    delete this;
  }

  return count;
}

CAMLCodec *CDVDAmlogicInfo::getAmlCodec() const
{
  CSingleLock lock(m_section);

  return m_amlCodec;
}

void CDVDAmlogicInfo::invalidate()
{
  CSingleLock lock(m_section);

  m_codec = NULL;
  m_amlCodec = NULL;
}

