/*
 *  DVDVideoCodecFFmpegVDPAU.cpp
 *  XBMC
 *
 *  Created by David Allonby on 10/02/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include "DVDVideoCodecFFmpegVDPAU.h"
#include "Surface.h"
using namespace Surface;

#include "vdpau_render.h"
#include "TextureManager.h"                         //DAVID-CHECKNEEDED
#include "cores/VideoRenderers/RenderManager.h"
#include "DVDVideoCodecFFmpeg.h"

#define ARSIZE(x) (sizeof(x) / sizeof((x)[0]))

static CDVDVideoCodecVDPAU *pSingleton = NULL;


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
  outputSurface = 0;
  lastFrameTime = nextFrameTime = 0;
}

CDVDVideoCodecVDPAU::~CDVDVideoCodecVDPAU()
{
  finiVDPAUOutput();
  finiVDPAUProcs();
	if (num_video_surfaces)
    free(videoSurfaces);
  pSingleton = NULL;
}

bool CDVDVideoCodecVDPAU::isVDPAUFormat(uint32_t format)
{
	if ((format >= PIX_FMT_VDPAU_MPEG1) && (format <= PIX_FMT_VDPAU_VC1_ADVANCED)) return true;
	else return false;
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


int CDVDVideoCodecVDPAU::configVDPAU(uint32_t width, uint32_t height,
                                     uint32_t format)
{
  if (vdpauConfigured) return 1;
  VdpStatus vdp_st;
  int i;
  VdpDecoderProfile vdp_decoder_profile;
  VdpChromaType vdp_chroma_type;
  uint32_t max_references;
  image_format = format;
  
  // FIXME: Are higher profiles able to decode all lower profile streams?
  switch (format) {
    case PIX_FMT_VDPAU_MPEG1:
      vdp_decoder_profile = VDP_DECODER_PROFILE_MPEG1;
      vdp_chroma_type = VDP_CHROMA_TYPE_420;
      num_video_surfaces = NUM_VIDEO_SURFACES_MPEG2;
      break;
    case PIX_FMT_VDPAU_MPEG2_SIMPLE:
      vdp_decoder_profile = VDP_DECODER_PROFILE_MPEG2_SIMPLE;
      vdp_chroma_type = VDP_CHROMA_TYPE_420;
      num_video_surfaces = NUM_VIDEO_SURFACES_MPEG2;
      break;
    case PIX_FMT_VDPAU_MPEG2_MAIN:
      vdp_decoder_profile = VDP_DECODER_PROFILE_MPEG2_MAIN;
      vdp_chroma_type = VDP_CHROMA_TYPE_420;
      num_video_surfaces = NUM_VIDEO_SURFACES_MPEG2;
      break;
    case PIX_FMT_VDPAU_H264_BASELINE:
      vdp_decoder_profile = VDP_DECODER_PROFILE_H264_BASELINE;
      vdp_chroma_type = VDP_CHROMA_TYPE_420;
      // Theoretically, "num_reference_surfaces+1" is correct.
      // However, to work around invalid/corrupt streams,
      // and/or ffmpeg DPB management issues,
      // we allocate more than we should need to allow problematic
      // streams to play.
      //num_video_surfaces = num_reference_surfaces + 1;
      num_video_surfaces = NUM_VIDEO_SURFACES_H264;
      break;
    case PIX_FMT_VDPAU_H264_MAIN:
      vdp_decoder_profile = VDP_DECODER_PROFILE_H264_MAIN;
      vdp_chroma_type = VDP_CHROMA_TYPE_420;
      // Theoretically, "num_reference_surfaces+1" is correct.
      // However, to work around invalid/corrupt streams,
      // and/or ffmpeg DPB management issues,
      // we allocate more than we should need to allow problematic
      // streams to play.
      //num_video_surfaces = num_reference_surfaces + 1;
      num_video_surfaces = NUM_VIDEO_SURFACES_H264;
      break;
    case PIX_FMT_VDPAU_H264_HIGH:
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
    case PIX_FMT_VDPAU_VC1_SIMPLE:
      vdp_decoder_profile = VDP_DECODER_PROFILE_VC1_SIMPLE;
      vdp_chroma_type = VDP_CHROMA_TYPE_420;
      num_video_surfaces = NUM_VIDEO_SURFACES_VC1;
      break;
    case PIX_FMT_VDPAU_VC1_MAIN:
      vdp_decoder_profile = VDP_DECODER_PROFILE_VC1_MAIN;
      vdp_chroma_type = VDP_CHROMA_TYPE_420;
      num_video_surfaces = NUM_VIDEO_SURFACES_VC1;
      break;
    case PIX_FMT_VDPAU_VC1_ADVANCED:
      vdp_decoder_profile = VDP_DECODER_PROFILE_VC1_ADVANCED;
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
  
	if (num_video_surfaces) {
    videoSurfaces = (VdpVideoSurface *)malloc(sizeof(VdpVideoSurface)*num_video_surfaces);
  } else {
    videoSurfaces = NULL;
  }
  
  switch (format) {
    case PIX_FMT_VDPAU_H264_MAIN:
    case PIX_FMT_VDPAU_H264_HIGH:
   {
     // FIXME: Use "h->sps.ref_frame_count" here instead.
     
     // Level 4.1 limits:
     uint32_t round_width = (width + 15) & ~15;
     uint32_t round_height = (height + 15) & ~15;
     //            uint32_t surf_size = (round_width * round_height * 3) / 2;
     max_references = 16;
     if (max_references > 16) {
       max_references = 16;
     }
   }
      break;
    default:
      max_references = 2;
      break;
  }
  
  if (isVDPAUFormat(image_format)) {
    vdp_st = vdp_decoder_create(vdp_device,
                                vdp_decoder_profile,
                                width,
                                height,
                                max_references,
                                &decoder);
    CHECK_ST
  }
  
  // Creation of VideoSurfaces
  for (i = 0; i < num_video_surfaces; i++) {
    vdp_st = vdp_video_surface_create(vdp_device,
                                      vdp_chroma_type,
                                      width,
                                      height,
                                      &videoSurfaces[i]);
    CHECK_ST
  }
  
  if (num_video_surfaces) {
    surface_render = (vdpau_render_state_t*)malloc(num_video_surfaces*sizeof(vdpau_render_state_t));
    memset(surface_render,0,num_video_surfaces*sizeof(vdpau_render_state_t));
    
    for (i = 0; i < num_video_surfaces; i++) {
      surface_render[i].magic = MP_VDPAU_RENDER_MAGIC;
      surface_render[i].state = MP_VDPAU_STATE_USED_FOR_RENDER;
      surface_render[i].surface = videoSurfaces[i];
    }
    
    // Creation of VideoMixer.
    VdpVideoMixerParameter parameters[] = {
      VDP_VIDEO_MIXER_PARAMETER_VIDEO_SURFACE_WIDTH,
      VDP_VIDEO_MIXER_PARAMETER_VIDEO_SURFACE_HEIGHT,
      VDP_VIDEO_MIXER_PARAMETER_CHROMA_TYPE
    };
    
    void const * parameter_values[] = {
      &width,
      &height,
      &vdp_chroma_type
    };
    
    vdp_st = vdp_video_mixer_create(vdp_device,
                                    0,
                                    NULL,
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
                                       width,
                                       height,
                                       &outputSurfaces[i]);
    CHECK_ST
  }
  surfaceNum = 0;
  outputSurface = outputSurfaces[surfaceNum];
  
  outRectVid.x0 = 0;
  outRectVid.y0 = 0;
  outRectVid.x1 = width;
  outRectVid.y1 = height;
  
  outRect.x0 = 0;
  outRect.x1 = width;
  outRect.y0 = 0;
  outRect.y1 = height;
  
  videoSurface = videoSurfaces[0];
  
  vdp_st = vdp_video_mixer_render(videoMixer,
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
  CHECK_ST
  vdpauConfigured = true;
  return 0;
}

enum PixelFormat CDVDVideoCodecVDPAU::VDPAUGetFormat(struct AVCodecContext * avctx,
                                                     const PixelFormat * fmt)
{
  if(avctx->vdpau_acceleration){
    avctx->get_buffer= VDPAUGetBuffer;
    avctx->release_buffer= VDPAUReleaseBuffer;
    avctx->draw_horiz_band = VDPAURenderFrame;
    avctx->slice_flags=SLICE_FLAG_CODED_ORDER|SLICE_FLAG_ALLOW_FIELD;
  }
  pSingleton->configVDPAU(avctx->width,avctx->height,fmt[0]);
  return fmt[0];
}

vdpau_render_state_t * CDVDVideoCodecVDPAU::VDPAUFindFreeSurface()
{
  int i; 
  for (i = 0 ; i < pSingleton->num_video_surfaces; i++)
   {
     //CLog::Log(LOGDEBUG,"find_free_surface(%i):0x%08x @ 0x%08x",i,pSingleton->surface_render[i].state, &(pSingleton->surface_render[i]));
     if (!(pSingleton->surface_render[i].state & MP_VDPAU_STATE_USED_FOR_REFERENCE)) {
       return &(pSingleton->surface_render[i]);
     }
   }
  return NULL;
}

int CDVDVideoCodecVDPAU::VDPAUGetBuffer(AVCodecContext *avctx, AVFrame *pic)
{
  struct pictureAge*   pA = (struct pictureAge*)avctx->opaque;

  pSingleton->configVDPAU(avctx->width,avctx->height,avctx->pix_fmt);
  vdpau_render_state_t * render;
  
  if(!avctx->vdpau_acceleration){
    CLog::Log(LOGERROR,"vdpau_get_buffer() How did we get here?!");
    assert(0);
    exit(1);
  }
  
  assert(avctx->draw_horiz_band == VDPAURenderFrame);
  assert(avctx->release_buffer == VDPAUReleaseBuffer);
  
  if(!pic->reference){
    pSingleton->b_count++;
  }else{
    pSingleton->ip_count++;
  }
  
  render = VDPAUFindFreeSurface();
	assert(render->magic == MP_VDPAU_RENDER_MAGIC);
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

  double *pts= (double*)malloc(sizeof(double));
  *pts= ((CDVDVideoCodecFFmpeg*)avctx->opaque)->m_pts;
  pic->opaque= pts;

  if(pic->reference)
   {   //I or P frame
     pic->age = pA->ip_age[0];
     pA->ip_age[0]= pA->ip_age[1];
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
  assert(render->magic == MP_VDPAU_RENDER_MAGIC);
  render->state |= MP_VDPAU_STATE_USED_FOR_REFERENCE;

  return 0;
}

void CDVDVideoCodecVDPAU::VDPAUReleaseBuffer(AVCodecContext *avctx, AVFrame *pic)
{
  vdpau_render_state_t * render;
  int i;
  
  if(pSingleton->ip_count <= 2 && pSingleton->b_count<=1){
    if(pic->reference)
      pSingleton->ip_count--;
    else
      pSingleton->b_count--;
  }
  
  // Mark the surface as not required for prediction
  render=(vdpau_render_state_t*)pic->data[2];
  assert(render != NULL);
  assert(render->magic == MP_VDPAU_RENDER_MAGIC);
  render->state &= ~MP_VDPAU_STATE_USED_FOR_REFERENCE;
  for(i=0; i<4; i++){
    pic->data[i]= NULL;
  }
  if (pic->opaque)
    free(pic->opaque);
  pic->opaque = NULL;
}

int CDVDVideoCodecVDPAU::VDPAUDrawSlice(uint8_t * image[], int stride[], int w, int h,
                                        int x, int y)
{
  VdpStatus vdp_st;
  vdpau_render_state_t * render;
  
  render = (vdpau_render_state_t*)image[2]; // this is a copy of private
  assert( render != NULL );
  assert(render->magic == MP_VDPAU_RENDER_MAGIC);
  
  /* VdpDecoderRender is called with decoding order. Decoded images are store in
   * videoSurface like rndr->surface. VdpVideoMixerRender put this videoSurface
   * to outputSurface which is displayable.
   */
  vdp_st = pSingleton->vdp_decoder_render(pSingleton->decoder,
                                          render->surface,
                                          (VdpPictureInfo const *)&(render->info),
                                          render->bitstreamBuffersUsed,
                                          render->bitstreamBuffers);
  CHECK_ST
  return 0;
}


void CDVDVideoCodecVDPAU::VDPAURenderFrame(struct AVCodecContext *s,
                                           const AVFrame *src, int offset[4],
                                           int y, int type, int height)
{
  VdpStatus vdp_st;


  int width= s->width;
  uint8_t *source[3]= {src->data[0], src->data[1], src->data[2]};

  assert(src->linesize[0]==0 && src->linesize[1]==0 && src->linesize[2]==0);
  assert(offset[0]==0 && offset[1]==0 && offset[2]==0);

  pSingleton->VDPAUDrawSlice(source, (int*)(src->linesize), width, height, 0, y);
}

void CDVDVideoCodecVDPAU::VDPAUPrePresent(AVCodecContext *avctx, AVFrame *pFrame)
{
  vdpau_render_state_t * render = (vdpau_render_state_t*)pFrame->data[2];
  VdpTime dummy;
  VdpStatus vdp_st;

  pSingleton->outputSurface = pSingleton->outputSurfaces[pSingleton->surfaceNum];
  usleep(2000);
/*  vdp_st = pSingleton->vdp_presentation_queue_block_until_surface_idle(
                                             pSingleton->vdp_flip_queue,
                                             pSingleton->outputSurface,
                                             &dummy);
*/  CHECK_ST
  vdp_st = pSingleton->vdp_video_mixer_render(pSingleton->videoMixer,
                                              VDP_INVALID_HANDLE,
                                              0,
                                              VDP_VIDEO_MIXER_PICTURE_STRUCTURE_FRAME,
                                              0,
                                              NULL,
                                              render->surface,
                                              0,
                                              NULL,
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
  VdpTime time;
  VdpStatus vdp_st;

  VdpPresentationQueueStatus status;

  vdp_st = pSingleton->vdp_presentation_queue_display(pSingleton->vdp_flip_queue,
                                                      pSingleton->outputSurface,
                                                      0,
                                                      0,
                                                      0);
  pSingleton->surfaceNum = pSingleton->surfaceNum ^ 1;
}
