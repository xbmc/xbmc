/*
 *      Copyright (C) 2010-2013 Team XBMC
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

#if (defined HAVE_CONFIG_H) && (!defined TARGET_WINDOWS)
  #include "config.h"
#elif defined(TARGET_WINDOWS)
#include "system.h"
#endif

#include "MMALCodec.h"

#include "DVDClock.h"
#include "DVDStreamInfo.h"
#include "windowing/WindowingFactory.h"
#include "DVDVideoCodec.h"
#include "utils/log.h"
#include "utils/TimeUtils.h"
#include "settings/Settings.h"
#include "settings/MediaSettings.h"
#include "ApplicationMessenger.h"
#include "Application.h"
#include "threads/Atomics.h"
#include "guilib/GUIWindowManager.h"
#include "cores/VideoRenderers/RenderFlags.h"
#include "settings/DisplaySettings.h"
#include "cores/VideoRenderers/RenderManager.h"

#include "linux/RBP.h"

#ifdef _DEBUG
#define MMAL_DEBUG_VERBOSE
#endif

#define CLASSNAME "CMMALVideoBuffer"

CMMALVideoBuffer::CMMALVideoBuffer(CMMALVideo *omv)
    : m_omv(omv), m_refs(0)
{
#if defined(MMAL_DEBUG_VERBOSE)
  CLog::Log(LOGDEBUG, "%s::%s %p", CLASSNAME, __func__, this);
#endif
  mmal_buffer = NULL;
  width = 0;
  height = 0;
  index = 0;
  m_aspect_ratio = 0.0f;
  m_changed_count = 0;
  dts = DVD_NOPTS_VALUE;
}

CMMALVideoBuffer::~CMMALVideoBuffer()
{
#if defined(MMAL_DEBUG_VERBOSE)
  CLog::Log(LOGDEBUG, "%s::%s %p", CLASSNAME, __func__, this);
#endif
}


CMMALVideoBuffer* CMMALVideoBuffer::Acquire()
{
  long count = AtomicIncrement(&m_refs);
  #if defined(MMAL_DEBUG_VERBOSE)
  CLog::Log(LOGDEBUG, "%s::%s %p (%p) ref:%ld", CLASSNAME, __func__, this, mmal_buffer, count);
  #endif
  (void)count;
  return this;
}

long CMMALVideoBuffer::Release()
{
  long count = AtomicDecrement(&m_refs);
#if defined(MMAL_DEBUG_VERBOSE)
CLog::Log(LOGDEBUG, "%s::%s %p (%p) ref:%ld", CLASSNAME, __func__, this, mmal_buffer, count);
#endif
  if (count == 0)
  {
    m_omv->ReleaseBuffer(this);
  }
  return count;
}

#undef CLASSNAME
#define CLASSNAME "CMMALVideo"

CMMALVideo::CMMALVideo()
{
  #if defined(MMAL_DEBUG_VERBOSE)
  CLog::Log(LOGDEBUG, "%s::%s %p", CLASSNAME, __func__, this);
  #endif
  pthread_mutex_init(&m_output_mutex, NULL);

  m_drop_state = false;
  m_decoded_width = 0;
  m_decoded_height = 0;

  m_finished = false;
  m_pFormatName = "mmal-xxxx";

  m_interlace_mode = MMAL_InterlaceProgressive;
  m_interlace_method = VS_INTERLACEMETHOD_NONE;
  m_startframe = false;
  m_decoderPts = DVD_NOPTS_VALUE;
  m_droppedPics = 0;
  m_decode_frame_number = 1;

  m_dec = NULL;
  m_dec_input = NULL;
  m_dec_output = NULL;
  m_dec_input_pool = NULL;
  m_dec_output_pool = NULL;

  m_deint = NULL;
  m_deint_connection = NULL;

  m_codingType = 0;

  m_changed_count = 0;
  m_changed_count_dec = 0;
  m_output_busy = 0;
  m_demux_queue_length = 0;
  m_es_format = mmal_format_alloc();
}

CMMALVideo::~CMMALVideo()
{
  #if defined(MMAL_DEBUG_VERBOSE)
  CLog::Log(LOGDEBUG, "%s::%s %p", CLASSNAME, __func__, this);
  #endif
  assert(m_finished);
  Reset();

  pthread_mutex_destroy(&m_output_mutex);

  if (m_deint && m_deint->control && m_deint->control->is_enabled)
    mmal_port_disable(m_deint->control);

  if (m_dec && m_dec->control && m_dec->control->is_enabled)
    mmal_port_disable(m_dec->control);

  if (m_dec_input && m_dec_input->is_enabled)
    mmal_port_disable(m_dec_input);
  m_dec_input = NULL;

  if (m_dec_output && m_dec_output->is_enabled)
    mmal_port_disable(m_dec_output);
  m_dec_output = NULL;

  if (m_deint_connection)
    mmal_connection_destroy(m_deint_connection);
  m_deint_connection = NULL;

  if (m_deint && m_deint->is_enabled)
    mmal_component_disable(m_deint);

  if (m_dec && m_dec->is_enabled)
      mmal_component_disable(m_dec);

  if (m_dec_input_pool)
    mmal_pool_destroy(m_dec_input_pool);
  m_dec_input_pool = NULL;

  if (m_dec_output_pool)
    mmal_pool_destroy(m_dec_output_pool);
  m_dec_output_pool = NULL;

  if (m_deint)
    mmal_component_destroy(m_deint);
  m_deint = NULL;

  if (m_dec)
    mmal_component_destroy(m_dec);
  m_dec = NULL;
  mmal_format_free(m_es_format);
  m_es_format = NULL;
}

void CMMALVideo::PortSettingsChanged(MMAL_PORT_T *port, MMAL_BUFFER_HEADER_T *buffer)
{
  MMAL_EVENT_FORMAT_CHANGED_T *fmt = mmal_event_format_changed_get(buffer);
  mmal_format_copy(m_es_format, fmt->format);
  m_changed_count++;

  if (m_es_format->es->video.crop.width && m_es_format->es->video.crop.height)
  {
    if (m_es_format->es->video.par.num && m_es_format->es->video.par.den)
      m_aspect_ratio = (float)(m_es_format->es->video.par.num * m_es_format->es->video.crop.width) / (m_es_format->es->video.par.den * m_es_format->es->video.crop.height);
    m_decoded_width = m_es_format->es->video.crop.width;
    m_decoded_height = m_es_format->es->video.crop.height;
    CLog::Log(LOGDEBUG, "%s::%s format changed: %dx%d %.2f frame:%d", CLASSNAME, __func__, m_decoded_width, m_decoded_height, m_aspect_ratio, m_changed_count);
  }
  else
    CLog::Log(LOGERROR, "%s::%s format changed: Unexpected %dx%d", CLASSNAME, __func__, m_es_format->es->video.crop.width, m_es_format->es->video.crop.height);
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
    CLog::Log(LOGDEBUG, "%s::%s format changed", CLASSNAME, __func__);
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


static void dec_input_port_cb(MMAL_PORT_T *port, MMAL_BUFFER_HEADER_T *buffer)
{
#if defined(MMAL_DEBUG_VERBOSE)
  CLog::Log(LOGDEBUG, "%s::%s port:%p buffer %p, len %d cmd:%x", CLASSNAME, __func__, port, buffer, buffer->length, buffer->cmd);
#endif
  mmal_buffer_header_release(buffer);
}


void CMMALVideo::dec_output_port_cb(MMAL_PORT_T *port, MMAL_BUFFER_HEADER_T *buffer)
{
#if defined(MMAL_DEBUG_VERBOSE)
  if (!(buffer->cmd == 0 && buffer->length > 0))
    CLog::Log(LOGDEBUG, "%s::%s port:%p buffer %p, len %d cmd:%x", CLASSNAME, __func__, port, buffer, buffer->length, buffer->cmd);
#endif
  bool kept = false;

  if (buffer->cmd == 0)
  {
    if (buffer->length > 0)
    {
      assert(!(buffer->flags & MMAL_BUFFER_HEADER_FLAG_DECODEONLY));
      double dts = DVD_NOPTS_VALUE;
      if (buffer->flags & MMAL_BUFFER_HEADER_FLAG_USER0)
      {
        pthread_mutex_lock(&m_output_mutex);
        if (!m_dts_queue.empty())
        {
          dts = m_dts_queue.front();
          m_dts_queue.pop();
        }
        else assert(0);
        pthread_mutex_unlock(&m_output_mutex);
      }

      if (m_drop_state)
      {
        CLog::Log(LOGDEBUG, "%s::%s - dropping %p (drop:%d)", CLASSNAME, __func__, buffer, m_drop_state);
      }
      else
      {
        CMMALVideoBuffer *omvb = new CMMALVideoBuffer(this);
        m_output_busy++;
#if defined(MMAL_DEBUG_VERBOSE)
        CLog::Log(LOGDEBUG, "%s::%s - %p (%p) buffer_size(%u) dts:%.3f pts:%.3f flags:%x:%x frame:%d",
          CLASSNAME, __func__, buffer, omvb, buffer->length, dts*1e-6, buffer->pts*1e-6, buffer->flags, buffer->type->video.flags, omvb->m_changed_count);
#endif
        omvb->mmal_buffer = buffer;
        buffer->user_data = (void *)omvb;
        omvb->m_changed_count = m_changed_count;
        omvb->dts = dts;
        omvb->width = m_decoded_width;
        omvb->height = m_decoded_height;
        omvb->m_aspect_ratio = m_aspect_ratio;
        pthread_mutex_lock(&m_output_mutex);
        m_output_ready.push(omvb);
        pthread_mutex_unlock(&m_output_mutex);
        kept = true;
      }
    }
  }
  else if (buffer->cmd == MMAL_EVENT_FORMAT_CHANGED)
  {
    PortSettingsChanged(port, buffer);
  }
  if (!kept)
    mmal_buffer_header_release(buffer);
}

static void dec_output_port_cb_static(MMAL_PORT_T *port, MMAL_BUFFER_HEADER_T *buffer)
{
  CMMALVideo *mmal = reinterpret_cast<CMMALVideo*>(port->userdata);
  mmal->dec_output_port_cb(port, buffer);
}

bool CMMALVideo::change_dec_output_format()
{
  MMAL_STATUS_T status;
  CLog::Log(LOGDEBUG, "%s::%s", CLASSNAME, __func__);

  MMAL_PARAMETER_VIDEO_INTERLACE_TYPE_T interlace_type = {{ MMAL_PARAMETER_VIDEO_INTERLACE_TYPE, sizeof( interlace_type )}};
  status = mmal_port_parameter_get( m_dec_output, &interlace_type.hdr );

  if (status == MMAL_SUCCESS)
  {
    if (m_interlace_mode != interlace_type.eMode)
    {
      CLog::Log(LOGDEBUG, "%s::%s Interlace mode %d->%d", CLASSNAME, __func__, m_interlace_mode, interlace_type.eMode);
      m_interlace_mode = interlace_type.eMode;
    }
  }
  else
    CLog::Log(LOGERROR, "%s::%s Failed to query interlace type on %s (status=%x %s)", CLASSNAME, __func__, m_dec_output->name, status, mmal_status_to_string(status));

  // todo: if we don't disable/enable we can do this from callback
  mmal_format_copy(m_dec_output->format, m_es_format);
  status = mmal_port_format_commit(m_dec_output);
  if (status != MMAL_SUCCESS)
  {
    CLog::Log(LOGERROR, "%s::%s Failed to commit decoder output port (status=%x %s)", CLASSNAME, __func__, status, mmal_status_to_string(status));
    return false;
  }
  return true;
}

bool CMMALVideo::CreateDeinterlace(EINTERLACEMETHOD interlace_method)
{
  MMAL_STATUS_T status;

  CLog::Log(LOGDEBUG, "%s::%s method:%d", CLASSNAME, __func__, interlace_method);

  assert(!m_deint);
  assert(m_dec_output == m_dec->output[0]);

  status = mmal_port_disable(m_dec_output);
  if (status != MMAL_SUCCESS)
  {
    CLog::Log(LOGERROR, "%s::%s Failed to disable decoder output port (status=%x %s)", CLASSNAME, __func__, status, mmal_status_to_string(status));
    return false;
  }

  /* Create deinterlace filter */
  status = mmal_component_create("vc.ril.image_fx", &m_deint);
  if (status != MMAL_SUCCESS)
  {
    CLog::Log(LOGERROR, "%s::%s Failed to create deinterlace component (status=%x %s)", CLASSNAME, __func__, status, mmal_status_to_string(status));
    return false;
  }
  MMAL_PARAMETER_IMAGEFX_PARAMETERS_T imfx_param = {{MMAL_PARAMETER_IMAGE_EFFECT_PARAMETERS, sizeof(imfx_param)}, MMAL_PARAM_IMAGEFX_DEINTERLACE_FAST, 1, {3}};
  status = mmal_port_parameter_set(m_deint->output[0], &imfx_param.hdr);
  if (status != MMAL_SUCCESS)
  {
    CLog::Log(LOGERROR, "%s::%s Failed to set deinterlace parameters (status=%x %s)", CLASSNAME, __func__, status, mmal_status_to_string(status));
    return false;
  }

  MMAL_PORT_T *m_deint_input = m_deint->input[0];
  m_deint_input->userdata = (struct MMAL_PORT_USERDATA_T *)this;

  // Now connect the decoder output port to deinterlace input port
  status =  mmal_connection_create(&m_deint_connection, m_dec->output[0], m_deint->input[0], MMAL_CONNECTION_FLAG_TUNNELLING | MMAL_CONNECTION_FLAG_ALLOCATION_ON_INPUT);
  if (status != MMAL_SUCCESS)
  {
    CLog::Log(LOGERROR, "%s::%s Failed to connect deinterlacer component %s (status=%x %s)", CLASSNAME, __func__, m_deint->name, status, mmal_status_to_string(status));
    return false;
  }

  status =  mmal_connection_enable(m_deint_connection);
  if (status != MMAL_SUCCESS)
  {
    CLog::Log(LOGERROR, "%s::%s Failed to enable connection %s (status=%x %s)", CLASSNAME, __func__, m_deint->name, status, mmal_status_to_string(status));
    return false;
  }

  mmal_format_copy(m_deint->output[0]->format, m_es_format);
  status = mmal_port_format_commit(m_deint->output[0]);
  if (status != MMAL_SUCCESS)
  {
    CLog::Log(LOGERROR, "%s::%s Failed to commit deint output format (status=%x %s)", CLASSNAME, __func__, status, mmal_status_to_string(status));
    return false;
  }

  status = mmal_component_enable(m_deint);
  if (status != MMAL_SUCCESS)
  {
    CLog::Log(LOGERROR, "%s::%s Failed to enable deinterlacer component %s (status=%x %s)", CLASSNAME, __func__, m_deint->name, status, mmal_status_to_string(status));
    return false;
  }

  m_deint->output[0]->buffer_size = m_deint->output[0]->buffer_size_min;
  m_deint->output[0]->buffer_num = m_deint->output[0]->buffer_num_recommended;
  m_deint->output[0]->userdata = (struct MMAL_PORT_USERDATA_T *)this;
  status = mmal_port_enable(m_deint->output[0], dec_output_port_cb_static);
  if (status != MMAL_SUCCESS)
  {
    CLog::Log(LOGERROR, "%s::%s Failed to enable decoder output port (status=%x %s)", CLASSNAME, __func__, status, mmal_status_to_string(status));
    return false;
  }

  m_dec_output = m_deint->output[0];
  m_interlace_method = interlace_method;

  return true;
}

bool CMMALVideo::DestroyDeinterlace()
{
  MMAL_STATUS_T status;

  CLog::Log(LOGDEBUG, "%s::%s", CLASSNAME, __func__);

  assert(m_deint);
  assert(m_dec_output == m_deint->output[0]);

  status = mmal_port_disable(m_dec_output);
  if (status != MMAL_SUCCESS)
  {
    CLog::Log(LOGERROR, "%s::%s Failed to disable decoder output port (status=%x %s)", CLASSNAME, __func__, status, mmal_status_to_string(status));
    return false;
  }

  status = mmal_connection_destroy(m_deint_connection);
  if (status != MMAL_SUCCESS)
  {
    CLog::Log(LOGERROR, "%s::%s Failed to destroy deinterlace connection (status=%x %s)", CLASSNAME, __func__, status, mmal_status_to_string(status));
    return false;
  }
  m_deint_connection = NULL;

  status = mmal_component_disable(m_deint);
  if (status != MMAL_SUCCESS)
  {
    CLog::Log(LOGERROR, "%s::%s Failed to disable deinterlace component (status=%x %s)", CLASSNAME, __func__, status, mmal_status_to_string(status));
    return false;
  }

  status = mmal_component_destroy(m_deint);
  if (status != MMAL_SUCCESS)
  {
    CLog::Log(LOGERROR, "%s::%s Failed to destroy deinterlace component (status=%x %s)", CLASSNAME, __func__, status, mmal_status_to_string(status));
    return false;
  }
  m_deint = NULL;

  m_dec->output[0]->buffer_size = m_dec->output[0]->buffer_size_min;
  m_dec->output[0]->buffer_num = m_dec->output[0]->buffer_num_recommended;
  m_dec->output[0]->userdata = (struct MMAL_PORT_USERDATA_T *)this;
  status = mmal_port_enable(m_dec->output[0], dec_output_port_cb_static);
  if (status != MMAL_SUCCESS)
  {
    CLog::Log(LOGERROR, "%s::%s Failed to enable decoder output port (status=%x %s)", CLASSNAME, __func__, status, mmal_status_to_string(status));
    return false;
  }

  m_dec_output = m_dec->output[0];
  m_interlace_method = VS_INTERLACEMETHOD_NONE;
  return true;
}

bool CMMALVideo::SendCodecConfigData()
{
  MMAL_STATUS_T status;
  if (!m_dec_input_pool)
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
  buffer->flags |= MMAL_BUFFER_HEADER_FLAG_FRAME_END | MMAL_BUFFER_HEADER_FLAG_CONFIG;
#if defined(MMAL_DEBUG_VERBOSE)
  CLog::Log(LOGDEBUG, "%s::%s - %-8p %-6d flags:%x", CLASSNAME, __func__, buffer, buffer->length, buffer->flags);
#endif
  status = mmal_port_send_buffer(m_dec_input, buffer);
  if (status != MMAL_SUCCESS)
  {
    CLog::Log(LOGERROR, "%s::%s Failed send buffer to decoder input port (status=%x %s)", CLASSNAME, __func__, status, mmal_status_to_string(status));
    return false;
  }
  return true;
}

bool CMMALVideo::Open(CDVDStreamInfo &hints, CDVDCodecOptions &options, MMALVideoPtr myself)
{
  #if defined(MMAL_DEBUG_VERBOSE)
  CLog::Log(LOGDEBUG, "%s::%s usemmal:%d software:%d %dx%d", CLASSNAME, __func__, CSettings::Get().GetBool("videoplayer.usemmal"), hints.software, hints.width, hints.height);
  #endif

  // we always qualify even if DVDFactoryCodec does this too.
  if (!CSettings::Get().GetBool("videoplayer.usemmal") || hints.software)
    return false;

  m_hints = hints;
  MMAL_STATUS_T status;
  MMAL_PARAMETER_BOOLEAN_T error_concealment;

  m_myself = myself;
  m_decoded_width  = hints.width;
  m_decoded_height = hints.height;

  // use aspect in stream if available
  if (m_hints.forced_aspect)
    m_aspect_ratio = m_hints.aspect;
  else
    m_aspect_ratio = 0.0;

  switch (hints.codec)
  {
    case AV_CODEC_ID_H264:
      // H.264
      m_codingType = MMAL_ENCODING_H264;
      m_pFormatName = "mmal-h264";
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

  // initialize mmal.
  status = mmal_component_create(MMAL_COMPONENT_DEFAULT_VIDEO_DECODER, &m_dec);
  if (status != MMAL_SUCCESS)
  {
    CLog::Log(LOGERROR, "%s::%s Failed to create MMAL decoder component %s (status=%x %s)", CLASSNAME, __func__, MMAL_COMPONENT_DEFAULT_VIDEO_DECODER, status, mmal_status_to_string(status));
    return false;
  }

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
  if (m_hints.width && m_hints.height)
  {
    m_dec_input->format->es->video.crop.width = m_hints.width;
    m_dec_input->format->es->video.crop.height = m_hints.height;

    m_dec_input->format->es->video.width = ALIGN_UP(m_hints.width, 32);
    m_dec_input->format->es->video.height = ALIGN_UP(m_hints.height, 16);
  }
  m_dec_input->format->flags |= MMAL_ES_FORMAT_FLAG_FRAMED;

  error_concealment.hdr.id = MMAL_PARAMETER_VIDEO_DECODE_ERROR_CONCEALMENT;
  error_concealment.hdr.size = sizeof(MMAL_PARAMETER_BOOLEAN_T);
  error_concealment.enable = MMAL_FALSE;
  status = mmal_port_parameter_set(m_dec_input, &error_concealment.hdr);
  if (status != MMAL_SUCCESS)
    CLog::Log(LOGERROR, "%s::%s Failed to disable error concealment on %s (status=%x %s)", CLASSNAME, __func__, m_dec_input->name, status, mmal_status_to_string(status));

  status = mmal_port_parameter_set_uint32(m_dec_input, MMAL_PARAMETER_EXTRA_BUFFERS, NUM_BUFFERS);
  if (status != MMAL_SUCCESS)
    CLog::Log(LOGERROR, "%s::%s Failed to enable extra buffers on %s (status=%x %s)", CLASSNAME, __func__, m_dec_input->name, status, mmal_status_to_string(status));

  status = mmal_port_format_commit(m_dec_input);
  if (status != MMAL_SUCCESS)
  {
    CLog::Log(LOGERROR, "%s::%s Failed to commit format for decoder input port %s (status=%x %s)", CLASSNAME, __func__, m_dec_input->name, status, mmal_status_to_string(status));
    return false;
  }
  m_dec_input->buffer_size = m_dec_input->buffer_size_recommended;
  m_dec_input->buffer_num = m_dec_input->buffer_num_recommended;

  m_dec_input->userdata = (struct MMAL_PORT_USERDATA_T *)this;
  status = mmal_port_enable(m_dec_input, dec_input_port_cb);
  if (status != MMAL_SUCCESS)
  {
    CLog::Log(LOGERROR, "%s::%s Failed to enable decoder input port %s (status=%x %s)", CLASSNAME, __func__, m_dec_input->name, status, mmal_status_to_string(status));
    return false;
  }

  m_dec_output = m_dec->output[0];

  // set up initial decoded frame format - will likely change from this
  m_dec_output->format->encoding = MMAL_ENCODING_OPAQUE;
  mmal_format_copy(m_es_format, m_dec_output->format);

  status = mmal_port_format_commit(m_dec_output);
  if (status != MMAL_SUCCESS)
  {
    CLog::Log(LOGERROR, "%s::%s Failed to commit decoder output format (status=%x %s)", CLASSNAME, __func__, status, mmal_status_to_string(status));
    return false;
  }

  m_dec_output->buffer_size = m_dec_output->buffer_size_min;
  m_dec_output->buffer_num = m_dec_output->buffer_num_recommended;
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

  m_dec_input_pool = mmal_pool_create(m_dec_input->buffer_num, m_dec_input->buffer_size);
  if (!m_dec_input_pool)
  {
    CLog::Log(LOGERROR, "%s::%s Failed to create pool for decoder input port (status=%x %s)", CLASSNAME, __func__, status, mmal_status_to_string(status));
    return false;
  }

  m_dec_output_pool = mmal_pool_create(m_dec_output->buffer_num, m_dec_output->buffer_size);
  if(!m_dec_output_pool)
  {
    CLog::Log(LOGERROR, "%s::%s Failed to create pool for decode output port (status=%x %s)", CLASSNAME, __func__, status, mmal_status_to_string(status));
    return false;
  }

  if (!SendCodecConfigData())
    return false;

  m_drop_state = false;
  m_startframe = false;

  return true;
}

void CMMALVideo::Dispose()
{
  // we are happy to exit, but let last shared pointer being deleted trigger the destructor
  bool done = false;
  Reset();
  pthread_mutex_lock(&m_output_mutex);
  if (!m_output_busy)
    done = true;
  m_finished = true;
  pthread_mutex_unlock(&m_output_mutex);
  #if defined(MMAL_DEBUG_VERBOSE)
  CLog::Log(LOGDEBUG, "%s::%s dts_queue(%d) ready_queue(%d) busy_queue(%d) done:%d", CLASSNAME, __func__, m_dts_queue.size(), m_output_ready.size(), m_output_busy, done);
  #endif
  if (done)
  {
    assert(m_dts_queue.empty());
    m_myself.reset();
  }
}

void CMMALVideo::SetDropState(bool bDrop)
{
#if defined(MMAL_DEBUG_VERBOSE)
  if (m_drop_state != bDrop)
    CLog::Log(LOGDEBUG, "%s::%s - m_drop_state(%d)", CLASSNAME, __func__, bDrop);
#endif
  m_drop_state = bDrop;
  if (m_drop_state)
  {
    while (1)
    {
      CMMALVideoBuffer *buffer = NULL;
      pthread_mutex_lock(&m_output_mutex);
      // fetch a output buffer and pop it off the ready list
      if (!m_output_ready.empty())
      {
        buffer = m_output_ready.front();
        m_output_ready.pop();
      }
      pthread_mutex_unlock(&m_output_mutex);
      if (buffer)
        ReleaseBuffer(buffer);
      else
        break;
    }
  }
}

int CMMALVideo::Decode(uint8_t* pData, int iSize, double dts, double pts)
{
  #if defined(MMAL_DEBUG_VERBOSE)
  //CLog::Log(LOGDEBUG, "%s::%s - %-8p %-6d dts:%.3f pts:%.3f dts_queue(%d) ready_queue(%d) busy_queue(%d)",
  //   CLASSNAME, __func__, pData, iSize, dts == DVD_NOPTS_VALUE ? 0.0 : dts*1e-6, pts == DVD_NOPTS_VALUE ? 0.0 : pts*1e-6, m_dts_queue.size(), m_output_ready.size(), m_output_busy);
  #endif

  unsigned int demuxer_bytes = 0;
  uint8_t *demuxer_content = NULL;
  MMAL_BUFFER_HEADER_T *buffer;
  MMAL_STATUS_T status;

  while (buffer = mmal_queue_get(m_dec_output_pool->queue), buffer)
    Recycle(buffer);
  // we need to queue then de-queue the demux packet, seems silly but
  // mmal might not have an input buffer available when we are called
  // and we must store the demuxer packet and try again later.
  // try to send any/all demux packets to mmal decoder.
  unsigned space = mmal_queue_length(m_dec_input_pool->queue) * m_dec_input->buffer_size;
  if (pData && m_demux_queue.empty() && space >= (unsigned int)iSize)
  {
    demuxer_bytes = iSize;
    demuxer_content = pData;
  }
  else if (pData && iSize)
  {
    mmal_demux_packet demux_packet;
    demux_packet.dts = dts;
    demux_packet.pts = pts;
    demux_packet.size = iSize;
    demux_packet.buff = new uint8_t[iSize];
    memcpy(demux_packet.buff, pData, iSize);
    m_demux_queue_length += demux_packet.size;
    m_demux_queue.push(demux_packet);
  }

  uint8_t *buffer_to_free = NULL;

  while (1)
  {
     while (buffer = mmal_queue_get(m_dec_output_pool->queue), buffer)
       Recycle(buffer);

     space = mmal_queue_length(m_dec_input_pool->queue) * m_dec_input->buffer_size;
     if (!demuxer_bytes && !m_demux_queue.empty())
     {
       mmal_demux_packet &demux_packet = m_demux_queue.front();
       if (space >= (unsigned int)demux_packet.size)
       {
         // need to lock here to retrieve an input buffer and pop the queue
         m_demux_queue_length -= demux_packet.size;
         m_demux_queue.pop();
         demuxer_bytes = (unsigned int)demux_packet.size;
         demuxer_content = demux_packet.buff;
         buffer_to_free = demux_packet.buff;
         dts = demux_packet.dts;
         pts = demux_packet.pts;
       }
     }
     if (demuxer_content)
     {
       // 500ms timeout
       buffer = mmal_queue_timedwait(m_dec_input_pool->queue, 500);
       if (!buffer)
       {
         CLog::Log(LOGERROR, "%s::%s - mmal_queue_get failed", CLASSNAME, __func__);
         return VC_ERROR;
       }

       mmal_buffer_header_reset(buffer);
       buffer->cmd = 0;
       if (m_startframe && pts == DVD_NOPTS_VALUE)
         pts = 0;
       buffer->pts = pts == DVD_NOPTS_VALUE ? MMAL_TIME_UNKNOWN : pts;
       buffer->dts = dts == DVD_NOPTS_VALUE ? MMAL_TIME_UNKNOWN : dts;
       buffer->length = demuxer_bytes > buffer->alloc_size ? buffer->alloc_size : demuxer_bytes;
       buffer->user_data = (void *)m_decode_frame_number;
       // set a flag so we can identify primary frames from generated frames (deinterlace)
       buffer->flags = MMAL_BUFFER_HEADER_FLAG_USER0;

       // Request decode only (maintain ref frames, but don't return a picture)
       if (m_drop_state)
         buffer->flags |= MMAL_BUFFER_HEADER_FLAG_DECODEONLY;

       memcpy(buffer->data, demuxer_content, buffer->length);
       demuxer_bytes   -= buffer->length;
       demuxer_content += buffer->length;

       if (demuxer_bytes == 0)
         buffer->flags |= MMAL_BUFFER_HEADER_FLAG_FRAME_END;

       #if defined(MMAL_DEBUG_VERBOSE)
       CLog::Log(LOGDEBUG, "%s::%s - %-8p %-6d/%-6d dts:%.3f pts:%.3f flags:%x dts_queue(%d) ready_queue(%d) busy_queue(%d) demux_queue(%d) space(%d)",
          CLASSNAME, __func__, buffer, buffer->length, demuxer_bytes, dts == DVD_NOPTS_VALUE ? 0.0 : dts*1e-6, pts == DVD_NOPTS_VALUE ? 0.0 : pts*1e-6, buffer->flags, m_dts_queue.size(), m_output_ready.size(), m_output_busy, m_demux_queue_length, mmal_queue_length(m_dec_input_pool->queue) * m_dec_input->buffer_size);
       #endif
       assert((int)buffer->length > 0);
       status = mmal_port_send_buffer(m_dec_input, buffer);
       if (status != MMAL_SUCCESS)
       {
         CLog::Log(LOGERROR, "%s::%s Failed send buffer to decoder input port (status=%x %s)", CLASSNAME, __func__, status, mmal_status_to_string(status));
         return VC_ERROR;
       }

       if (demuxer_bytes == 0)
       {
         m_decode_frame_number++;
         m_startframe = true;
         if (m_drop_state)
         {
           m_droppedPics += m_deint ? 2:1;
         }
         else
         {
           // only push if we are successful with feeding mmal
           pthread_mutex_lock(&m_output_mutex);
           m_dts_queue.push(dts);
           assert(m_dts_queue.size() < 5000);
           pthread_mutex_unlock(&m_output_mutex);
         }
         if (m_changed_count_dec != m_changed_count)
         {
           CLog::Log(LOGDEBUG, "%s::%s format changed frame:%d(%d)", CLASSNAME, __func__, m_changed_count_dec, m_changed_count);
           m_changed_count_dec = m_changed_count;
           if (!change_dec_output_format())
           {
             CLog::Log(LOGERROR, "%s::%s - change_dec_output_format() failed", CLASSNAME, __func__);
             return VC_ERROR;
           }
         }
         EDEINTERLACEMODE deinterlace_request = CMediaSettings::Get().GetCurrentVideoSettings().m_DeinterlaceMode;
         EINTERLACEMETHOD interlace_method = g_renderManager.AutoInterlaceMethod(CMediaSettings::Get().GetCurrentVideoSettings().m_InterlaceMethod);

         bool deinterlace = m_interlace_mode != MMAL_InterlaceProgressive;

         if (deinterlace_request == VS_DEINTERLACEMODE_OFF)
           deinterlace = false;
         else if (deinterlace_request == VS_DEINTERLACEMODE_FORCE)
           deinterlace = true;

         if (((deinterlace && interlace_method != m_interlace_method) || !deinterlace) && m_deint)
           DestroyDeinterlace();
         if (deinterlace && !m_deint)
           CreateDeinterlace(interlace_method);

         if (buffer_to_free)
         {
           delete [] buffer_to_free;
           buffer_to_free = NULL;
           demuxer_content = NULL;
           continue;
         }
         while (buffer = mmal_queue_get(m_dec_output_pool->queue), buffer)
           Recycle(buffer);
       }
    }
    if (!demuxer_bytes)
      break;
  }
  int ret = 0;
  if (!m_output_ready.empty())
  {
    #if defined(MMAL_DEBUG_VERBOSE)
    CLog::Log(LOGDEBUG, "%s::%s -  got output picture:%d", CLASSNAME, __func__, m_output_ready.size());
    #endif
    ret |= VC_PICTURE;
  }
  if (mmal_queue_length(m_dec_input_pool->queue) > 0 && !m_demux_queue_length)
  {
    #if defined(MMAL_DEBUG_VERBOSE)
    CLog::Log(LOGDEBUG, "%s::%s - got space for output: demux_queue(%d) space(%d)", CLASSNAME, __func__, m_demux_queue_length, mmal_queue_length(m_dec_input_pool->queue) * m_dec_input->buffer_size);
    #endif
    ret |= VC_BUFFER;
  }
  if (!ret)
  {
    CLog::Log(LOGDEBUG, "%s::%s - Nothing to do: dts_queue(%d) ready_queue(%d) busy_queue(%d) demux_queue(%d) space(%d)",
        CLASSNAME, __func__, m_dts_queue.size(), m_output_ready.size(), m_output_busy, m_demux_queue_length, mmal_queue_length(m_dec_input_pool->queue) * m_dec_input->buffer_size);
    Sleep(10); // otherwise we busy spin
  }
  return ret;
}

void CMMALVideo::Reset(void)
{
  #if defined(MMAL_DEBUG_VERBOSE)
  CLog::Log(LOGDEBUG, "%s::%s", CLASSNAME, __func__);
  #endif

  if (m_dec_input)
    mmal_port_disable(m_dec_input);
  if (m_deint_connection)
    mmal_connection_disable(m_deint_connection);
  if (m_dec_output)
    mmal_port_disable(m_dec_output);
  if (m_dec_input)
    mmal_port_enable(m_dec_input, dec_input_port_cb);
  if (m_deint_connection)
    mmal_connection_enable(m_deint_connection);
  if (m_dec_output)
    mmal_port_enable(m_dec_output, dec_output_port_cb_static);

  // blow all ready video frames
  bool old_drop_state = m_drop_state;
  SetDropState(true);
  pthread_mutex_lock(&m_output_mutex);
  while(!m_dts_queue.empty())
    m_dts_queue.pop();
  while (!m_demux_queue.empty())
    m_demux_queue.pop();
  m_demux_queue_length = 0;
  pthread_mutex_unlock(&m_output_mutex);
  if (!old_drop_state)
    SetDropState(false);

  SendCodecConfigData();

  m_startframe = false;
  m_decoderPts = DVD_NOPTS_VALUE;
  m_droppedPics = 0;
  m_decode_frame_number = 1;
}


void CMMALVideo::ReturnBuffer(CMMALVideoBuffer *buffer)
{
#if defined(MMAL_DEBUG_VERBOSE)
  CLog::Log(LOGDEBUG, "%s::%s %p (%d)", CLASSNAME, __func__, buffer, m_output_busy);
#endif

  mmal_buffer_header_release(buffer->mmal_buffer);
}

void CMMALVideo::Recycle(MMAL_BUFFER_HEADER_T *buffer)
{
#if defined(MMAL_DEBUG_VERBOSE)
  CLog::Log(LOGDEBUG, "%s::%s %p", CLASSNAME, __func__, buffer);
#endif

  MMAL_STATUS_T status;
  mmal_buffer_header_reset(buffer);
  buffer->cmd = 0;
  #if defined(MMAL_DEBUG_VERBOSE)
  CLog::Log(LOGDEBUG, "%s::%s Send buffer %p from pool to decoder output port %p dts_queue(%d) ready_queue(%d) busy_queue(%d)", CLASSNAME, __func__, buffer, m_dec_output,
    m_dts_queue.size(), m_output_ready.size(), m_output_busy);
  #endif
  status = mmal_port_send_buffer(m_dec_output, buffer);
  if (status != MMAL_SUCCESS)
  {
    CLog::Log(LOGERROR, "%s::%s - Failed send buffer to decoder output port (status=0%x %s)", CLASSNAME, __func__, status, mmal_status_to_string(status));
    return;
  }
}

void CMMALVideo::ReleaseBuffer(CMMALVideoBuffer *buffer)
{
  // remove from busy list
  pthread_mutex_lock(&m_output_mutex);
  assert(m_output_busy > 0);
  m_output_busy--;
  pthread_mutex_unlock(&m_output_mutex);
  ReturnBuffer(buffer);
  bool done = false;
  pthread_mutex_lock(&m_output_mutex);
  if (m_finished && !m_output_busy)
    done = true;
  pthread_mutex_unlock(&m_output_mutex);
  if (done)
    m_myself.reset();
  #if defined(MMAL_DEBUG_VERBOSE)
  CLog::Log(LOGDEBUG, "%s::%s %p (%p) dts_queue(%d) ready_queue(%d) busy_queue(%d) done:%d", CLASSNAME, __func__, buffer, buffer->mmal_buffer, m_dts_queue.size(), m_output_ready.size(), m_output_busy, done);
  #endif
  delete buffer;
}

bool CMMALVideo::GetPicture(DVDVideoPicture* pDvdVideoPicture)
{
  if (!m_output_ready.empty())
  {
    CMMALVideoBuffer *buffer;
    // fetch a output buffer and pop it off the ready list
    pthread_mutex_lock(&m_output_mutex);
    buffer = m_output_ready.front();
    m_output_ready.pop();
    pthread_mutex_unlock(&m_output_mutex);

    assert(buffer->mmal_buffer);
    memset(pDvdVideoPicture, 0, sizeof *pDvdVideoPicture);
    pDvdVideoPicture->format = RENDER_FMT_MMAL;
    pDvdVideoPicture->MMALBuffer = buffer;
    pDvdVideoPicture->color_range  = 0;
    pDvdVideoPicture->color_matrix = 4;
    pDvdVideoPicture->iWidth  = buffer->width ? buffer->width : m_decoded_width;
    pDvdVideoPicture->iHeight = buffer->height ? buffer->height : m_decoded_height;
    pDvdVideoPicture->iDisplayWidth  = pDvdVideoPicture->iWidth;
    pDvdVideoPicture->iDisplayHeight = pDvdVideoPicture->iHeight;
    //CLog::Log(LOGDEBUG, "%s::%s -  %dx%d %dx%d %dx%d %dx%d %f,%f", CLASSNAME, __func__, pDvdVideoPicture->iWidth, pDvdVideoPicture->iHeight, pDvdVideoPicture->iDisplayWidth, pDvdVideoPicture->iDisplayHeight, m_decoded_width, m_decoded_height, buffer->width, buffer->height, buffer->m_aspect_ratio, m_hints.aspect);

    if (buffer->m_aspect_ratio > 0.0)
    {
      pDvdVideoPicture->iDisplayWidth  = ((int)lrint(pDvdVideoPicture->iHeight * buffer->m_aspect_ratio)) & -3;
      if (pDvdVideoPicture->iDisplayWidth > pDvdVideoPicture->iWidth)
      {
        pDvdVideoPicture->iDisplayWidth  = pDvdVideoPicture->iWidth;
        pDvdVideoPicture->iDisplayHeight = ((int)lrint(pDvdVideoPicture->iWidth / buffer->m_aspect_ratio)) & -3;
      }
    }

    // timestamp is in microseconds
    pDvdVideoPicture->dts = buffer->dts;
    pDvdVideoPicture->pts = buffer->mmal_buffer->pts == MMAL_TIME_UNKNOWN || buffer->mmal_buffer->pts == 0 ? DVD_NOPTS_VALUE : buffer->mmal_buffer->pts;

    pDvdVideoPicture->MMALBuffer->Acquire();
    pDvdVideoPicture->iFlags  = DVP_FLAG_ALLOCATED;
#if defined(MMAL_DEBUG_VERBOSE)
    CLog::Log(LOGINFO, "%s::%s dts:%.3f pts:%.3f flags:%x:%x MMALBuffer:%p mmal_buffer:%p", CLASSNAME, __func__,
        pDvdVideoPicture->dts == DVD_NOPTS_VALUE ? 0.0 : pDvdVideoPicture->dts*1e-6, pDvdVideoPicture->pts == DVD_NOPTS_VALUE ? 0.0 : pDvdVideoPicture->pts*1e-6,
        pDvdVideoPicture->iFlags, buffer->mmal_buffer->flags, pDvdVideoPicture->MMALBuffer, pDvdVideoPicture->MMALBuffer->mmal_buffer);
#endif
    assert(!(buffer->mmal_buffer->flags & MMAL_BUFFER_HEADER_FLAG_DECODEONLY));
  }
  else
  {
    CLog::Log(LOGERROR, "%s::%s - called but m_output_ready is empty", CLASSNAME, __func__);
    return false;
  }

  if (pDvdVideoPicture->pts != DVD_NOPTS_VALUE)
    m_decoderPts = pDvdVideoPicture->pts;
  else
    m_decoderPts = pDvdVideoPicture->dts; // xxx is DVD_NOPTS_VALUE better?

  return true;
}

bool CMMALVideo::ClearPicture(DVDVideoPicture* pDvdVideoPicture)
{
  if (pDvdVideoPicture->format == RENDER_FMT_MMAL)
  {
#if defined(MMAL_DEBUG_VERBOSE)
    CLog::Log(LOGDEBUG, "%s::%s - %p (%p)", CLASSNAME, __func__, pDvdVideoPicture->MMALBuffer, pDvdVideoPicture->MMALBuffer->mmal_buffer);
#endif
    pDvdVideoPicture->MMALBuffer->Release();
  }
  memset(pDvdVideoPicture, 0, sizeof *pDvdVideoPicture);
  return true;
}

bool CMMALVideo::GetCodecStats(double &pts, int &droppedPics)
{
  pts = m_decoderPts;
  droppedPics = m_droppedPics;
  m_droppedPics = 0;
#if defined(MMAL_DEBUG_VERBOSE)
  //CLog::Log(LOGDEBUG, "%s::%s - pts:%.0f droppedPics:%d", CLASSNAME, __func__, pts, droppedPics);
#endif
  return true;
}
