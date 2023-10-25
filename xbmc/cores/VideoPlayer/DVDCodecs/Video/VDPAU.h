/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

/**
 * design goals:
 * - improve performance
 *   max out hw resources: e.g. make 1080p60 play on ION2
 *   allow advanced de-interlacing on ION
 *
 * - add vdpau/opengl interop
 *
 * - remove tight dependency to render thread
 *   prior design needed to hijack render thread in order to do
 *   gl interop functions. In particular this was a problem for
 *   init and clear down. Introduction of GL_NV_vdpau_interop
 *   increased the need to be independent from render thread
 *
 * - move to an actor based design in order to reduce the number
 *   of locks needed.
 */

#include "cores/VideoPlayer/Buffers/VideoBuffer.h"
#include "cores/VideoPlayer/DVDCodecs/Video/DVDVideoCodec.h"
#include "cores/VideoSettings.h"
#include "guilib/DispResource.h"
#include "threads/CriticalSection.h"
#include "threads/Event.h"
#include "threads/SharedSection.h"
#include "threads/Thread.h"
#include "utils/ActorProtocol.h"
#include "utils/Geometry.h"

#include <deque>
#include <list>
#include <map>
#include <mutex>
#include <utility>
#include <vector>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

extern "C" {
#include <libavutil/avutil.h>
#include <libavcodec/vdpau.h>
}

class CProcessInfo;

namespace VDPAU
{

/**
 * VDPAU interface to driver
 */

struct VDPAU_procs
{
  VdpGetProcAddress*vdp_get_proc_address;
  VdpDeviceDestroy*                    vdp_device_destroy;

  VdpVideoSurfaceCreate* vdp_video_surface_create;
  VdpVideoSurfaceDestroy* vdp_video_surface_destroy;
  VdpVideoSurfacePutBitsYCbCr* vdp_video_surface_put_bits_y_cb_cr;
  VdpVideoSurfaceGetBitsYCbCr* vdp_video_surface_get_bits_y_cb_cr;

  VdpOutputSurfacePutBitsYCbCr* vdp_output_surface_put_bits_y_cb_cr;
  VdpOutputSurfacePutBitsNative* vdp_output_surface_put_bits_native;
  VdpOutputSurfaceCreate* vdp_output_surface_create;
  VdpOutputSurfaceDestroy* vdp_output_surface_destroy;
  VdpOutputSurfaceGetBitsNative* vdp_output_surface_get_bits_native;
  VdpOutputSurfaceRenderOutputSurface* vdp_output_surface_render_output_surface;
  VdpOutputSurfacePutBitsIndexed* vdp_output_surface_put_bits_indexed;

  VdpVideoMixerCreate* vdp_video_mixer_create;
  VdpVideoMixerSetFeatureEnables* vdp_video_mixer_set_feature_enables;
  VdpVideoMixerQueryParameterSupport* vdp_video_mixer_query_parameter_support;
  VdpVideoMixerQueryFeatureSupport* vdp_video_mixer_query_feature_support;
  VdpVideoMixerDestroy* vdp_video_mixer_destroy;
  VdpVideoMixerRender* vdp_video_mixer_render;
  VdpVideoMixerSetAttributeValues* vdp_video_mixer_set_attribute_values;

  VdpGenerateCSCMatrix*  vdp_generate_csc_matrix;

  VdpGetErrorString* vdp_get_error_string;

  VdpDecoderCreate* vdp_decoder_create;
  VdpDecoderDestroy* vdp_decoder_destroy;
  VdpDecoderRender* vdp_decoder_render;
  VdpDecoderQueryCapabilities* vdp_decoder_query_caps;

  VdpPreemptionCallbackRegister* vdp_preemption_callback_register;
};

//-----------------------------------------------------------------------------
// VDPAU data structs
//-----------------------------------------------------------------------------

class CDecoder;

/**
 * Buffer statistics used to control number of frames in queue
 */

class CVdpauBufferStats
{
public:
  uint16_t decodedPics;
  uint16_t processedPics;
  uint16_t renderPics;
  uint64_t latency; // time decoder has waited for a frame, ideally there is no latency
  int codecFlags;
  bool canSkipDeint;
  bool draining;

  void IncDecoded()
  {
    std::unique_lock<CCriticalSection> l(m_sec);
    decodedPics++;
  }
  void DecDecoded()
  {
    std::unique_lock<CCriticalSection> l(m_sec);
    decodedPics--;
  }
  void IncProcessed()
  {
    std::unique_lock<CCriticalSection> l(m_sec);
    processedPics++;
  }
  void DecProcessed()
  {
    std::unique_lock<CCriticalSection> l(m_sec);
    processedPics--;
  }
  void IncRender()
  {
    std::unique_lock<CCriticalSection> l(m_sec);
    renderPics++;
  }
  void DecRender()
  {
    std::unique_lock<CCriticalSection> l(m_sec);
    renderPics--;
  }
  void Reset()
  {
    std::unique_lock<CCriticalSection> l(m_sec);
    decodedPics = 0;
    processedPics = 0;
    renderPics = 0;
    latency = 0;
  }
  void Get(uint16_t& decoded, uint16_t& processed, uint16_t& render)
  {
    std::unique_lock<CCriticalSection> l(m_sec);
    decoded = decodedPics, processed = processedPics, render = renderPics;
  }
  void SetParams(uint64_t time, int flags)
  {
    std::unique_lock<CCriticalSection> l(m_sec);
    latency = time;
    codecFlags = flags;
  }
  void GetParams(uint64_t& lat, int& flags)
  {
    std::unique_lock<CCriticalSection> l(m_sec);
    lat = latency;
    flags = codecFlags;
  }
  void SetCanSkipDeint(bool canSkip)
  {
    std::unique_lock<CCriticalSection> l(m_sec);
    canSkipDeint = canSkip;
  }
  bool CanSkipDeint()
  {
    std::unique_lock<CCriticalSection> l(m_sec);
    if (canSkipDeint)
      return true;
    else
      return false;
  }
  void SetDraining(bool drain)
  {
    std::unique_lock<CCriticalSection> l(m_sec);
    draining = drain;
  }
  bool IsDraining()
  {
    std::unique_lock<CCriticalSection> l(m_sec);
    if (draining)
      return true;
    else
      return false;
  }

private:
  CCriticalSection m_sec;
};

/**
 *  CVdpauConfig holds all configuration parameters needed by vdpau
 *  The structure is sent to the internal classes CMixer and COutput
 *  for init.
 */

class CVideoSurfaces;
class CVDPAUContext;

struct CVdpauConfig
{
  int surfaceWidth;
  int surfaceHeight;
  int vidWidth;
  int vidHeight;
  int outWidth;
  int outHeight;
  VdpDecoder vdpDecoder;
  VdpChromaType vdpChromaType;
  CVdpauBufferStats *stats;
  CDecoder *vdpau;
  int upscale;
  CVideoSurfaces *videoSurfaces;
  int numRenderBuffers;
  uint32_t maxReferences;
  bool useInteropYuv;
  CVDPAUContext *context;
  CProcessInfo *processInfo;
  int resetCounter;
  uint64_t timeOpened;
};

/**
 * Holds a decoded frame
 * Input to COutput for further processing
 */
struct CVdpauDecodedPicture
{
  CVdpauDecodedPicture() = default;
  CVdpauDecodedPicture(const CVdpauDecodedPicture &rhs)
  {
    *this = rhs;
  }
  CVdpauDecodedPicture& operator=(const CVdpauDecodedPicture& rhs)
  {
    DVDPic.SetParams(rhs.DVDPic);
    videoSurface = rhs.videoSurface;
    isYuv = rhs.isYuv;
    return *this;
  };
  VideoPicture DVDPic;
  VdpVideoSurface videoSurface;
  bool isYuv;
};

/**
 * Frame after having been processed by vdpau mixer
 */
struct CVdpauProcessedPicture
{
  CVdpauProcessedPicture() = default;
  CVdpauProcessedPicture(const CVdpauProcessedPicture& rhs)
  {
    *this = rhs;
  }
  CVdpauProcessedPicture& operator=(const CVdpauProcessedPicture& rhs)
  {
    DVDPic.SetParams(rhs.DVDPic);
    videoSurface = rhs.videoSurface;
    outputSurface = rhs.outputSurface;
    crop = rhs.crop;
    isYuv = rhs.isYuv;
    id = rhs.id;
    return *this;
  };

  VideoPicture DVDPic;
  VdpVideoSurface videoSurface = VDP_INVALID_HANDLE;
  VdpOutputSurface outputSurface = VDP_INVALID_HANDLE;
  bool crop;
  bool isYuv;
  int id = 0;
};

class CVdpauRenderPicture : public CVideoBuffer
{
public:
  explicit CVdpauRenderPicture(int id) : CVideoBuffer(id) { }
  VideoPicture DVDPic;
  CVdpauProcessedPicture procPic;
  int width;
  int height;
  CRect crop;
  void *device;
  void *procFunc;
  int64_t ident;
};

//-----------------------------------------------------------------------------
// Mixer
//-----------------------------------------------------------------------------

class CMixerControlProtocol : public Actor::Protocol
{
public:
  CMixerControlProtocol(std::string name, CEvent* inEvent, CEvent* outEvent)
    : Protocol(std::move(name), inEvent, outEvent)
  {
  }
  enum OutSignal
  {
    INIT = 0,
    FLUSH,
    TIMEOUT,
  };
  enum InSignal
  {
    ACC,
    ERROR,
  };
};

class CMixerDataProtocol : public Actor::Protocol
{
public:
  CMixerDataProtocol(std::string name, CEvent* inEvent, CEvent* outEvent)
    : Protocol(std::move(name), inEvent, outEvent)
  {
  }
  enum OutSignal
  {
    FRAME,
    BUFFER,
  };
  enum InSignal
  {
    PICTURE,
  };
};

/**
 * Embeds the vdpau video mixer
 * Embedded by COutput class, gets decoded frames from COutput, processes
 * them in mixer ands sends processed frames back to COutput
 */
class CMixer : private CThread
{
public:
  explicit CMixer(CEvent *inMsgEvent);
  ~CMixer() override;
  void Start();
  void Dispose();
  bool IsActive();
  CMixerControlProtocol m_controlPort;
  CMixerDataProtocol m_dataPort;
protected:
  void OnStartup() override;
  void OnExit() override;
  void Process() override;
  void StateMachine(int signal, Actor::Protocol *port, Actor::Message *msg);
  void Init();
  void Uninit();
  void Flush();
  void CreateVdpauMixer();
  void ProcessPicture();
  void InitCycle();
  void FiniCycle();
  void CheckFeatures();
  void SetPostProcFeatures(bool postProcEnabled);
  void PostProcOff();
  void InitCSCMatrix(int Width);
  bool GenerateStudioCSCMatrix(VdpColorStandard colorStandard, VdpCSCMatrix &studioCSCMatrix);
  void SetColor();
  void SetNoiseReduction();
  void SetSharpness();
  void SetDeintSkipChroma();
  void SetDeinterlacing();
  void SetHWUpscaling();
  void DisableHQScaling();
  std::string GetDeintStrFromInterlaceMethod(EINTERLACEMETHOD method);
  bool CheckStatus(VdpStatus vdp_st, int line);
  CEvent m_outMsgEvent;
  CEvent *m_inMsgEvent;
  int m_state;
  bool m_bStateMachineSelfTrigger;

  // extended state variables for state machine
  int m_extTimeout;
  bool m_vdpError;
  CVdpauConfig m_config;
  VdpVideoMixer m_videoMixer;
  VdpProcamp m_Procamp;
  VdpCSCMatrix  m_CSCMatrix;
  bool m_PostProc;
  float m_Brightness;
  float m_Contrast;
  float m_NoiseReduction;
  float m_Sharpness;
  int m_Deint;
  int m_Upscale;
  bool m_SeenInterlaceFlag;
  unsigned int m_ColorMatrix       : 4;
  VdpVideoMixerPictureStructure m_mixerfield;
  int m_mixerstep;
  int m_mixersteps;
  CVdpauProcessedPicture m_processPicture;
  std::queue<VdpOutputSurface> m_outputSurfaces;
  std::queue<CVdpauDecodedPicture> m_decodedPics;
  std::deque<CVdpauDecodedPicture> m_mixerInput;
};

//-----------------------------------------------------------------------------
// Output
//-----------------------------------------------------------------------------

class COutputControlProtocol : public Actor::Protocol
{
public:
  COutputControlProtocol(std::string name, CEvent* inEvent, CEvent* outEvent)
    : Actor::Protocol(std::move(name), inEvent, outEvent)
  {
  }
  enum OutSignal
  {
    INIT,
    FLUSH,
    PRECLEANUP,
    TIMEOUT,
  };
  enum InSignal
  {
    ACC,
    ERROR,
    STATS,
  };
};

class COutputDataProtocol : public Actor::Protocol
{
public:
  COutputDataProtocol(std::string name, CEvent* inEvent, CEvent* outEvent)
    : Actor::Protocol(std::move(name), inEvent, outEvent)
  {
  }
  enum OutSignal
  {
    NEWFRAME = 0,
    RETURNPIC,
  };
  enum InSignal
  {
    PICTURE,
  };
};

/**
 * COutput is embedded in CDecoder and embeds CMixer
 * The class has its own OpenGl context which is shared with render thread
 * COutput generated ready to render textures and passes them back to
 * CDecoder
 */
class CVdpauBufferPool;

class COutput : private CThread
{
public:
  COutput(CDecoder &decoder, CEvent *inMsgEvent);
  ~COutput() override;
  void Start();
  void Dispose();
  COutputControlProtocol m_controlPort;
  COutputDataProtocol m_dataPort;
protected:
  void OnStartup() override;
  void OnExit() override;
  void Process() override;
  void StateMachine(int signal, Actor::Protocol *port, Actor::Message *msg);
  bool HasWork();
  CVdpauRenderPicture *ProcessMixerPicture();
  void QueueReturnPicture(CVdpauRenderPicture *pic);
  void ProcessReturnPicture(CVdpauRenderPicture *pic);
  void ProcessSyncPicture();
  bool Init();
  bool Uninit();
  void Flush();
  bool EnsureBufferPool();
  void ReleaseBufferPool();
  void PreCleanup();
  void InitMixer();
  bool CheckStatus(VdpStatus vdp_st, int line);
  CEvent m_outMsgEvent;
  CEvent *m_inMsgEvent;
  int m_state;
  bool m_bStateMachineSelfTrigger;
  CDecoder &m_vdpau;

  // extended state variables for state machine
  int m_extTimeout;
  bool m_vdpError;
  CVdpauConfig m_config;
  std::shared_ptr<CVdpauBufferPool> m_bufferPool;
  CMixer m_mixer;
};

//-----------------------------------------------------------------------------
// VDPAU Video Surface states
//-----------------------------------------------------------------------------

class CVideoSurfaces
{
public:
  void AddSurface(VdpVideoSurface surf);
  void ClearReference(VdpVideoSurface surf);
  bool MarkRender(VdpVideoSurface surf);
  void ClearRender(VdpVideoSurface surf);
  bool IsValid(VdpVideoSurface surf);
  VdpVideoSurface GetFree(VdpVideoSurface surf);
  VdpVideoSurface RemoveNext(bool skiprender = false);
  void Reset();
  int Size();
protected:
  std::map<VdpVideoSurface, int> m_state;
  std::list<VdpVideoSurface> m_freeSurfaces;
  CCriticalSection m_section;
};

//-----------------------------------------------------------------------------
// VDPAU decoder
//-----------------------------------------------------------------------------

class CVDPAUContext
{
public:
  static bool EnsureContext(CVDPAUContext **ctx);
  void Release();
  VDPAU_procs& GetProcs();
  VdpDevice GetDevice();
  bool Supports(VdpVideoMixerFeature feature);
  VdpVideoMixerFeature* GetFeatures();
  int GetFeatureCount();
private:
  CVDPAUContext();
  void Close();
  bool LoadSymbols();
  bool CreateContext();
  void DestroyContext();
  void QueryProcs();
  void SpewHardwareAvailable();
  static CVDPAUContext *m_context;
  static CCriticalSection m_section;
  static Display *m_display;
  int m_refCount;
  VdpVideoMixerFeature m_vdpFeatures[14];
  int m_featureCount;
  static void *m_dlHandle;
  VdpDevice m_vdpDevice;
  VDPAU_procs m_vdpProcs;
  VdpStatus (*dl_vdp_device_create_x11)(Display* display, int screen, VdpDevice* device, VdpGetProcAddress **get_proc_address);
};

/**
 *  VDPAU main class
 */
class CDecoder
 : public IHardwareDecoder
 , public IDispResource
{
   friend class CVdpauBufferPool;

public:

  struct Desc
  {
    const char *name;
    uint32_t id;
    uint32_t aux; /* optional extra parameter... */
  };

  explicit CDecoder(CProcessInfo& processInfo);
  ~CDecoder() override;

  bool Open (AVCodecContext* avctx, AVCodecContext* mainctx, const enum AVPixelFormat) override;
  CDVDVideoCodec::VCReturn Decode (AVCodecContext* avctx, AVFrame* frame) override;
  bool GetPicture(AVCodecContext* avctx, VideoPicture* picture) override;
  void Reset() override;
  virtual void Close();
  long Release() override;
  bool CanSkipDeint() override;
  unsigned GetAllowedReferences() override { return 5; }

  CDVDVideoCodec::VCReturn Check(AVCodecContext* avctx) override;
  const std::string Name() override { return "vdpau"; }
  void SetCodecControl(int flags) override;

  bool Supports(VdpVideoMixerFeature feature);
  static bool IsVDPAUFormat(AVPixelFormat fmt);

  static void FFReleaseBuffer(void *opaque, uint8_t *data);
  static int FFGetBuffer(AVCodecContext *avctx, AVFrame *pic, int flags);
  static int Render(struct AVCodecContext *s, struct AVFrame *src,
                    const VdpPictureInfo *info, uint32_t buffers_used,
                    const VdpBitstreamBuffer *buffers);

  void OnLostDisplay() override;
  void OnResetDisplay() override;

  static IHardwareDecoder* Create(CDVDStreamInfo &hint, CProcessInfo &processInfo, AVPixelFormat fmt);
  static void Register();

protected:
  void SetWidthHeight(int width, int height);
  bool ConfigVDPAU(AVCodecContext *avctx, int ref_frames);
  bool CheckStatus(VdpStatus vdp_st, int line);
  void FiniVDPAUOutput();
  void ReturnRenderPicture(CVdpauRenderPicture *renderPic);
  long ReleasePicReference();

  static void ReadFormatOf( AVCodecID codec
                          , VdpDecoderProfile &decoder_profile
                          , VdpChromaType &chroma_type);

  // OnLostDevice triggers transition from all states to LOST
  // internal errors trigger transition from OPEN to RESET
  // OnResetDevice triggers transition from LOST to RESET
  enum EDisplayState
  { VDPAU_OPEN
  , VDPAU_RESET
  , VDPAU_LOST
  , VDPAU_ERROR
  } m_DisplayState;
  CCriticalSection m_DecoderSection;
  CEvent m_DisplayEvent;
  int m_ErrorCount;

  bool m_vdpauConfigured;
  CVdpauConfig m_vdpauConfig;
  CVideoSurfaces m_videoSurfaces;
  AVVDPAUContext m_hwContext;

  COutput m_vdpauOutput;
  CVdpauBufferStats m_bufferStats;
  CEvent m_inMsgEvent;
  CVdpauRenderPicture *m_presentPicture = nullptr;

  int m_codecControl;
  CProcessInfo& m_processInfo;

  static bool m_capGeneral;
};

}
