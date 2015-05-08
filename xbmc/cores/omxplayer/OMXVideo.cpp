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

#include "OMXVideo.h"

#include "utils/log.h"
#include "linux/XMemUtils.h"
#include "DVDDemuxers/DVDDemuxUtils.h"
#include "settings/AdvancedSettings.h"
#include "settings/MediaSettings.h"
#include "cores/VideoRenderers/RenderManager.h"
#include "xbmc/guilib/GraphicContext.h"
#include "settings/Settings.h"
#include "utils/BitstreamConverter.h"

#include "linux/RBP.h"

#include <sys/time.h>
#include <inttypes.h>

#ifdef CLASSNAME
#undef CLASSNAME
#endif
#define CLASSNAME "COMXVideo"

#define OMX_VIDEO_DECODER       "OMX.broadcom.video_decode"
#define OMX_H264BASE_DECODER    OMX_VIDEO_DECODER
#define OMX_H264MAIN_DECODER    OMX_VIDEO_DECODER
#define OMX_H264HIGH_DECODER    OMX_VIDEO_DECODER
#define OMX_MPEG4_DECODER       OMX_VIDEO_DECODER
#define OMX_MSMPEG4V1_DECODER   OMX_VIDEO_DECODER
#define OMX_MSMPEG4V2_DECODER   OMX_VIDEO_DECODER
#define OMX_MSMPEG4V3_DECODER   OMX_VIDEO_DECODER
#define OMX_MPEG4EXT_DECODER    OMX_VIDEO_DECODER
#define OMX_MPEG2V_DECODER      OMX_VIDEO_DECODER
#define OMX_VC1_DECODER         OMX_VIDEO_DECODER
#define OMX_WMV3_DECODER        OMX_VIDEO_DECODER
#define OMX_VP6_DECODER         OMX_VIDEO_DECODER
#define OMX_VP8_DECODER         OMX_VIDEO_DECODER
#define OMX_THEORA_DECODER      OMX_VIDEO_DECODER
#define OMX_MJPEG_DECODER       OMX_VIDEO_DECODER

#define MAX_TEXT_LENGTH 1024

COMXVideo::COMXVideo() : m_video_codec_name("")
{
  m_is_open           = false;
  m_extradata         = NULL;
  m_extrasize         = 0;
  m_deinterlace       = false;
  m_deinterlace_request = VS_DEINTERLACEMODE_OFF;
  m_hdmi_clock_sync   = false;
  m_drop_state        = false;
  m_decoded_width     = 0;
  m_decoded_height    = 0;
  m_omx_clock         = NULL;
  m_av_clock          = NULL;
  m_res_callback      = NULL;
  m_res_ctx           = NULL;
  m_submitted_eos     = false;
  m_failed_eos        = false;
  m_settings_changed  = false;
  m_setStartTime      = false;
  m_transform         = OMX_DISPLAY_ROT0;
}

COMXVideo::~COMXVideo()
{
  Close();
}

bool COMXVideo::SendDecoderConfig()
{
  CSingleLock lock (m_critSection);
  OMX_ERRORTYPE omx_err   = OMX_ErrorNone;

  /* send decoder config */
  if(m_extrasize > 0 && m_extradata != NULL)
  {
    OMX_BUFFERHEADERTYPE *omx_buffer = m_omx_decoder.GetInputBuffer();

    if(omx_buffer == NULL)
    {
      CLog::Log(LOGERROR, "%s::%s - buffer error 0x%08x", CLASSNAME, __func__, omx_err);
      return false;
    }

    omx_buffer->nOffset = 0;
    omx_buffer->nFilledLen = std::min((OMX_U32)m_extrasize, omx_buffer->nAllocLen);

    memset((unsigned char *)omx_buffer->pBuffer, 0x0, omx_buffer->nAllocLen);
    memcpy((unsigned char *)omx_buffer->pBuffer, m_extradata, omx_buffer->nFilledLen);
    omx_buffer->nFlags = OMX_BUFFERFLAG_CODECCONFIG | OMX_BUFFERFLAG_ENDOFFRAME;
  
    omx_err = m_omx_decoder.EmptyThisBuffer(omx_buffer);
    if (omx_err != OMX_ErrorNone)
    {
      CLog::Log(LOGERROR, "%s::%s - OMX_EmptyThisBuffer() failed with result(0x%x)\n", CLASSNAME, __func__, omx_err);
      m_omx_decoder.DecoderEmptyBufferDone(m_omx_decoder.GetComponent(), omx_buffer);
      return false;
    }
  }
  return true;
}

bool COMXVideo::NaluFormatStartCodes(enum AVCodecID codec, uint8_t *in_extradata, int in_extrasize)
{
  switch(codec)
  {
    case AV_CODEC_ID_H264:
      if (in_extrasize < 7 || in_extradata == NULL)
        return true;
      // valid avcC atom data always starts with the value 1 (version), otherwise annexb
      else if ( *in_extradata != 1 )
        return true;
    default: break;
  }
  return false;    
}

bool COMXVideo::PortSettingsChanged()
{
  CSingleLock lock (m_critSection);
  OMX_ERRORTYPE omx_err   = OMX_ErrorNone;

  if (m_settings_changed)
  {
    m_omx_decoder.DisablePort(m_omx_decoder.GetOutputPort(), true);
  }

  OMX_PARAM_PORTDEFINITIONTYPE port_image;
  OMX_INIT_STRUCTURE(port_image);
  port_image.nPortIndex = m_omx_decoder.GetOutputPort();
  omx_err = m_omx_decoder.GetParameter(OMX_IndexParamPortDefinition, &port_image);
  if(omx_err != OMX_ErrorNone)
  {
    CLog::Log(LOGERROR, "%s::%s - error m_omx_decoder.GetParameter(OMX_IndexParamPortDefinition) omx_err(0x%08x)", CLASSNAME, __func__, omx_err);
  }

  OMX_CONFIG_POINTTYPE pixel_aspect;
  OMX_INIT_STRUCTURE(pixel_aspect);
  pixel_aspect.nPortIndex = m_omx_decoder.GetOutputPort();
  omx_err = m_omx_decoder.GetParameter(OMX_IndexParamBrcmPixelAspectRatio, &pixel_aspect);
  if(omx_err != OMX_ErrorNone)
  {
    CLog::Log(LOGERROR, "%s::%s - error m_omx_decoder.GetParameter(OMX_IndexParamBrcmPixelAspectRatio) omx_err(0x%08x)", CLASSNAME, __func__, omx_err);
  }

  OMX_CONFIG_INTERLACETYPE interlace;
  OMX_INIT_STRUCTURE(interlace);
  interlace.nPortIndex = m_omx_decoder.GetOutputPort();
  omx_err = m_omx_decoder.GetConfig(OMX_IndexConfigCommonInterlace, &interlace);

  if(m_deinterlace_request == VS_DEINTERLACEMODE_FORCE)
    m_deinterlace = true;
  else if(m_deinterlace_request == VS_DEINTERLACEMODE_OFF)
    m_deinterlace = false;
  else
    m_deinterlace = interlace.eMode != OMX_InterlaceProgressive;

    CLog::Log(LOGDEBUG, "%s::%s - %dx%d@%.2f interlace:%d deinterlace:%d", CLASSNAME, __func__,
      port_image.format.video.nFrameWidth, port_image.format.video.nFrameHeight,
      port_image.format.video.xFramerate / (float)(1<<16), interlace.eMode, m_deinterlace);

  // let OMXPlayerVideo know about resolution so it can inform RenderManager
  if (m_res_callback)
  {
    float display_aspect = 0.0f;
    if (pixel_aspect.nX && pixel_aspect.nY)
      display_aspect = (float)pixel_aspect.nX * port_image.format.video.nFrameWidth /
        ((float)pixel_aspect.nY * port_image.format.video.nFrameHeight);
    m_res_callback(m_res_ctx, port_image.format.video.nFrameWidth, port_image.format.video.nFrameHeight,
        port_image.format.video.xFramerate / (float)(1<<16), display_aspect);
  }

  if (m_settings_changed)
  {
    m_omx_decoder.EnablePort(m_omx_decoder.GetOutputPort(), true);
    return true;
  }

  if(!m_omx_render.Initialize("OMX.broadcom.video_render", OMX_IndexParamVideoInit))
    return false;

  m_omx_render.ResetEos();

  if(!m_omx_sched.Initialize("OMX.broadcom.video_scheduler", OMX_IndexParamVideoInit))
    return false;

  if(m_deinterlace)
  {
    if(!m_omx_image_fx.Initialize("OMX.broadcom.image_fx", OMX_IndexParamImageInit))
      return false;
  }

  OMX_CONFIG_DISPLAYREGIONTYPE configDisplay;
  OMX_INIT_STRUCTURE(configDisplay);
  configDisplay.nPortIndex = m_omx_render.GetInputPort();

  configDisplay.set = OMX_DISPLAY_SET_TRANSFORM;
  configDisplay.transform = m_transform;
  omx_err = m_omx_render.SetConfig(OMX_IndexConfigDisplayRegion, &configDisplay);
  if(omx_err != OMX_ErrorNone)
  {
    CLog::Log(LOGWARNING, "%s::%s - could not set transform : %d", CLASSNAME, __func__, m_transform);
    return false;
  }

  if(m_hdmi_clock_sync)
  {
    OMX_CONFIG_LATENCYTARGETTYPE latencyTarget;
    OMX_INIT_STRUCTURE(latencyTarget);
    latencyTarget.nPortIndex = m_omx_render.GetInputPort();
    latencyTarget.bEnabled = OMX_TRUE;
    latencyTarget.nFilter = 2;
    latencyTarget.nTarget = 4000;
    latencyTarget.nShift = 3;
    latencyTarget.nSpeedFactor = -135;
    latencyTarget.nInterFactor = 500;
    latencyTarget.nAdjCap = 20;

    omx_err = m_omx_render.SetConfig(OMX_IndexConfigLatencyTarget, &latencyTarget);
    if (omx_err != OMX_ErrorNone)
    {
      CLog::Log(LOGERROR, "%s::%s - OMX_IndexConfigLatencyTarget omx_err(0%08x)", CLASSNAME, __func__, omx_err);
      return false;
    }
  }

  if(m_deinterlace)
  {
    EINTERLACEMETHOD interlace_method = g_renderManager.AutoInterlaceMethod(CMediaSettings::Get().GetCurrentVideoSettings().m_InterlaceMethod);
    bool advanced_deinterlace = (interlace_method == VS_INTERLACEMETHOD_MMAL_ADVANCED || interlace_method == VS_INTERLACEMETHOD_MMAL_ADVANCED_HALF) &&
        port_image.format.video.nFrameWidth * port_image.format.video.nFrameHeight <= 576 * 720;
    bool half_framerate = interlace_method == VS_INTERLACEMETHOD_MMAL_ADVANCED_HALF || interlace_method == VS_INTERLACEMETHOD_MMAL_BOB_HALF;
    if (!advanced_deinterlace)
    {
      // Image_fx assumed 3 frames of context. simple deinterlace doesn't require this
      OMX_PARAM_U32TYPE extra_buffers;
      OMX_INIT_STRUCTURE(extra_buffers);
      extra_buffers.nU32 = -2;

      omx_err = m_omx_image_fx.SetParameter(OMX_IndexParamBrcmExtraBuffers, &extra_buffers);
      if(omx_err != OMX_ErrorNone)
      {
        CLog::Log(LOGERROR, "%s::%s error OMX_IndexParamBrcmExtraBuffers omx_err(0x%08x)", CLASSNAME, __func__, omx_err);
        return false;
      }
    }

    OMX_CONFIG_IMAGEFILTERPARAMSTYPE image_filter;
    OMX_INIT_STRUCTURE(image_filter);

    image_filter.nPortIndex = m_omx_image_fx.GetOutputPort();
    image_filter.nNumParams = 3;
    image_filter.nParams[0] = 3;
    image_filter.nParams[1] = 0;
    image_filter.nParams[2] = half_framerate;
    if (!advanced_deinterlace)
      image_filter.eImageFilter = OMX_ImageFilterDeInterlaceFast;
    else
      image_filter.eImageFilter = OMX_ImageFilterDeInterlaceAdvanced;

    omx_err = m_omx_image_fx.SetConfig(OMX_IndexConfigCommonImageFilterParameters, &image_filter);
    if(omx_err != OMX_ErrorNone)
    {
      CLog::Log(LOGERROR, "%s::%s - OMX_IndexConfigCommonImageFilterParameters omx_err(0x%08x)", CLASSNAME, __func__, omx_err);
      return false;
    }
  }

  if(m_deinterlace)
  {
    m_omx_tunnel_decoder.Initialize(&m_omx_decoder, m_omx_decoder.GetOutputPort(), &m_omx_image_fx, m_omx_image_fx.GetInputPort());
    m_omx_tunnel_image_fx.Initialize(&m_omx_image_fx, m_omx_image_fx.GetOutputPort(), &m_omx_sched, m_omx_sched.GetInputPort());
  }
  else
  {
    m_omx_tunnel_decoder.Initialize(&m_omx_decoder, m_omx_decoder.GetOutputPort(), &m_omx_sched, m_omx_sched.GetInputPort());
  }
  m_omx_tunnel_sched.Initialize(&m_omx_sched, m_omx_sched.GetOutputPort(), &m_omx_render, m_omx_render.GetInputPort());

  m_omx_tunnel_clock.Initialize(m_omx_clock, m_omx_clock->GetInputPort()  + 1, &m_omx_sched, m_omx_sched.GetOutputPort()  + 1);

  omx_err = m_omx_tunnel_clock.Establish();
  if(omx_err != OMX_ErrorNone)
  {
    CLog::Log(LOGERROR, "%s::%s - m_omx_tunnel_clock.Establish omx_err(0x%08x)", CLASSNAME, __func__, omx_err);
    return false;
  }

  omx_err = m_omx_tunnel_decoder.Establish();
  if(omx_err != OMX_ErrorNone)
  {
    CLog::Log(LOGERROR, "%s::%s - m_omx_tunnel_decoder.Establish omx_err(0x%08x)", CLASSNAME, __func__, omx_err);
    return false;
  }

  if(m_deinterlace)
  {
    omx_err = m_omx_tunnel_image_fx.Establish();
    if(omx_err != OMX_ErrorNone)
    {
      CLog::Log(LOGERROR, "%s::%s - m_omx_tunnel_image_fx.Establish omx_err(0x%08x)", CLASSNAME, __func__, omx_err);
      return false;
    }

    omx_err = m_omx_image_fx.SetStateForComponent(OMX_StateExecuting);
    if (omx_err != OMX_ErrorNone)
    {
      CLog::Log(LOGERROR, "%s::%s - m_omx_image_fx.SetStateForComponent omx_err(0x%08x)", CLASSNAME, __func__, omx_err);
      return false;
    }
  }

  omx_err = m_omx_tunnel_sched.Establish();
  if(omx_err != OMX_ErrorNone)
  {
    CLog::Log(LOGERROR, "%s::%s - m_omx_tunnel_sched.Establish omx_err(0x%08x)", CLASSNAME, __func__, omx_err);
    return false;
  }

  omx_err = m_omx_sched.SetStateForComponent(OMX_StateExecuting);
  if (omx_err != OMX_ErrorNone)
  {
    CLog::Log(LOGERROR, "%s::%s - m_omx_sched.SetStateForComponent omx_err(0x%08x)", CLASSNAME, __func__, omx_err);
    return false;
  }

  omx_err = m_omx_render.SetStateForComponent(OMX_StateExecuting);
  if (omx_err != OMX_ErrorNone)
  {
    CLog::Log(LOGERROR, "%s::%s - m_omx_render.SetStateForComponent omx_err(0x%08x)", CLASSNAME, __func__, omx_err);
    return false;
  }

  m_settings_changed = true;
  return true;
}

bool COMXVideo::Open(CDVDStreamInfo &hints, OMXClock *clock, EDEINTERLACEMODE deinterlace, bool hdmi_clock_sync)
{
  CSingleLock lock (m_critSection);
  bool vflip = false;
  Close();
  OMX_ERRORTYPE omx_err   = OMX_ErrorNone;
  std::string decoder_name;
  m_settings_changed = false;
  m_setStartTime = true;

  m_res_ctx           = NULL;
  m_res_callback      = NULL;

  m_video_codec_name      = "";
  m_codingType            = OMX_VIDEO_CodingUnused;

  m_decoded_width  = hints.width;
  m_decoded_height = hints.height;

  m_hdmi_clock_sync = hdmi_clock_sync;
  m_submitted_eos = false;
  m_failed_eos    = false;

  if(hints.extrasize > 0 && hints.extradata != NULL)
  {
    m_extrasize = hints.extrasize;
    m_extradata = (uint8_t *)malloc(m_extrasize);
    memcpy(m_extradata, hints.extradata, hints.extrasize);
  }

  switch (hints.codec)
  {
    case AV_CODEC_ID_H264:
    {
      switch(hints.profile)
      {
        case FF_PROFILE_H264_BASELINE:
          // (role name) video_decoder.avc
          // H.264 Baseline profile
          decoder_name = OMX_H264BASE_DECODER;
          m_codingType = OMX_VIDEO_CodingAVC;
          m_video_codec_name = "omx-h264";
          break;
        case FF_PROFILE_H264_MAIN:
          // (role name) video_decoder.avc
          // H.264 Main profile
          decoder_name = OMX_H264MAIN_DECODER;
          m_codingType = OMX_VIDEO_CodingAVC;
          m_video_codec_name = "omx-h264";
          break;
        case FF_PROFILE_H264_HIGH:
          // (role name) video_decoder.avc
          // H.264 Main profile
          decoder_name = OMX_H264HIGH_DECODER;
          m_codingType = OMX_VIDEO_CodingAVC;
          m_video_codec_name = "omx-h264";
          break;
        case FF_PROFILE_UNKNOWN:
          decoder_name = OMX_H264HIGH_DECODER;
          m_codingType = OMX_VIDEO_CodingAVC;
          m_video_codec_name = "omx-h264";
          break;
        default:
          decoder_name = OMX_H264HIGH_DECODER;
          m_codingType = OMX_VIDEO_CodingAVC;
          m_video_codec_name = "omx-h264";
          break;
      }
    }
    break;
    case AV_CODEC_ID_MPEG4:
      // (role name) video_decoder.mpeg4
      // MPEG-4, DivX 4/5 and Xvid compatible
      decoder_name = OMX_MPEG4_DECODER;
      m_codingType = OMX_VIDEO_CodingMPEG4;
      m_video_codec_name = "omx-mpeg4";
      break;
    case AV_CODEC_ID_MPEG1VIDEO:
    case AV_CODEC_ID_MPEG2VIDEO:
      // (role name) video_decoder.mpeg2
      // MPEG-2
      decoder_name = OMX_MPEG2V_DECODER;
      m_codingType = OMX_VIDEO_CodingMPEG2;
      m_video_codec_name = "omx-mpeg2";
      break;
    case AV_CODEC_ID_H263:
      // (role name) video_decoder.mpeg4
      // MPEG-4, DivX 4/5 and Xvid compatible
      decoder_name = OMX_MPEG4_DECODER;
      m_codingType = OMX_VIDEO_CodingMPEG4;
      m_video_codec_name = "omx-h263";
      break;
    case AV_CODEC_ID_VP6:
      // this form is encoded upside down
      vflip = true;
      // fall through
    case AV_CODEC_ID_VP6F:
    case AV_CODEC_ID_VP6A:
      // (role name) video_decoder.vp6
      // VP6
      decoder_name = OMX_VP6_DECODER;
      m_codingType = OMX_VIDEO_CodingVP6;
      m_video_codec_name = "omx-vp6";
    break;
    case AV_CODEC_ID_VP8:
      // (role name) video_decoder.vp8
      // VP8
      decoder_name = OMX_VP8_DECODER;
      m_codingType = OMX_VIDEO_CodingVP8;
      m_video_codec_name = "omx-vp8";
    break;
    case AV_CODEC_ID_THEORA:
      // (role name) video_decoder.theora
      // theora
      decoder_name = OMX_THEORA_DECODER;
      m_codingType = OMX_VIDEO_CodingTheora;
      m_video_codec_name = "omx-theora";
    break;
    case AV_CODEC_ID_MJPEG:
    case AV_CODEC_ID_MJPEGB:
      // (role name) video_decoder.mjpg
      // mjpg
      decoder_name = OMX_MJPEG_DECODER;
      m_codingType = OMX_VIDEO_CodingMJPEG;
      m_video_codec_name = "omx-mjpeg";
    break;
    case AV_CODEC_ID_VC1:
    case AV_CODEC_ID_WMV3:
      // (role name) video_decoder.vc1
      // VC-1, WMV9
      decoder_name = OMX_VC1_DECODER;
      m_codingType = OMX_VIDEO_CodingWMV;
      m_video_codec_name = "omx-vc1";
      break;
    default:
      CLog::Log(LOGERROR, "COMXVideo::Open : Video codec unknown: %x", hints.codec);
      return false;
    break;
  }
  m_deinterlace_request = deinterlace;

  if(!m_omx_decoder.Initialize(decoder_name, OMX_IndexParamVideoInit))
    return false;

  if(clock == NULL)
    return false;

  m_av_clock = clock;
  m_omx_clock = m_av_clock->GetOMXClock();

  if(m_omx_clock->GetComponent() == NULL)
  {
    m_av_clock = NULL;
    m_omx_clock = NULL;
    return false;
  }

  omx_err = m_omx_decoder.SetStateForComponent(OMX_StateIdle);
  if (omx_err != OMX_ErrorNone)
  {
    CLog::Log(LOGERROR, "COMXVideo::Open m_omx_decoder.SetStateForComponent\n");
    return false;
  }

  OMX_VIDEO_PARAM_PORTFORMATTYPE formatType;
  OMX_INIT_STRUCTURE(formatType);
  formatType.nPortIndex = m_omx_decoder.GetInputPort();
  formatType.eCompressionFormat = m_codingType;

  if (hints.fpsscale > 0 && hints.fpsrate > 0)
  {
    formatType.xFramerate = (long long)(1<<16)*hints.fpsrate / hints.fpsscale;
  }
  else
  {
    formatType.xFramerate = 25 * (1<<16);
  }

  omx_err = m_omx_decoder.SetParameter(OMX_IndexParamVideoPortFormat, &formatType);
  if(omx_err != OMX_ErrorNone)
    return false;
  
  OMX_PARAM_PORTDEFINITIONTYPE portParam;
  OMX_INIT_STRUCTURE(portParam);
  portParam.nPortIndex = m_omx_decoder.GetInputPort();

  omx_err = m_omx_decoder.GetParameter(OMX_IndexParamPortDefinition, &portParam);
  if(omx_err != OMX_ErrorNone)
  {
    CLog::Log(LOGERROR, "COMXVideo::Open error OMX_IndexParamPortDefinition omx_err(0x%08x)\n", omx_err);
    return false;
  }

  portParam.nPortIndex = m_omx_decoder.GetInputPort();
  bool small_mem = g_RBP.GetArmMem() < 256;
  portParam.nBufferCountActual = small_mem ? VIDEO_BUFFERS:2*VIDEO_BUFFERS;
  portParam.format.video.nFrameWidth  = m_decoded_width;
  portParam.format.video.nFrameHeight = m_decoded_height;

  omx_err = m_omx_decoder.SetParameter(OMX_IndexParamPortDefinition, &portParam);
  if(omx_err != OMX_ErrorNone)
  {
    CLog::Log(LOGERROR, "COMXVideo::Open error OMX_IndexParamPortDefinition omx_err(0x%08x)\n", omx_err);
    return false;
  }

  // request portsettingschanged on aspect ratio change
  OMX_CONFIG_REQUESTCALLBACKTYPE notifications;
  OMX_INIT_STRUCTURE(notifications);
  notifications.nPortIndex = m_omx_decoder.GetOutputPort();
  notifications.nIndex = OMX_IndexParamBrcmPixelAspectRatio;
  notifications.bEnable = OMX_TRUE;

  omx_err = m_omx_decoder.SetParameter((OMX_INDEXTYPE)OMX_IndexConfigRequestCallback, &notifications);
  if (omx_err != OMX_ErrorNone)
  {
    CLog::Log(LOGERROR, "COMXVideo::Open OMX_IndexConfigRequestCallback error (0%08x)\n", omx_err);
    return false;
  }

  // request portsettingschanged on refresh rate change
  if (CSettings::Get().GetInt("videoplayer.adjustrefreshrate") == ADJUST_REFRESHRATE_ALWAYS)
  {
    notifications.nIndex = OMX_IndexParamPortDefinition;
    omx_err = m_omx_decoder.SetParameter((OMX_INDEXTYPE)OMX_IndexConfigRequestCallback, &notifications);
    if (omx_err != OMX_ErrorNone)
    {
      CLog::Log(LOGERROR, "COMXVideo::Open OMX_IndexConfigRequestCallback error (0%08x)\n", omx_err);
      //return false;
    }
  }
  OMX_PARAM_BRCMVIDEODECODEERRORCONCEALMENTTYPE concanParam;
  OMX_INIT_STRUCTURE(concanParam);
  if(g_advancedSettings.m_omxDecodeStartWithValidFrame)
    concanParam.bStartWithValidFrame = OMX_TRUE;
  else
    concanParam.bStartWithValidFrame = OMX_FALSE;

  omx_err = m_omx_decoder.SetParameter(OMX_IndexParamBrcmVideoDecodeErrorConcealment, &concanParam);
  if(omx_err != OMX_ErrorNone)
  {
    CLog::Log(LOGERROR, "COMXVideo::Open error OMX_IndexParamBrcmVideoDecodeErrorConcealment omx_err(0x%08x)\n", omx_err);
    return false;
  }

  if(NaluFormatStartCodes(hints.codec, m_extradata, m_extrasize))
  {
    OMX_NALSTREAMFORMATTYPE nalStreamFormat;
    OMX_INIT_STRUCTURE(nalStreamFormat);
    nalStreamFormat.nPortIndex = m_omx_decoder.GetInputPort();
    nalStreamFormat.eNaluFormat = OMX_NaluFormatStartCodes;

    omx_err = m_omx_decoder.SetParameter((OMX_INDEXTYPE)OMX_IndexParamNalStreamFormatSelect, &nalStreamFormat);
    if (omx_err != OMX_ErrorNone)
    {
      CLog::Log(LOGERROR, "COMXVideo::Open OMX_IndexParamNalStreamFormatSelect error (0%08x)\n", omx_err);
      return false;
    }
  }


  // Alloc buffers for the omx intput port.
  omx_err = m_omx_decoder.AllocInputBuffers();
  if (omx_err != OMX_ErrorNone)
  {
    CLog::Log(LOGERROR, "COMXVideo::Open AllocOMXInputBuffers error (0%08x)\n", omx_err);
    return false;
  }

  omx_err = m_omx_decoder.SetStateForComponent(OMX_StateExecuting);
  if (omx_err != OMX_ErrorNone)
  {
    CLog::Log(LOGERROR, "COMXVideo::Open error m_omx_decoder.SetStateForComponent\n");
    return false;
  }

  SendDecoderConfig();

  m_is_open           = true;
  m_drop_state        = false;

  switch(hints.orientation)
  {
    case 90:
      m_transform = OMX_DISPLAY_ROT90;
      break;
    case 180:
      m_transform = OMX_DISPLAY_ROT180;
      break;
    case 270:
      m_transform = OMX_DISPLAY_ROT270;
      break;
    default:
      m_transform = OMX_DISPLAY_ROT0;
      break;
  }
  if (vflip)
      m_transform = OMX_DISPLAY_MIRROR_ROT180;

  if(m_omx_decoder.BadState())
    return false;

  CLog::Log(LOGDEBUG,
    "%s::%s - decoder_component(0x%p), input_port(0x%x), output_port(0x%x) deinterlace %d hdmiclocksync %d\n",
    CLASSNAME, __func__, m_omx_decoder.GetComponent(), m_omx_decoder.GetInputPort(), m_omx_decoder.GetOutputPort(),
    m_deinterlace_request, m_hdmi_clock_sync);

  return true;
}


void COMXVideo::Close()
{
  CSingleLock lock (m_critSection);
  m_omx_tunnel_clock.Deestablish();
  m_omx_tunnel_decoder.Deestablish();
  if(m_deinterlace)
    m_omx_tunnel_image_fx.Deestablish();
  m_omx_tunnel_sched.Deestablish();

  m_omx_decoder.FlushInput();

  m_omx_sched.Deinitialize();
  m_omx_decoder.Deinitialize();
  if(m_deinterlace)
    m_omx_image_fx.Deinitialize();
  m_omx_render.Deinitialize();

  m_is_open       = false;

  if(m_extradata)
    free(m_extradata);
  m_extradata = NULL;
  m_extrasize = 0;

  m_video_codec_name  = "";
  m_deinterlace       = false;
  m_av_clock          = NULL;

  m_res_ctx           = NULL;
  m_res_callback      = NULL;
}

void COMXVideo::SetDropState(bool bDrop)
{
  m_drop_state = bDrop;
}

unsigned int COMXVideo::GetFreeSpace()
{
  CSingleLock lock (m_critSection);
  return m_omx_decoder.GetInputBufferSpace();
}

unsigned int COMXVideo::GetSize()
{
  CSingleLock lock (m_critSection);
  return m_omx_decoder.GetInputBufferSize();
}

bool COMXVideo::GetPlayerInfo(double &match, double &phase, double &pll)
{
  CSingleLock lock (m_critSection);
  OMX_ERRORTYPE omx_err;
  OMX_CONFIG_BRCMRENDERSTATSTYPE renderstats;

  if (!m_hdmi_clock_sync || !m_omx_render.IsInitialized())
    return false;
  OMX_INIT_STRUCTURE(renderstats);
  renderstats.nPortIndex = m_omx_render.GetInputPort();

  omx_err = m_omx_render.GetParameter(OMX_IndexConfigBrcmRenderStats, &renderstats);
  if(omx_err != OMX_ErrorNone)
  {
    CLog::Log(LOGERROR, "COMXVideo::GetPlayerInfo error GetParameter OMX_IndexParamPortDefinition omx_err(0x%08x)\n", omx_err);
    return false;
  }
  match = renderstats.nMatch * 1e-6;
  phase = (double)renderstats.nPhase / (double)renderstats.nPeriod;
  pll   = (double)renderstats.nPixelClock / (double)renderstats.nPixelClockNominal;
  return true;
}


int COMXVideo::Decode(uint8_t *pData, int iSize, double dts, double pts)
{
  CSingleLock lock (m_critSection);
  OMX_ERRORTYPE omx_err;

  if( m_drop_state || !m_is_open )
    return true;

  unsigned int demuxer_bytes = (unsigned int)iSize;
  uint8_t *demuxer_content = pData;

  if (demuxer_content && demuxer_bytes > 0)
  {
    while(demuxer_bytes)
    {
      // 500ms timeout
      OMX_BUFFERHEADERTYPE *omx_buffer = m_omx_decoder.GetInputBuffer(500);
      if(omx_buffer == NULL)
      {
        CLog::Log(LOGERROR, "OMXVideo::Decode timeout\n");
        return false;
      }

      omx_buffer->nFlags = 0;
      omx_buffer->nOffset = 0;
      omx_buffer->nTimeStamp = ToOMXTime((uint64_t)(pts != DVD_NOPTS_VALUE ? pts : dts != DVD_NOPTS_VALUE ? dts : 0));

      if(m_setStartTime)
      {
        omx_buffer->nFlags |= OMX_BUFFERFLAG_STARTTIME;
        CLog::Log(LOGDEBUG, "OMXVideo::Decode VDec : setStartTime %f\n", (pts == DVD_NOPTS_VALUE ? 0.0 : pts) / DVD_TIME_BASE);
        m_setStartTime = false;
      }
      else if (pts == DVD_NOPTS_VALUE && dts == DVD_NOPTS_VALUE)
        omx_buffer->nFlags |= OMX_BUFFERFLAG_TIME_UNKNOWN;
      else if (pts == DVD_NOPTS_VALUE)
        omx_buffer->nFlags |= OMX_BUFFERFLAG_TIME_IS_DTS;

      omx_buffer->nFilledLen = std::min((OMX_U32)demuxer_bytes, omx_buffer->nAllocLen);
      memcpy(omx_buffer->pBuffer, demuxer_content, omx_buffer->nFilledLen);

      demuxer_bytes -= omx_buffer->nFilledLen;
      demuxer_content += omx_buffer->nFilledLen;

      if(demuxer_bytes == 0)
        omx_buffer->nFlags |= OMX_BUFFERFLAG_ENDOFFRAME;

      omx_err = m_omx_decoder.EmptyThisBuffer(omx_buffer);
      if (omx_err != OMX_ErrorNone)
      {
        CLog::Log(LOGERROR, "%s::%s - OMX_EmptyThisBuffer() failed with result(0x%x)\n", CLASSNAME, __func__, omx_err);
        m_omx_decoder.DecoderEmptyBufferDone(m_omx_decoder.GetComponent(), omx_buffer);
        return false;
      }
      //CLog::Log(LOGINFO, "VideD: dts:%.0f pts:%.0f size:%d)\n", dts, pts, iSize);

      omx_err = m_omx_decoder.WaitForEvent(OMX_EventPortSettingsChanged, 0);
      if (omx_err == OMX_ErrorNone)
      {
        if(!PortSettingsChanged())
        {
          CLog::Log(LOGERROR, "%s::%s - error PortSettingsChanged omx_err(0x%08x)\n", CLASSNAME, __func__, omx_err);
          return false;
        }
      }
      omx_err = m_omx_decoder.WaitForEvent(OMX_EventParamOrConfigChanged, 0);
      if (omx_err == OMX_ErrorNone)
      {
        if(!PortSettingsChanged())
        {
          CLog::Log(LOGERROR, "%s::%s - error PortSettingsChanged (EventParamOrConfigChanged) omx_err(0x%08x)\n", CLASSNAME, __func__, omx_err);
        }
      }
    }
    return true;

  }
  
  return false;
}

void COMXVideo::Reset(void)
{
  CSingleLock lock (m_critSection);
  if(!m_is_open)
    return;

  m_setStartTime = true;
  m_omx_decoder.FlushAll();
  if(m_deinterlace)
    m_omx_image_fx.FlushAll();
  m_omx_sched.FlushAll();
}

///////////////////////////////////////////////////////////////////////////////////////////
void COMXVideo::SetVideoRect(const CRect& SrcRect, const CRect& DestRect, RENDER_STEREO_MODE video_mode, RENDER_STEREO_MODE display_mode)
{
  CSingleLock lock (m_critSection);
  if(!m_is_open)
    return;

  OMX_CONFIG_DISPLAYREGIONTYPE configDisplay;

  OMX_INIT_STRUCTURE(configDisplay);
  configDisplay.nPortIndex = m_omx_render.GetInputPort();
  configDisplay.set                 = (OMX_DISPLAYSETTYPE)(OMX_DISPLAY_SET_DEST_RECT|OMX_DISPLAY_SET_SRC_RECT|OMX_DISPLAY_SET_FULLSCREEN|OMX_DISPLAY_SET_NOASPECT|OMX_DISPLAY_SET_MODE);
  configDisplay.dest_rect.x_offset  = lrintf(DestRect.x1);
  configDisplay.dest_rect.y_offset  = lrintf(DestRect.y1);
  configDisplay.dest_rect.width     = lrintf(DestRect.Width());
  configDisplay.dest_rect.height    = lrintf(DestRect.Height());

  configDisplay.src_rect.x_offset   = lrintf(SrcRect.x1);
  configDisplay.src_rect.y_offset   = lrintf(SrcRect.y1);
  configDisplay.src_rect.width      = lrintf(SrcRect.Width());
  configDisplay.src_rect.height     = lrintf(SrcRect.Height());

  configDisplay.fullscreen = OMX_FALSE;
  configDisplay.noaspect = OMX_TRUE;

  if (video_mode == RENDER_STEREO_MODE_SPLIT_HORIZONTAL && display_mode == RENDER_STEREO_MODE_SPLIT_HORIZONTAL)
    configDisplay.mode = OMX_DISPLAY_MODE_STEREO_TOP_TO_TOP;
  else if (video_mode == RENDER_STEREO_MODE_SPLIT_HORIZONTAL && display_mode == RENDER_STEREO_MODE_SPLIT_VERTICAL)
    configDisplay.mode = OMX_DISPLAY_MODE_STEREO_TOP_TO_LEFT;
  else if (video_mode == RENDER_STEREO_MODE_SPLIT_VERTICAL && display_mode == RENDER_STEREO_MODE_SPLIT_HORIZONTAL)
    configDisplay.mode = OMX_DISPLAY_MODE_STEREO_LEFT_TO_TOP;
  else if (video_mode == RENDER_STEREO_MODE_SPLIT_VERTICAL && display_mode == RENDER_STEREO_MODE_SPLIT_VERTICAL)
    configDisplay.mode = OMX_DISPLAY_MODE_STEREO_LEFT_TO_LEFT;
  else
    configDisplay.mode = OMX_DISPLAY_MODE_LETTERBOX;

  m_omx_render.SetConfig(OMX_IndexConfigDisplayRegion, &configDisplay);

  CLog::Log(LOGDEBUG, "%s::%s %d,%d,%d,%d -> %d,%d,%d,%d mode:%d", CLASSNAME, __func__,
      configDisplay.src_rect.x_offset, configDisplay.src_rect.y_offset, configDisplay.src_rect.width, configDisplay.src_rect.height,
      configDisplay.dest_rect.x_offset, configDisplay.dest_rect.y_offset, configDisplay.dest_rect.width, configDisplay.dest_rect.height, configDisplay.mode);
}

int COMXVideo::GetInputBufferSize()
{
  CSingleLock lock (m_critSection);
  return m_omx_decoder.GetInputBufferSize();
}

void COMXVideo::SubmitEOS()
{
  CSingleLock lock (m_critSection);
  if(!m_is_open)
    return;

  m_submitted_eos = true;
  m_failed_eos = false;

  OMX_ERRORTYPE omx_err = OMX_ErrorNone;
  OMX_BUFFERHEADERTYPE *omx_buffer = m_omx_decoder.GetInputBuffer(1000);
  
  if(omx_buffer == NULL)
  {
    CLog::Log(LOGERROR, "%s::%s - buffer error 0x%08x", CLASSNAME, __func__, omx_err);
    m_failed_eos = true;
    return;
  }
  
  omx_buffer->nOffset     = 0;
  omx_buffer->nFilledLen  = 0;
  omx_buffer->nTimeStamp  = ToOMXTime(0LL);

  omx_buffer->nFlags = OMX_BUFFERFLAG_ENDOFFRAME | OMX_BUFFERFLAG_EOS | OMX_BUFFERFLAG_TIME_UNKNOWN;
  
  omx_err = m_omx_decoder.EmptyThisBuffer(omx_buffer);
  if (omx_err != OMX_ErrorNone)
  {
    CLog::Log(LOGERROR, "%s::%s - OMX_EmptyThisBuffer() failed with result(0x%x)\n", CLASSNAME, __func__, omx_err);
    m_omx_decoder.DecoderEmptyBufferDone(m_omx_decoder.GetComponent(), omx_buffer);
    return;
  }
  CLog::Log(LOGINFO, "%s::%s", CLASSNAME, __func__);
}

bool COMXVideo::IsEOS()
{
  CSingleLock lock (m_critSection);
  if(!m_is_open)
    return true;
  if (!m_failed_eos && !m_omx_render.IsEOS())
    return false;
  if (m_submitted_eos)
  {
    CLog::Log(LOGINFO, "%s::%s", CLASSNAME, __func__);
    m_submitted_eos = false;
  }
  return true;
}
