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

#include "DVDVideoCodec.h"
#include "cores/ffmpeg/DllAvCodec.h"
#include "cores/ffmpeg/DllAvFormat.h"
#include "cores/ffmpeg/DllSwScale.h"
#include "cores/ffmpeg/vdpau.h"
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#define GL_GLEXT_PROTOTYPES
#define GLX_GLXEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glx.h>
#include "utils/CriticalSection.h"

#define CHECK_ST \
if (vdp_st != VDP_STATUS_OK) \
CLog::Log(LOGERROR, " (VDPAU) Error: (%d) at %s:%d\n", vdp_st, __FILE__, __LINE__);
  //else CLog::Log(LOGNOTICE, " (VDPAU) Success at %s:%d\n", __FILE__, __LINE__);

#define CHECK_GL \
rv = glGetError(); \
if (rv) \
CLog::Log(LOGERROR, "openGL Error: %i",rv);

#define NUM_OUTPUT_SURFACES                5
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

class CDVDVideoCodecVDPAU : public CCriticalSection
{
public:
  CDVDVideoCodecVDPAU(int width, int height);
  virtual ~CDVDVideoCodecVDPAU();

  static void             FFReleaseBuffer(AVCodecContext *avctx, AVFrame *pic);
  static void             FFDrawSlice(struct AVCodecContext *s,
                               const AVFrame *src, int offset[4],
                               int y, int type, int height);
  static enum PixelFormat FFGetFormat(struct AVCodecContext * avctx,
                                         const enum PixelFormat * pix_fmt);
  static int              FFGetBuffer(AVCodecContext *avctx, AVFrame *pic);

  static void             VDPPreemptionCallbackFunction(VdpDevice device, void* context);

  vdpau_render_state * FindFreeSurface();
  void PrePresent(AVCodecContext *avctx, AVFrame *pFrame);
  void Present();
  void NotifySwap();
  bool IsVDPAUFormat(uint32_t format);
  int  ConfigVDPAU(AVCodecContext* avctx);
  void SpewHardwareAvailable();
  void InitCSCMatrix();
  void CheckStatus(VdpStatus vdp_st, int line);

  bool CheckDeviceCaps(uint32_t Param);
  void CheckRecover();
  void CheckFeatures();
  void SetColor();
  void SetTelecine();
  void SetNoiseReduction();
  void SetSharpness();
  void SetDeinterlacing();
  bool usingVDPAU;
  bool VDPAURecovered, VDPAUSwitching;

  VdpTime    lastSwapTime, frameLagTime, frameLagTimeRunning, frameLagAverage;
  VdpTime    previousTime;

  INT64      frameCounter;
  pictureAge picAge;
  bool       recover;
  VdpVideoSurface past[2], current, future;
  int        tmpDeint;
  float      tmpNoiseReduction, tmpSharpness;
  float      tmpBrightness, tmpContrast;
  bool       tmpInverseTelecine;
  bool       interlaced;
  int        outWidth, outHeight;
  int        lastDisplayedSurface;
  VdpProcamp m_Procamp;
  VdpCSCMatrix m_CSCMatrix;
  VdpDevice  GetVdpDevice() { return vdp_device; };

  AVCodecContext* m_avctx;

  //  protected:
  void      InitVDPAUProcs();
  VdpStatus FiniVDPAUProcs();
  void      InitVDPAUOutput();
  VdpStatus FiniVDPAUOutput();

  VdpDevice                            vdp_device;
  VdpGetProcAddress *                  vdp_get_proc_address;
  VdpPresentationQueueTarget           vdp_flip_target;
  VdpPresentationQueue                 vdp_flip_queue;
  VdpDeviceDestroy *                   vdp_device_destroy;

  VdpVideoSurfaceCreate *              vdp_video_surface_create;
  VdpVideoSurfaceDestroy *             vdp_video_surface_destroy;
  VdpVideoSurfacePutBitsYCbCr *        vdp_video_surface_put_bits_y_cb_cr;
  VdpVideoSurfaceGetBitsYCbCr *        vdp_video_surface_get_bits_y_cb_cr;

  VdpOutputSurfacePutBitsYCbCr *        vdp_output_surface_put_bits_y_cb_cr;
  VdpOutputSurfacePutBitsNative *       vdp_output_surface_put_bits_native;
  VdpOutputSurfaceCreate *              vdp_output_surface_create;
  VdpOutputSurfaceDestroy *             vdp_output_surface_destroy;
  VdpOutputSurfaceGetBitsNative *       vdp_output_surface_get_bits_native;
  VdpOutputSurfaceRenderOutputSurface * vdp_output_surface_render_output_surface;
  VdpOutputSurfacePutBitsIndexed *      vdp_output_surface_put_bits_indexed;

  VdpVideoMixerCreate *                vdp_video_mixer_create;
  VdpVideoMixerSetFeatureEnables *     vdp_video_mixer_set_feature_enables;
  VdpVideoMixerQueryParameterSupport * vdp_video_mixer_query_parameter_support;
  VdpVideoMixerDestroy *               vdp_video_mixer_destroy;
  VdpVideoMixerRender *                vdp_video_mixer_render;
  VdpVideoMixerSetAttributeValues *    vdp_video_mixer_set_attribute_values;

  VdpGenerateCSCMatrix *               vdp_generate_csc_matrix;

  VdpPresentationQueueTargetDestroy *         vdp_presentation_queue_target_destroy;
  VdpPresentationQueueCreate *                vdp_presentation_queue_create;
  VdpPresentationQueueDestroy *               vdp_presentation_queue_destroy;
  VdpPresentationQueueDisplay *               vdp_presentation_queue_display;
  VdpPresentationQueueBlockUntilSurfaceIdle * vdp_presentation_queue_block_until_surface_idle;
  VdpPresentationQueueTargetCreateX11 *       vdp_presentation_queue_target_create_x11;
  VdpPresentationQueueQuerySurfaceStatus *    vdp_presentation_queue_query_surface_status;
  VdpPresentationQueueGetTime *               vdp_presentation_queue_get_time;

  VdpGetErrorString *                         vdp_get_error_string;

  VdpDecoderCreate *            vdp_decoder_create;
  VdpDecoderDestroy *           vdp_decoder_destroy;
  VdpDecoderRender *            vdp_decoder_render;
  VdpDecoderQueryCapabilities * vdp_decoder_query_caps;

  VdpPreemptionCallbackRegister * vdp_preemption_callback_register;

  VdpVideoSurface * videoSurfaces;
  VdpVideoSurface   videoSurface;

  VdpOutputSurface  outputSurfaces[NUM_OUTPUT_SURFACES];
  VdpOutputSurface  outputSurface;

  VdpDecoder    decoder;
  VdpVideoMixer videoMixer;
  VdpRect       outRect;
  VdpRect       outRectVid;

  vdpau_render_state * surface_render;
  int      surfaceNum;
  uint32_t vid_width, vid_height;
  uint32_t image_format;
  uint32_t num_video_surfaces;
  uint32_t num_reference_surfaces;
  GLenum   rv;
  Display* m_Display;
  Surface::CSurface *m_Surface;
  bool     vdpauConfigured;
};

#endif // __DVDVIDEOCODECFFMMPEGVDPAU_H

