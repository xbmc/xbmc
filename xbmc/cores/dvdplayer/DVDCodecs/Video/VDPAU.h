
#pragma once
/*
 *      Copyright (C) 2005-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "system_gl.h"

#include <queue>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#define GLX_GLXEXT_PROTOTYPES
#include <GL/glx.h>

#include "DllAvUtil.h"
#include "DVDVideoCodec.h"
#include "DVDVideoCodecFFmpeg.h"
#include "libavcodec/vdpau.h"
#include "threads/CriticalSection.h"
#include "threads/SharedSection.h"
#include "settings/VideoSettings.h"
#include "guilib/DispResource.h"
#include "threads/Event.h"
namespace Surface { class CSurface; }

#define NUM_OUTPUT_SURFACES                4
#define NUM_VIDEO_SURFACES_MPEG2           10  // (1 frame being decoded, 2 reference)
#define NUM_VIDEO_SURFACES_H264            32 // (1 frame being decoded, up to 16 references)
#define NUM_VIDEO_SURFACES_VC1             10  // (same as MPEG-2)
#define NUM_OUTPUT_SURFACES_FOR_FULLHD     2
#define FULLHD_WIDTH                       1920

class CVDPAU
 : public CDVDVideoCodecFFmpeg::IHardwareDecoder
 , public IDispResource
{
public:

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

  CVDPAU();
  virtual ~CVDPAU();
  
  virtual bool Open      (AVCodecContext* avctx, const enum PixelFormat, unsigned int surfaces = 0);
  virtual int  Decode    (AVCodecContext* avctx, AVFrame* frame);
  virtual bool GetPicture(AVCodecContext* avctx, AVFrame* frame, DVDVideoPicture* picture);
  virtual void Reset();
  virtual void Close();

  virtual int  Check(AVCodecContext* avctx);

  virtual const std::string Name() { return "vdpau"; }

  bool MakePixmap(int width, int height);
  bool MakePixmapGL();

  void ReleasePixmap();
  void BindPixmap();

  PFNGLXBINDTEXIMAGEEXTPROC    glXBindTexImageEXT;
  PFNGLXRELEASETEXIMAGEEXTPROC glXReleaseTexImageEXT;
  GLXPixmap  m_glPixmap;
  Pixmap  m_Pixmap;

  static void             FFReleaseBuffer(AVCodecContext *avctx, AVFrame *pic);
  static void             FFDrawSlice(struct AVCodecContext *s,
                               const AVFrame *src, int offset[4],
                               int y, int type, int height);
  static int              FFGetBuffer(AVCodecContext *avctx, AVFrame *pic);

  void Present();
  bool ConfigVDPAU(AVCodecContext *avctx, int ref_frames);
  void SpewHardwareAvailable();
  void InitCSCMatrix(int Height);
  bool CheckStatus(VdpStatus vdp_st, int line);
  bool IsSurfaceValid(vdpau_render_state *render);

  void CheckFeatures();
  void SetColor();
  void SetNoiseReduction();
  void SetSharpness();
  void SetDeinterlacing();
  void SetHWUpscaling();

  pictureAge picAge;
  vdpau_render_state *past[2], *current, *future;
  int        tmpDeintMode, tmpDeintGUI, tmpDeint;
  float      tmpNoiseReduction, tmpSharpness;
  float      tmpBrightness, tmpContrast;
  int        OutWidth, OutHeight;
  bool       upScale;
  std::queue<DVDVideoPicture> m_DVDVideoPics;

  static inline void ClearUsedForRender(vdpau_render_state **st)
  {
    if (*st) {
      (*st)->state &= ~FF_VDPAU_STATE_USED_FOR_RENDER;
      *st = NULL;
    }
  }

  VdpProcamp    m_Procamp;
  VdpCSCMatrix  m_CSCMatrix;
  VdpDevice     HasDevice() { return vdp_device != VDP_INVALID_HANDLE; };
  VdpChromaType vdp_chroma_type;


  //  protected:
  void      InitVDPAUProcs();
  void      FiniVDPAUProcs();
  void      FiniVDPAUOutput();
  bool      ConfigOutputMethod(AVCodecContext *avctx, AVFrame *pFrame);
  bool      FiniOutputMethod();

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
  VdpVideoMixerQueryFeatureSupport *   vdp_video_mixer_query_feature_support;
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

  VdpOutputSurface  outputSurfaces[NUM_OUTPUT_SURFACES];
  VdpOutputSurface  outputSurface;
  VdpOutputSurface  presentSurface;

  VdpDecoder    decoder;
  VdpVideoMixer videoMixer;
  VdpRect       outRect;
  VdpRect       outRectVid;

  static void* dl_handle;
  VdpStatus (*dl_vdp_device_create_x11)(Display* display, int screen, VdpDevice* device, VdpGetProcAddress **get_proc_address);
  VdpStatus (*dl_vdp_get_proc_address)(VdpDevice device, VdpFuncId function_id, void** function_pointer);
  VdpStatus (*dl_vdp_preemption_callback_register)(VdpDevice device, VdpPreemptionCallback callback, void* context);

  int      surfaceNum;
  int      presentSurfaceNum;
  int      totalAvailableOutputSurfaces;
  uint32_t vid_width, vid_height;
  int      surface_width, surface_height;
  uint32_t max_references;
  Display* m_Display;
  bool     vdpauConfigured;
  uint32_t *m_BlackBar;


  VdpVideoMixerPictureStructure m_mixerfield;
  int                           m_mixerstep;

  bool Supports(VdpVideoMixerFeature feature);
  bool Supports(EINTERLACEMETHOD method);
  EINTERLACEMETHOD AutoInterlaceMethod();

  VdpVideoMixerFeature m_features[14];
  int                  m_feature_count;

  static bool IsVDPAUFormat(PixelFormat fmt);
  static void ReadFormatOf( PixelFormat fmt
                          , VdpDecoderProfile &decoder_profile
                          , VdpChromaType     &chroma_type);

  std::vector<vdpau_render_state*> m_videoSurfaces;
  DllAvUtil   m_dllAvUtil;

  enum VDPAUOutputMethod
  {
    OUTPUT_NONE,
    OUTPUT_PIXMAP,
    OUTPUT_GL_INTEROP_RGB,
    OUTPUT_GL_INTEROP_YUV
  };
  VDPAUOutputMethod m_vdpauOutputMethod;

  // OnLostDevice triggers transition from all states to LOST
  // internal errors trigger transition from OPEN to RESET
  // OnResetDevice triggers transition from LOST to RESET
  enum EDisplayState
  { VDPAU_OPEN
  , VDPAU_RESET
  , VDPAU_LOST
  , VDPAU_ERROR
  } m_DisplayState;
  CSharedSection m_DecoderSection;
  CSharedSection m_DisplaySection;
  CEvent         m_DisplayEvent;
  virtual void OnLostDevice();
  virtual void OnResetDevice();
};
