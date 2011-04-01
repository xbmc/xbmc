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

// switch to turn on GL_NV_vdpau_interop
#define VDPAU_GL_INTEROP

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
#include "cores/VideoRenderers/RenderFlags.h"

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

#define CHECK_VDPAU_RETURN(vdp, value) \
        do { \
          if(CheckStatus(vdp, __LINE__)) \
            return value; \
        } while(0);

//since libvdpau 0.4, vdp_device_create_x11() installs a callback on the Display*,
//if we unload libvdpau with dlclose(), we segfault on XCloseDisplay,
//so we just keep a static handle to libvdpau around
void* CVDPAU::dl_handle;

CVDPAU::CVDPAU()
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

  vdp_device = VDP_INVALID_HANDLE;

  picAge.b_age    = picAge.ip_age[0] = picAge.ip_age[1] = 256*256*256*64;
  vdpauConfigured = false;
  recover = false;
  m_mixerfield = VDP_VIDEO_MIXER_PICTURE_STRUCTURE_FRAME;
  m_mixerstep  = 0;

  for (int i=0;i<3;i++)
  {
    m_glPixmap[i] = 0;
    m_Pixmap[i] = 0;
    m_glContext[i] = 0;
  }

  if (!glXBindTexImageEXT)
    glXBindTexImageEXT    = (PFNGLXBINDTEXIMAGEEXTPROC)glXGetProcAddress((GLubyte *) "glXBindTexImageEXT");
  if (!glXReleaseTexImageEXT)
    glXReleaseTexImageEXT = (PFNGLXRELEASETEXIMAGEEXTPROC)glXGetProcAddress((GLubyte *) "glXReleaseTexImageEXT");

  hasVdpauGlInterop = false;
  m_GlInteropStatus = OUTPUT_NONE;
  m_renderThread = NULL;
  m_presentPicture = m_flipBuffer[0] = m_flipBuffer[1] = m_flipBuffer[2] = NULL;
  m_flipBufferIdx = 0;

#ifdef VDPAU_GL_INTEROP
#ifdef GL_NV_vdpau_interop
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

  hasVdpauGlInterop = glewIsSupported("GL_NV_vdpau_interop");
  if (hasVdpauGlInterop)
  {
    CLog::Log(LOGNOTICE, "CVDPAU::CVDPAU GL interop supported and being used");
  }
#endif
#endif

  totalAvailableOutputSurfaces = 0;
  presentSurface = VDP_INVALID_HANDLE;
  vid_width = vid_height = OutWidth = OutHeight = 0;
  memset(&outRect, 0, sizeof(VdpRect));
  memset(&outRectVid, 0, sizeof(VdpRect));

  tmpBrightness  = 0;
  tmpContrast    = 0;
  max_references = 0;

  for (int i = 0; i < NUM_OUTPUT_SURFACES; i++)
    outputSurfaces[i] = VDP_INVALID_HANDLE;

  videoMixer = VDP_INVALID_HANDLE;
  for (int i = 0; i < 3; i++)
  {
    vdp_flip_target[i] = VDP_INVALID_HANDLE;
    vdp_flip_queue[i] = VDP_INVALID_HANDLE;
  }

  upScale = g_advancedSettings.m_videoVDPAUScaling;
}

bool CVDPAU::Open(AVCodecContext* avctx, const enum PixelFormat)
{
  if(avctx->width  == 0
  || avctx->height == 0)
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
      if (!CDVDCodecUtils::IsVP3CompatibleWidth(avctx->width))
        CLog::Log(LOGWARNING,"(VDPAU) width %i might not be supported because of hardware bug", avctx->width);
   
      /* attempt to create a decoder with this width/height, some sizes are not supported by hw */
      VdpStatus vdp_st;
      vdp_st = vdp_decoder_create(vdp_device, profile, avctx->width, avctx->height, 5, &decoder);

      if(vdp_st != VDP_STATUS_OK)
      {
        CLog::Log(LOGERROR, " (VDPAU) Error: %s(%d) checking for decoder support\n", vdp_get_error_string(vdp_st), vdp_st);
        FiniVDPAUProcs();
        return false;
      }

      vdp_decoder_destroy(decoder);
      CheckStatus(vdp_st, __LINE__);
    }

    InitCSCMatrix(avctx->height);
    SetWidthHeight(avctx->width,avctx->height);

    m_vdpauOutputMethod = OUTPUT_NONE;
    glInteropFinish = false;

    /* finally setup ffmpeg */
    avctx->get_buffer      = CVDPAU::FFGetBuffer;
    avctx->release_buffer  = CVDPAU::FFReleaseBuffer;
    avctx->draw_horiz_band = CVDPAU::FFDrawSlice;
    avctx->slice_flags=SLICE_FLAG_CODED_ORDER|SLICE_FLAG_ALLOW_FIELD;
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
}

bool CVDPAU::MakePixmapGL(int index)
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

  m_glPixmap[index] = glXCreatePixmap(m_Display, fbConfigs[fbConfigIndex], m_Pixmap[index], pixmapAttribs);

  if (!m_glPixmap[index])
  {
    CLog::Log(LOGINFO, "GLX Error: Could not create Pixmap");
    XFree(fbConfigs);
    return false;
  }

  /* to make the pixmap usable, it needs to have any context associated with it */
  GLXContext  lastctx = glXGetCurrentContext();
  GLXDrawable lastdrw = glXGetCurrentDrawable();

  XVisualInfo *visInfo;
  visInfo = glXGetVisualFromFBConfig(m_Display, fbConfigs[fbConfigIndex]);
  if (!visInfo)
  {
    CLog::Log(LOGINFO, "GLX Error: Could not obtain X Visual Info for pixmap");
    XFree(fbConfigs);
    return false;
  }
  XFree(fbConfigs);

  CLog::Log(LOGINFO, "GLX: Creating Pixmap context");
  m_glContext[index] = glXCreateContext(m_Display, visInfo, NULL, True);
  XFree(visInfo);

  if (!glXMakeCurrent(m_Display, m_glPixmap[index], m_glContext[index]))
  {
    CLog::Log(LOGINFO, "GLX Error: Could not make Pixmap current");
    return false;
  }

  /* restore what thread had before */
  glXMakeCurrent(m_Display, lastdrw, lastctx);

  return true;

}

void CVDPAU::SetWidthHeight(int width, int height)
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
}

bool CVDPAU::MakePixmap(int index, int width, int height)
{
  CLog::Log(LOGNOTICE,"Creating %ix%i pixmap", OutWidth, OutHeight);

    // Get our window attribs.
  XWindowAttributes wndattribs;
  XGetWindowAttributes(m_Display, DefaultRootWindow(m_Display), &wndattribs); // returns a status but I don't know what success is

  m_Pixmap[index] = XCreatePixmap(m_Display,
                           DefaultRootWindow(m_Display),
                           OutWidth,
                           OutHeight,
                           wndattribs.depth);
  if (!m_Pixmap[index])
  {
    CLog::Log(LOGERROR, "GLX Error: MakePixmap: Unable to create XPixmap");
    return false;
  }

  XGCValues values = {};
  GC xgc;
  values.foreground = BlackPixel (m_Display, DefaultScreen (m_Display));
  xgc = XCreateGC(m_Display, m_Pixmap[index], GCForeground, &values);
  XFillRectangle(m_Display, m_Pixmap[index], xgc, 0, 0, OutWidth, OutHeight);
  XFreeGC(m_Display, xgc);

  if(!MakePixmapGL(index))
    return false;

  return true;
}

bool CVDPAU::SetTexture(int plane, int field)
{
  m_glTexture = 0;

  if(hasVdpauGlInterop)
  {
    m_glTexture = GLGetSurfaceTexture(plane, field);
  }
  if (m_glTexture)
    return true;
  else
    return false;
}

GLuint CVDPAU::GetTexture()
{
  return m_glTexture;
}

void CVDPAU::BindPixmap()
{
  if (hasVdpauGlInterop)
    return;

  if (m_glPixmap[m_flipBufferIdx])
  {
    if(m_flipBuffer[m_flipBufferIdx]->outputSurface != VDP_INVALID_HANDLE)
    {
      VdpPresentationQueueStatus status;
      VdpTime time;
      VdpStatus vdp_st;
      VdpOutputSurface surface = m_flipBuffer[m_flipBufferIdx]->outputSurface;

      vdp_st = vdp_presentation_queue_query_surface_status(
                    vdp_flip_queue[m_flipBufferIdx], surface, &status, &time);
      CheckStatus(vdp_st, __LINE__);
      while(status != VDP_PRESENTATION_QUEUE_STATUS_VISIBLE && vdp_st == VDP_STATUS_OK)
      {
        Sleep(1);
        vdp_st = vdp_presentation_queue_query_surface_status(
                      vdp_flip_queue[m_flipBufferIdx], surface, &status, &time);
        CheckStatus(vdp_st, __LINE__);
      }
    }
    
    glXBindTexImageEXT(m_Display, m_glPixmap[m_flipBufferIdx], GLX_FRONT_LEFT_EXT, NULL);
  }
  else CLog::Log(LOGERROR,"(VDPAU) BindPixmap called without valid pixmap");
}

void CVDPAU::ReleasePixmap()
{
  if (hasVdpauGlInterop)
    return;

  if (m_glPixmap[m_flipBufferIdx])
  {
    glXReleaseTexImageEXT(m_Display, m_glPixmap[m_flipBufferIdx], GLX_FRONT_LEFT_EXT);
  }
  else CLog::Log(LOGERROR,"(VDPAU) ReleasePixmap called without valid pixmap");
}

bool CVDPAU::CheckRecover(bool force)
{
  if (recover || force)
  {
    glInteropFinish = true;

    CLog::Log(LOGNOTICE,"Attempting recovery");

    FiniVDPAUOutput();
    FiniVDPAUProcs();

    recover = false;

    InitVDPAUProcs();

    return true;
  }
  return false;
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
  if (m_vdpauOutputMethod != OUTPUT_GL_INTEROP_YUV)
  {
    if (videoMixer == VDP_INVALID_HANDLE)
    {
      CLog::Log(LOGNOTICE, " (VDPAU) Creating the video mixer");
      // Creation of VideoMixer.
      VdpVideoMixerParameter parameters[] = {
        VDP_VIDEO_MIXER_PARAMETER_VIDEO_SURFACE_WIDTH,
        VDP_VIDEO_MIXER_PARAMETER_VIDEO_SURFACE_HEIGHT,
        VDP_VIDEO_MIXER_PARAMETER_CHROMA_TYPE};

      void const * parameter_values[] = {
        &vid_width,
        &vid_height,
        &vdp_chroma_type};

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
    if (tmpDeint != g_settings.m_currentVideoSettings.m_InterlaceMethod)
    {
      tmpDeint = g_settings.m_currentVideoSettings.m_InterlaceMethod;
      SetDeinterlacing();
    }
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
  || method == VS_INTERLACEMETHOD_VDPAU_NONE)
    return true;

  if (hasVdpauGlInterop)
  {
    if (method == VS_INTERLACEMETHOD_RENDER_BOB)
      return true;
  }

  for(SInterlaceMapping* p = g_interlace_mapping; p->method != VS_INTERLACEMETHOD_NONE; p++)
  {
    if(p->method == method)
      return Supports(p->feature);
  }
  return false;
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

  if (videoMixer == VDP_INVALID_HANDLE)
    return;

  EINTERLACEMETHOD method = g_settings.m_currentVideoSettings.m_InterlaceMethod;

  VdpVideoMixerFeature feature[] = { VDP_VIDEO_MIXER_FEATURE_DEINTERLACE_TEMPORAL,
                                     VDP_VIDEO_MIXER_FEATURE_DEINTERLACE_TEMPORAL_SPATIAL,
                                     VDP_VIDEO_MIXER_FEATURE_INVERSE_TELECINE };

  if (method == VS_INTERLACEMETHOD_AUTO)
  {
    VdpBool enabled[]={1,1,1};
    vdp_st = vdp_video_mixer_set_feature_enables(videoMixer, ARSIZE(feature), feature, enabled);
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
  CheckStatus(vdp_st, __LINE__);
}

void CVDPAU::SetDeinterlacingOff()
{
  VdpStatus vdp_st;

  if (videoMixer == VDP_INVALID_HANDLE)
    return;

  EINTERLACEMETHOD method = g_settings.m_currentVideoSettings.m_InterlaceMethod;

  VdpVideoMixerFeature feature[] = { VDP_VIDEO_MIXER_FEATURE_DEINTERLACE_TEMPORAL,
                                     VDP_VIDEO_MIXER_FEATURE_DEINTERLACE_TEMPORAL_SPATIAL,
                                     VDP_VIDEO_MIXER_FEATURE_INVERSE_TELECINE };

  VdpBool enabled[]={0,0,0};
  vdp_st = vdp_video_mixer_set_feature_enables(videoMixer, ARSIZE(feature), feature, enabled);
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
  vdp_st = dl_vdp_device_create_x11(m_Display, //x_display,
                                 mScreen, //x_screen,
                                 &vdp_device,
                                 &vdp_get_proc_address);

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

  vdp_st = vdp_preemption_callback_register(vdp_device,
                                   &VDPPreemptionCallbackFunction,
                                   (void*)this);
  CheckStatus(vdp_st, __LINE__);
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
  CheckStatus(vdp_st, __LINE__);
  decoder = VDP_INVALID_HANDLE;

  CSingleLock lock(m_videoSurfaceSec);
  for(unsigned int i = 0; i < m_videoSurfaces.size(); i++)
  {
    vdp_st = vdp_video_surface_destroy(m_videoSurfaces[i]->surface);
    CheckStatus(vdp_st, __LINE__);
    m_videoSurfaces[i]->surface = VDP_INVALID_HANDLE;
    free(m_videoSurfaces[i]);
  }
  m_videoSurfaces.clear();
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

  past[1] = past[0] = current = future[0] = future[1] = NULL;
  CLog::Log(LOGNOTICE, " (VDPAU) screenWidth:%i vidWidth:%i",OutWidth,vid_width);
  CLog::Log(LOGNOTICE, " (VDPAU) screenHeight:%i vidHeight:%i",OutHeight,vid_height);
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
                              vid_width,
                              vid_height,
                              max_references,
                              &decoder);
  CHECK_VDPAU_RETURN(vdp_st, false);

  m_vdpauOutputMethod = OUTPUT_NONE;

  vdpauConfigured = true;
  m_bNormalSpeed = true;
  return true;
}

bool CVDPAU::ConfigOutputMethod(AVCodecContext *avctx, AVFrame *pFrame)
{
  VdpStatus vdp_st;

  if (!pFrame)
    return true;

  // check if one of the vdpau interlacing methods are chosen
  m_bVdpauDeinterlacing = false;
  EINTERLACEMETHOD method = g_settings.m_currentVideoSettings.m_InterlaceMethod;
  if((method == VS_INTERLACEMETHOD_AUTO && pFrame->interlaced_frame)
     ||  method == VS_INTERLACEMETHOD_VDPAU_BOB
     ||  method == VS_INTERLACEMETHOD_VDPAU_TEMPORAL
     ||  method == VS_INTERLACEMETHOD_VDPAU_TEMPORAL_HALF
     ||  method == VS_INTERLACEMETHOD_VDPAU_TEMPORAL_SPATIAL
     ||  method == VS_INTERLACEMETHOD_VDPAU_TEMPORAL_SPATIAL_HALF
     ||  method == VS_INTERLACEMETHOD_VDPAU_INVERSE_TELECINE)
  {
    m_bVdpauDeinterlacing = true;
  }

  if (!m_bVdpauDeinterlacing && method != VS_INTERLACEMETHOD_VDPAU_NONE && hasVdpauGlInterop)
  {
    if (m_vdpauOutputMethod == OUTPUT_GL_INTEROP_YUV)
      return true;

    FiniOutputMethod();
    CLog::Log(LOGNOTICE, " (VDPAU) Configure YUV output");

    for (int i = 0; i < NUM_OUTPUT_SURFACES; i++)
    {
      m_allOutPic[i].render = NULL;
      m_freeOutPic.push_back(&m_allOutPic[i]);
    }

    m_vdpauOutputMethod = OUTPUT_GL_INTEROP_YUV;
  }
  // RGB config for pixmap and gl interop with vdpau deinterlacing
  else
  {
    if (m_vdpauOutputMethod == OUTPUT_GL_INTEROP_RGB
        || m_vdpauOutputMethod == OUTPUT_PIXMAP)
      return true;

    FiniOutputMethod();

    CLog::Log(LOGNOTICE, " (VDPAU) Configure RGB output");

    // create mixer thread
    Create();

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
      CHECK_VDPAU_RETURN(vdp_st, false)

      m_allOutPic[i].outputSurface = outputSurfaces[i];
      m_allOutPic[i].render = NULL;
      m_freeOutPic.push_back(&m_allOutPic[i]);
      totalAvailableOutputSurfaces++;
    }
    CLog::Log(LOGNOTICE, " (VDPAU) Total Output Surfaces Available: %i of a max (tmp: %i const: %i)",
                       totalAvailableOutputSurfaces,
                       tmpMaxOutputSurfaces,
                       NUM_OUTPUT_SURFACES);

    m_mixerCmd = 0;

    if (hasVdpauGlInterop)
      m_vdpauOutputMethod = OUTPUT_GL_INTEROP_RGB;
    else
    {
      for (int i = 0; i < 3; i++)
      {
        MakePixmap(i, OutWidth, OutHeight);
        vdp_st = vdp_presentation_queue_target_create_x11(vdp_device,
                                                          m_Pixmap[i], //x_window,
                                                          &vdp_flip_target[i]);
        CHECK_VDPAU_RETURN(vdp_st, false);

        vdp_st = vdp_presentation_queue_create(vdp_device,
                                               vdp_flip_target[i],
                                               &vdp_flip_queue[i]);
        CHECK_VDPAU_RETURN(vdp_st, false);
      }
      m_vdpauOutputMethod = OUTPUT_PIXMAP;
    }
  } // RBG

  return true;
}

bool CVDPAU::FiniOutputMethod()
{
  VdpStatus vdp_st;

  CSingleLock lock(m_flipSec);

  // stop mixer thread
  StopThread();

  presentSurface = VDP_INVALID_HANDLE;

  for (int i = 0; i < totalAvailableOutputSurfaces; i++)
  {
    vdp_st = vdp_output_surface_destroy(outputSurfaces[i]);
    CheckStatus(vdp_st, __LINE__);
    outputSurfaces[i] = VDP_INVALID_HANDLE;
  }
  totalAvailableOutputSurfaces = 0;

  if (videoMixer != VDP_INVALID_HANDLE)
  {
    vdp_st = vdp_video_mixer_destroy(videoMixer);
    CheckStatus(vdp_st, __LINE__);
    videoMixer = VDP_INVALID_HANDLE;
  }

  // destroy pixmap stuff
  for (int i = 0; i < 3; i++)
  {
    if (vdp_flip_queue[i] != VDP_INVALID_HANDLE)
    {
      vdp_st = vdp_presentation_queue_destroy(vdp_flip_queue[i]);
      CheckStatus(vdp_st, __LINE__);
      vdp_flip_queue[i] = VDP_INVALID_HANDLE;
    }
    if (vdp_flip_target[i] != VDP_INVALID_HANDLE)
    {
      vdp_st = vdp_presentation_queue_target_destroy(vdp_flip_target[i]);
      CheckStatus(vdp_st, __LINE__);
      vdp_flip_target[i] = VDP_INVALID_HANDLE;
    }
    if (m_glPixmap[i])
    {
      CLog::Log(LOGDEBUG, "GLX: Destroying glPixmap");
      glXReleaseTexImageEXT(m_Display, m_glPixmap[i], GLX_FRONT_LEFT_EXT);
      glXDestroyPixmap(m_Display, m_glPixmap[i]);
      m_glPixmap[i] = NULL;
    }
    if (m_Pixmap[i])
    {
      CLog::Log(LOGDEBUG, "GLX: Destroying XPixmap");
      XFreePixmap(m_Display, m_Pixmap[i]);
      m_Pixmap[i] = NULL;
    }
    if (m_glContext[i])
    {
      CLog::Log(LOGDEBUG, "GLX: Destroying glContext");
      glXDestroyContext(m_Display, m_glContext[i]);
      m_glContext[i] = NULL;
    }
  }

  { CSingleLock lock(m_mixerSec);
    while (!m_mixerMessages.empty())
      m_mixerMessages.pop();
    while (!m_mixerInput.empty())
      m_mixerInput.pop_front();
  }
  { CSingleLock lock(m_outPicSec);
    while (!m_freeOutPic.empty())
      m_freeOutPic.pop_front();
    while (!m_usedOutPic.empty())
      m_usedOutPic.pop_front();
  }
  m_presentPicture = m_flipBuffer[0] = m_flipBuffer[1] = m_flipBuffer[2] = NULL;
  m_flipBufferIdx = 0;

  // force cleanup of opengl interop
  glInteropFinish = true;
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

int CVDPAU::FFGetBuffer(AVCodecContext *avctx, AVFrame *pic)
{
  //CLog::Log(LOGNOTICE,"%s",__FUNCTION__);
  CDVDVideoCodecFFmpeg* ctx        = (CDVDVideoCodecFFmpeg*)avctx->opaque;
  CVDPAU*               vdp        = (CVDPAU*)ctx->GetHardware();
  struct pictureAge*    pA         = &vdp->picAge;

  // while we are waiting to recover we can't do anything
  if(vdp->recover)
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
    ReadFormatOf(avctx->pix_fmt, profile, vdp->vdp_chroma_type);
    render = (vdpau_render_state*)calloc(sizeof(vdpau_render_state), 1);
    vdp_st = vdp->vdp_video_surface_create(vdp->vdp_device,
                                           vdp->vdp_chroma_type,
                                           avctx->width,
                                           avctx->height,
                                           &render->surface);
    vdp->CheckStatus(vdp_st, __LINE__);
    if (vdp_st != VDP_STATUS_OK)
    {
      free(render);
      CLog::Log(LOGERROR, "CVDPAU::FFGetBuffer - No Video surface available could be created");
      return -1;
    }
    CSingleLock lock(vdp->m_videoSurfaceSec);
    vdp->m_videoSurfaces.push_back(render);
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
  vdpau_render_state * render;
  int i;

  render=(vdpau_render_state*)pic->data[0];
  if(!render)
  {
    CLog::Log(LOGERROR, "CVDPAU::FFDrawSlice - invalid context handle provided");
    return;
  }

  CSingleLock lock(vdp->m_videoSurfaceSec);
  render->state &= ~FF_VDPAU_STATE_USED_FOR_REFERENCE;
  for(i=0; i<4; i++)
    pic->data[i]= NULL;
}


void CVDPAU::FFDrawSlice(struct AVCodecContext *s,
                                           const AVFrame *src, int offset[4],
                                           int y, int type, int height)
{
  CDVDVideoCodecFFmpeg* ctx = (CDVDVideoCodecFFmpeg*)s->opaque;
  CVDPAU*               vdp = (CVDPAU*)ctx->GetHardware();

  /* while we are waiting to recover we can't do anything */
  if(vdp->recover)
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
  int retval;

  // configure vdpau output
  if (!ConfigOutputMethod(avctx, pFrame))
    return VC_FLUSHED;

  if (CheckRecover(false))
    return VC_FLUSHED;

  if (!vdpauConfigured)
    return VC_ERROR;

  CheckFeatures();

  if (( (int)outRectVid.x1 != OutWidth ) ||
      ( (int)outRectVid.y1 != OutHeight ))
  {
    outRectVid.x0 = 0;
    outRectVid.y0 = 0;
    outRectVid.x1 = OutWidth;
    outRectVid.y1 = OutHeight;
  }

  if (m_presentPicture)
  {
    CSingleLock lock(m_outPicSec);
    if (m_presentPicture->render)
      m_presentPicture->render->state &= ~FF_VDPAU_STATE_USED_FOR_RENDER;
    m_presentPicture->render = NULL;
    m_freeOutPic.push_back(m_presentPicture);
    m_presentPicture = NULL;
    int pics = m_usedOutPic.size() + m_freeOutPic.size();
    if (m_flipBuffer[0])
      pics++;
    if (m_flipBuffer[1])
      pics++;
    if (m_flipBuffer[2])
      pics++;
    CLog::Log(LOGDEBUG, "CVDPAU::Decode: last picture was not presented, noOfPics: %d", pics);
  }

  if(pFrame)
  { // we have a new frame from decoder

    vdpau_render_state * render = (vdpau_render_state*)pFrame->data[2];
    if(!render) // old style ffmpeg gave data on plane 0
      render = (vdpau_render_state*)pFrame->data[0];
    if(!render)
    {
      CLog::Log(LOGERROR, "CVDPAU::Decode: no valid frame");
      return VC_ERROR;
    }

    CSingleLock lock(m_videoSurfaceSec);
    render->state |= FF_VDPAU_STATE_USED_FOR_RENDER;
    lock.Leave();

    if (m_vdpauOutputMethod == OUTPUT_GL_INTEROP_YUV)
    {
      CSingleLock lock(m_outPicSec);

      // just in case, should not happen
      while (m_usedOutPic.size() > 0)
      {
        OutputPicture *pic = m_usedOutPic.front();
        if (pic->render)
          pic->render->state &= ~FF_VDPAU_STATE_USED_FOR_RENDER;
        pic->render = NULL;
        m_usedOutPic.pop_front();
        m_freeOutPic.push_back(pic);
        CLog::Log(LOGWARNING, "CVDPAU::Decode: yuv still pictures in queue");
      }

      OutputPicture *outPic = m_freeOutPic.front();
      m_freeOutPic.pop_front();
      memset(&outPic->DVDPic, 0, sizeof(DVDVideoPicture));
      ((CDVDVideoCodecFFmpeg*)avctx->opaque)->GetPictureCommon(&outPic->DVDPic);
      outPic->render = render;
      m_usedOutPic.push_back(outPic);
      lock.Leave();
      return VC_PICTURE | VC_BUFFER;
    }

    MixerMessage msg;
    msg.render = render;
    memset(&msg.DVDPic, 0, sizeof(DVDVideoPicture));
    ((CDVDVideoCodecFFmpeg*)avctx->opaque)->GetPictureCommon(&msg.DVDPic);
    msg.outRectVid = outRectVid;

    { CSingleLock lock(m_mixerSec);
      m_mixerMessages.push(msg);
    }
    m_msgSignal.Set();
  }

  if (m_vdpauOutputMethod == OUTPUT_GL_INTEROP_YUV)
  {
    CLog::Log(LOGERROR, "CVDPAU::Decode: mismatch in vdpau output method");
    return VC_ERROR;
  }

  if (avctx->hurry_up)
  {
    if (m_usedOutPic.size() > 0)
    {
      CSingleLock lock(m_outPicSec);
      OutputPicture *pic;
      pic = m_usedOutPic.front();
      m_usedOutPic.pop_front();
      pic->render = NULL;
      m_freeOutPic.push_back(pic);
      CLog::Log(LOGDEBUG, "CVDAPU::Decode: hurry drop picused: %d, picfree: %d",
                m_usedOutPic.size(), m_freeOutPic.size());
    }
    else
    {
      CSingleLock lock(m_mixerSec);
      m_mixerCmd |= MIXER_CMD_HURRY;
      CLog::Log(LOGDEBUG, "CVDAPU::Decode: hurry drop next pic in mixer msg: %d",
                m_mixerMessages.size());
    }
    m_dropCount++;

    // dropping should occur at the end of the queue, not in the middle
    // need to prevent ffmpeg from dropping frames
    CSingleLock lock(m_mixerSec);
    if (m_mixerMessages.size() < 3)
      return VC_BUFFER | VC_DROPPED;
    else
      return VC_DROPPED;
  }
  else
    m_dropCount = 0;

  int noOfMsg, noOfPics, noOfFreePics;
  noOfMsg = m_mixerMessages.size();
  noOfPics = m_usedOutPic.size();
  noOfFreePics = m_freeOutPic.size();

  // wait for mixer to get pic
  int usedPics, msgs;
  while (1)
  {
    { CSingleLock lock(m_outPicSec);
      usedPics = m_usedOutPic.size();
    }
    { CSingleLock lock(m_mixerSec);
      msgs = m_mixerMessages.size();
    }
    if (usedPics != 0 || msgs == 0)
      break;

    if (!m_picSignal.WaitMSec(100))
    {
      { CSingleLock lock(m_outPicSec);
         usedPics = m_usedOutPic.size();
      }
      { CSingleLock lock(m_mixerSec);
         msgs = m_mixerMessages.size();
      }
      CLog::Log(LOGWARNING, "CVDPAU::Decode: timed out waiting for picture, messages: %d, pics %d",
                 msgs, usedPics);
      break;
    }
  }

  retval = 0;
  if (msgs < 3 && usedPics < 4)
    retval |= VC_BUFFER;

  if (usedPics > 0)
  {
    retval |= VC_PICTURE;
  }

  // just in case - should not happen
  if (!retval)
  {
    CLog::Log(LOGERROR, "CVDAPU::Decode: no picture, used: %d", usedPics);
    retval | VC_ERROR;
  }

  return retval;
}

bool CVDPAU::GetPicture(AVCodecContext* avctx, AVFrame* frame, DVDVideoPicture* picture)
{
  { CSingleLock lock(m_outPicSec);

    if (m_presentPicture)
    {
      CLog::Log(LOGWARNING,"CVDPAU::GetPicture: old presentPicture still valid");
      if (m_presentPicture->render)
        m_presentPicture->render->state &= ~FF_VDPAU_STATE_USED_FOR_RENDER;
      m_presentPicture->render = NULL;
      m_freeOutPic.push_back(m_presentPicture);
      m_presentPicture = NULL;
    }
    else if (m_usedOutPic.size() > 0)
    {
      m_presentPicture = m_usedOutPic.front();
      m_usedOutPic.pop_front();
    }
    else
    {
      CLog::Log(LOGERROR,"CVDPAU::GetPicture: no picture");
      return false;
    }
  }

  *picture = m_presentPicture->DVDPic;

  if (m_presentPicture->render)
  {
    picture->format = DVDVideoPicture::FMT_VDPAU_420;
    if (!m_bNormalSpeed)
      picture->iFlags &= DVP_FLAG_DROPPED;
  }
  else
  {
    picture->format = DVDVideoPicture::FMT_VDPAU;
    picture->iFlags &= DVP_FLAG_DROPPED;
  }

  picture->iWidth = OutWidth;
  picture->iHeight = OutHeight;
  picture->vdpau = this;

  m_presentPicture->DVDPic = *picture;

  return true;
}

void CVDPAU::Reset()
{
  CSingleLock lock1(m_mixerSec);
  while (!m_mixerMessages.empty())
  {
    MixerMessage &tmp = m_mixerMessages.front();
    tmp.render->state &= ~FF_VDPAU_STATE_USED_FOR_RENDER;
    m_mixerMessages.pop();
  }
  lock1.Leave();

  m_mixerCmd |= MIXER_CMD_FLUSH;

  CSingleLock lock2(m_outPicSec);
  if (m_presentPicture)
  {
    if (m_presentPicture->render)
      m_presentPicture->render->state &= ~FF_VDPAU_STATE_USED_FOR_RENDER;
    m_presentPicture->render = NULL;
    m_freeOutPic.push_back(m_presentPicture);
    m_presentPicture = NULL;
  }
  while (!m_usedOutPic.empty())
  {
    OutputPicture *pic = m_usedOutPic.front();
    if (pic->render)
    {
      pic->render->state &= ~FF_VDPAU_STATE_USED_FOR_RENDER;
      pic->render = NULL;
    }
    m_usedOutPic.pop_front();
    m_freeOutPic.push_back(pic);
  }
}

// this is called from DVDPlayerVideo when seeking
// no need to have deinterlacing in this case
void CVDPAU::NormalSpeed(bool normal)
{
  if (normal)
  {
    SetDeinterlacing();
    m_bNormalSpeed = true;
  }
  else
  {
    SetDeinterlacingOff();
    m_bNormalSpeed = false;
  }
}

bool CVDPAU::AllowDecoderDrop()
{
  if (m_bVdpauDeinterlacing && m_dropCount < 5)
    return false;
  else
    return true;
}

void CVDPAU::Present()
{
//  CLog::Log(LOGNOTICE,"%s",__FUNCTION__);
  VdpStatus vdp_st;

  if (!m_presentPicture)
    CLog::Log(LOGWARNING, "CVDPAU::Present: present picture is NULL");

  int index = NextBuffer();

  if (m_flipBuffer[index])
  {
    if (m_flipBuffer[index]->render)
    {
      CSingleLock lock(m_videoSurfaceSec);
      m_flipBuffer[index]->render->state &= ~FF_VDPAU_STATE_USED_FOR_RENDER;
      m_flipBuffer[index]->render = NULL;
    }
    CSingleLock lock(m_outPicSec);
    m_freeOutPic.push_back(m_flipBuffer[index]);
    m_flipBuffer[index] = NULL;
  }

  m_flipBuffer[index] = m_presentPicture;
  m_presentPicture = NULL;

  if (hasVdpauGlInterop)
    return;

  presentSurface = m_flipBuffer[index]->outputSurface;

  vdp_st = vdp_presentation_queue_display(vdp_flip_queue[index],
                                          presentSurface,
                                          0,
                                          0,
                                          0);
  CheckStatus(vdp_st, __LINE__);
}

void CVDPAU::Flip()
{
//  CLog::Log(LOGNOTICE,"%s",__FUNCTION__);
  CSingleLock lock(m_flipSec);

  if (!m_flipBuffer[NextBuffer()])
    return;

  m_flipBufferIdx = NextBuffer();

  if (m_flipBuffer[NextBuffer()])
  {
    //map / unmap kills performance
//    GLUnmapSurface(m_flipBuffer[NextBuffer()]);
    if (m_flipBuffer[NextBuffer()]->render)
    {
      CSingleLock lock(m_videoSurfaceSec);
      m_flipBuffer[NextBuffer()]->render->state &= ~FF_VDPAU_STATE_USED_FOR_RENDER;
      m_flipBuffer[NextBuffer()]->render = NULL;
    }
    CSingleLock lock(m_outPicSec);
    m_freeOutPic.push_back(m_flipBuffer[NextBuffer()]);
    m_flipBuffer[NextBuffer()] = NULL;
  }
  m_bsurfaceMapped = false;
}

void CVDPAU::VDPPreemptionCallbackFunction(VdpDevice device, void* context)
{
  CLog::Log(LOGERROR,"VDPAU Device Preempted - attempting recovery");
  CVDPAU* pCtx = (CVDPAU*)context;
  pCtx->recover = true;
}

bool CVDPAU::CheckStatus(VdpStatus vdp_st, int line)
{
  if (vdp_st == VDP_STATUS_HANDLE_DEVICE_MISMATCH
  ||  vdp_st == VDP_STATUS_DISPLAY_PREEMPTED)
    recover = true;

  // no need to log errors about this case, as it will happen on cleanup
  if (vdp_st == VDP_STATUS_INVALID_HANDLE && recover && vdpauConfigured)
    return false;

  if (vdp_st != VDP_STATUS_OK)
  {
    CLog::Log(LOGERROR, " (VDPAU) Error: %s(%d) at %s:%d\n", vdp_get_error_string(vdp_st), vdp_st, __FILE__, line);
    recover = true;
    return true;
  }
  return false;
}

void CVDPAU::FlushMixer()
{
  { CSingleLock lock(m_videoSurfaceSec);
    while (!m_mixerInput.empty())
    {
      MixerMessage &tmp = m_mixerInput.front();
      tmp.render->state &= ~FF_VDPAU_STATE_USED_FOR_RENDER;
      m_mixerInput.pop_front();
    }
  }
  { CSingleLock lock(m_outPicSec);
    while (!m_usedOutPic.empty())
    {
      OutputPicture *pic = m_usedOutPic.front();
      if (pic->render)
      {
        CSingleLock lock1(m_videoSurfaceSec);
        pic->render->state &= ~FF_VDPAU_STATE_USED_FOR_RENDER;
        pic->render = NULL;
      }
      m_usedOutPic.pop_front();
      m_freeOutPic.push_back(pic);
    }
  }
}

void CVDPAU::Process()
{
  VdpStatus vdp_st;
  VdpTime time;
  unsigned int cmd;
  bool gotMsg;

  CSingleLock mixerLock(m_mixerSec);
  mixerLock.Leave();
  CSingleLock outPicLock(m_outPicSec);
  outPicLock.Leave();
  CSingleLock videoSurfaceLock(m_videoSurfaceSec);
  videoSurfaceLock.Leave();

  while (!m_bStop)
  {
    // wait for message
    gotMsg = false;
    int noOfFreePics;
    MixerMessage msg;
    cmd = 0;

    outPicLock.Enter();
    noOfFreePics = m_freeOutPic.size();
    outPicLock.Leave();

    mixerLock.Enter();
    if (!m_mixerMessages.empty() && noOfFreePics > 1)
    {
      msg = m_mixerMessages.front();
      m_mixerMessages.pop();
      m_mixerInput.push_front(msg);
      cmd = m_mixerCmd;
      m_mixerCmd = 0;
      gotMsg = true;
    }
    mixerLock.Leave();

    // flush mixer input queue and already rendered pics
    if (cmd & MIXER_CMD_FLUSH)
    {
      FlushMixer();
      continue;
    }

    // wait for next picture
    if (!gotMsg)
    {
      if (!m_msgSignal.WaitMSec(20))
        ; //CLog::Log(LOGNOTICE, "CVDPAU::Process ------------- wait");
      m_picSignal.Set();
      continue;
    }

    // need 2 past and 1 future for mixer
    if (m_mixerInput.size() < 3)
    {
      m_picSignal.Set();
      continue;
    }

    int mixersteps;
    VdpVideoMixerPictureStructure mixerfield;

    EINTERLACEMETHOD method = g_settings.m_currentVideoSettings.m_InterlaceMethod;
    if((method == VS_INTERLACEMETHOD_AUTO &&
  		        m_mixerInput[1].DVDPic.iFlags & DVP_FLAG_INTERLACED)
      ||  method == VS_INTERLACEMETHOD_VDPAU_BOB
      ||  method == VS_INTERLACEMETHOD_VDPAU_TEMPORAL
      ||  method == VS_INTERLACEMETHOD_VDPAU_TEMPORAL_HALF
      ||  method == VS_INTERLACEMETHOD_VDPAU_TEMPORAL_SPATIAL
      ||  method == VS_INTERLACEMETHOD_VDPAU_TEMPORAL_SPATIAL_HALF
      ||  method == VS_INTERLACEMETHOD_VDPAU_INVERSE_TELECINE )
    {
      if(method == VS_INTERLACEMETHOD_VDPAU_TEMPORAL_HALF
        || method == VS_INTERLACEMETHOD_VDPAU_TEMPORAL_SPATIAL_HALF)
        mixersteps = 1;
      else
        mixersteps = 2;

      if(m_mixerInput[1].DVDPic.iFlags & DVP_FLAG_TOP_FIELD_FIRST)
        mixerfield = VDP_VIDEO_MIXER_PICTURE_STRUCTURE_TOP_FIELD;
      else
        mixerfield = VDP_VIDEO_MIXER_PICTURE_STRUCTURE_BOTTOM_FIELD;
    }
    else
    {
      mixersteps = 1;
      mixerfield = VDP_VIDEO_MIXER_PICTURE_STRUCTURE_FRAME;
    }

    if (cmd & MIXER_CMD_HURRY)
    {
      mixersteps--;
    }

    // mixer stage
    for (int mixerstep = 0; mixerstep < mixersteps; mixerstep++)
    {
      if (mixerstep == 1)
      {
        if(mixerfield == VDP_VIDEO_MIXER_PICTURE_STRUCTURE_TOP_FIELD)
          mixerfield = VDP_VIDEO_MIXER_PICTURE_STRUCTURE_BOTTOM_FIELD;
        else
          mixerfield = VDP_VIDEO_MIXER_PICTURE_STRUCTURE_TOP_FIELD;
      }

      VdpVideoSurface past_surfaces[4] = { VDP_INVALID_HANDLE, VDP_INVALID_HANDLE, VDP_INVALID_HANDLE, VDP_INVALID_HANDLE };
      VdpVideoSurface futu_surfaces[2] = { VDP_INVALID_HANDLE, VDP_INVALID_HANDLE };
      uint32_t pastCount = 4;
      uint32_t futuCount = 2;

      VdpRect sourceRect = m_mixerInput[1].outRectVid;

      if(mixerfield == VDP_VIDEO_MIXER_PICTURE_STRUCTURE_FRAME)
      {
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
        if(mixerstep == 0)
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
          futu_surfaces[1] = m_mixerInput[0].render->surface;
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
          futu_surfaces[0] = m_mixerInput[0].render->surface;
          futu_surfaces[1] = m_mixerInput[0].render->surface;
        }
        sourceRect.x0 += 4;
        sourceRect.y0 += 2;
        sourceRect.x1 -= 4;
        sourceRect.y1 -= 2;
      }

      // get free pic from queue
      outPicLock.Enter();
      if (m_freeOutPic.empty())
      {
        CLog::Log(LOGERROR, "CVDPAU::Process: no output picture available");
        outPicLock.Leave();
        break;
      }
      OutputPicture *outPic = m_freeOutPic.front();
      m_freeOutPic.pop_front();
      outPicLock.Leave();
      outPic->render = NULL;

      // set pts / dts for interlaced pic
      outPic->DVDPic = m_mixerInput[1].DVDPic;
      if (mixerfield != VDP_VIDEO_MIXER_PICTURE_STRUCTURE_FRAME
          && method != VS_INTERLACEMETHOD_VDPAU_TEMPORAL_HALF
          && method != VS_INTERLACEMETHOD_VDPAU_TEMPORAL_SPATIAL_HALF)
      {
        outPic->DVDPic.iRepeatPicture = -0.5;
        if (mixerstep == 1)
        {
          outPic->DVDPic.dts = DVD_NOPTS_VALUE;
          outPic->DVDPic.pts = DVD_NOPTS_VALUE;
        }
      }

      // start vdpau video mixer
      vdp_st = vdp_video_mixer_render(videoMixer,
                                VDP_INVALID_HANDLE,
                                0,
                                mixerfield,
                                pastCount,
                                past_surfaces,
                                m_mixerInput[1].render->surface,
                                futuCount,
                                futu_surfaces,
                                &sourceRect,
                                outPic->outputSurface,
                                &(m_mixerInput[1].outRectVid),
                                &(m_mixerInput[1].outRectVid),
                                0,
                                NULL);
      CheckStatus(vdp_st, __LINE__);

      // put pic in out queue
      outPicLock.Enter();
      m_usedOutPic.push_back(outPic);
      outPicLock.Leave();

      // mixing could have taken a while, check for new command
      mixerLock.Enter();
      cmd = m_mixerCmd;
      m_picSignal.Set();
      mixerLock.Leave();

      if (cmd)
        break;

    }// for (mixer stage)
    while (m_mixerInput.size() > 3)
    {
      videoSurfaceLock.Enter();
      MixerMessage &tmp = m_mixerInput.back();
      tmp.render->state &= ~FF_VDPAU_STATE_USED_FOR_RENDER;
      m_mixerInput.pop_back();
      m_picSignal.Set();
      videoSurfaceLock.Leave();
    }
  }//while not stop
}

void CVDPAU::OnStartup()
{
  CLog::Log(LOGNOTICE, "CVDPAU::OnStartup: Mixer Thread created");
}

void CVDPAU::OnExit()
{
  CLog::Log(LOGNOTICE, "CVDPAU::OnExit: Mixer Thread terminated");
}

#ifdef GL_NV_vdpau_interop
void CVDPAU::GLInitInterop()
{
  m_renderThread = CThread::GetCurrentThreadId();

  while (glGetError() != GL_NO_ERROR) ;
  glVDPAUInitNV((GLvoid*)vdp_device, (GLvoid*)vdp_get_proc_address);
  if (glGetError() != GL_NO_ERROR)
  {
    CLog::Log(LOGERROR, "CVDPAU::GLInitInterop glVDPAUInitNV failed");
  }

  glInteropFinish = false;
  CLog::Log(LOGNOTICE, "CVDPAU::GlInitInterop: gl interop initialized");
}

void CVDPAU::GLFiniInterop()
{
  if (m_GlInteropStatus == OUTPUT_NONE)
  {
    glInteropFinish = false;
    return;
  }

  glVDPAUFiniNV();

  for (int i=0; i < NUM_OUTPUT_SURFACES; i++)
  {
    if (glIsTexture(m_allOutPic[i].texture[0]))
    {
      glVDPAUUnregisterSurfaceNV(m_allOutPic[i].glVdpauSurface);
      glDeleteTextures(1, m_allOutPic[i].texture);
    }
  }
  std::map<VdpVideoSurface, GLVideoSurface>::iterator it;
  for (it = m_videoSurfaceMap.begin(); it != m_videoSurfaceMap.end(); ++it)
  {
    glVDPAUUnregisterSurfaceNV(it->second.glVdpauSurface);
    glDeleteTextures(4, it->second.texture);
  }
  m_videoSurfaceMap.clear();

  m_GlInteropStatus = OUTPUT_NONE;
  glInteropFinish = false;
  CLog::Log(LOGNOTICE, "CVDPAU::GlFiniInterop: gl interop finished");
}

bool CVDPAU::GLMapSurface(OutputPicture *outPic)
{
  bool bReturn = true;
  if (outPic->DVDPic.format == DVDVideoPicture::FMT_VDPAU)
  {
    if (m_GlInteropStatus != OUTPUT_GL_INTEROP_RGB)
    {
      GLInitInterop();
      bReturn = GLRegisterOutputSurfaces();
      m_GlInteropStatus = OUTPUT_GL_INTEROP_RGB;
    }
//    glVDPAUMapSurfacesNV(1, &outPic->glVdpauSurface);
  }
  else if (outPic->DVDPic.format == DVDVideoPicture::FMT_VDPAU_420)
  {
    if (m_GlInteropStatus != OUTPUT_GL_INTEROP_YUV)
    {
      GLInitInterop();
      m_GlInteropStatus = OUTPUT_GL_INTEROP_YUV;
    }
    bReturn = GLRegisterVideoSurfaces(outPic);
    GLVideoSurface surface = m_videoSurfaceMap[outPic->render->surface];
//    glVDPAUMapSurfacesNV(1, &surface.glVdpauSurface);
    for (int i = 0; i < 4; i++)
      outPic->texture[i] = surface.texture[i];
  }
  return bReturn;
}

bool CVDPAU::GLUnmapSurface(OutputPicture *outPic)
{
  if (outPic->DVDPic.format == DVDVideoPicture::FMT_VDPAU)
  {
    glVDPAUUnmapSurfacesNV(1, &outPic->glVdpauSurface);
  }
  else if (outPic->DVDPic.format == DVDVideoPicture::FMT_VDPAU_420)
  {
    GLVideoSurface surface = m_videoSurfaceMap[outPic->render->surface];
    glVDPAUUnmapSurfacesNV(1, &surface.glVdpauSurface);
  }
  return true;
}

bool CVDPAU::GLRegisterOutputSurfaces()
{
  for (int i=0; i<NUM_OUTPUT_SURFACES;i++)
  {
    glGenTextures(1, m_allOutPic[i].texture);
    m_allOutPic[i].glVdpauSurface = glVDPAURegisterOutputSurfaceNV((GLvoid*)m_allOutPic[i].outputSurface,
                                               GL_TEXTURE_2D, 1, m_allOutPic[i].texture);
    if (glGetError() != GL_NO_ERROR)
    {
      CLog::Log(LOGERROR, "CVDPAU::GLRegisterOutputSurfaces error register output surface");
      return false;
    }
    glVDPAUSurfaceAccessNV(m_allOutPic[i].glVdpauSurface, GL_READ_ONLY);
    if (glGetError() != GL_NO_ERROR)
    {
      CLog::Log(LOGERROR, "CVDPAU::GLRegisterOutputSurfaces error setting access");
      return false;
    }
    glVDPAUMapSurfacesNV(1, &m_allOutPic[i].glVdpauSurface);
    if (glGetError() != GL_NO_ERROR)
    {
      CLog::Log(LOGERROR, "CVDPAU::GLRegisterOutputSurfaces error mapping surface");
      return false;
    }
  }
  return true;
}

bool CVDPAU::GLRegisterVideoSurfaces(OutputPicture *outPic)
{
  CSingleLock lock(m_videoSurfaceSec);
  bool bError = false;
  if (m_videoSurfaces.size() != m_videoSurfaceMap.size())
  {
    for (int i = 0; i < m_videoSurfaces.size(); i++)
    {
      if (m_videoSurfaceMap.find(m_videoSurfaces[i]->surface) == m_videoSurfaceMap.end())
      {
        GLVideoSurface glVideoSurface;
        while (glGetError() != GL_NO_ERROR) ;
        glGenTextures(4, glVideoSurface.texture);
        if (glGetError() != GL_NO_ERROR)
        {
           CLog::Log(LOGERROR, "CVDPAU::GLRegisterVideoSurfaces error creating texture");
           bError = true;
        }
        glVideoSurface.glVdpauSurface = glVDPAURegisterVideoSurfaceNV((GLvoid*)(m_videoSurfaces[i]->surface),
                                                  GL_TEXTURE_2D, 4, glVideoSurface.texture);
        if (glGetError() != GL_NO_ERROR)
        {
          CLog::Log(LOGERROR, "CVDPAU::GLRegisterVideoSurfaces error register video surface");
          bError = true;
        }
        glVDPAUSurfaceAccessNV(glVideoSurface.glVdpauSurface, GL_READ_ONLY);
        if (glGetError() != GL_NO_ERROR)
        {
          CLog::Log(LOGERROR, "CVDPAU::GLRegisterVideoSurfaces error setting access");
          bError = true;
        }
        glVDPAUMapSurfacesNV(1, &glVideoSurface.glVdpauSurface);
        if (glGetError() != GL_NO_ERROR)
        {
          CLog::Log(LOGERROR, "CVDPAU::GLRegisterVideoSurfaces error mapping surface");
          bError = true;
        }
        m_videoSurfaceMap[m_videoSurfaces[i]->surface] = glVideoSurface;
        if (bError)
          return false;
        CLog::Log(LOGNOTICE, "CVDPAU::GLRegisterVideoSurfaces registered surface");
      }
    }
  }
  return true;
}

#endif

GLuint CVDPAU::GLGetSurfaceTexture(int plane, int field)
{
  GLuint glReturn = 0;
  if (recover)
    return glReturn;

#ifdef GL_NV_vdpau_interop

  CSingleLock lock(m_flipSec);

  //check if current output method is valid
  if (m_GlInteropStatus == OUTPUT_GL_INTEROP_RGB)
  {
    if (m_flipBuffer[m_flipBufferIdx])
    {
      if (m_flipBuffer[m_flipBufferIdx]->DVDPic.format != DVDVideoPicture::FMT_VDPAU
          || field != 0)
        glInteropFinish = true;
    }
  }
  else if (m_GlInteropStatus == OUTPUT_GL_INTEROP_YUV)
  {
    if (m_flipBuffer[m_flipBufferIdx])
     {
       if (m_flipBuffer[m_flipBufferIdx]->DVDPic.format != DVDVideoPicture::FMT_VDPAU_420
           || (field != 1 && field != 2))
         glInteropFinish = true;
     }
  }

  // check for request to finish interop
  if (glInteropFinish)
  {
     GLFiniInterop();
  }

  // register and map surface
  if (m_flipBuffer[m_flipBufferIdx])
  {
    if (!m_bsurfaceMapped)
    {
      if (!GLMapSurface(m_flipBuffer[m_flipBufferIdx]))
      {
        glInteropFinish = true;
        return 0;
      }
      m_bsurfaceMapped = true;
    }
    if (plane == 0 && (field == 0))
    {
      glReturn = m_flipBuffer[m_flipBufferIdx]->texture[0];
    }
    else if (plane == 0 && (field == 1))
    {
      glReturn = m_flipBuffer[m_flipBufferIdx]->texture[0];
    }
    else if (plane == 1 && (field == 1))
    {
      glReturn = m_flipBuffer[m_flipBufferIdx]->texture[2];
    }
    else if (plane == 0 && (field == 2))
    {
      glReturn = m_flipBuffer[m_flipBufferIdx]->texture[1];
    }
    else if (plane == 1 && (field == 2))
    {
      glReturn = m_flipBuffer[m_flipBufferIdx]->texture[3];
    }
  }
  else
    CLog::Log(LOGWARNING, "CVDPAU::GLGetSurfaceTexture - no picture, index %d", m_flipBufferIdx);

#endif
  return glReturn;
}

int CVDPAU::NextBuffer()
{
  return (m_flipBufferIdx + 1) % 3;
}

long CVDPAU::Release()
{
#ifdef GL_NV_vdpau_interop
  if (m_renderThread == CThread::GetCurrentThreadId())
  {
    InterlockedIncrement(&m_references);
    long count = InterlockedDecrement(&m_references);
    if (count < 2)
    {
      CLog::Log(LOGNOTICE, "CVDPAU::Release");
      GLFiniInterop();
    }
  }
#endif
  return CDVDVideoCodecFFmpeg::IHardwareDecoder::Release();
}
#endif
