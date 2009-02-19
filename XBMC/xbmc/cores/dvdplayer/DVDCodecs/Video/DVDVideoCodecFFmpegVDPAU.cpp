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

#include "DVDVideoCodecFFmpegVDPAU.h"
#include "Surface.h"
using namespace Surface;
extern bool usingVDPAU;
#include "vdpau.h"
#include "TextureManager.h"                         //DAVID-CHECKNEEDED
#include "cores/VideoRenderers/RenderManager.h"
#include "DVDVideoCodecFFmpeg.h"
#include "Settings.h"

#define ARSIZE(x) (sizeof(x) / sizeof((x)[0]))

static CDVDVideoCodecVDPAU *pSingleton = NULL;

Desc decoder_profiles[] = {
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
const size_t decoder_profile_count = sizeof(decoder_profiles)/sizeof(Desc);

CDVDVideoCodecVDPAU::CDVDVideoCodecVDPAU(Display* display, Pixmap px)
{
  // Point the singleton to myself so we can use it to access our
  // instance variables from our static callbacks
  pSingleton = this;
  surfaceNum = 0;
  m_Pixmap = px;
  picAge.b_age = picAge.ip_age[0] = picAge.ip_age[1] = 256*256*256*64;
  m_Display=display;
  vdpauConfigured = false;
  initVDPAUProcs();
  initVDPAUOutput();
  vdp_preemption_callback_register(vdp_device,
                                   &vdpPreemptionCallbackFunction,
                                   (void*)this);
  recover = false;
  outputSurface = 0;
  noiseReduction = g_stSettings.m_currentVideoSettings.m_NoiseReduction;
  sharpness = g_stSettings.m_currentVideoSettings.m_Sharpness;
  inverseTelecine = g_stSettings.m_currentVideoSettings.m_InverseTelecine;
  lastFrameTime = nextFrameTime = 0;
  interlaced = false;
  m_avctx = NULL;
}

CDVDVideoCodecVDPAU::~CDVDVideoCodecVDPAU()
{
  finiVDPAUOutput();
  finiVDPAUProcs();
  if (videoSurfaces)
    free(videoSurfaces);
  pSingleton = NULL;
}

void CDVDVideoCodecVDPAU::checkRecover()
{
  if (recover) {
    XLockDisplay( g_graphicsContext.getScreenSurface()->GetDisplay() );
    CLog::Log(LOGNOTICE,"Attempting recovery");
    if (videoSurfaces)
      free(videoSurfaces);
    initVDPAUProcs();
    initVDPAUOutput();
    configVDPAU(m_avctx);
    vdp_preemption_callback_register(vdp_device,
                                     &vdpPreemptionCallbackFunction,
                                     (void*)this);
    recover = false;
    XUnlockDisplay( g_graphicsContext.getScreenSurface()->GetDisplay() );
  }
}

bool CDVDVideoCodecVDPAU::isVDPAUFormat(uint32_t format)
{
  if ((format >= PIX_FMT_VDPAU_H264) && (format <= PIX_FMT_VDPAU_VC1)) return true;
  else return false;
}

void CDVDVideoCodecVDPAU::checkFeatures()
{
  if (tmpInverseTelecine != g_stSettings.m_currentVideoSettings.m_InverseTelecine) {
    tmpInverseTelecine = g_stSettings.m_currentVideoSettings.m_InverseTelecine;
    setTelecine();
  }
  if (tmpNoiseReduction != noiseReduction) {
    tmpNoiseReduction = noiseReduction;
    setNoiseReduction();
  }
  if (tmpSharpness != g_stSettings.m_currentVideoSettings.m_Sharpness) {
    tmpSharpness = g_stSettings.m_currentVideoSettings.m_Sharpness;
    setSharpness();
  }
  if (tmpDeint != g_stSettings.m_currentVideoSettings.m_InterlaceMethod) {
    tmpDeint = g_stSettings.m_currentVideoSettings.m_InterlaceMethod;
    setDeinterlacing();
  }
}

void CDVDVideoCodecVDPAU::setTelecine()
{
  VdpBool enabled[1];
  VdpVideoMixerFeature feature = VDP_VIDEO_MIXER_FEATURE_INVERSE_TELECINE;
  VdpStatus vdp_st;
  
  if (interlaced && g_stSettings.m_currentVideoSettings.m_InverseTelecine)
    enabled[0] = true;
  else 
    enabled[0] = false;
  vdp_st = vdp_video_mixer_set_feature_enables(videoMixer, 1, &feature, enabled);
  CHECK_ST
}

void CDVDVideoCodecVDPAU::setNoiseReduction()
{
  VdpVideoMixerFeature feature[] = { VDP_VIDEO_MIXER_FEATURE_NOISE_REDUCTION };
  VdpVideoMixerAttribute attributes[] = { VDP_VIDEO_MIXER_ATTRIBUTE_NOISE_REDUCTION_LEVEL };
  VdpStatus vdp_st;
  
  if (!g_stSettings.m_currentVideoSettings.m_NoiseReduction) {
    VdpBool enabled[]= {0};
    vdp_st = vdp_video_mixer_set_feature_enables(videoMixer, 1, feature, enabled);
    CHECK_ST
    return;
  }
  VdpBool enabled[]={1};
  vdp_st = vdp_video_mixer_set_feature_enables(videoMixer, 1, feature, enabled);
  CHECK_ST
  void* nr[] = { &g_stSettings.m_currentVideoSettings.m_NoiseReduction };
  CLog::Log(LOGNOTICE,"Setting Sharpness to %f",g_stSettings.m_currentVideoSettings.m_NoiseReduction);
  vdp_st = vdp_video_mixer_set_attribute_values(videoMixer, 1, attributes, nr);
  CHECK_ST
}

void CDVDVideoCodecVDPAU::setSharpness()
{
  VdpVideoMixerFeature feature[] = { VDP_VIDEO_MIXER_FEATURE_SHARPNESS };
  VdpVideoMixerAttribute attributes[] = { VDP_VIDEO_MIXER_ATTRIBUTE_SHARPNESS_LEVEL };
  VdpStatus vdp_st;
  
  if (!g_stSettings.m_currentVideoSettings.m_Sharpness) {
    VdpBool enabled[]={0};
    vdp_st = vdp_video_mixer_set_feature_enables(videoMixer, 1, feature, enabled);
    CHECK_ST
    return;
  }
  VdpBool enabled[]={1};
  vdp_st = vdp_video_mixer_set_feature_enables(videoMixer, 1, feature, enabled);
  CHECK_ST
  void* sh[] = { &g_stSettings.m_currentVideoSettings.m_Sharpness };
  CLog::Log(LOGNOTICE,"Setting Sharpness to %f",g_stSettings.m_currentVideoSettings.m_Sharpness);
  vdp_st = vdp_video_mixer_set_attribute_values(videoMixer, 1, attributes, sh);
  CHECK_ST
}


void CDVDVideoCodecVDPAU::setDeinterlacing()
{
  VdpVideoMixerFeature feature[] = { VDP_VIDEO_MIXER_FEATURE_DEINTERLACE_TEMPORAL, 
                                     VDP_VIDEO_MIXER_FEATURE_DEINTERLACE_TEMPORAL_SPATIAL };
  VdpStatus vdp_st;
  
  if (!g_stSettings.m_currentVideoSettings.m_InterlaceMethod) {
    VdpBool enabled[]={0,0};
    vdp_st = vdp_video_mixer_set_feature_enables(videoMixer, 1, feature, enabled);
    CHECK_ST
    return;
  }
  else if (g_stSettings.m_currentVideoSettings.m_InterlaceMethod == 1) {
    VdpBool enabled[]={1,0};
    vdp_st = vdp_video_mixer_set_feature_enables(videoMixer, 1, feature, enabled);
    CHECK_ST
  }
  else if (g_stSettings.m_currentVideoSettings.m_InterlaceMethod == 2) {
    VdpBool enabled[]={1,1};
    vdp_st = vdp_video_mixer_set_feature_enables(videoMixer, 1, feature, enabled);
    CHECK_ST
  }    
}

void CDVDVideoCodecVDPAU::initVDPAUProcs()
{
  int mScreen = DefaultScreen(g_graphicsContext.getScreenSurface()->GetDisplay());
  VdpStatus vdp_st;
  
  // Create Device
  vdp_st = vdp_device_create_x11(g_graphicsContext.getScreenSurface()->GetDisplay(), //x_display,
                                 mScreen, //x_screen,
                                 &vdp_device,
                                 &vdp_get_proc_address);
  CHECK_ST
  
  vdp_st = vdp_get_proc_address(vdp_device,
                                VDP_FUNC_ID_DEVICE_DESTROY,
                                (void **)&vdp_device_destroy);
  CHECK_ST
  
  vdp_st = vdp_get_proc_address(vdp_device,
                                VDP_FUNC_ID_VIDEO_SURFACE_CREATE,
                                (void **)&vdp_video_surface_create);
  CHECK_ST
  
  vdp_st = vdp_get_proc_address(
                                vdp_device,
                                VDP_FUNC_ID_VIDEO_SURFACE_DESTROY,
                                (void **)&vdp_video_surface_destroy
                                );
  CHECK_ST
  
  vdp_st = vdp_get_proc_address(
                                vdp_device,
                                VDP_FUNC_ID_VIDEO_SURFACE_PUT_BITS_Y_CB_CR,
                                (void **)&vdp_video_surface_put_bits_y_cb_cr
                                );
  CHECK_ST
  
  vdp_st = vdp_get_proc_address(
                                vdp_device,
                                VDP_FUNC_ID_VIDEO_SURFACE_GET_BITS_Y_CB_CR,
                                (void **)&vdp_video_surface_get_bits_y_cb_cr
                                );
  CHECK_ST
  
  vdp_st = vdp_get_proc_address(
                                vdp_device,
                                VDP_FUNC_ID_OUTPUT_SURFACE_PUT_BITS_Y_CB_CR,
                                (void **)&vdp_output_surface_put_bits_y_cb_cr
                                );
  CHECK_ST
  
  vdp_st = vdp_get_proc_address(
                                vdp_device,
                                VDP_FUNC_ID_OUTPUT_SURFACE_PUT_BITS_NATIVE,
                                (void **)&vdp_output_surface_put_bits_native
                                );
  CHECK_ST
  
  vdp_st = vdp_get_proc_address(
                                vdp_device,
                                VDP_FUNC_ID_OUTPUT_SURFACE_CREATE,
                                (void **)&vdp_output_surface_create
                                );
  CHECK_ST
  
  vdp_st = vdp_get_proc_address(
                                vdp_device,
                                VDP_FUNC_ID_OUTPUT_SURFACE_DESTROY,
                                (void **)&vdp_output_surface_destroy
                                );
  CHECK_ST
  
  vdp_st = vdp_get_proc_address(
                                vdp_device,
                                VDP_FUNC_ID_OUTPUT_SURFACE_GET_BITS_NATIVE,
                                (void **)&vdp_output_surface_get_bits_native
                                );
  CHECK_ST
  
  vdp_st = vdp_get_proc_address(
                                vdp_device,
                                VDP_FUNC_ID_VIDEO_MIXER_CREATE,
                                (void **)&vdp_video_mixer_create
                                );
  CHECK_ST
  
  vdp_st = vdp_get_proc_address(
                                vdp_device,
                                VDP_FUNC_ID_VIDEO_MIXER_SET_FEATURE_ENABLES,
                                (void **)&vdp_video_mixer_set_feature_enables
                                );
  CHECK_ST
  
  vdp_st = vdp_get_proc_address(
                                vdp_device,
                                VDP_FUNC_ID_VIDEO_MIXER_DESTROY,
                                (void **)&vdp_video_mixer_destroy
                                );
  CHECK_ST
  
  vdp_st = vdp_get_proc_address(
                                vdp_device,
                                VDP_FUNC_ID_VIDEO_MIXER_RENDER,
                                (void **)&vdp_video_mixer_render
                                );
  CHECK_ST
  
  vdp_st = vdp_get_proc_address(
                                vdp_device,
                                VDP_FUNC_ID_GENERATE_CSC_MATRIX,
                                (void **)&vdp_generate_csc_matrix
                                );
  CHECK_ST
  
  vdp_st = vdp_get_proc_address(
                                vdp_device,
                                VDP_FUNC_ID_VIDEO_MIXER_SET_ATTRIBUTE_VALUES,
                                (void **)&vdp_video_mixer_set_attribute_values
                                );
  CHECK_ST
  
  vdp_st = vdp_get_proc_address(
                                vdp_device,
                                VDP_FUNC_ID_VIDEO_MIXER_QUERY_PARAMETER_SUPPORT,
                                (void **)&vdp_video_mixer_query_parameter_support
                                );
  CHECK_ST
  
  vdp_st = vdp_get_proc_address(
                                vdp_device,
                                VDP_FUNC_ID_PRESENTATION_QUEUE_TARGET_DESTROY,
                                (void **)&vdp_presentation_queue_target_destroy
                                );
  CHECK_ST
  
  vdp_st = vdp_get_proc_address(
                                vdp_device,
                                VDP_FUNC_ID_PRESENTATION_QUEUE_CREATE,
                                (void **)&vdp_presentation_queue_create
                                );
  CHECK_ST
  
  vdp_st = vdp_get_proc_address(
                                vdp_device,
                                VDP_FUNC_ID_PRESENTATION_QUEUE_DESTROY,
                                (void **)&vdp_presentation_queue_destroy
                                );
  CHECK_ST
  
  vdp_st = vdp_get_proc_address(
                                vdp_device,
                                VDP_FUNC_ID_PRESENTATION_QUEUE_DISPLAY,
                                (void **)&vdp_presentation_queue_display
                                );
  CHECK_ST
  
  vdp_st = vdp_get_proc_address(
                                vdp_device,
                                VDP_FUNC_ID_PRESENTATION_QUEUE_BLOCK_UNTIL_SURFACE_IDLE,
                                (void **)&vdp_presentation_queue_block_until_surface_idle
                                );
  CHECK_ST
  
  vdp_st = vdp_get_proc_address(
                                vdp_device,
                                VDP_FUNC_ID_PRESENTATION_QUEUE_TARGET_CREATE_X11,
                                (void **)&vdp_presentation_queue_target_create_x11
                                );
  CHECK_ST
  
  vdp_st = vdp_get_proc_address(
                                vdp_device,
                                VDP_FUNC_ID_DECODER_CREATE,
                                (void **)&vdp_decoder_create
                                );
  CHECK_ST
  
  vdp_st = vdp_get_proc_address(
                                vdp_device,
                                VDP_FUNC_ID_DECODER_DESTROY,
                                (void **)&vdp_decoder_destroy
                                );
  CHECK_ST
  
  vdp_st = vdp_get_proc_address(
                                vdp_device,
                                VDP_FUNC_ID_DECODER_RENDER,
                                (void **)&vdp_decoder_render
                                );
  CHECK_ST
  
  vdp_st = vdp_get_proc_address(
                                vdp_device,
                                VDP_FUNC_ID_DECODER_QUERY_CAPABILITIES,
                                (void **)&vdp_decoder_query_caps
                                );
  CHECK_ST
  
  vdp_st = vdp_get_proc_address(
                                vdp_device,
                                VDP_FUNC_ID_PRESENTATION_QUEUE_QUERY_SURFACE_STATUS,
                                (void **)&vdp_presentation_queue_query_surface_status
                                );
  CHECK_ST
  
  vdp_st = vdp_get_proc_address(
                                vdp_device,
                                VDP_FUNC_ID_PRESENTATION_QUEUE_GET_TIME,
                                (void **)&vdp_presentation_queue_get_time
                                );
  CHECK_ST
  
  // Added for draw_osd.
  vdp_st = vdp_get_proc_address(
                                vdp_device,
                                VDP_FUNC_ID_OUTPUT_SURFACE_RENDER_OUTPUT_SURFACE,
                                (void **)&vdp_output_surface_render_output_surface
                                );
  CHECK_ST
  
  vdp_st = vdp_get_proc_address(
                                vdp_device,
                                VDP_FUNC_ID_OUTPUT_SURFACE_PUT_BITS_INDEXED,
                                (void **)&vdp_output_surface_put_bits_indexed
                                );
  CHECK_ST
  
  vdp_st = vdp_get_proc_address(
                                vdp_device,
                                VDP_FUNC_ID_PREEMPTION_CALLBACK_REGISTER,
                                (void **)&vdp_preemption_callback_register
                                );
  CHECK_ST
  
}

VdpStatus CDVDVideoCodecVDPAU::finiVDPAUProcs()
{
  VdpStatus vdp_st;
  
  vdp_st = vdp_device_destroy(vdp_device);
  CHECK_ST
  
  return VDP_STATUS_OK;
}

void CDVDVideoCodecVDPAU::initVDPAUOutput()
{
  VdpStatus vdp_st;
  vdp_st = vdp_presentation_queue_target_create_x11(vdp_device,
                                                    m_Pixmap, //x_window,
                                                    &vdp_flip_target);
  CHECK_ST
  
  vdp_st = vdp_presentation_queue_create(vdp_device,
                                         vdp_flip_target,
                                         &vdp_flip_queue);
  CHECK_ST
  vdpauConfigured = false;
}

VdpStatus CDVDVideoCodecVDPAU::finiVDPAUOutput()
{
  VdpStatus vdp_st;
  
  vdp_st = vdp_presentation_queue_destroy(vdp_flip_queue);
  CHECK_ST
  
  vdp_st = vdp_presentation_queue_target_destroy(vdp_flip_target);
  CHECK_ST
  
  return VDP_STATUS_OK;
}


int CDVDVideoCodecVDPAU::configVDPAU(AVCodecContext* avctx)
{
  if (vdpauConfigured) return 1;
  VdpStatus vdp_st;
  int i;
  VdpDecoderProfile vdp_decoder_profile;
  VdpChromaType vdp_chroma_type;
  uint32_t max_references;
  vid_width = avctx->width;
  vid_height = avctx->height;
  image_format = avctx->pix_fmt;
  past[1] = past[0] = current = future = VDP_INVALID_HANDLE;
  CLog::Log(LOGNOTICE, "screenWidth:%i widWidth:%i",g_graphicsContext.GetWidth(),vid_width);
  // FIXME: Are higher profiles able to decode all lower profile streams?
  switch (image_format) {
    case PIX_FMT_VDPAU_MPEG1:
      vdp_decoder_profile = VDP_DECODER_PROFILE_MPEG1;
      vdp_chroma_type = VDP_CHROMA_TYPE_420;
      num_video_surfaces = NUM_VIDEO_SURFACES_MPEG2;
      break;
    case PIX_FMT_VDPAU_MPEG2:
      vdp_decoder_profile = VDP_DECODER_PROFILE_MPEG2_MAIN;
      vdp_chroma_type = VDP_CHROMA_TYPE_420;
      num_video_surfaces = NUM_VIDEO_SURFACES_MPEG2;
      break;
    case PIX_FMT_VDPAU_H264:
      vdp_decoder_profile = VDP_DECODER_PROFILE_H264_HIGH;
      vdp_chroma_type = VDP_CHROMA_TYPE_420;
      // Theoretically, "num_reference_surfaces+1" is correct.
      // However, to work around invalid/corrupt streams,
      // and/or ffmpeg DPB management issues,
      // we allocate more than we should need to allow problematic
      // streams to play.
      //num_video_surfaces = num_reference_surfaces + 1;
      num_video_surfaces = NUM_VIDEO_SURFACES_H264;
      break;
    case PIX_FMT_VDPAU_VC1:
      vdp_decoder_profile = VDP_DECODER_PROFILE_VC1_SIMPLE;
      vdp_chroma_type = VDP_CHROMA_TYPE_420;
      num_video_surfaces = NUM_VIDEO_SURFACES_VC1;
      break;
      
      /* Non VDPAU specific formats.
       * No HW acceleration. VdpDecoder will not be created and 
       * there will be no call for VdpDecoderRender.
       */
      /*case PIX_FMT_YV12:
       vdp_chroma_type = VDP_CHROMA_TYPE_420;
       num_video_surfaces = NUM_VIDEO_SURFACES_NON_ACCEL_YUV;
       break;*/
    case PIX_FMT_BGRA:
      // No need for videoSurfaces, directly renders to outputSurface.
      num_video_surfaces = NUM_VIDEO_SURFACES_NON_ACCEL_RGB;
      break;
    default:
      assert(0);
      return 1;
  }

  switch (image_format) {
    case PIX_FMT_VDPAU_H264:
   {
     max_references = avctx->ref_frames;
     if (max_references > 16) {
       max_references = 16;
     }
     num_video_surfaces = max_references+6;
   }
      break;
    default:
      max_references = 2;
      break;
  }

  if (num_video_surfaces) {
    videoSurfaces = (VdpVideoSurface *)malloc(sizeof(VdpVideoSurface)*num_video_surfaces);
  } else {
    videoSurfaces = NULL;
  }
  
  if (isVDPAUFormat(image_format)) {
    vdp_st = vdp_decoder_create(vdp_device,
                                vdp_decoder_profile,
                                vid_width,
                                vid_height,
                                max_references,
                                &decoder);
    CHECK_ST
  }
  
  // Creation of VideoSurfaces
  for (i = 0; i < num_video_surfaces; i++) {
    vdp_st = vdp_video_surface_create(vdp_device,
                                      vdp_chroma_type,
                                      vid_width,
                                      vid_height,
                                      &videoSurfaces[i]);
    CHECK_ST
  }
  
  if (num_video_surfaces) {
    surface_render = (vdpau_render_state*)malloc(num_video_surfaces*sizeof(vdpau_render_state));
    memset(surface_render,0,num_video_surfaces*sizeof(vdpau_render_state));
    
    for (i = 0; i < num_video_surfaces; i++) {
      //surface_render[i].magic = FF_VDPAU_RENDER_MAGIC;
      surface_render[i].state = FF_VDPAU_STATE_USED_FOR_RENDER;
      surface_render[i].surface = videoSurfaces[i];
    }
    
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
    
    VdpVideoMixerFeature features[] = {
      VDP_VIDEO_MIXER_FEATURE_NOISE_REDUCTION,
      VDP_VIDEO_MIXER_FEATURE_SHARPNESS,
      VDP_VIDEO_MIXER_FEATURE_DEINTERLACE_TEMPORAL,
      VDP_VIDEO_MIXER_FEATURE_DEINTERLACE_TEMPORAL_SPATIAL,
      VDP_VIDEO_MIXER_FEATURE_INVERSE_TELECINE
    };
    
    vdp_st = vdp_video_mixer_create(vdp_device,
                                    5,
                                    features,
                                    ARSIZE(parameters),
                                    parameters,
                                    parameter_values,
                                    &videoMixer);
    CHECK_ST
    
  } else {
    surface_render = NULL;
  }
  
  // Creation of outputSurfaces
  for (i = 0; i < NUM_OUTPUT_SURFACES; i++) {
    vdp_st = vdp_output_surface_create(vdp_device,
                                       VDP_RGBA_FORMAT_B8G8R8A8,
                                       vid_width,
                                       vid_height,
                                       &outputSurfaces[i]);
    CHECK_ST
  }
  surfaceNum = 0;
  outputSurface = outputSurfaces[surfaceNum];

  videoSurface = videoSurfaces[0];
  
  /*vdp_st = vdp_video_mixer_render(videoMixer,
                                  VDP_INVALID_HANDLE,
                                  0,
                                  VDP_VIDEO_MIXER_PICTURE_STRUCTURE_FRAME,
                                  0,
                                  NULL,
                                  videoSurface,
                                  0,
                                  NULL,
                                  NULL,
                                  outputSurface,
                                  &outRect,
                                  &outRectVid,
                                  0,
                                  NULL);
  CHECK_ST */
  spewHardwareAvailable();
  vdpauConfigured = true;
  return 0;
}

void CDVDVideoCodecVDPAU::spewHardwareAvailable()  //Copyright (c) 2008 Wladimir J. van der Laan  -- VDPInfo
{
  VdpStatus rv;
  CLog::Log(LOGNOTICE,"VDPAU Decoder capabilities:");
  CLog::Log(LOGNOTICE,"name          level macbs width height");
  CLog::Log(LOGNOTICE,"------------------------------------");
  for(int x=0; x<decoder_profile_count; ++x)
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

enum PixelFormat CDVDVideoCodecVDPAU::VDPAUGetFormat(struct AVCodecContext * avctx,
                                                     const PixelFormat * fmt)
{
  //CLog::Log(LOGNOTICE,"%s",__FUNCTION__);
  if(usingVDPAU){
    avctx->get_buffer= VDPAUGetBuffer;
    avctx->release_buffer= VDPAUReleaseBuffer;
    avctx->draw_horiz_band = VDPAURenderFrame;
    avctx->slice_flags=SLICE_FLAG_CODED_ORDER|SLICE_FLAG_ALLOW_FIELD;
  }
  pSingleton->configVDPAU(avctx);
  return fmt[0];
}

vdpau_render_state * CDVDVideoCodecVDPAU::VDPAUFindFreeSurface()
{
  int i; 
  for (i = 0 ; i < pSingleton->num_video_surfaces; i++)
   {
     //CLog::Log(LOGDEBUG,"find_free_surface(%i):0x%08x @ 0x%08x",i,pSingleton->surface_render[i].state, &(pSingleton->surface_render[i]));
     if (!(pSingleton->surface_render[i].state & FF_VDPAU_STATE_USED_FOR_REFERENCE)) {
       return &(pSingleton->surface_render[i]);
     }
   }
  return NULL;
}

int CDVDVideoCodecVDPAU::VDPAUGetBuffer(AVCodecContext *avctx, AVFrame *pic)
{
  //CLog::Log(LOGNOTICE,"%s",__FUNCTION__);
  struct pictureAge*   pA = (struct pictureAge*)avctx->opaque;

  pSingleton->m_avctx = avctx; 
  
  pSingleton->configVDPAU(avctx); //->width,avctx->height,avctx->pix_fmt);
  vdpau_render_state * render;
  
  if(!pic->reference){
    pSingleton->b_count++;
  }else{
    pSingleton->ip_count++;
  }
  
  render = VDPAUFindFreeSurface();
  //assert(render->magic == FF_VDPAU_RENDER_MAGIC);
  render->state = 0;
  
  pic->data[0]= (uint8_t*)render;
  pic->data[1]= (uint8_t*)render;
  pic->data[2]= (uint8_t*)render;
  
  /* Note, some (many) codecs in libavcodec must have stride1==stride2 && no changes between frames
   * lavc will check that and die with an error message, if its not true
   */
  pic->linesize[0]= 0;
  pic->linesize[1]= 0;
  pic->linesize[2]= 0;

/*  double *pts= (double*)malloc(sizeof(double));
  *pts= ((CDVDVideoCodecFFmpeg*)avctx->opaque)->m_pts;
  pic->opaque= pts;
*/
  if(pic->reference)
   {   //I or P frame
     pic->age = pA->ip_age[0];
     pA->ip_age[0]= pA->ip_age[1]+1;
     pA->ip_age[1]= 1;
     pA->b_age++;
   } else
    {   //B frame
      pic->age = pA->b_age;
      pA->ip_age[0]++;
      pA->ip_age[1]++;
      pA->b_age = 1;
    }
  pic->type= FF_BUFFER_TYPE_USER;
  
  assert(render != NULL);
  //assert(render->magic == FF_VDPAU_RENDER_MAGIC);
  render->state |= FF_VDPAU_STATE_USED_FOR_REFERENCE;
  
  return 0;
}

void CDVDVideoCodecVDPAU::VDPAUReleaseBuffer(AVCodecContext *avctx, AVFrame *pic)
{
  //CLog::Log(LOGNOTICE,"%s",__FUNCTION__);
  vdpau_render_state * render;
  int i;
  
  if(pSingleton->ip_count <= 2 && pSingleton->b_count<=1){
    if(pic->reference)
      pSingleton->ip_count--;
    else
      pSingleton->b_count--;
  }
  
  // Mark the surface as not required for prediction
  render=(vdpau_render_state*)pic->data[2];
  assert(render != NULL);
  //assert(render->magic == FF_VDPAU_RENDER_MAGIC);
  render->state &= ~FF_VDPAU_STATE_USED_FOR_REFERENCE;
  for(i=0; i<4; i++){
    pic->data[i]= NULL;
  }
  /*if (pic->opaque)
    free(pic->opaque);
  pic->opaque = NULL;*/
}

int CDVDVideoCodecVDPAU::VDPAUDrawSlice(uint8_t * image[], int stride[], int w, int h,
                                        int x, int y)
{
  //CLog::Log(LOGNOTICE,"%s",__FUNCTION__);
  VdpStatus vdp_st;
  vdpau_render_state * render;
  
  render = (vdpau_render_state*)image[2]; // this is a copy of private
  assert( render != NULL );
  //assert(render->magic == FF_VDPAU_RENDER_MAGIC);
  
  /* VdpDecoderRender is called with decoding order. Decoded images are store in
   * videoSurface like rndr->surface. VdpVideoMixerRender put this videoSurface
   * to outputSurface which is displayable.
   */
  pSingleton->checkRecover();
  vdp_st = pSingleton->vdp_decoder_render(pSingleton->decoder,
                                          render->surface,
                                          (VdpPictureInfo const *)&(render->info),
                                          render->bitstream_buffers_used,
                                          render->bitstream_buffers);
  CHECK_ST
  return 0;
}


void CDVDVideoCodecVDPAU::VDPAURenderFrame(struct AVCodecContext *s,
                                           const AVFrame *src, int offset[4],
                                           int y, int type, int height)
{
  //CLog::Log(LOGNOTICE,"%s",__FUNCTION__);
  int width= s->width;
  uint8_t *source[3]= {src->data[0], src->data[1], src->data[2]};
  
  assert(src->linesize[0]==0 && src->linesize[1]==0 && src->linesize[2]==0);
  assert(offset[0]==0 && offset[1]==0 && offset[2]==0);
  
  pSingleton->VDPAUDrawSlice(source, (int*)(src->linesize), width, height, 0, y);
}

void CDVDVideoCodecVDPAU::VDPAUPrePresent(AVCodecContext *avctx, AVFrame *pFrame)
{
  //CLog::Log(LOGNOTICE,"%s",__FUNCTION__);
  vdpau_render_state * render = (vdpau_render_state*)pFrame->data[2];
  VdpVideoMixerPictureStructure structure;
  VdpTime dummy;
  VdpStatus vdp_st;
  
  pSingleton->checkFeatures();

  pSingleton->configVDPAU(avctx); //->width,avctx->height,avctx->pix_fmt);
  pSingleton->outputSurface = pSingleton->outputSurfaces[pSingleton->surfaceNum];
  //  usleep(2000);
  pSingleton->past[1] = pSingleton->past[0];
  pSingleton->past[0] = pSingleton->current;
  pSingleton->current = pSingleton->future;
  pSingleton->future = render->surface;
  //if (pSingleton->past[1] == VDP_INVALID_HANDLE)
    //return;
  pSingleton->interlaced = pFrame->interlaced_frame;

  if (pSingleton->interlaced) {
    structure = pFrame->top_field_first ? VDP_VIDEO_MIXER_PICTURE_STRUCTURE_TOP_FIELD :
                                          VDP_VIDEO_MIXER_PICTURE_STRUCTURE_BOTTOM_FIELD;
    pSingleton->past[1] = pSingleton->past[0];
    pSingleton->past[0] = pSingleton->current;
    pSingleton->current = pSingleton->future;
    pSingleton->future = render->surface;
  }
  else structure = VDP_VIDEO_MIXER_PICTURE_STRUCTURE_FRAME;

  pSingleton->checkRecover();
  /*vdp_st = pSingleton->vdp_presentation_queue_block_until_surface_idle(
                                              pSingleton->vdp_flip_queue,
                                              pSingleton->outputSurface,
                                              &dummy);
  */
  if (( pSingleton->outRect.x1 != pSingleton->outWidth ) || 
      ( pSingleton->outRect.y1 != pSingleton->outHeight ))
  {
    pSingleton->outRectVid.x0 = 0;
    pSingleton->outRectVid.y0 = 0;
    pSingleton->outRectVid.x1 = pSingleton->vid_width;
    pSingleton->outRectVid.y1 = pSingleton->vid_height;

    if(g_graphicsContext.GetViewWindow().right < pSingleton->vid_width)
      pSingleton->outWidth = pSingleton->vid_width; 
      else pSingleton->outWidth = g_graphicsContext.GetViewWindow().right;
    if(g_graphicsContext.GetViewWindow().bottom < pSingleton->vid_height)
      pSingleton->outHeight = pSingleton->vid_height; 
      else pSingleton->outHeight = g_graphicsContext.GetViewWindow().bottom;

    pSingleton->outRect.x0 = 0;
    pSingleton->outRect.y0 = 0;
    pSingleton->outRect.x1 = pSingleton->outWidth;
    pSingleton->outRect.y1 = pSingleton->outHeight;
  }

  pSingleton->checkRecover();
  vdp_st = pSingleton->vdp_video_mixer_render(pSingleton->videoMixer,
                                              VDP_INVALID_HANDLE,
                                              0,
                                              structure,
                                              pSingleton->interlaced ? 2 : 0,
                                              pSingleton->interlaced ? pSingleton->past : NULL,
                                              pSingleton->interlaced ? pSingleton->current : render->surface,
                                              pSingleton->interlaced ? 1 : 0,
                                              pSingleton->interlaced ? &(pSingleton->future) : NULL,
                                              NULL,
                                              pSingleton->outputSurface,
                                              &(pSingleton->outRect),
                                              &(pSingleton->outRectVid),
                                              0,
                                              NULL);
  CHECK_ST
}

void CDVDVideoCodecVDPAU::VDPAUPresent()
{
  //CLog::Log(LOGNOTICE,"%s",__FUNCTION__);
  VdpStatus vdp_st;
  pSingleton->checkRecover();
  vdp_st = pSingleton->vdp_presentation_queue_display(pSingleton->vdp_flip_queue,
                                                      pSingleton->outputSurface,
                                                      0,
                                                      0,
                                                      0);
  pSingleton->surfaceNum = pSingleton->surfaceNum ^ 1;
}

void CDVDVideoCodecVDPAU::vdpPreemptionCallbackFunction(VdpDevice device, void* context)
{
  CLog::Log(LOGERROR,"VDPAU Device Preempted - attempting recovery");
  CDVDVideoCodecVDPAU* pCtx = (CDVDVideoCodecVDPAU*)context;
  pCtx->recover = true;
}

bool CDVDVideoCodecVDPAU::checkDeviceCaps(uint32_t Param)
{ 
  VdpStatus vdp_st;
  VdpBool supported = false;
  uint32_t max_level, max_macroblocks, max_width, max_height;
  vdp_st = pSingleton->vdp_decoder_query_caps(pSingleton->vdp_device, Param, 
                              &supported, &max_level, &max_macroblocks, &max_width, &max_height);
  return supported;
}
