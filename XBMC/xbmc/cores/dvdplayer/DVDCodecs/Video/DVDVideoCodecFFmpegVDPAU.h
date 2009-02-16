#pragma once
#ifndef __DVDVIDEOCODECFFMMPEGVDPAU_H
#define __DVDVIDEOCODECFFMMPEGVDPAU_H

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
#include "DVDVideoCodec.h"
#include "cores/ffmpeg/DllAvCodec.h"
#include "cores/ffmpeg/DllAvFormat.h"
#include "cores/ffmpeg/DllSwScale.h"
#include "cores/ffmpeg/vdpau_render.h"
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#define GL_GLEXT_PROTOTYPES
#define GLX_GLXEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glx.h>

#define CHECK_ST \
if (vdp_st != VDP_STATUS_OK) \
CLog::Log(LOGERROR, " (VDPAU) Error %d at %s:%d\n", vdp_st, __FILE__, __LINE__); \
  //else CLog::Log(LOGNOTICE, " (VDPAU) Success at %s:%d\n", __FILE__, __LINE__);


#define CHECK_GL \
rv = glGetError(); \
if (rv) \
CLog::Log(LOGERROR, "openGL Error: %i",rv);

#define NUM_OUTPUT_SURFACES                4
#define NUM_VIDEO_SURFACES_MPEG2           3  // (1 frame being decoded, 2 reference)
#define NUM_VIDEO_SURFACES_H264            32 // (1 frame being decoded, up to 16 references) 
#define NUM_VIDEO_SURFACES_VC1             3  // (same as MPEG-2)
#define NUM_VIDEO_SURFACES_NON_ACCEL_YUV   1  //  surfaces for YV12 etc. 
#define NUM_VIDEO_SURFACES_NON_ACCEL_RGB   0 // surfaces for RGB or YUV4:4:4
#define PALETTE_SIZE 256

struct pictureAge
{
  int b_age;
  int ip_age[2];
};

struct Desc
{
  const char *name;
  uint32_t id;
  uint32_t aux; /* optional extra parameter... */
};

class CDVDVideoCodecVDPAU {
public:
  static void VDPAUReleaseBuffer(AVCodecContext *avctx, AVFrame *pic);
  static void VDPAURenderFrame(struct AVCodecContext *s,
                               const AVFrame *src, int offset[4],
                               int y, int type, int height);
  static enum PixelFormat VDPAUGetFormat(struct AVCodecContext * avctx,
                                         const enum PixelFormat * pix_fmt);
  static int VDPAUGetBuffer(AVCodecContext *avctx, AVFrame *pic);  
  static vdpau_render_state_t * VDPAUFindFreeSurface();
  static int VDPAUDrawSlice(uint8_t * image[],
                            int stride[], 
                            int w, int h,
                            int x, int y);
  static void VDPAUPrePresent(AVCodecContext *avctx, AVFrame *pFrame);
  static void VDPAUPresent();
  CDVDVideoCodecVDPAU(Display* display, Pixmap px);
  virtual ~CDVDVideoCodecVDPAU();
  virtual bool isVDPAUFormat(uint32_t format);
  int configVDPAU(uint32_t width, uint32_t height,
                  uint32_t format);
  void spewHardwareAvailable();
  VdpTime lastFrameTime, nextFrameTime;
  pictureAge picAge;
  
  //  protected:
  virtual void initVDPAUProcs();
  virtual VdpStatus finiVDPAUProcs();
  virtual void initVDPAUOutput();
  virtual VdpStatus finiVDPAUOutput();
  VdpDevice           vdp_device;
  VdpGetProcAddress * vdp_get_proc_address;
  VdpPresentationQueueTarget vdp_flip_target;
  VdpPresentationQueue       vdp_flip_queue;
  VdpDeviceDestroy * vdp_device_destroy;
  VdpVideoSurfaceCreate * vdp_video_surface_create;
  VdpVideoSurfaceDestroy * vdp_video_surface_destroy;
  VdpVideoSurfacePutBitsYCbCr * vdp_video_surface_put_bits_y_cb_cr;
  VdpVideoSurfaceGetBitsYCbCr * vdp_video_surface_get_bits_y_cb_cr;
  VdpOutputSurfacePutBitsYCbCr * vdp_output_surface_put_bits_y_cb_cr;
  VdpOutputSurfacePutBitsNative * vdp_output_surface_put_bits_native;
  VdpOutputSurfaceCreate * vdp_output_surface_create;
  VdpOutputSurfaceDestroy * vdp_output_surface_destroy;
  VdpOutputSurfaceGetBitsNative * vdp_output_surface_get_bits_native;
  VdpVideoMixerCreate * vdp_video_mixer_create;
  VdpVideoMixerSetFeatureEnables * vdp_video_mixer_set_feature_enables;
  VdpVideoMixerDestroy * vdp_video_mixer_destroy;
  VdpVideoMixerRender * vdp_video_mixer_render;
  VdpGenerateCSCMatrix * vdp_generate_csc_matrix;
  VdpVideoMixerSetAttributeValues * vdp_video_mixer_set_attribute_values;  
  VdpPresentationQueueTargetDestroy * vdp_presentation_queue_target_destroy;
  VdpPresentationQueueCreate * vdp_presentation_queue_create;
  VdpPresentationQueueDestroy * vdp_presentation_queue_destroy;
  VdpPresentationQueueDisplay * vdp_presentation_queue_display;
  VdpPresentationQueueBlockUntilSurfaceIdle * vdp_presentation_queue_block_until_surface_idle;
  VdpPresentationQueueTargetCreateX11 * vdp_presentation_queue_target_create_x11;
  VdpPresentationQueueQuerySurfaceStatus * vdp_presentation_queue_query_surface_status;
  VdpPresentationQueueGetTime * vdp_presentation_queue_get_time;
  VdpOutputSurfaceRenderOutputSurface * vdp_output_surface_render_output_surface;
  VdpOutputSurfacePutBitsIndexed * vdp_output_surface_put_bits_indexed;
  VdpDecoderCreate * vdp_decoder_create;
  VdpDecoderDestroy * vdp_decoder_destroy;
  VdpDecoderRender * vdp_decoder_render;
  VdpDecoderQueryCapabilities * vdp_decoder_query_caps;
  VdpVideoSurface *videoSurfaces;
  VdpOutputSurface outputSurfaces[NUM_OUTPUT_SURFACES];
  VdpVideoSurface videoSurface;
  VdpOutputSurface outputSurface;
  VdpDecoder decoder;
  VdpVideoMixer videoMixer;
  VdpRect outRect;
  VdpRect outRectVid;
  vdpau_render_state_t * surface_render;
  int surfaceNum;
  uint32_t vid_width, vid_height;
  uint32_t image_format;
  uint32_t num_video_surfaces;
  uint32_t num_reference_surfaces;
  int ip_count, b_count;
  GLenum rv;
  Display* m_Display;
  Pixmap m_Pixmap;
  bool vdpauConfigured;
};

#endif // __DVDVIDEOCODECFFMMPEGVDPAU_H
