/*
 *      Copyright (C) 2005-2009 Team XBMC
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

#include "system.h"
#ifdef HAVE_LIBVDPAU
#include <dlfcn.h>
#include "windowing/WindowingFactory.h"
#include "VDPAU.h"
#include "guilib/TextureManager.h"
#include "cores/VideoRenderers/RenderManager.h"
#include "DVDVideoCodecFFmpeg.h"
#include "DVDClock.h"
#include "settings/Settings.h"
#include "settings/GUISettings.h"
#include "settings/AdvancedSettings.h"
#include "Application.h"
#include "utils/MathUtils.h"
#include "DVDCodecs/DVDCodecUtils.h"

#define ARSIZE(x) (sizeof(x) / sizeof((x)[0]))

CVDPAU::Desc decoder_profiles[] = {
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
const size_t decoder_profile_count = sizeof(decoder_profiles)/sizeof(CVDPAU::Desc);

static float studioCSC[3][4] =
{
    { 1.0f,        0.0f, 1.57480000f,-0.78740000f},
    { 1.0f,-0.18737736f,-0.46813736f, 0.32775736f},
    { 1.0f, 1.85556000f,        0.0f,-0.92780000f}
};

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

//since libvdpau 0.4, vdp_device_create_x11() installs a callback on the Display*,
//if we unload libvdpau with dlclose(), we segfault on XCloseDisplay,
//so we just keep a static handle to libvdpau around
void* CVDPAU::dl_handle;

CVDPAU::CVDPAU()
{
  glXBindTexImageEXT = NULL;
  glXReleaseTexImageEXT = NULL;
  vdp_device = VDP_INVALID_HANDLE;
  surfaceNum      = presentSurfaceNum = 0;
  picAge.b_age    = picAge.ip_age[0] = picAge.ip_age[1] = 256*256*256*64;
  vdpauConfigured = false;
  m_DisplayState = VDPAU_OPEN;
  m_mixerfield = VDP_VIDEO_MIXER_PICTURE_STRUCTURE_FRAME;
  m_mixerstep  = 0;

  m_glPixmap = 0;
  m_Pixmap = 0;
  if (!glXBindTexImageEXT)
    glXBindTexImageEXT    = (PFNGLXBINDTEXIMAGEEXTPROC)glXGetProcAddress((GLubyte *) "glXBindTexImageEXT");
  if (!glXReleaseTexImageEXT)
    glXReleaseTexImageEXT = (PFNGLXRELEASETEXIMAGEEXTPROC)glXGetProcAddress((GLubyte *) "glXReleaseTexImageEXT");

  totalAvailableOutputSurfaces = 0;
  outputSurface = presentSurface = VDP_INVALID_HANDLE;
  vdp_flip_target = VDP_INVALID_HANDLE;
  vdp_flip_queue = VDP_INVALID_HANDLE;
  vid_width = vid_height = OutWidth = OutHeight = 0;
  memset(&outRect, 0, sizeof(VdpRect));
  memset(&outRectVid, 0, sizeof(VdpRect));

  tmpBrightness  = 0;
  tmpContrast    = 0;
  tmpDeintMode   = 0;
  tmpDeintGUI    = 0;
  tmpDeint       = 0;
  max_references = 0;

  for (int i = 0; i < NUM_OUTPUT_SURFACES; i++)
    outputSurfaces[i] = VDP_INVALID_HANDLE;

  videoMixer = VDP_INVALID_HANDLE;
  m_BlackBar = NULL;

  upScale = g_advancedSettings.m_videoVDPAUScaling;
}

bool CVDPAU::Open(AVCodecContext* avctx, const enum PixelFormat, unsigned int surfaces)
{
  if(avctx->coded_width  == 0
  || avctx->coded_height == 0)
  {
    CLog::Log(LOGWARNING,"(VDPAU) no width/height available, can't init");
    return false;
  }

  if (!dl_handle)
  {
    dl_handle  = dlopen("libvdpau.so.1", RTLD_LAZY);
    if (!dl_handle)
    {
      const char* error = dlerror();
      if (!error)
        error = "dlerror() returned NULL";

      CLog::Log(LOGNOTICE,"(VDPAU) Unable to get handle to libvdpau: %s", error);
      //g_application.m_guiDialogKaiToast.QueueNotification(CGUIDialogKaiToast::Error, "VDPAU", error, 10000);

      return false;
    }
  }

  if (!m_dllAvUtil.Load())
    return false;

  InitVDPAUProcs();

  if (vdp_device != VDP_INVALID_HANDLE)
  {
    SpewHardwareAvailable();

    VdpDecoderProfile profile = 0;
    if(avctx->codec_id == CODEC_ID_H264)
      profile = VDP_DECODER_PROFILE_H264_HIGH;
#ifdef VDP_DECODER_PROFILE_MPEG4_PART2_ASP
    else if(avctx->codec_id == CODEC_ID_MPEG4)
      profile = VDP_DECODER_PROFILE_MPEG4_PART2_ASP;
#endif
    if(profile)
    {
      if (!CDVDCodecUtils::IsVP3CompatibleWidth(avctx->coded_width))
        CLog::Log(LOGWARNING,"(VDPAU) width %i might not be supported because of hardware bug", avctx->width);
   
      /* attempt to create a decoder with this width/height, some sizes are not supported by hw */
      VdpStatus vdp_st;
      vdp_st = vdp_decoder_create(vdp_device, profile, avctx->coded_width, avctx->coded_height, 5, &decoder);

      if(vdp_st != VDP_STATUS_OK)
      {
        CLog::Log(LOGERROR, " (VDPAU) Error: %s(%d) checking for decoder support\n", vdp_get_error_string(vdp_st), vdp_st);
        FiniVDPAUProcs();
        return false;
      }

      vdp_decoder_destroy(decoder);
      CheckStatus(vdp_st, __LINE__);
    }

    InitCSCMatrix(avctx->coded_height);

    /* finally setup ffmpeg */
    avctx->get_buffer      = CVDPAU::FFGetBuffer;
    avctx->release_buffer  = CVDPAU::FFReleaseBuffer;
    avctx->draw_horiz_band = CVDPAU::FFDrawSlice;
    avctx->slice_flags=SLICE_FLAG_CODED_ORDER|SLICE_FLAG_ALLOW_FIELD;

    g_Windowing.Register(this);
    return true;
  }
  return false;
}

CVDPAU::~CVDPAU()
{
  Close();
}

void CVDPAU::Close()
{
  CLog::Log(LOGNOTICE, " (VDPAU) %s", __FUNCTION__);

  FiniVDPAUOutput();
  FiniVDPAUProcs();

  while (!m_videoSurfaces.empty())
  {
    vdpau_render_state *render = m_videoSurfaces.back();
    m_videoSurfaces.pop_back();
    if (render->bitstream_buffers_allocated)
      m_dllAvUtil.av_freep(&render->bitstream_buffers);
    render->bitstream_buffers_allocated = 0;
    free(render);
  }

  g_Windowing.Unregister(this);
  m_dllAvUtil.Unload();
}

bool CVDPAU::MakePixmapGL()
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
    GLX_DOUBLEBUFFER, True,
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
  fbConfigs = glXChooseFBConfig(m_Display, DefaultScreen(m_Display), doubleVisAttributes, &num);
  if (fbConfigs==NULL)
  {
    CLog::Log(LOGERROR, "GLX Error: MakePixmap: No compatible framebuffers found");
    return false;
  }
  CLog::Log(LOGDEBUG, "Found %d fbconfigs.", num);
  fbConfigIndex = 0;
  CLog::Log(LOGDEBUG, "Using fbconfig index %d.", fbConfigIndex);

  m_glPixmap = glXCreatePixmap(m_Display, fbConfigs[fbConfigIndex], m_Pixmap, pixmapAttribs);

  if (!m_glPixmap)
  {
    CLog::Log(LOGINFO, "GLX Error: Could not create Pixmap");
    XFree(fbConfigs);
    return false;
  }
  XFree(fbConfigs);

  return true;
}

bool CVDPAU::MakePixmap(int width, int height)
{
  //pick the smallest dimensions, so we downscale with vdpau and upscale with opengl when appropriate
  //this requires the least amount of gpu memory bandwidth
  if (g_graphicsContext.GetWidth() < width || g_graphicsContext.GetHeight() < height || upScale)
  {
    //scale width to desktop size if the aspect ratio is the same or bigger than the desktop
    if ((double)height * g_graphicsContext.GetWidth() / width <= (double)g_graphicsContext.GetHeight())
    {
      OutWidth = g_graphicsContext.GetWidth();
      OutHeight = MathUtils::round_int((double)height * g_graphicsContext.GetWidth() / width);
    }
    else //scale height to the desktop size if the aspect ratio is smaller than the desktop
    {
      OutHeight = g_graphicsContext.GetHeight();
      OutWidth = MathUtils::round_int((double)width * g_graphicsContext.GetHeight() / height);
    }
  }
  else
  { //let opengl scale
    OutWidth = width;
    OutHeight = height;
  }

  CLog::Log(LOGNOTICE,"Creating %ix%i pixmap", OutWidth, OutHeight);

    // Get our window attribs.
  XWindowAttributes wndattribs;
  XGetWindowAttributes(m_Display, DefaultRootWindow(m_Display), &wndattribs); // returns a status but I don't know what success is

  m_Pixmap = XCreatePixmap(m_Display,
                           DefaultRootWindow(m_Display),
                           OutWidth,
                           OutHeight,
                           wndattribs.depth);
  if (!m_Pixmap)
  {
    CLog::Log(LOGERROR, "GLX Error: MakePixmap: Unable to create XPixmap");
    return false;
  }

  XGCValues values = {};
  GC xgc;
  values.foreground = BlackPixel (m_Display, DefaultScreen (m_Display));
  xgc = XCreateGC(m_Display, m_Pixmap, GCForeground, &values);
  XFillRectangle(m_Display, m_Pixmap, xgc, 0, 0, OutWidth, OutHeight);
  XFreeGC(m_Display, xgc);

  if(!MakePixmapGL())
    return false;

  return true;
}

void CVDPAU::BindPixmap()
{
  CSharedLock lock(m_DecoderSection);

  { CSharedLock dLock(m_DisplaySection);
    if (m_DisplayState != VDPAU_OPEN)
      return;
  }

  if (m_glPixmap)
  {
    if(presentSurface != VDP_INVALID_HANDLE)
    {
      VdpPresentationQueueStatus status;
      VdpTime time;
      VdpStatus vdp_st;
      vdp_st = vdp_presentation_queue_query_surface_status(
                    vdp_flip_queue, presentSurface, &status, &time);
      CheckStatus(vdp_st, __LINE__);
      while(status != VDP_PRESENTATION_QUEUE_STATUS_VISIBLE && vdp_st == VDP_STATUS_OK)
      {
        Sleep(1);
        vdp_st = vdp_presentation_queue_query_surface_status(
                      vdp_flip_queue, presentSurface, &status, &time);
        CheckStatus(vdp_st, __LINE__);
      }
    }
    
    glXBindTexImageEXT(m_Display, m_glPixmap, GLX_FRONT_LEFT_EXT, NULL);
  }
  else CLog::Log(LOGERROR,"(VDPAU) BindPixmap called without valid pixmap");
}

void CVDPAU::ReleasePixmap()
{
  CSharedLock lock(m_DecoderSection);

  { CSharedLock dLock(m_DisplaySection);
    if (m_DisplayState != VDPAU_OPEN)
      return;
  }

  if (m_glPixmap)
  {
    glXReleaseTexImageEXT(m_Display, m_glPixmap, GLX_FRONT_LEFT_EXT);
  }
  else CLog::Log(LOGERROR,"(VDPAU) ReleasePixmap called without valid pixmap");
}

void CVDPAU::OnLostDevice()
{
  CLog::Log(LOGNOTICE,"CVDPAU::OnLostDevice event");

  CExclusiveLock lock(m_DecoderSection);
  FiniVDPAUOutput();
  FiniVDPAUProcs();

  m_DisplayState = VDPAU_LOST;
  m_DisplayEvent.Reset();
}

void CVDPAU::OnResetDevice()
{
  CLog::Log(LOGNOTICE,"CVDPAU::OnResetDevice event");

  CExclusiveLock lock(m_DisplaySection);
  if (m_DisplayState == VDPAU_LOST)
  {
    m_DisplayState = VDPAU_RESET;
    m_DisplayEvent.Set();
  }
}

int CVDPAU::Check(AVCodecContext* avctx)
{
  EDisplayState state;

  { CSharedLock lock(m_DisplaySection);
    state = m_DisplayState;
  }

  if (state == VDPAU_LOST)
  {
    CLog::Log(LOGNOTICE,"CVDPAU::Check waiting for display reset event");
    if (!m_DisplayEvent.WaitMSec(2000))
    {
      CLog::Log(LOGERROR, "CVDPAU::Check - device didn't reset in reasonable time");
      return VC_ERROR;
    }
    { CSharedLock lock(m_DisplaySection);
      state = m_DisplayState;
    }
  }
  if (state == VDPAU_RESET || state == VDPAU_ERROR)
  {
    CLog::Log(LOGNOTICE,"Attempting recovery");

    CSingleLock gLock(g_graphicsContext);
    CExclusiveLock lock(m_DecoderSection);

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

bool CVDPAU::IsVDPAUFormat(PixelFormat format)
{
  if ((format >= PIX_FMT_VDPAU_H264) && (format <= PIX_FMT_VDPAU_VC1)) return true;
#if (defined PIX_FMT_VDPAU_MPEG4_IN_AVUTIL)
  if (format == PIX_FMT_VDPAU_MPEG4) return true;
#endif
  else return false;
}

void CVDPAU::CheckFeatures()
{
  if (videoMixer == VDP_INVALID_HANDLE)
  {
    CLog::Log(LOGNOTICE, " (VDPAU) Creating the video mixer");
    // Creation of VideoMixer.
    VdpVideoMixerParameter parameters[] = {
      VDP_VIDEO_MIXER_PARAMETER_VIDEO_SURFACE_WIDTH,
      VDP_VIDEO_MIXER_PARAMETER_VIDEO_SURFACE_HEIGHT,
      VDP_VIDEO_MIXER_PARAMETER_CHROMA_TYPE
    };

    void const * parameter_values[] = {
      &surface_width,
      &surface_height,
      &vdp_chroma_type
    };

    tmpBrightness = 0;
    tmpContrast = 0;
    tmpNoiseReduction = 0;
    tmpSharpness = 0;

    VdpStatus vdp_st = VDP_STATUS_ERROR;
    vdp_st = vdp_video_mixer_create(vdp_device,
                                    m_feature_count,
                                    m_features,
                                    ARSIZE(parameters),
                                    parameters,
                                    parameter_values,
                                    &videoMixer);
    CheckStatus(vdp_st, __LINE__);

    SetHWUpscaling();
  }

  if (tmpBrightness != g_settings.m_currentVideoSettings.m_Brightness ||
      tmpContrast   != g_settings.m_currentVideoSettings.m_Contrast)
  {
    SetColor();
    tmpBrightness = g_settings.m_currentVideoSettings.m_Brightness;
    tmpContrast = g_settings.m_currentVideoSettings.m_Contrast;
  }
  if (tmpNoiseReduction != g_settings.m_currentVideoSettings.m_NoiseReduction)
  {
    tmpNoiseReduction = g_settings.m_currentVideoSettings.m_NoiseReduction;
    SetNoiseReduction();
  }
  if (tmpSharpness != g_settings.m_currentVideoSettings.m_Sharpness)
  {
    tmpSharpness = g_settings.m_currentVideoSettings.m_Sharpness;
    SetSharpness();
  }
  if (  tmpDeintMode != g_settings.m_currentVideoSettings.m_DeinterlaceMode ||
        tmpDeintGUI  != g_settings.m_currentVideoSettings.m_InterlaceMethod ||
       (tmpDeintGUI == VS_INTERLACEMETHOD_AUTO && tmpDeint != AutoInterlaceMethod()))
  {
    tmpDeintMode = g_settings.m_currentVideoSettings.m_DeinterlaceMode;
    tmpDeintGUI  = g_settings.m_currentVideoSettings.m_InterlaceMethod;
    if (tmpDeintGUI == VS_INTERLACEMETHOD_AUTO)
      tmpDeint = AutoInterlaceMethod();
    else
      tmpDeint = tmpDeintGUI;

    SetDeinterlacing();
  }
}

bool CVDPAU::Supports(VdpVideoMixerFeature feature)
{
  for(int i = 0; i < m_feature_count; i++)
  {
    if(m_features[i] == feature)
      return true;
  }
  return false;
}

bool CVDPAU::Supports(EINTERLACEMETHOD method)
{
  if(method == VS_INTERLACEMETHOD_VDPAU_BOB
  || method == VS_INTERLACEMETHOD_AUTO
  || method == VS_INTERLACEMETHOD_AUTO_ION)
    return true;

  for(SInterlaceMapping* p = g_interlace_mapping; p->method != VS_INTERLACEMETHOD_NONE; p++)
  {
    if(p->method == method)
      return Supports(p->feature);
  }
  return false;
}

EINTERLACEMETHOD CVDPAU::AutoInterlaceMethod()
{
  return VS_INTERLACEMETHOD_VDPAU_TEMPORAL;
}

void CVDPAU::SetColor()
{
  VdpStatus vdp_st;

  if (tmpBrightness != g_settings.m_currentVideoSettings.m_Brightness)
    m_Procamp.brightness = (float)((g_settings.m_currentVideoSettings.m_Brightness)-50) / 100;
  if (tmpContrast != g_settings.m_currentVideoSettings.m_Contrast)
    m_Procamp.contrast = (float)((g_settings.m_currentVideoSettings.m_Contrast)+50) / 100;

  if(vid_height >= 600 || vid_width > 1024)
    vdp_st = vdp_generate_csc_matrix(&m_Procamp, VDP_COLOR_STANDARD_ITUR_BT_709, &m_CSCMatrix);
  else
    vdp_st = vdp_generate_csc_matrix(&m_Procamp, VDP_COLOR_STANDARD_ITUR_BT_601, &m_CSCMatrix);

  VdpVideoMixerAttribute attributes[] = { VDP_VIDEO_MIXER_ATTRIBUTE_CSC_MATRIX };
  if (g_guiSettings.GetBool("videoplayer.vdpaustudiolevel"))
  {
    void const * pm_CSCMatix[] = { &studioCSC };
    vdp_st = vdp_video_mixer_set_attribute_values(videoMixer, ARSIZE(attributes), attributes, pm_CSCMatix);
  }
  else
  {
    void const * pm_CSCMatix[] = { &m_CSCMatrix };
    vdp_st = vdp_video_mixer_set_attribute_values(videoMixer, ARSIZE(attributes), attributes, pm_CSCMatix);
  }
  CheckStatus(vdp_st, __LINE__);
}

void CVDPAU::SetNoiseReduction()
{
  if(!Supports(VDP_VIDEO_MIXER_FEATURE_NOISE_REDUCTION))
    return;

  VdpVideoMixerFeature feature[] = { VDP_VIDEO_MIXER_FEATURE_NOISE_REDUCTION };
  VdpVideoMixerAttribute attributes[] = { VDP_VIDEO_MIXER_ATTRIBUTE_NOISE_REDUCTION_LEVEL };
  VdpStatus vdp_st;

  if (!g_settings.m_currentVideoSettings.m_NoiseReduction)
  {
    VdpBool enabled[]= {0};
    vdp_st = vdp_video_mixer_set_feature_enables(videoMixer, ARSIZE(feature), feature, enabled);
    CheckStatus(vdp_st, __LINE__);
    return;
  }
  VdpBool enabled[]={1};
  vdp_st = vdp_video_mixer_set_feature_enables(videoMixer, ARSIZE(feature), feature, enabled);
  CheckStatus(vdp_st, __LINE__);
  void* nr[] = { &g_settings.m_currentVideoSettings.m_NoiseReduction };
  CLog::Log(LOGNOTICE,"Setting Noise Reduction to %f",g_settings.m_currentVideoSettings.m_NoiseReduction);
  vdp_st = vdp_video_mixer_set_attribute_values(videoMixer, ARSIZE(attributes), attributes, nr);
  CheckStatus(vdp_st, __LINE__);
}

void CVDPAU::SetSharpness()
{
  if(!Supports(VDP_VIDEO_MIXER_FEATURE_SHARPNESS))
    return;
  
  VdpVideoMixerFeature feature[] = { VDP_VIDEO_MIXER_FEATURE_SHARPNESS };
  VdpVideoMixerAttribute attributes[] = { VDP_VIDEO_MIXER_ATTRIBUTE_SHARPNESS_LEVEL };
  VdpStatus vdp_st;

  if (!g_settings.m_currentVideoSettings.m_Sharpness)
  {
    VdpBool enabled[]={0};
    vdp_st = vdp_video_mixer_set_feature_enables(videoMixer, ARSIZE(feature), feature, enabled);
    CheckStatus(vdp_st, __LINE__);
    return;
  }
  VdpBool enabled[]={1};
  vdp_st = vdp_video_mixer_set_feature_enables(videoMixer, ARSIZE(feature), feature, enabled);
  CheckStatus(vdp_st, __LINE__);
  void* sh[] = { &g_settings.m_currentVideoSettings.m_Sharpness };
  CLog::Log(LOGNOTICE,"Setting Sharpness to %f",g_settings.m_currentVideoSettings.m_Sharpness);
  vdp_st = vdp_video_mixer_set_attribute_values(videoMixer, ARSIZE(attributes), attributes, sh);
  CheckStatus(vdp_st, __LINE__);
}

void CVDPAU::SetHWUpscaling()
{
#ifdef VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L1
  if(!Supports(VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L1) || !upScale)
    return;

  VdpVideoMixerFeature feature[] = { VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L1 };
  VdpStatus vdp_st;
  VdpBool enabled[]={1};
  vdp_st = vdp_video_mixer_set_feature_enables(videoMixer, ARSIZE(feature), feature, enabled);
  CheckStatus(vdp_st, __LINE__);
#endif
}

void CVDPAU::SetDeinterlacing()
{
  VdpStatus vdp_st;
  EDEINTERLACEMODE   mode = g_settings.m_currentVideoSettings.m_DeinterlaceMode;
  EINTERLACEMETHOD method = g_settings.m_currentVideoSettings.m_InterlaceMethod;
  if (method == VS_INTERLACEMETHOD_AUTO)
    method = AutoInterlaceMethod();

  VdpVideoMixerFeature feature[] = { VDP_VIDEO_MIXER_FEATURE_DEINTERLACE_TEMPORAL,
                                     VDP_VIDEO_MIXER_FEATURE_DEINTERLACE_TEMPORAL_SPATIAL,
                                     VDP_VIDEO_MIXER_FEATURE_INVERSE_TELECINE };
  if (mode == VS_DEINTERLACEMODE_OFF)
  {
    VdpBool enabled[]={0,0,0};
    vdp_st = vdp_video_mixer_set_feature_enables(videoMixer, ARSIZE(feature), feature, enabled);
  }
  else
  {
    if (method == VS_INTERLACEMETHOD_AUTO_ION)
    {
      if (vid_height <= 576)
      {
        VdpBool enabled[]={1,1,0};
        vdp_st = vdp_video_mixer_set_feature_enables(videoMixer, ARSIZE(feature), feature, enabled);
      }
      else if (vid_height > 576)
      {
        VdpBool enabled[]={1,0,0};
        vdp_st = vdp_video_mixer_set_feature_enables(videoMixer, ARSIZE(feature), feature, enabled);
      }
    }
    else if (method == VS_INTERLACEMETHOD_VDPAU_TEMPORAL
         ||  method == VS_INTERLACEMETHOD_VDPAU_TEMPORAL_HALF)
    {
      VdpBool enabled[]={1,0,0};
      vdp_st = vdp_video_mixer_set_feature_enables(videoMixer, ARSIZE(feature), feature, enabled);
    }
    else if (method == VS_INTERLACEMETHOD_VDPAU_TEMPORAL_SPATIAL
         ||  method == VS_INTERLACEMETHOD_VDPAU_TEMPORAL_SPATIAL_HALF)
    {
      VdpBool enabled[]={1,1,0};
      vdp_st = vdp_video_mixer_set_feature_enables(videoMixer, ARSIZE(feature), feature, enabled);
    }
    else if (method == VS_INTERLACEMETHOD_VDPAU_INVERSE_TELECINE)
    {
      VdpBool enabled[]={1,0,1};
      vdp_st = vdp_video_mixer_set_feature_enables(videoMixer, ARSIZE(feature), feature, enabled);
    }
    else
    {
      VdpBool enabled[]={0,0,0};
      vdp_st = vdp_video_mixer_set_feature_enables(videoMixer, ARSIZE(feature), feature, enabled);
    }
  }

  CheckStatus(vdp_st, __LINE__);
}

void CVDPAU::InitVDPAUProcs()
{
  char* error;

  (void)dlerror();
  dl_vdp_device_create_x11 = (VdpStatus (*)(Display*, int, VdpDevice*, VdpStatus (**)(VdpDevice, VdpFuncId, void**)))dlsym(dl_handle, (const char*)"vdp_device_create_x11");
  error = dlerror();
  if (error)
  {
    CLog::Log(LOGERROR,"(VDPAU) - %s in %s",error,__FUNCTION__);
    vdp_device = VDP_INVALID_HANDLE;

    //g_application.m_guiDialogKaiToast.QueueNotification(CGUIDialogKaiToast::Error, "VDPAU", error, 10000);

    return;
  }

  if (dl_vdp_device_create_x11)
  {
    CSingleLock lock(g_graphicsContext);
    m_Display = g_Windowing.GetDisplay();
  }

  int mScreen = DefaultScreen(m_Display);
  VdpStatus vdp_st;

  // Create Device
  // tested on 64bit Ubuntu 11.10 and it deadlocked without this
  XLockDisplay(m_Display);
  vdp_st = dl_vdp_device_create_x11(m_Display, //x_display,
                                 mScreen, //x_screen,
                                 &vdp_device,
                                 &vdp_get_proc_address);
  XUnlockDisplay(m_Display);

  CLog::Log(LOGNOTICE,"vdp_device = 0x%08x vdp_st = 0x%08x",vdp_device,vdp_st);
  if (vdp_st != VDP_STATUS_OK)
  {
    CLog::Log(LOGERROR,"(VDPAU) unable to init VDPAU - vdp_st = 0x%x.  Falling back.",vdp_st);
    vdp_device = VDP_INVALID_HANDLE;
    return;
  }

  if (vdp_st != VDP_STATUS_OK)
  {
    CLog::Log(LOGERROR,"(VDPAU) - Unable to create X11 device in %s",__FUNCTION__);
    vdp_device = VDP_INVALID_HANDLE;
    return;
  }
#define VDP_PROC(id, proc) \
  do { \
    vdp_st = vdp_get_proc_address(vdp_device, id, (void**)&proc); \
    CheckStatus(vdp_st, __LINE__); \
  } while(0);

  VDP_PROC(VDP_FUNC_ID_GET_ERROR_STRING                    , vdp_get_error_string);
  VDP_PROC(VDP_FUNC_ID_DEVICE_DESTROY                      , vdp_device_destroy);
  VDP_PROC(VDP_FUNC_ID_GENERATE_CSC_MATRIX                 , vdp_generate_csc_matrix);
  VDP_PROC(VDP_FUNC_ID_VIDEO_SURFACE_CREATE                , vdp_video_surface_create);
  VDP_PROC(VDP_FUNC_ID_VIDEO_SURFACE_DESTROY               , vdp_video_surface_destroy);
  VDP_PROC(VDP_FUNC_ID_VIDEO_SURFACE_PUT_BITS_Y_CB_CR      , vdp_video_surface_put_bits_y_cb_cr);
  VDP_PROC(VDP_FUNC_ID_VIDEO_SURFACE_GET_BITS_Y_CB_CR      , vdp_video_surface_get_bits_y_cb_cr);
  VDP_PROC(VDP_FUNC_ID_OUTPUT_SURFACE_PUT_BITS_Y_CB_CR     , vdp_output_surface_put_bits_y_cb_cr);
  VDP_PROC(VDP_FUNC_ID_OUTPUT_SURFACE_PUT_BITS_NATIVE      , vdp_output_surface_put_bits_native);
  VDP_PROC(VDP_FUNC_ID_OUTPUT_SURFACE_CREATE               , vdp_output_surface_create);
  VDP_PROC(VDP_FUNC_ID_OUTPUT_SURFACE_DESTROY              , vdp_output_surface_destroy);
  VDP_PROC(VDP_FUNC_ID_OUTPUT_SURFACE_GET_BITS_NATIVE      , vdp_output_surface_get_bits_native);
  VDP_PROC(VDP_FUNC_ID_OUTPUT_SURFACE_RENDER_OUTPUT_SURFACE, vdp_output_surface_render_output_surface);
  VDP_PROC(VDP_FUNC_ID_OUTPUT_SURFACE_PUT_BITS_INDEXED     , vdp_output_surface_put_bits_indexed);  
  VDP_PROC(VDP_FUNC_ID_VIDEO_MIXER_CREATE                  , vdp_video_mixer_create);
  VDP_PROC(VDP_FUNC_ID_VIDEO_MIXER_SET_FEATURE_ENABLES     , vdp_video_mixer_set_feature_enables);
  VDP_PROC(VDP_FUNC_ID_VIDEO_MIXER_DESTROY                 , vdp_video_mixer_destroy);
  VDP_PROC(VDP_FUNC_ID_VIDEO_MIXER_RENDER                  , vdp_video_mixer_render);
  VDP_PROC(VDP_FUNC_ID_VIDEO_MIXER_SET_ATTRIBUTE_VALUES    , vdp_video_mixer_set_attribute_values);
  VDP_PROC(VDP_FUNC_ID_VIDEO_MIXER_QUERY_PARAMETER_SUPPORT , vdp_video_mixer_query_parameter_support);
  VDP_PROC(VDP_FUNC_ID_VIDEO_MIXER_QUERY_FEATURE_SUPPORT   , vdp_video_mixer_query_feature_support);
  VDP_PROC(VDP_FUNC_ID_DECODER_CREATE                      , vdp_decoder_create);
  VDP_PROC(VDP_FUNC_ID_DECODER_DESTROY                     , vdp_decoder_destroy);
  VDP_PROC(VDP_FUNC_ID_DECODER_RENDER                      , vdp_decoder_render);
  VDP_PROC(VDP_FUNC_ID_DECODER_QUERY_CAPABILITIES          , vdp_decoder_query_caps);
  VDP_PROC(VDP_FUNC_ID_PREEMPTION_CALLBACK_REGISTER        , vdp_preemption_callback_register);
  VDP_PROC(VDP_FUNC_ID_PRESENTATION_QUEUE_TARGET_DESTROY          , vdp_presentation_queue_target_destroy);
  VDP_PROC(VDP_FUNC_ID_PRESENTATION_QUEUE_CREATE                  , vdp_presentation_queue_create);
  VDP_PROC(VDP_FUNC_ID_PRESENTATION_QUEUE_DESTROY                 , vdp_presentation_queue_destroy);
  VDP_PROC(VDP_FUNC_ID_PRESENTATION_QUEUE_DISPLAY                 , vdp_presentation_queue_display);
  VDP_PROC(VDP_FUNC_ID_PRESENTATION_QUEUE_BLOCK_UNTIL_SURFACE_IDLE, vdp_presentation_queue_block_until_surface_idle);
  VDP_PROC(VDP_FUNC_ID_PRESENTATION_QUEUE_TARGET_CREATE_X11       , vdp_presentation_queue_target_create_x11);
  VDP_PROC(VDP_FUNC_ID_PRESENTATION_QUEUE_QUERY_SURFACE_STATUS    , vdp_presentation_queue_query_surface_status);
  VDP_PROC(VDP_FUNC_ID_PRESENTATION_QUEUE_GET_TIME                , vdp_presentation_queue_get_time);
  
#undef VDP_PROC

  // set all vdpau resources to invalid
  vdp_flip_target = VDP_INVALID_HANDLE;
  vdp_flip_queue = VDP_INVALID_HANDLE;
  videoMixer = VDP_INVALID_HANDLE;
  totalAvailableOutputSurfaces = 0;
  presentSurface = VDP_INVALID_HANDLE;
  outputSurface = VDP_INVALID_HANDLE;
  for (int i = 0; i < NUM_OUTPUT_SURFACES; i++)
    outputSurfaces[i] = VDP_INVALID_HANDLE;

  m_vdpauOutputMethod = OUTPUT_NONE;

  CExclusiveLock lock(m_DisplaySection);
  m_DisplayState = VDPAU_OPEN;
  vdpauConfigured = false;
}

void CVDPAU::FiniVDPAUProcs()
{
  if (vdp_device == VDP_INVALID_HANDLE) return;

  VdpStatus vdp_st;
  vdp_st = vdp_device_destroy(vdp_device);
  CheckStatus(vdp_st, __LINE__);
  vdp_device = VDP_INVALID_HANDLE;
  vdpauConfigured = false;
}

void CVDPAU::InitCSCMatrix(int Height)
{
  VdpStatus vdp_st;
  m_Procamp.struct_version = VDP_PROCAMP_VERSION;
  m_Procamp.brightness     = 0.0;
  m_Procamp.contrast       = 1.0;
  m_Procamp.saturation     = 1.0;
  m_Procamp.hue            = 0;
  vdp_st = vdp_generate_csc_matrix(&m_Procamp,
                                   (Height < 720)? VDP_COLOR_STANDARD_ITUR_BT_601 : VDP_COLOR_STANDARD_ITUR_BT_709,
                                   &m_CSCMatrix);
  CheckStatus(vdp_st, __LINE__);
}

void CVDPAU::FiniVDPAUOutput()
{
  FiniOutputMethod();

  if (vdp_device == VDP_INVALID_HANDLE || !vdpauConfigured) return;

  CLog::Log(LOGNOTICE, " (VDPAU) %s", __FUNCTION__);

  VdpStatus vdp_st;

  vdp_st = vdp_decoder_destroy(decoder);
  if (CheckStatus(vdp_st, __LINE__))
    return;
  decoder = VDP_INVALID_HANDLE;

  for (unsigned int i = 0; i < m_videoSurfaces.size(); ++i)
  {
    vdpau_render_state *render = m_videoSurfaces[i];
    if (render->surface != VDP_INVALID_HANDLE)
    {
      vdp_st = vdp_video_surface_destroy(render->surface);
      render->surface = VDP_INVALID_HANDLE;
    }
    if (CheckStatus(vdp_st, __LINE__))
      return;
  }
}


void CVDPAU::ReadFormatOf( PixelFormat fmt
                         , VdpDecoderProfile &vdp_decoder_profile
                         , VdpChromaType     &vdp_chroma_type)
{
  switch (fmt)
  {
    case PIX_FMT_VDPAU_MPEG1:
      vdp_decoder_profile = VDP_DECODER_PROFILE_MPEG1;
      vdp_chroma_type     = VDP_CHROMA_TYPE_420;
      break;
    case PIX_FMT_VDPAU_MPEG2:
      vdp_decoder_profile = VDP_DECODER_PROFILE_MPEG2_MAIN;
      vdp_chroma_type     = VDP_CHROMA_TYPE_420;
      break;
    case PIX_FMT_VDPAU_H264:
      vdp_decoder_profile = VDP_DECODER_PROFILE_H264_HIGH;
      vdp_chroma_type     = VDP_CHROMA_TYPE_420;
      break;
    case PIX_FMT_VDPAU_WMV3:
      vdp_decoder_profile = VDP_DECODER_PROFILE_VC1_MAIN;
      vdp_chroma_type     = VDP_CHROMA_TYPE_420;
      break;
    case PIX_FMT_VDPAU_VC1:
      vdp_decoder_profile = VDP_DECODER_PROFILE_VC1_ADVANCED;
      vdp_chroma_type     = VDP_CHROMA_TYPE_420;
      break;
#if (defined PIX_FMT_VDPAU_MPEG4_IN_AVUTIL) && \
    (defined VDP_DECODER_PROFILE_MPEG4_PART2_ASP)
    case PIX_FMT_VDPAU_MPEG4:
      vdp_decoder_profile = VDP_DECODER_PROFILE_MPEG4_PART2_ASP;
      vdp_chroma_type     = VDP_CHROMA_TYPE_420;
#endif
    default:
      vdp_decoder_profile = 0;
      vdp_chroma_type     = 0;
  }
}


bool CVDPAU::ConfigVDPAU(AVCodecContext* avctx, int ref_frames)
{
  FiniVDPAUOutput();

  VdpStatus vdp_st;
  VdpDecoderProfile vdp_decoder_profile;
  vid_width = avctx->width;
  vid_height = avctx->height;
  surface_width = avctx->coded_width;
  surface_height = avctx->coded_height;

  past[1] = past[0] = current = future = NULL;
  CLog::Log(LOGNOTICE, " (VDPAU) screenWidth:%i vidWidth:%i surfaceWidth:%i",OutWidth,vid_width,surface_width);
  CLog::Log(LOGNOTICE, " (VDPAU) screenHeight:%i vidHeight:%i surfaceHeight:%i",OutHeight,vid_height,surface_height);
  ReadFormatOf(avctx->pix_fmt, vdp_decoder_profile, vdp_chroma_type);

  if(avctx->pix_fmt == PIX_FMT_VDPAU_H264)
  {
     max_references = ref_frames;
     if (max_references > 16) max_references = 16;
     if (max_references < 5)  max_references = 5;
  }
  else
    max_references = 2;

  vdp_st = vdp_decoder_create(vdp_device,
                              vdp_decoder_profile,
                              surface_width,
                              surface_height,
                              max_references,
                              &decoder);
  if (CheckStatus(vdp_st, __LINE__))
    return false;

  m_vdpauOutputMethod = OUTPUT_NONE;

  vdpauConfigured = true;
  return true;
}

bool CVDPAU::ConfigOutputMethod(AVCodecContext *avctx, AVFrame *pFrame)
{
  VdpStatus vdp_st;

  if (m_vdpauOutputMethod == OUTPUT_PIXMAP)
    return true;

  FiniOutputMethod();

  MakePixmap(avctx->width,avctx->height);

  vdp_st = vdp_presentation_queue_target_create_x11(vdp_device,
                                                    m_Pixmap, //x_window,
                                                    &vdp_flip_target);
  if (CheckStatus(vdp_st, __LINE__))
    return false;

  vdp_st = vdp_presentation_queue_create(vdp_device,
                                         vdp_flip_target,
                                         &vdp_flip_queue);
  if (CheckStatus(vdp_st, __LINE__))
    return false;

  totalAvailableOutputSurfaces = 0;

  int tmpMaxOutputSurfaces = NUM_OUTPUT_SURFACES;
  if (vid_width == FULLHD_WIDTH)
    tmpMaxOutputSurfaces = NUM_OUTPUT_SURFACES_FOR_FULLHD;

  // Creation of outputSurfaces
  for (int i = 0; i < NUM_OUTPUT_SURFACES && i < tmpMaxOutputSurfaces; i++)
  {
    vdp_st = vdp_output_surface_create(vdp_device,
                                       VDP_RGBA_FORMAT_B8G8R8A8,
                                       OutWidth,
                                       OutHeight,
                                       &outputSurfaces[i]);
    if (CheckStatus(vdp_st, __LINE__))
      return false;
    totalAvailableOutputSurfaces++;
  }
  CLog::Log(LOGNOTICE, " (VDPAU) Total Output Surfaces Available: %i of a max (tmp: %i const: %i)",
                       totalAvailableOutputSurfaces,
                       tmpMaxOutputSurfaces,
                       NUM_OUTPUT_SURFACES);

  // create 3 pitches of black lines needed for clipping top
  // and bottom lines when de-interlacing
  m_BlackBar = new uint32_t[3*OutWidth];
  memset(m_BlackBar, 0, 3*OutWidth*sizeof(uint32_t));

  surfaceNum = presentSurfaceNum = 0;
  outputSurface = presentSurface = VDP_INVALID_HANDLE;
  videoMixer = VDP_INVALID_HANDLE;

  m_vdpauOutputMethod = OUTPUT_PIXMAP;

  return true;
}

bool CVDPAU::FiniOutputMethod()
{
  VdpStatus vdp_st;

  if (vdp_flip_queue != VDP_INVALID_HANDLE)
  {
    vdp_st = vdp_presentation_queue_destroy(vdp_flip_queue);
    vdp_flip_queue = VDP_INVALID_HANDLE;
    CheckStatus(vdp_st, __LINE__);
  }

  if (vdp_flip_target != VDP_INVALID_HANDLE)
  {
    vdp_st = vdp_presentation_queue_target_destroy(vdp_flip_target);
    vdp_flip_target = VDP_INVALID_HANDLE;
    CheckStatus(vdp_st, __LINE__);
  }

  if (m_glPixmap)
  {
    CLog::Log(LOGDEBUG, "GLX: Destroying glPixmap");
    glXDestroyPixmap(m_Display, m_glPixmap);
    m_glPixmap = None;
  }

  if (m_Pixmap)
  {
    CLog::Log(LOGDEBUG, "GLX: Destroying XPixmap");
    XFreePixmap(m_Display, m_Pixmap);
    m_Pixmap = None;
  }

  outputSurface = presentSurface = VDP_INVALID_HANDLE;

  for (int i = 0; i < totalAvailableOutputSurfaces; i++)
  {
    if (outputSurfaces[i] == VDP_INVALID_HANDLE)
       continue;
    vdp_st = vdp_output_surface_destroy(outputSurfaces[i]);
    outputSurfaces[i] = VDP_INVALID_HANDLE;
    CheckStatus(vdp_st, __LINE__);
  }

  if (videoMixer != VDP_INVALID_HANDLE)
  {
    vdp_st = vdp_video_mixer_destroy(videoMixer);
    videoMixer = VDP_INVALID_HANDLE;
    if (CheckStatus(vdp_st, __LINE__));
  }

  if (m_BlackBar)
  {
    delete [] m_BlackBar;
    m_BlackBar = NULL;
  }

  while (!m_DVDVideoPics.empty())
    m_DVDVideoPics.pop();

  return true;
}

void CVDPAU::SpewHardwareAvailable()  //Copyright (c) 2008 Wladimir J. van der Laan  -- VDPInfo
{
  VdpStatus rv;
  CLog::Log(LOGNOTICE,"VDPAU Decoder capabilities:");
  CLog::Log(LOGNOTICE,"name          level macbs width height");
  CLog::Log(LOGNOTICE,"------------------------------------");
  for(unsigned int x=0; x<decoder_profile_count; ++x)
  {
    VdpBool is_supported = false;
    uint32_t max_level, max_macroblocks, max_width, max_height;
    rv = vdp_decoder_query_caps(vdp_device, decoder_profiles[x].id,
                                &is_supported, &max_level, &max_macroblocks, &max_width, &max_height);
    if(rv == VDP_STATUS_OK && is_supported)
    {
      CLog::Log(LOGNOTICE,"%-16s %2i %5i %5i %5i\n", decoder_profiles[x].name,
                max_level, max_macroblocks, max_width, max_height);
    }
  }
  CLog::Log(LOGNOTICE,"------------------------------------");
  m_feature_count = 0;
#define CHECK_SUPPORT(feature)  \
  do { \
    VdpBool supported; \
    if(vdp_video_mixer_query_feature_support(vdp_device, feature, &supported) == VDP_STATUS_OK && supported) { \
      CLog::Log(LOGNOTICE, "Mixer feature: "#feature);  \
      m_features[m_feature_count++] = feature; \
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

bool CVDPAU::IsSurfaceValid(vdpau_render_state *render)
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

int CVDPAU::FFGetBuffer(AVCodecContext *avctx, AVFrame *pic)
{
  //CLog::Log(LOGNOTICE,"%s",__FUNCTION__);
  CDVDVideoCodecFFmpeg* ctx        = (CDVDVideoCodecFFmpeg*)avctx->opaque;
  CVDPAU*               vdp        = (CVDPAU*)ctx->GetHardware();
  struct pictureAge*    pA         = &vdp->picAge;

  // while we are waiting to recover we can't do anything
  CSharedLock lock(vdp->m_DecoderSection);

  { CSharedLock dLock(vdp->m_DisplaySection);
    if(vdp->m_DisplayState != VDPAU_OPEN)
    {
      CLog::Log(LOGWARNING, "CVDPAU::FFGetBuffer - returning due to awaiting recovery");
      return -1;
    }
  }

  vdpau_render_state * render = NULL;

  // find unused surface
  for(unsigned int i = 0; i < vdp->m_videoSurfaces.size(); i++)
  {
    if(!(vdp->m_videoSurfaces[i]->state & (FF_VDPAU_STATE_USED_FOR_REFERENCE | FF_VDPAU_STATE_USED_FOR_RENDER)))
    {
      render = vdp->m_videoSurfaces[i];
      render->state = 0;
      break;
    }
  }

  VdpStatus vdp_st = VDP_STATUS_ERROR;
  if (render == NULL)
  {
    // create a new surface
    VdpDecoderProfile profile;
    ReadFormatOf(avctx->pix_fmt, profile, vdp->vdp_chroma_type);
    render = (vdpau_render_state*)calloc(sizeof(vdpau_render_state), 1);
    if (render == NULL)
    {
      CLog::Log(LOGWARNING, "CVDPAU::FFGetBuffer - calloc failed");
      return -1;
    }
    render->surface = VDP_INVALID_HANDLE;
    vdp->m_videoSurfaces.push_back(render);
  }

  if (render->surface == VDP_INVALID_HANDLE)
  {
    vdp_st = vdp->vdp_video_surface_create(vdp->vdp_device,
                                         vdp->vdp_chroma_type,
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

  if (render == NULL)
    return -1;

  pic->data[1] =  pic->data[2] = NULL;
  pic->data[0]= (uint8_t*)render;

  pic->linesize[0] = pic->linesize[1] =  pic->linesize[2] = 0;

  if(pic->reference)
  {
    pic->age = pA->ip_age[0];
    pA->ip_age[0]= pA->ip_age[1]+1;
    pA->ip_age[1]= 1;
    pA->b_age++;
  }
  else
  {
    pic->age = pA->b_age;
    pA->ip_age[0]++;
    pA->ip_age[1]++;
    pA->b_age = 1;
  }
  pic->type= FF_BUFFER_TYPE_USER;

  render->state |= FF_VDPAU_STATE_USED_FOR_REFERENCE;
  pic->reordered_opaque= avctx->reordered_opaque;
  return 0;
}

void CVDPAU::FFReleaseBuffer(AVCodecContext *avctx, AVFrame *pic)
{
  //CLog::Log(LOGNOTICE,"%s",__FUNCTION__);
  CDVDVideoCodecFFmpeg* ctx        = (CDVDVideoCodecFFmpeg*)avctx->opaque;
  CVDPAU*               vdp        = (CVDPAU*)ctx->GetHardware();
  vdpau_render_state  * render;
  unsigned int i;

  CSharedLock lock(vdp->m_DecoderSection);

  render=(vdpau_render_state*)pic->data[0];
  if(!render)
  {
    CLog::Log(LOGERROR, "CVDPAU::FFReleaseBuffer - invalid context handle provided");
    return;
  }

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


void CVDPAU::FFDrawSlice(struct AVCodecContext *s,
                                           const AVFrame *src, int offset[4],
                                           int y, int type, int height)
{
  CDVDVideoCodecFFmpeg* ctx = (CDVDVideoCodecFFmpeg*)s->opaque;
  CVDPAU*               vdp = (CVDPAU*)ctx->GetHardware();

  // while we are waiting to recover we can't do anything
  CSharedLock lock(vdp->m_DecoderSection);

  { CSharedLock dLock(vdp->m_DisplaySection);
    if(vdp->m_DisplayState != VDPAU_OPEN)
      return;
  }


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
  if(s->pix_fmt == PIX_FMT_VDPAU_H264)
    max_refs = render->info.h264.num_ref_frames;

  if(vdp->decoder == VDP_INVALID_HANDLE
  || vdp->vdpauConfigured == false
  || vdp->max_references < max_refs)
  {
    if(!vdp->ConfigVDPAU(s, max_refs))
      return;
  }

  vdp_st = vdp->vdp_decoder_render(vdp->decoder,
                                   render->surface,
                                   (VdpPictureInfo const *)&(render->info),
                                   render->bitstream_buffers_used,
                                   render->bitstream_buffers);
  vdp->CheckStatus(vdp_st, __LINE__);
}

int CVDPAU::Decode(AVCodecContext *avctx, AVFrame *pFrame)
{
  //CLog::Log(LOGNOTICE,"%s",__FUNCTION__);
  VdpStatus vdp_st;
  VdpTime time;

  int result = Check(avctx);
  if (result)
    return result;

  CSharedLock lock(m_DecoderSection);

  if (!vdpauConfigured)
    return VC_ERROR;

  // configure vdpau output
  if (!ConfigOutputMethod(avctx, pFrame))
    return VC_FLUSHED;

  outputSurface = outputSurfaces[surfaceNum];

  CheckFeatures();

  if (( (int)outRectVid.x1 != OutWidth ) ||
      ( (int)outRectVid.y1 != OutHeight ))
  {
    outRectVid.x0 = 0;
    outRectVid.y0 = 0;
    outRectVid.x1 = OutWidth;
    outRectVid.y1 = OutHeight;
  }

  EDEINTERLACEMODE   mode = g_settings.m_currentVideoSettings.m_DeinterlaceMode;
  EINTERLACEMETHOD method = g_settings.m_currentVideoSettings.m_InterlaceMethod;
  if (method == VS_INTERLACEMETHOD_AUTO)
    method = AutoInterlaceMethod();

  if(pFrame)
  { // we have a new frame from decoder

    vdpau_render_state * render = (vdpau_render_state*)pFrame->data[2];
    if(!render) // old style ffmpeg gave data on plane 0
      render = (vdpau_render_state*)pFrame->data[0];
    if(!render)
      return VC_ERROR;

    // ffmpeg vc-1 decoder does not flush, make sure the data buffer is still valid
    if (!IsSurfaceValid(render))
    {
      CLog::Log(LOGWARNING, "CVDPAU::Decode - ignoring invalid buffer");
      return VC_BUFFER;
    }

    render->state |= FF_VDPAU_STATE_USED_FOR_RENDER;

    ClearUsedForRender(&past[0]);
    past[0] = past[1];
    past[1] = current;
    current = future;
    future = render;

    DVDVideoPicture DVDPic;
    memset(&DVDPic, 0, sizeof(DVDVideoPicture));
    ((CDVDVideoCodecFFmpeg*)avctx->opaque)->GetPictureCommon(&DVDPic);
    m_DVDVideoPics.push(DVDPic);

    int pics = m_DVDVideoPics.size();
    if (pics < 2)
        return VC_BUFFER;
    else if (pics > 2)
    {
      // this should not normally happen
      CLog::Log(LOGERROR, "CVDPAU::Decode - invalid number of pictures in queue");
      while (pics-- != 2)
        m_DVDVideoPics.pop();
    }

    if (mode == VS_DEINTERLACEMODE_FORCE
    || (mode == VS_DEINTERLACEMODE_AUTO && m_DVDVideoPics.front().iFlags & DVP_FLAG_INTERLACED))
    {
      if((method == VS_INTERLACEMETHOD_AUTO_ION
      ||  method == VS_INTERLACEMETHOD_VDPAU_BOB
      ||  method == VS_INTERLACEMETHOD_VDPAU_TEMPORAL
      ||  method == VS_INTERLACEMETHOD_VDPAU_TEMPORAL_HALF
      ||  method == VS_INTERLACEMETHOD_VDPAU_TEMPORAL_SPATIAL
      ||  method == VS_INTERLACEMETHOD_VDPAU_TEMPORAL_SPATIAL_HALF
      ||  method == VS_INTERLACEMETHOD_VDPAU_INVERSE_TELECINE ))
      {
        if((method == VS_INTERLACEMETHOD_AUTO_ION && vid_height > 576)
        || method == VS_INTERLACEMETHOD_VDPAU_TEMPORAL_HALF
        || method == VS_INTERLACEMETHOD_VDPAU_TEMPORAL_SPATIAL_HALF
        || avctx->skip_frame == AVDISCARD_NONREF)
          m_mixerstep = 0;
        else
          m_mixerstep = 1;

        if(m_DVDVideoPics.front().iFlags & DVP_FLAG_TOP_FIELD_FIRST)
          m_mixerfield = VDP_VIDEO_MIXER_PICTURE_STRUCTURE_TOP_FIELD;
        else
          m_mixerfield = VDP_VIDEO_MIXER_PICTURE_STRUCTURE_BOTTOM_FIELD;
      }
      else
      {
        m_mixerstep  = 0;
        m_mixerfield = VDP_VIDEO_MIXER_PICTURE_STRUCTURE_FRAME;
      }
    }
    else
    {
      m_mixerstep  = 0;
      m_mixerfield = VDP_VIDEO_MIXER_PICTURE_STRUCTURE_FRAME;
    }

  }
  else if(m_mixerstep == 1)
  { // no new frame given, output second field of old frame

    if(avctx->skip_frame == AVDISCARD_NONREF)
    {
      ClearUsedForRender(&past[1]);
      m_DVDVideoPics.pop();
      return VC_BUFFER;
    }

    m_mixerstep = 2;
    if(m_mixerfield == VDP_VIDEO_MIXER_PICTURE_STRUCTURE_TOP_FIELD)
      m_mixerfield = VDP_VIDEO_MIXER_PICTURE_STRUCTURE_BOTTOM_FIELD;
    else
      m_mixerfield = VDP_VIDEO_MIXER_PICTURE_STRUCTURE_TOP_FIELD;
  }
  else
  {
    CLog::Log(LOGERROR, "CVDPAU::Decode - invalid mixer state reached");
    return VC_BUFFER;
  }

  VdpVideoSurface past_surfaces[2] = { VDP_INVALID_HANDLE, VDP_INVALID_HANDLE };
  VdpVideoSurface futu_surfaces[1] = { VDP_INVALID_HANDLE };

  if(m_mixerfield == VDP_VIDEO_MIXER_PICTURE_STRUCTURE_FRAME)
  {
    if (past[0])
      past_surfaces[1] = past[0]->surface;
    if (past[1])
      past_surfaces[0] = past[1]->surface;
    futu_surfaces[0] = future->surface;
  }
  else
  {
    if(m_mixerstep == 1)
    { // first field
      if (past[1])
      {
        past_surfaces[1] = past[1]->surface;
        past_surfaces[0] = past[1]->surface;
      }
      futu_surfaces[0] = current->surface;
    }
    else
    { // second field
      if (past[1])
        past_surfaces[1] = past[1]->surface;
      past_surfaces[0] = current->surface;
      futu_surfaces[0] = future->surface;
    }
  }

  vdp_st = vdp_presentation_queue_block_until_surface_idle(vdp_flip_queue,outputSurface,&time);

  VdpRect sourceRect = {0,0,vid_width, vid_height};

  vdp_st = vdp_video_mixer_render(videoMixer,
                                  VDP_INVALID_HANDLE,
                                  0, 
                                  m_mixerfield,
                                  2,
                                  past_surfaces,
                                  current->surface,
                                  1,
                                  futu_surfaces,
                                  &sourceRect,
                                  outputSurface,
                                  &(outRectVid),
                                  &(outRectVid),
                                  0,
                                  NULL);
  CheckStatus(vdp_st, __LINE__);

  surfaceNum++;
  if (surfaceNum >= totalAvailableOutputSurfaces) surfaceNum = 0;

  if(m_mixerfield == VDP_VIDEO_MIXER_PICTURE_STRUCTURE_FRAME)
  {
    ClearUsedForRender(&past[0]);
    return VC_BUFFER | VC_PICTURE;
  }
  else
  {
    // in order to clip top and bottom lines when de-interlacing
    // we black those lines as a work around for not working
    // background colour using the mixer
    // pixel perfect is preferred over overscanning or zooming

    VdpRect clipRect = outRectVid;
    clipRect.y1 = clipRect.y0 + 2;
    uint32_t *data[] = {m_BlackBar};
    uint32_t pitches[] = {outRectVid.x1};
    vdp_st = vdp_output_surface_put_bits_native(outputSurface,
                                        (void**)data,
                                        pitches,
                                        &clipRect);
    CheckStatus(vdp_st, __LINE__);

    clipRect = outRectVid;
    clipRect.y0 = clipRect.y1 - 2;
    vdp_st = vdp_output_surface_put_bits_native(outputSurface,
                                        (void**)data,
                                        pitches,
                                        &clipRect);
    CheckStatus(vdp_st, __LINE__);

    if(m_mixerstep == 1)
      return VC_PICTURE;
    else
    {
      ClearUsedForRender(&past[1]);
      return VC_BUFFER | VC_PICTURE;
    }
  }
}

bool CVDPAU::GetPicture(AVCodecContext* avctx, AVFrame* frame, DVDVideoPicture* picture)
{
  CSharedLock lock(m_DecoderSection);

  { CSharedLock dLock(m_DisplaySection);
    if (m_DisplayState != VDPAU_OPEN)
      return false;
  }

  *picture = m_DVDVideoPics.front();
  // if this is the first field of an interlaced frame, we'll need
  // this same picture for the second field later
  if (m_mixerstep != 1)
    m_DVDVideoPics.pop();

  picture->format = DVDVideoPicture::FMT_VDPAU;
  picture->iFlags &= DVP_FLAG_DROPPED;
  picture->iWidth = OutWidth;
  picture->iHeight = OutHeight;
  picture->vdpau = this;

  if(m_mixerstep)
  {
    picture->iRepeatPicture = -0.5;
    if(m_mixerstep > 1)
    {
      picture->dts = DVD_NOPTS_VALUE;
      picture->pts = DVD_NOPTS_VALUE;
    }
  }
  return true;
}

void CVDPAU::Reset()
{
  // invalidate surfaces and picture queue when seeking
  ClearUsedForRender(&past[0]);
  ClearUsedForRender(&past[1]);
  ClearUsedForRender(&current);
  ClearUsedForRender(&future);

  while (!m_DVDVideoPics.empty())
    m_DVDVideoPics.pop();
}

void CVDPAU::Present()
{
  //CLog::Log(LOGNOTICE,"%s",__FUNCTION__);
  VdpStatus vdp_st;

  CSharedLock lock(m_DecoderSection);

  { CSharedLock dLock(m_DisplaySection);
    if (m_DisplayState != VDPAU_OPEN)
      return;
  }

  presentSurface = outputSurface;

  vdp_st = vdp_presentation_queue_display(vdp_flip_queue,
                                          presentSurface,
                                          0,
                                          0,
                                          0);
  CheckStatus(vdp_st, __LINE__);
}

bool CVDPAU::CheckStatus(VdpStatus vdp_st, int line)
{
  if (vdp_st != VDP_STATUS_OK)
  {
    CLog::Log(LOGERROR, " (VDPAU) Error: %s(%d) at %s:%d\n", vdp_get_error_string(vdp_st), vdp_st, __FILE__, line);

    CExclusiveLock lock(m_DisplaySection);

    if(m_DisplayState == VDPAU_OPEN)
    {
      if (vdp_st == VDP_STATUS_DISPLAY_PREEMPTED)
        m_DisplayState = VDPAU_LOST;
      else
        m_DisplayState = VDPAU_ERROR;
    }

    return true;
  }
  return false;
}

#endif
