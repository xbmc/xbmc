/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *  See LICENSES/README.md for more information.
 */

#include "VAAPI.h"

#include "DVDVideoCodec.h"
#include "ServiceBroker.h"
#include "cores/VideoPlayer/DVDCodecs/DVDCodecUtils.h"
#include "cores/VideoPlayer/DVDCodecs/DVDFactoryCodec.h"
#include "cores/VideoPlayer/Interface/TimingConstants.h"
#include "cores/VideoPlayer/Process/ProcessInfo.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "settings/lib/Setting.h"
#include "utils/MemUtils.h"
#include "utils/StringUtils.h"
#include "utils/XTimeUtils.h"
#include "utils/log.h"
#include "windowing/GraphicContext.h"

#include <array>
#include <mutex>

#include <drm_fourcc.h>
#include <va/va_drm.h>
#include <va/va_drmcommon.h>

extern "C" {
#include <libavutil/avutil.h>
#include <libavutil/hwcontext.h>
#include <libavutil/hwcontext_vaapi.h>
#include <libavutil/opt.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
}

#include "system_egl.h"

#include <EGL/eglext.h>
#include <va/va_vpp.h>
#include <xf86drm.h>

#if VA_CHECK_VERSION(1, 0, 0)
# include <va/va_str.h>
#endif

using namespace VAAPI;
using namespace std::chrono_literals;

#define NUM_RENDER_PICS 7

constexpr auto SETTING_VIDEOPLAYER_USEVAAPI = "videoplayer.usevaapi";
constexpr auto SETTING_VIDEOPLAYER_USEVAAPIAV1 = "videoplayer.usevaapiav1";
constexpr auto SETTING_VIDEOPLAYER_USEVAAPIHEVC = "videoplayer.usevaapihevc";
constexpr auto SETTING_VIDEOPLAYER_USEVAAPIMPEG2 = "videoplayer.usevaapimpeg2";
constexpr auto SETTING_VIDEOPLAYER_USEVAAPIMPEG4 = "videoplayer.usevaapimpeg4";
constexpr auto SETTING_VIDEOPLAYER_USEVAAPIVC1 = "videoplayer.usevaapivc1";
constexpr auto SETTING_VIDEOPLAYER_USEVAAPIVP8 = "videoplayer.usevaapivp8";
constexpr auto SETTING_VIDEOPLAYER_USEVAAPIVP9 = "videoplayer.usevaapivp9";
constexpr auto SETTING_VIDEOPLAYER_PREFERVAAPIRENDER = "videoplayer.prefervaapirender";

void VAAPI::VaErrorCallback(void *user_context, const char *message)
{
  std::string str{message};
  CLog::Log(LOGERROR, "libva error: {}", StringUtils::TrimRight(str));
}

void VAAPI::VaInfoCallback(void *user_context, const char *message)
{
  std::string str{message};
  CLog::Log(LOGDEBUG, "libva info: {}", StringUtils::TrimRight(str));
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

CVAAPIContext *CVAAPIContext::m_context = 0;
CCriticalSection CVAAPIContext::m_section;

CVAAPIContext::CVAAPIContext()
{
  m_context = 0;
  m_refCount = 0;
  m_profiles = NULL;
}

void CVAAPIContext::Release(CDecoder *decoder)
{
  std::unique_lock<CCriticalSection> lock(m_section);

  auto it = find(m_decoders.begin(), m_decoders.end(), decoder);
  if (it != m_decoders.end())
    m_decoders.erase(it);

  m_refCount--;
  if (m_refCount <= 0)
  {
    Close();
    delete this;
    m_context = 0;
  }
}

void CVAAPIContext::Close()
{
  CLog::Log(LOGINFO, "VAAPI::Close - closing decoder context");
  if (m_renderNodeFD >= 0)
  {
    close(m_renderNodeFD);
  }

  DestroyContext();
}

bool CVAAPIContext::EnsureContext(CVAAPIContext **ctx, CDecoder *decoder)
{
  std::unique_lock<CCriticalSection> lock(m_section);

  if (m_context)
  {
    m_context->m_refCount++;
    *ctx = m_context;
    if (!m_context->IsValidDecoder(decoder))
      m_context->m_decoders.push_back(decoder);
    return true;
  }

  m_context = new CVAAPIContext();
  *ctx = m_context;
  {
    std::unique_lock<CCriticalSection> gLock(CServiceBroker::GetWinSystem()->GetGfxContext());
    if (!m_context->CreateContext())
    {
      delete m_context;
      m_context = 0;
      *ctx = NULL;
      return false;
    }
  }

  m_context->m_refCount++;

  if (!m_context->IsValidDecoder(decoder))
    m_context->m_decoders.push_back(decoder);
  *ctx = m_context;
  return true;
}

void CVAAPIContext::SetValidDRMVaDisplayFromRenderNode()
{
  int const buf_size{128};
  char name[buf_size];
  int fd{-1};

  // 128 is the start of the NUM in renderD<NUM>
  for (int i = 128; i < (128 + 16); i++)
  {
    snprintf(name, buf_size, "/dev/dri/renderD%d", i);

    fd = open(name, O_RDWR);

    if (fd < 0)
    {
      continue;
    }

    auto display = vaGetDisplayDRM(fd);

    if (display != nullptr)
    {
      m_renderNodeFD = fd;
      m_display = display;
      return;
    }
    close(fd);
  }

  CLog::Log(LOGERROR, "Failed to find any open render nodes in /dev/dri/renderD<num>");
}

void CVAAPIContext::SetVaDisplayForSystem()
{
  m_display = CDecoder::m_pWinSystem->GetVADisplay();

  // Fallback to DRM
  if (!m_display)
  {
    // Render nodes depends on kernel >= 3.15
    SetValidDRMVaDisplayFromRenderNode();
  }
}

bool CVAAPIContext::CreateContext()
{
  SetVaDisplayForSystem();

  if (m_display == nullptr)
  {
    CLog::Log(LOGERROR, "Failed to find any VaDisplays for this system");
    return false;
  }

#if VA_CHECK_VERSION(1, 0, 0)
  vaSetErrorCallback(m_display, VaErrorCallback, nullptr);
  vaSetInfoCallback(m_display, VaInfoCallback, nullptr);
#endif

  int major_version, minor_version;
  if (!CheckSuccess(vaInitialize(m_display, &major_version, &minor_version), "vaInitialize"))
  {
    vaTerminate(m_display);
    m_display = NULL;
    return false;
  }

  CLog::Log(LOGDEBUG, LOGVIDEO, "VAAPI - initialize version {}.{}", major_version, minor_version);
  CLog::Log(LOGDEBUG, LOGVIDEO, "VAAPI - driver in use: {}", vaQueryVendorString(m_display));

  QueryCaps();
  if (!m_profileCount)
    return false;

  return true;
}

void CVAAPIContext::DestroyContext()
{
  delete[] m_profiles;
  if (m_display)
  {
    if (CheckSuccess(vaTerminate(m_display), "vaTerminate"))
    {
      m_display = NULL;
    }
    else
    {
#if VA_CHECK_VERSION(1, 0, 0)
      vaSetErrorCallback(m_display, nullptr, nullptr);
      vaSetInfoCallback(m_display, nullptr, nullptr);
#endif
    }
  }
}

void CVAAPIContext::QueryCaps()
{
  m_profileCount = 0;

  int max_profiles = vaMaxNumProfiles(m_display);
  m_profiles = new VAProfile[max_profiles];

  if (!CheckSuccess(vaQueryConfigProfiles(m_display, m_profiles, &m_profileCount), "vaQueryConfigProfiles"))
    return;

  for(int i = 0; i < m_profileCount; i++)
  {
#if VA_CHECK_VERSION(1, 0, 0)
    CLog::Log(LOGDEBUG, LOGVIDEO, "VAAPI - profile {}", vaProfileStr(m_profiles[i]));
#else
    CLog::Log(LOGDEBUG, LOGVIDEO, "VAAPI - profile {}", m_profiles[i]);
#endif
  }
}

VAConfigAttrib CVAAPIContext::GetAttrib(VAProfile profile)
{
  std::unique_lock<CCriticalSection> lock(m_section);

  VAConfigAttrib attrib;
  attrib.type = VAConfigAttribRTFormat;
  CheckSuccess(vaGetConfigAttributes(m_display, profile, VAEntrypointVLD, &attrib, 1), "vaGetConfigAttributes");

  return attrib;
}

bool CVAAPIContext::SupportsProfile(VAProfile profile)
{
  std::unique_lock<CCriticalSection> lock(m_section);

  for (int i=0; i<m_profileCount; i++)
  {
    if (m_profiles[i] == profile)
      return true;
  }
  return false;
}

VAConfigID CVAAPIContext::CreateConfig(VAProfile profile, VAConfigAttrib attrib)
{
  std::unique_lock<CCriticalSection> lock(m_section);

  VAConfigID config = VA_INVALID_ID;
  CheckSuccess(vaCreateConfig(m_display, profile, VAEntrypointVLD, &attrib, 1, &config), "vaCreateConfig");

  return config;
}

bool CVAAPIContext::CheckSuccess(VAStatus status, const std::string& function)
{
  if (status != VA_STATUS_SUCCESS)
  {
    CLog::Log(LOGERROR, "VAAPI/context {} error: {} ({})", function, vaErrorStr(status), status);
    return false;
  }
  return true;
}

VADisplay CVAAPIContext::GetDisplay()
{
  return m_display;
}

bool CVAAPIContext::IsValidDecoder(CDecoder *decoder)
{
  auto it = find(m_decoders.begin(), m_decoders.end(), decoder);
  if (it != m_decoders.end())
    return true;

  return false;
}

void CVAAPIContext::FFReleaseBuffer(void *opaque, uint8_t *data)
{
  CDecoder *va = static_cast<CDecoder*>(opaque);
  if (m_context && m_context->IsValidDecoder(va))
  {
    va->FFReleaseBuffer(data);
  }
}

//-----------------------------------------------------------------------------
// VAAPI Video Surface states
//-----------------------------------------------------------------------------

#define SURFACE_USED_FOR_REFERENCE 0x01
#define SURFACE_USED_FOR_RENDER    0x02

void CVideoSurfaces::AddSurface(VASurfaceID surf)
{
  std::unique_lock<CCriticalSection> lock(m_section);
  m_state[surf] = 0;
  m_freeSurfaces.push_back(surf);
}

void CVideoSurfaces::ClearReference(VASurfaceID surf)
{
  std::unique_lock<CCriticalSection> lock(m_section);
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

bool CVideoSurfaces::MarkRender(VASurfaceID surf)
{
  std::unique_lock<CCriticalSection> lock(m_section);
  if (m_state.find(surf) == m_state.end())
  {
    CLog::Log(LOGWARNING, "CVideoSurfaces::MarkRender - surface invalid");
    return false;
  }
  auto it = std::find(m_freeSurfaces.begin(), m_freeSurfaces.end(), surf);
  if (it != m_freeSurfaces.end())
  {
    m_freeSurfaces.erase(it);
  }
  m_state[surf] |= SURFACE_USED_FOR_RENDER;
  return true;
}

void CVideoSurfaces::ClearRender(VASurfaceID surf)
{
  std::unique_lock<CCriticalSection> lock(m_section);
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

bool CVideoSurfaces::IsValid(VASurfaceID surf)
{
  std::unique_lock<CCriticalSection> lock(m_section);
  if (m_state.find(surf) != m_state.end())
    return true;
  else
    return false;
}

VASurfaceID CVideoSurfaces::GetFree(VASurfaceID surf)
{
  std::unique_lock<CCriticalSection> lock(m_section);
  if (m_state.find(surf) != m_state.end())
  {
    auto it = std::find(m_freeSurfaces.begin(), m_freeSurfaces.end(), surf);
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
    VASurfaceID freeSurf = m_freeSurfaces.front();
    m_freeSurfaces.pop_front();
    m_state[freeSurf] = SURFACE_USED_FOR_REFERENCE;
    return freeSurf;
  }

  return VA_INVALID_SURFACE;
}

VASurfaceID CVideoSurfaces::GetAtIndex(int idx)
{
  if ((size_t) idx >= m_state.size())
    return VA_INVALID_SURFACE;

  auto it = m_state.begin();
  for(int i = 0; i < idx; i++)
    ++it;
  return it->first;
}

VASurfaceID CVideoSurfaces::RemoveNext(bool skiprender)
{
  std::unique_lock<CCriticalSection> lock(m_section);
  VASurfaceID surf;
  for(auto it = m_state.begin(); it != m_state.end(); ++it)
  {
    if (skiprender && it->second & SURFACE_USED_FOR_RENDER)
      continue;
    surf = it->first;
    m_state.erase(surf);

    auto it2 = std::find(m_freeSurfaces.begin(), m_freeSurfaces.end(), surf);
    if (it2 != m_freeSurfaces.end())
      m_freeSurfaces.erase(it2);
    return surf;
  }
  return VA_INVALID_SURFACE;
}

void CVideoSurfaces::Reset()
{
  std::unique_lock<CCriticalSection> lock(m_section);
  m_freeSurfaces.clear();
  m_state.clear();
}

int CVideoSurfaces::Size()
{
  std::unique_lock<CCriticalSection> lock(m_section);
  return m_state.size();
}

bool CVideoSurfaces::HasFree()
{
  std::unique_lock<CCriticalSection> lock(m_section);
  return !m_freeSurfaces.empty();
}

int CVideoSurfaces::NumFree()
{
  std::unique_lock<CCriticalSection> lock(m_section);
  return m_freeSurfaces.size();
}

bool CVideoSurfaces::HasRefs()
{
  std::unique_lock<CCriticalSection> lock(m_section);
  for (const auto &i : m_state)
  {
    if (i.second & SURFACE_USED_FOR_REFERENCE)
    return true;
  }
  return false;
}

//-----------------------------------------------------------------------------
// VAAPI
//-----------------------------------------------------------------------------

bool CDecoder::m_capGeneral = false;
bool CDecoder::m_capDeepColor = false;
IVaapiWinSystem* CDecoder::m_pWinSystem = nullptr;

CDecoder::CDecoder(CProcessInfo& processInfo) :
  m_vaapiOutput(*this, &m_inMsgEvent),
  m_processInfo(processInfo)
{
  m_vaapiConfig.videoSurfaces = &m_videoSurfaces;

  m_vaapiConfigured = false;
  m_DisplayState = VAAPI_OPEN;
  m_vaapiConfig.context = 0;
  m_vaapiConfig.configId = VA_INVALID_ID;
  m_vaapiConfig.processInfo = &m_processInfo;
  m_getBufferError = 0;
}

CDecoder::~CDecoder()
{
  Close();
}

bool CDecoder::Open(AVCodecContext* avctx, AVCodecContext* mainctx, const enum AVPixelFormat fmt)
{
  if (!m_capGeneral)
    return false;

  // check if user wants to decode this format with VAAPI
  std::map<AVCodecID, std::string> settings_map = {
      {AV_CODEC_ID_H263, SETTING_VIDEOPLAYER_USEVAAPIMPEG4},
      {AV_CODEC_ID_MPEG4, SETTING_VIDEOPLAYER_USEVAAPIMPEG4},
      {AV_CODEC_ID_WMV3, SETTING_VIDEOPLAYER_USEVAAPIVC1},
      {AV_CODEC_ID_VC1, SETTING_VIDEOPLAYER_USEVAAPIVC1},
      {AV_CODEC_ID_MPEG2VIDEO, SETTING_VIDEOPLAYER_USEVAAPIMPEG2},
      {AV_CODEC_ID_VP8, SETTING_VIDEOPLAYER_USEVAAPIVP8},
      {AV_CODEC_ID_VP9, SETTING_VIDEOPLAYER_USEVAAPIVP9},
      {AV_CODEC_ID_HEVC, SETTING_VIDEOPLAYER_USEVAAPIHEVC},
      {AV_CODEC_ID_AV1, SETTING_VIDEOPLAYER_USEVAAPIAV1},
  };

  auto entry = settings_map.find(avctx->codec_id);
  if (entry != settings_map.end())
  {
    auto settingsComponent = CServiceBroker::GetSettingsComponent();
    if (!settingsComponent)
      return false;

    auto settings = settingsComponent->GetSettings();
    if (!settings)
      return false;

    auto setting = settings->GetSetting(entry->second);
    if (!setting)
    {
      CLog::Log(LOGERROR, "Failed to load setting for: {}", entry->second);
      return false;
    }

    bool enabled = settings->GetBool(entry->second) && setting->IsVisible();
    if (!enabled)
      return false;
  }

  CLog::Log(LOGDEBUG, LOGVIDEO, "VAAPI - open decoder");

  if (!CVAAPIContext::EnsureContext(&m_vaapiConfig.context, this))
    return false;

  if(avctx->coded_width  == 0 ||
     avctx->coded_height == 0)
  {
    CLog::Log(LOGWARNING,"VAAPI::Open: no width/height available, can't init");
    return false;
  }

  m_vaapiConfig.driverIsMesa = StringUtils::StartsWith(vaQueryVendorString(m_vaapiConfig.context->GetDisplay()), "Mesa");
  m_vaapiConfig.vidWidth = avctx->width;
  m_vaapiConfig.vidHeight = avctx->height;
  m_vaapiConfig.outWidth = avctx->width;
  m_vaapiConfig.outHeight = avctx->height;
  m_vaapiConfig.surfaceWidth = avctx->coded_width;
  m_vaapiConfig.surfaceHeight = avctx->coded_height;
  m_vaapiConfig.aspect = avctx->sample_aspect_ratio;
  m_vaapiConfig.bitDepth = avctx->bits_per_raw_sample;
  m_DisplayState = VAAPI_OPEN;
  m_vaapiConfigured = false;
  m_presentPicture = nullptr;
  m_getBufferError = 0;

  VAProfile profile;
  switch (avctx->codec_id)
  {
    case AV_CODEC_ID_MPEG2VIDEO:
      profile = VAProfileMPEG2Main;
      if (!m_vaapiConfig.context->SupportsProfile(profile))
        return false;
      break;
    case AV_CODEC_ID_MPEG4:
    case AV_CODEC_ID_H263:
      profile = VAProfileMPEG4AdvancedSimple;
      if (!m_vaapiConfig.context->SupportsProfile(profile))
        return false;
      break;
    case AV_CODEC_ID_H264:
    {
      if (avctx->profile == FF_PROFILE_H264_CONSTRAINED_BASELINE)
      {
        profile = VAProfileH264ConstrainedBaseline;
        if (!m_vaapiConfig.context->SupportsProfile(profile))
          return false;
      }
      else
      {
        if(avctx->profile == FF_PROFILE_H264_MAIN)
        {
          profile = VAProfileH264Main;
          if (m_vaapiConfig.context->SupportsProfile(profile))
            break;
        }
        profile = VAProfileH264High;
        if (!m_vaapiConfig.context->SupportsProfile(profile))
          return false;
      }
      break;
    }
    case AV_CODEC_ID_HEVC:
    {
      if (avctx->profile == FF_PROFILE_HEVC_MAIN_10)
      {
        if (!m_capDeepColor)
          return false;

        profile = VAProfileHEVCMain10;
      }
      else if (avctx->profile == FF_PROFILE_HEVC_MAIN)
        profile = VAProfileHEVCMain;
      else
        profile = VAProfileNone;
      if (!m_vaapiConfig.context->SupportsProfile(profile))
        return false;
      break;
    }
    case AV_CODEC_ID_VP8:
    {
      profile = VAProfileVP8Version0_3;
      if (!m_vaapiConfig.context->SupportsProfile(profile))
        return false;
      break;
    }
    case AV_CODEC_ID_VP9:
    {
      if (avctx->profile == FF_PROFILE_VP9_0)
        profile = VAProfileVP9Profile0;
      else if (avctx->profile == FF_PROFILE_VP9_2)
        profile = VAProfileVP9Profile2;
      else
        profile = VAProfileNone;
      if (!m_vaapiConfig.context->SupportsProfile(profile))
        return false;
      break;
    }
    case AV_CODEC_ID_WMV3:
      profile = VAProfileVC1Main;
      if (!m_vaapiConfig.context->SupportsProfile(profile))
        return false;
      break;
    case AV_CODEC_ID_VC1:
      profile = VAProfileVC1Advanced;
      if (!m_vaapiConfig.context->SupportsProfile(profile))
        return false;
      break;
#if VA_CHECK_VERSION(1, 8, 0)
    case AV_CODEC_ID_AV1:
    {
      if (avctx->profile == FF_PROFILE_AV1_MAIN)
        profile = VAProfileAV1Profile0;
      else if (avctx->profile == FF_PROFILE_AV1_HIGH)
        profile = VAProfileAV1Profile1;
      else
        profile = VAProfileNone;
      if (!m_vaapiConfig.context->SupportsProfile(profile))
        return false;
      break;
    }
#endif
    default:
      return false;
  }

  m_vaapiConfig.profile = profile;
  m_vaapiConfig.attrib = m_vaapiConfig.context->GetAttrib(profile);
  if ((m_vaapiConfig.attrib.value & (VA_RT_FORMAT_YUV420 | VA_RT_FORMAT_YUV420_10BPP)) == 0)
  {
    CLog::Log(LOGERROR, "VAAPI - invalid yuv format {:x}", m_vaapiConfig.attrib.value);
    return false;
  }

  if (avctx->codec_id == AV_CODEC_ID_H264)
  {
    m_vaapiConfig.maxReferences = avctx->refs;
    if (m_vaapiConfig.maxReferences > 16)
      m_vaapiConfig.maxReferences = 16;
    if (m_vaapiConfig.maxReferences < 5)
      m_vaapiConfig.maxReferences = 5;
  }
  else if (avctx->codec_id == AV_CODEC_ID_HEVC)
    m_vaapiConfig.maxReferences = 16;
  else if (avctx->codec_id == AV_CODEC_ID_VP9)
    m_vaapiConfig.maxReferences = 8;
  else if (avctx->codec_id == AV_CODEC_ID_AV1)
    m_vaapiConfig.maxReferences = 21;
  else
    m_vaapiConfig.maxReferences = 2;

  // add an extra surface for safety, some faulty material
  // make ffmpeg require more buffers
  m_vaapiConfig.maxReferences += 6;

  if (!ConfigVAAPI())
  {
    return false;
  }

  m_deviceRef = std::unique_ptr<AVBufferRef, AVBufferRefDeleter>(
      av_hwdevice_ctx_alloc(AV_HWDEVICE_TYPE_VAAPI), AVBufferRefDeleter());

  AVHWDeviceContext* deviceCtx = (AVHWDeviceContext*)m_deviceRef->data;
  AVVAAPIDeviceContext *vaapiDeviceCtx = (AVVAAPIDeviceContext*)deviceCtx->hwctx;
  AVBufferRef* framesRef = av_hwframe_ctx_alloc(m_deviceRef.get());
  AVHWFramesContext *framesCtx = (AVHWFramesContext*)framesRef->data;
  AVVAAPIFramesContext *vaapiFramesCtx = (AVVAAPIFramesContext*)framesCtx->hwctx;

  vaapiDeviceCtx->display = m_vaapiConfig.dpy;
  vaapiDeviceCtx->driver_quirks = AV_VAAPI_DRIVER_QUIRK_RENDER_PARAM_BUFFERS;
  vaapiFramesCtx->nb_attributes = 0;
  vaapiFramesCtx->nb_surfaces = m_videoSurfaces.Size();
  VASurfaceID *surfaceIds = (VASurfaceID*)av_malloc(vaapiFramesCtx->nb_surfaces *  sizeof(VASurfaceID));
  for (int i=0; i<vaapiFramesCtx->nb_surfaces; ++i)
    surfaceIds[i] = m_videoSurfaces.GetAtIndex(i);
  vaapiFramesCtx->surface_ids = surfaceIds;
  framesCtx->format = AV_PIX_FMT_VAAPI;
  framesCtx->width  = avctx->coded_width;
  framesCtx->height = avctx->coded_height;

  avctx->hw_frames_ctx = framesRef;
  avctx->get_buffer2 = CDecoder::FFGetBuffer;
  avctx->slice_flags = SLICE_FLAG_CODED_ORDER|SLICE_FLAG_ALLOW_FIELD;

  return true;
}

void CDecoder::Close()
{
  CLog::Log(LOGINFO, "VAAPI::{}", __FUNCTION__);

  std::unique_lock<CCriticalSection> lock(m_DecoderSection);

  FiniVAAPIOutput();

  m_deviceRef.reset();

  if (m_vaapiConfig.context)
    m_vaapiConfig.context->Release(this);
  m_vaapiConfig.context = 0;
}

long CDecoder::Release()
{
  if (m_presentPicture)
  {
    m_presentPicture->Release();
    m_presentPicture = nullptr;
  }
  // check if we should do some pre-cleanup here
  // a second decoder might need resources
  if (m_vaapiConfigured == true)
  {
    std::unique_lock<CCriticalSection> lock(m_DecoderSection);
    CLog::Log(LOGDEBUG, LOGVIDEO, "VAAPI::Release pre-cleanup");

    std::unique_lock<CCriticalSection> lock1(CServiceBroker::GetWinSystem()->GetGfxContext());
    Message *reply;
    if (m_vaapiOutput.m_controlPort.SendOutMessageSync(COutputControlProtocol::PRECLEANUP, &reply,
                                                       2s))
    {
      bool success = reply->signal == COutputControlProtocol::ACC ? true : false;
      reply->Release();
      if (!success)
      {
        CLog::Log(LOGERROR, "VAAPI::{} - pre-cleanup returned error", __FUNCTION__);
        m_DisplayState = VAAPI_ERROR;
      }
    }
    else
    {
      CLog::Log(LOGERROR, "VAAPI::{} - pre-cleanup timed out", __FUNCTION__);
      m_DisplayState = VAAPI_ERROR;
    }

    VASurfaceID surf;
    while((surf = m_videoSurfaces.RemoveNext(true)) != VA_INVALID_SURFACE)
    {
      CheckSuccess(vaDestroySurfaces(m_vaapiConfig.dpy, &surf, 1), "vaDestroySurfaces");
    }
  }
  return IHardwareDecoder::Release();
}

long CDecoder::ReleasePicReference()
{
  return IHardwareDecoder::Release();
}

int CDecoder::FFGetBuffer(AVCodecContext *avctx, AVFrame *pic, int flags)
{
  ICallbackHWAccel* cb = static_cast<ICallbackHWAccel*>(avctx->opaque);
  CDecoder* va = static_cast<CDecoder*>(cb->GetHWAccel());

  // while we are waiting to recover we can't do anything
  std::unique_lock<CCriticalSection> lock(va->m_DecoderSection);

  if(va->m_DisplayState != VAAPI_OPEN)
  {
    CLog::Log(LOGWARNING, "VAAPI::FFGetBuffer - returning due to awaiting recovery");
    return -1;
  }

  VASurfaceID surf = (VASurfaceID)(uintptr_t)pic->data[3];
  surf = va->m_videoSurfaces.GetFree(surf != 0 ? surf : VA_INVALID_SURFACE);

  if (surf == VA_INVALID_SURFACE)
  {
    uint16_t decoded, processed, render;
    bool vpp;
    va->m_bufferStats.Get(decoded, processed, render, vpp);
    CLog::Log(LOGWARNING, "VAAPI::FFGetBuffer - no surface available - dec: {}, render: {}",
              decoded, render);
    va->m_getBufferError++;
    return -1;
  }

  va->m_getBufferError = 0;

  pic->data[1] = pic->data[2] = NULL;
  pic->data[0] = (uint8_t*)(uintptr_t)surf;
  pic->data[3] = (uint8_t*)(uintptr_t)surf;
  pic->linesize[0] = pic->linesize[1] =  pic->linesize[2] = 0;
  AVBufferRef *buffer = av_buffer_create(pic->data[3], 0, CVAAPIContext::FFReleaseBuffer, va, 0);
  if (!buffer)
  {
    CLog::Log(LOGERROR, "VAAPI::{} - error creating buffer", __FUNCTION__);
    return -1;
  }
  pic->buf[0] = buffer;

#if LIBAVCODEC_VERSION_MAJOR < 60
  pic->reordered_opaque = avctx->reordered_opaque;
#endif

  va->Acquire();
  return 0;
}

void CDecoder::FFReleaseBuffer(uint8_t *data)
{
  {
    VASurfaceID surf;

    std::unique_lock<CCriticalSection> lock(m_DecoderSection);

    surf = (VASurfaceID)(uintptr_t)data;
    m_videoSurfaces.ClearReference(surf);
  }

  IHardwareDecoder::Release();
}

void CDecoder::SetCodecControl(int flags)
{
  m_codecControl = flags & (DVD_CODEC_CTRL_DRAIN | DVD_CODEC_CTRL_HURRY);
}

CDVDVideoCodec::VCReturn CDecoder::Decode(AVCodecContext* avctx, AVFrame* pFrame)
{
  CDVDVideoCodec::VCReturn result = Check(avctx);
  if (result != CDVDVideoCodec::VC_NOBUFFER && result != CDVDVideoCodec::VC_NONE)
    return result;

  std::unique_lock<CCriticalSection> lock(m_DecoderSection);

  if (!m_vaapiConfigured)
    return CDVDVideoCodec::VC_ERROR;

  if (pFrame)
  { // we have a new frame from decoder

    VASurfaceID surf = (VASurfaceID)(uintptr_t)pFrame->data[3];
    // ffmpeg vc-1 decoder does not flush, make sure the data buffer is still valid
    if (!m_videoSurfaces.IsValid(surf))
    {
      CLog::Log(LOGWARNING, "VAAPI::Decode - ignoring invalid buffer");
      return CDVDVideoCodec::VC_BUFFER;
    }
    m_videoSurfaces.MarkRender(surf);

    // send frame to output for processing
    CVaapiDecodedPicture *pic = new CVaapiDecodedPicture();
    static_cast<ICallbackHWAccel*>(avctx->opaque)->GetPictureCommon(&(pic->DVDPic));
    m_codecControl = pic->DVDPic.iFlags & (DVD_CODEC_CTRL_HURRY | DVD_CODEC_CTRL_NO_POSTPROC);
    pic->videoSurface = surf;
    m_bufferStats.IncDecoded();
    CPayloadWrap<CVaapiDecodedPicture> *payload = new CPayloadWrap<CVaapiDecodedPicture>(pic);
    m_vaapiOutput.m_dataPort.SendOutMessage(COutputDataProtocol::NEWFRAME, payload);
  }

  uint16_t decoded, processed, render;
  bool vpp;
  Message *msg;
  while (m_vaapiOutput.m_controlPort.ReceiveInMessage(&msg))
  {
    if (msg->signal == COutputControlProtocol::ERROR)
    {
      m_DisplayState = VAAPI_ERROR;
      msg->Release();
      return CDVDVideoCodec::VC_ERROR;
    }
    msg->Release();
  }

  bool drain = (m_codecControl & DVD_CODEC_CTRL_DRAIN);

  m_bufferStats.Get(decoded, processed, render, vpp);
  // if all pics are drained, break the loop by setting VC_EOF
  if (drain && decoded <= 0 && processed <= 0 && render <= 0)
    return CDVDVideoCodec::VC_EOF;

  while (true)
  {
    // first fill the buffers to keep vaapi busy
    if (!drain && decoded < 2 && processed < 3 && m_videoSurfaces.HasFree())
    {
      return CDVDVideoCodec::VC_BUFFER;
    }
    else if (m_vaapiOutput.m_dataPort.ReceiveInMessage(&msg))
    {
      if (msg->signal == COutputDataProtocol::PICTURE)
      {
        if (m_presentPicture)
        {
          m_presentPicture->Release();
          m_presentPicture = nullptr;
        }

        m_presentPicture = *(CVaapiRenderPicture**)msg->data;
        m_bufferStats.DecRender();
        m_bufferStats.SetParams(0, m_codecControl);
        msg->Release();
        return CDVDVideoCodec::VC_PICTURE;
      }
      msg->Release();
    }
    else if (m_vaapiOutput.m_controlPort.ReceiveInMessage(&msg))
    {
      if (msg->signal == COutputControlProtocol::STATS)
      {
        msg->Release();
        m_bufferStats.Get(decoded, processed, render, vpp);
        if (!drain && decoded < 2 && processed < 3)
        {
          return CDVDVideoCodec::VC_BUFFER;
        }
      }
      else
      {
        msg->Release();
        m_DisplayState = VAAPI_ERROR;
        return CDVDVideoCodec::VC_ERROR;
      }
    }

    if (!m_inMsgEvent.Wait(2000ms))
      break;
  }

  CLog::Log(LOGERROR,
            "VAAPI::{} - timed out waiting for output message - decoded: {}, proc: {}, has free "
            "surface: {}",
            __FUNCTION__, decoded, processed, m_videoSurfaces.HasFree() ? "yes" : "no");
  m_DisplayState = VAAPI_ERROR;

  return CDVDVideoCodec::VC_ERROR;
}

CDVDVideoCodec::VCReturn CDecoder::Check(AVCodecContext* avctx)
{
  EDisplayState state;

  {
    std::unique_lock<CCriticalSection> lock(m_DecoderSection);
    state = m_DisplayState;
  }

  if (state == VAAPI_LOST)
  {
    CLog::Log(LOGDEBUG, LOGVIDEO, "VAAPI::Check waiting for display reset event");
    if (!m_DisplayEvent.Wait(4000ms))
    {
      CLog::Log(LOGERROR, "VAAPI::Check - device didn't reset in reasonable time");
      state = VAAPI_RESET;
    }
    else
    {
      std::unique_lock<CCriticalSection> lock(m_DecoderSection);
      state = m_DisplayState;
    }
  }
  if (state == VAAPI_RESET || state == VAAPI_ERROR)
  {
    std::unique_lock<CCriticalSection> lock(m_DecoderSection);

    avcodec_flush_buffers(avctx);
    FiniVAAPIOutput();
    if (m_vaapiConfig.context)
      m_vaapiConfig.context->Release(this);
    m_vaapiConfig.context = 0;

    if (CVAAPIContext::EnsureContext(&m_vaapiConfig.context, this) && ConfigVAAPI())
    {
      m_DisplayState = VAAPI_OPEN;
    }

    if (state == VAAPI_RESET)
      return CDVDVideoCodec::VC_FLUSHED;
    else
      return CDVDVideoCodec::VC_ERROR;
  }

  if (m_getBufferError > 0 && m_getBufferError < 5)
  {
    // if there is no other error, sleep for a short while
    // in order not to drain player's message queue
    KODI::TIME::Sleep(10ms);

    return CDVDVideoCodec::VC_NOBUFFER;
  }

  return CDVDVideoCodec::VC_NONE;
}

bool CDecoder::GetPicture(AVCodecContext* avctx, VideoPicture* picture)
{
  if (picture->videoBuffer)
  {
    picture->videoBuffer->Release();
    picture->videoBuffer = nullptr;
  }

  std::unique_lock<CCriticalSection> lock(m_DecoderSection);

  if (m_DisplayState != VAAPI_OPEN)
    return false;

  picture->SetParams(m_presentPicture->DVDPic);
  picture->videoBuffer = m_presentPicture;
  m_presentPicture = nullptr;

  return true;
}

void CDecoder::Reset()
{
  std::unique_lock<CCriticalSection> lock(m_DecoderSection);

  if (m_presentPicture)
  {
    m_presentPicture->Release();
    m_presentPicture = nullptr;
  }

  if (!m_vaapiConfigured)
    return;

  Message *reply;
  if (m_vaapiOutput.m_controlPort.SendOutMessageSync(COutputControlProtocol::FLUSH, &reply, 2s))
  {
    bool success = reply->signal == COutputControlProtocol::ACC ? true : false;
    reply->Release();
    if (!success)
    {
      CLog::Log(LOGERROR, "VAAPI::{} - flush returned error", __FUNCTION__);
      m_DisplayState = VAAPI_ERROR;
    }
    else
    {
      m_bufferStats.Reset();
      m_getBufferError = 0;
    }
  }
  else
  {
    CLog::Log(LOGERROR, "VAAPI::{} - flush timed out", __FUNCTION__);
    m_DisplayState = VAAPI_ERROR;
  }
}

bool CDecoder::CanSkipDeint()
{
  return m_bufferStats.CanSkipDeint();
}

bool CDecoder::CheckSuccess(VAStatus status, const std::string& function)
{
  if (status != VA_STATUS_SUCCESS)
  {
    CLog::Log(LOGERROR, "VAAPI/decoder {} error: {} ({})", function, vaErrorStr(status), status);
    m_ErrorCount++;

    if(m_DisplayState == VAAPI_OPEN)
    {
      if (m_ErrorCount > 2)
        m_DisplayState = VAAPI_ERROR;
    }
    return false;
  }
  m_ErrorCount = 0;
  return true;
}

bool CDecoder::ConfigVAAPI()
{
  m_vaapiConfig.dpy = m_vaapiConfig.context->GetDisplay();
  m_vaapiConfig.attrib = m_vaapiConfig.context->GetAttrib(m_vaapiConfig.profile);
  if ((m_vaapiConfig.attrib.value & (VA_RT_FORMAT_YUV420 | VA_RT_FORMAT_YUV420_10BPP)) == 0)
  {
    CLog::Log(LOGERROR, "VAAPI - invalid yuv format {:x}", m_vaapiConfig.attrib.value);
    return false;
  }

  m_vaapiConfig.configId = m_vaapiConfig.context->CreateConfig(m_vaapiConfig.profile,
                                                               m_vaapiConfig.attrib);
  if (m_vaapiConfig.configId == VA_INVALID_ID)
    return false;

  // create surfaces
  unsigned int format = VA_RT_FORMAT_YUV420;
  std::int32_t pixelFormat = VA_FOURCC_NV12;

  if ((m_vaapiConfig.profile == VAProfileHEVCMain10 || m_vaapiConfig.profile == VAProfileVP9Profile2
#if VA_CHECK_VERSION(1, 8, 0)
       || m_vaapiConfig.profile == VAProfileAV1Profile0
#endif
       ) &&
      m_vaapiConfig.bitDepth == 10)
  {
    format = VA_RT_FORMAT_YUV420_10BPP;
    pixelFormat = VA_FOURCC_P010;
  }

  VASurfaceAttrib attribs[1], *attrib;
  attrib = attribs;
  attrib->flags = VA_SURFACE_ATTRIB_SETTABLE;
  attrib->type = VASurfaceAttribPixelFormat;
  attrib->value.type = VAGenericValueTypeInteger;
  attrib->value.value.i = pixelFormat;

  VASurfaceID surfaces[32];
  int nb_surfaces = m_vaapiConfig.maxReferences;
  if (!CheckSuccess(
      vaCreateSurfaces(m_vaapiConfig.dpy, format, m_vaapiConfig.surfaceWidth,
          m_vaapiConfig.surfaceHeight, surfaces,
          nb_surfaces, attribs, 1), "vaCreateSurfaces"))
  {
    return false;
  }
  for (int i=0; i<nb_surfaces; i++)
  {
    m_videoSurfaces.AddSurface(surfaces[i]);
  }

  // initialize output
  std::unique_lock<CCriticalSection> lock(CServiceBroker::GetWinSystem()->GetGfxContext());
  m_vaapiConfig.stats = &m_bufferStats;
  m_bufferStats.Reset();
  m_vaapiOutput.Start();
  Message *reply;
  if (m_vaapiOutput.m_controlPort.SendOutMessageSync(COutputControlProtocol::INIT, &reply, 2s,
                                                     &m_vaapiConfig, sizeof(m_vaapiConfig)))
  {
    bool success = reply->signal == COutputControlProtocol::ACC ? true : false;
    if (!success)
    {
      reply->Release();
      CLog::Log(LOGERROR, "VAAPI::{} - vaapi output returned error", __FUNCTION__);
      m_vaapiOutput.Dispose();
      return false;
    }
    reply->Release();
  }
  else
  {
    CLog::Log(LOGERROR, "VAAPI::{} - failed to init output", __FUNCTION__);
    m_vaapiOutput.Dispose();
    return false;
  }

  m_inMsgEvent.Reset();
  m_vaapiConfigured = true;
  m_ErrorCount = 0;

  return true;
}

void CDecoder::FiniVAAPIOutput()
{
  if (!m_vaapiConfigured)
    return;

  // uninit output
  m_vaapiOutput.Dispose();
  m_vaapiConfigured = false;

  // destroy surfaces
  CLog::Log(LOGDEBUG, LOGVIDEO, "VAAPI::FiniVAAPIOutput destroying {} video surfaces",
            m_videoSurfaces.Size());
  VASurfaceID surf;
  while((surf = m_videoSurfaces.RemoveNext()) != VA_INVALID_SURFACE)
  {
    CheckSuccess(vaDestroySurfaces(m_vaapiConfig.dpy, &surf, 1), "vaDestroySurfaces");
  }
  m_videoSurfaces.Reset();

  // destroy vaapi config
  if (m_vaapiConfig.configId != VA_INVALID_ID)
    CheckSuccess(vaDestroyConfig(m_vaapiConfig.dpy, m_vaapiConfig.configId), "vaDestroyConfig");
  m_vaapiConfig.configId = VA_INVALID_ID;
}

void CDecoder::ReturnRenderPicture(CVaapiRenderPicture *renderPic)
{
  m_vaapiOutput.m_dataPort.SendOutMessage(COutputDataProtocol::RETURNPIC, &renderPic, sizeof(renderPic));
}

IHardwareDecoder* CDecoder::Create(CDVDStreamInfo &hint, CProcessInfo &processInfo, AVPixelFormat fmt)
{
  // https://github.com/FFmpeg/FFmpeg/blob/56450a0ee4fdda160f4039fc2ae33edfd27765c9/doc/APIchanges#L18-L26
  if (fmt == AV_PIX_FMT_VAAPI &&
      CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(SETTING_VIDEOPLAYER_USEVAAPI))
    return new VAAPI::CDecoder(processInfo);

  return nullptr;
}

void CDecoder::Register(IVaapiWinSystem *winSystem, bool deepColor)
{
  m_pWinSystem = winSystem;

  CVaapiConfig config;
  if (!CVAAPIContext::EnsureContext(&config.context, nullptr))
    return;

  m_capGeneral = true;
  m_capDeepColor = deepColor;
  CDVDFactoryCodec::RegisterHWAccel("vaapi", CDecoder::Create);
  config.context->Release(nullptr);

  auto settingsComponent = CServiceBroker::GetSettingsComponent();
  if (!settingsComponent)
    return;

  auto settings = settingsComponent->GetSettings();
  if (!settings)
    return;

  constexpr std::array<const char*, 9> vaapiSettings = {
      SETTING_VIDEOPLAYER_USEVAAPI,     SETTING_VIDEOPLAYER_USEVAAPIMPEG4,
      SETTING_VIDEOPLAYER_USEVAAPIVC1,  SETTING_VIDEOPLAYER_USEVAAPIMPEG2,
      SETTING_VIDEOPLAYER_USEVAAPIVP8,  SETTING_VIDEOPLAYER_USEVAAPIVP9,
      SETTING_VIDEOPLAYER_USEVAAPIHEVC, SETTING_VIDEOPLAYER_PREFERVAAPIRENDER,
      SETTING_VIDEOPLAYER_USEVAAPIAV1};

  for (const auto vaapiSetting : vaapiSettings)
  {
    auto setting = settings->GetSetting(vaapiSetting);
    if (!setting)
    {
      CLog::Log(LOGERROR, "Failed to load setting for: {}", vaapiSetting);
      continue;
    }

    setting->SetVisible(true);
  }
}

void CDecoder::AVBufferRefDeleter::operator()(AVBufferRef* p) const
{
  av_buffer_unref(&p);
}

//-----------------------------------------------------------------------------
// Buffer Pool
//-----------------------------------------------------------------------------

/**
 * Buffer pool holds allocated vaapi and gl resources
 * Embedded in COutput
 */
class VAAPI::CVaapiBufferPool : public IVideoBufferPool
{
public:
  explicit CVaapiBufferPool(CDecoder &decoder);
  ~CVaapiBufferPool() override;
  CVideoBuffer* Get() override;
  void Return(int id) override;
  CVaapiRenderPicture* GetVaapi();
  bool HasFree();
  void QueueReturnPicture(CVaapiRenderPicture *pic);
  CVaapiRenderPicture* ProcessSyncPicture();
  void Init();
  void DeleteTextures(bool precleanup);

  std::deque<CVaapiProcessedPicture> processedPics;
  std::deque<CVaapiProcessedPicture> processedPicsAway;
  std::deque<CVaapiDecodedPicture> decodedPics;
  int procPicId;

protected:
  std::vector<CVaapiRenderPicture*> allRenderPics;
  std::deque<int> usedRenderPics;
  std::deque<int> freeRenderPics;
  std::deque<int> syncRenderPics;

  CDecoder &m_vaapi;
};

CVaapiBufferPool::CVaapiBufferPool(CDecoder &decoder)
  : m_vaapi(decoder)
{
  CVaapiRenderPicture *pic;
  for (unsigned int i = 0; i < NUM_RENDER_PICS; i++)
  {
    pic = new CVaapiRenderPicture(i);
    allRenderPics.push_back(pic);
    freeRenderPics.push_back(i);
  }
}

CVaapiBufferPool::~CVaapiBufferPool()
{
  CVaapiRenderPicture *pic;
  for (unsigned int i = 0; i < NUM_RENDER_PICS; i++)
  {
    pic = allRenderPics[i];
    delete pic;
  }
  allRenderPics.clear();
}

CVideoBuffer* CVaapiBufferPool::Get()
{
  if (freeRenderPics.empty())
    return nullptr;

  int idx = freeRenderPics.front();
  freeRenderPics.pop_front();
  usedRenderPics.push_back(idx);

  CVideoBuffer *retPic = allRenderPics[idx];
  retPic->Acquire(GetPtr());

  m_vaapi.Acquire();

  return retPic;
}

void CVaapiBufferPool::Return(int id)
{
  CVaapiRenderPicture *pic = allRenderPics[id];

  m_vaapi.ReturnRenderPicture(pic);
  m_vaapi.ReleasePicReference();
}

CVaapiRenderPicture* CVaapiBufferPool::GetVaapi()
{
  return dynamic_cast<CVaapiRenderPicture*>(Get());
}

bool CVaapiBufferPool::HasFree()
{
  return !freeRenderPics.empty();
}

void CVaapiBufferPool::QueueReturnPicture(CVaapiRenderPicture *pic)
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
    CLog::Log(LOGWARNING, "CVaapiRenderPicture::QueueReturnPicture - pic not found");
    return;
  }

  // check if already queued
  auto it2 = find(syncRenderPics.begin(), syncRenderPics.end(), *it);
  if (it2 == syncRenderPics.end())
  {
    syncRenderPics.push_back(*it);
  }
}

CVaapiRenderPicture* CVaapiBufferPool::ProcessSyncPicture()
{
  CVaapiRenderPicture *retPic = nullptr;

  for (auto it = syncRenderPics.begin(); it != syncRenderPics.end(); ++it)
  {
    retPic = allRenderPics[*it];

    freeRenderPics.push_back(*it);

    auto it2 = find(usedRenderPics.begin(), usedRenderPics.end(),*it);
    if (it2 == usedRenderPics.end())
    {
      CLog::Log(LOGERROR, "CVaapiRenderPicture::ProcessSyncPicture - pic not found in queue");
    }
    else
    {
      usedRenderPics.erase(it2);
    }
    it = syncRenderPics.erase(it);

    if (!retPic->valid)
    {
      CLog::Log(LOGDEBUG, LOGVIDEO, "CVaapiRenderPicture::{} - return of invalid render pic",
                __FUNCTION__);
      retPic = nullptr;
    }
    break;
  }
  return retPic;
}

void CVaapiBufferPool::Init()
{
  for (auto &pic : allRenderPics)
  {
    pic->avFrame = av_frame_alloc();
    pic->valid = false;
  }
  procPicId = 0;
}

void CVaapiBufferPool::DeleteTextures(bool precleanup)
{
  for (auto &pic : allRenderPics)
  {
    if (precleanup && pic->valid)
      continue;

    av_frame_free(&pic->avFrame);
    pic->valid = false;
  }
}

void CVaapiRenderPicture::GetPlanes(uint8_t*(&planes)[YuvImage::MAX_PLANES])
{
  planes[0] = avFrame->data[0];
  planes[1] = avFrame->data[1];
  planes[2] = avFrame->data[2];
}

void CVaapiRenderPicture::GetStrides(int(&strides)[YuvImage::MAX_PLANES])
{
  strides[0] = avFrame->linesize[0];
  strides[1] = avFrame->linesize[1];
  strides[2] = avFrame->linesize[2];
}

//-----------------------------------------------------------------------------
// Output
//-----------------------------------------------------------------------------
COutput::COutput(CDecoder& decoder, CEvent* inMsgEvent)
  : CThread("Vaapi-Output"),
    m_controlPort("OutputControlPort", inMsgEvent, &m_outMsgEvent),
    m_dataPort("OutputDataPort", inMsgEvent, &m_outMsgEvent),
    m_vaapi(decoder),
    m_bufferPool(std::make_shared<CVaapiBufferPool>(decoder))
{
  m_inMsgEvent = inMsgEvent;
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
  std::unique_lock<CCriticalSection> lock(CServiceBroker::GetWinSystem()->GetGfxContext());
  m_bStop = true;
  m_outMsgEvent.Set();
  StopThread();
  m_controlPort.Purge();
  m_dataPort.Purge();
}

void COutput::OnStartup()
{
  CLog::Log(LOGINFO, "COutput::OnStartup: Output Thread created");
}

void COutput::OnExit()
{
  CLog::Log(LOGINFO, "COutput::OnExit: Output Thread terminated");
}

enum OUTPUT_STATES
{
  O_TOP = 0,                      // 0
  O_TOP_ERROR,                    // 1
  O_TOP_UNCONFIGURED,             // 2
  O_TOP_CONFIGURED,               // 3
  O_TOP_CONFIGURED_IDLE,          // 4
  O_TOP_CONFIGURED_WORK,          // 5
  O_TOP_CONFIGURED_STEP1,         // 6
  O_TOP_CONFIGURED_STEP2,         // 7
  O_TOP_CONFIGURED_OUTPUT,        // 8
};

int VAAPI_OUTPUT_parentStates[] = {
    -1,
    0, //TOP_ERROR
    0, //TOP_UNCONFIGURED
    0, //TOP_CONFIGURED
    3, //TOP_CONFIGURED_IDLE
    3, //TOP_CONFIGURED_WORK
    3, //TOP_CONFIGURED_STEP1
    3, //TOP_CONFIGURED_STEP2
    3, //TOP_CONFIGURED_OUTPUT
};

void COutput::StateMachine(int signal, Protocol *port, Message *msg)
{
  for (int state = m_state; ; state = VAAPI_OUTPUT_parentStates[state])
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
          CVaapiRenderPicture *pic;
          pic = *((CVaapiRenderPicture**)msg->data);
          QueueReturnPicture(pic);
          return;
        case COutputDataProtocol::RETURNPROCPIC:
          int id;
          id = *((int*)msg->data);
          ProcessReturnProcPicture(id);
          return;
        default:
          break;
        }
      }
      {
        std::string portName = port == NULL ? "timer" : port->portName;
        CLog::Log(LOGWARNING, "COutput::{} - signal: {} form port: {} not handled for state: {}",
                  __FUNCTION__, signal, portName, m_state);
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
          CVaapiConfig *data;
          data = reinterpret_cast<CVaapiConfig*>(msg->data);
          if (data)
          {
            m_config = *data;
          }
          Init();

          // set initial number of
          EnsureBufferPool();
          m_state = O_TOP_CONFIGURED_IDLE;
          msg->Reply(COutputControlProtocol::ACC);
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
          m_state = O_TOP_CONFIGURED_IDLE;
          return;
        case COutputControlProtocol::PRECLEANUP:
          Flush();
          ReleaseBufferPool(true);
          msg->Reply(COutputControlProtocol::ACC);
          m_state = O_TOP_UNCONFIGURED;
          m_extTimeout = 10s;
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
          CPayloadWrap<CVaapiDecodedPicture> *payload;
          payload = dynamic_cast<CPayloadWrap<CVaapiDecodedPicture>*>(msg->payloadObj.get());
          if (payload)
          {
            m_bufferPool->decodedPics.push_back(*(payload->GetPlayload()));
            m_extTimeout = 0ms;
          }
          return;
        case COutputDataProtocol::RETURNPIC:
          CVaapiRenderPicture *pic;
          pic = *((CVaapiRenderPicture**)msg->data);
          QueueReturnPicture(pic);
          m_controlPort.SendInMessage(COutputControlProtocol::STATS);
          m_extTimeout = 0ms;
          return;
        case COutputDataProtocol::RETURNPROCPIC:
          int id;
          id = *((int*)msg->data);
          ProcessReturnProcPicture(id);
          m_extTimeout = 0ms;
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
          ProcessSyncPicture();
          m_extTimeout = 100ms;
          if (HasWork())
          {
            m_state = O_TOP_CONFIGURED_WORK;
            m_extTimeout = 0ms;
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
          if (PreferPP())
          {
            m_currentPicture = m_bufferPool->decodedPics.front();
            m_bufferPool->decodedPics.pop_front();
            InitCycle();
            m_state = O_TOP_CONFIGURED_STEP1;
            m_extTimeout = 0ms;
            return;
          }
          else if (m_bufferPool->HasFree() &&
                   !m_bufferPool->processedPics.empty())
          {
            m_state = O_TOP_CONFIGURED_OUTPUT;
            m_extTimeout = 0ms;
            return;
          }
          else
            m_state = O_TOP_CONFIGURED_IDLE;
          m_extTimeout = 100ms;
          return;
        default:
          break;
        }
      }
      break;

    case O_TOP_CONFIGURED_STEP1:
      if (port == NULL) // timeout
      {
        switch (signal)
        {
        case COutputControlProtocol::TIMEOUT:
        {
          if (!m_pp->AddPicture(m_currentPicture))
          {
            m_state = O_TOP_ERROR;
            return;
          }
          CVaapiProcessedPicture outPic;
          if (m_pp->Filter(outPic))
          {
            m_config.stats->IncProcessed();
            m_bufferPool->processedPics.push_back(outPic);
            m_state = O_TOP_CONFIGURED_STEP2;
          }
          else
          {
            m_state = O_TOP_CONFIGURED_IDLE;
          }
          m_config.stats->DecDecoded();
          m_controlPort.SendInMessage(COutputControlProtocol::STATS);
          m_extTimeout = 0ms;
          return;
        }
        default:
          break;
        }
      }
      break;

    case O_TOP_CONFIGURED_STEP2:
      if (port == NULL) // timeout
      {
        switch (signal)
        {
        case COutputControlProtocol::TIMEOUT:
        {
          CVaapiProcessedPicture outPic;
          if (m_pp->Filter(outPic))
          {
            m_bufferPool->processedPics.push_back(outPic);
            m_config.stats->IncProcessed();
            m_extTimeout = 0ms;
            return;
          }
          m_state = O_TOP_CONFIGURED_IDLE;
          m_extTimeout = 0ms;
          return;
        }
        default:
          break;
        }
      }
      break;

    case O_TOP_CONFIGURED_OUTPUT:
      if (port == NULL) // timeout
      {
        switch (signal)
        {
        case COutputControlProtocol::TIMEOUT:
          if (!m_bufferPool->processedPics.empty())
          {
            CVaapiRenderPicture *outPic;
            CVaapiProcessedPicture procPic;
            procPic = m_bufferPool->processedPics.front();
            m_bufferPool->processedPics.pop_front();
            outPic = ProcessPicture(procPic);
            if (outPic)
            {
              m_config.stats->IncRender();
              m_dataPort.SendInMessage(COutputDataProtocol::PICTURE, &outPic, sizeof(outPic));
            }
            m_config.stats->DecProcessed();
          }
          m_state = O_TOP_CONFIGURED_IDLE;
          m_extTimeout = 0ms;
          return;
        default:
          break;
        }
      }
      break;

    default: // we are in no state, should not happen
      CLog::Log(LOGERROR, "COutput::{} - no valid state: {}", __FUNCTION__, m_state);
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
  m_extTimeout = 1s;
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
    else if (m_outMsgEvent.Wait(m_extTimeout))
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
  m_diMethods.numDiMethods = 0;

  m_pp = new CFFmpegPostproc();
  m_pp->PreInit(m_config, &m_diMethods);
  delete m_pp;

  m_pp = new CVppPostproc();
  m_pp->PreInit(m_config, &m_diMethods);
  delete m_pp;

  m_pp = nullptr;

  std::list<EINTERLACEMETHOD> deintMethods;
  deintMethods.assign(m_diMethods.diMethods, m_diMethods.diMethods + m_diMethods.numDiMethods);
  m_config.processInfo->UpdateDeinterlacingMethods(deintMethods);
  m_config.processInfo->SetDeinterlacingMethodDefault(EINTERLACEMETHOD::VS_INTERLACEMETHOD_VAAPI_BOB);

  m_seenInterlaced = false;

  return true;
}

bool COutput::Uninit()
{
  ProcessSyncPicture();
  ReleaseBufferPool();
  if (m_pp)
  {
    std::shared_ptr<CPostproc> pp(m_pp);
    m_discardedPostprocs.push_back(pp);
    m_pp->Discard(this, &COutput::ReadyForDisposal);
    m_pp = nullptr;
  }

  if (!m_discardedPostprocs.empty())
  {
    CLog::Log(LOGERROR, "VAAPI::COutput::Uninit - not all CPostprcs released");
  }
  return true;
}

void COutput::Flush()
{
  Message *msg;
  while (m_dataPort.ReceiveOutMessage(&msg))
  {
    if (msg->signal == COutputDataProtocol::NEWFRAME)
    {
      CPayloadWrap<CVaapiDecodedPicture> *payload;
      payload = dynamic_cast<CPayloadWrap<CVaapiDecodedPicture>*>(msg->payloadObj.get());
      if (payload)
      {
        CVaapiDecodedPicture pic = *(payload->GetPlayload());
        m_config.videoSurfaces->ClearRender(pic.videoSurface);
      }
    }
    else if (msg->signal == COutputDataProtocol::RETURNPIC)
    {
      CVaapiRenderPicture *pic;
      pic = *((CVaapiRenderPicture**)msg->data);
      QueueReturnPicture(pic);
    }
    msg->Release();
  }

  while (m_dataPort.ReceiveInMessage(&msg))
  {
    if (msg->signal == COutputDataProtocol::PICTURE)
    {
      CVaapiRenderPicture *pic;
      pic = *((CVaapiRenderPicture**)msg->data);
      pic->Release();
    }
    msg->Release();
  }

  for (unsigned int i = 0; i < m_bufferPool->decodedPics.size(); i++)
  {
    m_config.videoSurfaces->ClearRender(m_bufferPool->decodedPics[i].videoSurface);
  }
  m_bufferPool->decodedPics.clear();

  for (unsigned int i = 0; i < m_bufferPool->processedPics.size(); i++)
  {
    ReleaseProcessedPicture(m_bufferPool->processedPics[i]);
  }
  m_bufferPool->processedPics.clear();

  if (m_pp)
    m_pp->Flush();
}

bool COutput::HasWork()
{
  // send a pic to renderer
  if (m_bufferPool->HasFree() && !m_bufferPool->processedPics.empty())
    return true;

  bool ppWantsPic = true;
  if (m_pp)
    ppWantsPic = m_pp->WantsPic();

  if (!m_bufferPool->decodedPics.empty() && m_bufferPool->processedPics.size() < 4 && ppWantsPic)
    return true;

  return false;
}

bool COutput::PreferPP()
{
  if (!m_bufferPool->decodedPics.empty())
  {
    if (!m_pp)
      return true;

    if (!m_pp->WantsPic())
      return false;

    if (!m_pp->DoesSync() && m_bufferPool->processedPics.size() < 4)
      return true;

    if (!m_bufferPool->HasFree() || m_bufferPool->processedPics.empty())
      return true;
  }

  return false;
}

void COutput::InitCycle()
{
  uint64_t latency;
  int flags;
  m_config.stats->GetParams(latency, flags);

  m_config.stats->SetCanSkipDeint(false);

  EINTERLACEMETHOD method = m_config.processInfo->GetVideoSettings().m_InterlaceMethod;
  bool interlaced = m_currentPicture.DVDPic.iFlags & DVP_FLAG_INTERLACED;
  // Remember whether any interlaced frames were encountered already.
  // If this is the case, the deinterlace method will never automatically be switched to NONE again in
  // order to not change deint methods every few frames in PAFF streams.
  m_seenInterlaced = m_seenInterlaced || interlaced;

  if (!(flags & DVD_CODEC_CTRL_NO_POSTPROC) &&
      m_seenInterlaced &&
      method != VS_INTERLACEMETHOD_NONE)
  {
    if (!m_config.processInfo->Supports(method))
      method = VS_INTERLACEMETHOD_VAAPI_BOB;

    if (m_pp && !m_pp->UpdateDeintMethod(method))
    {
      CLog::Log(LOGDEBUG, LOGVIDEO, "VAAPI output: Current postproc does not want new deinterlace mode, removing");
      std::shared_ptr<CPostproc> pp(m_pp);
      m_discardedPostprocs.push_back(pp);
      m_pp->Discard(this, &COutput::ReadyForDisposal);
      m_pp = nullptr;
      m_config.processInfo->SetVideoDeintMethod("unknown");
    }
    if (!m_pp)
    {
      if (method == VS_INTERLACEMETHOD_DEINTERLACE ||
          method == VS_INTERLACEMETHOD_RENDER_BOB)
      {
        CLog::Log(LOGDEBUG, LOGVIDEO, "VAAPI output: Initializing ffmpeg postproc");
        m_pp = new CFFmpegPostproc();
        m_config.stats->SetVpp(false);
      }
      else
      {
        CLog::Log(LOGDEBUG, LOGVIDEO, "VAAPI output: Initializing vaapi postproc");
        m_pp = new CVppPostproc();
        m_config.stats->SetVpp(true);
      }
      if (m_pp->PreInit(m_config))
      {
        m_pp->Init(method);
      }
      else
      {
        CLog::Log(LOGERROR, "VAAPI output: Postproc preinit failed");
        delete m_pp;
        m_pp = nullptr;
      }
    }
  }
  // progressive
  else
  {
    method = VS_INTERLACEMETHOD_NONE;

    if (m_pp && !m_pp->UpdateDeintMethod(method))
    {
      CLog::Log(LOGDEBUG, LOGVIDEO, "VAAPI output: Current postproc does not want new deinterlace mode, removing");
      std::shared_ptr<CPostproc> pp(m_pp);
      m_discardedPostprocs.push_back(pp);
      m_pp->Discard(this, &COutput::ReadyForDisposal);
      m_pp = nullptr;
      m_config.processInfo->SetVideoDeintMethod("unknown");
    }
    if (!m_pp)
    {
      const bool preferVaapiRender = CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(SETTING_VIDEOPLAYER_PREFERVAAPIRENDER);
      // For 1080p/i or below, always use CVppPostproc even when not deinterlacing
      // Reason is: mesa cannot dynamically switch surfaces between use for VAAPI post-processing
      // and use for direct export, so we run into trouble if we or the user want to switch
      // deinterlacing on/off mid-stream.
      // See also: https://bugs.freedesktop.org/show_bug.cgi?id=105145
      const bool alwaysInsertVpp = m_config.driverIsMesa &&
                                   ((m_config.vidWidth * m_config.vidHeight) <= (1920 * 1080)) &&
                                   interlaced;

      m_config.stats->SetVpp(false);
      if (!preferVaapiRender)
      {
        CLog::Log(LOGDEBUG, LOGVIDEO, "VAAPI output: Initializing ffmpeg postproc");
        m_pp = new CFFmpegPostproc();
      }
      else if (alwaysInsertVpp)
      {
        CLog::Log(LOGDEBUG, LOGVIDEO, "VAAPI output: Initializing vaapi postproc");
        m_pp = new CVppPostproc();
        m_config.stats->SetVpp(true);
      }
      else
      {
        CLog::Log(LOGDEBUG, LOGVIDEO, "VAAPI output: Initializing skip postproc");
        m_pp = new CSkipPostproc();
      }
      if (m_pp->PreInit(m_config))
      {
        m_pp->Init(method);
      }
      else
      {
        CLog::Log(LOGERROR, "VAAPI output: Postproc preinit failed");
        delete m_pp;
        m_pp = nullptr;
      }
    }
  }
  if (!m_pp) // fallback
  {
    CLog::Log(LOGDEBUG, LOGVIDEO, "VAAPI output: Initializing skip postproc as fallback");
    m_pp = new CSkipPostproc();
    m_config.stats->SetVpp(false);
    if (m_pp->PreInit(m_config))
      m_pp->Init(method);
  }
}

CVaapiRenderPicture* COutput::ProcessPicture(CVaapiProcessedPicture &pic)
{
  CVaapiRenderPicture *retPic;
  retPic = m_bufferPool->GetVaapi();
  retPic->DVDPic.SetParams(pic.DVDPic);

  if (!pic.source)
  {
    CLog::Log(LOGERROR, "VAAPI::ProcessPicture - pic has no source");
    retPic->Release();
    return nullptr;
  }

  if (pic.source->UseVideoSurface())
  {
    vaSyncSurface(m_config.dpy, pic.videoSurface);
    pic.id = m_bufferPool->procPicId++;
    m_bufferPool->processedPicsAway.push_back(pic);
    retPic->procPic = pic;
    retPic->vadsp = m_config.dpy;
  }
  else
  {
    av_frame_move_ref(retPic->avFrame, pic.frame);
    pic.source->ClearRef(pic);
    retPic->procPic.videoSurface = VA_INVALID_ID;
  }

  retPic->DVDPic.dts = DVD_NOPTS_VALUE;
  retPic->DVDPic.iWidth = m_config.vidWidth;
  retPic->DVDPic.iHeight = m_config.vidHeight;

  retPic->valid = true;

  return retPic;
}

void COutput::ReleaseProcessedPicture(CVaapiProcessedPicture &pic)
{
  if (!pic.source)
  {
    return;
  }
  pic.source->ClearRef(pic);
  pic.source = nullptr;
}

void COutput::QueueReturnPicture(CVaapiRenderPicture *pic)
{
  m_bufferPool->QueueReturnPicture(pic);
  ProcessSyncPicture();
}

void COutput::ProcessSyncPicture()
{
  CVaapiRenderPicture *pic;

  pic = m_bufferPool->ProcessSyncPicture();

  if (pic)
  {
    ProcessReturnPicture(pic);
  }
}

void COutput::ProcessReturnPicture(CVaapiRenderPicture *pic)
{
  if (pic->avFrame)
    av_frame_unref(pic->avFrame);

  ProcessReturnProcPicture(pic->procPic.id);
  pic->valid = false;
}

void COutput::ProcessReturnProcPicture(int id)
{
  for (auto it=m_bufferPool->processedPicsAway.begin(); it!=m_bufferPool->processedPicsAway.end(); ++it)
  {
    if (it->id == id)
    {
      ReleaseProcessedPicture(*it);
      m_bufferPool->processedPicsAway.erase(it);
      break;
    }
  }
}

void COutput::EnsureBufferPool()
{
  m_bufferPool->Init();
}

void COutput::ReleaseBufferPool(bool precleanup)
{
  ProcessSyncPicture();

  m_bufferPool->DeleteTextures(precleanup);

  for (unsigned int i = 0; i < m_bufferPool->decodedPics.size(); i++)
  {
    m_config.videoSurfaces->ClearRender(m_bufferPool->decodedPics[i].videoSurface);
  }
  m_bufferPool->decodedPics.clear();

  for (unsigned int i = 0; i < m_bufferPool->processedPics.size(); i++)
  {
    ReleaseProcessedPicture(m_bufferPool->processedPics[i]);
  }
  m_bufferPool->processedPics.clear();
}

void COutput::ReadyForDisposal(CPostproc *pp)
{
  for (auto it = m_discardedPostprocs.begin(); it != m_discardedPostprocs.end(); ++it)
  {
    if ((*it).get() == pp)
    {
      m_discardedPostprocs.erase(it);
      break;
    }
  }
}

//-----------------------------------------------------------------------------
// Postprocessing
//-----------------------------------------------------------------------------

bool CSkipPostproc::PreInit(CVaapiConfig &config, SDiMethods *methods)
{
  m_config = config;
  return true;
}

bool CSkipPostproc::Init(EINTERLACEMETHOD method)
{
  m_config.processInfo->SetVideoDeintMethod("none");
  return true;
}

bool CSkipPostproc::AddPicture(CVaapiDecodedPicture &inPic)
{
  m_pic = inPic;
  m_step = 0;
  return true;
}

bool CSkipPostproc::Filter(CVaapiProcessedPicture &outPic)
{
  if (m_step > 0)
    return false;
  outPic.DVDPic.SetParams(m_pic.DVDPic);
  outPic.videoSurface = m_pic.videoSurface;
  m_refsToSurfaces++;
  outPic.source = this;
  outPic.DVDPic.iFlags &= ~(DVP_FLAG_TOP_FIELD_FIRST |
                            DVP_FLAG_REPEAT_TOP_FIELD |
                            DVP_FLAG_INTERLACED);
  m_step++;
  return true;
}

void CSkipPostproc::ClearRef(CVaapiProcessedPicture &pic)
{
  m_config.videoSurfaces->ClearRender(pic.videoSurface);
  m_refsToSurfaces--;

  if (m_pOut && m_refsToSurfaces <= 0)
    (m_pOut->*m_cbDispose)(this);
}

void CSkipPostproc::Flush()
{

}

bool CSkipPostproc::UpdateDeintMethod(EINTERLACEMETHOD method)
{
  if (method == VS_INTERLACEMETHOD_NONE)
    return true;

  return false;
}

bool CSkipPostproc::DoesSync()
{
  return false;
}

bool CSkipPostproc::UseVideoSurface()
{
  return true;
}

void CSkipPostproc::Discard(COutput *output, ReadyToDispose cb)
{
  m_pOut = output;
  m_cbDispose = cb;
  if (m_refsToSurfaces <= 0)
    (m_pOut->*m_cbDispose)(this);
}

//-----------------------------------------------------------------------------
// VPP Postprocessing
//-----------------------------------------------------------------------------

CVppPostproc::CVppPostproc()
{
}

CVppPostproc::~CVppPostproc()
{
  Dispose();
}

bool CVppPostproc::PreInit(CVaapiConfig &config, SDiMethods *methods)
{
  m_config = config;

  // create config
  if (!CheckSuccess(
      vaCreateConfig(m_config.dpy, VAProfileNone, VAEntrypointVideoProc, NULL, 0, &m_configId),
      "vaCreateConfig"))
  {
    CLog::Log(LOGDEBUG, LOGVIDEO, "CVppPostproc::PreInit  - VPP init failed in vaCreateConfig");

    return false;
  }

  VASurfaceAttrib attribs[1], *attrib;
  attrib = attribs;
  attrib->flags = VA_SURFACE_ATTRIB_SETTABLE;
  attrib->type = VASurfaceAttribPixelFormat;
  attrib->value.type = VAGenericValueTypeInteger;
  attrib->value.value.i = VA_FOURCC_NV12;

  // create surfaces
  VASurfaceID surfaces[32];
  unsigned int format = VA_RT_FORMAT_YUV420;
  if (m_config.profile == VAProfileHEVCMain10)
  {
    format = VA_RT_FORMAT_YUV420_10BPP;
    attrib->value.value.i = VA_FOURCC_P010;
  }
  int nb_surfaces = NUM_RENDER_PICS;
  if (!CheckSuccess(
      vaCreateSurfaces(m_config.dpy, format, m_config.surfaceWidth, m_config.surfaceHeight,
          surfaces, nb_surfaces,
          attribs, 1), "vaCreateSurfaces"))
  {
    CLog::Log(LOGDEBUG, LOGVIDEO, "CVppPostproc::PreInit  - VPP init failed in vaCreateSurfaces");

    return false;
  }
  for (int i=0; i<nb_surfaces; i++)
  {
    m_videoSurfaces.AddSurface(surfaces[i]);
  }

  // create vaapi decoder context
  if (!CheckSuccess(
      vaCreateContext(m_config.dpy, m_configId, m_config.surfaceWidth, m_config.surfaceHeight, 0,
          surfaces,
          nb_surfaces, &m_contextId), "vaCreateContext"))
  {
    m_contextId = VA_INVALID_ID;
    CLog::Log(LOGDEBUG, LOGVIDEO, "CVppPostproc::PreInit  - VPP init failed in vaCreateContext");

    return false;
  }

  VAProcFilterType filters[VAProcFilterCount];
  unsigned int numFilters = VAProcFilterCount;
  VAProcFilterCapDeinterlacing deinterlacingCaps[VAProcDeinterlacingCount];
  unsigned int numDeinterlacingCaps = VAProcDeinterlacingCount;

  if (!CheckSuccess(vaQueryVideoProcFilters(m_config.dpy, m_contextId, filters, &numFilters),
      "vaQueryVideoProcFilters"))
  {
    CLog::Log(LOGDEBUG, LOGVIDEO, "CVppPostproc::PreInit  - VPP init failed in vaQueryVideoProcFilters");

    return false;
  }

  if (!CheckSuccess(vaQueryVideoProcFilterCaps(m_config.dpy, m_contextId, VAProcFilterDeinterlacing,
      deinterlacingCaps,
      &numDeinterlacingCaps), "vaQueryVideoProcFilterCaps"))
  {
    CLog::Log(LOGDEBUG, LOGVIDEO, "CVppPostproc::PreInit  - VPP init failed in vaQueryVideoProcFilterCaps");

    return false;
  }

  if (methods)
  {
    for (unsigned int i = 0; i < numFilters; i++)
    {
      if (filters[i] == VAProcFilterDeinterlacing)
      {
        for (unsigned int j = 0; j < numDeinterlacingCaps; j++)
        {
          if (deinterlacingCaps[j].type == VAProcDeinterlacingBob)
          {
            methods->diMethods[methods->numDiMethods++] = VS_INTERLACEMETHOD_VAAPI_BOB;
          }
          else if (deinterlacingCaps[j].type == VAProcDeinterlacingMotionAdaptive)
          {
            methods->diMethods[methods->numDiMethods++] = VS_INTERLACEMETHOD_VAAPI_MADI;
          }
          else if (deinterlacingCaps[j].type == VAProcDeinterlacingMotionCompensated)
          {
            methods->diMethods[methods->numDiMethods++] = VS_INTERLACEMETHOD_VAAPI_MACI;
          }
        }
      }
    }
  }
  return true;
}

bool CVppPostproc::Init(EINTERLACEMETHOD method)
{
  m_forwardRefs = 0;
  m_backwardRefs = 0;
  m_currentIdx = 0;
  m_frameCount = 0;
  m_vppMethod = VS_INTERLACEMETHOD_AUTO;

  return UpdateDeintMethod(method);
}


bool CVppPostproc::UpdateDeintMethod(EINTERLACEMETHOD method)
{
  if (method == m_vppMethod)
  {
    return true;
  }

  m_vppMethod = method;
  m_forwardRefs = 0;
  m_backwardRefs = 0;

  if (m_filter != VA_INVALID_ID)
  {
    CheckSuccess(vaDestroyBuffer(m_config.dpy, m_filter), "vaDestroyBuffer");
    m_filter = VA_INVALID_ID;
  }

  VAProcDeinterlacingType vppMethod;
  switch (method)
  {
  case VS_INTERLACEMETHOD_VAAPI_BOB:
    vppMethod = VAProcDeinterlacingBob;
    m_config.processInfo->SetVideoDeintMethod("vaapi-bob");
    break;
  case VS_INTERLACEMETHOD_VAAPI_MADI:
    vppMethod = VAProcDeinterlacingMotionAdaptive;
    m_config.processInfo->SetVideoDeintMethod("vaapi-madi");
    break;
  case VS_INTERLACEMETHOD_VAAPI_MACI:
    vppMethod = VAProcDeinterlacingMotionCompensated;
    m_config.processInfo->SetVideoDeintMethod("vaapi-mcdi");
    break;
  case VS_INTERLACEMETHOD_NONE:
    // Early exit, filter parameter buffer not needed then
    m_config.processInfo->SetVideoDeintMethod("vaapi-none");
    return true;
  default:
    m_config.processInfo->SetVideoDeintMethod("unknown");
    return false;
  }

  VAProcFilterParameterBufferDeinterlacing filterparams;
  filterparams.type = VAProcFilterDeinterlacing;
  filterparams.algorithm = vppMethod;
  filterparams.flags = 0;

  if (!CheckSuccess(vaCreateBuffer(m_config.dpy, m_contextId, VAProcFilterParameterBufferType,
      sizeof(filterparams), 1,
      &filterparams, &m_filter), "vaCreateBuffer"))
  {
    m_filter = VA_INVALID_ID;
    return false;
  }

  VAProcPipelineCaps pplCaps;
  if (!CheckSuccess(vaQueryVideoProcPipelineCaps(m_config.dpy, m_contextId, &m_filter, 1, &pplCaps),
      "vaQueryVideoProcPipelineCaps"))
  {
    return false;
  }

  m_forwardRefs = pplCaps.num_forward_references;
  m_backwardRefs = pplCaps.num_backward_references;

  return true;
}

void CVppPostproc::Dispose()
{
  // make sure surfaces are idle
  for (int i=0; i<m_videoSurfaces.Size(); i++)
  {
    CheckSuccess(vaSyncSurface(m_config.dpy, m_videoSurfaces.GetAtIndex(i)), "vaSyncSurface");
  }

  if (m_filter != VA_INVALID_ID)
  {
    CheckSuccess(vaDestroyBuffer(m_config.dpy, m_filter), "vaDestroyBuffer");
    m_filter = VA_INVALID_ID;
  }
  if (m_contextId != VA_INVALID_ID)
  {
    CheckSuccess(vaDestroyContext(m_config.dpy, m_contextId), "vaDestroyContext");
    m_contextId = VA_INVALID_ID;
  }
  VASurfaceID surf;
  while((surf = m_videoSurfaces.RemoveNext()) != VA_INVALID_SURFACE)
  {
    CheckSuccess(vaDestroySurfaces(m_config.dpy, &surf, 1), "vaDestroySurface");
  }
  m_videoSurfaces.Reset();

  if (m_configId != VA_INVALID_ID)
  {
    CheckSuccess(vaDestroyConfig(m_config.dpy, m_configId), "vaDestroyConfig");
    m_configId = VA_INVALID_ID;
  }

  // release all decoded pictures
  Flush();
}

bool CVppPostproc::AddPicture(CVaapiDecodedPicture &pic)
{
  pic.index = m_frameCount;
  m_decodedPics.push_front(pic);
  m_frameCount++;
  m_step = 0;
  m_config.stats->SetCanSkipDeint(true);
  return true;
}

bool CVppPostproc::Filter(CVaapiProcessedPicture &outPic)
{
  if (m_step>1)
  {
    Advance();
    return false;
  }

  // we need a free render target
  VASurfaceID surf = m_videoSurfaces.GetFree(VA_INVALID_SURFACE);
  if (surf == VA_INVALID_SURFACE)
  {
    CLog::Log(LOGERROR, "VAAPI - VPP - no free render target");
    return false;
  }
  // clear reference in case we return false
  m_videoSurfaces.ClearReference(surf);

  // move window of frames we are looking at to account for backward (=future) refs
  const auto currentIdx = m_currentIdx - m_backwardRefs;

  // make sure we have all needed forward refs
  if ((currentIdx - m_forwardRefs) < m_decodedPics.back().index)
  {
    Advance();
    return false;
  }

  auto it = std::find_if(m_decodedPics.begin(), m_decodedPics.end(),
                         [currentIdx](const CVaapiDecodedPicture &picture){
                           return picture.index == currentIdx;
                         });
  if (it==m_decodedPics.end())
  {
    return false;
  }
  outPic.DVDPic.SetParams(it->DVDPic);

  // skip deinterlacing cycle if requested
  if ((m_step == 1) &&
      ((outPic.DVDPic.iFlags & DVD_CODEC_CTRL_SKIPDEINT) || !(outPic.DVDPic.iFlags & DVP_FLAG_INTERLACED) || (m_vppMethod == VS_INTERLACEMETHOD_NONE)))
  {
    Advance();
    return false;
  }

  // vpp deinterlacing
  VAProcFilterParameterBufferDeinterlacing *filterParams;
  VABufferID pipelineBuf;
  VAProcPipelineParameterBuffer *pipelineParams;
  VARectangle inputRegion;
  VARectangle outputRegion;

  if (!CheckSuccess(vaBeginPicture(m_config.dpy, m_contextId, surf), "vaBeginPicture"))
  {
    return false;
  }

  if (!CheckSuccess(vaCreateBuffer(m_config.dpy, m_contextId, VAProcPipelineParameterBufferType,
          sizeof(VAProcPipelineParameterBuffer), 1, NULL, &pipelineBuf), "vaCreateBuffer"))
  {
    return false;
  }
  if (!CheckSuccess(vaMapBuffer(m_config.dpy, pipelineBuf, (void**) &pipelineParams),
      "vaMapBuffer"))
  {
    return false;
  }
  memset(pipelineParams, 0, sizeof(VAProcPipelineParameterBuffer));

  inputRegion.x = outputRegion.x = 0;
  inputRegion.y = outputRegion.y = 0;
  inputRegion.width = outputRegion.width = m_config.surfaceWidth;
  inputRegion.height = outputRegion.height = m_config.surfaceHeight;

  pipelineParams->output_region = &outputRegion;
  pipelineParams->surface_region = &inputRegion;
  pipelineParams->output_background_color = 0xff000000;
  pipelineParams->filter_flags = 0;

  VASurfaceID forwardRefs[32];
  VASurfaceID backwardRefs[32];
  pipelineParams->forward_references = forwardRefs;
  pipelineParams->backward_references = backwardRefs;
  pipelineParams->num_forward_references = 0;
  pipelineParams->num_backward_references = 0;

  int maxPic = currentIdx + m_backwardRefs;
  int minPic = currentIdx - m_forwardRefs;
  int curPic = currentIdx;

  // deinterlace flag
  if (m_vppMethod != VS_INTERLACEMETHOD_NONE)
  {
    unsigned int flags = 0;

    if (it->DVDPic.iFlags & DVP_FLAG_INTERLACED)
    {
      if (it->DVDPic.iFlags & DVP_FLAG_TOP_FIELD_FIRST)
        flags = 0;
      else
        flags = VA_DEINTERLACING_BOTTOM_FIELD_FIRST | VA_DEINTERLACING_BOTTOM_FIELD;

      if (m_step)
      {
        if (flags & VA_DEINTERLACING_BOTTOM_FIELD)
          flags &= ~VA_DEINTERLACING_BOTTOM_FIELD;
        else
          flags |= VA_DEINTERLACING_BOTTOM_FIELD;
      }
    }
    if (!CheckSuccess(vaMapBuffer(m_config.dpy, m_filter, (void**) &filterParams), "vaMapBuffer"))
    {
      return false;
    }
    filterParams->flags = flags;
    if (!CheckSuccess(vaUnmapBuffer(m_config.dpy, m_filter), "vaUnmapBuffer"))
    {
      return false;
    }

    pipelineParams->filters = &m_filter;
    pipelineParams->num_filters = 1;
  }
  else
  {
    pipelineParams->num_filters = 0;
  }

  // references
  double ptsLast = DVD_NOPTS_VALUE;
  double pts = DVD_NOPTS_VALUE;

  pipelineParams->surface = VA_INVALID_SURFACE;
  for (const auto &picture : m_decodedPics)
  {
    if (picture.index >= minPic && picture.index <= maxPic)
    {
      if (picture.index > curPic)
      {
        backwardRefs[(picture.index - curPic) - 1] = picture.videoSurface;
        pipelineParams->num_backward_references++;
      }
      else if (picture.index == curPic)
      {
        pipelineParams->surface = picture.videoSurface;
        pts = picture.DVDPic.pts;
      }
      if (picture.index < curPic)
      {
        forwardRefs[(curPic - picture.index) - 1] = picture.videoSurface;
        pipelineParams->num_forward_references++;
        if (picture.index == curPic - 1)
          ptsLast = picture.DVDPic.pts;
      }
    }
  }

  // set pts for 2nd frame
  if (m_step && pts != DVD_NOPTS_VALUE && ptsLast != DVD_NOPTS_VALUE)
    outPic.DVDPic.pts += (pts-ptsLast)/2;

  if (pipelineParams->surface == VA_INVALID_SURFACE)
    return false;

  if (!CheckSuccess(vaUnmapBuffer(m_config.dpy, pipelineBuf), "vaUnmmapBuffer"))
  {
    return false;
  }

  if (!CheckSuccess(vaRenderPicture(m_config.dpy, m_contextId, &pipelineBuf, 1), "vaRenderPicture"))
  {
    return false;
  }

  if (!CheckSuccess(vaEndPicture(m_config.dpy, m_contextId), "vaEndPicture"))
  {
    return false;
  }

  if (!CheckSuccess(vaDestroyBuffer(m_config.dpy, pipelineBuf), "vaDestroyBuffer"))
  {
    return false;
  }

  m_step++;
  outPic.videoSurface = m_videoSurfaces.GetFree(surf);
  outPic.source = this;
  outPic.DVDPic.iFlags &= ~(DVP_FLAG_TOP_FIELD_FIRST |
                            DVP_FLAG_REPEAT_TOP_FIELD |
                            DVP_FLAG_INTERLACED);

  return true;
}

void CVppPostproc::Advance()
{
  m_currentIdx++;

  // release all unneeded refs
  auto it = m_decodedPics.begin();
  while (it != m_decodedPics.end())
  {
    if (it->index < m_currentIdx - m_forwardRefs - m_backwardRefs)
    {
      m_config.videoSurfaces->ClearRender(it->videoSurface);
      it = m_decodedPics.erase(it);
    }
    else
      ++it;
  }
}

void CVppPostproc::ClearRef(CVaapiProcessedPicture &pic)
{
  m_videoSurfaces.ClearReference(pic.videoSurface);

  if (m_pOut && !m_videoSurfaces.HasRefs())
    (m_pOut->*m_cbDispose)(this);
}

void CVppPostproc::Flush()
{
  // release all decoded pictures
  auto it = m_decodedPics.begin();
  while (it != m_decodedPics.end())
  {
    m_config.videoSurfaces->ClearRender(it->videoSurface);
    it = m_decodedPics.erase(it);
  }
}

bool CVppPostproc::DoesSync()
{
  return false;
}

bool CVppPostproc::WantsPic()
{
  // need at least 2 for deinterlacing
  if (m_videoSurfaces.NumFree() > 1)
    return true;

  return false;
}

bool CVppPostproc::UseVideoSurface()
{
  return true;
}

void CVppPostproc::Discard(COutput *output, ReadyToDispose cb)
{
  m_pOut = output;
  m_cbDispose = cb;
  if (!m_videoSurfaces.HasRefs())
    (m_pOut->*m_cbDispose)(this);
}

bool CVppPostproc::CheckSuccess(VAStatus status, const std::string& function)
{
  if (status != VA_STATUS_SUCCESS)
  {
    CLog::Log(LOGERROR, "VAAPI/vpp {} error: {} ({})", function, vaErrorStr(status), status);
    return false;
  }
  return true;
}

//-----------------------------------------------------------------------------
// FFmpeg Postprocessing
//-----------------------------------------------------------------------------

#define CACHED_BUFFER_SIZE 4096

CFFmpegPostproc::CFFmpegPostproc()
{
  m_cache = NULL;
  m_pFilterFrameIn = NULL;
  m_pFilterFrameOut = NULL;
  m_pFilterGraph = NULL;
  m_DVDPic.pts = DVD_NOPTS_VALUE;
  m_frametime = 0;
  m_lastOutPts = DVD_NOPTS_VALUE;
}

CFFmpegPostproc::~CFFmpegPostproc()
{
  Close();
  KODI::MEMORY::AlignedFree(m_cache);
  m_dllSSE4.Unload();
  av_frame_free(&m_pFilterFrameIn);
  av_frame_free(&m_pFilterFrameOut);
}

bool CFFmpegPostproc::PreInit(CVaapiConfig &config, SDiMethods *methods)
{
  m_config = config;
  bool use_filter = true;

  // copying large surfaces via sse4 is a bit slow
  // we just return false here as the primary use case the
  // sse4 copy method is deinterlacing of max 1080i content
  if (m_config.vidWidth > 1920 || m_config.vidHeight > 1088)
    return false;

  VAImage image;
  image.image_id = VA_INVALID_ID;
  VASurfaceID surface = config.videoSurfaces->GetAtIndex(0);
  VAStatus status = vaDeriveImage(config.dpy, surface, &image);
  if (status != VA_STATUS_SUCCESS)
  {
    CLog::Log(LOGINFO, "VAAPI::SupportsFilter vaDeriveImage not supported by driver - ffmpeg postprocessing and CPU-copy rendering will not be available");
    use_filter = false;
  }
  if (use_filter && (image.format.fourcc != VA_FOURCC_NV12))
  {
    CLog::Log(LOGWARNING,"VAAPI::SupportsFilter image format not NV12");
    use_filter = false;
  }
  if (use_filter && ((image.pitches[0] % 64) || (image.pitches[1] % 64)))
  {
    CLog::Log(LOGWARNING,"VAAPI::SupportsFilter patches no multiple of 64");
    use_filter = false;
  }
  if (image.image_id != VA_INVALID_ID)
    CheckSuccess(vaDestroyImage(config.dpy, image.image_id), "vaDestroyImage");

  if (use_filter && !m_dllSSE4.Load())
  {
    CLog::Log(LOGERROR,"VAAPI::SupportsFilter failed loading sse4 lib");
    use_filter = false;
  }

  if (use_filter)
  {
    m_cache = static_cast<uint8_t*>(KODI::MEMORY::AlignedMalloc(CACHED_BUFFER_SIZE, 64));
    if (methods)
    {
      methods->diMethods[methods->numDiMethods++] = VS_INTERLACEMETHOD_DEINTERLACE;
      methods->diMethods[methods->numDiMethods++] = VS_INTERLACEMETHOD_RENDER_BOB;
    }
  }
  return use_filter;
}

bool CFFmpegPostproc::Init(EINTERLACEMETHOD method)
{
  if (!(m_pFilterGraph = avfilter_graph_alloc()))
  {
    CLog::Log(LOGERROR, "VAAPI::CFFmpegPostproc::Init - unable to alloc filter graph");
    return false;
  }

  const AVFilter* srcFilter = avfilter_get_by_name("buffer");
  const AVFilter* outFilter = avfilter_get_by_name("buffersink");

  std::string args =
      StringUtils::Format("video_size={}x{}:pix_fmt={}:time_base={}/{}:pixel_aspect={}/{}",
                          m_config.vidWidth, m_config.vidHeight, AV_PIX_FMT_NV12, 1, 1,
                          (m_config.aspect.num != 0) ? m_config.aspect.num : 1,
                          (m_config.aspect.num != 0) ? m_config.aspect.den : 1);

  if (avfilter_graph_create_filter(&m_pFilterIn, srcFilter, "src", args.c_str(), NULL, m_pFilterGraph) < 0)
  {
    CLog::Log(LOGERROR, "VAAPI::CFFmpegPostproc::Init - avfilter_graph_create_filter: src");
    return false;
  }

  if (avfilter_graph_create_filter(&m_pFilterOut, outFilter, "out", NULL, NULL, m_pFilterGraph) < 0)
  {
    CLog::Log(LOGERROR, "CFFmpegPostproc::Init  - avfilter_graph_create_filter: out");
    return false;
  }

  enum AVPixelFormat pix_fmts[] = { AV_PIX_FMT_NV12, AV_PIX_FMT_NONE };
  if (av_opt_set_int_list(m_pFilterOut, "pix_fmts", pix_fmts,  AV_PIX_FMT_NONE, AV_OPT_SEARCH_CHILDREN) < 0)
  {
    CLog::Log(LOGERROR, "VAAPI::CFFmpegPostproc::Init  - failed settings pix formats");
    return false;
  }

  AVFilterInOut* outputs = avfilter_inout_alloc();
  AVFilterInOut* inputs  = avfilter_inout_alloc();

  outputs->name    = av_strdup("in");
  outputs->filter_ctx = m_pFilterIn;
  outputs->pad_idx = 0;
  outputs->next    = NULL;

  inputs->name    = av_strdup("out");
  inputs->filter_ctx = m_pFilterOut;
  inputs->pad_idx = 0;
  inputs->next    = NULL;

  if (method == VS_INTERLACEMETHOD_DEINTERLACE)
  {
    std::string filter;

    filter = "bwdif=1:-1";

    if (avfilter_graph_parse_ptr(m_pFilterGraph, filter.c_str(), &inputs, &outputs, NULL) < 0)
    {
      CLog::Log(LOGERROR, "VAAPI::CFFmpegPostproc::Init  - avfilter_graph_parse");
      avfilter_inout_free(&outputs);
      avfilter_inout_free(&inputs);
      return false;
    }

    avfilter_inout_free(&outputs);
    avfilter_inout_free(&inputs);

    if (avfilter_graph_config(m_pFilterGraph, NULL) < 0)
    {
      CLog::Log(LOGERROR, "VAAPI::CFFmpegPostproc::Init  - avfilter_graph_config");
      return false;
    }

    m_config.processInfo->SetVideoDeintMethod("bwdif");
  }
  else if (method == VS_INTERLACEMETHOD_RENDER_BOB ||
           method == VS_INTERLACEMETHOD_NONE)
  {
    CLog::Log(LOGDEBUG, LOGVIDEO, "VAAPI::CFFmpegPostproc::Init  - skip deinterlacing");
    avfilter_inout_free(&outputs);
    avfilter_inout_free(&inputs);
    m_config.processInfo->SetVideoDeintMethod("none");
  }
  else
  {
    avfilter_inout_free(&outputs);
    avfilter_inout_free(&inputs);
    m_config.processInfo->SetVideoDeintMethod("unknown");
    return false;
  }
  m_diMethod = method;

  m_pFilterFrameIn = av_frame_alloc();
  m_pFilterFrameOut = av_frame_alloc();
  return true;
}

bool CFFmpegPostproc::AddPicture(CVaapiDecodedPicture &inPic)
{
  VASurfaceID surf = inPic.videoSurface;
  VAImage image;
  uint8_t *buf;
  if (m_DVDPic.pts != DVD_NOPTS_VALUE && inPic.DVDPic.pts != DVD_NOPTS_VALUE)
  {
    m_frametime = inPic.DVDPic.pts - m_DVDPic.pts;
  }
  m_DVDPic.SetParams(inPic.DVDPic);
  bool result = false;

  if (!CheckSuccess(vaSyncSurface(m_config.dpy, surf), "vaSyncSurface"))
    goto error;

  if (!CheckSuccess(vaDeriveImage(m_config.dpy, surf, &image), "vaDeriveImage"))
    goto error;

  if (!CheckSuccess(vaMapBuffer(m_config.dpy, image.buf, (void**) &buf), "vaMapBuffer"))
    goto error;

  m_pFilterFrameIn->format = AV_PIX_FMT_NV12;
  m_pFilterFrameIn->width = m_config.vidWidth;
  m_pFilterFrameIn->height = m_config.vidHeight;
  m_pFilterFrameIn->linesize[0] = image.pitches[0];
  m_pFilterFrameIn->linesize[1] = image.pitches[1];
  m_pFilterFrameIn->interlaced_frame = (inPic.DVDPic.iFlags & DVP_FLAG_INTERLACED) ? 1 : 0;
  m_pFilterFrameIn->top_field_first = (inPic.DVDPic.iFlags & DVP_FLAG_TOP_FIELD_FIRST) ? 1 : 0;

  if (inPic.DVDPic.pts == DVD_NOPTS_VALUE)
    m_pFilterFrameIn->pts = AV_NOPTS_VALUE;
  else
    m_pFilterFrameIn->pts = (inPic.DVDPic.pts / DVD_TIME_BASE) * AV_TIME_BASE;

  m_pFilterFrameIn->pkt_dts = m_pFilterFrameIn->pts;
  m_pFilterFrameIn->best_effort_timestamp = m_pFilterFrameIn->pts;

  av_frame_get_buffer(m_pFilterFrameIn, 64);

  uint8_t *src, *dst;
  src = buf + image.offsets[0];
  dst = m_pFilterFrameIn->data[0];
  m_dllSSE4.copy_frame(src, dst, m_cache, m_config.vidWidth, m_config.vidHeight, image.pitches[0]);

  src = buf + image.offsets[1];
  dst = m_pFilterFrameIn->data[1];
  m_dllSSE4.copy_frame(src, dst, m_cache, image.width, image.height/2, image.pitches[1]);

  m_pFilterFrameIn->linesize[0] = image.pitches[0];
  m_pFilterFrameIn->linesize[1] = image.pitches[1];
  m_pFilterFrameIn->data[2] = NULL;
  m_pFilterFrameIn->data[3] = NULL;
  m_pFilterFrameIn->pkt_size = image.data_size;

  CheckSuccess(vaUnmapBuffer(m_config.dpy, image.buf), "vaUnmapBuffer");
  CheckSuccess(vaDestroyImage(m_config.dpy, image.image_id), "vaDestroyImage");

  if (m_diMethod == VS_INTERLACEMETHOD_DEINTERLACE)
  {
    if (av_buffersrc_add_frame(m_pFilterIn, m_pFilterFrameIn) < 0)
    {
      CLog::Log(LOGERROR, "CFFmpegPostproc::AddPicture - av_buffersrc_add_frame");
      goto error;
    }
  }
  else if (m_diMethod == VS_INTERLACEMETHOD_RENDER_BOB ||
           m_diMethod == VS_INTERLACEMETHOD_NONE)
  {
    av_frame_move_ref(m_pFilterFrameOut, m_pFilterFrameIn);
    m_step = 0;
  }
  av_frame_unref(m_pFilterFrameIn);

  result = true;

error:
  m_config.videoSurfaces->ClearRender(surf);
  return result;
}

bool CFFmpegPostproc::Filter(CVaapiProcessedPicture &outPic)
{
  outPic.DVDPic.SetParams(m_DVDPic);
  if (m_diMethod == VS_INTERLACEMETHOD_DEINTERLACE)
  {
    int result;
    result = av_buffersink_get_frame(m_pFilterOut, m_pFilterFrameOut);

    if(result  == AVERROR(EAGAIN) || result == AVERROR_EOF)
      return false;
    else if(result < 0)
    {
      CLog::Log(LOGERROR, "CFFmpegPostproc::Filter - av_buffersink_get_frame");
      return false;
    }
    outPic.DVDPic.iFlags &= ~(DVP_FLAG_TOP_FIELD_FIRST |
                              DVP_FLAG_REPEAT_TOP_FIELD |
                              DVP_FLAG_INTERLACED);
  }
  else if (m_diMethod == VS_INTERLACEMETHOD_RENDER_BOB ||
           m_diMethod == VS_INTERLACEMETHOD_NONE)
  {
    if (m_step > 0)
      return false;
  }

  m_step++;
  outPic.frame = av_frame_clone(m_pFilterFrameOut);
  av_frame_unref(m_pFilterFrameOut);

  outPic.source = this;
  m_refsToPics++;

  int64_t bpts = outPic.frame->best_effort_timestamp;
  if(bpts != AV_NOPTS_VALUE)
  {
    outPic.DVDPic.pts = (double)bpts * DVD_TIME_BASE / AV_TIME_BASE;
  }
  else
    outPic.DVDPic.pts = DVD_NOPTS_VALUE;

  double pts = outPic.DVDPic.pts;
  if (m_lastOutPts != DVD_NOPTS_VALUE && m_lastOutPts == pts)
  {
    outPic.DVDPic.pts += m_frametime/2;
  }
  m_lastOutPts = pts;

  return true;
}

void CFFmpegPostproc::ClearRef(CVaapiProcessedPicture &pic)
{
  av_frame_free(&pic.frame);
  m_refsToPics--;

  if (m_pOut && m_refsToPics <= 0 && m_cbDispose)
    (m_pOut->*m_cbDispose)(this);
}

void CFFmpegPostproc::Close()
{
  if (m_pFilterGraph)
  {
    avfilter_graph_free(&m_pFilterGraph);
  }
}

void CFFmpegPostproc::Flush()
{
  Close();
  Init(m_diMethod);
  m_DVDPic.pts = DVD_NOPTS_VALUE;
  m_frametime = 0;
  m_lastOutPts = DVD_NOPTS_VALUE;
}

bool CFFmpegPostproc::UpdateDeintMethod(EINTERLACEMETHOD method)
{
  /// \todo switching between certain methods could be done without deinit/init
  return (m_diMethod == method);
}

bool CFFmpegPostproc::DoesSync()
{
  return true;
}

bool CFFmpegPostproc::UseVideoSurface()
{
  return false;
}

void CFFmpegPostproc::Discard(COutput *output, ReadyToDispose cb)
{
  m_pOut = output;
  m_cbDispose = cb;
  if (m_refsToPics <= 0)
    (m_pOut->*m_cbDispose)(this);
}

bool CFFmpegPostproc::CheckSuccess(VAStatus status, const std::string& function)
{
  if (status != VA_STATUS_SUCCESS)
  {
    CLog::Log(LOGERROR, "VAAPI/ffpp error: {} ({})", function, vaErrorStr(status), status);
    return false;
  }
  return true;
}
