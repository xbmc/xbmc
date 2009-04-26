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

#include "stdafx.h"
#ifdef HAVE_LIBVDPAU
#include <dlfcn.h>
#include "VDPAU.h"
#include "Surface.h"
using namespace Surface;
#include "vdpau.h"
#include "TextureManager.h"                         //DAVID-CHECKNEEDED
#include "cores/VideoRenderers/RenderManager.h"
#include "DVDVideoCodecFFmpeg.h"
#include "Settings.h"
#define ARSIZE(x) (sizeof(x) / sizeof((x)[0]))

CVDPAU*          g_VDPAU;

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

CVDPAU::CVDPAU(int width, int height)
{
  vdp_device = NULL;
  dl_handle  = NULL;
  dl_handle  = dlopen("libvdpau.so.1", RTLD_LAZY);
  if (!dl_handle) return;
  surfaceNum      = presentSurfaceNum = 0;
  picAge.b_age    = picAge.ip_age[0] = picAge.ip_age[1] = 256*256*256*64;
  vdpauConfigured = false;
  CSingleLock lock(g_graphicsContext);
  m_Surface = new CSurface(g_graphicsContext.getScreenSurface());
  m_Surface->MakePixmap(width,height);
  m_Display = g_graphicsContext.getScreenSurface()->GetDisplay();
  InitVDPAUProcs();
  recover = VDPAURecovered = false;
  totalAvailableOutputSurfaces = outputSurface = presentSurface = 0;
  vid_width = vid_height = outWidth = outHeight = 0;
  memset(&outRect, 0, sizeof(VdpRect));
  memset(&outRectVid, 0, sizeof(VdpRect));

  tmpBrightness  = 0;
  tmpContrast    = 0;
  interlaced     = false;
  VDPAUSwitching = false;
  max_references = 0;
 
  if (vdp_device)
    InitCSCMatrix();

  for (int i = 0; i < NUM_OUTPUT_SURFACES; i++)
    outputSurfaces[i] = VDP_INVALID_HANDLE;

  videoMixer = VDP_INVALID_HANDLE;
}

CVDPAU::~CVDPAU()
{
  CLog::Log(LOGNOTICE, " (VDPAU) %s", __FUNCTION__);
  m_Surface->ReleasePixmap();
  FiniVDPAUOutput();
  FiniVDPAUProcs();
  if (m_Surface)
  {
    CLog::Log(LOGNOTICE,"Deleting m_Surface in CVDPAU");
    delete m_Surface;
    m_Surface = NULL;
  }
  if (dl_handle)
  {
    dlclose(dl_handle);
    dl_handle = NULL;
  }
}

void CVDPAU::CheckRecover(bool force)
{
  if (recover || force)
  {
    CLog::Log(LOGNOTICE,"Attempting recovery");

    VDPAUSwitching = true;
    FiniVDPAUOutput();
    FiniVDPAUProcs();

    CSingleLock lock(g_graphicsContext);
    XLockDisplay(m_Display);
    InitVDPAUProcs();
    XUnlockDisplay(m_Display);

    VDPAURecovered = true;
    VDPAUSwitching = false;
    recover = false;
  }
}

bool CVDPAU::IsVDPAUFormat(PixelFormat format)
{
  if ((format >= PIX_FMT_VDPAU_H264) && (format <= PIX_FMT_VDPAU_VC1)) return true;
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

    int featuresCount = 0;
    VdpVideoMixerFeature features[5];

    features[featuresCount++] = VDP_VIDEO_MIXER_FEATURE_NOISE_REDUCTION;
    features[featuresCount++] = VDP_VIDEO_MIXER_FEATURE_SHARPNESS;
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
  }

  if (tmpBrightness != g_stSettings.m_currentVideoSettings.m_Brightness ||
      tmpContrast   != g_stSettings.m_currentVideoSettings.m_Contrast)
  {
    SetColor();
    tmpBrightness = g_stSettings.m_currentVideoSettings.m_Brightness;
    tmpContrast = g_stSettings.m_currentVideoSettings.m_Contrast;
  }
  if (tmpNoiseReduction != g_stSettings.m_currentVideoSettings.m_NoiseReduction)
  {
    tmpNoiseReduction = g_stSettings.m_currentVideoSettings.m_NoiseReduction;
    SetNoiseReduction();
  }
  if (tmpSharpness != g_stSettings.m_currentVideoSettings.m_Sharpness)
  {
    tmpSharpness = g_stSettings.m_currentVideoSettings.m_Sharpness;
    SetSharpness();
  }

  if (interlaced && tmpDeint && tmpDeint != g_stSettings.m_currentVideoSettings.m_InterlaceMethod)
  {
    tmpDeint = g_stSettings.m_currentVideoSettings.m_InterlaceMethod;
    SetDeinterlacing();
  }
}

void CVDPAU::SetColor()
{
  VdpStatus vdp_st;

  if (tmpBrightness != g_stSettings.m_currentVideoSettings.m_Brightness)
    m_Procamp.brightness = (float)((g_stSettings.m_currentVideoSettings.m_Brightness)-50) / 100;
  if (tmpContrast != g_stSettings.m_currentVideoSettings.m_Contrast)
    m_Procamp.contrast = (float)((g_stSettings.m_currentVideoSettings.m_Contrast)+50) / 100;
  vdp_st = vdp_generate_csc_matrix(&m_Procamp, 
                                   VDP_COLOR_STANDARD_ITUR_BT_709,
                                   &m_CSCMatrix);
  VdpVideoMixerAttribute attributes[] = { VDP_VIDEO_MIXER_ATTRIBUTE_CSC_MATRIX };
  void const * pm_CSCMatix[] = { &m_CSCMatrix };
  vdp_st = vdp_video_mixer_set_attribute_values(videoMixer, ARSIZE(attributes), attributes, pm_CSCMatix);
  CheckStatus(vdp_st, __LINE__);
}

void CVDPAU::SetNoiseReduction()
{
  VdpVideoMixerFeature feature[] = { VDP_VIDEO_MIXER_FEATURE_NOISE_REDUCTION };
  VdpVideoMixerAttribute attributes[] = { VDP_VIDEO_MIXER_ATTRIBUTE_NOISE_REDUCTION_LEVEL };
  VdpStatus vdp_st;

  if (!g_stSettings.m_currentVideoSettings.m_NoiseReduction) 
  {
    VdpBool enabled[]= {0};
    vdp_st = vdp_video_mixer_set_feature_enables(videoMixer, ARSIZE(feature), feature, enabled);
    CheckStatus(vdp_st, __LINE__);
    return;
  }
  VdpBool enabled[]={1};
  vdp_st = vdp_video_mixer_set_feature_enables(videoMixer, ARSIZE(feature), feature, enabled);
  CheckStatus(vdp_st, __LINE__);
  void* nr[] = { &g_stSettings.m_currentVideoSettings.m_NoiseReduction };
  CLog::Log(LOGNOTICE,"Setting Noise Reduction to %f",g_stSettings.m_currentVideoSettings.m_NoiseReduction);
  vdp_st = vdp_video_mixer_set_attribute_values(videoMixer, ARSIZE(attributes), attributes, nr);
  CheckStatus(vdp_st, __LINE__);
}

void CVDPAU::SetSharpness()
{
  VdpVideoMixerFeature feature[] = { VDP_VIDEO_MIXER_FEATURE_SHARPNESS };
  VdpVideoMixerAttribute attributes[] = { VDP_VIDEO_MIXER_ATTRIBUTE_SHARPNESS_LEVEL };
  VdpStatus vdp_st;

  if (!g_stSettings.m_currentVideoSettings.m_Sharpness) 
  {
    VdpBool enabled[]={0};
    vdp_st = vdp_video_mixer_set_feature_enables(videoMixer, ARSIZE(feature), feature, enabled);
    CheckStatus(vdp_st, __LINE__);
    return;
  }
  VdpBool enabled[]={1};
  vdp_st = vdp_video_mixer_set_feature_enables(videoMixer, ARSIZE(feature), feature, enabled);
  CheckStatus(vdp_st, __LINE__);
  void* sh[] = { &g_stSettings.m_currentVideoSettings.m_Sharpness };
  CLog::Log(LOGNOTICE,"Setting Sharpness to %f",g_stSettings.m_currentVideoSettings.m_Sharpness);
  vdp_st = vdp_video_mixer_set_attribute_values(videoMixer, ARSIZE(attributes), attributes, sh);
  CheckStatus(vdp_st, __LINE__);
}


void CVDPAU::SetDeinterlacing()
{
  VdpVideoMixerFeature feature[] = { VDP_VIDEO_MIXER_FEATURE_DEINTERLACE_TEMPORAL,
                                     VDP_VIDEO_MIXER_FEATURE_DEINTERLACE_TEMPORAL_SPATIAL,
                                     VDP_VIDEO_MIXER_FEATURE_INVERSE_TELECINE };

  VdpStatus vdp_st;

  if (!g_stSettings.m_currentVideoSettings.m_InterlaceMethod) 
  {
    VdpBool enabled[]={0,0,0};
    vdp_st = vdp_video_mixer_set_feature_enables(videoMixer, ARSIZE(feature), feature, enabled);
    CheckStatus(vdp_st, __LINE__);
  }
  else if (g_stSettings.m_currentVideoSettings.m_InterlaceMethod == VS_INTERLACEMETHOD_AUTO) 
  {
    VdpBool enabled[]={1,0,0};
    vdp_st = vdp_video_mixer_set_feature_enables(videoMixer, ARSIZE(feature), feature, enabled);
    CheckStatus(vdp_st, __LINE__);
  }
  else if (g_stSettings.m_currentVideoSettings.m_InterlaceMethod == VS_INTERLACEMETHOD_RENDER_BLEND) 
  {
    VdpBool enabled[]={1,1,0};
    vdp_st = vdp_video_mixer_set_feature_enables(videoMixer, ARSIZE(feature), feature, enabled);
    CheckStatus(vdp_st, __LINE__);
  }
  else if (g_stSettings.m_currentVideoSettings.m_InterlaceMethod == VS_INTERLACEMETHOD_INVERSE_TELECINE) 
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
    vdp_device = NULL;
    return;
  }

  int mScreen = DefaultScreen(m_Display);
  VdpStatus vdp_st;

  // Create Device
  vdp_st = dl_vdp_device_create_x11(m_Display, //x_display,
                                 mScreen, //x_screen,
                                 &vdp_device,
                                 &vdp_get_proc_address);
  CheckStatus(vdp_st, __LINE__);
  if (vdp_st != VDP_STATUS_OK) 
  {
    CLog::Log(LOGERROR,"(VDPAU) - Unable to create X11 device in %s",__FUNCTION__);
    vdp_device = NULL;
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

VdpStatus CVDPAU::FiniVDPAUProcs()
{
  VdpStatus vdp_st = VDP_STATUS_ERROR;
  if (!vdp_device) return VDP_STATUS_OK;

  vdp_st = vdp_device_destroy(vdp_device);
  CheckStatus(vdp_st, __LINE__);
  vdpauConfigured = false;
  return VDP_STATUS_OK;
}

void CVDPAU::InitVDPAUOutput()
{
  VdpStatus vdp_st;
  vdp_st = vdp_presentation_queue_target_create_x11(vdp_device,
                                                    m_Surface->GetXPixmap(), //x_window,
                                                    &vdp_flip_target);
  CheckStatus(vdp_st, __LINE__);

  vdp_st = vdp_presentation_queue_create(vdp_device,
                                         vdp_flip_target,
                                         &vdp_flip_queue);
  CheckStatus(vdp_st, __LINE__);
}

void CVDPAU::InitCSCMatrix()
{
  VdpStatus vdp_st;
  m_Procamp.struct_version = VDP_PROCAMP_VERSION;
  m_Procamp.brightness     = 0.0;
  m_Procamp.contrast       = 1.0;
  m_Procamp.saturation     = 1.0;
  m_Procamp.hue            = 0;
  vdp_st = vdp_generate_csc_matrix(&m_Procamp,
                                   VDP_COLOR_STANDARD_ITUR_BT_709,
                                   &m_CSCMatrix);
  CheckStatus(vdp_st, __LINE__);
}

VdpStatus CVDPAU::FiniVDPAUOutput()
{
  CLog::Log(LOGNOTICE, " (VDPAU) %s", __FUNCTION__);
  VdpStatus vdp_st = VDP_STATUS_ERROR;

  if (!vdp_device) return VDP_STATUS_OK;
  if (!vdpauConfigured) return VDP_STATUS_OK;

  vdp_st = vdp_decoder_destroy(decoder);
  CheckStatus(vdp_st, __LINE__);

  vdp_st = vdp_presentation_queue_destroy(vdp_flip_queue);
  CheckStatus(vdp_st, __LINE__);

  vdp_st = vdp_presentation_queue_target_destroy(vdp_flip_target);
  CheckStatus(vdp_st, __LINE__);

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

  return VDP_STATUS_OK;
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
    default:
      vdp_decoder_profile = 0;
      vdp_chroma_type     = 0;
  }
}


int CVDPAU::ConfigVDPAU(AVCodecContext* avctx, int ref_frames)
{
  if (vdpauConfigured || !avctx) return 1;
  VdpStatus vdp_st;
  VdpDecoderProfile vdp_decoder_profile;
  vid_width = avctx->width;
  vid_height = avctx->height;

  past[1] = past[0] = current = future = VDP_INVALID_HANDLE;
  CLog::Log(LOGNOTICE, " (VDPAU) screenWidth:%i vidWidth:%i",g_graphicsContext.GetWidth(),vid_width);
  ReadFormatOf(avctx->pix_fmt, vdp_decoder_profile, vdp_chroma_type);

  if(avctx->pix_fmt == PIX_FMT_VDPAU_H264)
  {
     max_references = ref_frames;
     if (max_references > 16) max_references = 16;
     if (max_references < 5)  max_references = 5;
  }
  else
    max_references = 2;

  if (IsVDPAUFormat(avctx->pix_fmt)) 
  {
    vdp_st = vdp_decoder_create(vdp_device,
                                vdp_decoder_profile,
                                vid_width,
                                vid_height,
                                max_references,
                                &decoder);
    CheckStatus(vdp_st, __LINE__);
  }

  InitVDPAUOutput();

  totalAvailableOutputSurfaces = 0;

  int tmpMaxOutputSurfaces = NUM_OUTPUT_SURFACES;
  if (vid_width == FULLHD_WIDTH)
    tmpMaxOutputSurfaces = NUM_OUTPUT_SURFACES_FOR_FULLHD; 

  // Creation of outputSurfaces
  for (int i = 0; i < NUM_OUTPUT_SURFACES && i < tmpMaxOutputSurfaces; i++) 
  {
    vdp_st = vdp_output_surface_create(vdp_device,
                                       VDP_RGBA_FORMAT_B8G8R8A8,
                                       vid_width,
                                       vid_height,
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
  assert(totalAvailableOutputSurfaces > 0);

  surfaceNum = presentSurfaceNum = 0;
  outputSurface = outputSurfaces[surfaceNum];

  SpewHardwareAvailable();
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
  //pSingleton->CheckRecover();
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

  // make sure device is recovered
  vdp->CheckRecover();

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
  {
    CExclusiveLock lock(g_renderManager.GetSection());
    vdp->FiniVDPAUOutput();
    vdp->ConfigVDPAU(s, max_refs);
  }

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

  if (!vdpauConfigured)
    return;

  outputSurface = outputSurfaces[surfaceNum];
  interlaced = pFrame->interlaced_frame;

  CheckFeatures();
  if (interlaced && tmpDeint)
  {
    past[1] = past[0];
    past[0] = current;
    current = future;
    future = render->surface;
  }
  else
    current = render->surface;

  if (interlaced && tmpDeint)
    structure = pFrame->top_field_first ? VDP_VIDEO_MIXER_PICTURE_STRUCTURE_TOP_FIELD :
                                          VDP_VIDEO_MIXER_PICTURE_STRUCTURE_BOTTOM_FIELD;
  else structure = VDP_VIDEO_MIXER_PICTURE_STRUCTURE_FRAME; 

  if (interlaced && tmpDeint)
  {
    past[1] = past[0];
    past[0] = current;
    current = future;
    future = render->surface;
  }

  if (( outRectVid.x1 != vid_width ) ||
      ( outRectVid.y1 != vid_height ))
  {
    outRectVid.x0 = 0;
    outRectVid.y0 = 0;
    outRectVid.x1 = vid_width;
    outRectVid.y1 = vid_height;

    CSingleLock lock(g_graphicsContext);
    if(g_graphicsContext.GetViewWindow().right < (long)vid_width)
      outWidth = vid_width;
    else
      outWidth = g_graphicsContext.GetViewWindow().right;
    if(g_graphicsContext.GetViewWindow().bottom < (long)vid_height)
      outHeight = vid_height;
    else
      outHeight = g_graphicsContext.GetViewWindow().bottom;

    outRect.x0 = 0;
    outRect.y0 = 0;
    outRect.x1 = outWidth;
    outRect.y1 = outHeight;
  }
  //CLog::Log(LOGNOTICE,"surfaceNum %i",surfaceNum);
//  vdp_st = vdp_presentation_queue_block_until_surface_idle(vdp_flip_queue,outputSurface,&time);
//  CheckStatus(vdp_st, __LINE__);
  vdp_st = vdp_video_mixer_render(videoMixer,
                                  VDP_INVALID_HANDLE,
                                  0,
                                  structure,
                                  (interlaced && tmpDeint)? ARSIZE(past) : 0, //2,
                                  (interlaced && tmpDeint)? past : NULL, //past,
                                  current,
                                  (interlaced && tmpDeint)? 1 : 0, //1,
                                  (interlaced && tmpDeint)? &(future) : NULL, //&(future),
                                  NULL,
                                  outputSurface,
                                  &(outRect),
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
  //CLog::Log(LOGNOTICE,"presentSurfaceNum %i",presentSurfaceNum);
  VdpStatus vdp_st;
  presentSurface = outputSurfaces[presentSurfaceNum];
  vdp_st = vdp_presentation_queue_display(vdp_flip_queue,
                                          outputSurface,
                                          0,
                                          0,
                                          0);
  CheckStatus(vdp_st, __LINE__);
  presentSurfaceNum++;
  if (presentSurfaceNum >= totalAvailableOutputSurfaces) presentSurfaceNum = 0;
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
  }
  if (vdp_st == VDP_STATUS_HANDLE_DEVICE_MISMATCH)
    CheckRecover(true);
}

#endif
