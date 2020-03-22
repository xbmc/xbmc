/*
 *  Copyright (C) 2010-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#if defined(TARGET_WINDOWS)
#endif

#include <interface/mmal/util/mmal_util.h>
#include <interface/mmal/util/mmal_default_components.h>
#include <interface/mmal/util/mmal_util_params.h>

#include "MMALCodec.h"

#include "ServiceBroker.h"
#include "DVDClock.h"
#include "DVDStreamInfo.h"
#include "cores/VideoPlayer/DVDCodecs/DVDCodecs.h"
#include "cores/VideoPlayer/DVDCodecs/DVDFactoryCodec.h"
#include "DVDVideoCodec.h"
#include "utils/log.h"
#include "utils/TimeUtils.h"
#include "settings/MediaSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "messaging/ApplicationMessenger.h"
#include "Application.h"
#include "guilib/GUIWindowManager.h"
#include "cores/VideoPlayer/VideoRenderers/RenderFlags.h"
#include "settings/DisplaySettings.h"
#include "settings/SettingsComponent.h"
#include "cores/VideoPlayer/VideoRenderers/RenderManager.h"
#include "cores/VideoPlayer/Interface/Addon/TimingConstants.h"

#include "platform/linux/RBP.h"

using namespace KODI::MESSAGING;
using namespace MMAL;

#define CLASSNAME "CMMALVideoBuffer"

#define VERBOSE 0

CMMALVideoBuffer::CMMALVideoBuffer(int id) : CMMALBuffer(id)
{
}

CMMALVideoBuffer::~CMMALVideoBuffer()
{
}

#undef CLASSNAME
#define CLASSNAME "CMMALVideo"

CMMALVideo::CMMALVideo(CProcessInfo &processInfo) : CDVDVideoCodec(processInfo)
{
  CLog::Log(LOGDEBUG, LOGVIDEO, "%s::%s %p", CLASSNAME, __func__, static_cast<void*>(this));

  m_decoded_width = 0;
  m_decoded_height = 0;
  m_decoded_aligned_width = 0;
  m_decoded_aligned_height = 0;

  m_finished = false;
  m_pFormatName = "mmal-xxxx";

  m_interlace_mode = MMAL_InterlaceProgressive;
  m_decoderPts = DVD_NOPTS_VALUE;
  m_demuxerPts = DVD_NOPTS_VALUE;

  m_dec = NULL;
  m_dec_input = NULL;
  m_dec_output = NULL;
  m_dec_input_pool = NULL;
  m_pool = nullptr;

  m_codingType = 0;

  m_es_format = mmal_format_alloc();
  m_preroll = true;
  m_speed = DVD_PLAYSPEED_NORMAL;
  m_fps = 0.0f;
  m_num_decoded = 0;
  m_codecControlFlags = 0;
  m_got_eos = false;
  m_packet_num = 0;
  m_packet_num_eos = ~0;
}

CMMALVideo::~CMMALVideo()
{
  CLog::Log(LOGDEBUG, LOGVIDEO, "%s::%s %p", CLASSNAME, __func__, static_cast<void*>(this));
  if (!m_finished)
    Dispose();

  CSingleLock lock(m_sharedSection);

  if (m_dec && m_dec->control && m_dec->control->is_enabled)
    mmal_port_disable(m_dec->control);

  if (m_dec_input && m_dec_input->is_enabled)
    mmal_port_disable(m_dec_input);

  m_dec_output = NULL;

  if (m_dec_input_pool)
    mmal_port_pool_destroy(m_dec_input, m_dec_input_pool);
  m_dec_input_pool = NULL;
  m_dec_input = NULL;

  m_dec = NULL;
  mmal_format_free(m_es_format);
  m_es_format = NULL;
}

void CMMALVideo::PortSettingsChanged(MMAL_PORT_T *port, MMAL_BUFFER_HEADER_T *buffer)
{
  CSingleLock lock(m_sharedSection);
  MMAL_EVENT_FORMAT_CHANGED_T *fmt = mmal_event_format_changed_get(buffer);
  mmal_format_copy(m_es_format, fmt->format);

  if (m_es_format->es->video.crop.width && m_es_format->es->video.crop.height)
  {
    if (m_es_format->es->video.par.num && m_es_format->es->video.par.den)
      m_aspect_ratio = (float)(m_es_format->es->video.par.num * m_es_format->es->video.crop.width) / (m_es_format->es->video.par.den * m_es_format->es->video.crop.height);
    m_decoded_width = m_es_format->es->video.crop.width;
    m_decoded_height = m_es_format->es->video.crop.height;
    m_decoded_aligned_width = m_es_format->es->video.width;
    m_decoded_aligned_height = m_es_format->es->video.height;

    m_processInfo.SetVideoDimensions(m_decoded_width, m_decoded_height);
    m_processInfo.SetVideoDAR(m_aspect_ratio);

    CLog::Log(LOGDEBUG, LOGVIDEO, "%s::%s format changed: %dx%d (%dx%d) %.2f", CLASSNAME, __func__, m_decoded_width, m_decoded_height, m_decoded_aligned_width, m_decoded_aligned_height, m_aspect_ratio);
  }
  else
    CLog::Log(LOGERROR, "%s::%s format changed: Unexpected %dx%d (%dx%d)", CLASSNAME, __func__, m_es_format->es->video.crop.width, m_es_format->es->video.crop.height, m_decoded_aligned_width, m_decoded_aligned_height);

  if (!change_dec_output_format())
    CLog::Log(LOGERROR, "%s::%s - change_dec_output_format() failed", CLASSNAME, __func__);
}

void CMMALVideo::dec_control_port_cb(MMAL_PORT_T *port, MMAL_BUFFER_HEADER_T *buffer)
{
  MMAL_STATUS_T status;

  if (buffer->cmd == MMAL_EVENT_ERROR)
  {
    status = (MMAL_STATUS_T)*(uint32_t *)buffer->data;
    CLog::Log(LOGERROR, "%s::%s Error (status=%x %s)", CLASSNAME, __func__, status, mmal_status_to_string(status));
  }
  else if (buffer->cmd == MMAL_EVENT_FORMAT_CHANGED)
  {
    CLog::Log(LOGDEBUG, LOGVIDEO, "%s::%s format changed", CLASSNAME, __func__);
    PortSettingsChanged(port, buffer);
  }
  else
    CLog::Log(LOGERROR, "%s::%s other (cmd:%x data:%x)", CLASSNAME, __func__, buffer->cmd, *(uint32_t *)buffer->data);

  mmal_buffer_header_release(buffer);
}

static void dec_control_port_cb_static(MMAL_PORT_T *port, MMAL_BUFFER_HEADER_T *buffer)
{
  CMMALVideo *mmal = reinterpret_cast<CMMALVideo*>(port->userdata);
  mmal->dec_control_port_cb(port, buffer);
}


void CMMALVideo::dec_input_port_cb(MMAL_PORT_T *port, MMAL_BUFFER_HEADER_T *buffer)
{
  CLog::Log(LOGDEBUG, LOGVIDEO, "%s::%s port:%p buffer %p, len %d cmd:%x", CLASSNAME, __func__,
            static_cast<void*>(port), static_cast<void*>(buffer), buffer->length, buffer->cmd);
  mmal_buffer_header_release(buffer);
  CSingleLock output_lock(m_output_mutex);
  m_output_cond.notifyAll();
}

static void dec_input_port_cb_static(MMAL_PORT_T *port, MMAL_BUFFER_HEADER_T *buffer)
{
  CMMALVideo *mmal = reinterpret_cast<CMMALVideo*>(port->userdata);
  mmal->dec_input_port_cb(port, buffer);
}


void CMMALVideo::dec_output_port_cb(MMAL_PORT_T *port, MMAL_BUFFER_HEADER_T *buffer)
{
  if (!(buffer->cmd == 0 && buffer->length > 0))
    {
      CLog::Log(LOGDEBUG, LOGVIDEO, "%s::%s port:%p buffer %p, len %d cmd:%x flags:%x", CLASSNAME,
                __func__, static_cast<void*>(port), static_cast<void*>(buffer), buffer->length,
                buffer->cmd, buffer->flags);
    }

  bool kept = false;
  CMMALVideoBuffer *omvb = (CMMALVideoBuffer *)buffer->user_data;

  assert(!(buffer->flags & MMAL_BUFFER_HEADER_FLAG_TRANSMISSION_FAILED));
  if (buffer->cmd == 0)
  {
    if (buffer->length > 0)
    {
      if (buffer->pts != MMAL_TIME_UNKNOWN)
        m_decoderPts = buffer->pts;
      else if (buffer->dts != MMAL_TIME_UNKNOWN)
        m_decoderPts = buffer->dts;

      assert(!(buffer->flags & MMAL_BUFFER_HEADER_FLAG_DECODEONLY));
      assert(omvb);
      assert(omvb->mmal_buffer == buffer);
      bool wanted = true;
      // we don't keep up when running at 60fps in the background so switch to half rate
      if (m_fps > 40.0f && !CServiceBroker::GetWinSystem()->GetGfxContext().IsFullScreenVideo() && !(m_num_decoded & 1))
        wanted = false;
      if ((buffer->flags & MMAL_BUFFER_HEADER_FLAG_CORRUPTED))
        wanted = false;
      m_num_decoded++;
      CLog::Log(LOGDEBUG, LOGVIDEO,
                "%s::%s - omvb:%p mmal:%p len:%u dts:%.3f pts:%.3f flags:%x:%x pool:%p %dx%d "
                "(%dx%d) %dx%d (%dx%d) enc:%.4s",
                CLASSNAME, __func__, static_cast<void*>(buffer), static_cast<void*>(omvb),
                buffer->length, buffer->dts * 1e-6, buffer->pts * 1e-6, buffer->flags,
                buffer->type->video.flags, static_cast<void*>(m_pool.get()), omvb->Width(),
                omvb->Height(), omvb->AlignedWidth(), omvb->AlignedHeight(), m_decoded_width,
                m_decoded_height, m_decoded_aligned_width, m_decoded_aligned_height,
                (char*)&omvb->Encoding());
      if (wanted)
      {
        std::shared_ptr<CMMALPool> pool = std::dynamic_pointer_cast<CMMALPool>(m_pool);
        if (pool)
          pool->Configure(AV_PIX_FMT_NONE, m_decoded_width, m_decoded_height, m_decoded_aligned_width, m_decoded_aligned_height, 128);
        omvb->m_aspect_ratio = m_aspect_ratio;
        {
          CSingleLock output_lock(m_output_mutex);
          m_output_ready.push(omvb);
          m_output_cond.notifyAll();
        }
        kept = true;
      }
    }
    if (buffer->flags & MMAL_BUFFER_HEADER_FLAG_EOS)
    {
      CSingleLock output_lock(m_output_mutex);
      m_got_eos = true;
      m_output_cond.notifyAll();
    }
  }
  else if (buffer->cmd == MMAL_EVENT_FORMAT_CHANGED)
  {
    PortSettingsChanged(port, buffer);
  }
  if (!kept)
  {
    if (omvb)
      omvb->Release();
    else
      mmal_buffer_header_release(buffer);
  }
}

static void dec_output_port_cb_static(MMAL_PORT_T *port, MMAL_BUFFER_HEADER_T *buffer)
{
  CMMALVideo *mmal = reinterpret_cast<CMMALVideo*>(port->userdata);
  mmal->dec_output_port_cb(port, buffer);
}

bool CMMALVideo::change_dec_output_format()
{
  CSingleLock lock(m_sharedSection);
  MMAL_STATUS_T status;
  CLog::Log(LOGDEBUG, LOGVIDEO, "%s::%s", CLASSNAME, __func__);

  MMAL_PARAMETER_VIDEO_INTERLACE_TYPE_T interlace_type = {{ MMAL_PARAMETER_VIDEO_INTERLACE_TYPE, sizeof( interlace_type )}};
  status = mmal_port_parameter_get( m_dec_output, &interlace_type.hdr );

  if (status == MMAL_SUCCESS)
  {
    if (m_interlace_mode != interlace_type.eMode)
    {
      CLog::Log(LOGDEBUG, LOGVIDEO, "%s::%s Interlace mode %d->%d", CLASSNAME, __func__, m_interlace_mode, interlace_type.eMode);
      m_interlace_mode = interlace_type.eMode;
    }
  }
  else
    CLog::Log(LOGERROR, "%s::%s Failed to query interlace type on %s (status=%x %s)", CLASSNAME, __func__, m_dec_output->name, status, mmal_status_to_string(status));

  mmal_format_copy(m_dec_output->format, m_es_format);

  status = mmal_port_parameter_set_boolean(m_dec_output, MMAL_PARAMETER_ZERO_COPY, MMAL_TRUE);
  if (status != MMAL_SUCCESS)
    CLog::Log(LOGERROR, "%s::%s Failed to enable zero copy mode on %s (status=%x %s)", CLASSNAME, __func__, m_dec_output->name, status, mmal_status_to_string(status));

  status = mmal_port_format_commit(m_dec_output);
  if (status != MMAL_SUCCESS)
  {
    CLog::Log(LOGERROR, "%s::%s Failed to commit decoder output port (status=%x %s)", CLASSNAME, __func__, status, mmal_status_to_string(status));
    return false;
  }
  return true;
}

bool CMMALVideo::SendCodecConfigData()
{
  CSingleLock lock(m_sharedSection);
  MMAL_STATUS_T status;
  if (!m_dec_input_pool || !m_hints.extrasize)
    return true;
  // send code config data
  MMAL_BUFFER_HEADER_T *buffer = mmal_queue_timedwait(m_dec_input_pool->queue, 500);
  if (!buffer)
  {
    CLog::Log(LOGERROR, "%s::%s - mmal_queue_get failed", CLASSNAME, __func__);
    return false;
  }

  mmal_buffer_header_reset(buffer);
  buffer->cmd = 0;
  buffer->length = std::min(m_hints.extrasize, buffer->alloc_size);
  memcpy(buffer->data, m_hints.extradata, buffer->length);
  buffer->flags = MMAL_BUFFER_HEADER_FLAG_FRAME_END | MMAL_BUFFER_HEADER_FLAG_CONFIG;
  CLog::Log(LOGDEBUG, LOGVIDEO, "%s::%s - %-8p %-6d flags:%x", CLASSNAME, __func__, static_cast<void*>(buffer),
            buffer->length, buffer->flags);
  status = mmal_port_send_buffer(m_dec_input, buffer);
  if (status != MMAL_SUCCESS)
  {
    CLog::Log(LOGERROR, "%s::%s Failed send buffer to decoder input port (status=%x %s)", CLASSNAME, __func__, status, mmal_status_to_string(status));
    return false;
  }
  return true;
}

bool CMMALVideo::Open(CDVDStreamInfo &hints, CDVDCodecOptions &options)
{
  CSingleLock lock(m_sharedSection);
  CLog::Log(LOGDEBUG, LOGVIDEO, "%s::%s usemmal:%d options:%x %dx%d", CLASSNAME, __func__, CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_VIDEOPLAYER_USEMMAL), hints.codecOptions, hints.width, hints.height);

  // This occurs at start of m2ts files before streams have been fully identified - just ignore
  if (!hints.width)
    return false;
  // we always qualify even if DVDFactoryCodec does this too.
  if (!CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_VIDEOPLAYER_USEMMAL) || (hints.codecOptions & CODEC_FORCE_SOFTWARE))
    return false;

  std::list<EINTERLACEMETHOD> deintMethods;
  deintMethods.push_back(EINTERLACEMETHOD::VS_INTERLACEMETHOD_AUTO);
  deintMethods.push_back(EINTERLACEMETHOD::VS_INTERLACEMETHOD_MMAL_ADVANCED);
  deintMethods.push_back(EINTERLACEMETHOD::VS_INTERLACEMETHOD_MMAL_ADVANCED_HALF);
  deintMethods.push_back(EINTERLACEMETHOD::VS_INTERLACEMETHOD_MMAL_BOB);
  deintMethods.push_back(EINTERLACEMETHOD::VS_INTERLACEMETHOD_MMAL_BOB_HALF);
  m_processInfo.UpdateDeinterlacingMethods(deintMethods);

  m_hints = hints;
  MMAL_STATUS_T status;

  m_decoded_width = hints.width;
  m_decoded_height = hints.height;

  m_decoded_aligned_width = ALIGN_UP(m_decoded_width, 32);
  m_decoded_aligned_height = ALIGN_UP(m_decoded_height, 16);

  // use aspect in stream if available
  if (m_hints.forced_aspect)
    m_aspect_ratio = m_hints.aspect;
  else
    m_aspect_ratio = 0.0;

  switch (hints.codec)
  {
    case AV_CODEC_ID_H264:
      // H.264
      switch (hints.profile)
      {
        // Cannot hardware decode Hi10P without artifacts - switch to software on Pi2/Pi3
        case FF_PROFILE_H264_HIGH_10:
        case FF_PROFILE_H264_HIGH_10_INTRA:
          if (g_RBP.RaspberryPiVersion() > 1)
            return false;
      }
      m_codingType = MMAL_ENCODING_H264;
      m_pFormatName = "mmal-h264";
      if (CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_VIDEOPLAYER_SUPPORTMVC))
      {
        m_codingType = MMAL_ENCODING_MVC;
        m_pFormatName= "mmal-mvc";
      }
    break;
    case AV_CODEC_ID_H263:
    case AV_CODEC_ID_MPEG4:
      // MPEG-4, DivX 4/5 and Xvid compatible
      m_codingType = MMAL_ENCODING_MP4V;
      m_pFormatName = "mmal-mpeg4";
    break;
    case AV_CODEC_ID_MPEG1VIDEO:
    case AV_CODEC_ID_MPEG2VIDEO:
      // MPEG-2
      m_codingType = MMAL_ENCODING_MP2V;
      m_pFormatName = "mmal-mpeg2";
    break;
    case AV_CODEC_ID_VP6:
      // this form is encoded upside down
      // fall through
    case AV_CODEC_ID_VP6F:
    case AV_CODEC_ID_VP6A:
      // VP6
      m_codingType = MMAL_ENCODING_VP6;
      m_pFormatName = "mmal-vp6";
    break;
    case AV_CODEC_ID_VP8:
      // VP8
      m_codingType = MMAL_ENCODING_VP8;
      m_pFormatName = "mmal-vp8";
    break;
    case AV_CODEC_ID_THEORA:
      // theora
      m_codingType = MMAL_ENCODING_THEORA;
      m_pFormatName = "mmal-theora";
    break;
    case AV_CODEC_ID_MJPEG:
    case AV_CODEC_ID_MJPEGB:
      // mjpg
      m_codingType = MMAL_ENCODING_MJPEG;
      m_pFormatName = "mmal-mjpg";
    break;
    case AV_CODEC_ID_VC1:
    case AV_CODEC_ID_WMV3:
      // VC-1, WMV9
      m_codingType = MMAL_ENCODING_WVC1;
      m_pFormatName = "mmal-vc1";
      break;
    default:
      CLog::Log(LOGERROR, "%s::%s : Video codec unknown: %x", CLASSNAME, __func__, hints.codec);
      return false;
    break;
  }

  if ( (m_codingType == MMAL_ENCODING_MP2V && !g_RBP.GetCodecMpg2() ) ||
       (m_codingType == MMAL_ENCODING_WVC1 && !g_RBP.GetCodecWvc1() ) )
  {
    CLog::Log(LOGWARNING, "%s::%s Codec %s is not supported", CLASSNAME, __func__, m_pFormatName);
    return false;
  }

  /* Create video component with attached pool */
  m_pool = std::make_shared<CMMALPool>(MMAL_COMPONENT_DEFAULT_VIDEO_DECODER, false, MMAL_NUM_OUTPUT_BUFFERS, 128, MMAL_ENCODING_OPAQUE, MMALStateHWDec);
  if (!m_pool)
  {
    CLog::Log(LOGERROR, "%s::%s Failed to create pool for video output", CLASSNAME, __func__);
    return false;
  }
  std::shared_ptr<CMMALPool> pool = std::dynamic_pointer_cast<CMMALPool>(m_pool);
  pool->SetProcessInfo(&m_processInfo);
  m_dec = pool->GetComponent();

  m_dec->control->userdata = (struct MMAL_PORT_USERDATA_T *)this;
  status = mmal_port_enable(m_dec->control, dec_control_port_cb_static);
  if (status != MMAL_SUCCESS)
  {
    CLog::Log(LOGERROR, "%s::%s Failed to enable decoder control port %s (status=%x %s)", CLASSNAME, __func__, MMAL_COMPONENT_DEFAULT_VIDEO_DECODER, status, mmal_status_to_string(status));
    return false;
  }

  m_dec_input = m_dec->input[0];

  m_dec_input->format->type = MMAL_ES_TYPE_VIDEO;
  m_dec_input->format->encoding = m_codingType;
  if (m_decoded_width && m_decoded_height)
  {
    m_dec_input->format->es->video.crop.width = m_decoded_width;
    m_dec_input->format->es->video.crop.height = m_decoded_height;

    m_dec_input->format->es->video.width = m_decoded_aligned_width;
    m_dec_input->format->es->video.height = m_decoded_aligned_width;
  }
  if (hints.fpsrate > 0 && hints.fpsscale > 0)
  {
    m_dec_input->format->es->video.frame_rate.num = hints.fpsrate;
    m_dec_input->format->es->video.frame_rate.den = hints.fpsscale;
    m_fps = hints.fpsrate / hints.fpsscale;
  }
  else
    m_fps = 0.0f;
  m_dec_input->format->flags |= MMAL_ES_FORMAT_FLAG_FRAMED;

  status = mmal_port_parameter_set_boolean(m_dec_input, MMAL_PARAMETER_VIDEO_DECODE_ERROR_CONCEALMENT, MMAL_FALSE);
  if (status != MMAL_SUCCESS)
    CLog::Log(LOGERROR, "%s::%s Failed to disable error concealment on %s (status=%x %s)", CLASSNAME, __func__, m_dec_input->name, status, mmal_status_to_string(status));

  // we need an extra buffer when seeking as a picture remains on screen from old seek point
  status = mmal_port_parameter_set_uint32(m_dec_input, MMAL_PARAMETER_EXTRA_BUFFERS, GetAllowedReferences() + 1);
  if (status != MMAL_SUCCESS)
    CLog::Log(LOGERROR, "%s::%s Failed to enable extra buffers on %s (status=%x %s)", CLASSNAME, __func__, m_dec_input->name, status, mmal_status_to_string(status));

  status = mmal_port_parameter_set_uint32(m_dec_input, MMAL_PARAMETER_VIDEO_INTERPOLATE_TIMESTAMPS, 1);
  if (status != MMAL_SUCCESS)
    CLog::Log(LOGERROR, "%s::%s Failed to disable interpolate timestamps mode on %s (status=%x %s)", CLASSNAME, __func__, m_dec_input->name, status, mmal_status_to_string(status));

  // limit number of callback structures in video_decode to reduce latency. Too low and video hangs.
  // negative numbers have special meaning. -1=size of DPB -2=size of DPB+1
  status = mmal_port_parameter_set_uint32(m_dec_input, MMAL_PARAMETER_VIDEO_MAX_NUM_CALLBACKS, -5);
  if (status != MMAL_SUCCESS)
    CLog::Log(LOGERROR, "%s::%s Failed to configure max num callbacks on %s (status=%x %s)", CLASSNAME, __func__, m_dec_input->name, status, mmal_status_to_string(status));

  status = mmal_port_parameter_set_boolean(m_dec_input, MMAL_PARAMETER_ZERO_COPY,  MMAL_TRUE);
  if (status != MMAL_SUCCESS)
    CLog::Log(LOGERROR, "%s::%s Failed to enable zero copy mode on %s (status=%x %s)", CLASSNAME, __func__, m_dec_input->name, status, mmal_status_to_string(status));

  status = mmal_port_format_commit(m_dec_input);
  if (status != MMAL_SUCCESS)
  {
    CLog::Log(LOGERROR, "%s::%s Failed to commit format for decoder input port %s (status=%x %s)", CLASSNAME, __func__, m_dec_input->name, status, mmal_status_to_string(status));
    return false;
  }
  // use a small number of large buffers to keep latency under control
  m_dec_input->buffer_size = 1024*1024;
  m_dec_input->buffer_num = 2;

  m_dec_input->userdata = (struct MMAL_PORT_USERDATA_T *)this;
  status = mmal_port_enable(m_dec_input, dec_input_port_cb_static);
  if (status != MMAL_SUCCESS)
  {
    CLog::Log(LOGERROR, "%s::%s Failed to enable decoder input port %s (status=%x %s)", CLASSNAME, __func__, m_dec_input->name, status, mmal_status_to_string(status));
    return false;
  }

  m_dec_output = m_dec->output[0];

  mmal_format_copy(m_es_format, m_dec_output->format);

  status = mmal_port_format_commit(m_dec_output);
  if (status != MMAL_SUCCESS)
  {
    CLog::Log(LOGERROR, "%s::%s Failed to commit decoder output format (status=%x %s)", CLASSNAME, __func__, status, mmal_status_to_string(status));
    return false;
  }

  m_dec_output->buffer_size = m_dec_output->buffer_size_min;
  m_dec_output->buffer_num = MMAL_NUM_OUTPUT_BUFFERS;
  m_dec_output->userdata = (struct MMAL_PORT_USERDATA_T *)this;
  status = mmal_port_enable(m_dec_output, dec_output_port_cb_static);
  if (status != MMAL_SUCCESS)
  {
    CLog::Log(LOGERROR, "%s::%s Failed to enable decoder output port (status=%x %s)", CLASSNAME, __func__, status, mmal_status_to_string(status));
    return false;
  }

  status = mmal_component_enable(m_dec);
  if (status != MMAL_SUCCESS)
  {
    CLog::Log(LOGERROR, "%s::%s Failed to enable decoder component %s (status=%x %s)", CLASSNAME, __func__, m_dec->name, status, mmal_status_to_string(status));
    return false;
  }

  m_dec_input_pool = mmal_port_pool_create(m_dec_input, m_dec_input->buffer_num, m_dec_input->buffer_size);
  if (!m_dec_input_pool)
  {
    CLog::Log(LOGERROR, "%s::%s Failed to create pool for decoder input port (status=%x %s)", CLASSNAME, __func__, status, mmal_status_to_string(status));
    return false;
  }

  if (!SendCodecConfigData())
    return false;

  if (pool)
    pool->Prime();
  m_preroll = !m_hints.stills;
  m_speed = DVD_PLAYSPEED_NORMAL;

  m_processInfo.SetVideoDecoderName(m_pFormatName, true);
  m_processInfo.SetVideoDimensions(m_decoded_width, m_decoded_height);
  m_processInfo.SetVideoDAR(m_aspect_ratio);

  return true;
}

void CMMALVideo::Dispose()
{
  CSingleLock lock(m_sharedSection);
  m_finished = true;
  Reset();
}

bool CMMALVideo::AddData(const DemuxPacket &packet)
{
  uint8_t* pData = packet.pData;
  int iSize = packet.iSize;
  CSingleLock lock(m_sharedSection);
  //if (CServiceBroker::GetLogging().CanLogComponent(LOGVIDEO))
  //  CLog::Log(LOGDEBUG, "%s::%s - %-8p %-6d dts:%.3f pts:%.3f ready_queue(%d)",
  //    CLASSNAME, __func__, pData, iSize, dts == DVD_NOPTS_VALUE ? 0.0 : packet.dts*1e-6, packet.pts == DVD_NOPTS_VALUE ? 0.0 : packet.pts*1e-6, m_output_ready.size());

  MMAL_BUFFER_HEADER_T *buffer;
  MMAL_STATUS_T status;
  assert(pData != nullptr && iSize > 0); // no longer valid

  while (iSize > 0)
  {
    // 500ms timeout
    lock.Leave();
    buffer = mmal_queue_timedwait(m_dec_input_pool->queue, 500);
    if (!buffer)
    {
      CLog::Log(LOGERROR, "%s::%s - mmal_queue_get failed", CLASSNAME, __func__);
      return false;
    }
    lock.Enter();

    mmal_buffer_header_reset(buffer);
    buffer->cmd = 0;
    buffer->pts = packet.pts == DVD_NOPTS_VALUE ? MMAL_TIME_UNKNOWN : packet.pts;
    buffer->dts = packet.dts == DVD_NOPTS_VALUE ? MMAL_TIME_UNKNOWN : packet.dts;
    if (m_hints.ptsinvalid) buffer->pts = MMAL_TIME_UNKNOWN;
    buffer->length = (uint32_t)iSize > buffer->alloc_size ? buffer->alloc_size : (uint32_t)iSize;
    // set a flag so we can identify primary frames from generated frames (deinterlace)
    buffer->flags = 0;
    if (m_codecControlFlags & DVD_CODEC_CTRL_DROP_ANY)
      buffer->flags |= MMAL_BUFFER_HEADER_FLAG_USER3;

    if (pData)
      memcpy(buffer->data, pData, buffer->length);
    iSize -= buffer->length;
    pData += buffer->length;

    if (iSize == 0)
    {
      m_packet_num++;
      buffer->flags |= MMAL_BUFFER_HEADER_FLAG_FRAME_END;
    }
    CLog::Log(LOGDEBUG, LOGVIDEO,
              "%s::%s - %-8p %-6d/%-6d dts:%.3f pts:%.3f flags:%x ready_queue(%d)", CLASSNAME,
              __func__, static_cast<void*>(buffer), buffer->length, iSize,
              packet.dts == DVD_NOPTS_VALUE ? 0.0 : packet.dts * 1e-6,
              packet.pts == DVD_NOPTS_VALUE ? 0.0 : packet.pts * 1e-6, buffer->flags,
              m_output_ready.size());
    status = mmal_port_send_buffer(m_dec_input, buffer);
    if (status != MMAL_SUCCESS)
    {
      CLog::Log(LOGERROR, "%s::%s Failed send buffer to decoder input port (status=%x %s)", CLASSNAME, __func__, status, mmal_status_to_string(status));
      return false;
    }
  }
  if (packet.pts != DVD_NOPTS_VALUE)
    m_demuxerPts = packet.pts;
  else if (packet.dts != DVD_NOPTS_VALUE)
    m_demuxerPts = packet.dts;

  if (m_demuxerPts != DVD_NOPTS_VALUE && m_decoderPts == DVD_NOPTS_VALUE)
    m_decoderPts = m_demuxerPts;

  return true;
}

void CMMALVideo::Reset(void)
{
  CSingleLock lock(m_sharedSection);
  CLog::Log(LOGDEBUG, LOGVIDEO, "%s::%s", CLASSNAME, __func__);

  if (m_dec_input && m_dec_input->is_enabled)
    mmal_port_disable(m_dec_input);
  if (m_dec_output && m_dec_output->is_enabled)
    mmal_port_disable(m_dec_output);
  if (!m_finished)
  {
    if (m_dec_input)
      mmal_port_enable(m_dec_input, dec_input_port_cb_static);
    if (m_dec_output)
      mmal_port_enable(m_dec_output, dec_output_port_cb_static);
  }
  // blow all ready video frames
  while (1)
  {
    CMMALVideoBuffer *buffer = NULL;
    {
      CSingleLock output_lock(m_output_mutex);
      // fetch a output buffer and pop it off the ready list
      if (!m_output_ready.empty())
      {
        buffer = m_output_ready.front();
        m_output_ready.pop();
      }
      m_output_cond.notifyAll();
    }
    if (buffer)
      buffer->Release();
    else
      break;
  }

  if (!m_finished)
  {
    SendCodecConfigData();
    std::shared_ptr<CMMALPool> pool = std::dynamic_pointer_cast<CMMALPool>(m_pool);
    if (pool)
      pool->Prime();
  }
  m_decoderPts = DVD_NOPTS_VALUE;
  m_demuxerPts = DVD_NOPTS_VALUE;
  m_codecControlFlags = 0;
  m_num_decoded = 0;
  m_got_eos = false;
  m_packet_num = 0;
  m_packet_num_eos = ~0;
  m_preroll = !m_hints.stills && (m_speed == DVD_PLAYSPEED_NORMAL || m_speed == DVD_PLAYSPEED_PAUSE);
}

void CMMALVideo::SetSpeed(int iSpeed)
{
  CLog::Log(LOGDEBUG, LOGVIDEO, "%s::%s %d->%d", CLASSNAME, __func__, m_speed, iSpeed);

  m_speed = iSpeed;
}

CDVDVideoCodec::VCReturn CMMALVideo::GetPicture(VideoPicture* picture)
{
  CSingleLock lock(m_sharedSection);
  MMAL_STATUS_T status;
  bool drain = (m_codecControlFlags & DVD_CODEC_CTRL_DRAIN) ? true : false;
  bool send_eos = drain && !m_got_eos && m_packet_num_eos != m_packet_num;

  // we don't get an EOS response if no packets have been sent
  if (!drain)
    m_got_eos = false;
  else if (m_packet_num == 0 && send_eos)
    m_got_eos = true;

  if (send_eos && !m_got_eos)
  {
    MMAL_BUFFER_HEADER_T *buffer;
    // 500ms timeout
    lock.Leave();
    buffer = mmal_queue_timedwait(m_dec_input_pool->queue, 500);
    lock.Enter();

    if (buffer)
    {
      mmal_buffer_header_reset(buffer);
      buffer->cmd = 0;
      buffer->flags |= MMAL_BUFFER_HEADER_FLAG_FRAME_END | MMAL_BUFFER_HEADER_FLAG_EOS;
      m_packet_num_eos = m_packet_num;
      m_got_eos = false;
      status = mmal_port_send_buffer(m_dec_input, buffer);
      if (status != MMAL_SUCCESS)
      {
        CLog::Log(LOGERROR, "%s::%s Failed send buffer to decoder input port (status=%x %s)", CLASSNAME, __func__, status, mmal_status_to_string(status));
        return VC_ERROR;
      }
      else
        CLog::Log(LOGDEBUG, LOGVIDEO, "%s::%s - Send EOS (%d, %d, %d)", CLASSNAME, __func__, m_got_eos, m_packet_num, m_packet_num_eos);
    }
    else
    {
      CLog::Log(LOGWARNING, "%s::%s - mmal_queue_get failed", CLASSNAME, __func__);
      // lets assume decoder has returned all it will
      m_got_eos = true;
    }
  }

  // we've built up quite a lot of data in decoder - try to throttle it
  double queued = m_decoderPts != DVD_NOPTS_VALUE && m_demuxerPts != DVD_NOPTS_VALUE ? m_demuxerPts - m_decoderPts : 0.0;
  bool full = queued > DVD_MSEC_TO_TIME(1000);
  CDVDVideoCodec::VCReturn ret = CDVDVideoCodec::VC_NONE;

  CMMALVideoBuffer *buffer = nullptr;
  XbmcThreads::EndTime delay(500);
  while (ret == CDVDVideoCodec::VC_NONE && !delay.IsTimePast())
  {
    CSingleLock output_lock(m_output_mutex);
    unsigned int pics = m_output_ready.size();
    if (m_preroll && (pics >= GetAllowedReferences() || drain))
      m_preroll = false;
    if (pics > 0 && !m_preroll)
    {
      // fetch a output buffer and pop it off the ready list
      buffer = m_output_ready.front();
      m_output_ready.pop();
      m_output_cond.notifyAll();
      ret = CDVDVideoCodec::VC_PICTURE;
    }
    else if (m_got_eos)
      ret = CDVDVideoCodec::VC_EOF;
    else if ((m_preroll || pics <= 1) && mmal_queue_length(m_dec_input_pool->queue) > 0)
      ret = CDVDVideoCodec::VC_BUFFER;
    if (ret == CDVDVideoCodec::VC_NONE)
    {
      // otherwise we busy spin
      lock.Leave();
      m_output_cond.wait(output_lock, delay.MillisLeft());
      lock.Enter();
    }
  }

  if (ret == CDVDVideoCodec::VC_PICTURE)
  {
    assert(buffer && buffer->mmal_buffer);
    if (picture->videoBuffer)
      picture->videoBuffer->Release();
    picture->videoBuffer = dynamic_cast<CVideoBuffer*>(buffer);
    assert(picture->videoBuffer);
    picture->color_range  = 0;
    picture->iWidth = buffer->Width() ? buffer->Width() : m_decoded_width;
    picture->iHeight = buffer->Height() ? buffer->Height() : m_decoded_height;
    picture->iDisplayWidth  = picture->iWidth;
    picture->iDisplayHeight = picture->iHeight;
    //CLog::Log(LOGDEBUG, "%s::%s -  %dx%d %dx%d %dx%d %dx%d %f,%f", CLASSNAME, __func__, picture->iWidth, picture->iHeight, picture->iDisplayWidth, picture->iDisplayHeight, m_decoded_width, m_decoded_height, buffer->Width(), buffer->Height(), buffer->m_aspect_ratio, m_hints.aspect);

    if (buffer->m_aspect_ratio > 0.0)
    {
      picture->iDisplayWidth  = ((int)lrint(picture->iHeight * buffer->m_aspect_ratio)) & -3;
      if (picture->iDisplayWidth > picture->iWidth)
      {
        picture->iDisplayWidth  = picture->iWidth;
        picture->iDisplayHeight = ((int)lrint(picture->iWidth / buffer->m_aspect_ratio)) & -3;
      }
    }

    // timestamp is in microseconds
    picture->dts = buffer->mmal_buffer->dts == MMAL_TIME_UNKNOWN ? DVD_NOPTS_VALUE : buffer->mmal_buffer->dts;
    picture->pts = buffer->mmal_buffer->pts == MMAL_TIME_UNKNOWN ? DVD_NOPTS_VALUE : buffer->mmal_buffer->pts;
    picture->iRepeatPicture = 0;
    picture->iFlags  = 0;
    if (buffer->mmal_buffer->flags & MMAL_BUFFER_HEADER_FLAG_USER3)
      picture->iFlags |= DVP_FLAG_DROPPED;
    CLog::Log(LOGINFO, LOGVIDEO,
              "%s::%s dts:%.3f pts:%.3f flags:%x:%x MMALBuffer:%p mmal_buffer:%p", CLASSNAME,
              __func__, picture->dts == DVD_NOPTS_VALUE ? 0.0 : picture->dts * 1e-6,
              picture->pts == DVD_NOPTS_VALUE ? 0.0 : picture->pts * 1e-6, picture->iFlags,
              buffer->mmal_buffer->flags, static_cast<void*>(buffer),
              static_cast<void*>(buffer->mmal_buffer));
    assert(!(buffer->mmal_buffer->flags & MMAL_BUFFER_HEADER_FLAG_DECODEONLY));
    buffer->mmal_buffer->flags &= ~MMAL_BUFFER_HEADER_FLAG_USER3;
    buffer->m_stills = m_hints.stills;
  }

  if (ret == CDVDVideoCodec::VC_NONE)
    CLog::Log(LOGWARNING, "%s::%s - ret(%x) pics(%d) inputs(%d) slept(%2d) queued(%.2f) (%.2f:%.2f) full(%d) flags(%x) preroll(%d) eos(%d %d/%d)", CLASSNAME, __func__, ret, m_output_ready.size(), mmal_queue_length(m_dec_input_pool->queue), 500-delay.MillisLeft(), queued*1e-6, m_demuxerPts*1e-6, m_decoderPts*1e-6, full, m_codecControlFlags,  m_preroll, m_got_eos, m_packet_num, m_packet_num_eos);
  else
    CLog::Log(LOGDEBUG, LOGVIDEO, "%s::%s - ret(%x) pics(%d) inputs(%d) slept(%2d) queued(%.2f) (%.2f:%.2f) full(%d) flags(%x) preroll(%d) eos(%d %d/%d)", CLASSNAME, __func__, ret, m_output_ready.size(), mmal_queue_length(m_dec_input_pool->queue), 500-delay.MillisLeft(), queued*1e-6, m_demuxerPts*1e-6, m_decoderPts*1e-6, full, m_codecControlFlags,  m_preroll, m_got_eos, m_packet_num, m_packet_num_eos);

  return ret;
}

void CMMALVideo::SetCodecControl(int flags)
{
  CSingleLock lock(m_sharedSection);
  if (m_codecControlFlags != flags)
    CLog::Log(LOGDEBUG, LOGVIDEO, "%s::%s %x->%x", CLASSNAME, __func__, m_codecControlFlags, flags);
  m_codecControlFlags = flags;
}

CDVDVideoCodec* CMMALVideo::Create(CProcessInfo &processInfo)
 {
   return new CMMALVideo(processInfo);
 }

void CMMALVideo::Register()
{
  CDVDFactoryCodec::RegisterHWVideoCodec("mmal", CMMALVideo::Create);
}
