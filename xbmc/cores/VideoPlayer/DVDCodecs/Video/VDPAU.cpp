/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://kodi.tv
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

#include "VDPAU.h"
#include "ServiceBroker.h"
#include <dlfcn.h>
#include "windowing/X11/WinSystemX11.h"
#include "guilib/GraphicContext.h"
#include "guilib/TextureManager.h"
#include "cores/VideoPlayer/DVDCodecs/DVDFactoryCodec.h"
#include "cores/VideoPlayer/Process/ProcessInfo.h"
#include "cores/VideoPlayer/VideoRenderers/RenderManager.h"
#include "cores/VideoPlayer/Interface/Addon/TimingConstants.h"
#include "rendering/RenderSystem.h"
#include "settings/Settings.h"
#include "settings/AdvancedSettings.h"
#include "Application.h"
#include "utils/MathUtils.h"
#include "utils/TimeUtils.h"
#include "DVDCodecs/DVDCodecUtils.h"
#include "cores/VideoPlayer/VideoRenderers/RenderFlags.h"
#include "utils/log.h"

using namespace Actor;
using namespace VDPAU;
#define NUM_RENDER_PICS 7
#define NUM_CROP_PIX 3

#define ARSIZE(x) (sizeof(x) / sizeof((x)[0]))

CDecoder::Desc decoder_profiles[] = {
{"MPEG1",        VDP_DECODER_PROFILE_MPEG1},
{"MPEG2_SIMPLE", VDP_DECODER_PROFILE_MPEG2_SIMPLE},
{"MPEG2_MAIN",   VDP_DECODER_PROFILE_MPEG2_MAIN},
{"H264_BASELINE",VDP_DECODER_PROFILE_H264_BASELINE},
{"H264_MAIN",    VDP_DECODER_PROFILE_H264_MAIN},
{"H264_HIGH",    VDP_DECODER_PROFILE_H264_HIGH},
{"VC1_SIMPLE",   VDP_DECODER_PROFILE_VC1_SIMPLE},
{"VC1_MAIN",     VDP_DECODER_PROFILE_VC1_MAIN},
{"VC1_ADVANCED", VDP_DECODER_PROFILE_VC1_ADVANCED},
{"MPEG4_PART2_ASP", VDP_DECODER_PROFILE_MPEG4_PART2_ASP},
#ifdef VDP_DECODER_PROFILE_HEVC_MAIN
{"HEVC_MAIN", VDP_DECODER_PROFILE_HEVC_MAIN},
#endif
};
const size_t decoder_profile_count = sizeof(decoder_profiles)/sizeof(CDecoder::Desc);

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

static float studioCSCKCoeffs601[3] = {0.299, 0.587, 0.114}; //BT601 {Kr, Kg, Kb}
static float studioCSCKCoeffs709[3] = {0.2126, 0.7152, 0.0722}; //BT709 {Kr, Kg, Kb}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

CVDPAUContext *CVDPAUContext::m_context = 0;
CCriticalSection CVDPAUContext::m_section;
Display *CVDPAUContext::m_display = 0;
void *CVDPAUContext::m_dlHandle = 0;

CVDPAUContext::CVDPAUContext()
{
  m_context = 0;
  m_refCount = 0;
}

void CVDPAUContext::Release()
{
  CSingleLock lock(m_section);

  m_refCount--;
  if (m_refCount <= 0)
  {
    Close();
    delete this;
    m_context = 0;
  }
}

void CVDPAUContext::Close()
{
  CLog::Log(LOGNOTICE, "VDPAU::Close - closing decoder context");
  DestroyContext();
}

bool CVDPAUContext::EnsureContext(CVDPAUContext **ctx)
{
  CSingleLock lock(m_section);

  if (m_context)
  {
    m_context->m_refCount++;
    *ctx = m_context;
    return true;
  }

  m_context = new CVDPAUContext();
  *ctx = m_context;
  {
    CSingleLock gLock(g_graphicsContext);
    if (!m_context->LoadSymbols() || !m_context->CreateContext())
    {
      delete m_context;
      m_context = 0;
      *ctx = NULL;
      return false;
    }
  }

  m_context->m_refCount++;

  *ctx = m_context;
  return true;
}

VDPAU_procs& CVDPAUContext::GetProcs()
{
  return m_vdpProcs;
}

VdpVideoMixerFeature* CVDPAUContext::GetFeatures()
{
  return m_vdpFeatures;
}

int CVDPAUContext::GetFeatureCount()
{
  return m_featureCount;
}

bool CVDPAUContext::LoadSymbols()
{
  if (!m_dlHandle)
  {
    m_dlHandle  = dlopen("libvdpau.so.1", RTLD_LAZY);
    if (!m_dlHandle)
    {
      const char* error = dlerror();
      if (!error)
        error = "dlerror() returned NULL";

      CLog::Log(LOGERROR,"VDPAU::LoadSymbols: Unable to get handle to lib: %s", error);
      return false;
    }
  }

  char* error;
  (void)dlerror();
  dl_vdp_device_create_x11 = (VdpStatus (*)(Display*, int, VdpDevice*, VdpStatus (**)(VdpDevice, VdpFuncId, void**)))dlsym(m_dlHandle, (const char*)"vdp_device_create_x11");
  error = dlerror();
  if (error)
  {
    CLog::Log(LOGERROR,"(VDPAU) - %s in %s",error,__FUNCTION__);
    m_vdpDevice = VDP_INVALID_HANDLE;
    return false;
  }
  return true;
}

bool CVDPAUContext::CreateContext()
{
  CLog::Log(LOGNOTICE,"VDPAU::CreateContext - creating decoder context");

  int mScreen;
  { CSingleLock lock(g_graphicsContext);

    if (!m_display)
      m_display = XOpenDisplay(NULL);

    if (!m_display)
      return false;

    mScreen = CServiceBroker::GetWinSystem().GetCurrentScreen();
  }

  VdpStatus vdp_st;
  // Create Device
  vdp_st = dl_vdp_device_create_x11(m_display,
                                    mScreen,
                                   &m_vdpDevice,
                                   &m_vdpProcs.vdp_get_proc_address);

  CLog::Log(LOGNOTICE,"vdp_device = 0x%08x vdp_st = 0x%08x",m_vdpDevice,vdp_st);
  if (vdp_st != VDP_STATUS_OK)
  {
    CLog::Log(LOGERROR,"(VDPAU) unable to init VDPAU - vdp_st = 0x%x.  Falling back.",vdp_st);
    m_vdpDevice = VDP_INVALID_HANDLE;
    return false;
  }

  QueryProcs();
  SpewHardwareAvailable();
  return true;
}

void CVDPAUContext::QueryProcs()
{
  VdpStatus vdp_st;

#define VDP_PROC(id, proc) \
  do { \
    vdp_st = m_vdpProcs.vdp_get_proc_address(m_vdpDevice, id, (void**)&proc); \
    if (vdp_st != VDP_STATUS_OK) \
    { \
      CLog::Log(LOGERROR, "CVDPAUContext::GetProcs - failed to get proc id"); \
    } \
  } while(0);

  VDP_PROC(VDP_FUNC_ID_GET_ERROR_STRING                    , m_vdpProcs.vdp_get_error_string);
  VDP_PROC(VDP_FUNC_ID_DEVICE_DESTROY                      , m_vdpProcs.vdp_device_destroy);
  VDP_PROC(VDP_FUNC_ID_GENERATE_CSC_MATRIX                 , m_vdpProcs.vdp_generate_csc_matrix);
  VDP_PROC(VDP_FUNC_ID_VIDEO_SURFACE_CREATE                , m_vdpProcs.vdp_video_surface_create);
  VDP_PROC(VDP_FUNC_ID_VIDEO_SURFACE_DESTROY               , m_vdpProcs.vdp_video_surface_destroy);
  VDP_PROC(VDP_FUNC_ID_VIDEO_SURFACE_PUT_BITS_Y_CB_CR      , m_vdpProcs.vdp_video_surface_put_bits_y_cb_cr);
  VDP_PROC(VDP_FUNC_ID_VIDEO_SURFACE_GET_BITS_Y_CB_CR      , m_vdpProcs.vdp_video_surface_get_bits_y_cb_cr);
  VDP_PROC(VDP_FUNC_ID_OUTPUT_SURFACE_PUT_BITS_Y_CB_CR     , m_vdpProcs.vdp_output_surface_put_bits_y_cb_cr);
  VDP_PROC(VDP_FUNC_ID_OUTPUT_SURFACE_PUT_BITS_NATIVE      , m_vdpProcs.vdp_output_surface_put_bits_native);
  VDP_PROC(VDP_FUNC_ID_OUTPUT_SURFACE_CREATE               , m_vdpProcs.vdp_output_surface_create);
  VDP_PROC(VDP_FUNC_ID_OUTPUT_SURFACE_DESTROY              , m_vdpProcs.vdp_output_surface_destroy);
  VDP_PROC(VDP_FUNC_ID_OUTPUT_SURFACE_GET_BITS_NATIVE      , m_vdpProcs.vdp_output_surface_get_bits_native);
  VDP_PROC(VDP_FUNC_ID_OUTPUT_SURFACE_RENDER_OUTPUT_SURFACE, m_vdpProcs.vdp_output_surface_render_output_surface);
  VDP_PROC(VDP_FUNC_ID_OUTPUT_SURFACE_PUT_BITS_INDEXED     , m_vdpProcs.vdp_output_surface_put_bits_indexed);
  VDP_PROC(VDP_FUNC_ID_VIDEO_MIXER_CREATE                  , m_vdpProcs.vdp_video_mixer_create);
  VDP_PROC(VDP_FUNC_ID_VIDEO_MIXER_SET_FEATURE_ENABLES     , m_vdpProcs.vdp_video_mixer_set_feature_enables);
  VDP_PROC(VDP_FUNC_ID_VIDEO_MIXER_DESTROY                 , m_vdpProcs.vdp_video_mixer_destroy);
  VDP_PROC(VDP_FUNC_ID_VIDEO_MIXER_RENDER                  , m_vdpProcs.vdp_video_mixer_render);
  VDP_PROC(VDP_FUNC_ID_VIDEO_MIXER_SET_ATTRIBUTE_VALUES    , m_vdpProcs.vdp_video_mixer_set_attribute_values);
  VDP_PROC(VDP_FUNC_ID_VIDEO_MIXER_QUERY_PARAMETER_SUPPORT , m_vdpProcs.vdp_video_mixer_query_parameter_support);
  VDP_PROC(VDP_FUNC_ID_VIDEO_MIXER_QUERY_FEATURE_SUPPORT   , m_vdpProcs.vdp_video_mixer_query_feature_support);
  VDP_PROC(VDP_FUNC_ID_DECODER_CREATE                      , m_vdpProcs.vdp_decoder_create);
  VDP_PROC(VDP_FUNC_ID_DECODER_DESTROY                     , m_vdpProcs.vdp_decoder_destroy);
  VDP_PROC(VDP_FUNC_ID_DECODER_RENDER                      , m_vdpProcs.vdp_decoder_render);
  VDP_PROC(VDP_FUNC_ID_DECODER_QUERY_CAPABILITIES          , m_vdpProcs.vdp_decoder_query_caps);
#undef VDP_PROC
}

VdpDevice CVDPAUContext::GetDevice()
{
  return m_vdpDevice;
}

void CVDPAUContext::DestroyContext()
{
  if (!m_vdpProcs.vdp_device_destroy)
    return;

  m_vdpProcs.vdp_device_destroy(m_vdpDevice);
  m_vdpDevice = VDP_INVALID_HANDLE;
}

void CVDPAUContext::SpewHardwareAvailable()  //Copyright (c) 2008 Wladimir J. van der Laan  -- VDPInfo
{
  VdpStatus rv;
  CLog::Log(LOGNOTICE,"VDPAU Decoder capabilities:");
  CLog::Log(LOGNOTICE,"name          level macbs width height");
  CLog::Log(LOGNOTICE,"------------------------------------");
  for(unsigned int x=0; x<decoder_profile_count; ++x)
  {
    VdpBool is_supported = false;
    uint32_t max_level, max_macroblocks, max_width, max_height;
    rv = m_vdpProcs.vdp_decoder_query_caps(m_vdpDevice, decoder_profiles[x].id,
                                &is_supported, &max_level, &max_macroblocks, &max_width, &max_height);
    if(rv == VDP_STATUS_OK && is_supported)
    {
      CLog::Log(LOGNOTICE,"%-16s %2i %5i %5i %5i\n", decoder_profiles[x].name,
                max_level, max_macroblocks, max_width, max_height);
    }
  }
  CLog::Log(LOGNOTICE,"------------------------------------");
  m_featureCount = 0;
#define CHECK_SUPPORT(feature)  \
  do { \
    VdpBool supported; \
    if(m_vdpProcs.vdp_video_mixer_query_feature_support(m_vdpDevice, feature, &supported) == VDP_STATUS_OK && supported) { \
      CLog::Log(LOGNOTICE, "Mixer feature: "#feature);  \
      m_vdpFeatures[m_featureCount++] = feature; \
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

bool CVDPAUContext::Supports(VdpVideoMixerFeature feature)
{
  for(int i = 0; i < m_featureCount; i++)
  {
    if(m_vdpFeatures[i] == feature)
      return true;
  }
  return false;
}

//-----------------------------------------------------------------------------
// VDPAU Video Surface states
//-----------------------------------------------------------------------------

#define SURFACE_USED_FOR_REFERENCE 0x01
#define SURFACE_USED_FOR_RENDER    0x02

void CVideoSurfaces::AddSurface(VdpVideoSurface surf)
{
  CSingleLock lock(m_section);
  m_state[surf] = SURFACE_USED_FOR_REFERENCE;
}

void CVideoSurfaces::ClearReference(VdpVideoSurface surf)
{
  CSingleLock lock(m_section);
  if (m_state.find(surf) == m_state.end())
  {
    CLog::Log(LOGWARNING, "CVideoSurfaces::ClearReference - surface invalid");
    return;
  }
  m_state[surf] &= ~SURFACE_USED_FOR_REFERENCE;
  if (m_state[surf] == 0)
  {
    m_freeSurfaces.push_back(surf);
  }
}

bool CVideoSurfaces::MarkRender(VdpVideoSurface surf)
{
  CSingleLock lock(m_section);
  if (m_state.find(surf) == m_state.end())
  {
    CLog::Log(LOGWARNING, "CVideoSurfaces::MarkRender - surface invalid");
    return false;
  }
  std::list<VdpVideoSurface>::iterator it;
  it = std::find(m_freeSurfaces.begin(), m_freeSurfaces.end(), surf);
  if (it != m_freeSurfaces.end())
  {
    m_freeSurfaces.erase(it);
  }
  m_state[surf] |= SURFACE_USED_FOR_RENDER;
  return true;
}

void CVideoSurfaces::ClearRender(VdpVideoSurface surf)
{
  CSingleLock lock(m_section);
  if (m_state.find(surf) == m_state.end())
  {
    CLog::Log(LOGWARNING, "CVideoSurfaces::ClearRender - surface invalid");
    return;
  }
  m_state[surf] &= ~SURFACE_USED_FOR_RENDER;
  if (m_state[surf] == 0)
  {
    m_freeSurfaces.push_back(surf);
  }
}

bool CVideoSurfaces::IsValid(VdpVideoSurface surf)
{
  CSingleLock lock(m_section);
  if (m_state.find(surf) != m_state.end())
    return true;
  else
    return false;
}

VdpVideoSurface CVideoSurfaces::GetFree(VdpVideoSurface surf)
{
  CSingleLock lock(m_section);
  if (m_state.find(surf) != m_state.end())
  {
    std::list<VdpVideoSurface>::iterator it;
    it = std::find(m_freeSurfaces.begin(), m_freeSurfaces.end(), surf);
    if (it == m_freeSurfaces.end())
    {
      CLog::Log(LOGWARNING, "CVideoSurfaces::GetFree - surface not free");
    }
    else
    {
      m_freeSurfaces.erase(it);
      m_state[surf] = SURFACE_USED_FOR_REFERENCE;
      return surf;
    }
  }

  if (!m_freeSurfaces.empty())
  {
    VdpVideoSurface freeSurf = m_freeSurfaces.front();
    m_freeSurfaces.pop_front();
    m_state[freeSurf] = SURFACE_USED_FOR_REFERENCE;
    return freeSurf;
  }

  return VDP_INVALID_HANDLE;
}

VdpVideoSurface CVideoSurfaces::RemoveNext(bool skiprender)
{
  CSingleLock lock(m_section);
  VdpVideoSurface surf;
  std::map<VdpVideoSurface, int>::iterator it;
  for(it = m_state.begin(); it != m_state.end(); ++it)
  {
    if (skiprender && it->second & SURFACE_USED_FOR_RENDER)
      continue;
    surf = it->first;
    m_state.erase(surf);

    std::list<VdpVideoSurface>::iterator it2;
    it2 = std::find(m_freeSurfaces.begin(), m_freeSurfaces.end(), surf);
    if (it2 != m_freeSurfaces.end())
      m_freeSurfaces.erase(it2);
    return surf;
  }
  return VDP_INVALID_HANDLE;
}

void CVideoSurfaces::Reset()
{
  CSingleLock lock(m_section);
  m_freeSurfaces.clear();
  m_state.clear();
}

int CVideoSurfaces::Size()
{
  CSingleLock lock(m_section);
  return m_state.size();
}

bool CVideoSurfaces::HasRefs()
{
  CSingleLock lock(m_section);
  for (const auto &i : m_state)
  {
    if (i.second & SURFACE_USED_FOR_REFERENCE)
    return true;
  }
  return false;
}

//-----------------------------------------------------------------------------
// CVDPAU
//-----------------------------------------------------------------------------

bool CDecoder::m_capGeneral = false;

CDecoder::CDecoder(CProcessInfo& processInfo) :
    m_vdpauOutput(*this, &m_inMsgEvent), m_processInfo(processInfo)
{
  m_vdpauConfig.videoSurfaces = &m_videoSurfaces;

  m_vdpauConfigured = false;
  m_DisplayState = VDPAU_OPEN;
  m_vdpauConfig.context = 0;
  m_vdpauConfig.processInfo = &m_processInfo;
  m_vdpauConfig.resetCounter = 0;
}

bool CDecoder::Open(AVCodecContext* avctx, AVCodecContext* mainctx, const enum AVPixelFormat fmt)
{
  // check if user wants to decode this format with VDPAU
  std::string gpuvendor = CServiceBroker::GetRenderSystem().GetRenderVendor();
  std::transform(gpuvendor.begin(), gpuvendor.end(), gpuvendor.begin(), ::tolower);
  // nvidia is whitelisted despite for mpeg-4 we need to query user settings
  if ((gpuvendor.compare(0, 6, "nvidia") != 0)  || (avctx->codec_id == AV_CODEC_ID_MPEG4) || (avctx->codec_id == AV_CODEC_ID_H263))
  {
    std::map<AVCodecID, std::string> settings_map = {
      { AV_CODEC_ID_H263, CSettings::SETTING_VIDEOPLAYER_USEVDPAUMPEG4 },
      { AV_CODEC_ID_MPEG4, CSettings::SETTING_VIDEOPLAYER_USEVDPAUMPEG4 },
      { AV_CODEC_ID_WMV3, CSettings::SETTING_VIDEOPLAYER_USEVDPAUVC1 },
      { AV_CODEC_ID_VC1, CSettings::SETTING_VIDEOPLAYER_USEVDPAUVC1 },
      { AV_CODEC_ID_MPEG2VIDEO, CSettings::SETTING_VIDEOPLAYER_USEVDPAUMPEG2 },
    };
    if (CDVDVideoCodec::IsCodecDisabled(settings_map, avctx->codec_id))
      return false;
  }

  if (!CServiceBroker::GetRenderSystem().IsExtSupported("GL_NV_vdpau_interop"))
  {
    CLog::Log(LOGNOTICE, "VDPAU::Open: required extension GL_NV_vdpau_interop not found");
    return false;
  }

  if (avctx->coded_width  == 0 ||
     avctx->coded_height == 0)
  {
    CLog::Log(LOGWARNING,"VDPAU::Open: no width/height available, can't init");
    return false;
  }
  m_vdpauConfig.numRenderBuffers = 5;

  if (!CVDPAUContext::EnsureContext(&m_vdpauConfig.context))
    return false;

  m_DisplayState = VDPAU_OPEN;
  m_vdpauConfigured = false;

  m_presentPicture = 0;

  {
    VdpDecoderProfile profile = 0;

    // convert FFMPEG codec ID to VDPAU profile.
    ReadFormatOf(avctx->codec_id, profile, m_vdpauConfig.vdpChromaType);
    if(profile)
    {
      VdpStatus vdp_st;
      VdpBool is_supported = false;
      uint32_t max_level, max_macroblocks, max_width, max_height;

      // query device capabilities to ensure that VDPAU can handle the requested codec
      vdp_st = m_vdpauConfig.context->GetProcs().vdp_decoder_query_caps(m_vdpauConfig.context->GetDevice(),
               profile, &is_supported, &max_level, &max_macroblocks, &max_width, &max_height);

      // test to make sure there is a possibility the codec will work
      if (CheckStatus(vdp_st, __LINE__))
      {
        CLog::Log(LOGERROR, "VDPAU::Open: error %s(%d) checking for decoder support", m_vdpauConfig.context->GetProcs().vdp_get_error_string(vdp_st), vdp_st);
        return false;
      }

      if (max_width < (uint32_t) avctx->coded_width || max_height < (uint32_t) avctx->coded_height)
      {
        CLog::Log(LOGWARNING,"VDPAU::Open: requested picture dimensions (%i, %i) exceed hardware capabilities ( %i, %i).",
	                      avctx->coded_width, avctx->coded_height, max_width, max_height);
        return false;
      }

      if (!CDVDCodecUtils::IsVP3CompatibleWidth(avctx->coded_width))
        CLog::Log(LOGWARNING,"VDPAU::Open width %i might not be supported because of hardware bug", avctx->width);

      // attempt to create a decoder with this width/height, some sizes are not supported by hw
      vdp_st = m_vdpauConfig.context->GetProcs().vdp_decoder_create(m_vdpauConfig.context->GetDevice(), profile, avctx->coded_width, avctx->coded_height, 5, &m_vdpauConfig.vdpDecoder);

      if (CheckStatus(vdp_st, __LINE__))
      {
        CLog::Log(LOGERROR, "VDPAU::Open: error: %s(%d) checking for decoder support", m_vdpauConfig.context->GetProcs().vdp_get_error_string(vdp_st), vdp_st);
        return false;
      }

      m_vdpauConfig.context->GetProcs().vdp_decoder_destroy(m_vdpauConfig.vdpDecoder);
      CheckStatus(vdp_st, __LINE__);

      // finally setup ffmpeg
      memset(&m_hwContext, 0, sizeof(AVVDPAUContext));
      m_hwContext.render2 = CDecoder::Render;
      avctx->get_buffer2 = CDecoder::FFGetBuffer;
      avctx->slice_flags = SLICE_FLAG_CODED_ORDER|SLICE_FLAG_ALLOW_FIELD;
      avctx->hwaccel_context = &m_hwContext;

      CServiceBroker::GetWinSystem().Register(this);
      m_avctx = mainctx;
      return true;
    }
  }
  return false;
}

CDecoder::~CDecoder()
{
  Close();
}

void CDecoder::Close()
{
  CLog::Log(LOGNOTICE, " (VDPAU) %s", __FUNCTION__);

  CServiceBroker::GetWinSystem().Unregister(this);

  CSingleLock lock(m_DecoderSection);

  FiniVDPAUOutput();
  m_vdpauOutput.Dispose();

  if (m_vdpauConfig.context)
    m_vdpauConfig.context->Release();
  m_vdpauConfig.context = 0;
}

long CDecoder::Release()
{
  // if ffmpeg holds any references, flush buffers
  if (m_avctx && m_videoSurfaces.HasRefs())
  {
    avcodec_flush_buffers(m_avctx);
  }

  // check if we should do some pre-cleanup here
  // a second decoder might need resources
  if (m_vdpauConfigured == true)
  {
    CSingleLock lock(m_DecoderSection);
    CLog::Log(LOGNOTICE,"CVDPAU::Release pre-cleanup");

    Message *reply;
    if (m_vdpauOutput.m_controlPort.SendOutMessageSync(COutputControlProtocol::PRECLEANUP,
                                                   &reply,
                                                   2000))
    {
      bool success = reply->signal == COutputControlProtocol::ACC ? true : false;
      reply->Release();
      if (!success)
      {
        CLog::Log(LOGERROR, "VDPAU::%s - pre-cleanup returned error", __FUNCTION__);
        m_DisplayState = VDPAU_ERROR;
      }
    }
    else
    {
      CLog::Log(LOGERROR, "VDPAU::%s - pre-cleanup timed out", __FUNCTION__);
      m_DisplayState = VDPAU_ERROR;
    }

    VdpVideoSurface surf;
    while((surf = m_videoSurfaces.RemoveNext(true)) != VDP_INVALID_HANDLE)
    {
      m_vdpauConfig.context->GetProcs().vdp_video_surface_destroy(surf);
    }
  }
  return IHardwareDecoder::Release();
}

long CDecoder::ReleasePicReference()
{
  return IHardwareDecoder::Release();
}

void CDecoder::SetWidthHeight(int width, int height)
{
  m_vdpauConfig.upscale = g_advancedSettings.m_videoVDPAUScaling;

  //pick the smallest dimensions, so we downscale with vdpau and upscale with opengl when appropriate
  //this requires the least amount of gpu memory bandwidth
  if (g_graphicsContext.GetWidth() < width || g_graphicsContext.GetHeight() < height || m_vdpauConfig.upscale >= 0)
  {
    //scale width to desktop size if the aspect ratio is the same or bigger than the desktop
    if ((double)height * g_graphicsContext.GetWidth() / width <= (double)g_graphicsContext.GetHeight())
    {
      m_vdpauConfig.outWidth = g_graphicsContext.GetWidth();
      m_vdpauConfig.outHeight = MathUtils::round_int((double)height * g_graphicsContext.GetWidth() / width);
    }
    else //scale height to the desktop size if the aspect ratio is smaller than the desktop
    {
      m_vdpauConfig.outHeight = g_graphicsContext.GetHeight();
      m_vdpauConfig.outWidth = MathUtils::round_int((double)width * g_graphicsContext.GetHeight() / height);
    }
  }
  else
  { //let opengl scale
    m_vdpauConfig.outWidth = width;
    m_vdpauConfig.outHeight = height;
  }
  CLog::Log(LOGDEBUG, LOGVIDEO, "CVDPAU::SetWidthHeight Setting OutWidth: %i OutHeight: %i", m_vdpauConfig.outWidth, m_vdpauConfig.outHeight);
}

void CDecoder::OnLostDisplay()
{
  CLog::Log(LOGNOTICE,"CVDPAU::OnLostDevice event");

  int count = g_graphicsContext.exit();

  CSingleLock lock(m_DecoderSection);
  FiniVDPAUOutput();
  if (m_vdpauConfig.context)
    m_vdpauConfig.context->Release();
  m_vdpauConfig.context = 0;

  m_DisplayState = VDPAU_LOST;
  lock.Leave();
  m_DisplayEvent.Reset();

  g_graphicsContext.restore(count);
}

void CDecoder::OnResetDisplay()
{
  CLog::Log(LOGNOTICE,"CVDPAU::OnResetDevice event");

  int count = g_graphicsContext.exit();

  CSingleLock lock(m_DecoderSection);
  if (m_DisplayState == VDPAU_LOST)
  {
    m_DisplayState = VDPAU_RESET;
    lock.Leave();
    m_DisplayEvent.Set();
  }

  g_graphicsContext.restore(count);
}

CDVDVideoCodec::VCReturn CDecoder::Check(AVCodecContext* avctx)
{
  EDisplayState state;

  { CSingleLock lock(m_DecoderSection);
    state = m_DisplayState;
  }

  if (state == VDPAU_LOST)
  {
    CLog::Log(LOGNOTICE,"CVDPAU::Check waiting for display reset event");
    if (!m_DisplayEvent.WaitMSec(4000))
    {
      CLog::Log(LOGERROR, "CVDPAU::Check - device didn't reset in reasonable time");
      state = VDPAU_RESET;
    }
    else
    {
      CSingleLock lock(m_DecoderSection);
      state = m_DisplayState;
    }
  }
  if (state == VDPAU_RESET || state == VDPAU_ERROR)
  {
    CSingleLock lock(m_DecoderSection);

    avcodec_flush_buffers(avctx);
    FiniVDPAUOutput();
    if (m_vdpauConfig.context)
      m_vdpauConfig.context->Release();
    m_vdpauConfig.context = 0;

    if (CVDPAUContext::EnsureContext(&m_vdpauConfig.context))
    {
      m_DisplayState = VDPAU_OPEN;
      m_vdpauConfigured = false;
    }

    if (state == VDPAU_RESET)
      return CDVDVideoCodec::VC_FLUSHED;
    else
      return CDVDVideoCodec::VC_ERROR;
  }
  return CDVDVideoCodec::VC_NONE;
}

bool CDecoder::IsVDPAUFormat(AVPixelFormat format)
{
  if (format == AV_PIX_FMT_VDPAU)
    return true;
  else
    return false;
}

bool CDecoder::Supports(VdpVideoMixerFeature feature)
{
  return m_vdpauConfig.context->Supports(feature);
}

void CDecoder::FiniVDPAUOutput()
{
  if (!m_vdpauConfigured)
    return;

  CLog::Log(LOGNOTICE, " (VDPAU) %s", __FUNCTION__);

  // uninit output
  m_vdpauOutput.Dispose();
  m_vdpauConfigured = false;

  VdpStatus vdp_st;

  vdp_st = m_vdpauConfig.context->GetProcs().vdp_decoder_destroy(m_vdpauConfig.vdpDecoder);
  if (CheckStatus(vdp_st, __LINE__))
    return;
  m_vdpauConfig.vdpDecoder = VDP_INVALID_HANDLE;

  CLog::Log(LOGDEBUG, LOGVIDEO, "CVDPAU::FiniVDPAUOutput destroying %d video surfaces", m_videoSurfaces.Size());

  VdpVideoSurface surf;
  while((surf = m_videoSurfaces.RemoveNext()) != VDP_INVALID_HANDLE)
  {
    m_vdpauConfig.context->GetProcs().vdp_video_surface_destroy(surf);
    if (CheckStatus(vdp_st, __LINE__))
      return;
  }
  m_videoSurfaces.Reset();
}

void CDecoder::ReadFormatOf( AVCodecID codec
                           , VdpDecoderProfile &vdp_decoder_profile
                           , VdpChromaType     &vdp_chroma_type)
{
  switch (codec)
  {
    case AV_CODEC_ID_MPEG1VIDEO:
      vdp_decoder_profile = VDP_DECODER_PROFILE_MPEG1;
      vdp_chroma_type     = VDP_CHROMA_TYPE_420;
      break;
    case AV_CODEC_ID_MPEG2VIDEO:
      vdp_decoder_profile = VDP_DECODER_PROFILE_MPEG2_MAIN;
      vdp_chroma_type     = VDP_CHROMA_TYPE_420;
      break;
    case AV_CODEC_ID_H264:
      vdp_decoder_profile = VDP_DECODER_PROFILE_H264_HIGH;
      vdp_chroma_type     = VDP_CHROMA_TYPE_420;
      break;
#ifdef VDP_DECODER_PROFILE_HEVC_MAIN
    case AV_CODEC_ID_HEVC:
      vdp_decoder_profile = VDP_DECODER_PROFILE_HEVC_MAIN;
      vdp_chroma_type     = VDP_CHROMA_TYPE_420;
      break;
#endif
    case AV_CODEC_ID_WMV3:
      vdp_decoder_profile = VDP_DECODER_PROFILE_VC1_MAIN;
      vdp_chroma_type     = VDP_CHROMA_TYPE_420;
      break;
    case AV_CODEC_ID_VC1:
      vdp_decoder_profile = VDP_DECODER_PROFILE_VC1_ADVANCED;
      vdp_chroma_type     = VDP_CHROMA_TYPE_420;
      break;
    case AV_CODEC_ID_MPEG4:
      vdp_decoder_profile = VDP_DECODER_PROFILE_MPEG4_PART2_ASP;
      vdp_chroma_type     = VDP_CHROMA_TYPE_420;
      break;
    default:
      vdp_decoder_profile = 0;
      vdp_chroma_type     = 0;
      break;
  }
}

bool CDecoder::ConfigVDPAU(AVCodecContext* avctx, int ref_frames)
{
  FiniVDPAUOutput();

  VdpStatus vdp_st;
  VdpDecoderProfile vdp_decoder_profile;

  m_vdpauConfig.vidWidth = avctx->width;
  m_vdpauConfig.vidHeight = avctx->height;
  m_vdpauConfig.surfaceWidth = avctx->coded_width;
  m_vdpauConfig.surfaceHeight = avctx->coded_height;

  SetWidthHeight(avctx->width,avctx->height);

  CLog::Log(LOGNOTICE, " (VDPAU) screenWidth:%i vidWidth:%i surfaceWidth:%i",m_vdpauConfig.outWidth,m_vdpauConfig.vidWidth,m_vdpauConfig.surfaceWidth);
  CLog::Log(LOGNOTICE, " (VDPAU) screenHeight:%i vidHeight:%i surfaceHeight:%i",m_vdpauConfig.outHeight,m_vdpauConfig.vidHeight,m_vdpauConfig.surfaceHeight);

  ReadFormatOf(avctx->codec_id, vdp_decoder_profile, m_vdpauConfig.vdpChromaType);

  if (avctx->codec_id == AV_CODEC_ID_H264)
  {
    m_vdpauConfig.maxReferences = ref_frames;
    if (m_vdpauConfig.maxReferences > 16) m_vdpauConfig.maxReferences = 16;
    if (m_vdpauConfig.maxReferences < 5)  m_vdpauConfig.maxReferences = 5;
  }
  else if (avctx->codec_id == AV_CODEC_ID_HEVC)
  {
    // The DPB works quite differently in hevc and there isn't  a per-file max
    // reference number, so we force the maximum number (source: upstream ffmpeg)
    m_vdpauConfig.maxReferences = 16;
  }
  else
    m_vdpauConfig.maxReferences = 2;

  vdp_st = m_vdpauConfig.context->GetProcs().vdp_decoder_create(m_vdpauConfig.context->GetDevice(),
                              vdp_decoder_profile,
                              m_vdpauConfig.surfaceWidth,
                              m_vdpauConfig.surfaceHeight,
                              m_vdpauConfig.maxReferences,
                              &m_vdpauConfig.vdpDecoder);
  if (CheckStatus(vdp_st, __LINE__))
    return false;

  // initialize output
  CSingleLock lock(g_graphicsContext);
  m_vdpauConfig.stats = &m_bufferStats;
  m_vdpauConfig.vdpau = this;
  m_bufferStats.Reset();
  m_vdpauOutput.Start();
  Message *reply;
  if (m_vdpauOutput.m_controlPort.SendOutMessageSync(COutputControlProtocol::INIT,
                                                 &reply,
                                                 2000,
                                                 &m_vdpauConfig,
                                                 sizeof(m_vdpauConfig)))
  {
    bool success = reply->signal == COutputControlProtocol::ACC ? true : false;
    reply->Release();
    if (!success)
    {
      CLog::Log(LOGERROR, "VDPAU::%s - vdpau output returned error", __FUNCTION__);
      m_vdpauOutput.Dispose();
      return false;
    }
  }
  else
  {
    CLog::Log(LOGERROR, "VDPAU::%s - failed to init output", __FUNCTION__);
    m_vdpauOutput.Dispose();
    return false;
  }

  m_inMsgEvent.Reset();
  m_vdpauConfigured = true;
  m_ErrorCount = 0;
  m_vdpauConfig.resetCounter++;
  return true;
}

int CDecoder::FFGetBuffer(AVCodecContext *avctx, AVFrame *pic, int flags)
{
  ICallbackHWAccel* cb = static_cast<ICallbackHWAccel*>(avctx->opaque);
  CDecoder* vdp = static_cast<CDecoder*>(cb->GetHWAccel());

  // while we are waiting to recover we can't do anything
  CSingleLock lock(vdp->m_DecoderSection);

  if(vdp->m_DisplayState != VDPAU_OPEN)
  {
    CLog::Log(LOGWARNING, "CVDPAU::FFGetBuffer - returning due to awaiting recovery");
    return -1;
  }

  VdpVideoSurface surf = (VdpVideoSurface)(uintptr_t)pic->data[3];
  surf = vdp->m_videoSurfaces.GetFree(surf != 0 ? surf : VDP_INVALID_HANDLE);

  VdpStatus vdp_st = VDP_STATUS_ERROR;
  if (surf == VDP_INVALID_HANDLE)
  {
    // create a new surface
    VdpDecoderProfile profile;
    ReadFormatOf(avctx->codec_id, profile, vdp->m_vdpauConfig.vdpChromaType);

    vdp_st = vdp->m_vdpauConfig.context->GetProcs().vdp_video_surface_create(vdp->m_vdpauConfig.context->GetDevice(),
                                         vdp->m_vdpauConfig.vdpChromaType,
                                         avctx->coded_width,
                                         avctx->coded_height,
                                         &surf);
    vdp->CheckStatus(vdp_st, __LINE__);
    if (vdp_st != VDP_STATUS_OK)
    {
      CLog::Log(LOGERROR, "CVDPAU::FFGetBuffer - No Video surface available could be created");
      return -1;
    }
    vdp->m_videoSurfaces.AddSurface(surf);
  }

  pic->data[1] = pic->data[2] = NULL;
  pic->data[0] = (uint8_t*)(uintptr_t)surf;
  pic->data[3] = (uint8_t*)(uintptr_t)surf;
  pic->linesize[0] = pic->linesize[1] =  pic->linesize[2] = 0;
  AVBufferRef *buffer = av_buffer_create(pic->data[3], 0, FFReleaseBuffer, vdp, 0);
  if (!buffer)
  {
    CLog::Log(LOGERROR, "CVDPAU::%s - error creating buffer", __FUNCTION__);
    return -1;
  }
  pic->buf[0] = buffer;

  pic->reordered_opaque= avctx->reordered_opaque;
  return 0;
}

void CDecoder::FFReleaseBuffer(void *opaque, uint8_t *data)
{
  CDecoder *vdp = static_cast<CDecoder*>(opaque);

  VdpVideoSurface surf;

  CSingleLock lock(vdp->m_DecoderSection);

  surf = (VdpVideoSurface)(uintptr_t)data;

  vdp->m_videoSurfaces.ClearReference(surf);
}

int CDecoder::Render(struct AVCodecContext *s, struct AVFrame *src,
                     const VdpPictureInfo *info, uint32_t buffers_used,
                     const VdpBitstreamBuffer *buffers)
{
  ICallbackHWAccel* ctx = static_cast<ICallbackHWAccel*>(s->opaque);
  CDecoder* vdp = static_cast<CDecoder*>(ctx->GetHWAccel());

  // while we are waiting to recover we can't do anything
  CSingleLock lock(vdp->m_DecoderSection);

  if(vdp->m_DisplayState != VDPAU_OPEN)
    return -1;

  if(src->linesize[0] || src->linesize[1] || src->linesize[2])
  {
    CLog::Log(LOGERROR, "CVDPAU::FFDrawSlice - invalid linesizes or offsets provided");
    return -1;
  }

  VdpStatus vdp_st;
  VdpVideoSurface surf = (VdpVideoSurface)(uintptr_t)src->data[3];

  // ffmpeg vc-1 decoder does not flush, make sure the data buffer is still valid
  if (!vdp->m_videoSurfaces.IsValid(surf))
  {
    CLog::Log(LOGWARNING, "CVDPAU::FFDrawSlice - ignoring invalid buffer");
    return -1;
  }

  uint32_t max_refs = 0;
  if(s->codec_id == AV_CODEC_ID_H264)
    max_refs = s->refs;

  if(vdp->m_vdpauConfig.vdpDecoder == VDP_INVALID_HANDLE
  || vdp->m_vdpauConfigured == false
  || vdp->m_vdpauConfig.maxReferences < max_refs)
  {
    if(!vdp->ConfigVDPAU(s, max_refs))
      return -1;
  }

  uint64_t startTime = CurrentHostCounter();
  uint16_t decoded, processed, rend;
  vdp->m_bufferStats.Get(decoded, processed, rend);
  vdp_st = vdp->m_vdpauConfig.context->GetProcs().vdp_decoder_render(vdp->m_vdpauConfig.vdpDecoder,
                                                                     surf, info, buffers_used, buffers);
  if (vdp->CheckStatus(vdp_st, __LINE__))
    return -1;

  uint64_t diff = CurrentHostCounter() - startTime;
  if (diff*1000/CurrentHostFrequency() > 30)
    CLog::Log(LOGDEBUG, LOGVIDEO, "CVDPAU::DrawSlice - VdpDecoderRender long decoding: %d ms, dec: %d, proc: %d, rend: %d", (int)((diff*1000)/CurrentHostFrequency()), decoded, processed, rend);

  return 0;
}

void CDecoder::SetCodecControl(int flags)
{
  m_codecControl = flags & (DVD_CODEC_CTRL_DRAIN | DVD_CODEC_CTRL_HURRY);
  if (m_codecControl & DVD_CODEC_CTRL_DRAIN)
    m_bufferStats.SetDraining(true);
  else
    m_bufferStats.SetDraining(false);
}

CDVDVideoCodec::VCReturn CDecoder::Decode(AVCodecContext *avctx, AVFrame *pFrame)
{
  CDVDVideoCodec::VCReturn result = Check(avctx);
  if (result != CDVDVideoCodec::VC_NONE)
    return result;

  CSingleLock lock(m_DecoderSection);

  if (!m_vdpauConfigured)
    return CDVDVideoCodec::VC_ERROR;

  if(pFrame)
  { // we have a new frame from decoder

    VdpVideoSurface surf = (VdpVideoSurface)(uintptr_t)pFrame->data[3];
    // ffmpeg vc-1 decoder does not flush, make sure the data buffer is still valid
    if (!m_videoSurfaces.IsValid(surf))
    {
      CLog::Log(LOGWARNING, "CVDPAU::Decode - ignoring invalid buffer");
      return CDVDVideoCodec::VC_BUFFER;
    }
    m_videoSurfaces.MarkRender(surf);

    // send frame to output for processing
    CVdpauDecodedPicture pic;
    static_cast<ICallbackHWAccel*>(avctx->opaque)->GetPictureCommon(&pic.DVDPic);
    pic.videoSurface = surf;
    pic.DVDPic.color_space = avctx->colorspace;
    m_bufferStats.IncDecoded();
    m_vdpauOutput.m_dataPort.SendOutMessage(COutputDataProtocol::NEWFRAME, &pic, sizeof(pic));

    m_codecControl = pic.DVDPic.iFlags & (DVD_CODEC_CTRL_DRAIN | DVD_CODEC_CTRL_NO_POSTPROC);
  }

  uint16_t decoded, processed, render;
  Message *msg;
  while (m_vdpauOutput.m_controlPort.ReceiveInMessage(&msg))
  {
    if (msg->signal == COutputControlProtocol::ERROR)
    {
      m_DisplayState = VDPAU_ERROR;
      msg->Release();
      return CDVDVideoCodec::VC_BUFFER;
    }
    msg->Release();
  }

  bool drain = (m_codecControl & DVD_CODEC_CTRL_DRAIN);

  m_bufferStats.Get(decoded, processed, render);
  // if all pics are drained, break the loop by setting VC_EOF
  if (drain && decoded <= 0 && processed <= 0 && render <= 0)
    return CDVDVideoCodec::VC_EOF;

  uint64_t startTime = CurrentHostCounter();
  while (true)
  {
    // first fill the buffers to keep vdpau busy
    // mixer will run with decoded >= 2. output is limited by number of output surfaces
    // In case mixer is bypassed we limit by looking at processed
    if (!drain && decoded < 3 && processed < 3)
    {
      return CDVDVideoCodec::VC_BUFFER;
    }
    else if (m_vdpauOutput.m_dataPort.ReceiveInMessage(&msg))
    {
      if (msg->signal == COutputDataProtocol::PICTURE)
      {
        if (m_presentPicture)
        {
          m_presentPicture->Release();
          m_presentPicture = nullptr;
        }

        m_presentPicture = *(CVdpauRenderPicture**)msg->data;
        m_bufferStats.DecRender();
        msg->Release();
        uint64_t diff = CurrentHostCounter() - startTime;
        m_bufferStats.SetParams(diff, m_codecControl);
        return CDVDVideoCodec::VC_PICTURE;
      }
      msg->Release();
    }
    else if (m_vdpauOutput.m_controlPort.ReceiveInMessage(&msg))
    {
      if (msg->signal == COutputControlProtocol::STATS)
      {
        msg->Release();
        m_bufferStats.Get(decoded, processed, render);
        if (!drain && decoded < 3 && processed < 3)
        {
          return CDVDVideoCodec::VC_BUFFER;
        }
      }
      else
      {
        m_DisplayState = VDPAU_ERROR;
        msg->Release();
        return CDVDVideoCodec::VC_ERROR;
      }
    }

    if (!m_inMsgEvent.WaitMSec(2000))
      break;
  }

  CLog::Log(LOGERROR, "VDPAU::%s - timed out waiting for output message", __FUNCTION__);
  m_DisplayState = VDPAU_ERROR;

  return CDVDVideoCodec::VC_ERROR;
}

bool CDecoder::GetPicture(AVCodecContext* avctx, VideoPicture* picture)
{
  if (picture->videoBuffer)
  {
    picture->videoBuffer->Release();
    picture->videoBuffer = nullptr;
  }

  CSingleLock lock(m_DecoderSection);

  if (m_DisplayState != VDPAU_OPEN)
    return false;

  picture->SetParams(m_presentPicture->DVDPic);
  picture->videoBuffer = m_presentPicture;
  m_presentPicture = nullptr;

  return true;
}

void CDecoder::Reset()
{
  CSingleLock lock(m_DecoderSection);

  if (m_presentPicture)
  {
    m_presentPicture->Release();
    m_presentPicture = nullptr;
  }

  if (!m_vdpauConfigured)
    return;

  Message *reply;
  if (m_vdpauOutput.m_controlPort.SendOutMessageSync(COutputControlProtocol::FLUSH,
                                                 &reply,
                                                 2000))
  {
    bool success = reply->signal == COutputControlProtocol::ACC ? true : false;
    reply->Release();
    if (!success)
    {
      CLog::Log(LOGERROR, "VDPAU::%s - flush returned error", __FUNCTION__);
      m_DisplayState = VDPAU_ERROR;
    }
    else
      m_bufferStats.Reset();
  }
  else
  {
    CLog::Log(LOGERROR, "VDPAU::%s - flush timed out", __FUNCTION__);
    m_DisplayState = VDPAU_ERROR;
  }
}

bool CDecoder::CanSkipDeint()
{
  return m_bufferStats.CanSkipDeint();
}

void CDecoder::ReturnRenderPicture(CVdpauRenderPicture *renderPic)
{
  m_vdpauOutput.m_dataPort.SendOutMessage(COutputDataProtocol::RETURNPIC, &renderPic, sizeof(renderPic));
}

bool CDecoder::CheckStatus(VdpStatus vdp_st, int line)
{
  if (vdp_st != VDP_STATUS_OK)
  {
    CLog::Log(LOGERROR, " (VDPAU) Error: %s(%d) at %s:%d\n", m_vdpauConfig.context->GetProcs().vdp_get_error_string(vdp_st), vdp_st, __FILE__, line);

    m_ErrorCount++;

    if(m_DisplayState == VDPAU_OPEN)
    {
      if (vdp_st == VDP_STATUS_DISPLAY_PREEMPTED)
      {
        m_DisplayEvent.Reset();
        m_DisplayState = VDPAU_LOST;
      }
      else if (m_ErrorCount > 2)
        m_DisplayState = VDPAU_ERROR;
    }

    return true;
  }
  m_ErrorCount = 0;
  return false;
}

IHardwareDecoder* CDecoder::Create(CDVDStreamInfo &hint, CProcessInfo &processInfo, AVPixelFormat fmt)
 {
   if (CDecoder::IsVDPAUFormat(fmt) && CServiceBroker::GetSettings().GetBool(CSettings::SETTING_VIDEOPLAYER_USEVDPAU))
     return new VDPAU::CDecoder(processInfo);

   return nullptr;
 }

void CDecoder::Register()
{
  CVDPAUContext *context;
  if (!CVDPAUContext::EnsureContext(&context))
    return;

  context->Release();

  m_capGeneral = true;

  CDVDFactoryCodec::RegisterHWAccel("vdpau", CDecoder::Create);
}

//-----------------------------------------------------------------------------
// BufferPool
//-----------------------------------------------------------------------------

class VDPAU::CVdpauBufferPool : public IVideoBufferPool
{
public:
  explicit CVdpauBufferPool(CDecoder &decoder);
  ~CVdpauBufferPool() override;
  CVideoBuffer* Get() override;
  void Return(int id) override;
  CVdpauRenderPicture* GetVdpau();
  bool HasFree();
  void QueueReturnPicture(CVdpauRenderPicture *pic);
  CVdpauRenderPicture* ProcessSyncPicture();
  void InvalidateUsed();

  unsigned short numOutputSurfaces;
  std::vector<VdpOutputSurface> outputSurfaces;
  std::queue<CVdpauProcessedPicture> processedPics;
  std::deque<CVdpauProcessedPicture> processedPicsAway;

  int procPicId = 0;

protected:
  std::vector<CVdpauRenderPicture*> allRenderPics;
  std::deque<int> usedRenderPics;
  std::deque<int> freeRenderPics;
  std::deque<int> syncRenderPics;

  CDecoder &m_vdpau;
};

CVdpauBufferPool::CVdpauBufferPool(CDecoder &decoder)
  : m_vdpau(decoder)
{
  CVdpauRenderPicture *pic;
  for (unsigned int i = 0; i < NUM_RENDER_PICS; i++)
  {
    pic = new CVdpauRenderPicture(i);
    allRenderPics.push_back(pic);
    freeRenderPics.push_back(i);
  }
}

CVdpauBufferPool::~CVdpauBufferPool()
{
  CVdpauRenderPicture *pic;
  for (unsigned int i = 0; i < NUM_RENDER_PICS; i++)
  {
    pic = allRenderPics[i];
    delete pic;
  }
  allRenderPics.clear();
}

CVideoBuffer* CVdpauBufferPool::Get()
{
  if (freeRenderPics.empty())
    return nullptr;

  int idx = freeRenderPics.front();
  freeRenderPics.pop_front();
  usedRenderPics.push_back(idx);

  CVideoBuffer *retPic = allRenderPics[idx];
  retPic->Acquire(GetPtr());

  m_vdpau.Acquire();

  return retPic;
}

void CVdpauBufferPool::Return(int id)
{
  CVdpauRenderPicture *pic = allRenderPics[id];

  m_vdpau.ReturnRenderPicture(pic);
  m_vdpau.ReleasePicReference();
}

CVdpauRenderPicture* CVdpauBufferPool::GetVdpau()
{
  return dynamic_cast<CVdpauRenderPicture*>(Get());
}

bool CVdpauBufferPool::HasFree()
{
  return !freeRenderPics.empty();
}

void CVdpauBufferPool::QueueReturnPicture(CVdpauRenderPicture *pic)
{
  std::deque<int>::iterator it;
  for (it = usedRenderPics.begin(); it != usedRenderPics.end(); ++it)
  {
    if (allRenderPics[*it] == pic)
    {
      break;
    }
  }

  if (it == usedRenderPics.end())
  {
    CLog::Log(LOGWARNING, "COutput::QueueReturnPicture - pic not found");
    return;
  }

  // check if already queued
  std::deque<int>::iterator it2 = find(syncRenderPics.begin(),
                                       syncRenderPics.end(),
                                       *it);
  if (it2 == syncRenderPics.end())
  {
    syncRenderPics.push_back(*it);
  }
}

CVdpauRenderPicture* CVdpauBufferPool::ProcessSyncPicture()
{
  CVdpauRenderPicture *retPic = nullptr;

  std::deque<int>::iterator it;
  for (it = syncRenderPics.begin(); it != syncRenderPics.end(); )
  {
    retPic = allRenderPics[*it];

    freeRenderPics.push_back(*it);

    std::deque<int>::iterator it2 = find(usedRenderPics.begin(),
                                         usedRenderPics.end(),
                                         *it);
    if (it2 == usedRenderPics.end())
    {
      CLog::Log(LOGERROR, "COutput::ProcessSyncPicture - pic not found in queue");
    }
    else
    {
      usedRenderPics.erase(it2);
    }
    it = syncRenderPics.erase(it);

    break;
  }
  return retPic;
}

void CVdpauBufferPool::InvalidateUsed()
{
  std::deque<int>::iterator it;
  for (it = usedRenderPics.begin(); it != usedRenderPics.end(); ++it)
  {
    allRenderPics[*it]->procPic.outputSurface = VDP_INVALID_HANDLE;
    allRenderPics[*it]->procPic.videoSurface = VDP_INVALID_HANDLE;
  }
}

//-----------------------------------------------------------------------------
// Mixer
//-----------------------------------------------------------------------------
CMixer::CMixer(CEvent *inMsgEvent) :
  CThread("Vdpau Mixer"),
  m_controlPort("ControlPort", inMsgEvent, &m_outMsgEvent),
  m_dataPort("DataPort", inMsgEvent, &m_outMsgEvent)
{
  m_inMsgEvent = inMsgEvent;
}

CMixer::~CMixer()
{
  Dispose();
}

void CMixer::Start()
{
  Create();
}

void CMixer::Dispose()
{
  m_bStop = true;
  m_outMsgEvent.Set();
  StopThread();

  m_controlPort.Purge();
  m_dataPort.Purge();
}

bool CMixer::IsActive()
{
  return IsRunning();
}

void CMixer::OnStartup()
{
  CLog::Log(LOGNOTICE, "CMixer::OnStartup: Output Thread created");
}

void CMixer::OnExit()
{
  CLog::Log(LOGNOTICE, "CMixer::OnExit: Output Thread terminated");
}

enum MIXER_STATES
{
  M_TOP = 0,                      // 0
  M_TOP_ERROR,                    // 1
  M_TOP_UNCONFIGURED,             // 2
  M_TOP_CONFIGURED,               // 3
  M_TOP_CONFIGURED_WAIT1,         // 4
  M_TOP_CONFIGURED_STEP1,         // 5
  M_TOP_CONFIGURED_WAIT2,         // 6
  M_TOP_CONFIGURED_STEP2,         // 7
};

int MIXER_parentStates[] = {
    -1,
    0, //TOP_ERROR
    0, //TOP_UNCONFIGURED
    0, //TOP_CONFIGURED
    3, //TOP_CONFIGURED_WAIT1
    3, //TOP_CONFIGURED_STEP1
    3, //TOP_CONFIGURED_WAIT2
    3, //TOP_CONFIGURED_STEP2
};

void CMixer::StateMachine(int signal, Protocol *port, Message *msg)
{
  for (int state = m_state; ; state = MIXER_parentStates[state])
  {
    switch (state)
    {
    case M_TOP: // TOP
      if (port == &m_controlPort)
      {
        switch (signal)
        {
        case CMixerControlProtocol::FLUSH:
          Flush();
          msg->Reply(CMixerControlProtocol::ACC);
          return;
        default:
          break;
        }
      }
      {
        std::string portName = port == NULL ? "timer" : port->portName;
        CLog::Log(LOGWARNING, "CMixer::%s - signal: %d form port: %s not handled for state: %d", __FUNCTION__, signal, portName.c_str(), m_state);
      }
      return;

    case M_TOP_ERROR: // TOP
      break;

    case M_TOP_UNCONFIGURED:
      if (port == &m_controlPort)
      {
        switch (signal)
        {
        case CMixerControlProtocol::INIT:
          CVdpauConfig *data;
          data = (CVdpauConfig*)msg->data;
          if (data)
          {
            m_config = *data;
          }
          Init();
          if (!m_vdpError)
          {
            m_state = M_TOP_CONFIGURED_WAIT1;
            msg->Reply(CMixerControlProtocol::ACC);
          }
          else
          {
            msg->Reply(CMixerControlProtocol::ERROR);
          }
          return;
        default:
          break;
        }
      }
      break;

    case M_TOP_CONFIGURED:
      if (port == &m_controlPort)
      {
        switch (signal)
        {
        case CMixerControlProtocol::FLUSH:
          Flush();
          msg->Reply(CMixerControlProtocol::ACC);
          m_state = M_TOP_CONFIGURED_WAIT1;
          return;
        default:
          break;
        }
      }
      else if (port == &m_dataPort)
      {
        switch (signal)
        {
        case CMixerDataProtocol::FRAME:
          CVdpauDecodedPicture *frame;
          frame = (CVdpauDecodedPicture*)msg->data;
          if (frame)
          {
            m_decodedPics.push(*frame);
          }
          m_extTimeout = 0;
          return;
        case CMixerDataProtocol::BUFFER:
          VdpOutputSurface *surf;
          surf = (VdpOutputSurface*)msg->data;
          if (surf)
          {
            m_outputSurfaces.push(*surf);
          }
          m_extTimeout = 0;
          return;
        default:
          break;
        }
      }
      break;

    case M_TOP_CONFIGURED_WAIT1:
      if (port == NULL) // timeout
      {
        switch (signal)
        {
        case CMixerControlProtocol::TIMEOUT:
          if (!m_decodedPics.empty() && !m_outputSurfaces.empty())
          {
            m_state = M_TOP_CONFIGURED_STEP1;
            m_bStateMachineSelfTrigger = true;
          }
          else if (!m_outputSurfaces.empty() &&
                   m_config.stats->IsDraining() &&
                   m_mixerInput.size() >= 1)
          {
            CVdpauDecodedPicture pic;
            pic.DVDPic.SetParams(m_mixerInput[0].DVDPic);
            pic.videoSurface = VDP_INVALID_HANDLE;
            m_decodedPics.push(pic);
            m_state = M_TOP_CONFIGURED_STEP1;
            m_bStateMachineSelfTrigger = true;
          }
          else
          {
            m_extTimeout = 100;
          }
          return;
        default:
          break;
        }
      }
      break;

    case M_TOP_CONFIGURED_STEP1:
      if (port == NULL) // timeout
      {
        switch (signal)
        {
        case CMixerControlProtocol::TIMEOUT:
          m_mixerInput.push_front(m_decodedPics.front());
          m_decodedPics.pop();
          if (m_mixerInput.size() < 2)
          {
            m_state = M_TOP_CONFIGURED_WAIT1;
            m_extTimeout = 0;
            return;
          }
          InitCycle();
          ProcessPicture();
          if (m_vdpError)
          {
            m_state = M_TOP_CONFIGURED_WAIT1;
            m_extTimeout = 1000;
            return;
          }
          if (!m_processPicture.isYuv)
            m_outputSurfaces.pop();
          m_config.stats->IncProcessed();
          m_config.stats->DecDecoded();
          m_dataPort.SendInMessage(CMixerDataProtocol::PICTURE,&m_processPicture,sizeof(m_processPicture));
          if (m_mixersteps > 1)
          {
            m_state = M_TOP_CONFIGURED_WAIT2;
            m_extTimeout = 0;
          }
          else
          {
            FiniCycle();
            m_state = M_TOP_CONFIGURED_WAIT1;
            m_extTimeout = 0;
          }
          return;
        default:
          break;
        }
      }
      break;

    case M_TOP_CONFIGURED_WAIT2:
      if (port == NULL) // timeout
      {
        switch (signal)
        {
        case CMixerControlProtocol::TIMEOUT:
          if (!m_outputSurfaces.empty())
          {
            m_state = M_TOP_CONFIGURED_STEP2;
            m_bStateMachineSelfTrigger = true;
          }
          else
          {
            m_extTimeout = 100;
          }
          return;
        default:
          break;
        }
      }
      break;

    case M_TOP_CONFIGURED_STEP2:
       if (port == NULL) // timeout
       {
         switch (signal)
         {
         case CMixerControlProtocol::TIMEOUT:
           m_processPicture.outputSurface = m_outputSurfaces.front();
           m_mixerstep = 1;
           ProcessPicture();
           if (m_vdpError)
           {
             m_state = M_TOP_CONFIGURED_WAIT1;
             m_extTimeout = 1000;
             return;
           }
           if (!m_processPicture.isYuv)
             m_outputSurfaces.pop();
           m_config.stats->IncProcessed();
           m_dataPort.SendInMessage(CMixerDataProtocol::PICTURE,&m_processPicture,sizeof(m_processPicture));
           FiniCycle();
           m_state = M_TOP_CONFIGURED_WAIT1;
           m_extTimeout = 0;
           return;
         default:
           break;
         }
       }
       break;

    default: // we are in no state, should not happen
      CLog::Log(LOGERROR, "CMixer::%s - no valid state: %d", __FUNCTION__, m_state);
      return;
    }
  } // for
}

void CMixer::Process()
{
  Message *msg = NULL;
  Protocol *port = NULL;
  bool gotMsg;

  m_state = M_TOP_UNCONFIGURED;
  m_extTimeout = 1000;
  m_bStateMachineSelfTrigger = false;

  while (!m_bStop)
  {
    gotMsg = false;

    if (m_bStateMachineSelfTrigger)
    {
      m_bStateMachineSelfTrigger = false;
      // self trigger state machine
      StateMachine(msg->signal, port, msg);
      if (!m_bStateMachineSelfTrigger)
      {
        msg->Release();
        msg = NULL;
      }
      continue;
    }
    // check control port
    else if (m_controlPort.ReceiveOutMessage(&msg))
    {
      gotMsg = true;
      port = &m_controlPort;
    }
    // check data port
    else if (m_dataPort.ReceiveOutMessage(&msg))
    {
      gotMsg = true;
      port = &m_dataPort;
    }

    if (gotMsg)
    {
      StateMachine(msg->signal, port, msg);
      if (!m_bStateMachineSelfTrigger)
      {
        msg->Release();
        msg = NULL;
      }
      continue;
    }

    // wait for message
    else if (m_outMsgEvent.WaitMSec(m_extTimeout))
    {
      continue;
    }
    // time out
    else
    {
      msg = m_controlPort.GetMessage();
      msg->signal = CMixerControlProtocol::TIMEOUT;
      port = 0;
      // signal timeout to state machine
      StateMachine(msg->signal, port, msg);
      if (!m_bStateMachineSelfTrigger)
      {
        msg->Release();
        msg = NULL;
      }
    }
  }
  Uninit();
}

void CMixer::CreateVdpauMixer()
{
  CLog::Log(LOGNOTICE, " (VDPAU) Creating the video mixer");

  InitCSCMatrix(m_config.vidWidth);

  VdpVideoMixerParameter parameters[] = {
    VDP_VIDEO_MIXER_PARAMETER_VIDEO_SURFACE_WIDTH,
    VDP_VIDEO_MIXER_PARAMETER_VIDEO_SURFACE_HEIGHT,
    VDP_VIDEO_MIXER_PARAMETER_CHROMA_TYPE};

  void const * parameter_values[] = {
    &m_config.surfaceWidth,
    &m_config.surfaceHeight,
    &m_config.vdpChromaType};

  VdpStatus vdp_st = m_config.context->GetProcs().vdp_video_mixer_create(m_config.context->GetDevice(),
                                m_config.context->GetFeatureCount(),
                                m_config.context->GetFeatures(),
                                ARSIZE(parameters),
                                parameters,
                                parameter_values,
                                &m_videoMixer);
  CheckStatus(vdp_st, __LINE__);

}

void CMixer::InitCSCMatrix(int Width)
{
  m_Procamp.struct_version = VDP_PROCAMP_VERSION;
  m_Procamp.brightness     = 0.0;
  m_Procamp.contrast       = 1.0;
  m_Procamp.saturation     = 1.0;
  m_Procamp.hue            = 0;
}

void CMixer::CheckFeatures()
{
  if (m_Upscale != m_config.upscale)
  {
    SetHWUpscaling();
    m_Upscale = m_config.upscale;
  }
  if (m_Brightness != m_config.processInfo->GetVideoSettings().m_Brightness ||
      m_Contrast   != m_config.processInfo->GetVideoSettings().m_Contrast ||
      m_ColorMatrix != m_mixerInput[1].DVDPic.color_space)
  {
    SetColor();
    m_Brightness = m_config.processInfo->GetVideoSettings().m_Brightness;
    m_Contrast = m_config.processInfo->GetVideoSettings().m_Contrast;
    m_ColorMatrix = m_mixerInput[1].DVDPic.color_space;
  }
  if (m_NoiseReduction != m_config.processInfo->GetVideoSettings().m_NoiseReduction)
  {
    m_NoiseReduction = m_config.processInfo->GetVideoSettings().m_NoiseReduction;
    SetNoiseReduction();
  }
  if (m_Sharpness != m_config.processInfo->GetVideoSettings().m_Sharpness)
  {
    m_Sharpness = m_config.processInfo->GetVideoSettings().m_Sharpness;
    SetSharpness();
  }
  if (m_Deint != m_config.processInfo->GetVideoSettings().m_InterlaceMethod)
  {
    m_Deint = m_config.processInfo->GetVideoSettings().m_InterlaceMethod;
    SetDeinterlacing();
  }
}

void CMixer::SetPostProcFeatures(bool postProcEnabled)
{
  if (m_PostProc != postProcEnabled)
  {
    if (postProcEnabled)
    {
      SetNoiseReduction();
      SetSharpness();
      SetDeinterlacing();
      SetHWUpscaling();
    }
    else
      PostProcOff();
    m_PostProc = postProcEnabled;
  }
}

void CMixer::PostProcOff()
{
  VdpStatus vdp_st;

  if (m_videoMixer == VDP_INVALID_HANDLE)
    return;

  VdpVideoMixerFeature feature[] = { VDP_VIDEO_MIXER_FEATURE_DEINTERLACE_TEMPORAL,
                                     VDP_VIDEO_MIXER_FEATURE_DEINTERLACE_TEMPORAL_SPATIAL,
                                     VDP_VIDEO_MIXER_FEATURE_INVERSE_TELECINE};

  VdpBool enabled[]={0,0,0};
  vdp_st = m_config.context->GetProcs().vdp_video_mixer_set_feature_enables(m_videoMixer, ARSIZE(feature), feature, enabled);
  CheckStatus(vdp_st, __LINE__);

  if(m_config.vdpau->Supports(VDP_VIDEO_MIXER_FEATURE_NOISE_REDUCTION))
  {
    VdpVideoMixerFeature feature[] = { VDP_VIDEO_MIXER_FEATURE_NOISE_REDUCTION};

    VdpBool enabled[]={0};
    vdp_st = m_config.context->GetProcs().vdp_video_mixer_set_feature_enables(m_videoMixer, ARSIZE(feature), feature, enabled);
    CheckStatus(vdp_st, __LINE__);
  }

  if(m_config.vdpau->Supports(VDP_VIDEO_MIXER_FEATURE_SHARPNESS))
  {
    VdpVideoMixerFeature feature[] = { VDP_VIDEO_MIXER_FEATURE_SHARPNESS};

    VdpBool enabled[]={0};
    vdp_st = m_config.context->GetProcs().vdp_video_mixer_set_feature_enables(m_videoMixer, ARSIZE(feature), feature, enabled);
    CheckStatus(vdp_st, __LINE__);
  }

  DisableHQScaling();
}

bool CMixer::GenerateStudioCSCMatrix(VdpColorStandard colorStandard, VdpCSCMatrix &studioCSCMatrix)
{
   // instead use studioCSCKCoeffs601[3], studioCSCKCoeffs709[3] to generate float[3][4] matrix (float studioCSC[3][4])
   // m00 = mRY = red: luma factor (contrast factor) (1.0)
   // m10 = mGY = green: luma factor (contrast factor) (1.0)
   // m20 = mBY = blue: luma factor (contrast factor) (1.0)
   //
   // m01 = mRB = red: blue color diff coeff (0.0)
   // m11 = mGB = green: blue color diff coeff (-2Kb(1-Kb)/(Kg))
   // m21 = mBB = blue: blue color diff coeff ((1-Kb)/0.5)
   //
   // m02 = mRR = red: red color diff coeff ((1-Kr)/0.5)
   // m12 = mGR = green: red color diff coeff (-2Kr(1-Kr)/(Kg))
   // m22 = mBR = blue: red color diff coeff (0.0)
   //
   // m03 = mRC = red: colour zero offset (brightness factor) (-(1-Kr)/0.5 * (128/255))
   // m13 = mGC = green: colour zero offset (brightness factor) ((256/255) * (Kb(1-Kb) + Kr(1-Kr)) / Kg)
   // m23 = mBC = blue: colour zero offset (brightness factor) (-(1-Kb)/0.5 * (128/255))

   // columns
   int Y = 0;
   int Cb = 1;
   int Cr = 2;
   int C = 3;
   // rows
   int R = 0;
   int G = 1;
   int B = 2;
   // colour standard coefficients for red, geen, blue
   double Kr, Kg, Kb;
   // colour diff zero position (use standard 8-bit coding precision)
   double CDZ = 128; //256*0.5
   // range excursion (use standard 8-bit coding precision)
   double EXC = 255; //256-1

   if (colorStandard == VDP_COLOR_STANDARD_ITUR_BT_601)
   {
      Kr = studioCSCKCoeffs601[0];
      Kg = studioCSCKCoeffs601[1];
      Kb = studioCSCKCoeffs601[2];
   }
   else // assume VDP_COLOR_STANDARD_ITUR_BT_709
   {
      Kr = studioCSCKCoeffs709[0];
      Kg = studioCSCKCoeffs709[1];
      Kb = studioCSCKCoeffs709[2];
   }
   // we keep luma unscaled to retain the levels present in source so that 16-235 luma is converted to RGB 16-235
   studioCSCMatrix[R][Y] = 1.0;
   studioCSCMatrix[G][Y] = 1.0;
   studioCSCMatrix[B][Y] = 1.0;

   studioCSCMatrix[R][Cb] = 0.0;
   studioCSCMatrix[G][Cb] = (double)-2 * Kb * (1 - Kb) / Kg;
   studioCSCMatrix[B][Cb] = (double)(1 - Kb) / 0.5;

   studioCSCMatrix[R][Cr] = (double)(1 - Kr) / 0.5;
   studioCSCMatrix[G][Cr] = (double)-2 * Kr * (1 - Kr) / Kg;
   studioCSCMatrix[B][Cr] = 0.0;

   studioCSCMatrix[R][C] = (double)-1 * studioCSCMatrix[R][Cr] * CDZ/EXC;
   studioCSCMatrix[G][C] = (double)-1 * (studioCSCMatrix[G][Cb] + studioCSCMatrix[G][Cr]) * CDZ/EXC;
   studioCSCMatrix[B][C] = (double)-1 * studioCSCMatrix[B][Cb] * CDZ/EXC;

   return true;
}

void CMixer::SetColor()
{
  VdpStatus vdp_st;

  if (m_Brightness != m_config.processInfo->GetVideoSettings().m_Brightness)
    m_Procamp.brightness = (float)((m_config.processInfo->GetVideoSettings().m_Brightness)-50) / 100;
  if (m_Contrast != m_config.processInfo->GetVideoSettings().m_Contrast)
    m_Procamp.contrast = (float)((m_config.processInfo->GetVideoSettings().m_Contrast)+50) / 100;

  VdpColorStandard colorStandard;
  switch(m_mixerInput[1].DVDPic.color_space)
  {
    case AVCOL_SPC_BT709:
      colorStandard = VDP_COLOR_STANDARD_ITUR_BT_709;
      break;
    case AVCOL_SPC_BT470BG:
    case AVCOL_SPC_SMPTE170M:
      colorStandard = VDP_COLOR_STANDARD_ITUR_BT_601;
      break;
    case AVCOL_SPC_SMPTE240M:
      colorStandard = VDP_COLOR_STANDARD_SMPTE_240M;
      break;
    case AVCOL_SPC_FCC:
    case AVCOL_SPC_UNSPECIFIED:
    case AVCOL_SPC_RGB:
    default:
      if(m_config.surfaceWidth > 1000)
        colorStandard = VDP_COLOR_STANDARD_ITUR_BT_709;
      else
        colorStandard = VDP_COLOR_STANDARD_ITUR_BT_601;
  }

  VdpVideoMixerAttribute attributes[] = { VDP_VIDEO_MIXER_ATTRIBUTE_CSC_MATRIX };
  if (CServiceBroker::GetSettings().GetBool(CSettings::SETTING_VIDEOSCREEN_LIMITEDRANGE))
  {
    float studioCSC[3][4];
    GenerateStudioCSCMatrix(colorStandard, studioCSC);
    void const * pm_CSCMatrix[] = { &studioCSC };
    vdp_st = m_config.context->GetProcs().vdp_video_mixer_set_attribute_values(m_videoMixer, ARSIZE(attributes), attributes, pm_CSCMatrix);
  }
  else
  {
    vdp_st = m_config.context->GetProcs().vdp_generate_csc_matrix(&m_Procamp, colorStandard, &m_CSCMatrix);
    if(vdp_st != VDP_STATUS_ERROR)
    {
      void const * pm_CSCMatrix[] = { &m_CSCMatrix };
      vdp_st = m_config.context->GetProcs().vdp_video_mixer_set_attribute_values(m_videoMixer, ARSIZE(attributes), attributes, pm_CSCMatrix);
    }
  }

  CheckStatus(vdp_st, __LINE__);
}

void CMixer::SetNoiseReduction()
{
  if(!m_config.vdpau->Supports(VDP_VIDEO_MIXER_FEATURE_NOISE_REDUCTION))
    return;

  VdpVideoMixerFeature feature[] = { VDP_VIDEO_MIXER_FEATURE_NOISE_REDUCTION };
  VdpVideoMixerAttribute attributes[] = { VDP_VIDEO_MIXER_ATTRIBUTE_NOISE_REDUCTION_LEVEL };
  VdpStatus vdp_st;

  if (!m_config.processInfo->GetVideoSettings().m_NoiseReduction)
  {
    VdpBool enabled[]= {0};
    vdp_st = m_config.context->GetProcs().vdp_video_mixer_set_feature_enables(m_videoMixer, ARSIZE(feature), feature, enabled);
    CheckStatus(vdp_st, __LINE__);
    return;
  }
  VdpBool enabled[]={1};
  vdp_st = m_config.context->GetProcs().vdp_video_mixer_set_feature_enables(m_videoMixer, ARSIZE(feature), feature, enabled);
  CheckStatus(vdp_st, __LINE__);
  float noiseReduction = m_config.processInfo->GetVideoSettings().m_NoiseReduction;
  void* nr[] = { &noiseReduction };
  CLog::Log(LOGNOTICE,"Setting Noise Reduction to %f", m_config.processInfo->GetVideoSettings().m_NoiseReduction);
  vdp_st = m_config.context->GetProcs().vdp_video_mixer_set_attribute_values(m_videoMixer, ARSIZE(attributes), attributes, nr);
  CheckStatus(vdp_st, __LINE__);
}

void CMixer::SetSharpness()
{
  if(!m_config.vdpau->Supports(VDP_VIDEO_MIXER_FEATURE_SHARPNESS))
    return;

  VdpVideoMixerFeature feature[] = { VDP_VIDEO_MIXER_FEATURE_SHARPNESS };
  VdpVideoMixerAttribute attributes[] = { VDP_VIDEO_MIXER_ATTRIBUTE_SHARPNESS_LEVEL };
  VdpStatus vdp_st;

  if (!m_config.processInfo->GetVideoSettings().m_Sharpness)
  {
    VdpBool enabled[]={0};
    vdp_st = m_config.context->GetProcs().vdp_video_mixer_set_feature_enables(m_videoMixer, ARSIZE(feature), feature, enabled);
    CheckStatus(vdp_st, __LINE__);
    return;
  }
  VdpBool enabled[]={1};
  vdp_st = m_config.context->GetProcs().vdp_video_mixer_set_feature_enables(m_videoMixer, ARSIZE(feature), feature, enabled);
  CheckStatus(vdp_st, __LINE__);
  float sharpness = m_config.processInfo->GetVideoSettings().m_Sharpness;
  void* sh[] = { &sharpness };
  CLog::Log(LOGNOTICE,"Setting Sharpness to %f", m_config.processInfo->GetVideoSettings().m_Sharpness);
  vdp_st = m_config.context->GetProcs().vdp_video_mixer_set_attribute_values(m_videoMixer, ARSIZE(attributes), attributes, sh);
  CheckStatus(vdp_st, __LINE__);
}

void CMixer::SetDeinterlacing()
{
  VdpStatus vdp_st;

  if (m_videoMixer == VDP_INVALID_HANDLE)
    return;

  EINTERLACEMETHOD method = m_config.processInfo->GetVideoSettings().m_InterlaceMethod;

  VdpVideoMixerFeature feature[] = { VDP_VIDEO_MIXER_FEATURE_DEINTERLACE_TEMPORAL,
                                     VDP_VIDEO_MIXER_FEATURE_DEINTERLACE_TEMPORAL_SPATIAL,
                                     VDP_VIDEO_MIXER_FEATURE_INVERSE_TELECINE };

  if (method == VS_INTERLACEMETHOD_NONE)
  {
    VdpBool enabled[] = {0,0,0};
    vdp_st = m_config.context->GetProcs().vdp_video_mixer_set_feature_enables(m_videoMixer, ARSIZE(feature), feature, enabled);
  }
  else
  {
    // fall back path if called with non supported method
    if (!m_config.processInfo->Supports(method))
    {
      method = VS_INTERLACEMETHOD_VDPAU_TEMPORAL;
      m_Deint = VS_INTERLACEMETHOD_VDPAU_TEMPORAL;
    }

    if (method == VS_INTERLACEMETHOD_VDPAU_TEMPORAL
    ||  method == VS_INTERLACEMETHOD_VDPAU_TEMPORAL_HALF)
    {
      VdpBool enabled[] = {1,0,0};
      if (g_advancedSettings.m_videoVDPAUtelecine)
        enabled[2] = 1;
      vdp_st = m_config.context->GetProcs().vdp_video_mixer_set_feature_enables(m_videoMixer, ARSIZE(feature), feature, enabled);
    }
    else if (method == VS_INTERLACEMETHOD_VDPAU_TEMPORAL_SPATIAL
         ||  method == VS_INTERLACEMETHOD_VDPAU_TEMPORAL_SPATIAL_HALF)
    {
      VdpBool enabled[] = {1,1,0};
      if (g_advancedSettings.m_videoVDPAUtelecine)
        enabled[2] = 1;
      vdp_st = m_config.context->GetProcs().vdp_video_mixer_set_feature_enables(m_videoMixer, ARSIZE(feature), feature, enabled);
    }
    else
    {
      VdpBool enabled[]={0,0,0};
      vdp_st = m_config.context->GetProcs().vdp_video_mixer_set_feature_enables(m_videoMixer, ARSIZE(feature), feature, enabled);
    }
  }
  CheckStatus(vdp_st, __LINE__);

  SetDeintSkipChroma();

  m_config.useInteropYuv = !CServiceBroker::GetSettings().GetBool(CSettings::SETTING_VIDEOPLAYER_USEVDPAUMIXER);

  std::string deintStr = GetDeintStrFromInterlaceMethod(method);
  // update deinterlacing method used in processInfo (none if progressive)
  m_config.processInfo->SetVideoDeintMethod(deintStr);
}

void CMixer::SetDeintSkipChroma()
{
  VdpVideoMixerAttribute attribute[] = { VDP_VIDEO_MIXER_ATTRIBUTE_SKIP_CHROMA_DEINTERLACE};
  VdpStatus vdp_st;

  uint8_t val;
  if (g_advancedSettings.m_videoVDPAUdeintSkipChromaHD && m_config.outHeight >= 720)
    val = 1;
  else
    val = 0;

  void const *values[]={&val};
  vdp_st = m_config.context->GetProcs().vdp_video_mixer_set_attribute_values(m_videoMixer, ARSIZE(attribute), attribute, values);

  CheckStatus(vdp_st, __LINE__);
}

void CMixer::SetHWUpscaling()
{
#ifdef VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L1

  VdpStatus vdp_st;
  VdpBool enabled[]={1};
  switch (m_config.upscale)
  {
    case 9:
       if (m_config.vdpau->Supports(VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L9))
       {
          VdpVideoMixerFeature feature[] = { VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L9 };
          vdp_st = m_config.context->GetProcs().vdp_video_mixer_set_feature_enables(m_videoMixer, ARSIZE(feature), feature, enabled);
          break;
       }
    case 8:
       if (m_config.vdpau->Supports(VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L8))
       {
          VdpVideoMixerFeature feature[] = { VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L8 };
          vdp_st = m_config.context->GetProcs().vdp_video_mixer_set_feature_enables(m_videoMixer, ARSIZE(feature), feature, enabled);
          break;
       }
    case 7:
       if (m_config.vdpau->Supports(VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L7))
       {
          VdpVideoMixerFeature feature[] = { VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L7 };
          vdp_st = m_config.context->GetProcs().vdp_video_mixer_set_feature_enables(m_videoMixer, ARSIZE(feature), feature, enabled);
          break;
       }
    case 6:
       if (m_config.vdpau->Supports(VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L6))
       {
          VdpVideoMixerFeature feature[] = { VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L6 };
          vdp_st = m_config.context->GetProcs().vdp_video_mixer_set_feature_enables(m_videoMixer, ARSIZE(feature), feature, enabled);
          break;
       }
    case 5:
       if (m_config.vdpau->Supports(VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L5))
       {
          VdpVideoMixerFeature feature[] = { VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L5 };
          vdp_st = m_config.context->GetProcs().vdp_video_mixer_set_feature_enables(m_videoMixer, ARSIZE(feature), feature, enabled);
          break;
       }
    case 4:
       if (m_config.vdpau->Supports(VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L4))
       {
          VdpVideoMixerFeature feature[] = { VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L4 };
          vdp_st = m_config.context->GetProcs().vdp_video_mixer_set_feature_enables(m_videoMixer, ARSIZE(feature), feature, enabled);
          break;
       }
    case 3:
       if (m_config.vdpau->Supports(VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L3))
       {
          VdpVideoMixerFeature feature[] = { VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L3 };
          vdp_st = m_config.context->GetProcs().vdp_video_mixer_set_feature_enables(m_videoMixer, ARSIZE(feature), feature, enabled);
          break;
       }
    case 2:
       if (m_config.vdpau->Supports(VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L2))
       {
          VdpVideoMixerFeature feature[] = { VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L2 };
          vdp_st = m_config.context->GetProcs().vdp_video_mixer_set_feature_enables(m_videoMixer, ARSIZE(feature), feature, enabled);
          break;
       }
    case 1:
       if (m_config.vdpau->Supports(VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L1))
       {
          VdpVideoMixerFeature feature[] = { VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L1 };
          vdp_st = m_config.context->GetProcs().vdp_video_mixer_set_feature_enables(m_videoMixer, ARSIZE(feature), feature, enabled);
          break;
       }
    default:
       DisableHQScaling();
       return;
  }
  CheckStatus(vdp_st, __LINE__);
#endif
}

void CMixer::DisableHQScaling()
{
  VdpStatus vdp_st;

  if (m_videoMixer == VDP_INVALID_HANDLE)
    return;

  if(m_config.vdpau->Supports(VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L1))
  {
    VdpVideoMixerFeature feature[] = { VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L1 };
    VdpBool enabled[]={0};
    vdp_st = m_config.context->GetProcs().vdp_video_mixer_set_feature_enables(m_videoMixer, ARSIZE(feature), feature, enabled);
    CheckStatus(vdp_st, __LINE__);
  }

  if(m_config.vdpau->Supports(VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L2))
  {
    VdpVideoMixerFeature feature[] = { VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L2 };
    VdpBool enabled[]={0};
    vdp_st = m_config.context->GetProcs().vdp_video_mixer_set_feature_enables(m_videoMixer, ARSIZE(feature), feature, enabled);
    CheckStatus(vdp_st, __LINE__);
  }

  if(m_config.vdpau->Supports(VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L3))
  {
    VdpVideoMixerFeature feature[] = { VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L3 };
    VdpBool enabled[]={0};
    vdp_st = m_config.context->GetProcs().vdp_video_mixer_set_feature_enables(m_videoMixer, ARSIZE(feature), feature, enabled);
    CheckStatus(vdp_st, __LINE__);
  }

  if(m_config.vdpau->Supports(VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L4))
  {
    VdpVideoMixerFeature feature[] = { VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L4 };
    VdpBool enabled[]={0};
    vdp_st = m_config.context->GetProcs().vdp_video_mixer_set_feature_enables(m_videoMixer, ARSIZE(feature), feature, enabled);
    CheckStatus(vdp_st, __LINE__);
  }

  if(m_config.vdpau->Supports(VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L5))
  {
    VdpVideoMixerFeature feature[] = { VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L5 };
    VdpBool enabled[]={0};
    vdp_st = m_config.context->GetProcs().vdp_video_mixer_set_feature_enables(m_videoMixer, ARSIZE(feature), feature, enabled);
    CheckStatus(vdp_st, __LINE__);
  }

  if(m_config.vdpau->Supports(VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L6))
  {
    VdpVideoMixerFeature feature[] = { VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L6 };
    VdpBool enabled[]={0};
    vdp_st = m_config.context->GetProcs().vdp_video_mixer_set_feature_enables(m_videoMixer, ARSIZE(feature), feature, enabled);
    CheckStatus(vdp_st, __LINE__);
  }

  if(m_config.vdpau->Supports(VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L7))
  {
    VdpVideoMixerFeature feature[] = { VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L7 };
    VdpBool enabled[]={0};
    vdp_st = m_config.context->GetProcs().vdp_video_mixer_set_feature_enables(m_videoMixer, ARSIZE(feature), feature, enabled);
    CheckStatus(vdp_st, __LINE__);
  }

  if(m_config.vdpau->Supports(VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L8))
  {
    VdpVideoMixerFeature feature[] = { VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L8 };
    VdpBool enabled[]={0};
    vdp_st = m_config.context->GetProcs().vdp_video_mixer_set_feature_enables(m_videoMixer, ARSIZE(feature), feature, enabled);
    CheckStatus(vdp_st, __LINE__);
  }

  if(m_config.vdpau->Supports(VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L9))
  {
    VdpVideoMixerFeature feature[] = { VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L9 };
    VdpBool enabled[]={0};
    vdp_st = m_config.context->GetProcs().vdp_video_mixer_set_feature_enables(m_videoMixer, ARSIZE(feature), feature, enabled);
    CheckStatus(vdp_st, __LINE__);
  }
}

void CMixer::Init()
{
  m_Brightness = 0.0;
  m_Contrast = 0.0;
  m_NoiseReduction = 0.0;
  m_Sharpness = 0.0;
  m_Deint = 0;
  m_Upscale = 0;
  m_SeenInterlaceFlag = false;
  m_ColorMatrix = 0;
  m_PostProc = false;
  m_vdpError = false;

  m_config.upscale = g_advancedSettings.m_videoVDPAUScaling;
  m_config.useInteropYuv = !CServiceBroker::GetSettings().GetBool(CSettings::SETTING_VIDEOPLAYER_USEVDPAUMIXER);

  CreateVdpauMixer();

  // update deinterlacing methods in processInfo
  std::list<EINTERLACEMETHOD> deintMethods;
  deintMethods.push_back(VS_INTERLACEMETHOD_NONE);
  for(SInterlaceMapping* p = g_interlace_mapping; p->method != VS_INTERLACEMETHOD_NONE; p++)
  {
    if (p->method == VS_INTERLACEMETHOD_VDPAU_INVERSE_TELECINE)
      continue;

    if (m_config.vdpau->Supports(p->feature))
      deintMethods.push_back(p->method);
  }
  deintMethods.push_back(VS_INTERLACEMETHOD_VDPAU_BOB);
  deintMethods.push_back(VS_INTERLACEMETHOD_RENDER_BOB);
  m_config.processInfo->UpdateDeinterlacingMethods(deintMethods);
  m_config.processInfo->SetDeinterlacingMethodDefault(EINTERLACEMETHOD::VS_INTERLACEMETHOD_VDPAU_TEMPORAL);
}

void CMixer::Uninit()
{
  Flush();
  while (!m_outputSurfaces.empty())
  {
    m_outputSurfaces.pop();
  }
  m_config.context->GetProcs().vdp_video_mixer_destroy(m_videoMixer);
}

void CMixer::Flush()
{
  while (!m_mixerInput.empty())
  {
    CVdpauDecodedPicture pic = m_mixerInput.back();
    m_mixerInput.pop_back();
    m_config.videoSurfaces->ClearRender(pic.videoSurface);
  }
  while (!m_decodedPics.empty())
  {
    CVdpauDecodedPicture pic = m_decodedPics.front();
    m_decodedPics.pop();
    m_config.videoSurfaces->ClearRender(pic.videoSurface);
  }
  Message *msg;
  while (m_dataPort.ReceiveOutMessage(&msg))
  {
    if (msg->signal == CMixerDataProtocol::FRAME)
    {
      CVdpauDecodedPicture pic = *reinterpret_cast<CVdpauDecodedPicture*>(msg->data);
      m_config.videoSurfaces->ClearRender(pic.videoSurface);
    }
    else if (msg->signal == CMixerDataProtocol::BUFFER)
    {
      VdpOutputSurface *surf;
      surf = (VdpOutputSurface*)msg->data;
      m_outputSurfaces.push(*surf);
    }
    msg->Release();
  }
}

std::string CMixer::GetDeintStrFromInterlaceMethod(EINTERLACEMETHOD method)
{
  switch (method)
  {
    case VS_INTERLACEMETHOD_NONE:
      return "none";
    case VS_INTERLACEMETHOD_VDPAU_BOB:
      return "vdpau-bob";
    case VS_INTERLACEMETHOD_VDPAU_TEMPORAL:
      return "vdpau-temp";
    case VS_INTERLACEMETHOD_VDPAU_TEMPORAL_HALF:
      return "vdpau-temp-half";
    case VS_INTERLACEMETHOD_VDPAU_TEMPORAL_SPATIAL:
      return "vdpau-temp-spat";
    case VS_INTERLACEMETHOD_VDPAU_TEMPORAL_SPATIAL_HALF:
      return "vdpau-temp-spat-half";
    case VS_INTERLACEMETHOD_RENDER_BOB:
      return "bob";
    default:
      return "unknown";
  }
}

void CMixer::InitCycle()
{
  CheckFeatures();
  int flags;
  uint64_t latency;
  m_config.stats->GetParams(latency, flags);
  if (flags & DVD_CODEC_CTRL_NO_POSTPROC)
    SetPostProcFeatures(false);
  else
    SetPostProcFeatures(true);

  m_config.stats->SetCanSkipDeint(false);

  EINTERLACEMETHOD method = m_config.processInfo->GetVideoSettings().m_InterlaceMethod;
  bool interlaced = m_mixerInput[1].DVDPic.iFlags & DVP_FLAG_INTERLACED;
  m_SeenInterlaceFlag |= interlaced;

  if (!(flags & DVD_CODEC_CTRL_NO_POSTPROC) &&
      interlaced &&
      method != VS_INTERLACEMETHOD_NONE)
  {
    if (!m_config.processInfo->Supports(method))
      method = VS_INTERLACEMETHOD_VDPAU_TEMPORAL;

    if (method == VS_INTERLACEMETHOD_VDPAU_BOB ||
        method == VS_INTERLACEMETHOD_VDPAU_TEMPORAL ||
        method == VS_INTERLACEMETHOD_VDPAU_TEMPORAL_HALF ||
        method == VS_INTERLACEMETHOD_VDPAU_TEMPORAL_SPATIAL ||
        method == VS_INTERLACEMETHOD_VDPAU_TEMPORAL_SPATIAL_HALF)
    {
      if(method == VS_INTERLACEMETHOD_VDPAU_TEMPORAL_HALF ||
         method == VS_INTERLACEMETHOD_VDPAU_TEMPORAL_SPATIAL_HALF ||
         !g_graphicsContext.IsFullScreenVideo())
        m_mixersteps = 1;
      else
      {
        m_mixersteps = 2;
        m_config.stats->SetCanSkipDeint(true);
      }

      if (m_mixerInput[1].DVDPic.iFlags & DVD_CODEC_CTRL_SKIPDEINT)
      {
        m_mixersteps = 1;
      }

      if(m_mixerInput[1].DVDPic.iFlags & DVP_FLAG_TOP_FIELD_FIRST)
        m_mixerfield = VDP_VIDEO_MIXER_PICTURE_STRUCTURE_TOP_FIELD;
      else
        m_mixerfield = VDP_VIDEO_MIXER_PICTURE_STRUCTURE_BOTTOM_FIELD;

      m_mixerInput[1].DVDPic.iFlags &= ~(DVP_FLAG_TOP_FIELD_FIRST |
                                        DVP_FLAG_REPEAT_TOP_FIELD |
                                        DVP_FLAG_INTERLACED);
      m_mixerInput[1].isYuv = false;
      m_config.useInteropYuv = false;
    }
    else if (method == VS_INTERLACEMETHOD_RENDER_BOB)
    {
      m_mixersteps = 1;
      m_mixerfield = VDP_VIDEO_MIXER_PICTURE_STRUCTURE_FRAME;
      m_mixerInput[1].isYuv = true;
      m_config.useInteropYuv = true;
    }
  }
  else
  {
    m_mixersteps = 1;
    m_mixerfield = VDP_VIDEO_MIXER_PICTURE_STRUCTURE_FRAME;

    if (m_config.useInteropYuv)
      m_mixerInput[1].isYuv = true;
    else
    {
      m_mixerInput[1].DVDPic.iFlags &= ~(DVP_FLAG_TOP_FIELD_FIRST |
                                        DVP_FLAG_REPEAT_TOP_FIELD |
                                        DVP_FLAG_INTERLACED);
      m_mixerInput[1].isYuv = false;
    }
  }
  m_mixerstep = 0;
  //m_mixerInput[1].DVDPic.format = RENDER_FMT_VDPAU;

  m_processPicture.crop = false;
  if (!m_mixerInput[1].isYuv)
  {
    m_processPicture.outputSurface = m_outputSurfaces.front();
    m_mixerInput[1].DVDPic.iWidth = m_config.outWidth;
    m_mixerInput[1].DVDPic.iHeight = m_config.outHeight;
    if (m_SeenInterlaceFlag)
    {
      double ratio = (double)m_mixerInput[1].DVDPic.iDisplayHeight / m_mixerInput[1].DVDPic.iHeight;
      m_mixerInput[1].DVDPic.iDisplayHeight = lrint(ratio*(m_mixerInput[1].DVDPic.iHeight-NUM_CROP_PIX*2));
      m_processPicture.crop = true;
    }
  }
  else
  {
    m_mixerInput[1].DVDPic.iWidth = m_config.vidWidth;
    m_mixerInput[1].DVDPic.iHeight = m_config.vidHeight;
  }

  m_processPicture.isYuv = m_mixerInput[1].isYuv;
  m_processPicture.DVDPic.SetParams(m_mixerInput[1].DVDPic);
  m_processPicture.videoSurface = m_mixerInput[1].videoSurface;
}

void CMixer::FiniCycle()
{
  // Keep video surfaces for one 2 cycles longer than used
  // by mixer. This avoids blocking in decoder.
  // NVidia recommends num_ref + 5
  size_t surfToKeep = 5;

  if (m_mixerInput.size() > 0 &&
      (m_mixerInput[0].videoSurface == VDP_INVALID_HANDLE))
    surfToKeep = 1;

  while (m_mixerInput.size() > surfToKeep)
  {
    CVdpauDecodedPicture &tmp = m_mixerInput.back();
    if (!m_processPicture.isYuv)
    {
      m_config.videoSurfaces->ClearRender(tmp.videoSurface);
    }
    m_mixerInput.pop_back();
  }

  if (surfToKeep == 1)
    m_mixerInput.clear();
}

void CMixer::ProcessPicture()
{
  if (m_processPicture.isYuv)
    return;

  VdpStatus vdp_st;

  if (m_mixerstep == 1)
  {
    if(m_mixerfield == VDP_VIDEO_MIXER_PICTURE_STRUCTURE_TOP_FIELD)
      m_mixerfield = VDP_VIDEO_MIXER_PICTURE_STRUCTURE_BOTTOM_FIELD;
    else
      m_mixerfield = VDP_VIDEO_MIXER_PICTURE_STRUCTURE_TOP_FIELD;
  }

  VdpVideoSurface past_surfaces[4] = { VDP_INVALID_HANDLE, VDP_INVALID_HANDLE, VDP_INVALID_HANDLE, VDP_INVALID_HANDLE };
  VdpVideoSurface futu_surfaces[2] = { VDP_INVALID_HANDLE, VDP_INVALID_HANDLE };
  uint32_t pastCount = 4;
  uint32_t futuCount = 2;

  if(m_mixerfield == VDP_VIDEO_MIXER_PICTURE_STRUCTURE_FRAME)
  {
    // use only 2 past 1 future for progressive/weave
    // (only used for postproc anyway eg noise reduction)
    if (m_mixerInput.size() > 3)
      past_surfaces[1] = m_mixerInput[3].videoSurface;
    if (m_mixerInput.size() > 2)
      past_surfaces[0] = m_mixerInput[2].videoSurface;
    if (m_mixerInput.size() > 1)
      futu_surfaces[0] = m_mixerInput[0].videoSurface;
    pastCount = 2;
    futuCount = 1;
  }
  else
  {
    if(m_mixerstep == 0)
    { // first field
      if (m_mixerInput.size() > 3)
      {
        past_surfaces[3] = m_mixerInput[3].videoSurface;
        past_surfaces[2] = m_mixerInput[3].videoSurface;
      }
      if (m_mixerInput.size() > 2)
      {
        past_surfaces[1] = m_mixerInput[2].videoSurface;
        past_surfaces[0] = m_mixerInput[2].videoSurface;
      }
      futu_surfaces[0] = m_mixerInput[1].videoSurface;
      futu_surfaces[1] = m_mixerInput[0].videoSurface;
    }
    else
    { // second field
      if (m_mixerInput.size() > 3)
      {
        past_surfaces[3] = m_mixerInput[3].videoSurface;
      }
      if (m_mixerInput.size() > 2)
      {
        past_surfaces[2] = m_mixerInput[2].videoSurface;
        past_surfaces[1] = m_mixerInput[2].videoSurface;
      }
      past_surfaces[0] = m_mixerInput[1].videoSurface;
      futu_surfaces[0] = m_mixerInput[0].videoSurface;
      futu_surfaces[1] = m_mixerInput[0].videoSurface;

      if (m_mixerInput[0].DVDPic.pts != DVD_NOPTS_VALUE &&
          m_mixerInput[1].DVDPic.pts != DVD_NOPTS_VALUE)
      {
        m_processPicture.DVDPic.pts = m_mixerInput[1].DVDPic.pts +
                                     (m_mixerInput[0].DVDPic.pts -
                                      m_mixerInput[1].DVDPic.pts) / 2;
      }
      else
        m_processPicture.DVDPic.pts = DVD_NOPTS_VALUE;
      m_processPicture.DVDPic.dts = DVD_NOPTS_VALUE;
    }
    m_processPicture.DVDPic.iRepeatPicture = 0.0;
  } // interlaced

  VdpRect sourceRect;
  sourceRect.x0 = 0;
  sourceRect.y0 = 0;
  sourceRect.x1 = m_config.vidWidth;
  sourceRect.y1 = m_config.vidHeight;

  VdpRect destRect;
  destRect.x0 = 0;
  destRect.y0 = 0;
  destRect.x1 = m_config.outWidth;
  destRect.y1 = m_config.outHeight;

  // start vdpau video mixer
  vdp_st = m_config.context->GetProcs().vdp_video_mixer_render(m_videoMixer,
                                VDP_INVALID_HANDLE,
                                0,
                                m_mixerfield,
                                pastCount,
                                past_surfaces,
                                m_mixerInput[1].videoSurface,
                                futuCount,
                                futu_surfaces,
                                &sourceRect,
                                m_processPicture.outputSurface,
                                &destRect,
                                &destRect,
                                0,
                                NULL);
  CheckStatus(vdp_st, __LINE__);
}


bool CMixer::CheckStatus(VdpStatus vdp_st, int line)
{
  if (vdp_st != VDP_STATUS_OK)
  {
    CLog::Log(LOGERROR, " (VDPAU) Error: %s(%d) at %s:%d\n", m_config.context->GetProcs().vdp_get_error_string(vdp_st), vdp_st, __FILE__, line);
    m_vdpError = true;
    return true;
  }
  return false;
}

//-----------------------------------------------------------------------------
// Output
//-----------------------------------------------------------------------------
COutput::COutput(CDecoder &decoder, CEvent *inMsgEvent) :
  CThread("Vdpau Output"),
  m_controlPort("OutputControlPort", inMsgEvent, &m_outMsgEvent),
  m_dataPort("OutputDataPort", inMsgEvent, &m_outMsgEvent),
  m_vdpau(decoder),
  m_mixer(&m_outMsgEvent)
{
  m_inMsgEvent = inMsgEvent;
  m_bufferPool = std::make_shared<CVdpauBufferPool>(decoder);
}

void COutput::Start()
{
  Create();
}

COutput::~COutput()
{
  Dispose();
}

void COutput::Dispose()
{
  CSingleLock lock(g_graphicsContext);
  m_bStop = true;
  m_outMsgEvent.Set();
  StopThread();
  m_controlPort.Purge();
  m_dataPort.Purge();
}

void COutput::OnStartup()
{
  CLog::Log(LOGNOTICE, "COutput::OnStartup: Output Thread created");
}

void COutput::OnExit()
{
  CLog::Log(LOGNOTICE, "COutput::OnExit: Output Thread terminated");
}

enum OUTPUT_STATES
{
  O_TOP = 0,                      // 0
  O_TOP_ERROR,                    // 1
  O_TOP_UNCONFIGURED,             // 2
  O_TOP_CONFIGURED,               // 3
  O_TOP_CONFIGURED_IDLE,          // 4
  O_TOP_CONFIGURED_WORK,          // 5
};

int VDPAU_OUTPUT_parentStates[] = {
    -1,
    0, //TOP_ERROR
    0, //TOP_UNCONFIGURED
    0, //TOP_CONFIGURED
    3, //TOP_CONFIGURED_IDLE
    3, //TOP_CONFIGURED_WORK
};

void COutput::StateMachine(int signal, Protocol *port, Message *msg)
{
  for (int state = m_state; ; state = VDPAU_OUTPUT_parentStates[state])
  {
    switch (state)
    {
    case O_TOP: // TOP
      if (port == &m_controlPort)
      {
        switch (signal)
        {
        case COutputControlProtocol::FLUSH:
          msg->Reply(COutputControlProtocol::ACC);
          return;
        case COutputControlProtocol::PRECLEANUP:
          msg->Reply(COutputControlProtocol::ACC);
          return;
        default:
          break;
        }
      }
      else if (port == &m_dataPort)
      {
        switch (signal)
        {
        case COutputDataProtocol::RETURNPIC:
          CVdpauRenderPicture *pic;
          pic = *((CVdpauRenderPicture**)msg->data);
          QueueReturnPicture(pic);
          return;
        default:
          break;
        }
      }
      {
        std::string portName = port == NULL ? "timer" : port->portName;
        CLog::Log(LOGWARNING, "COutput::%s - signal: %d form port: %s not handled for state: %d", __FUNCTION__, signal, portName.c_str(), m_state);
      }
      return;

    case O_TOP_ERROR:
      break;

    case O_TOP_UNCONFIGURED:
      if (port == &m_controlPort)
      {
        switch (signal)
        {
        case COutputControlProtocol::INIT:
          CVdpauConfig *data;
          data = (CVdpauConfig*)msg->data;
          if (data)
          {
            m_config = *data;
          }
          Init();
          Message *reply;
          if (m_mixer.m_controlPort.SendOutMessageSync(CMixerControlProtocol::INIT,
                                     &reply, 1000, &m_config, sizeof(m_config)))
          {
            if (reply->signal != CMixerControlProtocol::ACC)
              m_vdpError = true;
            reply->Release();
          }

          // set initial number of
          m_bufferPool->numOutputSurfaces = 4;
          EnsureBufferPool();
          if (!m_vdpError)
          {
            m_state = O_TOP_CONFIGURED_IDLE;
            msg->Reply(COutputControlProtocol::ACC, &m_config, sizeof(m_config));
          }
          else
          {
            m_state = O_TOP_ERROR;
            msg->Reply(COutputControlProtocol::ERROR);
          }
          return;
        default:
          break;
        }
      }
      break;

    case O_TOP_CONFIGURED:
      if (port == &m_controlPort)
      {
        switch (signal)
        {
        case COutputControlProtocol::FLUSH:
          Flush();
          msg->Reply(COutputControlProtocol::ACC);
          return;
        case COutputControlProtocol::PRECLEANUP:
          Flush();
          PreCleanup();
          msg->Reply(COutputControlProtocol::ACC);
          return;
        default:
          break;
        }
      }
      else if (port == &m_dataPort)
      {
        switch (signal)
        {
        case COutputDataProtocol::NEWFRAME:
          CVdpauDecodedPicture *frame;
          frame = (CVdpauDecodedPicture*)msg->data;
          if (frame)
          {
            m_mixer.m_dataPort.SendOutMessage(CMixerDataProtocol::FRAME,
                                               frame,sizeof(CVdpauDecodedPicture));
          }
          return;
        case COutputDataProtocol::RETURNPIC:
          CVdpauRenderPicture *pic;
          pic = *((CVdpauRenderPicture**)msg->data);
          QueueReturnPicture(pic);
          m_controlPort.SendInMessage(COutputControlProtocol::STATS);
          m_state = O_TOP_CONFIGURED_WORK;
          m_extTimeout = 0;
          return;
        default:
          break;
        }
      }
      else if (port == &m_mixer.m_dataPort)
      {
        switch (signal)
        {
        case CMixerDataProtocol::PICTURE:
          CVdpauProcessedPicture *pic;
          pic = (CVdpauProcessedPicture*)msg->data;
          m_bufferPool->processedPics.push(*pic);
          m_state = O_TOP_CONFIGURED_WORK;
          m_extTimeout = 0;
          return;
        default:
          break;
        }
      }
      break;

    case O_TOP_CONFIGURED_IDLE:
      if (port == NULL) // timeout
      {
        switch (signal)
        {
        case COutputControlProtocol::TIMEOUT:
          m_extTimeout = 100;
          if (HasWork())
          {
            m_state = O_TOP_CONFIGURED_WORK;
            m_extTimeout = 0;
          }
          return;
        default:
          break;
        }
      }
      break;

    case O_TOP_CONFIGURED_WORK:
      if (port == NULL) // timeout
      {
        switch (signal)
        {
        case COutputControlProtocol::TIMEOUT:
          if (HasWork())
          {
            CVdpauRenderPicture *pic;
            pic = ProcessMixerPicture();
            if (pic)
            {
              m_config.stats->DecProcessed();
              m_config.stats->IncRender();
              m_dataPort.SendInMessage(COutputDataProtocol::PICTURE, &pic, sizeof(pic));
            }
            m_extTimeout = 1;
          }
          else
          {
            m_state = O_TOP_CONFIGURED_IDLE;
            m_extTimeout = 0;
          }
          return;
        default:
          break;
        }
      }
      break;

    default: // we are in no state, should not happen
      CLog::Log(LOGERROR, "COutput::%s - no valid state: %d", __FUNCTION__, m_state);
      return;
    }
  } // for
}

void COutput::Process()
{
  Message *msg = NULL;
  Protocol *port = NULL;
  bool gotMsg;

  m_state = O_TOP_UNCONFIGURED;
  m_extTimeout = 1000;
  m_bStateMachineSelfTrigger = false;

  while (!m_bStop)
  {
    gotMsg = false;

    if (m_bStateMachineSelfTrigger)
    {
      m_bStateMachineSelfTrigger = false;
      // self trigger state machine
      StateMachine(msg->signal, port, msg);
      if (!m_bStateMachineSelfTrigger)
      {
        msg->Release();
        msg = NULL;
      }
      continue;
    }
    // check control port
    else if (m_controlPort.ReceiveOutMessage(&msg))
    {
      gotMsg = true;
      port = &m_controlPort;
    }
    // check data port
    else if (m_dataPort.ReceiveOutMessage(&msg))
    {
      gotMsg = true;
      port = &m_dataPort;
    }
    // check mixer data port
    else if (m_mixer.m_dataPort.ReceiveInMessage(&msg))
    {
      gotMsg = true;
      port = &m_mixer.m_dataPort;
    }
    if (gotMsg)
    {
      StateMachine(msg->signal, port, msg);
      if (!m_bStateMachineSelfTrigger)
      {
        msg->Release();
        msg = NULL;
      }
      continue;
    }

    // wait for message
    else if (m_outMsgEvent.WaitMSec(m_extTimeout))
    {
      continue;
    }
    // time out
    else
    {
      msg = m_controlPort.GetMessage();
      msg->signal = COutputControlProtocol::TIMEOUT;
      port = 0;
      // signal timeout to state machine
      StateMachine(msg->signal, port, msg);
      if (!m_bStateMachineSelfTrigger)
      {
        msg->Release();
        msg = NULL;
      }
    }
  }
  Flush();
  Uninit();
}

bool COutput::Init()
{
  m_mixer.Start();
  m_vdpError = false;

  return true;
}

bool COutput::Uninit()
{
  m_mixer.Dispose();
  ProcessSyncPicture();
  ReleaseBufferPool();
  return true;
}

void COutput::Flush()
{
  if (m_mixer.IsActive())
  {
    Message *reply;
    if (m_mixer.m_controlPort.SendOutMessageSync(CMixerControlProtocol::FLUSH,
                                                 &reply,
                                                 2000))
    {
      reply->Release();
    }
    else
      CLog::Log(LOGERROR, "Coutput::%s - failed to flush mixer", __FUNCTION__);
  }

  Message *msg;

  while (m_dataPort.ReceiveInMessage(&msg))
  {
    if (msg->signal == COutputDataProtocol::PICTURE)
    {
      CVdpauRenderPicture *pic;
      pic = *((CVdpauRenderPicture**)msg->data);
      pic->Release();
    }
    msg->Release();
  }

  while (m_dataPort.ReceiveOutMessage(&msg))
  {
    if (msg->signal == COutputDataProtocol::NEWFRAME)
    {
      CVdpauDecodedPicture pic = *reinterpret_cast<CVdpauDecodedPicture*>(msg->data);
      m_config.videoSurfaces->ClearRender(pic.videoSurface);
    }
    else if (msg->signal == COutputDataProtocol::RETURNPIC)
    {
      CVdpauRenderPicture *pic;
      pic = *((CVdpauRenderPicture**)msg->data);
      QueueReturnPicture(pic);
    }
    msg->Release();
  }

  while (m_mixer.m_dataPort.ReceiveInMessage(&msg))
  {
    if (msg->signal == CMixerDataProtocol::PICTURE)
    {
      CVdpauProcessedPicture pic = *reinterpret_cast<CVdpauProcessedPicture*>(msg->data);
      m_bufferPool->processedPics.push(pic);
    }
    msg->Release();
  }

  // reset used render flag which was cleared on mixer flush
  for (auto &awayPic : m_bufferPool->processedPicsAway)
  {
    if (awayPic.isYuv)
    {
      m_config.videoSurfaces->MarkRender(awayPic.videoSurface);
    }
  }

  // clear processed pics
  while(!m_bufferPool->processedPics.empty())
  {
    CVdpauProcessedPicture procPic = m_bufferPool->processedPics.front();
    if (!procPic.isYuv)
    {
      m_mixer.m_dataPort.SendOutMessage(CMixerDataProtocol::BUFFER, &procPic.outputSurface, sizeof(procPic.outputSurface));
    }
    else if (procPic.isYuv)
    {
      m_config.videoSurfaces->ClearRender(procPic.videoSurface);
    }
    m_bufferPool->processedPics.pop();
  }
}

bool COutput::HasWork()
{
  if (!m_bufferPool->processedPics.empty() && m_bufferPool->HasFree())
    return true;
  return false;
}

CVdpauRenderPicture* COutput::ProcessMixerPicture()
{
  CVdpauRenderPicture *retPic = NULL;

  if (!m_bufferPool->processedPics.empty() && m_bufferPool->HasFree())
  {
    retPic = m_bufferPool->GetVdpau();
    CVdpauProcessedPicture procPic = m_bufferPool->processedPics.front();
    procPic.id = m_bufferPool->procPicId++;
    m_bufferPool->processedPics.pop();
    m_bufferPool->processedPicsAway.push_back(procPic);
    retPic->procPic = procPic;
    retPic->device = reinterpret_cast<void*>(m_config.context->GetDevice());
    retPic->procFunc = reinterpret_cast<void*>(m_config.context->GetProcs().vdp_get_proc_address);
    retPic->ident = m_config.vdpau + m_config.resetCounter;

    retPic->DVDPic.SetParams(procPic.DVDPic);
    if (!procPic.isYuv)
    {
      m_config.useInteropYuv = false;
      m_bufferPool->numOutputSurfaces = NUM_RENDER_PICS;
      EnsureBufferPool();
      retPic->width = m_config.outWidth;
      retPic->height = m_config.outHeight;
      retPic->crop.x1 = 0;
      retPic->crop.y1 = procPic.crop ? NUM_CROP_PIX : 0;
      retPic->crop.x2 = m_config.outWidth;
      retPic->crop.y2 = m_config.outHeight - retPic->crop.y1;
    }
    else
    {
      m_config.useInteropYuv = true;
      retPic->width = m_config.surfaceWidth;
      retPic->height = m_config.surfaceHeight;
      retPic->crop.x1 = 0;
      retPic->crop.y1 = 0;
      retPic->crop.x2 = m_config.surfaceWidth - m_config.vidWidth;
      retPic->crop.y2 = m_config.surfaceHeight - m_config.vidHeight;
    }
  }
  return retPic;
}

void COutput::QueueReturnPicture(CVdpauRenderPicture *pic)
{
  m_bufferPool->QueueReturnPicture(pic);
  ProcessSyncPicture();
}

void COutput::ProcessSyncPicture()
{
  CVdpauRenderPicture *pic;

  pic = m_bufferPool->ProcessSyncPicture();

  while (pic != nullptr)
  {
    ProcessReturnPicture(pic);
    pic = m_bufferPool->ProcessSyncPicture();
  }
}

void COutput::ProcessReturnPicture(CVdpauRenderPicture *pic)
{
  for (auto it=m_bufferPool->processedPicsAway.begin(); it!=m_bufferPool->processedPicsAway.end(); ++it)
  {
    if (it->id == pic->procPic.id)
    {
      if (pic->procPic.isYuv)
      {
        VdpVideoSurface surf = pic->procPic.videoSurface;
        if (surf != VDP_INVALID_HANDLE)
          m_config.videoSurfaces->ClearRender(surf);
      }
      else
      {
        VdpOutputSurface outSurf = pic->procPic.outputSurface;
        if (outSurf != VDP_INVALID_HANDLE)
          m_mixer.m_dataPort.SendOutMessage(CMixerDataProtocol::BUFFER, &outSurf, sizeof(outSurf));
      }
      m_bufferPool->processedPicsAway.erase(it);
      break;
    }
  }
}

bool COutput::EnsureBufferPool()
{
  VdpStatus vdp_st;

  // Creation of outputSurfaces
  VdpOutputSurface outputSurface;
  for (int i = m_bufferPool->outputSurfaces.size(); i < m_bufferPool->numOutputSurfaces; i++)
  {
    vdp_st = m_config.context->GetProcs().vdp_output_surface_create(m_config.context->GetDevice(),
                                      VDP_RGBA_FORMAT_B8G8R8A8,
                                      m_config.outWidth,
                                      m_config.outHeight,
                                      &outputSurface);
    if (CheckStatus(vdp_st, __LINE__))
      return false;
    m_bufferPool->outputSurfaces.push_back(outputSurface);

    m_mixer.m_dataPort.SendOutMessage(CMixerDataProtocol::BUFFER,
                                      &outputSurface,
                                      sizeof(VdpOutputSurface));
    CLog::Log(LOGNOTICE, "VDPAU::COutput::InitBufferPool - Output Surface created");
  }
  return true;
}

void COutput::ReleaseBufferPool()
{
  VdpStatus vdp_st;

  // release all output surfaces
  m_bufferPool->InvalidateUsed();
  for (unsigned int i = 0; i < m_bufferPool->outputSurfaces.size(); ++i)
  {
    if (m_bufferPool->outputSurfaces[i] == VDP_INVALID_HANDLE)
      continue;
    vdp_st = m_config.context->GetProcs().vdp_output_surface_destroy(m_bufferPool->outputSurfaces[i]);
    CheckStatus(vdp_st, __LINE__);
  }
  m_bufferPool->outputSurfaces.clear();

  ProcessSyncPicture();
}

void COutput::PreCleanup()
{

  VdpStatus vdp_st;

  m_mixer.Dispose();
  ProcessSyncPicture();

  for (unsigned int i = 0; i < m_bufferPool->outputSurfaces.size(); ++i)
  {
    if (m_bufferPool->outputSurfaces[i] == VDP_INVALID_HANDLE)
      continue;

    // check if output surface is in use
    bool used = false;
    for (auto &picAway : m_bufferPool->processedPicsAway)
    {
      if (picAway.outputSurface == m_bufferPool->outputSurfaces[i])
      {
        used = true;
        break;
      }
    }
    if (used)
      continue;

    vdp_st = m_config.context->GetProcs().vdp_output_surface_destroy(m_bufferPool->outputSurfaces[i]);
    CheckStatus(vdp_st, __LINE__);

    m_bufferPool->outputSurfaces[i] = VDP_INVALID_HANDLE;
    CLog::Log(LOGDEBUG, LOGVIDEO, "VDPAU::PreCleanup - released output surface");
  }

}

void COutput::InitMixer()
{
  for (unsigned int i = 0; i < m_bufferPool->outputSurfaces.size(); ++i)
  {
    m_mixer.m_dataPort.SendOutMessage(CMixerDataProtocol::BUFFER,
                                      &m_bufferPool->outputSurfaces[i],
                                      sizeof(VdpOutputSurface));
  }
}

bool COutput::CheckStatus(VdpStatus vdp_st, int line)
{
  if (vdp_st != VDP_STATUS_OK)
  {
    CLog::Log(LOGERROR, " (VDPAU) Error: %s(%d) at %s:%d\n", m_config.context->GetProcs().vdp_get_error_string(vdp_st), vdp_st, __FILE__, line);
    m_vdpError = true;
    return true;
  }
  return false;
}
