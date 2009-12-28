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
#include "WindowingFactory.h"
#include "VDPAU.h"
#include "TextureManager.h"
#include "cores/VideoRenderers/RenderManager.h"
#include "DVDVideoCodecFFmpeg.h"
#include "Settings.h"
#include "GUISettings.h"
#define ARSIZE(x) (sizeof(x) / sizeof((x)[0]))

CVDPAU*          g_VDPAU=NULL;

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
};
const size_t decoder_profile_count = sizeof(decoder_profiles)/sizeof(CVDPAU::Desc);

static float studioCSC[3][4] = 
{
    { 1.0f,        0.0f, 1.57480000f,-0.78740000f},
    { 1.0f,-0.18737736f,-0.46813736f, 0.32775736f},
    { 1.0f, 1.85556000f,        0.0f,-0.92780000f}
};

CVDPAU::CVDPAU(int width, int height, CodecID codec)
{
  glXBindTexImageEXT = NULL;
  glXReleaseTexImageEXT = NULL;
  vdp_device = VDP_INVALID_HANDLE;
  dl_handle  = NULL;
  surfaceNum      = presentSurfaceNum = 0;
  picAge.b_age    = picAge.ip_age[0] = picAge.ip_age[1] = 256*256*256*64;
  vdpauConfigured = false;
  recover = VDPAURecovered = false;

  m_glPixmap = 0;
  m_glPixmapTexture = 0;
  m_Pixmap = 0;
  m_glContext = 0;
  if (!glXBindTexImageEXT)
    glXBindTexImageEXT    = (PFNGLXBINDTEXIMAGEEXTPROC)glXGetProcAddress((GLubyte *) "glXBindTexImageEXT");
  if (!glXReleaseTexImageEXT)
    glXReleaseTexImageEXT = (PFNGLXRELEASETEXIMAGEEXTPROC)glXGetProcAddress((GLubyte *) "glXReleaseTexImageEXT");

  totalAvailableOutputSurfaces = outputSurface = presentSurface = 0;
  vid_width = vid_height = OutWidth = OutHeight = 0;
  memset(&outRect, 0, sizeof(VdpRect));
  memset(&outRectVid, 0, sizeof(VdpRect));

  tmpBrightness  = 0;
  tmpContrast    = 0;
  interlaced     = false;
  VDPAUSwitching = false;
  max_references = 0;

  for (int i = 0; i < NUM_OUTPUT_SURFACES; i++)
    outputSurfaces[i] = VDP_INVALID_HANDLE;

  videoMixer = VDP_INVALID_HANDLE;

  dl_handle  = dlopen("libvdpau.so.1", RTLD_LAZY);
  if (!dl_handle) 
  {
    CLog::Log(LOGNOTICE,"(VDPAU) unable to get handle to libvdpau");
    return;
  }

  InitVDPAUProcs();

  if (vdp_device != VDP_INVALID_HANDLE)
  {
    SpewHardwareAvailable();

    VdpDecoderProfile profile = 0;
    if(codec == CODEC_ID_H264)
      profile = VDP_DECODER_PROFILE_H264_HIGH;
    else if(codec == CODEC_ID_MPEG4)
      profile = VDP_DECODER_PROFILE_MPEG4_PART2_ASP;

    if(profile)
    {
      /* attempt to create a decoder with this width/height, some sizes are note supported by hw */
      VdpStatus vdp_st;
      vdp_st = vdp_decoder_create(vdp_device, profile, width, height, 5, &decoder);

      if(vdp_st != VDP_STATUS_OK)
      {
        CLog::Log(LOGERROR, " (VDPAU) Error: %s(%d) checking for decoder support\n", vdp_get_error_string(vdp_st), vdp_st);
        FiniVDPAUProcs();
        return;
      }

      vdp_decoder_destroy(decoder);
      CheckStatus(vdp_st, __LINE__);
    }

    InitCSCMatrix(height);
    MakePixmap(width,height);
  }
}

CVDPAU::~CVDPAU()
{
  CLog::Log(LOGNOTICE, " (VDPAU) %s", __FUNCTION__);
  if (m_glPixmap)
  {
    CLog::Log(LOGINFO, "GLX: Destroying glPixmap");
    glXDestroyPixmap(m_Display, m_glPixmap);
    m_glPixmap = NULL;
  }
  if (m_Pixmap)
  {
    CLog::Log(LOGINFO, "GLX: Destroying XPixmap");
    XFreePixmap(m_Display, m_Pixmap);
    m_Pixmap = NULL;
  }

  FiniVDPAUOutput();
  FiniVDPAUProcs();

  if (m_glContext)
  {
    CLog::Log(LOGINFO, "GLX: Destroying glContext");
    glXDestroyContext(m_Display, m_glContext);
    m_glContext = NULL;
  }

  if (dl_handle)
  {
    dlclose(dl_handle);
    dl_handle = NULL;
  }
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
  m_glContext = glXCreateContext(m_Display, visInfo, NULL, True);
  XFree(visInfo);

  if (!glXMakeCurrent(m_Display, m_glPixmap, m_glContext))
  {
    CLog::Log(LOGINFO, "GLX Error: Could not make Pixmap current");
    return false;
  }

  /* restore what thread had before */
  glXMakeCurrent(m_Display, lastdrw, lastctx);

  return true;

}

bool CVDPAU::MakePixmap(int width, int height)
{
  OutWidth = g_graphicsContext.GetWidth();
  OutHeight = g_graphicsContext.GetHeight();

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
  if (m_glPixmap)
  {
    glXReleaseTexImageEXT(m_Display, m_glPixmap, GLX_FRONT_LEFT_EXT);
    glXBindTexImageEXT(m_Display, m_glPixmap, GLX_FRONT_LEFT_EXT, NULL);
  }
  else CLog::Log(LOGERROR,"(VDPAU) BindPixmap called without valid pixmap");
}

void CVDPAU::ReleasePixmap()
{
  if (m_glPixmap)
  {
    glXReleaseTexImageEXT(m_Display, m_glPixmap, GLX_FRONT_LEFT_EXT);
  }
  else CLog::Log(LOGERROR,"(VDPAU) ReleasePixmap called without valid pixmap");
}

void CVDPAU::CheckRecover(bool force)
{
  if (recover || force)
  {
    CLog::Log(LOGNOTICE,"Attempting recovery");

    VDPAUSwitching = true;
    FiniVDPAUOutput();
    FiniVDPAUProcs();

    InitVDPAUProcs();

    VDPAURecovered = true;
    VDPAUSwitching = false;
    recover = false;
  }
}

bool CVDPAU::IsVDPAUFormat(PixelFormat format)
{
  if ((format >= PIX_FMT_VDPAU_H264) && (format <= PIX_FMT_VDPAU_VC1)) return true;
  if (format == PIX_FMT_VDPAU_MPEG4) return true;
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
      &vid_width,
      &vid_height,
      &vdp_chroma_type
    };

    tmpBrightness = 0;
    tmpContrast = 0;
    tmpNoiseReduction = 0;
    tmpSharpness = 0;
    int featuresCount = 0;
    VdpVideoMixerFeature features[6];

    features[featuresCount++] = VDP_VIDEO_MIXER_FEATURE_NOISE_REDUCTION;
    features[featuresCount++] = VDP_VIDEO_MIXER_FEATURE_SHARPNESS;
#ifdef VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L1
    if (g_guiSettings.GetBool("videoplayer.vdpauUpscalingLevel"))
    {
      CLog::Log(LOGNOTICE,"(VDPAU) enabling VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L1");
      features[featuresCount++] = VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L1;
    }
#endif
    if (interlaced && tmpDeint)
    {
      CLog::Log(LOGNOTICE, " (VDPAU) Enabling deinterlacing features for the video mixer");
      features[featuresCount++] = VDP_VIDEO_MIXER_FEATURE_DEINTERLACE_TEMPORAL;
      features[featuresCount++] = VDP_VIDEO_MIXER_FEATURE_DEINTERLACE_TEMPORAL_SPATIAL;
      features[featuresCount++] = VDP_VIDEO_MIXER_FEATURE_INVERSE_TELECINE;
    }

    VdpStatus vdp_st = VDP_STATUS_ERROR;
    vdp_st = vdp_video_mixer_create(vdp_device,
                                    featuresCount,
                                    features,
                                    ARSIZE(parameters),
                                    parameters,
                                    parameter_values,
                                    &videoMixer);
    CheckStatus(vdp_st, __LINE__);
#ifdef VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L1
    if (g_guiSettings.GetBool("videoplayer.vdpauUpscalingLevel"))
    {
      SetHWUpscaling();
    }
#endif
  }

  if (tmpBrightness != g_settings.m_currentVideoSettings.m_Brightness ||
      tmpContrast   != g_settings.m_currentVideoSettings.m_Contrast)
  {
    SetColor(vid_height);
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

  if (interlaced && tmpDeint && tmpDeint != g_settings.m_currentVideoSettings.m_InterlaceMethod)
  {
    tmpDeint = g_settings.m_currentVideoSettings.m_InterlaceMethod;
    SetDeinterlacing();
  }
}

void CVDPAU::SetColor(int Height)
{
  VdpStatus vdp_st;

  if (tmpBrightness != g_settings.m_currentVideoSettings.m_Brightness)
    m_Procamp.brightness = (float)((g_settings.m_currentVideoSettings.m_Brightness)-50) / 100;
  if (tmpContrast != g_settings.m_currentVideoSettings.m_Contrast)
    m_Procamp.contrast = (float)((g_settings.m_currentVideoSettings.m_Contrast)+50) / 100;
  vdp_st = vdp_generate_csc_matrix(&m_Procamp, 
                                   (Height < 720)? VDP_COLOR_STANDARD_ITUR_BT_601 : VDP_COLOR_STANDARD_ITUR_BT_709,
                                   &m_CSCMatrix);
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
/* Disabled for 9.11
  CLog::Log(LOGNOTICE,"Enabling VDPAU HQ Upscaling");
  VdpVideoMixerFeature feature[] = { VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L1 };
  VdpStatus vdp_st;
  VdpBool available;
  
  vdp_st = vdp_video_mixer_query_feature_support(vdp_device, VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L1, &available);
  
  if ( (vdp_st != VDP_STATUS_OK) || !available )
  {
    CLog::Log(LOGNOTICE,"VDPAU HQ upscaling not available");
    return;
  }
  
  VdpBool enabled[]={1};
  vdp_st = vdp_video_mixer_set_feature_enables(videoMixer, ARSIZE(feature), feature, enabled);
  CheckStatus(vdp_st, __LINE__);
*/
#endif
}

void CVDPAU::SetDeinterlacing()
{
  VdpVideoMixerFeature feature[] = { VDP_VIDEO_MIXER_FEATURE_DEINTERLACE_TEMPORAL,
                                     VDP_VIDEO_MIXER_FEATURE_DEINTERLACE_TEMPORAL_SPATIAL,
                                     VDP_VIDEO_MIXER_FEATURE_INVERSE_TELECINE };

  VdpStatus vdp_st;

  if (!g_settings.m_currentVideoSettings.m_InterlaceMethod) 
  {
    VdpBool enabled[]={0,0,0};
    vdp_st = vdp_video_mixer_set_feature_enables(videoMixer, ARSIZE(feature), feature, enabled);
    CheckStatus(vdp_st, __LINE__);
  }
  else if (g_settings.m_currentVideoSettings.m_InterlaceMethod == VS_INTERLACEMETHOD_AUTO) 
  {
    VdpBool enabled[]={1,0,0};
    vdp_st = vdp_video_mixer_set_feature_enables(videoMixer, ARSIZE(feature), feature, enabled);
    CheckStatus(vdp_st, __LINE__);
  }
  else if (g_settings.m_currentVideoSettings.m_InterlaceMethod == VS_INTERLACEMETHOD_RENDER_BLEND) 
  {
    VdpBool enabled[]={1,1,0};
    vdp_st = vdp_video_mixer_set_feature_enables(videoMixer, ARSIZE(feature), feature, enabled);
    CheckStatus(vdp_st, __LINE__);
  }
  else if (g_settings.m_currentVideoSettings.m_InterlaceMethod == VS_INTERLACEMETHOD_INVERSE_TELECINE) 
  {
    VdpBool enabled[]={0,0,1};
    vdp_st = vdp_video_mixer_set_feature_enables(videoMixer, ARSIZE(feature), feature, enabled);
    CheckStatus(vdp_st, __LINE__);
  }
}

void CVDPAU::InitVDPAUProcs()
{
  char* error;

  dl_vdp_device_create_x11 = (VdpStatus (*)(Display*, int, VdpDevice*, VdpStatus (**)(VdpDevice, VdpFuncId, void**)))dlsym(dl_handle, (const char*)"vdp_device_create_x11");
  error = dlerror();
  if (error)
  {
    CLog::Log(LOGERROR,"(VDPAU) - %s in %s",error,__FUNCTION__);
    vdp_device = VDP_INVALID_HANDLE;
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

  vdp_st = vdp_get_proc_address(vdp_device,
                                VDP_FUNC_ID_GET_ERROR_STRING,
                                (void **)&vdp_get_error_string);
  CheckStatus(vdp_st, __LINE__);

  vdp_st = vdp_get_proc_address(vdp_device,
                                VDP_FUNC_ID_DEVICE_DESTROY,
                                (void **)&vdp_device_destroy);
  CheckStatus(vdp_st, __LINE__);

  vdp_st = vdp_get_proc_address(vdp_device,
                                VDP_FUNC_ID_VIDEO_SURFACE_CREATE,
                                (void **)&vdp_video_surface_create);
  CheckStatus(vdp_st, __LINE__);

  vdp_st = vdp_get_proc_address(
                                vdp_device,
                                VDP_FUNC_ID_VIDEO_SURFACE_DESTROY,
                                (void **)&vdp_video_surface_destroy
                                );
  CheckStatus(vdp_st, __LINE__);

  vdp_st = vdp_get_proc_address(
                                vdp_device,
                                VDP_FUNC_ID_VIDEO_SURFACE_PUT_BITS_Y_CB_CR,
                                (void **)&vdp_video_surface_put_bits_y_cb_cr
                                );
  CheckStatus(vdp_st, __LINE__);

  vdp_st = vdp_get_proc_address(
                                vdp_device,
                                VDP_FUNC_ID_VIDEO_SURFACE_GET_BITS_Y_CB_CR,
                                (void **)&vdp_video_surface_get_bits_y_cb_cr
                                );
  CheckStatus(vdp_st, __LINE__);

  vdp_st = vdp_get_proc_address(
                                vdp_device,
                                VDP_FUNC_ID_OUTPUT_SURFACE_PUT_BITS_Y_CB_CR,
                                (void **)&vdp_output_surface_put_bits_y_cb_cr
                                );
  CheckStatus(vdp_st, __LINE__);

  vdp_st = vdp_get_proc_address(
                                vdp_device,
                                VDP_FUNC_ID_OUTPUT_SURFACE_PUT_BITS_NATIVE,
                                (void **)&vdp_output_surface_put_bits_native
                                );
  CheckStatus(vdp_st, __LINE__);

  vdp_st = vdp_get_proc_address(
                                vdp_device,
                                VDP_FUNC_ID_OUTPUT_SURFACE_CREATE,
                                (void **)&vdp_output_surface_create
                                );
  CheckStatus(vdp_st, __LINE__);

  vdp_st = vdp_get_proc_address(
                                vdp_device,
                                VDP_FUNC_ID_OUTPUT_SURFACE_DESTROY,
                                (void **)&vdp_output_surface_destroy
                                );
  CheckStatus(vdp_st, __LINE__);

  vdp_st = vdp_get_proc_address(
                                vdp_device,
                                VDP_FUNC_ID_OUTPUT_SURFACE_GET_BITS_NATIVE,
                                (void **)&vdp_output_surface_get_bits_native
                                );
  CheckStatus(vdp_st, __LINE__);

  vdp_st = vdp_get_proc_address(
                                vdp_device,
                                VDP_FUNC_ID_VIDEO_MIXER_CREATE,
                                (void **)&vdp_video_mixer_create
                                );
  CheckStatus(vdp_st, __LINE__);

  vdp_st = vdp_get_proc_address(
                                vdp_device,
                                VDP_FUNC_ID_VIDEO_MIXER_SET_FEATURE_ENABLES,
                                (void **)&vdp_video_mixer_set_feature_enables
                                );
  CheckStatus(vdp_st, __LINE__);

  vdp_st = vdp_get_proc_address(
                                vdp_device,
                                VDP_FUNC_ID_VIDEO_MIXER_DESTROY,
                                (void **)&vdp_video_mixer_destroy
                                );
  CheckStatus(vdp_st, __LINE__);

  vdp_st = vdp_get_proc_address(
                                vdp_device,
                                VDP_FUNC_ID_VIDEO_MIXER_RENDER,
                                (void **)&vdp_video_mixer_render
                                );
  CheckStatus(vdp_st, __LINE__);

  vdp_st = vdp_get_proc_address(
                                vdp_device,
                                VDP_FUNC_ID_GENERATE_CSC_MATRIX,
                                (void **)&vdp_generate_csc_matrix
                                );
  CheckStatus(vdp_st, __LINE__);

  vdp_st = vdp_get_proc_address(
                                vdp_device,
                                VDP_FUNC_ID_VIDEO_MIXER_SET_ATTRIBUTE_VALUES,
                                (void **)&vdp_video_mixer_set_attribute_values
                                );
  CheckStatus(vdp_st, __LINE__);

  vdp_st = vdp_get_proc_address(
                                vdp_device,
                                VDP_FUNC_ID_VIDEO_MIXER_QUERY_PARAMETER_SUPPORT,
                                (void **)&vdp_video_mixer_query_parameter_support
                                );
  CheckStatus(vdp_st, __LINE__);

  vdp_st = vdp_get_proc_address(
                                vdp_device,
                                VDP_FUNC_ID_VIDEO_MIXER_QUERY_FEATURE_SUPPORT,
                                (void **)&vdp_video_mixer_query_feature_support
                                );
  CheckStatus(vdp_st, __LINE__);  

  vdp_st = vdp_get_proc_address(
                                vdp_device,
                                VDP_FUNC_ID_PRESENTATION_QUEUE_TARGET_DESTROY,
                                (void **)&vdp_presentation_queue_target_destroy
                                );
  CheckStatus(vdp_st, __LINE__);

  vdp_st = vdp_get_proc_address(
                                vdp_device,
                                VDP_FUNC_ID_PRESENTATION_QUEUE_CREATE,
                                (void **)&vdp_presentation_queue_create
                                );
  CheckStatus(vdp_st, __LINE__);

  vdp_st = vdp_get_proc_address(
                                vdp_device,
                                VDP_FUNC_ID_PRESENTATION_QUEUE_DESTROY,
                                (void **)&vdp_presentation_queue_destroy
                                );
  CheckStatus(vdp_st, __LINE__);

  vdp_st = vdp_get_proc_address(
                                vdp_device,
                                VDP_FUNC_ID_PRESENTATION_QUEUE_DISPLAY,
                                (void **)&vdp_presentation_queue_display
                                );
  CheckStatus(vdp_st, __LINE__);

  vdp_st = vdp_get_proc_address(
                                vdp_device,
                                VDP_FUNC_ID_PRESENTATION_QUEUE_BLOCK_UNTIL_SURFACE_IDLE,
                                (void **)&vdp_presentation_queue_block_until_surface_idle
                                );
  CheckStatus(vdp_st, __LINE__);

  vdp_st = vdp_get_proc_address(
                                vdp_device,
                                VDP_FUNC_ID_PRESENTATION_QUEUE_TARGET_CREATE_X11,
                                (void **)&vdp_presentation_queue_target_create_x11
                                );
  CheckStatus(vdp_st, __LINE__);

  vdp_st = vdp_get_proc_address(
                                vdp_device,
                                VDP_FUNC_ID_DECODER_CREATE,
                                (void **)&vdp_decoder_create
                                );
  CheckStatus(vdp_st, __LINE__);

  vdp_st = vdp_get_proc_address(
                                vdp_device,
                                VDP_FUNC_ID_DECODER_DESTROY,
                                (void **)&vdp_decoder_destroy
                                );
  CheckStatus(vdp_st, __LINE__);

  vdp_st = vdp_get_proc_address(
                                vdp_device,
                                VDP_FUNC_ID_DECODER_RENDER,
                                (void **)&vdp_decoder_render
                                );
  CheckStatus(vdp_st, __LINE__);

  vdp_st = vdp_get_proc_address(
                                vdp_device,
                                VDP_FUNC_ID_DECODER_QUERY_CAPABILITIES,
                                (void **)&vdp_decoder_query_caps
                                );
  CheckStatus(vdp_st, __LINE__);

  vdp_st = vdp_get_proc_address(
                                vdp_device,
                                VDP_FUNC_ID_PRESENTATION_QUEUE_QUERY_SURFACE_STATUS,
                                (void **)&vdp_presentation_queue_query_surface_status
                                );
  CheckStatus(vdp_st, __LINE__);

  vdp_st = vdp_get_proc_address(
                                vdp_device,
                                VDP_FUNC_ID_PRESENTATION_QUEUE_GET_TIME,
                                (void **)&vdp_presentation_queue_get_time
                                );
  CheckStatus(vdp_st, __LINE__);

  vdp_st = vdp_get_proc_address(
                                vdp_device,
                                VDP_FUNC_ID_OUTPUT_SURFACE_RENDER_OUTPUT_SURFACE,
                                (void **)&vdp_output_surface_render_output_surface
                                );
  CheckStatus(vdp_st, __LINE__);

  vdp_st = vdp_get_proc_address(
                                vdp_device,
                                VDP_FUNC_ID_OUTPUT_SURFACE_PUT_BITS_INDEXED,
                                (void **)&vdp_output_surface_put_bits_indexed
                                );
  CheckStatus(vdp_st, __LINE__);

  vdp_st = vdp_get_proc_address(
                                vdp_device,
                                VDP_FUNC_ID_PREEMPTION_CALLBACK_REGISTER,
                                (void **)&vdp_preemption_callback_register
                                );
  CheckStatus(vdp_st, __LINE__);

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
  if (vdp_device == VDP_INVALID_HANDLE || !vdpauConfigured) return;

  CLog::Log(LOGNOTICE, " (VDPAU) %s", __FUNCTION__);

  VdpStatus vdp_st;

  vdp_st = vdp_decoder_destroy(decoder);
  CheckStatus(vdp_st, __LINE__);
  decoder = VDP_INVALID_HANDLE;

  vdp_st = vdp_presentation_queue_destroy(vdp_flip_queue);
  CheckStatus(vdp_st, __LINE__);
  vdp_flip_queue = VDP_INVALID_HANDLE;

  vdp_st = vdp_presentation_queue_target_destroy(vdp_flip_target);
  CheckStatus(vdp_st, __LINE__);
  vdp_flip_target = VDP_INVALID_HANDLE;

  for (int i = 0; i < totalAvailableOutputSurfaces; i++) 
  {
    vdp_st = vdp_output_surface_destroy(outputSurfaces[i]);
    CheckStatus(vdp_st, __LINE__);
    outputSurfaces[i] = VDP_INVALID_HANDLE;
  }

  vdp_st = vdp_video_mixer_destroy(videoMixer);
  CheckStatus(vdp_st, __LINE__);
  videoMixer = VDP_INVALID_HANDLE;

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
    case PIX_FMT_VDPAU_MPEG4:
      vdp_decoder_profile = VDP_DECODER_PROFILE_MPEG4_PART2_ASP;
      vdp_chroma_type     = VDP_CHROMA_TYPE_420;
    default:
      vdp_decoder_profile = 0;
      vdp_chroma_type     = 0;
  }
}


int CVDPAU::ConfigVDPAU(AVCodecContext* avctx, int ref_frames)
{
  FiniVDPAUOutput();

  VdpStatus vdp_st;
  VdpDecoderProfile vdp_decoder_profile;
  vid_width = avctx->width;
  vid_height = avctx->height;

  past[1] = past[0] = current = future = VDP_INVALID_HANDLE;
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
  CheckStatus(vdp_st, __LINE__);

  vdp_st = vdp_presentation_queue_target_create_x11(vdp_device,
                                                    m_Pixmap, //x_window,
                                                    &vdp_flip_target);
  CheckStatus(vdp_st, __LINE__);

  vdp_st = vdp_presentation_queue_create(vdp_device,
                                         vdp_flip_target,
                                         &vdp_flip_queue);
  CheckStatus(vdp_st, __LINE__);

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
    CheckStatus(vdp_st, __LINE__);

    if (vdp_st != VDP_STATUS_OK)
      break;

    totalAvailableOutputSurfaces++;
  }
  CLog::Log(LOGNOTICE, " (VDPAU) Total Output Surfaces Available: %i of a max (tmp: %i const: %i)", 
                       totalAvailableOutputSurfaces,
                       tmpMaxOutputSurfaces,
                       NUM_OUTPUT_SURFACES);

  surfaceNum = presentSurfaceNum = 0;
  outputSurface = outputSurfaces[surfaceNum];

  vdpauConfigured = true;
  return 0;
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
}

enum PixelFormat CVDPAU::FFGetFormat(struct AVCodecContext * avctx,
                                                     const PixelFormat * fmt)
{
  //CLog::Log(LOGNOTICE," (VDPAU) %s",__FUNCTION__);
  avctx->get_buffer      = FFGetBuffer;
  avctx->release_buffer  = FFReleaseBuffer;
  avctx->draw_horiz_band = FFDrawSlice;
  avctx->slice_flags=SLICE_FLAG_CODED_ORDER|SLICE_FLAG_ALLOW_FIELD;
  return fmt[0];
}


int CVDPAU::FFGetBuffer(AVCodecContext *avctx, AVFrame *pic)
{
  //CLog::Log(LOGNOTICE,"%s",__FUNCTION__);
  CDVDVideoCodecFFmpeg* ctx        = (CDVDVideoCodecFFmpeg*)avctx->opaque;
  CVDPAU*               vdp        = ctx->GetContextVDPAU();
  struct pictureAge*    pA         = &vdp->picAge;
  
  vdpau_render_state * render = NULL;

  vdp->CheckRecover(false);

  // find unused surface
  for(unsigned int i = 0; i < vdp->m_videoSurfaces.size(); i++)
  {
    if(!(vdp->m_videoSurfaces[i]->state & FF_VDPAU_STATE_USED_FOR_REFERENCE))
    {
      render = vdp->m_videoSurfaces[i];
      render->state = 0;
      break;
    }
  }

  int tries = 0;
  VdpStatus vdp_st = VDP_STATUS_ERROR;
  if (render == NULL)
  {
    while(vdp_st != VDP_STATUS_OK && tries < NUM_VIDEO_SURFACES_MAX_TRIES)
    {
      tries++;
      CLog::Log(LOGNOTICE, " (VDPAU) Didnt find a Video Surface Available (Total: %i). Creating a new one. TRY #%i",
                          vdp->m_videoSurfaces.size(), tries);
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
      if (vdp_st == VDP_STATUS_OK)
        vdp->m_videoSurfaces.push_back(render);
    }
    if (vdp_st != VDP_STATUS_OK)
      CLog::Log(LOGNOTICE, " (VDPAU) No Video surface available could be created.... continuing with an invalid handler");
  }

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

  assert(render != NULL);
  render->state |= FF_VDPAU_STATE_USED_FOR_REFERENCE;
  pic->reordered_opaque= avctx->reordered_opaque;
  return 0;
}

void CVDPAU::FFReleaseBuffer(AVCodecContext *avctx, AVFrame *pic)
{
  //CLog::Log(LOGNOTICE,"%s",__FUNCTION__);
  vdpau_render_state * render;
  int i;

  render=(vdpau_render_state*)pic->data[0];
  assert(render != NULL);

  render->state &= ~FF_VDPAU_STATE_USED_FOR_REFERENCE;
  for(i=0; i<4; i++)
    pic->data[i]= NULL;  
}


void CVDPAU::FFDrawSlice(struct AVCodecContext *s,
                                           const AVFrame *src, int offset[4],
                                           int y, int type, int height)
{
  //CLog::Log(LOGNOTICE,"%s",__FUNCTION__);
  CDVDVideoCodecFFmpeg* ctx = (CDVDVideoCodecFFmpeg*)s->opaque;
  CVDPAU*               vdp = ctx->GetContextVDPAU();

  vdp->CheckRecover(false);

  assert(src->linesize[0]==0 && src->linesize[1]==0 && src->linesize[2]==0);
  assert(offset[0]==0 && offset[1]==0 && offset[2]==0);

  VdpStatus vdp_st;
  vdpau_render_state * render;

  render = (vdpau_render_state*)src->data[0];
  assert( render != NULL );

  uint32_t max_refs = 0;
  if(s->pix_fmt == PIX_FMT_VDPAU_H264)
    max_refs = render->info.h264.num_ref_frames;

  if(vdp->decoder == VDP_INVALID_HANDLE 
  || vdp->vdpauConfigured == false
  || vdp->max_references < max_refs)
    vdp->ConfigVDPAU(s, max_refs);

  vdp_st = vdp->vdp_decoder_render(vdp->decoder,
                                   render->surface,
                                   (VdpPictureInfo const *)&(render->info),
                                   render->bitstream_buffers_used,
                                   render->bitstream_buffers);
  vdp->CheckStatus(vdp_st, __LINE__);
}

void CVDPAU::PrePresent(AVCodecContext *avctx, AVFrame *pFrame)
{
  //CLog::Log(LOGNOTICE,"%s",__FUNCTION__);
  vdpau_render_state * render = (vdpau_render_state*)pFrame->data[0];
  VdpVideoMixerPictureStructure structure;
  VdpStatus vdp_st;
  VdpTime time;

  if (!vdpauConfigured)
    return;

  outputSurface = outputSurfaces[surfaceNum];

  CheckFeatures();

  if (interlaced && tmpDeint)
    structure = pFrame->top_field_first ? VDP_VIDEO_MIXER_PICTURE_STRUCTURE_TOP_FIELD :
                                          VDP_VIDEO_MIXER_PICTURE_STRUCTURE_BOTTOM_FIELD;
  else structure = VDP_VIDEO_MIXER_PICTURE_STRUCTURE_FRAME; 

  past[0] = past[1];
  past[1] = current;
  current = render->surface;

  if (( (int)outRectVid.x1 != OutWidth ) ||
      ( (int)outRectVid.y1 != OutHeight ))
  {
    outRectVid.x0 = 0;
    outRectVid.y0 = 0;
    outRectVid.x1 = OutWidth;
    outRectVid.y1 = OutHeight;
  }
  
  if (current == VDP_INVALID_HANDLE)
    current = render->surface;
  
  vdp_st = vdp_presentation_queue_block_until_surface_idle(vdp_flip_queue,outputSurface,&time);

  vdp_st = vdp_video_mixer_render(videoMixer,
                                  VDP_INVALID_HANDLE,
                                  0,
                                  structure,
                                  2,
                                  past,
                                  current,
                                  0,
                                  NULL,
                                  NULL,
                                  outputSurface,
                                  &(outRectVid),
                                  &(outRectVid),
                                  0,
                                  NULL);
  CheckStatus(vdp_st, __LINE__);
  
  surfaceNum++;
  if (surfaceNum >= totalAvailableOutputSurfaces) surfaceNum = 0;
}

void CVDPAU::Present()
{
  //CLog::Log(LOGNOTICE,"%s",__FUNCTION__);
  VdpStatus vdp_st;

  vdp_st = vdp_presentation_queue_display(vdp_flip_queue,
                                          outputSurface,
                                          0,
                                          0,
                                          0);
  CheckStatus(vdp_st, __LINE__);
}

void CVDPAU::VDPPreemptionCallbackFunction(VdpDevice device, void* context)
{
  CLog::Log(LOGERROR,"VDPAU Device Preempted - attempting recovery");
  CVDPAU* pCtx = (CVDPAU*)context;
  if (!pCtx->VDPAUSwitching)
    pCtx->recover = true;
}

void CVDPAU::CheckStatus(VdpStatus vdp_st, int line)
{
  if (vdp_st != VDP_STATUS_OK)
  {
    CLog::Log(LOGERROR, " (VDPAU) Error: %s(%d) at %s:%d\n", vdp_get_error_string(vdp_st), vdp_st, __FILE__, line);
    if (vdpauConfigured && !VDPAUSwitching) 
      recover = true;
  }
  if (vdp_st == VDP_STATUS_HANDLE_DEVICE_MISMATCH)
    recover = true;
}

#endif
