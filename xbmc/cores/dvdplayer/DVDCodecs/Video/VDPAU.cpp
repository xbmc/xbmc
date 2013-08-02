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

#include "system.h"
#ifdef HAVE_LIBVDPAU
#include <dlfcn.h>
#include "windowing/WindowingFactory.h"
#include "VDPAU.h"
#include "guilib/GraphicContext.h"
#include "guilib/TextureManager.h"
#include "cores/VideoRenderers/RenderManager.h"
#include "DVDVideoCodecFFmpeg.h"
#include "DVDClock.h"
#include "settings/Settings.h"
#include "settings/AdvancedSettings.h"
#include "settings/MediaSettings.h"
#include "Application.h"
#include "utils/MathUtils.h"
#include "utils/TimeUtils.h"
#include "DVDCodecs/DVDCodecUtils.h"
#include "cores/VideoRenderers/RenderFlags.h"

using namespace VDPAU;
#define NUM_RENDER_PICS 9

#define ARSIZE(x) (sizeof(x) / sizeof((x)[0]))

CDecoder::Desc decoder_profiles[] = {
{"MPEG1",        VDP_DECODER_PROFILE_MPEG1},
{"MPEG2_SIMPLE", VDP_DECODER_PROFILE_MPEG2_SIMPLE},
{"MPEG2_MAIN",   VDP_DECODER_PROFILE_MPEG2_MAIN},
{"H264_BASELINE",VDP_DECODER_PROFILE_H264_BASELINE},
{"H264_MAIN",    VDP_DECODER_PROFILE_H264_MAIN},
{"H264_HIGH",    VDP_DECODER_PROFILE_H264_HIGH},
{"VC1_SIMPLE",   VDP_DECODER_PROFILE_VC1_SIMPLE},
{"VC1_MAIN",     VDP_DECODER_PROFILE_VC1_MAIN},
{"VC1_ADVANCED", VDP_DECODER_PROFILE_VC1_ADVANCED},
#ifdef VDP_DECODER_PROFILE_MPEG4_PART2_ASP
{"MPEG4_PART2_ASP", VDP_DECODER_PROFILE_MPEG4_PART2_ASP},
#endif
};
const size_t decoder_profile_count = sizeof(decoder_profiles)/sizeof(CDecoder::Desc);

static struct SInterlaceMapping
{
  const EINTERLACEMETHOD     method;
  const VdpVideoMixerFeature feature;
} g_interlace_mapping[] = 
{ {VS_INTERLACEMETHOD_VDPAU_TEMPORAL             , VDP_VIDEO_MIXER_FEATURE_DEINTERLACE_TEMPORAL}
, {VS_INTERLACEMETHOD_VDPAU_TEMPORAL_HALF        , VDP_VIDEO_MIXER_FEATURE_DEINTERLACE_TEMPORAL}
, {VS_INTERLACEMETHOD_VDPAU_TEMPORAL_SPATIAL     , VDP_VIDEO_MIXER_FEATURE_DEINTERLACE_TEMPORAL_SPATIAL}
, {VS_INTERLACEMETHOD_VDPAU_TEMPORAL_SPATIAL_HALF, VDP_VIDEO_MIXER_FEATURE_DEINTERLACE_TEMPORAL_SPATIAL}
, {VS_INTERLACEMETHOD_VDPAU_INVERSE_TELECINE     , VDP_VIDEO_MIXER_FEATURE_INVERSE_TELECINE}
, {VS_INTERLACEMETHOD_NONE                       , (VdpVideoMixerFeature)-1}
};

static float studioCSCKCoeffs601[3] = {0.299, 0.587, 0.114}; //BT601 {Kr, Kg, Kb}
static float studioCSCKCoeffs709[3] = {0.2126, 0.7152, 0.0722}; //BT709 {Kr, Kg, Kb}

//since libvdpau 0.4, vdp_device_create_x11() installs a callback on the Display*,
//if we unload libvdpau with dlclose(), we segfault on XCloseDisplay,
//so we just keep a static handle to libvdpau around
void* CDecoder::dl_handle;

//-----------------------------------------------------------------------------
// CVDPAU
//-----------------------------------------------------------------------------

CDecoder::CDecoder() : m_vdpauOutput(&m_inMsgEvent)
{
  m_vdpauConfig.vdpDevice = VDP_INVALID_HANDLE;
  m_vdpauConfig.videoSurfaces = &m_videoSurfaces;
  m_vdpauConfig.videoSurfaceSec = &m_videoSurfaceSec;

  m_vdpauConfigured = false;
  m_hwContext.bitstream_buffers_allocated = 0;
  m_DisplayState = VDPAU_OPEN;
}

bool CDecoder::Open(AVCodecContext* avctx, const enum PixelFormat, unsigned int surfaces)
{
  if(avctx->coded_width  == 0
  || avctx->coded_height == 0)
  {
    CLog::Log(LOGWARNING,"(VDPAU) no width/height available, can't init");
    return false;
  }
  m_vdpauConfig.numRenderBuffers = surfaces;
  m_decoderThread = CThread::GetCurrentThreadId();

  if ((avctx->codec_id == AV_CODEC_ID_MPEG4) && !g_advancedSettings.m_videoAllowMpeg4VDPAU)
    return false;

  if (!dl_handle)
  {
    dl_handle  = dlopen("libvdpau.so.1", RTLD_LAZY);
    if (!dl_handle)
    {
      const char* error = dlerror();
      if (!error)
        error = "dlerror() returned NULL";

      CLog::Log(LOGNOTICE,"(VDPAU) Unable to get handle to libvdpau: %s", error);
      return false;
    }
  }

  if (!m_dllAvUtil.Load())
    return false;

  InitVDPAUProcs();
  m_presentPicture = 0;

  if (m_vdpauConfig.vdpDevice != VDP_INVALID_HANDLE)
  {
    SpewHardwareAvailable();

    VdpDecoderProfile profile = 0;
    if(avctx->codec_id == AV_CODEC_ID_H264)
      profile = VDP_DECODER_PROFILE_H264_HIGH;
#ifdef VDP_DECODER_PROFILE_MPEG4_PART2_ASP
    else if(avctx->codec_id == AV_CODEC_ID_MPEG4)
      profile = VDP_DECODER_PROFILE_MPEG4_PART2_ASP;
#endif
    if(profile)
    {
      if (!CDVDCodecUtils::IsVP3CompatibleWidth(avctx->coded_width))
        CLog::Log(LOGWARNING,"(VDPAU) width %i might not be supported because of hardware bug", avctx->width);
   
      /* attempt to create a decoder with this width/height, some sizes are not supported by hw */
      VdpStatus vdp_st;
      vdp_st = m_vdpauConfig.vdpProcs.vdp_decoder_create(m_vdpauConfig.vdpDevice, profile, avctx->coded_width, avctx->coded_height, 5, &m_vdpauConfig.vdpDecoder);

      if(vdp_st != VDP_STATUS_OK)
      {
        CLog::Log(LOGERROR, " (VDPAU) Error: %s(%d) checking for decoder support\n", m_vdpauConfig.vdpProcs.vdp_get_error_string(vdp_st), vdp_st);
        FiniVDPAUProcs();
        return false;
      }

      m_vdpauConfig.vdpProcs.vdp_decoder_destroy(m_vdpauConfig.vdpDecoder);
      CheckStatus(vdp_st, __LINE__);
    }

    /* finally setup ffmpeg */
    memset(&m_hwContext, 0, sizeof(AVVDPAUContext));
    m_hwContext.render = CDecoder::Render;
    m_hwContext.bitstream_buffers_allocated = 0;
    avctx->get_buffer      = CDecoder::FFGetBuffer;
    avctx->release_buffer  = CDecoder::FFReleaseBuffer;
    avctx->draw_horiz_band = CDecoder::FFDrawSlice;
    avctx->slice_flags=SLICE_FLAG_CODED_ORDER|SLICE_FLAG_ALLOW_FIELD;
    avctx->hwaccel_context = &m_hwContext;
    avctx->thread_count    = 1;

    g_Windowing.Register(this);
    return true;
  }
  return false;
}

CDecoder::~CDecoder()
{
  Close();
}

void CDecoder::Close()
{
  CLog::Log(LOGNOTICE, " (VDPAU) %s", __FUNCTION__);

  g_Windowing.Unregister(this);

  CSingleLock lock(m_DecoderSection);

  FiniVDPAUOutput();
  FiniVDPAUProcs();
  m_vdpauOutput.Dispose();

  while (!m_videoSurfaces.empty())
  {
    vdpau_render_state *render = m_videoSurfaces.back();
    m_videoSurfaces.pop_back();
    if (render->bitstream_buffers_allocated)
      m_dllAvUtil.av_freep(&render->bitstream_buffers);
    render->bitstream_buffers_allocated = 0;
    free(render);
  }

  if (m_hwContext.bitstream_buffers_allocated)
  {
    m_dllAvUtil.av_freep(&m_hwContext.bitstream_buffers);
  }

  m_dllAvUtil.Unload();
}

long CDecoder::Release()
{
  // check if we should do some pre-cleanup here
  // a second decoder might need resources
  if (m_vdpauConfigured == true)
  {
    CSingleLock lock(m_DecoderSection);
    CLog::Log(LOGNOTICE,"CVDPAU::Release pre-cleanup");

    Message *reply;
    if (m_vdpauOutput.m_controlPort.SendOutMessageSync(COutputControlProtocol::PRECLEANUP,
                                                   &reply,
                                                   2000))
    {
      bool success = reply->signal == COutputControlProtocol::ACC ? true : false;
      reply->Release();
      if (!success)
      {
        CLog::Log(LOGERROR, "VDPAU::%s - pre-cleanup returned error", __FUNCTION__);
        m_DisplayState = VDPAU_ERROR;
      }
    }
    else
    {
      CLog::Log(LOGERROR, "VDPAU::%s - pre-cleanup timed out", __FUNCTION__);
      m_DisplayState = VDPAU_ERROR;
    }

    for(unsigned int i = 0; i < m_videoSurfaces.size(); ++i)
    {
      vdpau_render_state *render = m_videoSurfaces[i];
      if (render->surface != VDP_INVALID_HANDLE && !(render->state & FF_VDPAU_STATE_USED_FOR_RENDER))
      {
        m_vdpauConfig.vdpProcs.vdp_video_surface_destroy(render->surface);
        render->surface = VDP_INVALID_HANDLE;
      }
    }
  }
  return IHardwareDecoder::Release();
}

long CDecoder::ReleasePicReference()
{
  return IHardwareDecoder::Release();
}

void CDecoder::SetWidthHeight(int width, int height)
{
  m_vdpauConfig.upscale = g_advancedSettings.m_videoVDPAUScaling;

  //pick the smallest dimensions, so we downscale with vdpau and upscale with opengl when appropriate
  //this requires the least amount of gpu memory bandwidth
  if (g_graphicsContext.GetWidth() < width || g_graphicsContext.GetHeight() < height || m_vdpauConfig.upscale >= 0)
  {
    //scale width to desktop size if the aspect ratio is the same or bigger than the desktop
    if ((double)height * g_graphicsContext.GetWidth() / width <= (double)g_graphicsContext.GetHeight())
    {
      m_vdpauConfig.outWidth = g_graphicsContext.GetWidth();
      m_vdpauConfig.outHeight = MathUtils::round_int((double)height * g_graphicsContext.GetWidth() / width);
    }
    else //scale height to the desktop size if the aspect ratio is smaller than the desktop
    {
      m_vdpauConfig.outHeight = g_graphicsContext.GetHeight();
      m_vdpauConfig.outWidth = MathUtils::round_int((double)width * g_graphicsContext.GetHeight() / height);
    }
  }
  else
  { //let opengl scale
    m_vdpauConfig.outWidth = width;
    m_vdpauConfig.outHeight = height;
  }
  CLog::Log(LOGDEBUG, "CVDPAU::SetWidthHeight Setting OutWidth: %i OutHeight: %i", m_vdpauConfig.outWidth, m_vdpauConfig.outHeight);
}

void CDecoder::OnLostDevice()
{
  CLog::Log(LOGNOTICE,"CVDPAU::OnLostDevice event");

  CSingleLock lock(m_DecoderSection);
  FiniVDPAUOutput();
  FiniVDPAUProcs();

  m_DisplayState = VDPAU_LOST;
  lock.Leave();
  m_DisplayEvent.Reset();
}

void CDecoder::OnResetDevice()
{
  CLog::Log(LOGNOTICE,"CVDPAU::OnResetDevice event");

  CSingleLock lock(m_DecoderSection);
  if (m_DisplayState == VDPAU_LOST)
  {
    m_DisplayState = VDPAU_RESET;
    lock.Leave();
    m_DisplayEvent.Set();
  }
}

int CDecoder::Check(AVCodecContext* avctx)
{
  EDisplayState state;

  { CSingleLock lock(m_DecoderSection);
    state = m_DisplayState;
  }

  if (state == VDPAU_LOST)
  {
    CLog::Log(LOGNOTICE,"CVDPAU::Check waiting for display reset event");
    if (!m_DisplayEvent.WaitMSec(2000))
    {
      CLog::Log(LOGERROR, "CVDPAU::Check - device didn't reset in reasonable time");
      state = VDPAU_RESET;
    }
    else
    {
      CSingleLock lock(m_DecoderSection);
      state = m_DisplayState;
    }
  }
  if (state == VDPAU_RESET || state == VDPAU_ERROR)
  {
    CSingleLock lock(m_DecoderSection);

    FiniVDPAUOutput();
    FiniVDPAUProcs();

    InitVDPAUProcs();

    if (state == VDPAU_RESET)
      return VC_FLUSHED;
    else
      return VC_ERROR;
  }
  return 0;
}

bool CDecoder::IsVDPAUFormat(PixelFormat format)
{
  if (format == AV_PIX_FMT_VDPAU)
    return true;
  else
    return false;
}

bool CDecoder::Supports(VdpVideoMixerFeature feature)
{
  for(int i = 0; i < m_vdpauConfig.featureCount; i++)
  {
    if(m_vdpauConfig.vdpFeatures[i] == feature)
      return true;
  }
  return false;
}

bool CDecoder::Supports(EINTERLACEMETHOD method)
{
  if(method == VS_INTERLACEMETHOD_VDPAU_BOB
  || method == VS_INTERLACEMETHOD_AUTO)
    return true;

  if (!m_vdpauConfig.usePixmaps)
  {
    if (method == VS_INTERLACEMETHOD_RENDER_BOB)
      return true;
  }

  if (method == VS_INTERLACEMETHOD_VDPAU_INVERSE_TELECINE)
    return false;

  for(SInterlaceMapping* p = g_interlace_mapping; p->method != VS_INTERLACEMETHOD_NONE; p++)
  {
    if(p->method == method)
      return Supports(p->feature);
  }
  return false;
}

EINTERLACEMETHOD CDecoder::AutoInterlaceMethod()
{
  return VS_INTERLACEMETHOD_RENDER_BOB;
}

void CDecoder::InitVDPAUProcs()
{
  char* error;

  (void)dlerror();
  dl_vdp_device_create_x11 = (VdpStatus (*)(Display*, int, VdpDevice*, VdpStatus (**)(VdpDevice, VdpFuncId, void**)))dlsym(dl_handle, (const char*)"vdp_device_create_x11");
  error = dlerror();
  if (error)
  {
    CLog::Log(LOGERROR,"(VDPAU) - %s in %s",error,__FUNCTION__);
    m_vdpauConfig.vdpDevice = VDP_INVALID_HANDLE;
    return;
  }

  if (dl_vdp_device_create_x11)
  {
    m_Display = XOpenDisplay(NULL);
  }

  int mScreen = g_Windowing.GetCurrentScreen();
  VdpStatus vdp_st;

  // Create Device
  vdp_st = dl_vdp_device_create_x11(m_Display, //x_display,
                                 mScreen, //x_screen,
                                 &m_vdpauConfig.vdpDevice,
                                 &m_vdpauConfig.vdpProcs.vdp_get_proc_address);

  CLog::Log(LOGNOTICE,"vdp_device = 0x%08x vdp_st = 0x%08x",m_vdpauConfig.vdpDevice,vdp_st);
  if (vdp_st != VDP_STATUS_OK)
  {
    CLog::Log(LOGERROR,"(VDPAU) unable to init VDPAU - vdp_st = 0x%x.  Falling back.",vdp_st);
    m_vdpauConfig.vdpDevice = VDP_INVALID_HANDLE;
    return;
  }

#define VDP_PROC(id, proc) \
  do { \
    vdp_st = m_vdpauConfig.vdpProcs.vdp_get_proc_address(m_vdpauConfig.vdpDevice, id, (void**)&proc); \
    CheckStatus(vdp_st, __LINE__); \
  } while(0);

  VDP_PROC(VDP_FUNC_ID_GET_ERROR_STRING                    , m_vdpauConfig.vdpProcs.vdp_get_error_string);
  VDP_PROC(VDP_FUNC_ID_DEVICE_DESTROY                      , m_vdpauConfig.vdpProcs.vdp_device_destroy);
  VDP_PROC(VDP_FUNC_ID_GENERATE_CSC_MATRIX                 , m_vdpauConfig.vdpProcs.vdp_generate_csc_matrix);
  VDP_PROC(VDP_FUNC_ID_VIDEO_SURFACE_CREATE                , m_vdpauConfig.vdpProcs.vdp_video_surface_create);
  VDP_PROC(VDP_FUNC_ID_VIDEO_SURFACE_DESTROY               , m_vdpauConfig.vdpProcs.vdp_video_surface_destroy);
  VDP_PROC(VDP_FUNC_ID_VIDEO_SURFACE_PUT_BITS_Y_CB_CR      , m_vdpauConfig.vdpProcs.vdp_video_surface_put_bits_y_cb_cr);
  VDP_PROC(VDP_FUNC_ID_VIDEO_SURFACE_GET_BITS_Y_CB_CR      , m_vdpauConfig.vdpProcs.vdp_video_surface_get_bits_y_cb_cr);
  VDP_PROC(VDP_FUNC_ID_OUTPUT_SURFACE_PUT_BITS_Y_CB_CR     , m_vdpauConfig.vdpProcs.vdp_output_surface_put_bits_y_cb_cr);
  VDP_PROC(VDP_FUNC_ID_OUTPUT_SURFACE_PUT_BITS_NATIVE      , m_vdpauConfig.vdpProcs.vdp_output_surface_put_bits_native);
  VDP_PROC(VDP_FUNC_ID_OUTPUT_SURFACE_CREATE               , m_vdpauConfig.vdpProcs.vdp_output_surface_create);
  VDP_PROC(VDP_FUNC_ID_OUTPUT_SURFACE_DESTROY              , m_vdpauConfig.vdpProcs.vdp_output_surface_destroy);
  VDP_PROC(VDP_FUNC_ID_OUTPUT_SURFACE_GET_BITS_NATIVE      , m_vdpauConfig.vdpProcs.vdp_output_surface_get_bits_native);
  VDP_PROC(VDP_FUNC_ID_OUTPUT_SURFACE_RENDER_OUTPUT_SURFACE, m_vdpauConfig.vdpProcs.vdp_output_surface_render_output_surface);
  VDP_PROC(VDP_FUNC_ID_OUTPUT_SURFACE_PUT_BITS_INDEXED     , m_vdpauConfig.vdpProcs.vdp_output_surface_put_bits_indexed);
  VDP_PROC(VDP_FUNC_ID_VIDEO_MIXER_CREATE                  , m_vdpauConfig.vdpProcs.vdp_video_mixer_create);
  VDP_PROC(VDP_FUNC_ID_VIDEO_MIXER_SET_FEATURE_ENABLES     , m_vdpauConfig.vdpProcs.vdp_video_mixer_set_feature_enables);
  VDP_PROC(VDP_FUNC_ID_VIDEO_MIXER_DESTROY                 , m_vdpauConfig.vdpProcs.vdp_video_mixer_destroy);
  VDP_PROC(VDP_FUNC_ID_VIDEO_MIXER_RENDER                  , m_vdpauConfig.vdpProcs.vdp_video_mixer_render);
  VDP_PROC(VDP_FUNC_ID_VIDEO_MIXER_SET_ATTRIBUTE_VALUES    , m_vdpauConfig.vdpProcs.vdp_video_mixer_set_attribute_values);
  VDP_PROC(VDP_FUNC_ID_VIDEO_MIXER_QUERY_PARAMETER_SUPPORT , m_vdpauConfig.vdpProcs.vdp_video_mixer_query_parameter_support);
  VDP_PROC(VDP_FUNC_ID_VIDEO_MIXER_QUERY_FEATURE_SUPPORT   , m_vdpauConfig.vdpProcs.vdp_video_mixer_query_feature_support);
  VDP_PROC(VDP_FUNC_ID_DECODER_CREATE                      , m_vdpauConfig.vdpProcs.vdp_decoder_create);
  VDP_PROC(VDP_FUNC_ID_DECODER_DESTROY                     , m_vdpauConfig.vdpProcs.vdp_decoder_destroy);
  VDP_PROC(VDP_FUNC_ID_DECODER_RENDER                      , m_vdpauConfig.vdpProcs.vdp_decoder_render);
  VDP_PROC(VDP_FUNC_ID_DECODER_QUERY_CAPABILITIES          , m_vdpauConfig.vdpProcs.vdp_decoder_query_caps);
  VDP_PROC(VDP_FUNC_ID_PRESENTATION_QUEUE_TARGET_DESTROY          , m_vdpauConfig.vdpProcs.vdp_presentation_queue_target_destroy);
  VDP_PROC(VDP_FUNC_ID_PRESENTATION_QUEUE_CREATE                  , m_vdpauConfig.vdpProcs.vdp_presentation_queue_create);
  VDP_PROC(VDP_FUNC_ID_PRESENTATION_QUEUE_DESTROY                 , m_vdpauConfig.vdpProcs.vdp_presentation_queue_destroy);
  VDP_PROC(VDP_FUNC_ID_PRESENTATION_QUEUE_DISPLAY                 , m_vdpauConfig.vdpProcs.vdp_presentation_queue_display);
  VDP_PROC(VDP_FUNC_ID_PRESENTATION_QUEUE_BLOCK_UNTIL_SURFACE_IDLE, m_vdpauConfig.vdpProcs.vdp_presentation_queue_block_until_surface_idle);
  VDP_PROC(VDP_FUNC_ID_PRESENTATION_QUEUE_TARGET_CREATE_X11       , m_vdpauConfig.vdpProcs.vdp_presentation_queue_target_create_x11);
  VDP_PROC(VDP_FUNC_ID_PRESENTATION_QUEUE_QUERY_SURFACE_STATUS    , m_vdpauConfig.vdpProcs.vdp_presentation_queue_query_surface_status);
  VDP_PROC(VDP_FUNC_ID_PRESENTATION_QUEUE_GET_TIME                , m_vdpauConfig.vdpProcs.vdp_presentation_queue_get_time);

#undef VDP_PROC

  // set all vdpau resources to invalid
  m_DisplayState = VDPAU_OPEN;
  m_vdpauConfigured = false;
}

void CDecoder::FiniVDPAUProcs()
{
  if (m_vdpauConfig.vdpDevice == VDP_INVALID_HANDLE) return;

  VdpStatus vdp_st;
  vdp_st = m_vdpauConfig.vdpProcs.vdp_device_destroy(m_vdpauConfig.vdpDevice);
  CheckStatus(vdp_st, __LINE__);
  m_vdpauConfig.vdpDevice = VDP_INVALID_HANDLE;
}

void CDecoder::FiniVDPAUOutput()
{
  if (m_vdpauConfig.vdpDevice == VDP_INVALID_HANDLE || !m_vdpauConfigured) return;

  CLog::Log(LOGNOTICE, " (VDPAU) %s", __FUNCTION__);

  // uninit output
  m_vdpauOutput.Dispose();
  m_vdpauConfigured = false;

  VdpStatus vdp_st;

  vdp_st = m_vdpauConfig.vdpProcs.vdp_decoder_destroy(m_vdpauConfig.vdpDecoder);
  if (CheckStatus(vdp_st, __LINE__))
    return;
  m_vdpauConfig.vdpDecoder = VDP_INVALID_HANDLE;

  CSingleLock lock(m_videoSurfaceSec);
  CLog::Log(LOGDEBUG, "CVDPAU::FiniVDPAUOutput destroying %d video surfaces", (int)m_videoSurfaces.size());

  for(unsigned int i = 0; i < m_videoSurfaces.size(); ++i)
  {
    vdpau_render_state *render = m_videoSurfaces[i];
    if (render->surface != VDP_INVALID_HANDLE)
    {
      vdp_st = m_vdpauConfig.vdpProcs.vdp_video_surface_destroy(render->surface);
      render->surface = VDP_INVALID_HANDLE;
    }
    if (CheckStatus(vdp_st, __LINE__))
      return;
  }
}

void CDecoder::ReadFormatOf( AVCodecID codec
                           , VdpDecoderProfile &vdp_decoder_profile
                           , VdpChromaType     &vdp_chroma_type)
{
  switch (codec)
  {
    case AV_CODEC_ID_MPEG1VIDEO:
      vdp_decoder_profile = VDP_DECODER_PROFILE_MPEG1;
      vdp_chroma_type     = VDP_CHROMA_TYPE_420;
      break;
    case AV_CODEC_ID_MPEG2VIDEO:
      vdp_decoder_profile = VDP_DECODER_PROFILE_MPEG2_MAIN;
      vdp_chroma_type     = VDP_CHROMA_TYPE_420;
      break;
    case AV_CODEC_ID_H264:
      vdp_decoder_profile = VDP_DECODER_PROFILE_H264_HIGH;
      vdp_chroma_type     = VDP_CHROMA_TYPE_420;
      break;
    case AV_CODEC_ID_WMV3:
      vdp_decoder_profile = VDP_DECODER_PROFILE_VC1_MAIN;
      vdp_chroma_type     = VDP_CHROMA_TYPE_420;
      break;
    case AV_CODEC_ID_VC1:
      vdp_decoder_profile = VDP_DECODER_PROFILE_VC1_ADVANCED;
      vdp_chroma_type     = VDP_CHROMA_TYPE_420;
      break;
    case AV_CODEC_ID_MPEG4:
      vdp_decoder_profile = VDP_DECODER_PROFILE_MPEG4_PART2_ASP;
      vdp_chroma_type     = VDP_CHROMA_TYPE_420;
      break;
    default:
      vdp_decoder_profile = 0;
      vdp_chroma_type     = 0;
      break;
  }
}

bool CDecoder::ConfigVDPAU(AVCodecContext* avctx, int ref_frames)
{
  FiniVDPAUOutput();

  VdpStatus vdp_st;
  VdpDecoderProfile vdp_decoder_profile;

  m_vdpauConfig.vidWidth = avctx->width;
  m_vdpauConfig.vidHeight = avctx->height;
  m_vdpauConfig.surfaceWidth = avctx->coded_width;
  m_vdpauConfig.surfaceHeight = avctx->coded_height;

  SetWidthHeight(avctx->width,avctx->height);

  CLog::Log(LOGNOTICE, " (VDPAU) screenWidth:%i vidWidth:%i surfaceWidth:%i",m_vdpauConfig.outWidth,m_vdpauConfig.vidWidth,m_vdpauConfig.surfaceWidth);
  CLog::Log(LOGNOTICE, " (VDPAU) screenHeight:%i vidHeight:%i surfaceHeight:%i",m_vdpauConfig.outHeight,m_vdpauConfig.vidHeight,m_vdpauConfig.surfaceHeight);

  ReadFormatOf(avctx->codec_id, vdp_decoder_profile, m_vdpauConfig.vdpChromaType);

  if(avctx->codec_id == AV_CODEC_ID_H264)
  {
    m_vdpauConfig.maxReferences = ref_frames;
    if (m_vdpauConfig.maxReferences > 16) m_vdpauConfig.maxReferences = 16;
    if (m_vdpauConfig.maxReferences < 5)  m_vdpauConfig.maxReferences = 5;
  }
  else
    m_vdpauConfig.maxReferences = 2;

  vdp_st = m_vdpauConfig.vdpProcs.vdp_decoder_create(m_vdpauConfig.vdpDevice,
                              vdp_decoder_profile,
                              m_vdpauConfig.surfaceWidth,
                              m_vdpauConfig.surfaceHeight,
                              m_vdpauConfig.maxReferences,
                              &m_vdpauConfig.vdpDecoder);
  if (CheckStatus(vdp_st, __LINE__))
    return false;

  // initialize output
  CSingleLock lock(g_graphicsContext);
  m_vdpauConfig.stats = &m_bufferStats;
  m_vdpauConfig.vdpau = this;
  m_bufferStats.Reset();
  m_vdpauOutput.Start();
  Message *reply;
  if (m_vdpauOutput.m_controlPort.SendOutMessageSync(COutputControlProtocol::INIT,
                                                 &reply,
                                                 2000,
                                                 &m_vdpauConfig,
                                                 sizeof(m_vdpauConfig)))
  {
    bool success = reply->signal == COutputControlProtocol::ACC ? true : false;
    reply->Release();
    if (!success)
    {
      CLog::Log(LOGERROR, "VDPAU::%s - vdpau output returned error", __FUNCTION__);
      m_vdpauOutput.Dispose();
      return false;
    }
  }
  else
  {
    CLog::Log(LOGERROR, "VDPAU::%s - failed to init output", __FUNCTION__);
    m_vdpauOutput.Dispose();
    return false;
  }

  m_inMsgEvent.Reset();
  m_vdpauConfigured = true;
  return true;
}

void CDecoder::SpewHardwareAvailable()  //CopyrighVDPAUt (c) 2008 Wladimir J. van der Laan  -- VDPInfo
{
  VdpStatus rv;
  CLog::Log(LOGNOTICE,"VDPAU Decoder capabilities:");
  CLog::Log(LOGNOTICE,"name          level macbs width height");
  CLog::Log(LOGNOTICE,"------------------------------------");
  for(unsigned int x=0; x<decoder_profile_count; ++x)
  {
    VdpBool is_supported = false;
    uint32_t max_level, max_macroblocks, max_width, max_height;
    rv = m_vdpauConfig.vdpProcs.vdp_decoder_query_caps(m_vdpauConfig.vdpDevice, decoder_profiles[x].id,
                                &is_supported, &max_level, &max_macroblocks, &max_width, &max_height);
    if(rv == VDP_STATUS_OK && is_supported)
    {
      CLog::Log(LOGNOTICE,"%-16s %2i %5i %5i %5i\n", decoder_profiles[x].name,
                max_level, max_macroblocks, max_width, max_height);
    }
  }
  CLog::Log(LOGNOTICE,"------------------------------------");
  m_vdpauConfig.featureCount = 0;
#define CHECK_SUPPORT(feature)  \
  do { \
    VdpBool supported; \
    if(m_vdpauConfig.vdpProcs.vdp_video_mixer_query_feature_support(m_vdpauConfig.vdpDevice, feature, &supported) == VDP_STATUS_OK && supported) { \
      CLog::Log(LOGNOTICE, "Mixer feature: "#feature);  \
      m_vdpauConfig.vdpFeatures[m_vdpauConfig.featureCount++] = feature; \
    } \
  } while(false)

  CHECK_SUPPORT(VDP_VIDEO_MIXER_FEATURE_NOISE_REDUCTION);
  CHECK_SUPPORT(VDP_VIDEO_MIXER_FEATURE_SHARPNESS);
  CHECK_SUPPORT(VDP_VIDEO_MIXER_FEATURE_DEINTERLACE_TEMPORAL);
  CHECK_SUPPORT(VDP_VIDEO_MIXER_FEATURE_DEINTERLACE_TEMPORAL_SPATIAL);
  CHECK_SUPPORT(VDP_VIDEO_MIXER_FEATURE_INVERSE_TELECINE);
#ifdef VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L1
  CHECK_SUPPORT(VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L1);
  CHECK_SUPPORT(VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L2);
  CHECK_SUPPORT(VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L3);
  CHECK_SUPPORT(VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L4);
  CHECK_SUPPORT(VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L5);
  CHECK_SUPPORT(VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L6);
  CHECK_SUPPORT(VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L7);
  CHECK_SUPPORT(VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L8);
  CHECK_SUPPORT(VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L9);
#endif
#undef CHECK_SUPPORT

}

bool CDecoder::IsSurfaceValid(vdpau_render_state *render)
{
  // find render state in queue
  bool found(false);
  unsigned int i;
  for(i = 0; i < m_videoSurfaces.size(); ++i)
  {
    if(m_videoSurfaces[i] == render)
    {
      found = true;
      break;
    }
  }
  if (!found)
  {
    CLog::Log(LOGERROR,"%s - video surface not found", __FUNCTION__);
    return false;
  }
  if (m_videoSurfaces[i]->surface == VDP_INVALID_HANDLE)
  {
    m_videoSurfaces[i]->state = 0;
    return false;
  }

  return true;
}

int CDecoder::FFGetBuffer(AVCodecContext *avctx, AVFrame *pic)
{
  //CLog::Log(LOGNOTICE,"%s",__FUNCTION__);
  CDVDVideoCodecFFmpeg* ctx        = (CDVDVideoCodecFFmpeg*)avctx->opaque;
  CDecoder*             vdp        = (CDecoder*)ctx->GetHardware();

  // while we are waiting to recover we can't do anything
  CSingleLock lock(vdp->m_DecoderSection);

  if(vdp->m_DisplayState != VDPAU_OPEN)
  {
    CLog::Log(LOGWARNING, "CVDPAU::FFGetBuffer - returning due to awaiting recovery");
    return -1;
  }

  vdpau_render_state * render = NULL;

  // find unused surface
  { CSingleLock lock(vdp->m_videoSurfaceSec);
    for(unsigned int i = 0; i < vdp->m_videoSurfaces.size(); i++)
    {
      if(!(vdp->m_videoSurfaces[i]->state & (FF_VDPAU_STATE_USED_FOR_REFERENCE | FF_VDPAU_STATE_USED_FOR_RENDER)))
      {
        render = vdp->m_videoSurfaces[i];
        render->state = 0;
        break;
      }
    }
  }

  VdpStatus vdp_st = VDP_STATUS_ERROR;
  if (render == NULL)
  {
    // create a new surface
    VdpDecoderProfile profile;
    ReadFormatOf(avctx->codec_id, profile, vdp->m_vdpauConfig.vdpChromaType);
    render = (vdpau_render_state*)calloc(sizeof(vdpau_render_state), 1);
    if (render == NULL)
    {
      CLog::Log(LOGWARNING, "CVDPAU::FFGetBuffer - calloc failed");
      return -1;
    }
    CSingleLock lock(vdp->m_videoSurfaceSec);
    render->surface = VDP_INVALID_HANDLE;
    vdp->m_videoSurfaces.push_back(render);
  }

  if (render->surface == VDP_INVALID_HANDLE)
  {
    vdp_st = vdp->m_vdpauConfig.vdpProcs.vdp_video_surface_create(vdp->m_vdpauConfig.vdpDevice,
                                         vdp->m_vdpauConfig.vdpChromaType,
                                         avctx->coded_width,
                                         avctx->coded_height,
                                         &render->surface);
    vdp->CheckStatus(vdp_st, __LINE__);
    if (vdp_st != VDP_STATUS_OK)
    {
      free(render);
      CLog::Log(LOGERROR, "CVDPAU::FFGetBuffer - No Video surface available could be created");
      return -1;
    }
  }

  pic->data[1] = pic->data[2] = NULL;
  pic->data[0] = (uint8_t*)render;
  pic->data[3] = (uint8_t*)(uintptr_t)render->surface;

  pic->linesize[0] = pic->linesize[1] =  pic->linesize[2] = 0;

  pic->type= FF_BUFFER_TYPE_USER;

  render->state |= FF_VDPAU_STATE_USED_FOR_REFERENCE;
  pic->reordered_opaque= avctx->reordered_opaque;
  return 0;
}

void CDecoder::FFReleaseBuffer(AVCodecContext *avctx, AVFrame *pic)
{
  //CLog::Log(LOGNOTICE,"%s",__FUNCTION__);
  CDVDVideoCodecFFmpeg* ctx        = (CDVDVideoCodecFFmpeg*)avctx->opaque;
  CDecoder*             vdp        = (CDecoder*)ctx->GetHardware();

  vdpau_render_state  * render;
  unsigned int i;

  CSingleLock lock(vdp->m_DecoderSection);

  render=(vdpau_render_state*)pic->data[0];
  if(!render)
  {
    CLog::Log(LOGERROR, "CVDPAU::FFReleaseBuffer - invalid context handle provided");
    return;
  }

  CSingleLock vLock(vdp->m_videoSurfaceSec);
  render->state &= ~FF_VDPAU_STATE_USED_FOR_REFERENCE;
  for(i=0; i<4; i++)
    pic->data[i]= NULL;

  // find render state in queue
  if (!vdp->IsSurfaceValid(render))
  {
    CLog::Log(LOGDEBUG, "CVDPAU::FFReleaseBuffer - ignoring invalid buffer");
    return;
  }

  render->state &= ~FF_VDPAU_STATE_USED_FOR_REFERENCE;
}

VdpStatus CDecoder::Render( VdpDecoder decoder, VdpVideoSurface target,
                            VdpPictureInfo const *picture_info,
                            uint32_t bitstream_buffer_count,
                            VdpBitstreamBuffer const * bitstream_buffers)
{
  return VDP_STATUS_OK;
}

void CDecoder::FFDrawSlice(struct AVCodecContext *s,
                                           const AVFrame *src, int offset[4],
                                           int y, int type, int height)
{
  CDVDVideoCodecFFmpeg* ctx = (CDVDVideoCodecFFmpeg*)s->opaque;
  CDecoder*               vdp = (CDecoder*)ctx->GetHardware();

  // while we are waiting to recover we can't do anything
  CSingleLock lock(vdp->m_DecoderSection);

  if(vdp->m_DisplayState != VDPAU_OPEN)
    return;

  if(src->linesize[0] || src->linesize[1] || src->linesize[2]
  || offset[0] || offset[1] || offset[2])
  {
    CLog::Log(LOGERROR, "CVDPAU::FFDrawSlice - invalid linesizes or offsets provided");
    return;
  }

  VdpStatus vdp_st;
  vdpau_render_state * render;

  render = (vdpau_render_state*)src->data[0];
  if(!render)
  {
    CLog::Log(LOGERROR, "CVDPAU::FFDrawSlice - invalid context handle provided");
    return;
  }

  // ffmpeg vc-1 decoder does not flush, make sure the data buffer is still valid
  if (!vdp->IsSurfaceValid(render))
  {
    CLog::Log(LOGWARNING, "CVDPAU::FFDrawSlice - ignoring invalid buffer");
    return;
  }

  uint32_t max_refs = 0;
  if(s->codec_id == AV_CODEC_ID_H264)
    max_refs = vdp->m_hwContext.info.h264.num_ref_frames;

  if(vdp->m_vdpauConfig.vdpDecoder == VDP_INVALID_HANDLE
  || vdp->m_vdpauConfigured == false
  || vdp->m_vdpauConfig.maxReferences < max_refs)
  {
    if(!vdp->ConfigVDPAU(s, max_refs))
      return;
  }

//  uint64_t startTime = CurrentHostCounter();
  uint16_t decoded, processed, rend;
  vdp->m_bufferStats.Get(decoded, processed, rend);
  vdp_st = vdp->m_vdpauConfig.vdpProcs.vdp_decoder_render(vdp->m_vdpauConfig.vdpDecoder,
                                   render->surface,
                                   (VdpPictureInfo const *)&(vdp->m_hwContext.info),
                                   vdp->m_hwContext.bitstream_buffers_used,
                                   vdp->m_hwContext.bitstream_buffers);
  vdp->CheckStatus(vdp_st, __LINE__);
//  uint64_t diff = CurrentHostCounter() - startTime;
//  if (diff*1000/CurrentHostFrequency() > 30)
//    CLog::Log(LOGWARNING,"CVDPAU::DrawSlice - VdpDecoderRender long decoding: %d ms, dec: %d, proc: %d, rend: %d", (int)((diff*1000)/CurrentHostFrequency()), decoded, processed, rend);

}


int CDecoder::Decode(AVCodecContext *avctx, AVFrame *pFrame)
{
  int result = Check(avctx);
  if (result)
    return result;

  CSingleLock lock(m_DecoderSection);

  if (!m_vdpauConfigured)
    return VC_ERROR;

  if(pFrame)
  { // we have a new frame from decoder

    vdpau_render_state * render = (vdpau_render_state*)pFrame->data[0];
    if(!render)
    {
      CLog::Log(LOGERROR, "CVDPAU::Decode: no valid frame");
      return VC_ERROR;
    }

    // ffmpeg vc-1 decoder does not flush, make sure the data buffer is still valid
    if (!IsSurfaceValid(render))
    {
      CLog::Log(LOGWARNING, "CVDPAU::Decode - ignoring invalid buffer");
      return VC_BUFFER;
    }

    CSingleLock lock(m_videoSurfaceSec);
    render->state |= FF_VDPAU_STATE_USED_FOR_RENDER;
    lock.Leave();

    // send frame to output for processing
    CVdpauDecodedPicture pic;
    memset(&pic.DVDPic, 0, sizeof(pic.DVDPic));
    ((CDVDVideoCodecFFmpeg*)avctx->opaque)->GetPictureCommon(&pic.DVDPic);
    pic.render = render;
    pic.DVDPic.color_matrix = avctx->colorspace;
    m_bufferStats.IncDecoded();
    m_vdpauOutput.m_dataPort.SendOutMessage(COutputDataProtocol::NEWFRAME, &pic, sizeof(pic));

    //TODO
    // m_codecControl = pic.DVDPic.iFlags & (DVP_FLAG_DRAIN | DVP_FLAG_NO_POSTPROC);
  }

  int retval = 0;
  uint16_t decoded, processed, render;
  Message *msg;
  while (m_vdpauOutput.m_controlPort.ReceiveInMessage(&msg))
  {
    if (msg->signal == COutputControlProtocol::ERROR)
    {
      m_DisplayState = VDPAU_ERROR;
      retval |= VC_ERROR;
    }
    msg->Release();
  }

  m_bufferStats.Get(decoded, processed, render);

  uint64_t startTime = CurrentHostCounter();
  while (!retval)
  {
    if (m_vdpauOutput.m_dataPort.ReceiveInMessage(&msg))
    {
      if (msg->signal == COutputDataProtocol::PICTURE)
      {
        if (m_presentPicture)
        {
          m_presentPicture->ReturnUnused();
          m_presentPicture = 0;
        }

        m_presentPicture = *(CVdpauRenderPicture**)msg->data;
        m_presentPicture->vdpau = this;
        m_bufferStats.DecRender();
        m_bufferStats.Get(decoded, processed, render);
        retval |= VC_PICTURE;
        msg->Release();
        break;
      }
      msg->Release();
    }
    else if (m_vdpauOutput.m_controlPort.ReceiveInMessage(&msg))
    {
      if (msg->signal == COutputControlProtocol::STATS)
      {
        m_bufferStats.Get(decoded, processed, render);
      }
      else
      {
        m_DisplayState = VDPAU_ERROR;
        retval |= VC_ERROR;
      }
      msg->Release();
    }

    // TODO
    if (1) //(m_codecControl & DVP_FLAG_DRAIN))
    {
      if (decoded + processed + render < 4)
      {
        retval |= VC_BUFFER;
      }
    }
    else
    {
      if (decoded < 4 && (processed + render) < 3)
      {
        retval |= VC_BUFFER;
      }
    }

    if (!retval && !m_inMsgEvent.WaitMSec(2000))
      break;
  }
  uint64_t diff = CurrentHostCounter() - startTime;
  if (retval & VC_PICTURE)
  {
    m_bufferStats.SetParams(diff, m_codecControl);
  }
  if (diff*1000/CurrentHostFrequency() > 50)
    CLog::Log(LOGDEBUG,"CVDPAU::Decode long wait: %d", (int)((diff*1000)/CurrentHostFrequency()));

  if (!retval)
  {
    CLog::Log(LOGERROR, "VDPAU::%s - timed out waiting for output message", __FUNCTION__);
    m_DisplayState = VDPAU_ERROR;
    retval |= VC_ERROR;
  }

  return retval;
}

bool CDecoder::GetPicture(AVCodecContext* avctx, AVFrame* frame, DVDVideoPicture* picture)
{
  CSingleLock lock(m_DecoderSection);

  if (m_DisplayState != VDPAU_OPEN)
    return false;

  *picture = m_presentPicture->DVDPic;
  picture->vdpau = m_presentPicture;

  return true;
}

void CDecoder::Reset()
{
  CSingleLock lock(m_DecoderSection);

  if (!m_vdpauConfigured)
    return;

  Message *reply;
  if (m_vdpauOutput.m_controlPort.SendOutMessageSync(COutputControlProtocol::FLUSH,
                                                 &reply,
                                                 2000))
  {
    bool success = reply->signal == COutputControlProtocol::ACC ? true : false;
    reply->Release();
    if (!success)
    {
      CLog::Log(LOGERROR, "VDPAU::%s - flush returned error", __FUNCTION__);
      m_DisplayState = VDPAU_ERROR;
    }
    else
      m_bufferStats.Reset();
  }
  else
  {
    CLog::Log(LOGERROR, "VDPAU::%s - flush timed out", __FUNCTION__);
    m_DisplayState = VDPAU_ERROR;
  }
}

bool CDecoder::CanSkipDeint()
{
  return m_bufferStats.CanSkipDeint();
}

void CDecoder::ReturnRenderPicture(CVdpauRenderPicture *renderPic)
{
  m_vdpauOutput.m_dataPort.SendOutMessage(COutputDataProtocol::RETURNPIC, &renderPic, sizeof(renderPic));
}

bool CDecoder::CheckStatus(VdpStatus vdp_st, int line)
{
  if (vdp_st != VDP_STATUS_OK)
  {
    CLog::Log(LOGERROR, " (VDPAU) Error: %s(%d) at %s:%d\n", m_vdpauConfig.vdpProcs.vdp_get_error_string(vdp_st), vdp_st, __FILE__, line);

    if(m_DisplayState == VDPAU_OPEN)
    {
      if (vdp_st == VDP_STATUS_DISPLAY_PREEMPTED)
      {
        m_DisplayEvent.Reset();
        m_DisplayState = VDPAU_LOST;
      }
      else
        m_DisplayState = VDPAU_ERROR;
    }

    return true;
  }
  return false;
}

//-----------------------------------------------------------------------------
// RenderPicture
//-----------------------------------------------------------------------------

CVdpauRenderPicture* CVdpauRenderPicture::Acquire()
{
  CSingleLock lock(*renderPicSection);

  if (refCount == 0)
    vdpau->Acquire();

  refCount++;
  return this;
}

long CVdpauRenderPicture::Release()
{
  CSingleLock lock(*renderPicSection);

  refCount--;
  if (refCount > 0)
    return refCount;

  lock.Leave();
  vdpau->ReturnRenderPicture(this);
  vdpau->ReleasePicReference();

  return refCount;
}

void CVdpauRenderPicture::ReturnUnused()
{
  { CSingleLock lock(*renderPicSection);
    if (refCount > 0)
      return;
  }
  if (vdpau)
    vdpau->ReturnRenderPicture(this);
}
//-----------------------------------------------------------------------------
// Mixer
//-----------------------------------------------------------------------------
CMixer::CMixer(CEvent *inMsgEvent) :
  CThread("Vdpau Mixer Thread"),
  m_controlPort("ControlPort", inMsgEvent, &m_outMsgEvent),
  m_dataPort("DataPort", inMsgEvent, &m_outMsgEvent)
{
  m_inMsgEvent = inMsgEvent;
}

CMixer::~CMixer()
{
  Dispose();
}

void CMixer::Start()
{
  Create();
}

void CMixer::Dispose()
{
  m_bStop = true;
  m_outMsgEvent.Set();
  StopThread();

  m_controlPort.Purge();
  m_dataPort.Purge();
}

bool CMixer::IsActive()
{
  return IsRunning();
}

void CMixer::OnStartup()
{
  CLog::Log(LOGNOTICE, "CMixer::OnStartup: Output Thread created");
}

void CMixer::OnExit()
{
  CLog::Log(LOGNOTICE, "CMixer::OnExit: Output Thread terminated");
}

enum MIXER_STATES
{
  M_TOP = 0,                      // 0
  M_TOP_ERROR,                    // 1
  M_TOP_UNCONFIGURED,             // 2
  M_TOP_CONFIGURED,               // 3
  M_TOP_CONFIGURED_WAIT1,         // 4
  M_TOP_CONFIGURED_STEP1,         // 5
  M_TOP_CONFIGURED_WAIT2,         // 6
  M_TOP_CONFIGURED_STEP2,         // 7
};

int MIXER_parentStates[] = {
    -1,
    0, //TOP_ERROR
    0, //TOP_UNCONFIGURED
    0, //TOP_CONFIGURED
    3, //TOP_CONFIGURED_WAIT1
    3, //TOP_CONFIGURED_STEP1
    3, //TOP_CONFIGURED_WAIT2
    3, //TOP_CONFIGURED_STEP2
};

void CMixer::StateMachine(int signal, Protocol *port, Message *msg)
{
  for (int state = m_state; ; state = MIXER_parentStates[state])
  {
    switch (state)
    {
    case M_TOP: // TOP
      if (port == &m_controlPort)
      {
        switch (signal)
        {
        case CMixerControlProtocol::FLUSH:
          Flush();
          msg->Reply(CMixerControlProtocol::ACC);
          return;
        default:
          break;
        }
      }
      {
        std::string portName = port == NULL ? "timer" : port->portName;
        CLog::Log(LOGWARNING, "CMixer::%s - signal: %d form port: %s not handled for state: %d", __FUNCTION__, signal, portName.c_str(), m_state);
      }
      return;

    case M_TOP_ERROR: // TOP
      break;

    case M_TOP_UNCONFIGURED:
      if (port == &m_controlPort)
      {
        switch (signal)
        {
        case CMixerControlProtocol::INIT:
          CVdpauConfig *data;
          data = (CVdpauConfig*)msg->data;
          if (data)
          {
            m_config = *data;
          }
          Init();
          if (!m_vdpError)
          {
            m_state = M_TOP_CONFIGURED_WAIT1;
            msg->Reply(CMixerControlProtocol::ACC);
          }
          else
          {
            msg->Reply(CMixerControlProtocol::ERROR);
          }
          return;
        default:
          break;
        }
      }
      break;

    case M_TOP_CONFIGURED:
      if (port == &m_dataPort)
      {
        switch (signal)
        {
        case CMixerDataProtocol::FRAME:
          CVdpauDecodedPicture *frame;
          frame = (CVdpauDecodedPicture*)msg->data;
          if (frame)
          {
            m_decodedPics.push(*frame);
          }
          m_extTimeout = 0;
          return;
        case CMixerDataProtocol::BUFFER:
          VdpOutputSurface *surf;
          surf = (VdpOutputSurface*)msg->data;
          if (surf)
          {
            m_outputSurfaces.push(*surf);
          }
          m_extTimeout = 0;
          return;
        default:
          break;
        }
      }
      break;

    case M_TOP_CONFIGURED_WAIT1:
      if (port == NULL) // timeout
      {
        switch (signal)
        {
        case CMixerControlProtocol::TIMEOUT:
          if (!m_decodedPics.empty() && !m_outputSurfaces.empty())
          {
            m_state = M_TOP_CONFIGURED_STEP1;
            m_bStateMachineSelfTrigger = true;
          }
          else
          {
//            if (m_extTimeout != 0)
//            {
//              SetPostProcFeatures(false);
//              CLog::Log(LOGWARNING,"CVDPAU::Mixer timeout - decoded: %d, outputSurf: %d", (int)m_decodedPics.size(), (int)m_outputSurfaces.size());
//            }
            m_extTimeout = 100;
          }
          return;
        default:
          break;
        }
      }
      break;

    case M_TOP_CONFIGURED_STEP1:
      if (port == NULL) // timeout
      {
        switch (signal)
        {
        case CMixerControlProtocol::TIMEOUT:
          m_mixerInput.push_front(m_decodedPics.front());
          m_decodedPics.pop();
          if (m_mixerInput.size() < 2)
          {
            m_state = M_TOP_CONFIGURED_WAIT1;
            m_extTimeout = 0;
            return;
          }
          InitCycle();
          ProcessPicture();
          if (m_vdpError)
          {
            m_state = M_TOP_CONFIGURED_WAIT1;
            m_extTimeout = 1000;
            return;
          }
          if (m_processPicture.DVDPic.format != RENDER_FMT_VDPAU_420)
            m_outputSurfaces.pop();
          m_config.stats->IncProcessed();
          m_config.stats->DecDecoded();
          m_dataPort.SendInMessage(CMixerDataProtocol::PICTURE,&m_processPicture,sizeof(m_processPicture));
          if (m_mixersteps > 1)
          {
            m_state = M_TOP_CONFIGURED_WAIT2;
            m_extTimeout = 0;
          }
          else
          {
            FiniCycle();
            m_state = M_TOP_CONFIGURED_WAIT1;
            m_extTimeout = 0;
          }
          return;
        default:
          break;
        }
      }
      break;

    case M_TOP_CONFIGURED_WAIT2:
      if (port == NULL) // timeout
      {
        switch (signal)
        {
        case CMixerControlProtocol::TIMEOUT:
          if (!m_outputSurfaces.empty())
          {
            m_state = M_TOP_CONFIGURED_STEP2;
            m_bStateMachineSelfTrigger = true;
          }
          else
          {
//            if (m_extTimeout != 0)
//            {
//              SetPostProcFeatures(false);
//              CLog::Log(LOGNOTICE,"---mixer wait2 decoded: %d, outputSurf: %d", (int)m_decodedPics.size(), (int)m_outputSurfaces.size());
//            }
            m_extTimeout = 100;
          }
          return;
        default:
          break;
        }
      }
      break;

    case M_TOP_CONFIGURED_STEP2:
       if (port == NULL) // timeout
       {
         switch (signal)
         {
         case CMixerControlProtocol::TIMEOUT:
           m_processPicture.outputSurface = m_outputSurfaces.front();
           m_mixerstep = 1;
           ProcessPicture();
           if (m_vdpError)
           {
             m_state = M_TOP_CONFIGURED_WAIT1;
             m_extTimeout = 1000;
             return;
           }
           if (m_processPicture.DVDPic.format != RENDER_FMT_VDPAU_420)
             m_outputSurfaces.pop();
           m_config.stats->IncProcessed();
           m_dataPort.SendInMessage(CMixerDataProtocol::PICTURE,&m_processPicture,sizeof(m_processPicture));
           FiniCycle();
           m_state = M_TOP_CONFIGURED_WAIT1;
           m_extTimeout = 0;
           return;
         default:
           break;
         }
       }
       break;

    default: // we are in no state, should not happen
      CLog::Log(LOGERROR, "CMixer::%s - no valid state: %d", __FUNCTION__, m_state);
      return;
    }
  } // for
}

void CMixer::Process()
{
  Message *msg = NULL;
  Protocol *port = NULL;
  bool gotMsg;

  m_state = M_TOP_UNCONFIGURED;
  m_extTimeout = 1000;
  m_bStateMachineSelfTrigger = false;

  while (!m_bStop)
  {
    gotMsg = false;

    if (m_bStateMachineSelfTrigger)
    {
      m_bStateMachineSelfTrigger = false;
      // self trigger state machine
      StateMachine(msg->signal, port, msg);
      if (!m_bStateMachineSelfTrigger)
      {
        msg->Release();
        msg = NULL;
      }
      continue;
    }
    // check control port
    else if (m_controlPort.ReceiveOutMessage(&msg))
    {
      gotMsg = true;
      port = &m_controlPort;
    }
    // check data port
    else if (m_dataPort.ReceiveOutMessage(&msg))
    {
      gotMsg = true;
      port = &m_dataPort;
    }

    if (gotMsg)
    {
      StateMachine(msg->signal, port, msg);
      if (!m_bStateMachineSelfTrigger)
      {
        msg->Release();
        msg = NULL;
      }
      continue;
    }

    // wait for message
    else if (m_outMsgEvent.WaitMSec(m_extTimeout))
    {
      continue;
    }
    // time out
    else
    {
      msg = m_controlPort.GetMessage();
      msg->signal = CMixerControlProtocol::TIMEOUT;
      port = 0;
      // signal timeout to state machine
      StateMachine(msg->signal, port, msg);
      if (!m_bStateMachineSelfTrigger)
      {
        msg->Release();
        msg = NULL;
      }
    }
  }
  Uninit();
}

void CMixer::CreateVdpauMixer()
{
  CLog::Log(LOGNOTICE, " (VDPAU) Creating the video mixer");

  InitCSCMatrix(m_config.vidWidth);

  VdpVideoMixerParameter parameters[] = {
    VDP_VIDEO_MIXER_PARAMETER_VIDEO_SURFACE_WIDTH,
    VDP_VIDEO_MIXER_PARAMETER_VIDEO_SURFACE_HEIGHT,
    VDP_VIDEO_MIXER_PARAMETER_CHROMA_TYPE};

  void const * parameter_values[] = {
    &m_config.surfaceWidth,
    &m_config.surfaceHeight,
    &m_config.vdpChromaType};

  VdpStatus vdp_st = VDP_STATUS_ERROR;
  vdp_st = m_config.vdpProcs.vdp_video_mixer_create(m_config.vdpDevice,
                                m_config.featureCount,
                                m_config.vdpFeatures,
                                ARSIZE(parameters),
                                parameters,
                                parameter_values,
                                &m_videoMixer);
  CheckStatus(vdp_st, __LINE__);

  // create 3 pitches of black lines needed for clipping top
  // and bottom lines when de-interlacing
  m_BlackBar = new uint32_t[3*m_config.outWidth];
  memset(m_BlackBar, 0, 3*m_config.outWidth*sizeof(uint32_t));

}

void CMixer::InitCSCMatrix(int Width)
{
  m_Procamp.struct_version = VDP_PROCAMP_VERSION;
  m_Procamp.brightness     = 0.0;
  m_Procamp.contrast       = 1.0;
  m_Procamp.saturation     = 1.0;
  m_Procamp.hue            = 0;
}

void CMixer::CheckFeatures()
{
  if (m_Upscale != m_config.upscale)
  {
    SetHWUpscaling();
    m_Upscale = m_config.upscale;
  }
  if (m_Brightness != CMediaSettings::Get().GetCurrentVideoSettings().m_Brightness ||
      m_Contrast   != CMediaSettings::Get().GetCurrentVideoSettings().m_Contrast ||
      m_ColorMatrix != m_mixerInput[1].DVDPic.color_matrix)
  {
    SetColor();
    m_Brightness = CMediaSettings::Get().GetCurrentVideoSettings().m_Brightness;
    m_Contrast = CMediaSettings::Get().GetCurrentVideoSettings().m_Contrast;
    m_ColorMatrix = m_mixerInput[1].DVDPic.color_matrix;
  }
  if (m_NoiseReduction != CMediaSettings::Get().GetCurrentVideoSettings().m_NoiseReduction)
  {
    m_NoiseReduction = CMediaSettings::Get().GetCurrentVideoSettings().m_NoiseReduction;
    SetNoiseReduction();
  }
  if (m_Sharpness != CMediaSettings::Get().GetCurrentVideoSettings().m_Sharpness)
  {
    m_Sharpness = CMediaSettings::Get().GetCurrentVideoSettings().m_Sharpness;
    SetSharpness();
  }
  if (m_DeintMode != CMediaSettings::Get().GetCurrentVideoSettings().m_DeinterlaceMode ||
      m_Deint     != CMediaSettings::Get().GetCurrentVideoSettings().m_InterlaceMethod)
  {
    m_DeintMode = CMediaSettings::Get().GetCurrentVideoSettings().m_DeinterlaceMode;
    m_Deint     = CMediaSettings::Get().GetCurrentVideoSettings().m_InterlaceMethod;
    SetDeinterlacing();
  }
}

void CMixer::SetPostProcFeatures(bool postProcEnabled)
{
  if (m_PostProc != postProcEnabled)
  {
    if (postProcEnabled)
    {
      SetNoiseReduction();
      SetSharpness();
      SetDeinterlacing();
      SetHWUpscaling();
    }
    else
      PostProcOff();
    m_PostProc = postProcEnabled;
  }
}

void CMixer::PostProcOff()
{
  VdpStatus vdp_st;

  if (m_videoMixer == VDP_INVALID_HANDLE)
    return;

  VdpVideoMixerFeature feature[] = { VDP_VIDEO_MIXER_FEATURE_DEINTERLACE_TEMPORAL,
                                     VDP_VIDEO_MIXER_FEATURE_DEINTERLACE_TEMPORAL_SPATIAL,
                                     VDP_VIDEO_MIXER_FEATURE_INVERSE_TELECINE};

  VdpBool enabled[]={0,0,0};
  vdp_st = m_config.vdpProcs.vdp_video_mixer_set_feature_enables(m_videoMixer, ARSIZE(feature), feature, enabled);
  CheckStatus(vdp_st, __LINE__);

  if(m_config.vdpau->Supports(VDP_VIDEO_MIXER_FEATURE_NOISE_REDUCTION))
  {
    VdpVideoMixerFeature feature[] = { VDP_VIDEO_MIXER_FEATURE_NOISE_REDUCTION};

    VdpBool enabled[]={0};
    vdp_st = m_config.vdpProcs.vdp_video_mixer_set_feature_enables(m_videoMixer, ARSIZE(feature), feature, enabled);
    CheckStatus(vdp_st, __LINE__);
  }

  if(m_config.vdpau->Supports(VDP_VIDEO_MIXER_FEATURE_SHARPNESS))
  {
    VdpVideoMixerFeature feature[] = { VDP_VIDEO_MIXER_FEATURE_SHARPNESS};

    VdpBool enabled[]={0};
    vdp_st = m_config.vdpProcs.vdp_video_mixer_set_feature_enables(m_videoMixer, ARSIZE(feature), feature, enabled);
    CheckStatus(vdp_st, __LINE__);
  }

  DisableHQScaling();
}

bool CMixer::GenerateStudioCSCMatrix(VdpColorStandard colorStandard, VdpCSCMatrix &studioCSCMatrix)
{
   // instead use studioCSCKCoeffs601[3], studioCSCKCoeffs709[3] to generate float[3][4] matrix (float studioCSC[3][4])
   // m00 = mRY = red: luma factor (contrast factor) (1.0)
   // m10 = mGY = green: luma factor (contrast factor) (1.0)
   // m20 = mBY = blue: luma factor (contrast factor) (1.0)
   //
   // m01 = mRB = red: blue color diff coeff (0.0)
   // m11 = mGB = green: blue color diff coeff (-2Kb(1-Kb)/(Kg))
   // m21 = mBB = blue: blue color diff coeff ((1-Kb)/0.5)
   //
   // m02 = mRR = red: red color diff coeff ((1-Kr)/0.5)
   // m12 = mGR = green: red color diff coeff (-2Kr(1-Kr)/(Kg))
   // m22 = mBR = blue: red color diff coeff (0.0)
   //
   // m03 = mRC = red: colour zero offset (brightness factor) (-(1-Kr)/0.5 * (128/255))
   // m13 = mGC = green: colour zero offset (brightness factor) ((256/255) * (Kb(1-Kb) + Kr(1-Kr)) / Kg)
   // m23 = mBC = blue: colour zero offset (brightness factor) (-(1-Kb)/0.5 * (128/255))

   // columns
   int Y = 0;
   int Cb = 1;
   int Cr = 2;
   int C = 3;
   // rows
   int R = 0;
   int G = 1;
   int B = 2;
   // colour standard coefficients for red, geen, blue
   double Kr, Kg, Kb;
   // colour diff zero position (use standard 8-bit coding precision)
   double CDZ = 128; //256*0.5
   // range excursion (use standard 8-bit coding precision)
   double EXC = 255; //256-1

   if (colorStandard == VDP_COLOR_STANDARD_ITUR_BT_601)
   {
      Kr = studioCSCKCoeffs601[0];
      Kg = studioCSCKCoeffs601[1];
      Kb = studioCSCKCoeffs601[2];
   }
   else // assume VDP_COLOR_STANDARD_ITUR_BT_709
   {
      Kr = studioCSCKCoeffs709[0];
      Kg = studioCSCKCoeffs709[1];
      Kb = studioCSCKCoeffs709[2];
   }
   // we keep luma unscaled to retain the levels present in source so that 16-235 luma is converted to RGB 16-235
   studioCSCMatrix[R][Y] = 1.0;
   studioCSCMatrix[G][Y] = 1.0;
   studioCSCMatrix[B][Y] = 1.0;

   studioCSCMatrix[R][Cb] = 0.0;
   studioCSCMatrix[G][Cb] = (double)-2 * Kb * (1 - Kb) / Kg;
   studioCSCMatrix[B][Cb] = (double)(1 - Kb) / 0.5;

   studioCSCMatrix[R][Cr] = (double)(1 - Kr) / 0.5;
   studioCSCMatrix[G][Cr] = (double)-2 * Kr * (1 - Kr) / Kg;
   studioCSCMatrix[B][Cr] = 0.0;

   studioCSCMatrix[R][C] = (double)-1 * studioCSCMatrix[R][Cr] * CDZ/EXC;
   studioCSCMatrix[G][C] = (double)-1 * (studioCSCMatrix[G][Cb] + studioCSCMatrix[G][Cr]) * CDZ/EXC;
   studioCSCMatrix[B][C] = (double)-1 * studioCSCMatrix[B][Cb] * CDZ/EXC;

   return true;
}

void CMixer::SetColor()
{
  VdpStatus vdp_st;

  if (m_Brightness != CMediaSettings::Get().GetCurrentVideoSettings().m_Brightness)
    m_Procamp.brightness = (float)((CMediaSettings::Get().GetCurrentVideoSettings().m_Brightness)-50) / 100;
  if (m_Contrast != CMediaSettings::Get().GetCurrentVideoSettings().m_Contrast)
    m_Procamp.contrast = (float)((CMediaSettings::Get().GetCurrentVideoSettings().m_Contrast)+50) / 100;

  VdpColorStandard colorStandard;
  switch(m_mixerInput[1].DVDPic.color_matrix)
  {
    case AVCOL_SPC_BT709:
      colorStandard = VDP_COLOR_STANDARD_ITUR_BT_709;
      break;
    case AVCOL_SPC_BT470BG:
    case AVCOL_SPC_SMPTE170M:
      colorStandard = VDP_COLOR_STANDARD_ITUR_BT_601;
      break;
    case AVCOL_SPC_SMPTE240M:
      colorStandard = VDP_COLOR_STANDARD_SMPTE_240M;
      break;
    case AVCOL_SPC_FCC:
    case AVCOL_SPC_UNSPECIFIED:
    case AVCOL_SPC_RGB:
    default:
      if(m_config.surfaceWidth > 1000)
        colorStandard = VDP_COLOR_STANDARD_ITUR_BT_709;
      else
        colorStandard = VDP_COLOR_STANDARD_ITUR_BT_601;
  }

  VdpVideoMixerAttribute attributes[] = { VDP_VIDEO_MIXER_ATTRIBUTE_CSC_MATRIX };
  if (CSettings::Get().GetBool("videoscreen.limitedrange"))
  {
    float studioCSC[3][4];
    GenerateStudioCSCMatrix(colorStandard, studioCSC);
    void const * pm_CSCMatix[] = { &studioCSC };
    vdp_st = m_config.vdpProcs.vdp_video_mixer_set_attribute_values(m_videoMixer, ARSIZE(attributes), attributes, pm_CSCMatix);
  }
  else
  {
    vdp_st = m_config.vdpProcs.vdp_generate_csc_matrix(&m_Procamp, colorStandard, &m_CSCMatrix);
    void const * pm_CSCMatix[] = { &m_CSCMatrix };
    vdp_st = m_config.vdpProcs.vdp_video_mixer_set_attribute_values(m_videoMixer, ARSIZE(attributes), attributes, pm_CSCMatix);
  }

  CheckStatus(vdp_st, __LINE__);
}

void CMixer::SetNoiseReduction()
{
  if(!m_config.vdpau->Supports(VDP_VIDEO_MIXER_FEATURE_NOISE_REDUCTION))
    return;

  VdpVideoMixerFeature feature[] = { VDP_VIDEO_MIXER_FEATURE_NOISE_REDUCTION };
  VdpVideoMixerAttribute attributes[] = { VDP_VIDEO_MIXER_ATTRIBUTE_NOISE_REDUCTION_LEVEL };
  VdpStatus vdp_st;

  if (!CMediaSettings::Get().GetCurrentVideoSettings().m_NoiseReduction)
  {
    VdpBool enabled[]= {0};
    vdp_st = m_config.vdpProcs.vdp_video_mixer_set_feature_enables(m_videoMixer, ARSIZE(feature), feature, enabled);
    CheckStatus(vdp_st, __LINE__);
    return;
  }
  VdpBool enabled[]={1};
  vdp_st = m_config.vdpProcs.vdp_video_mixer_set_feature_enables(m_videoMixer, ARSIZE(feature), feature, enabled);
  CheckStatus(vdp_st, __LINE__);
  void* nr[] = { &CMediaSettings::Get().GetCurrentVideoSettings().m_NoiseReduction };
  CLog::Log(LOGNOTICE,"Setting Noise Reduction to %f",CMediaSettings::Get().GetCurrentVideoSettings().m_NoiseReduction);
  vdp_st = m_config.vdpProcs.vdp_video_mixer_set_attribute_values(m_videoMixer, ARSIZE(attributes), attributes, nr);
  CheckStatus(vdp_st, __LINE__);
}

void CMixer::SetSharpness()
{
  if(!m_config.vdpau->Supports(VDP_VIDEO_MIXER_FEATURE_SHARPNESS))
    return;

  VdpVideoMixerFeature feature[] = { VDP_VIDEO_MIXER_FEATURE_SHARPNESS };
  VdpVideoMixerAttribute attributes[] = { VDP_VIDEO_MIXER_ATTRIBUTE_SHARPNESS_LEVEL };
  VdpStatus vdp_st;

  if (!CMediaSettings::Get().GetCurrentVideoSettings().m_Sharpness)
  {
    VdpBool enabled[]={0};
    vdp_st = m_config.vdpProcs.vdp_video_mixer_set_feature_enables(m_videoMixer, ARSIZE(feature), feature, enabled);
    CheckStatus(vdp_st, __LINE__);
    return;
  }
  VdpBool enabled[]={1};
  vdp_st = m_config.vdpProcs.vdp_video_mixer_set_feature_enables(m_videoMixer, ARSIZE(feature), feature, enabled);
  CheckStatus(vdp_st, __LINE__);
  void* sh[] = { &CMediaSettings::Get().GetCurrentVideoSettings().m_Sharpness };
  CLog::Log(LOGNOTICE,"Setting Sharpness to %f",CMediaSettings::Get().GetCurrentVideoSettings().m_Sharpness);
  vdp_st = m_config.vdpProcs.vdp_video_mixer_set_attribute_values(m_videoMixer, ARSIZE(attributes), attributes, sh);
  CheckStatus(vdp_st, __LINE__);
}

EINTERLACEMETHOD CMixer::GetDeinterlacingMethod(bool log /* = false */)
{
  EINTERLACEMETHOD method = CMediaSettings::Get().GetCurrentVideoSettings().m_InterlaceMethod;
  if (method == VS_INTERLACEMETHOD_AUTO)
  {
    int deint = -1;
//    if (m_config.outHeight >= 720)
//      deint = g_advancedSettings.m_videoVDPAUdeintHD;
//    else
//      deint = g_advancedSettings.m_videoVDPAUdeintSD;

    if (deint != -1)
    {
      if (m_config.vdpau->Supports(EINTERLACEMETHOD(deint)))
      {
        method = EINTERLACEMETHOD(deint);
        if (log)
          CLog::Log(LOGNOTICE, "CVDPAU::GetDeinterlacingMethod: set de-interlacing to %d",  deint);
      }
      else
      {
        if (log)
          CLog::Log(LOGWARNING, "CVDPAU::GetDeinterlacingMethod: method for de-interlacing (advanced settings) not supported");
      }
    }
  }
  return method;
}

void CMixer::SetDeinterlacing()
{
  VdpStatus vdp_st;

  if (m_videoMixer == VDP_INVALID_HANDLE)
    return;

  EDEINTERLACEMODE   mode = CMediaSettings::Get().GetCurrentVideoSettings().m_DeinterlaceMode;
  EINTERLACEMETHOD method = GetDeinterlacingMethod(true);

  VdpVideoMixerFeature feature[] = { VDP_VIDEO_MIXER_FEATURE_DEINTERLACE_TEMPORAL,
                                     VDP_VIDEO_MIXER_FEATURE_DEINTERLACE_TEMPORAL_SPATIAL,
                                     VDP_VIDEO_MIXER_FEATURE_INVERSE_TELECINE };

  if (mode == VS_DEINTERLACEMODE_OFF)
  {
    VdpBool enabled[] = {0,0,0};
    vdp_st = m_config.vdpProcs.vdp_video_mixer_set_feature_enables(m_videoMixer, ARSIZE(feature), feature, enabled);
  }
  else
  {
    if (method == VS_INTERLACEMETHOD_AUTO)
    {
      VdpBool enabled[] = {1,0,0};
      if (g_advancedSettings.m_videoVDPAUtelecine)
        enabled[2] = 1;
      vdp_st = m_config.vdpProcs.vdp_video_mixer_set_feature_enables(m_videoMixer, ARSIZE(feature), feature, enabled);
    }
    else if (method == VS_INTERLACEMETHOD_VDPAU_TEMPORAL
         ||  method == VS_INTERLACEMETHOD_VDPAU_TEMPORAL_HALF)
    {
      VdpBool enabled[] = {1,0,0};
      if (g_advancedSettings.m_videoVDPAUtelecine)
        enabled[2] = 1;
      vdp_st = m_config.vdpProcs.vdp_video_mixer_set_feature_enables(m_videoMixer, ARSIZE(feature), feature, enabled);
    }
    else if (method == VS_INTERLACEMETHOD_VDPAU_TEMPORAL_SPATIAL
         ||  method == VS_INTERLACEMETHOD_VDPAU_TEMPORAL_SPATIAL_HALF)
    {
      VdpBool enabled[] = {1,1,0};
      if (g_advancedSettings.m_videoVDPAUtelecine)
        enabled[2] = 1;
      vdp_st = m_config.vdpProcs.vdp_video_mixer_set_feature_enables(m_videoMixer, ARSIZE(feature), feature, enabled);
    }
    else
    {
      VdpBool enabled[]={0,0,0};
      vdp_st = m_config.vdpProcs.vdp_video_mixer_set_feature_enables(m_videoMixer, ARSIZE(feature), feature, enabled);
    }
  }
  CheckStatus(vdp_st, __LINE__);

  SetDeintSkipChroma();

  m_config.useInteropYuv = !CSettings::Get().GetBool("videoplayer.usevdpaumixer");
}

void CMixer::SetDeintSkipChroma()
{
  VdpVideoMixerAttribute attribute[] = { VDP_VIDEO_MIXER_ATTRIBUTE_SKIP_CHROMA_DEINTERLACE};
  VdpStatus vdp_st;

  uint8_t val;
  if (g_advancedSettings.m_videoVDPAUdeintSkipChromaHD && m_config.outHeight >= 720)
    val = 1;
  else
    val = 0;

  void const *values[]={&val};
  vdp_st = m_config.vdpProcs.vdp_video_mixer_set_attribute_values(m_videoMixer, ARSIZE(attribute), attribute, values);

  CheckStatus(vdp_st, __LINE__);
}

void CMixer::SetHWUpscaling()
{
#ifdef VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L1

  VdpStatus vdp_st;
  VdpBool enabled[]={1};
  switch (m_config.upscale)
  {
    case 9:
       if (m_config.vdpau->Supports(VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L9))
       {
          VdpVideoMixerFeature feature[] = { VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L9 };
          vdp_st = m_config.vdpProcs.vdp_video_mixer_set_feature_enables(m_videoMixer, ARSIZE(feature), feature, enabled);
          break;
       }
    case 8:
       if (m_config.vdpau->Supports(VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L8))
       {
          VdpVideoMixerFeature feature[] = { VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L8 };
          vdp_st = m_config.vdpProcs.vdp_video_mixer_set_feature_enables(m_videoMixer, ARSIZE(feature), feature, enabled);
          break;
       }
    case 7:
       if (m_config.vdpau->Supports(VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L7))
       {
          VdpVideoMixerFeature feature[] = { VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L7 };
          vdp_st = m_config.vdpProcs.vdp_video_mixer_set_feature_enables(m_videoMixer, ARSIZE(feature), feature, enabled);
          break;
       }
    case 6:
       if (m_config.vdpau->Supports(VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L6))
       {
          VdpVideoMixerFeature feature[] = { VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L6 };
          vdp_st = m_config.vdpProcs.vdp_video_mixer_set_feature_enables(m_videoMixer, ARSIZE(feature), feature, enabled);
          break;
       }
    case 5:
       if (m_config.vdpau->Supports(VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L5))
       {
          VdpVideoMixerFeature feature[] = { VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L5 };
          vdp_st = m_config.vdpProcs.vdp_video_mixer_set_feature_enables(m_videoMixer, ARSIZE(feature), feature, enabled);
          break;
       }
    case 4:
       if (m_config.vdpau->Supports(VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L4))
       {
          VdpVideoMixerFeature feature[] = { VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L4 };
          vdp_st = m_config.vdpProcs.vdp_video_mixer_set_feature_enables(m_videoMixer, ARSIZE(feature), feature, enabled);
          break;
       }
    case 3:
       if (m_config.vdpau->Supports(VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L3))
       {
          VdpVideoMixerFeature feature[] = { VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L3 };
          vdp_st = m_config.vdpProcs.vdp_video_mixer_set_feature_enables(m_videoMixer, ARSIZE(feature), feature, enabled);
          break;
       }
    case 2:
       if (m_config.vdpau->Supports(VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L2))
       {
          VdpVideoMixerFeature feature[] = { VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L2 };
          vdp_st = m_config.vdpProcs.vdp_video_mixer_set_feature_enables(m_videoMixer, ARSIZE(feature), feature, enabled);
          break;
       }
    case 1:
       if (m_config.vdpau->Supports(VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L1))
       {
          VdpVideoMixerFeature feature[] = { VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L1 };
          vdp_st = m_config.vdpProcs.vdp_video_mixer_set_feature_enables(m_videoMixer, ARSIZE(feature), feature, enabled);
          break;
       }
    default:
       DisableHQScaling();
       return;
  }
  CheckStatus(vdp_st, __LINE__);
#endif
}

void CMixer::DisableHQScaling()
{
  VdpStatus vdp_st;

  if (m_videoMixer == VDP_INVALID_HANDLE)
    return;

  if(m_config.vdpau->Supports(VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L1))
  {
    VdpVideoMixerFeature feature[] = { VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L1 };
    VdpBool enabled[]={0};
    vdp_st = m_config.vdpProcs.vdp_video_mixer_set_feature_enables(m_videoMixer, ARSIZE(feature), feature, enabled);
    CheckStatus(vdp_st, __LINE__);
  }

  if(m_config.vdpau->Supports(VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L2))
  {
    VdpVideoMixerFeature feature[] = { VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L2 };
    VdpBool enabled[]={0};
    vdp_st = m_config.vdpProcs.vdp_video_mixer_set_feature_enables(m_videoMixer, ARSIZE(feature), feature, enabled);
    CheckStatus(vdp_st, __LINE__);
  }

  if(m_config.vdpau->Supports(VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L3))
  {
    VdpVideoMixerFeature feature[] = { VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L3 };
    VdpBool enabled[]={0};
    vdp_st = m_config.vdpProcs.vdp_video_mixer_set_feature_enables(m_videoMixer, ARSIZE(feature), feature, enabled);
    CheckStatus(vdp_st, __LINE__);
  }

  if(m_config.vdpau->Supports(VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L4))
  {
    VdpVideoMixerFeature feature[] = { VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L4 };
    VdpBool enabled[]={0};
    vdp_st = m_config.vdpProcs.vdp_video_mixer_set_feature_enables(m_videoMixer, ARSIZE(feature), feature, enabled);
    CheckStatus(vdp_st, __LINE__);
  }

  if(m_config.vdpau->Supports(VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L5))
  {
    VdpVideoMixerFeature feature[] = { VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L5 };
    VdpBool enabled[]={0};
    vdp_st = m_config.vdpProcs.vdp_video_mixer_set_feature_enables(m_videoMixer, ARSIZE(feature), feature, enabled);
    CheckStatus(vdp_st, __LINE__);
  }

  if(m_config.vdpau->Supports(VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L6))
  {
    VdpVideoMixerFeature feature[] = { VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L6 };
    VdpBool enabled[]={0};
    vdp_st = m_config.vdpProcs.vdp_video_mixer_set_feature_enables(m_videoMixer, ARSIZE(feature), feature, enabled);
    CheckStatus(vdp_st, __LINE__);
  }

  if(m_config.vdpau->Supports(VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L7))
  {
    VdpVideoMixerFeature feature[] = { VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L7 };
    VdpBool enabled[]={0};
    vdp_st = m_config.vdpProcs.vdp_video_mixer_set_feature_enables(m_videoMixer, ARSIZE(feature), feature, enabled);
    CheckStatus(vdp_st, __LINE__);
  }

  if(m_config.vdpau->Supports(VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L8))
  {
    VdpVideoMixerFeature feature[] = { VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L8 };
    VdpBool enabled[]={0};
    vdp_st = m_config.vdpProcs.vdp_video_mixer_set_feature_enables(m_videoMixer, ARSIZE(feature), feature, enabled);
    CheckStatus(vdp_st, __LINE__);
  }

  if(m_config.vdpau->Supports(VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L9))
  {
    VdpVideoMixerFeature feature[] = { VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L9 };
    VdpBool enabled[]={0};
    vdp_st = m_config.vdpProcs.vdp_video_mixer_set_feature_enables(m_videoMixer, ARSIZE(feature), feature, enabled);
    CheckStatus(vdp_st, __LINE__);
  }
}


void CMixer::Init()
{
  m_Brightness = 0.0;
  m_Contrast = 0.0;
  m_NoiseReduction = 0.0;
  m_Sharpness = 0.0;
  m_DeintMode = 0;
  m_Deint = 0;
  m_ColorMatrix = 0;
  m_PostProc = false;
  m_vdpError = false;

  m_config.upscale = g_advancedSettings.m_videoVDPAUScaling;
  m_config.useInteropYuv = !CSettings::Get().GetBool("videoplayer.usevdpaumixer");

  CreateVdpauMixer();
}

void CMixer::Uninit()
{
  Flush();
  while (!m_outputSurfaces.empty())
  {
    m_outputSurfaces.pop();
  }
  m_config.vdpProcs.vdp_video_mixer_destroy(m_videoMixer);

  delete [] m_BlackBar;
}

void CMixer::Flush()
{
  while (!m_mixerInput.empty())
  {
    CVdpauDecodedPicture pic = m_mixerInput.back();
    m_mixerInput.pop_back();
    CSingleLock lock(*m_config.videoSurfaceSec);
    if (pic.render)
      pic.render->state &= ~FF_VDPAU_STATE_USED_FOR_RENDER;
  }
  while (!m_decodedPics.empty())
  {
    CVdpauDecodedPicture pic = m_decodedPics.front();
    m_decodedPics.pop();
    CSingleLock lock(*m_config.videoSurfaceSec);
    if (pic.render)
      pic.render->state &= ~FF_VDPAU_STATE_USED_FOR_RENDER;
  }
  Message *msg;
  while (m_dataPort.ReceiveOutMessage(&msg))
  {
    if (msg->signal == CMixerDataProtocol::FRAME)
    {
      CVdpauDecodedPicture pic = *(CVdpauDecodedPicture*)msg->data;
      CSingleLock lock(*m_config.videoSurfaceSec);
      if (pic.render)
        pic.render->state &= ~FF_VDPAU_STATE_USED_FOR_RENDER;
    }
    else if (msg->signal == CMixerDataProtocol::BUFFER)
    {
      VdpOutputSurface *surf;
      surf = (VdpOutputSurface*)msg->data;
      m_outputSurfaces.push(*surf);
    }
    msg->Release();
  }
}

void CMixer::InitCycle()
{
  CheckFeatures();
  uint64_t latency;
  int flags;
  m_config.stats->GetParams(latency, flags);
  latency = (latency*1000)/CurrentHostFrequency();
  // TODO
  if (0) //flags & DVP_FLAG_NO_POSTPROC)
    SetPostProcFeatures(false);
  else
    SetPostProcFeatures(true);

  m_config.stats->SetCanSkipDeint(false);

  EDEINTERLACEMODE   mode = CMediaSettings::Get().GetCurrentVideoSettings().m_DeinterlaceMode;
  EINTERLACEMETHOD method = GetDeinterlacingMethod();
  bool interlaced = m_mixerInput[1].DVDPic.iFlags & DVP_FLAG_INTERLACED;

  // TODO
  if (//!(flags & DVP_FLAG_NO_POSTPROC) &&
      (mode == VS_DEINTERLACEMODE_FORCE ||
      (mode == VS_DEINTERLACEMODE_AUTO && interlaced)))
  {
    if((method == VS_INTERLACEMETHOD_AUTO && interlaced)
      ||  method == VS_INTERLACEMETHOD_VDPAU_BOB
      ||  method == VS_INTERLACEMETHOD_VDPAU_TEMPORAL
      ||  method == VS_INTERLACEMETHOD_VDPAU_TEMPORAL_HALF
      ||  method == VS_INTERLACEMETHOD_VDPAU_TEMPORAL_SPATIAL
      ||  method == VS_INTERLACEMETHOD_VDPAU_TEMPORAL_SPATIAL_HALF
      ||  method == VS_INTERLACEMETHOD_VDPAU_INVERSE_TELECINE )
    {
      if(method == VS_INTERLACEMETHOD_VDPAU_TEMPORAL_HALF
        || method == VS_INTERLACEMETHOD_VDPAU_TEMPORAL_SPATIAL_HALF
        || !g_graphicsContext.IsFullScreenVideo())
        m_mixersteps = 1;
      else
      {
        m_mixersteps = 2;
        m_config.stats->SetCanSkipDeint(true);
      }

      // TODO
      if (0) //m_mixerInput[1].DVDPic.iFlags & DVP_FLAG_DROPDEINT)
      {
        m_mixersteps = 1;
      }

      if(m_mixerInput[1].DVDPic.iFlags & DVP_FLAG_TOP_FIELD_FIRST)
        m_mixerfield = VDP_VIDEO_MIXER_PICTURE_STRUCTURE_TOP_FIELD;
      else
        m_mixerfield = VDP_VIDEO_MIXER_PICTURE_STRUCTURE_BOTTOM_FIELD;

      m_mixerInput[1].DVDPic.format = RENDER_FMT_VDPAU;
      m_mixerInput[1].DVDPic.iFlags &= ~(DVP_FLAG_TOP_FIELD_FIRST |
                                        DVP_FLAG_REPEAT_TOP_FIELD |
                                        DVP_FLAG_INTERLACED);
      m_config.useInteropYuv = false;
    }
    else if (method == VS_INTERLACEMETHOD_RENDER_BOB)
    {
      m_mixersteps = 1;
      m_mixerfield = VDP_VIDEO_MIXER_PICTURE_STRUCTURE_FRAME;
      m_mixerInput[1].DVDPic.format = RENDER_FMT_VDPAU_420;
      m_config.useInteropYuv = true;
    }
    else
    {
      CLog::Log(LOGERROR, "CMixer::%s - interlace method: %d not supported, setting to AUTO", __FUNCTION__, method);
      m_mixersteps = 1;
      m_mixerfield = VDP_VIDEO_MIXER_PICTURE_STRUCTURE_FRAME;
      m_mixerInput[1].DVDPic.format = RENDER_FMT_VDPAU;
      m_mixerInput[1].DVDPic.iFlags &= ~(DVP_FLAG_TOP_FIELD_FIRST |
                                        DVP_FLAG_REPEAT_TOP_FIELD |
                                        DVP_FLAG_INTERLACED);

      CMediaSettings::Get().GetCurrentVideoSettings().m_InterlaceMethod = VS_INTERLACEMETHOD_AUTO;
    }
  }
  else
  {
    m_mixersteps = 1;
    m_mixerfield = VDP_VIDEO_MIXER_PICTURE_STRUCTURE_FRAME;

    if (m_config.useInteropYuv)
      m_mixerInput[1].DVDPic.format = RENDER_FMT_VDPAU_420;
    else
    {
      m_mixerInput[1].DVDPic.format = RENDER_FMT_VDPAU;
      m_mixerInput[1].DVDPic.iFlags &= ~(DVP_FLAG_TOP_FIELD_FIRST |
                                        DVP_FLAG_REPEAT_TOP_FIELD |
                                        DVP_FLAG_INTERLACED);
    }
  }
  m_mixerstep = 0;

  if (m_mixerInput[1].DVDPic.format == RENDER_FMT_VDPAU)
  {
    m_processPicture.outputSurface = m_outputSurfaces.front();
    m_mixerInput[1].DVDPic.iWidth = m_config.outWidth;
    m_mixerInput[1].DVDPic.iHeight = m_config.outHeight;
  }
  else
  {
    m_mixerInput[1].DVDPic.iWidth = m_config.vidWidth;
    m_mixerInput[1].DVDPic.iHeight = m_config.vidHeight;
  }

  m_processPicture.DVDPic = m_mixerInput[1].DVDPic;
  m_processPicture.render = m_mixerInput[1].render;
}

void CMixer::FiniCycle()
{
  while (m_mixerInput.size() > 3)
  {
    CVdpauDecodedPicture &tmp = m_mixerInput.back();
    if (tmp.render && m_processPicture.DVDPic.format != RENDER_FMT_VDPAU_420)
    {
      CSingleLock lock(*m_config.videoSurfaceSec);
      tmp.render->state &= ~FF_VDPAU_STATE_USED_FOR_RENDER;
    }
    m_mixerInput.pop_back();
//    m_config.stats->DecDecoded();
  }
}

void CMixer::ProcessPicture()
{
  if (m_processPicture.DVDPic.format == RENDER_FMT_VDPAU_420)
    return;

  VdpStatus vdp_st;

  if (m_mixerstep == 1)
  {
    if(m_mixerfield == VDP_VIDEO_MIXER_PICTURE_STRUCTURE_TOP_FIELD)
      m_mixerfield = VDP_VIDEO_MIXER_PICTURE_STRUCTURE_BOTTOM_FIELD;
    else
      m_mixerfield = VDP_VIDEO_MIXER_PICTURE_STRUCTURE_TOP_FIELD;
  }

  VdpVideoSurface past_surfaces[4] = { VDP_INVALID_HANDLE, VDP_INVALID_HANDLE, VDP_INVALID_HANDLE, VDP_INVALID_HANDLE };
  VdpVideoSurface futu_surfaces[2] = { VDP_INVALID_HANDLE, VDP_INVALID_HANDLE };
  uint32_t pastCount = 4;
  uint32_t futuCount = 2;

  if(m_mixerfield == VDP_VIDEO_MIXER_PICTURE_STRUCTURE_FRAME)
  {
    // use only 2 past 1 future for progressive/weave
    // (only used for postproc anyway eg noise reduction)
    if (m_mixerInput.size() > 3)
      past_surfaces[1] = m_mixerInput[3].render->surface;
    if (m_mixerInput.size() > 2)
      past_surfaces[0] = m_mixerInput[2].render->surface;
    futu_surfaces[0] = m_mixerInput[0].render->surface;
    pastCount = 2;
    futuCount = 1;
  }
  else
  {
    if(m_mixerstep == 0)
    { // first field
      if (m_mixerInput.size() > 3)
      {
        past_surfaces[3] = m_mixerInput[3].render->surface;
        past_surfaces[2] = m_mixerInput[3].render->surface;
      }
      if (m_mixerInput.size() > 2)
      {
        past_surfaces[1] = m_mixerInput[2].render->surface;
        past_surfaces[0] = m_mixerInput[2].render->surface;
      }
      futu_surfaces[0] = m_mixerInput[1].render->surface;
      futu_surfaces[1] = m_mixerInput[0].render->surface;;
    }
    else
    { // second field
      if (m_mixerInput.size() > 3)
      {
        past_surfaces[3] = m_mixerInput[3].render->surface;
      }
      if (m_mixerInput.size() > 2)
      {
        past_surfaces[2] = m_mixerInput[2].render->surface;
        past_surfaces[1] = m_mixerInput[2].render->surface;
      }
      past_surfaces[0] = m_mixerInput[1].render->surface;
      futu_surfaces[0] = m_mixerInput[1].render->surface;
      futu_surfaces[1] = m_mixerInput[1].render->surface;

      m_processPicture.DVDPic.pts = m_mixerInput[1].DVDPic.pts +
                                   (m_mixerInput[0].DVDPic.pts -
                                    m_mixerInput[1].DVDPic.pts) / 2;
      m_processPicture.DVDPic.dts = DVD_NOPTS_VALUE;
    }
    m_processPicture.DVDPic.iRepeatPicture = 0.0;
  } // interlaced

  VdpRect sourceRect;
  sourceRect.x0 = 0;
  sourceRect.y0 = 0;
  sourceRect.x1 = m_config.vidWidth;
  sourceRect.y1 = m_config.vidHeight;

  VdpRect destRect;
  destRect.x0 = 0;
  destRect.y0 = 0;
  destRect.x1 = m_config.outWidth;
  destRect.y1 = m_config.outHeight;

  // start vdpau video mixer
  vdp_st = m_config.vdpProcs.vdp_video_mixer_render(m_videoMixer,
                                VDP_INVALID_HANDLE,
                                0,
                                m_mixerfield,
                                pastCount,
                                past_surfaces,
                                m_mixerInput[1].render->surface,
                                futuCount,
                                futu_surfaces,
                                &sourceRect,
                                m_processPicture.outputSurface,
                                &destRect,
                                &destRect,
                                0,
                                NULL);
  CheckStatus(vdp_st, __LINE__);

  if (m_mixerfield != VDP_VIDEO_MIXER_PICTURE_STRUCTURE_FRAME)
  {
    // in order to clip top and bottom lines when de-interlacing
    // we black those lines as a work around for not working
    // background colour using the mixer
    // pixel perfect is preferred over overscanning or zooming

    VdpRect clipRect = destRect;
    clipRect.y1 = clipRect.y0 + 2;
    uint32_t *data[] = {m_BlackBar};
    uint32_t pitches[] = {destRect.x1};
    vdp_st = m_config.vdpProcs.vdp_output_surface_put_bits_native(m_processPicture.outputSurface,
                                            (void**)data,
                                            pitches,
                                            &clipRect);
    CheckStatus(vdp_st, __LINE__);

    clipRect = destRect;
    clipRect.y0 = clipRect.y1 - 2;
    vdp_st = m_config.vdpProcs.vdp_output_surface_put_bits_native(m_processPicture.outputSurface,
                                            (void**)data,
                                            pitches,
                                            &clipRect);
    CheckStatus(vdp_st, __LINE__);
  }
}


bool CMixer::CheckStatus(VdpStatus vdp_st, int line)
{
  if (vdp_st != VDP_STATUS_OK)
  {
    CLog::Log(LOGERROR, " (VDPAU) Error: %s(%d) at %s:%d\n", m_config.vdpProcs.vdp_get_error_string(vdp_st), vdp_st, __FILE__, line);
    m_vdpError = true;
    return true;
  }
  return false;
}

//-----------------------------------------------------------------------------
// Output
//-----------------------------------------------------------------------------
COutput::COutput(CEvent *inMsgEvent) :
  CThread("Vdpau Output Thread"),
  m_controlPort("OutputControlPort", inMsgEvent, &m_outMsgEvent),
  m_dataPort("OutputDataPort", inMsgEvent, &m_outMsgEvent),
  m_mixer(&m_outMsgEvent)
{
  m_inMsgEvent = inMsgEvent;

  CVdpauRenderPicture pic;
  pic.renderPicSection = &m_bufferPool.renderPicSec;
  pic.refCount = 0;
  for (unsigned int i = 0; i < NUM_RENDER_PICS; i++)
  {
    m_bufferPool.allRenderPics.push_back(pic);
  }
  for (unsigned int i = 0; i < m_bufferPool.allRenderPics.size(); ++i)
  {
    m_bufferPool.freeRenderPics.push_back(&m_bufferPool.allRenderPics[i]);
  }
}

void COutput::Start()
{
  Create();
}

COutput::~COutput()
{
  Dispose();

  m_bufferPool.freeRenderPics.clear();
  m_bufferPool.usedRenderPics.clear();
  m_bufferPool.allRenderPics.clear();
}

void COutput::Dispose()
{
  CSingleLock lock(g_graphicsContext);
  m_bStop = true;
  m_outMsgEvent.Set();
  StopThread();
  m_controlPort.Purge();
  m_dataPort.Purge();
}

void COutput::OnStartup()
{
  CLog::Log(LOGNOTICE, "COutput::OnStartup: Output Thread created");
}

void COutput::OnExit()
{
  CLog::Log(LOGNOTICE, "COutput::OnExit: Output Thread terminated");
}

enum OUTPUT_STATES
{
  O_TOP = 0,                      // 0
  O_TOP_ERROR,                    // 1
  O_TOP_UNCONFIGURED,             // 2
  O_TOP_CONFIGURED,               // 3
  O_TOP_CONFIGURED_IDLE,          // 4
  O_TOP_CONFIGURED_WORK,          // 5
};

int VDPAU_OUTPUT_parentStates[] = {
    -1,
    0, //TOP_ERROR
    0, //TOP_UNCONFIGURED
    0, //TOP_CONFIGURED
    3, //TOP_CONFIGURED_IDLE
    3, //TOP_CONFIGURED_WORK
};

void COutput::StateMachine(int signal, Protocol *port, Message *msg)
{
  for (int state = m_state; ; state = VDPAU_OUTPUT_parentStates[state])
  {
    switch (state)
    {
    case O_TOP: // TOP
      if (port == &m_controlPort)
      {
        switch (signal)
        {
        case COutputControlProtocol::FLUSH:
          msg->Reply(COutputControlProtocol::ACC);
          return;
        case COutputControlProtocol::PRECLEANUP:
          msg->Reply(COutputControlProtocol::ACC);
          return;
        default:
          break;
        }
      }
      else if (port == &m_dataPort)
      {
        switch (signal)
        {
        case COutputDataProtocol::RETURNPIC:
          CVdpauRenderPicture *pic;
          pic = *((CVdpauRenderPicture**)msg->data);
          ProcessReturnPicture(pic);
          return;
        default:
          break;
        }
      }
      {
        std::string portName = port == NULL ? "timer" : port->portName;
        CLog::Log(LOGWARNING, "COutput::%s - signal: %d form port: %s not handled for state: %d", __FUNCTION__, signal, portName.c_str(), m_state);
      }
      return;

    case O_TOP_ERROR:
      break;

    case O_TOP_UNCONFIGURED:
      if (port == &m_controlPort)
      {
        switch (signal)
        {
        case COutputControlProtocol::INIT:
          CVdpauConfig *data;
          data = (CVdpauConfig*)msg->data;
          if (data)
          {
            m_config = *data;
          }
          Init();
          Message *reply;
          if (m_mixer.m_controlPort.SendOutMessageSync(CMixerControlProtocol::INIT,
                                     &reply, 1000, &m_config, sizeof(m_config)))
          {
            if (reply->signal != CMixerControlProtocol::ACC)
              m_vdpError = true;
            reply->Release();
          }

          // set initial number of
          m_bufferPool.numOutputSurfaces = 4;
          EnsureBufferPool();
          if (!m_vdpError)
          {
            m_state = O_TOP_CONFIGURED_IDLE;
            msg->Reply(COutputControlProtocol::ACC);
          }
          else
          {
            m_state = O_TOP_ERROR;
            msg->Reply(COutputControlProtocol::ERROR);
          }
          return;
        default:
          break;
        }
      }
      break;

    case O_TOP_CONFIGURED:
      if (port == &m_controlPort)
      {
        switch (signal)
        {
        case COutputControlProtocol::FLUSH:
          Flush();
          msg->Reply(COutputControlProtocol::ACC);
          return;
        case COutputControlProtocol::PRECLEANUP:
          Flush();
          PreCleanup();
          msg->Reply(COutputControlProtocol::ACC);
          return;
        default:
          break;
        }
      }
      else if (port == &m_dataPort)
      {
        switch (signal)
        {
        case COutputDataProtocol::NEWFRAME:
          CVdpauDecodedPicture *frame;
          frame = (CVdpauDecodedPicture*)msg->data;
          if (frame)
          {
            m_mixer.m_dataPort.SendOutMessage(CMixerDataProtocol::FRAME,
                                               frame,sizeof(CVdpauDecodedPicture));
          }
          return;
        case COutputDataProtocol::RETURNPIC:
          CVdpauRenderPicture *pic;
          pic = *((CVdpauRenderPicture**)msg->data);
          ProcessReturnPicture(pic);
          m_controlPort.SendInMessage(COutputControlProtocol::STATS);
          m_state = O_TOP_CONFIGURED_WORK;
          m_extTimeout = 0;
          return;
        default:
          break;
        }
      }
      else if (port == &m_mixer.m_dataPort)
      {
        switch (signal)
        {
        case CMixerDataProtocol::PICTURE:
          CVdpauProcessedPicture *pic;
          pic = (CVdpauProcessedPicture*)msg->data;
          m_bufferPool.processedPics.push(*pic);
          m_state = O_TOP_CONFIGURED_WORK;
          m_extTimeout = 0;
          return;
        default:
          break;
        }
      }
      break;

    case O_TOP_CONFIGURED_IDLE:
      if (port == NULL) // timeout
      {
        switch (signal)
        {
        case COutputControlProtocol::TIMEOUT:
//          uint16_t decoded, processed, render;
//          m_config.stats->Get(decoded, processed, render);
//          CLog::Log(LOGDEBUG, "CVDPAU::COutput - timeout idle: decoded: %d, proc: %d, render: %d", decoded, processed, render);
          return;
        default:
          break;
        }
      }
      break;

    case O_TOP_CONFIGURED_WORK:
      if (port == NULL) // timeout
      {
        switch (signal)
        {
        case COutputControlProtocol::TIMEOUT:
          if (HasWork())
          {
            CVdpauRenderPicture *pic;
            pic = ProcessMixerPicture();
            if (pic)
            {
              m_config.stats->DecProcessed();
              m_config.stats->IncRender();
              m_dataPort.SendInMessage(COutputDataProtocol::PICTURE, &pic, sizeof(pic));
            }
            m_extTimeout = 1;
          }
          else
          {
            m_state = O_TOP_CONFIGURED_IDLE;
            m_extTimeout = 100;
          }
          return;
        default:
          break;
        }
      }
      break;

    default: // we are in no state, should not happen
      CLog::Log(LOGERROR, "COutput::%s - no valid state: %d", __FUNCTION__, m_state);
      return;
    }
  } // for
}

void COutput::Process()
{
  Message *msg = NULL;
  Protocol *port = NULL;
  bool gotMsg;

  m_state = O_TOP_UNCONFIGURED;
  m_extTimeout = 1000;
  m_bStateMachineSelfTrigger = false;

  while (!m_bStop)
  {
    gotMsg = false;

    if (m_bStateMachineSelfTrigger)
    {
      m_bStateMachineSelfTrigger = false;
      // self trigger state machine
      StateMachine(msg->signal, port, msg);
      if (!m_bStateMachineSelfTrigger)
      {
        msg->Release();
        msg = NULL;
      }
      continue;
    }
    // check control port
    else if (m_controlPort.ReceiveOutMessage(&msg))
    {
      gotMsg = true;
      port = &m_controlPort;
    }
    // check data port
    else if (m_dataPort.ReceiveOutMessage(&msg))
    {
      gotMsg = true;
      port = &m_dataPort;
    }
    // check mixer data port
    else if (m_mixer.m_dataPort.ReceiveInMessage(&msg))
    {
      gotMsg = true;
      port = &m_mixer.m_dataPort;
    }
    if (gotMsg)
    {
      StateMachine(msg->signal, port, msg);
      if (!m_bStateMachineSelfTrigger)
      {
        msg->Release();
        msg = NULL;
      }
      continue;
    }

    // wait for message
    else if (m_outMsgEvent.WaitMSec(m_extTimeout))
    {
      continue;
    }
    // time out
    else
    {
      msg = m_controlPort.GetMessage();
      msg->signal = COutputControlProtocol::TIMEOUT;
      port = 0;
      // signal timeout to state machine
      StateMachine(msg->signal, port, msg);
      if (!m_bStateMachineSelfTrigger)
      {
        msg->Release();
        msg = NULL;
      }
    }
  }
  Flush();
  Uninit();
}

bool COutput::Init()
{
  if (!CreateGlxContext())
    return false;

  if (!GLInit())
    return false;

  m_mixer.Start();
  m_vdpError = false;

  return true;
}

bool COutput::Uninit()
{
  m_mixer.Dispose();
  GLUnmapSurfaces();
  GLUnbindPixmaps();
  ReleaseBufferPool();
  DestroyGlxContext();
  return true;
}

void COutput::Flush()
{
  if (m_mixer.IsActive())
  {
    Message *reply;
    if (m_mixer.m_controlPort.SendOutMessageSync(CMixerControlProtocol::FLUSH,
                                                 &reply,
                                                 2000))
    {
      reply->Release();
    }
    else
      CLog::Log(LOGERROR, "Coutput::%s - failed to flush mixer", __FUNCTION__);
  }

  Message *msg;
  while (m_mixer.m_dataPort.ReceiveInMessage(&msg))
  {
    if (msg->signal == CMixerDataProtocol::PICTURE)
    {
      CVdpauProcessedPicture pic = *(CVdpauProcessedPicture*)msg->data;
      if (pic.DVDPic.format == RENDER_FMT_VDPAU_420)
      {
        CSingleLock lock(*m_config.videoSurfaceSec);
        if (pic.render)
          pic.render->state &= ~FF_VDPAU_STATE_USED_FOR_RENDER;
      }
    }
    msg->Release();
  }

  while (m_dataPort.ReceiveOutMessage(&msg))
  {
    if (msg->signal == COutputDataProtocol::NEWFRAME)
    {
      CVdpauDecodedPicture pic = *(CVdpauDecodedPicture*)msg->data;
      CSingleLock lock(*m_config.videoSurfaceSec);
      if (pic.render)
        pic.render->state &= ~FF_VDPAU_STATE_USED_FOR_RENDER;
    }
    else if (msg->signal == COutputDataProtocol::RETURNPIC)
    {
      CVdpauRenderPicture *pic;
      pic = *((CVdpauRenderPicture**)msg->data);
      ProcessReturnPicture(pic);
    }
    msg->Release();
  }

  while (m_dataPort.ReceiveInMessage(&msg))
  {
    if (msg->signal == COutputDataProtocol::PICTURE)
    {
      CVdpauRenderPicture *pic;
      pic = *((CVdpauRenderPicture**)msg->data);
      ProcessReturnPicture(pic);
    }
  }

  // reset used render flag which was cleared on mixer flush
  std::deque<CVdpauRenderPicture*>::iterator it;
  for (it = m_bufferPool.usedRenderPics.begin(); it != m_bufferPool.usedRenderPics.end(); ++it)
  {
    if ((*it)->DVDPic.format == RENDER_FMT_VDPAU_420)
    {
      std::map<VdpVideoSurface, VdpauBufferPool::GLVideoSurface>::iterator it2;
      it2 = m_bufferPool.glVideoSurfaceMap.find((*it)->sourceIdx);
      if (it2 == m_bufferPool.glVideoSurfaceMap.end())
      {
        CLog::Log(LOGDEBUG, "COutput::Flush - gl surface not found");
        continue;
      }
      vdpau_render_state *render = it2->second.sourceVuv;
      if (render)
        render->state |= FF_VDPAU_STATE_USED_FOR_RENDER;
    }
  }
}

bool COutput::HasWork()
{
  if (m_config.usePixmaps)
  {
    if (!m_bufferPool.processedPics.empty() && FindFreePixmap() >= 0)
      return true;
    if (!m_bufferPool.notVisiblePixmaps.empty() && !m_bufferPool.freeRenderPics.empty())
      return true;
    return false;
  }
  else
  {
    if (!m_bufferPool.processedPics.empty() && !m_bufferPool.freeRenderPics.empty())
      return true;
    return false;
  }
}

CVdpauRenderPicture* COutput::ProcessMixerPicture()
{
  CVdpauRenderPicture *retPic = 0;

  if (m_config.usePixmaps)
  {
    if (!m_bufferPool.processedPics.empty() && FindFreePixmap() >= 0)
    {
      unsigned int i = FindFreePixmap();
      VdpauBufferPool::Pixmaps *pixmap = &m_bufferPool.pixmaps[i];
      pixmap->used = true;
      CVdpauProcessedPicture pic = m_bufferPool.processedPics.front();
      m_bufferPool.processedPics.pop();
      pixmap->surface = pic.outputSurface;
      pixmap->DVDPic = pic.DVDPic;
      pixmap->id = i;
      m_bufferPool.notVisiblePixmaps.push_back(pixmap);
      m_config.vdpProcs.vdp_presentation_queue_display(pixmap->vdp_flip_queue,
                                                       pixmap->surface,0,0,0);
    }
    if (!m_bufferPool.notVisiblePixmaps.empty() && !m_bufferPool.freeRenderPics.empty())
    {
      VdpStatus vdp_st;
      VdpTime time;
      VdpPresentationQueueStatus status;
      VdpauBufferPool::Pixmaps *pixmap = m_bufferPool.notVisiblePixmaps.front();
      vdp_st = m_config.vdpProcs.vdp_presentation_queue_query_surface_status(
                        pixmap->vdp_flip_queue, pixmap->surface, &status, &time);

      if (vdp_st == VDP_STATUS_OK && status == VDP_PRESENTATION_QUEUE_STATUS_VISIBLE)
      {
        retPic = m_bufferPool.freeRenderPics.front();
        m_bufferPool.freeRenderPics.pop_front();
        m_bufferPool.usedRenderPics.push_back(retPic);
        retPic->sourceIdx = pixmap->id;
        retPic->DVDPic = pixmap->DVDPic;
        retPic->valid = true;
        retPic->texture[0] = pixmap->texture;
        retPic->crop = CRect(0,0,0,0);
        m_bufferPool.notVisiblePixmaps.pop_front();
        m_mixer.m_dataPort.SendOutMessage(CMixerDataProtocol::BUFFER, &pixmap->surface, sizeof(pixmap->surface));
      }
    }
  } // pixmap
  else if (!m_bufferPool.processedPics.empty() && !m_bufferPool.freeRenderPics.empty())
  {
    retPic = m_bufferPool.freeRenderPics.front();
    m_bufferPool.freeRenderPics.pop_front();
    m_bufferPool.usedRenderPics.push_back(retPic);
    CVdpauProcessedPicture procPic = m_bufferPool.processedPics.front();
    m_bufferPool.processedPics.pop();

    retPic->DVDPic = procPic.DVDPic;
    retPic->valid = true;
    if (retPic->DVDPic.format == RENDER_FMT_VDPAU)
    {
      m_config.useInteropYuv = false;
      m_bufferPool.numOutputSurfaces = NUM_RENDER_PICS;
      EnsureBufferPool();
      GLMapSurfaces();
      retPic->sourceIdx = procPic.outputSurface;
      retPic->texture[0] = m_bufferPool.glOutputSurfaceMap[procPic.outputSurface].texture[0];
      retPic->crop = CRect(0,0,0,0);
    }
    else
    {
      m_config.useInteropYuv = true;
      GLMapSurfaces();
      retPic->sourceIdx = procPic.render->surface;
      for (unsigned int i=0; i<4; ++i)
        retPic->texture[i] = m_bufferPool.glVideoSurfaceMap[procPic.render->surface].texture[i];
      retPic->texWidth = m_config.surfaceWidth;
      retPic->texHeight = m_config.surfaceHeight;
      retPic->crop.x1 = 0;
      retPic->crop.y1 = 0;
      retPic->crop.x2 = m_config.surfaceWidth - m_config.vidWidth;
      retPic->crop.y2 = m_config.surfaceHeight - m_config.vidHeight;
    }
  }
  return retPic;
}

void COutput::ProcessReturnPicture(CVdpauRenderPicture *pic)
{
  std::deque<CVdpauRenderPicture*>::iterator it;
  it = std::find(m_bufferPool.usedRenderPics.begin(), m_bufferPool.usedRenderPics.end(), pic);
  if (it == m_bufferPool.usedRenderPics.end())
  {
    CLog::Log(LOGWARNING, "COutput::ProcessReturnPicture - pic not found");
    return;
  }
  m_bufferPool.usedRenderPics.erase(it);
  m_bufferPool.freeRenderPics.push_back(pic);
  if (!pic->valid)
  {
    CLog::Log(LOGDEBUG, "COutput::%s - return of invalid render pic", __FUNCTION__);
    return;
  }

  if (m_config.usePixmaps)
  {
    m_bufferPool.pixmaps[pic->sourceIdx].used = false;
    return;
  }
  else if (pic->DVDPic.format == RENDER_FMT_VDPAU_420)
  {
    std::map<VdpVideoSurface, VdpauBufferPool::GLVideoSurface>::iterator it;
    it = m_bufferPool.glVideoSurfaceMap.find(pic->sourceIdx);
    if (it == m_bufferPool.glVideoSurfaceMap.end())
    {
      CLog::Log(LOGDEBUG, "COutput::ProcessReturnPicture - gl surface not found");
      return;
    }
    vdpau_render_state *render = it->second.sourceVuv;
    CSingleLock lock(*m_config.videoSurfaceSec);
    render->state &= ~FF_VDPAU_STATE_USED_FOR_RENDER;
  }
  else if (pic->DVDPic.format == RENDER_FMT_VDPAU)
  {
    std::map<VdpOutputSurface, VdpauBufferPool::GLVideoSurface>::iterator it;
    it = m_bufferPool.glOutputSurfaceMap.find(pic->sourceIdx);
    if (it == m_bufferPool.glOutputSurfaceMap.end())
    {
      CLog::Log(LOGDEBUG, "COutput::ProcessReturnPicture - gl surface not found");
      return;
    }
    VdpOutputSurface outSurf = it->second.sourceRgb;
    m_mixer.m_dataPort.SendOutMessage(CMixerDataProtocol::BUFFER, &outSurf, sizeof(outSurf));
  }
}

int COutput::FindFreePixmap()
{
  // find free pixmap
  unsigned int i;
  for (i = 0; i < m_bufferPool.pixmaps.size(); ++i)
  {
    if (!m_bufferPool.pixmaps[i].used)
      break;
  }
  if (i == m_bufferPool.pixmaps.size())
    return -1;
  else
    return i;
}

bool COutput::EnsureBufferPool()
{
  VdpStatus vdp_st;

  // Creation of outputSurfaces
  VdpOutputSurface outputSurface;
  for (int i = m_bufferPool.outputSurfaces.size(); i < m_bufferPool.numOutputSurfaces; i++)
  {
    vdp_st = m_config.vdpProcs.vdp_output_surface_create(m_config.vdpDevice,
                                      VDP_RGBA_FORMAT_B8G8R8A8,
                                      m_config.outWidth,
                                      m_config.outHeight,
                                      &outputSurface);
    if (CheckStatus(vdp_st, __LINE__))
      return false;
    m_bufferPool.outputSurfaces.push_back(outputSurface);

    m_mixer.m_dataPort.SendOutMessage(CMixerDataProtocol::BUFFER,
                                      &outputSurface,
                                      sizeof(VdpOutputSurface));
    CLog::Log(LOGNOTICE, "VDPAU::COutput::InitBufferPool - Output Surface created");
  }


  if (m_config.usePixmaps && m_bufferPool.pixmaps.empty())
  {
    // create pixmpas
    VdpauBufferPool::Pixmaps pixmap;
    unsigned int numPixmaps = NUM_RENDER_PICS;
    for (unsigned int i = 0; i < numPixmaps; i++)
    {
      pixmap.pixmap = None;
      pixmap.glPixmap = None;
      pixmap.vdp_flip_queue = VDP_INVALID_HANDLE;
      pixmap.vdp_flip_target = VDP_INVALID_HANDLE;
      MakePixmap(pixmap);
      glXMakeCurrent(m_Display, None, NULL);
      vdp_st = m_config.vdpProcs.vdp_presentation_queue_target_create_x11(m_config.vdpDevice,
                                             pixmap.pixmap, //x_window,
                                             &pixmap.vdp_flip_target);

      CheckStatus(vdp_st, __LINE__);

      vdp_st = m_config.vdpProcs.vdp_presentation_queue_create(m_config.vdpDevice,
                                            pixmap.vdp_flip_target,
                                            &pixmap.vdp_flip_queue);
      CheckStatus(vdp_st, __LINE__);
      glXMakeCurrent(m_Display, m_glPixmap, m_glContext);

      pixmap.id = i;
      pixmap.used = false;
      m_bufferPool.pixmaps.push_back(pixmap);
    }
    GLBindPixmaps();
  }

  return true;
}

void COutput::ReleaseBufferPool()
{
  VdpStatus vdp_st;

  CSingleLock lock(m_bufferPool.renderPicSec);

  if (m_config.usePixmaps)
  {
    for (unsigned int i = 0; i < m_bufferPool.pixmaps.size(); ++i)
    {
      if (m_bufferPool.pixmaps[i].vdp_flip_queue != VDP_INVALID_HANDLE)
      {
        vdp_st = m_config.vdpProcs.vdp_presentation_queue_destroy(m_bufferPool.pixmaps[i].vdp_flip_queue);
        CheckStatus(vdp_st, __LINE__);
      }
      if (m_bufferPool.pixmaps[i].vdp_flip_target != VDP_INVALID_HANDLE)
      {
        vdp_st = m_config.vdpProcs.vdp_presentation_queue_target_destroy(m_bufferPool.pixmaps[i].vdp_flip_target);
        CheckStatus(vdp_st, __LINE__);
      }
      if (m_bufferPool.pixmaps[i].glPixmap)
      {
        glXDestroyPixmap(m_Display, m_bufferPool.pixmaps[i].glPixmap);
      }
      if (m_bufferPool.pixmaps[i].pixmap)
      {
        XFreePixmap(m_Display, m_bufferPool.pixmaps[i].pixmap);
      }
    }
    m_bufferPool.pixmaps.clear();
  }

  // release all output surfaces
  for (unsigned int i = 0; i < m_bufferPool.outputSurfaces.size(); ++i)
  {
    if (m_bufferPool.outputSurfaces[i] == VDP_INVALID_HANDLE)
      continue;
    vdp_st = m_config.vdpProcs.vdp_output_surface_destroy(m_bufferPool.outputSurfaces[i]);
    CheckStatus(vdp_st, __LINE__);
  }
  m_bufferPool.outputSurfaces.clear();

  // invalidate all used render pictures
  for (unsigned int i = 0; i < m_bufferPool.usedRenderPics.size(); ++i)
  {
    m_bufferPool.usedRenderPics[i]->valid = false;
  }
}

void COutput::PreCleanup()
{

  VdpStatus vdp_st;

  m_mixer.Dispose();

  CSingleLock lock(m_bufferPool.renderPicSec);
  for (unsigned int i = 0; i < m_bufferPool.outputSurfaces.size(); ++i)
  {
    if (m_bufferPool.outputSurfaces[i] == VDP_INVALID_HANDLE)
      continue;

    // check if output surface is in use
    bool used = false;
    std::deque<CVdpauRenderPicture*>::iterator it;
    for (it = m_bufferPool.usedRenderPics.begin(); it != m_bufferPool.usedRenderPics.end(); ++it)
    {
      if (((*it)->sourceIdx == m_bufferPool.outputSurfaces[i]) && (*it)->valid)
      {
        used = true;
        break;
      }
    }
    if (used)
      continue;

#ifdef GL_NV_vdpau_interop
    // unmap surface
    std::map<VdpOutputSurface, VdpauBufferPool::GLVideoSurface>::iterator it_map;
    it_map = m_bufferPool.glOutputSurfaceMap.find(m_bufferPool.outputSurfaces[i]);
    if (it_map == m_bufferPool.glOutputSurfaceMap.end())
    {
      CLog::Log(LOGERROR, "%s - could not find gl surface", __FUNCTION__);
      continue;
    }
    glVDPAUUnregisterSurfaceNV(it_map->second.glVdpauSurface);
    glDeleteTextures(1, it_map->second.texture);
    m_bufferPool.glOutputSurfaceMap.erase(it_map);
#endif

    vdp_st = m_config.vdpProcs.vdp_output_surface_destroy(m_bufferPool.outputSurfaces[i]);
    CheckStatus(vdp_st, __LINE__);

    m_bufferPool.outputSurfaces[i] = VDP_INVALID_HANDLE;

    CLog::Log(LOGDEBUG, "VDPAU::PreCleanup - released output surface");
  }

}

void COutput::InitMixer()
{
  for (unsigned int i = 0; i < m_bufferPool.outputSurfaces.size(); ++i)
  {
    m_mixer.m_dataPort.SendOutMessage(CMixerDataProtocol::BUFFER,
                                      &m_bufferPool.outputSurfaces[i],
                                      sizeof(VdpOutputSurface));
  }
}

bool COutput::MakePixmap(VdpauBufferPool::Pixmaps &pixmap)
{
  CLog::Log(LOGNOTICE,"Creating %ix%i pixmap", m_config.outWidth, m_config.outHeight);

    // Get our window attribs.
  XWindowAttributes wndattribs;
  XGetWindowAttributes(m_Display, g_Windowing.GetWindow(), &wndattribs);

  pixmap.pixmap = XCreatePixmap(m_Display,
                           g_Windowing.GetWindow(),
                           m_config.outWidth,
                           m_config.outHeight,
                           wndattribs.depth);
  if (!pixmap.pixmap)
  {
    CLog::Log(LOGERROR, "VDPAU::COUtput::MakePixmap - GLX Error: MakePixmap: Unable to create XPixmap");
    return false;
  }

//  XGCValues values = {};
//  GC xgc;
//  values.foreground = BlackPixel (m_Display, DefaultScreen (m_Display));
//  xgc = XCreateGC(m_Display, pixmap.pixmap, GCForeground, &values);
//  XFillRectangle(m_Display, pixmap.pixmap, xgc, 0, 0, m_config.outWidth, m_config.outHeight);
//  XFreeGC(m_Display, xgc);

  if(!MakePixmapGL(pixmap))
    return false;

  return true;
}

bool COutput::MakePixmapGL(VdpauBufferPool::Pixmaps &pixmap)
{
  int num=0;
  int fbConfigIndex = 0;

  int doubleVisAttributes[] = {
    GLX_RENDER_TYPE, GLX_RGBA_BIT,
    GLX_RED_SIZE, 8,
    GLX_GREEN_SIZE, 8,
    GLX_BLUE_SIZE, 8,
    GLX_ALPHA_SIZE, 8,
    GLX_DEPTH_SIZE, 8,
    GLX_DRAWABLE_TYPE, GLX_PIXMAP_BIT,
    GLX_BIND_TO_TEXTURE_RGBA_EXT, True,
    GLX_DOUBLEBUFFER, False,
    GLX_Y_INVERTED_EXT, True,
    GLX_X_RENDERABLE, True,
    None
  };

  int pixmapAttribs[] = {
    GLX_TEXTURE_TARGET_EXT, GLX_TEXTURE_2D_EXT,
    GLX_TEXTURE_FORMAT_EXT, GLX_TEXTURE_FORMAT_RGBA_EXT,
    None
  };

  GLXFBConfig *fbConfigs;
  fbConfigs = glXChooseFBConfig(m_Display, g_Windowing.GetCurrentScreen(), doubleVisAttributes, &num);
  if (fbConfigs==NULL)
  {
    CLog::Log(LOGERROR, "VDPAU::COutput::MakPixmapGL - No compatible framebuffers found");
    return false;
  }
  fbConfigIndex = 0;

  pixmap.glPixmap = glXCreatePixmap(m_Display, fbConfigs[fbConfigIndex], pixmap.pixmap, pixmapAttribs);

  if (!pixmap.glPixmap)
  {
    CLog::Log(LOGERROR, "VDPAU::COutput::MakPixmapGL - Could not create Pixmap");
    XFree(fbConfigs);
    return false;
  }
  XFree(fbConfigs);
  return true;
}

bool COutput::GLInit()
{
  glXBindTexImageEXT = NULL;
  glXReleaseTexImageEXT = NULL;
#ifdef GL_NV_vdpau_interop
  glVDPAUInitNV = NULL;
  glVDPAUFiniNV = NULL;
  glVDPAURegisterOutputSurfaceNV = NULL;
  glVDPAURegisterVideoSurfaceNV = NULL;
  glVDPAUIsSurfaceNV = NULL;
  glVDPAUUnregisterSurfaceNV = NULL;
  glVDPAUSurfaceAccessNV = NULL;
  glVDPAUMapSurfacesNV = NULL;
  glVDPAUUnmapSurfacesNV = NULL;
  glVDPAUGetSurfaceivNV = NULL;
#endif

  m_config.usePixmaps = false;

#ifdef GL_NV_vdpau_interop
  if (glewIsSupported("GL_NV_vdpau_interop"))
  {
    if (!glVDPAUInitNV)
      glVDPAUInitNV    = (PFNGLVDPAUINITNVPROC)glXGetProcAddress((GLubyte *) "glVDPAUInitNV");
    if (!glVDPAUFiniNV)
      glVDPAUFiniNV = (PFNGLVDPAUFININVPROC)glXGetProcAddress((GLubyte *) "glVDPAUFiniNV");
    if (!glVDPAURegisterOutputSurfaceNV)
      glVDPAURegisterOutputSurfaceNV    = (PFNGLVDPAUREGISTEROUTPUTSURFACENVPROC)glXGetProcAddress((GLubyte *) "glVDPAURegisterOutputSurfaceNV");
    if (!glVDPAURegisterVideoSurfaceNV)
      glVDPAURegisterVideoSurfaceNV    = (PFNGLVDPAUREGISTERVIDEOSURFACENVPROC)glXGetProcAddress((GLubyte *) "glVDPAURegisterVideoSurfaceNV");
    if (!glVDPAUIsSurfaceNV)
      glVDPAUIsSurfaceNV    = (PFNGLVDPAUISSURFACENVPROC)glXGetProcAddress((GLubyte *) "glVDPAUIsSurfaceNV");
    if (!glVDPAUUnregisterSurfaceNV)
      glVDPAUUnregisterSurfaceNV = (PFNGLVDPAUUNREGISTERSURFACENVPROC)glXGetProcAddress((GLubyte *) "glVDPAUUnregisterSurfaceNV");
    if (!glVDPAUSurfaceAccessNV)
      glVDPAUSurfaceAccessNV    = (PFNGLVDPAUSURFACEACCESSNVPROC)glXGetProcAddress((GLubyte *) "glVDPAUSurfaceAccessNV");
    if (!glVDPAUMapSurfacesNV)
      glVDPAUMapSurfacesNV = (PFNGLVDPAUMAPSURFACESNVPROC)glXGetProcAddress((GLubyte *) "glVDPAUMapSurfacesNV");
    if (!glVDPAUUnmapSurfacesNV)
      glVDPAUUnmapSurfacesNV = (PFNGLVDPAUUNMAPSURFACESNVPROC)glXGetProcAddress((GLubyte *) "glVDPAUUnmapSurfacesNV");
    if (!glVDPAUGetSurfaceivNV)
      glVDPAUGetSurfaceivNV = (PFNGLVDPAUGETSURFACEIVNVPROC)glXGetProcAddress((GLubyte *) "glVDPAUGetSurfaceivNV");

    CLog::Log(LOGNOTICE, "VDPAU::COutput GL interop supported");
  }
  else
#endif
  {
    m_config.usePixmaps = true;
    CSettings::Get().SetBool("videoplayer.usevdpaumixer",true);
  }
  if (!glXBindTexImageEXT)
    glXBindTexImageEXT    = (PFNGLXBINDTEXIMAGEEXTPROC)glXGetProcAddress((GLubyte *) "glXBindTexImageEXT");
  if (!glXReleaseTexImageEXT)
    glXReleaseTexImageEXT = (PFNGLXRELEASETEXIMAGEEXTPROC)glXGetProcAddress((GLubyte *) "glXReleaseTexImageEXT");

#ifdef GL_NV_vdpau_interop
  if (!m_config.usePixmaps)
  {
    while (glGetError() != GL_NO_ERROR);
    glVDPAUInitNV(reinterpret_cast<void*>(m_config.vdpDevice), reinterpret_cast<void*>(m_config.vdpProcs.vdp_get_proc_address));
    if (glGetError() != GL_NO_ERROR)
    {
      CLog::Log(LOGERROR, "VDPAU::COutput - GLInitInterop glVDPAUInitNV failed");
      m_vdpError = true;
      return false;
    }
    CLog::Log(LOGNOTICE, "VDPAU::COutput: vdpau gl interop initialized");
  }
#endif
  return true;
}

void COutput::GLMapSurfaces()
{
#ifdef GL_NV_vdpau_interop
  if (m_config.usePixmaps)
    return;

  if (m_config.useInteropYuv)
  {
    VdpauBufferPool::GLVideoSurface glSurface;
    if (m_config.videoSurfaces->size() != m_bufferPool.glVideoSurfaceMap.size())
    {
      CSingleLock lock(*m_config.videoSurfaceSec);
      for (unsigned int i = 0; i < m_config.videoSurfaces->size(); i++)
      {
        if ((*m_config.videoSurfaces)[i]->surface == VDP_INVALID_HANDLE)
          continue;

        if (m_bufferPool.glVideoSurfaceMap.find((*m_config.videoSurfaces)[i]->surface) == m_bufferPool.glVideoSurfaceMap.end())
        {
          glSurface.sourceVuv = (*m_config.videoSurfaces)[i];
          while (glGetError() != GL_NO_ERROR) ;
          glGenTextures(4, glSurface.texture);
          if (glGetError() != GL_NO_ERROR)
          {
             CLog::Log(LOGERROR, "VDPAU::COutput error creating texture");
             m_vdpError = true;
          }
          glSurface.glVdpauSurface = glVDPAURegisterVideoSurfaceNV(reinterpret_cast<void*>((*m_config.videoSurfaces)[i]->surface),
                                                    GL_TEXTURE_2D, 4, glSurface.texture);

          if (glGetError() != GL_NO_ERROR)
          {
            CLog::Log(LOGERROR, "VDPAU::COutput error register video surface");
            m_vdpError = true;
          }
          glVDPAUSurfaceAccessNV(glSurface.glVdpauSurface, GL_READ_ONLY);
          if (glGetError() != GL_NO_ERROR)
          {
            CLog::Log(LOGERROR, "VDPAU::COutput error setting access");
            m_vdpError = true;
          }
          glVDPAUMapSurfacesNV(1, &glSurface.glVdpauSurface);
          if (glGetError() != GL_NO_ERROR)
          {
            CLog::Log(LOGERROR, "VDPAU::COutput error mapping surface");
            m_vdpError = true;
          }
          m_bufferPool.glVideoSurfaceMap[(*m_config.videoSurfaces)[i]->surface] = glSurface;
          if (m_vdpError)
            return;
          CLog::Log(LOGNOTICE, "VDPAU::COutput registered surface");
        }
      }
    }
  }
  else
  {
    if (m_bufferPool.glOutputSurfaceMap.size() != m_bufferPool.numOutputSurfaces)
    {
      VdpauBufferPool::GLVideoSurface glSurface;
      for (unsigned int i = m_bufferPool.glOutputSurfaceMap.size(); i<m_bufferPool.outputSurfaces.size(); i++)
      {
        glSurface.sourceRgb = m_bufferPool.outputSurfaces[i];
        glGenTextures(1, glSurface.texture);
        glSurface.glVdpauSurface = glVDPAURegisterOutputSurfaceNV(reinterpret_cast<void*>(m_bufferPool.outputSurfaces[i]),
                                               GL_TEXTURE_2D, 1, glSurface.texture);
        if (glGetError() != GL_NO_ERROR)
        {
          CLog::Log(LOGERROR, "VDPAU::COutput error register output surface");
          m_vdpError = true;
        }
        glVDPAUSurfaceAccessNV(glSurface.glVdpauSurface, GL_READ_ONLY);
        if (glGetError() != GL_NO_ERROR)
        {
          CLog::Log(LOGERROR, "VDPAU::COutput error setting access");
          m_vdpError = true;
        }
        glVDPAUMapSurfacesNV(1, &glSurface.glVdpauSurface);
        if (glGetError() != GL_NO_ERROR)
        {
          CLog::Log(LOGERROR, "VDPAU::COutput error mapping surface");
          m_vdpError = true;
        }
        m_bufferPool.glOutputSurfaceMap[m_bufferPool.outputSurfaces[i]] = glSurface;
        if (m_vdpError)
          return;
      }
      CLog::Log(LOGNOTICE, "VDPAU::COutput registered output surfaces");
    }
  }
#endif
}

void COutput::GLUnmapSurfaces()
{
#ifdef GL_NV_vdpau_interop
  if (m_config.usePixmaps)
    return;

  { CSingleLock lock(*m_config.videoSurfaceSec);
    std::map<VdpVideoSurface, VdpauBufferPool::GLVideoSurface>::iterator it;
    for (it = m_bufferPool.glVideoSurfaceMap.begin(); it != m_bufferPool.glVideoSurfaceMap.end(); ++it)
    {
      glVDPAUUnregisterSurfaceNV(it->second.glVdpauSurface);
      glDeleteTextures(4, it->second.texture);
    }
    m_bufferPool.glVideoSurfaceMap.clear();
  }

  std::map<VdpOutputSurface, VdpauBufferPool::GLVideoSurface>::iterator it;
  for (it = m_bufferPool.glOutputSurfaceMap.begin(); it != m_bufferPool.glOutputSurfaceMap.end(); ++it)
  {
    glVDPAUUnregisterSurfaceNV(it->second.glVdpauSurface);
    glDeleteTextures(1, it->second.texture);
  }
  m_bufferPool.glOutputSurfaceMap.clear();

  glVDPAUFiniNV();

  CLog::Log(LOGNOTICE, "VDPAU::COutput: vdpau gl interop finished");

#endif
}

void COutput::GLBindPixmaps()
{
  if (!m_config.usePixmaps)
    return;

  for (unsigned int i = 0; i < m_bufferPool.pixmaps.size(); i++)
  {
    // create texture
    glGenTextures(1, &m_bufferPool.pixmaps[i].texture);

    //bind texture
    glBindTexture(GL_TEXTURE_2D, m_bufferPool.pixmaps[i].texture);

    // bind pixmap
    glXBindTexImageEXT(m_Display, m_bufferPool.pixmaps[i].glPixmap, GLX_FRONT_LEFT_EXT, NULL);

    glBindTexture(GL_TEXTURE_2D, 0);
  }

  CLog::Log(LOGNOTICE, "VDPAU::COutput: bound pixmaps");
}

void COutput::GLUnbindPixmaps()
{
  if (!m_config.usePixmaps)
    return;

  for (unsigned int i = 0; i < m_bufferPool.pixmaps.size(); i++)
  {
    // create texture
    if (!glIsTexture(m_bufferPool.pixmaps[i].texture))
      continue;

    //bind texture
    glBindTexture(GL_TEXTURE_2D, m_bufferPool.pixmaps[i].texture);

    // release pixmap
    glXReleaseTexImageEXT(m_Display, m_bufferPool.pixmaps[i].glPixmap, GLX_FRONT_LEFT_EXT);

    glBindTexture(GL_TEXTURE_2D, 0);

    glDeleteTextures(1, &m_bufferPool.pixmaps[i].texture);
  }
  CLog::Log(LOGNOTICE, "VDPAU::COutput: unbound pixmaps");
}

bool COutput::CheckStatus(VdpStatus vdp_st, int line)
{
  if (vdp_st != VDP_STATUS_OK)
  {
    CLog::Log(LOGERROR, " (VDPAU) Error: %s(%d) at %s:%d\n", m_config.vdpProcs.vdp_get_error_string(vdp_st), vdp_st, __FILE__, line);
    m_vdpError = true;
    return true;
  }
  return false;
}

bool COutput::CreateGlxContext()
{
  GLXContext   glContext;

  m_Display = g_Windowing.GetDisplay();
  glContext = g_Windowing.GetGlxContext();
  m_Window = g_Windowing.GetWindow();

  // Get our window attribs.
  XWindowAttributes wndattribs;
  XGetWindowAttributes(m_Display, m_Window, &wndattribs);

  // Get visual Info
  XVisualInfo visInfo;
  visInfo.visualid = wndattribs.visual->visualid;
  int nvisuals = 0;
  XVisualInfo* visuals = XGetVisualInfo(m_Display, VisualIDMask, &visInfo, &nvisuals);
  if (nvisuals != 1)
  {
    CLog::Log(LOGERROR, "VDPAU::COutput::CreateGlxContext - could not find visual");
    return false;
  }
  visInfo = visuals[0];
  XFree(visuals);

  m_pixmap = XCreatePixmap(m_Display,
                           m_Window,
                           192,
                           108,
                           visInfo.depth);
  if (!m_pixmap)
  {
    CLog::Log(LOGERROR, "VDPAU::COutput::CreateGlxContext - Unable to create XPixmap");
    return false;
  }

  // create gl pixmap
  m_glPixmap = glXCreateGLXPixmap(m_Display, &visInfo, m_pixmap);

  if (!m_glPixmap)
  {
    CLog::Log(LOGINFO, "VDPAU::COutput::CreateGlxContext - Could not create glPixmap");
    return false;
  }

  m_glContext = glXCreateContext(m_Display, &visInfo, glContext, True);

  if (!glXMakeCurrent(m_Display, m_glPixmap, m_glContext))
  {
    CLog::Log(LOGINFO, "VDPAU::COutput::CreateGlxContext - Could not make Pixmap current");
    return false;
  }

  CLog::Log(LOGNOTICE, "VDPAU::COutput::CreateGlxContext - created context");
  return true;
}

bool COutput::DestroyGlxContext()
{
  if (m_glContext)
  {
    glXMakeCurrent(m_Display, None, NULL);
    glXDestroyContext(m_Display, m_glContext);
  }
  m_glContext = 0;

  if (m_glPixmap)
    glXDestroyPixmap(m_Display, m_glPixmap);
  m_glPixmap = 0;

  if (m_pixmap)
    XFreePixmap(m_Display, m_pixmap);
  m_pixmap = 0;

  return true;
}

#endif
