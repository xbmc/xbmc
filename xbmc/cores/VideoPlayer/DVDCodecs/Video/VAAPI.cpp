/*
 *      Copyright (C) 2005-2014 Team XBMC
 *      http://xbmc.org
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
#include "system.h"
#include "VAAPI.h"
#include "ServiceBroker.h"
#include "windowing/WindowingFactory.h"
#include "DVDVideoCodec.h"
#include "cores/VideoPlayer/DVDCodecs/DVDCodecUtils.h"
#include "cores/VideoPlayer/DVDClock.h"
#include "cores/VideoPlayer/Process/ProcessInfo.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "threads/SingleLock.h"
#include "settings/Settings.h"
#include "guilib/GraphicContext.h"
#include "settings/MediaSettings.h"
#include "settings/AdvancedSettings.h"
#include <va/va_drm.h>
#include <va/va_drmcommon.h>
#include <drm_fourcc.h>
#include "linux/XTimeUtils.h"
#include "linux/XMemUtils.h"

extern "C" {
#include "libavutil/avutil.h"
#include "libavutil/opt.h"
#include "libavfilter/buffersink.h"
#include "libavfilter/buffersrc.h"
}

#ifdef HAVE_X11
#include <va/va_x11.h>
#endif

#include <va/va_vpp.h>
#include <xf86drm.h>

using namespace VAAPI;
#define NUM_RENDER_PICS 7

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

CVAAPIContext *CVAAPIContext::m_context = 0;
CCriticalSection CVAAPIContext::m_section;

#ifdef HAVE_X11
Display *CVAAPIContext::m_X11dpy = 0;
#endif

CVAAPIContext::CVAAPIContext()
{
  m_context = 0;
  m_refCount = 0;
  m_attributes = NULL;
  m_profiles = NULL;
  m_display = NULL;
}

void CVAAPIContext::Release(CDecoder *decoder)
{
  CSingleLock lock(m_section);

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
  CLog::Log(LOGNOTICE, "VAAPI::Close - closing decoder context");
  if (m_renderNodeFD >= 0)
  {
    close(m_renderNodeFD);
  }

  DestroyContext();
}

bool CVAAPIContext::EnsureContext(CVAAPIContext **ctx, CDecoder *decoder)
{
  CSingleLock lock(m_section);

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
    CSingleLock gLock(g_graphicsContext);
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
    snprintf(name, buf_size, "/dev/dri/renderD%u", i);

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
  }

  CLog::Log(LOGERROR, "Failed to find any open render nodes in /dev/dri/renderD<num>");
}

void CVAAPIContext::SetVaDisplayForSystem()
{
  auto win_system = g_Windowing.GetWinSystem();
  if (win_system == WINDOW_SYSTEM_X11)
  {
#if HAVE_X11
    { CSingleLock lock(g_graphicsContext);
      if (m_X11dpy == nullptr)
        m_X11dpy = XOpenDisplay(nullptr);
    }

    m_display = vaGetDisplay(m_X11dpy);
#endif
  }
  else
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

  int major_version, minor_version;
  if (!CheckSuccess(vaInitialize(m_display, &major_version, &minor_version)))
  {
    m_display = NULL;
    return false;
  }

  if (g_advancedSettings.CanLogComponent(LOGVIDEO))
    CLog::Log(LOGDEBUG, "VAAPI - initialize version %d.%d", major_version, minor_version);

  if (g_advancedSettings.CanLogComponent(LOGVIDEO))
    CLog::Log(LOGDEBUG, "VAAPI - driver in use: %s", vaQueryVendorString(m_display));

  QueryCaps();
  if (!m_profileCount)
    return false;

  if (!m_attributeCount)
    CLog::Log(LOGWARNING, "VAAPI - driver did not return anything from vlVaQueryDisplayAttributes");

  return true;
}

void CVAAPIContext::DestroyContext()
{
  delete[] m_attributes;
  delete[] m_profiles;
  if (m_display)
    CheckSuccess(vaTerminate(m_display));
}

void CVAAPIContext::QueryCaps()
{
  m_attributeCount = 0;
  m_profileCount = 0;

  int max_attributes = vaMaxNumDisplayAttributes(m_display);
  m_attributes = new VADisplayAttribute[max_attributes];

  if (!CheckSuccess(vaQueryDisplayAttributes(m_display, m_attributes, &m_attributeCount)))
    return;

  for(int i = 0; i < m_attributeCount; i++)
  {
    VADisplayAttribute * const display_attr = &m_attributes[i];
    if (g_advancedSettings.CanLogComponent(LOGVIDEO))
    {
      CLog::Log(LOGDEBUG, "VAAPI - attrib %d (%s/%s) min %d max %d value 0x%x\n"
                         , display_attr->type
                         ,(display_attr->flags & VA_DISPLAY_ATTRIB_GETTABLE) ? "get" : "---"
                         ,(display_attr->flags & VA_DISPLAY_ATTRIB_SETTABLE) ? "set" : "---"
                         , display_attr->min_value
                         , display_attr->max_value
                         , display_attr->value);
    }
  }

  int max_profiles = vaMaxNumProfiles(m_display);
  m_profiles = new VAProfile[max_profiles];

  if (!CheckSuccess(vaQueryConfigProfiles(m_display, m_profiles, &m_profileCount)))
    return;

  for(int i = 0; i < m_profileCount; i++)
  {
    if (g_advancedSettings.CanLogComponent(LOGVIDEO))
      CLog::Log(LOGDEBUG, "VAAPI - profile %d", m_profiles[i]);
  }
}

VAConfigAttrib CVAAPIContext::GetAttrib(VAProfile profile)
{
  CSingleLock lock(m_section);

  VAConfigAttrib attrib;
  attrib.type = VAConfigAttribRTFormat;
  CheckSuccess(vaGetConfigAttributes(m_display, profile, VAEntrypointVLD, &attrib, 1));

  return attrib;
}

bool CVAAPIContext::SupportsProfile(VAProfile profile)
{
  CSingleLock lock(m_section);

  for (int i=0; i<m_profileCount; i++)
  {
    if (m_profiles[i] == profile)
      return true;
  }
  return false;
}

VAConfigID CVAAPIContext::CreateConfig(VAProfile profile, VAConfigAttrib attrib)
{
  CSingleLock lock(m_section);

  VAConfigID config = VA_INVALID_ID;
  CheckSuccess(vaCreateConfig(m_display, profile, VAEntrypointVLD, &attrib, 1, &config));

  return config;
}

bool CVAAPIContext::CheckSuccess(VAStatus status)
{
  if (status != VA_STATUS_SUCCESS)
  {
    CLog::Log(LOGERROR, "VAAPI::%s error: %s", __FUNCTION__, vaErrorStr(status));
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
  CDecoder *va = (CDecoder*)opaque;
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
  CSingleLock lock(m_section);
  m_state[surf] = 0;
  m_freeSurfaces.push_back(surf);
}

void CVideoSurfaces::ClearReference(VASurfaceID surf)
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

bool CVideoSurfaces::MarkRender(VASurfaceID surf)
{
  CSingleLock lock(m_section);
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

bool CVideoSurfaces::IsValid(VASurfaceID surf)
{
  CSingleLock lock(m_section);
  if (m_state.find(surf) != m_state.end())
    return true;
  else
    return false;
}

VASurfaceID CVideoSurfaces::GetFree(VASurfaceID surf)
{
  CSingleLock lock(m_section);
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
  CSingleLock lock(m_section);
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
  CSingleLock lock(m_section);
  m_freeSurfaces.clear();
  m_state.clear();
}

int CVideoSurfaces::Size()
{
  CSingleLock lock(m_section);
  return m_state.size();
}

bool CVideoSurfaces::HasFree()
{
  CSingleLock lock(m_section);
  return !m_freeSurfaces.empty();
}

int CVideoSurfaces::NumFree()
{
  CSingleLock lock(m_section);
  return m_freeSurfaces.size();
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
// VAAPI
//-----------------------------------------------------------------------------

CDecoder::CDecoder(CProcessInfo& processInfo) :
  m_vaapiOutput(&m_inMsgEvent),
  m_processInfo(processInfo)
{
  m_vaapiConfig.videoSurfaces = &m_videoSurfaces;

  m_vaapiConfigured = false;
  m_DisplayState = VAAPI_OPEN;
  m_vaapiConfig.context = 0;
  m_vaapiConfig.contextId = VA_INVALID_ID;
  m_vaapiConfig.configId = VA_INVALID_ID;
  m_vaapiConfig.processInfo = &m_processInfo;
  m_avctx = NULL;
  m_getBufferError = 0;
}

CDecoder::~CDecoder()
{
  Close();
}

bool CDecoder::Open(AVCodecContext* avctx, AVCodecContext* mainctx, const enum AVPixelFormat fmt, unsigned int surfaces)
{
  // don't support broken wrappers by default
  // nvidia cards with a vaapi to vdpau wrapper
  // fglrx cards with xvba-va-driver
  std::string gpuvendor = g_Windowing.GetRenderVendor();
  std::transform(gpuvendor.begin(), gpuvendor.end(), gpuvendor.begin(), ::tolower);
  if (gpuvendor.compare(0, 5, "intel") != 0)
  {
    // user might force VAAPI enabled, cause he might know better
    if (g_advancedSettings.m_videoVAAPIforced)
    {
      CLog::Log(LOGWARNING, "VAAPI was not tested on your hardware / driver stack: %s. If it will crash and burn complain with your gpu vendor.", gpuvendor.c_str());
    }
    else
    {
      return false;
    }
  }

  // check if user wants to decode this format with VAAPI
  std::map<AVCodecID, std::string> settings_map = {
    { AV_CODEC_ID_H263, CSettings::SETTING_VIDEOPLAYER_USEVAAPIMPEG4 },
    { AV_CODEC_ID_MPEG4, CSettings::SETTING_VIDEOPLAYER_USEVAAPIMPEG4 },
    { AV_CODEC_ID_WMV3, CSettings::SETTING_VIDEOPLAYER_USEVAAPIVC1 },
    { AV_CODEC_ID_VC1, CSettings::SETTING_VIDEOPLAYER_USEVAAPIVC1 },
    { AV_CODEC_ID_MPEG2VIDEO, CSettings::SETTING_VIDEOPLAYER_USEVAAPIMPEG2 },
  };
  if (CDVDVideoCodec::IsCodecDisabled(settings_map, avctx->codec_id))
    return false;

  if (g_advancedSettings.CanLogComponent(LOGVIDEO))
    CLog::Log(LOGDEBUG,"VAAPI - open decoder");

  if (!CVAAPIContext::EnsureContext(&m_vaapiConfig.context, this))
    return false;

  if(avctx->coded_width  == 0 ||
     avctx->coded_height == 0)
  {
    CLog::Log(LOGWARNING,"VAAPI::Open: no width/height available, can't init");
    return false;
  }

  m_vaapiConfig.vidWidth = avctx->width;
  m_vaapiConfig.vidHeight = avctx->height;
  m_vaapiConfig.outWidth = avctx->width;
  m_vaapiConfig.outHeight = avctx->height;
  m_vaapiConfig.surfaceWidth = avctx->coded_width;
  m_vaapiConfig.surfaceHeight = avctx->coded_height;
  m_vaapiConfig.aspect = avctx->sample_aspect_ratio;
  m_decoderThread = CThread::GetCurrentThreadId();
  m_DisplayState = VAAPI_OPEN;
  m_vaapiConfigured = false;
  m_presentPicture = 0;
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
      if (avctx->profile == FF_PROFILE_H264_BASELINE)
      {
        profile = VAProfileH264Baseline;
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
        profile = VAProfileHEVCMain10;
      else if (avctx->profile == FF_PROFILE_HEVC_MAIN)
        profile = VAProfileHEVCMain;
      else
        profile = VAProfileNone;
      if (!m_vaapiConfig.context->SupportsProfile(profile))
        return false;
      break;
    }
#if VA_CHECK_VERSION(0,38,1)
    case AV_CODEC_ID_VP9:
    {
      profile = VAProfileVP9Profile0;
      if (!m_vaapiConfig.context->SupportsProfile(profile))
        return false;
      break;
    }
#endif
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
    default:
      return false;
  }

  m_vaapiConfig.profile = profile;
  m_vaapiConfig.attrib = m_vaapiConfig.context->GetAttrib(profile);
  if ((m_vaapiConfig.attrib.value & (VA_RT_FORMAT_YUV420 | VA_RT_FORMAT_YUV420_10BPP)) == 0)
  {
    CLog::Log(LOGERROR, "VAAPI - invalid yuv format %x", m_vaapiConfig.attrib.value);
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
  else
    m_vaapiConfig.maxReferences = 2;

  // add an extra surface for safety, some faulty material
  // make ffmpeg require more buffers
  m_vaapiConfig.maxReferences += surfaces + 1;

  if (!ConfigVAAPI())
  {
    return false;
  }

  avctx->hwaccel_context = &m_hwContext;
  avctx->get_buffer2 = CDecoder::FFGetBuffer;
  avctx->slice_flags = SLICE_FLAG_CODED_ORDER|SLICE_FLAG_ALLOW_FIELD;

  mainctx->hwaccel_context = &m_hwContext;
  mainctx->get_buffer2 = CDecoder::FFGetBuffer;
  mainctx->slice_flags = SLICE_FLAG_CODED_ORDER|SLICE_FLAG_ALLOW_FIELD;

  m_avctx = mainctx;
  return true;
}

void CDecoder::Close()
{
  CLog::Log(LOGNOTICE, "VAAPI::%s", __FUNCTION__);

  CSingleLock lock(m_DecoderSection);

  FiniVAAPIOutput();

  if (m_vaapiConfig.context)
    m_vaapiConfig.context->Release(this);
  m_vaapiConfig.context = 0;
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
  if (m_vaapiConfigured == true)
  {
    CSingleLock lock(m_DecoderSection);
    if (g_advancedSettings.CanLogComponent(LOGVIDEO))
      CLog::Log(LOGDEBUG,"VAAPI::Release pre-cleanup");

    CSingleLock lock1(g_graphicsContext);
    Message *reply;
    if (m_vaapiOutput.m_controlPort.SendOutMessageSync(COutputControlProtocol::PRECLEANUP,
                                                   &reply,
                                                   2000))
    {
      bool success = reply->signal == COutputControlProtocol::ACC ? true : false;
      reply->Release();
      if (!success)
      {
        CLog::Log(LOGERROR, "VAAPI::%s - pre-cleanup returned error", __FUNCTION__);
        m_DisplayState = VAAPI_ERROR;
      }
    }
    else
    {
      CLog::Log(LOGERROR, "VAAPI::%s - pre-cleanup timed out", __FUNCTION__);
      m_DisplayState = VAAPI_ERROR;
    }

    VASurfaceID surf;
    while((surf = m_videoSurfaces.RemoveNext(true)) != VA_INVALID_SURFACE)
    {
      CheckSuccess(vaDestroySurfaces(m_vaapiConfig.dpy, &surf, 1));
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
  CDVDVideoCodecFFmpeg* ctx = (CDVDVideoCodecFFmpeg*)avctx->opaque;
  CDecoder*             va  = (CDecoder*)ctx->GetHardware();

  // while we are waiting to recover we can't do anything
  CSingleLock lock(va->m_DecoderSection);

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
    CLog::Log(LOGWARNING, "VAAPI::FFGetBuffer - no surface available - dec: %d, render: %d",
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
    CLog::Log(LOGERROR, "VAAPI::%s - error creating buffer", __FUNCTION__);
    return -1;
  }
  pic->buf[0] = buffer;

  pic->reordered_opaque = avctx->reordered_opaque;
  va->Acquire();
  return 0;
}

void CDecoder::FFReleaseBuffer(uint8_t *data)
{
  {
    VASurfaceID surf;

    CSingleLock lock(m_DecoderSection);

    surf = (VASurfaceID)(uintptr_t)data;
    m_videoSurfaces.ClearReference(surf);
  }

  IHardwareDecoder::Release();
}

void CDecoder::SetCodecControl(int flags)
{
  m_codecControl = flags & (DVD_CODEC_CTRL_DRAIN | DVD_CODEC_CTRL_HURRY);
}

int CDecoder::Decode(AVCodecContext* avctx, AVFrame* pFrame)
{
  int result = Check(avctx);
  if (result && result != VC_NOBUFFER)
    return result;

  CSingleLock lock(m_DecoderSection);

  if (!m_vaapiConfigured)
    return VC_ERROR;

  if (pFrame)
  { // we have a new frame from decoder

    VASurfaceID surf = (VASurfaceID)(uintptr_t)pFrame->data[3];
    // ffmpeg vc-1 decoder does not flush, make sure the data buffer is still valid
    if (!m_videoSurfaces.IsValid(surf))
    {
      CLog::Log(LOGWARNING, "VAAPI::Decode - ignoring invalid buffer");
      return VC_BUFFER;
    }
    m_videoSurfaces.MarkRender(surf);

    // send frame to output for processing
    CVaapiDecodedPicture pic;
    memset(&pic.DVDPic, 0, sizeof(pic.DVDPic));
    ((CDVDVideoCodecFFmpeg*)avctx->opaque)->GetPictureCommon(&pic.DVDPic);
    pic.videoSurface = surf;
    pic.DVDPic.color_matrix = avctx->colorspace;
    m_bufferStats.IncDecoded();
    m_vaapiOutput.m_dataPort.SendOutMessage(COutputDataProtocol::NEWFRAME, &pic, sizeof(pic));

    m_codecControl = pic.DVDPic.iFlags & (DVD_CODEC_CTRL_HURRY | DVD_CODEC_CTRL_NO_POSTPROC);
  }

  int retval = 0;
  uint16_t decoded, processed, render;
  bool vpp;
  Message *msg;
  while (m_vaapiOutput.m_controlPort.ReceiveInMessage(&msg))
  {
    if (msg->signal == COutputControlProtocol::ERROR)
    {
      m_DisplayState = VAAPI_ERROR;
      retval |= VC_ERROR;
    }
    msg->Release();
  }

  m_bufferStats.Get(decoded, processed, render, vpp);

  while (!retval)
  {
    bool drain = (m_codecControl & DVD_CODEC_CTRL_DRAIN);
    // if all pics are drained, break the loop by setting VC_BUFFER
    if (drain && decoded <= 0 && processed <= 0 && render <= 0)
      drain = false;

    // first fill the buffers to keep vaapi busy
    if (decoded < 2 && processed < 3 && m_videoSurfaces.HasFree() && !drain)
    {
      retval |= VC_BUFFER;
    }
    else if (m_vaapiOutput.m_dataPort.ReceiveInMessage(&msg))
    {
      if (msg->signal == COutputDataProtocol::PICTURE)
      {
        if (m_presentPicture)
        {
          m_presentPicture->ReturnUnused();
          m_presentPicture = 0;
        }

        m_presentPicture = *(CVaapiRenderPicture**)msg->data;
        m_presentPicture->vaapi = this;
        m_bufferStats.DecRender();
        m_bufferStats.Get(decoded, processed, render, vpp);
        retval |= VC_PICTURE;
        msg->Release();
        break;
      }
      msg->Release();
    }
    else if (m_vaapiOutput.m_controlPort.ReceiveInMessage(&msg))
    {
      if (msg->signal == COutputControlProtocol::STATS)
      {
        m_bufferStats.Get(decoded, processed, render, vpp);
      }
      else
      {
        m_DisplayState = VAAPI_ERROR;
        retval |= VC_ERROR;
      }
      msg->Release();
    }

    if (decoded < 2 && processed < 3)
    {
      retval |= VC_BUFFER;
    }

    if (!retval && !m_inMsgEvent.WaitMSec(2000))
      break;
  }
  if (retval & VC_PICTURE)
  {
    m_bufferStats.SetParams(0, m_codecControl);
  }

  if (!retval)
  {
    CLog::Log(LOGERROR, "VAAPI::%s - timed out waiting for output message - decoded: %d, proc: %d, has free surface: %s",
                        __FUNCTION__, decoded, processed, m_videoSurfaces.HasFree() ? "yes" : "no");
    m_DisplayState = VAAPI_ERROR;
    retval |= VC_ERROR;
  }

  return retval;
}

int CDecoder::Check(AVCodecContext* avctx)
{
  int ret = 0;
  EDisplayState state;

  { CSingleLock lock(m_DecoderSection);
    state = m_DisplayState;
  }

  if (state == VAAPI_LOST)
  {
    if (g_advancedSettings.CanLogComponent(LOGVIDEO))
      CLog::Log(LOGDEBUG,"VAAPI::Check waiting for display reset event");
    if (!m_DisplayEvent.WaitMSec(4000))
    {
      CLog::Log(LOGERROR, "VAAPI::Check - device didn't reset in reasonable time");
      state = VAAPI_RESET;
    }
    else
    {
      CSingleLock lock(m_DecoderSection);
      state = m_DisplayState;
    }
  }
  if (state == VAAPI_RESET || state == VAAPI_ERROR)
  {
    CSingleLock lock(m_DecoderSection);

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
      return VC_FLUSHED;
    else
      return VC_ERROR;
  }

  if (m_getBufferError > 0 && m_getBufferError < 5)
  {
    // if there is no other error, sleep for a short while
    // in order not to drain player's message queue
    if (!ret)
      Sleep(20);

    ret |= VC_NOBUFFER;
  }

  return ret;
}

bool CDecoder::GetPicture(AVCodecContext* avctx, DVDVideoPicture* picture)
{
  CSingleLock lock(m_DecoderSection);

  if (m_DisplayState != VAAPI_OPEN)
    return false;

  *picture = m_presentPicture->DVDPic;
  picture->vaapi = m_presentPicture;

  return true;
}

void CDecoder::Reset()
{
  CSingleLock lock(m_DecoderSection);

  if (!m_vaapiConfigured)
    return;

  Message *reply;
  if (m_vaapiOutput.m_controlPort.SendOutMessageSync(COutputControlProtocol::FLUSH,
                                                 &reply,
                                                 2000))
  {
    bool success = reply->signal == COutputControlProtocol::ACC ? true : false;
    reply->Release();
    if (!success)
    {
      CLog::Log(LOGERROR, "VAAPI::%s - flush returned error", __FUNCTION__);
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
    CLog::Log(LOGERROR, "VAAPI::%s - flush timed out", __FUNCTION__);
    m_DisplayState = VAAPI_ERROR;
  }
}

bool CDecoder::CanSkipDeint()
{
  return m_bufferStats.CanSkipDeint();
}

bool CDecoder::CheckSuccess(VAStatus status)
{
  if (status != VA_STATUS_SUCCESS)
  {
    CLog::Log(LOGERROR, "VAAPI::%s - error: %s", __FUNCTION__, vaErrorStr(status));
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
  memset(&m_hwContext, 0, sizeof(vaapi_context));

  m_vaapiConfig.dpy = m_vaapiConfig.context->GetDisplay();
  m_vaapiConfig.attrib = m_vaapiConfig.context->GetAttrib(m_vaapiConfig.profile);
  if ((m_vaapiConfig.attrib.value & (VA_RT_FORMAT_YUV420 | VA_RT_FORMAT_YUV420_10BPP)) == 0)
  {
    CLog::Log(LOGERROR, "VAAPI - invalid yuv format %x", m_vaapiConfig.attrib.value);
    return false;
  }

  m_vaapiConfig.configId = m_vaapiConfig.context->CreateConfig(m_vaapiConfig.profile,
                                                               m_vaapiConfig.attrib);
  if (m_vaapiConfig.configId == VA_INVALID_ID)
    return false;

  // create surfaces
  VASurfaceID surfaces[32];
  unsigned int format = VA_RT_FORMAT_YUV420;
  if (m_vaapiConfig.profile == VAProfileHEVCMain10)
    format = VA_RT_FORMAT_YUV420_10BPP;
  int nb_surfaces = m_vaapiConfig.maxReferences;
  if (!CheckSuccess(vaCreateSurfaces(m_vaapiConfig.dpy,
                                     format,
                                     m_vaapiConfig.surfaceWidth,
                                     m_vaapiConfig.surfaceHeight,
                                     surfaces,
                                     nb_surfaces,
                                     NULL, 0)))
  {
    return false;
  }
  for (int i=0; i<nb_surfaces; i++)
  {
    m_videoSurfaces.AddSurface(surfaces[i]);
  }

  // create vaapi decoder context
  if (!CheckSuccess(vaCreateContext(m_vaapiConfig.dpy,
                                    m_vaapiConfig.configId,
                                    m_vaapiConfig.surfaceWidth,
                                    m_vaapiConfig.surfaceHeight,
                                    VA_PROGRESSIVE,
                                    surfaces,
                                    nb_surfaces,
                                    &m_vaapiConfig.contextId)))
  {
    m_vaapiConfig.contextId = VA_INVALID_ID;
    return false;
  }

  // initialize output
  CSingleLock lock(g_graphicsContext);
  m_vaapiConfig.stats = &m_bufferStats;
  m_vaapiConfig.vaapi = this;
  m_bufferStats.Reset();
  m_vaapiOutput.Start();
  Message *reply;
  if (m_vaapiOutput.m_controlPort.SendOutMessageSync(COutputControlProtocol::INIT,
                                                     &reply,
                                                     2000,
                                                     &m_vaapiConfig,
                                                     sizeof(m_vaapiConfig)))
  {
    bool success = reply->signal == COutputControlProtocol::ACC ? true : false;
    if (!success)
    {
      reply->Release();
      CLog::Log(LOGERROR, "VAAPI::%s - vaapi output returned error", __FUNCTION__);
      m_vaapiOutput.Dispose();
      return false;
    }
    reply->Release();
  }
  else
  {
    CLog::Log(LOGERROR, "VAAPI::%s - failed to init output", __FUNCTION__);
    m_vaapiOutput.Dispose();
    return false;
  }

  m_hwContext.config_id = m_vaapiConfig.configId;
  m_hwContext.context_id = m_vaapiConfig.contextId;
  m_hwContext.display = m_vaapiConfig.dpy;

  m_inMsgEvent.Reset();
  m_vaapiConfigured = true;
  m_ErrorCount = 0;

  return true;
}

void CDecoder::FiniVAAPIOutput()
{
  if (!m_vaapiConfigured)
    return;

  memset(&m_hwContext, 0, sizeof(vaapi_context));

  // uninit output
  m_vaapiOutput.Dispose();
  m_vaapiConfigured = false;

  // destroy decoder context
  if (m_vaapiConfig.contextId != VA_INVALID_ID)
    CheckSuccess(vaDestroyContext(m_vaapiConfig.dpy, m_vaapiConfig.contextId));
  m_vaapiConfig.contextId = VA_INVALID_ID;

  // destroy surfaces
  if (g_advancedSettings.CanLogComponent(LOGVIDEO))
    CLog::Log(LOGDEBUG, "VAAPI::FiniVAAPIOutput destroying %d video surfaces", m_videoSurfaces.Size());
  VASurfaceID surf;
  while((surf = m_videoSurfaces.RemoveNext()) != VA_INVALID_SURFACE)
  {
    CheckSuccess(vaDestroySurfaces(m_vaapiConfig.dpy, &surf, 1));
  }
  m_videoSurfaces.Reset();

  // destroy vaapi config
  if (m_vaapiConfig.configId != VA_INVALID_ID)
    CheckSuccess(vaDestroyConfig(m_vaapiConfig.dpy, m_vaapiConfig.configId));
  m_vaapiConfig.configId = VA_INVALID_ID;
}

void CDecoder::ReturnRenderPicture(CVaapiRenderPicture *renderPic)
{
  m_vaapiOutput.m_dataPort.SendOutMessage(COutputDataProtocol::RETURNPIC, &renderPic, sizeof(renderPic));
}

void CDecoder::ReturnProcPicture(int id)
{
  m_vaapiOutput.m_dataPort.SendOutMessage(COutputDataProtocol::RETURNPROCPIC, &id, sizeof(int));
}

//-----------------------------------------------------------------------------
// RenderPicture
//-----------------------------------------------------------------------------

CVaapiRenderPicture* CVaapiRenderPicture::Acquire()
{
  CSingleLock lock(renderPicSection);

  if (refCount == 0)
    vaapi->Acquire();

  refCount++;
  return this;
}

long CVaapiRenderPicture::Release()
{
  CSingleLock lock(renderPicSection);

  refCount--;
  if (refCount > 0)
    return refCount;

  lock.Leave();
  vaapi->ReturnRenderPicture(this);
  vaapi->ReleasePicReference();

  return 0;
}

void CVaapiRenderPicture::ReturnUnused()
{
  { CSingleLock lock(renderPicSection);
    if (refCount > 0)
      return;
  }
  if (vaapi)
    vaapi->ReturnRenderPicture(this);
}

void CVaapiRenderPicture::Sync()
{
#ifdef GL_ARB_sync
  CSingleLock lock(renderPicSection);
  if (usefence)
  {
    if(glIsSync(fence))
    {
      glDeleteSync(fence);
      fence = None;
    }
    fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
  }
#endif
}

bool CVaapiRenderPicture::GLMapSurface()
{
  VAStatus status;
  glInterop.vaImage.image_id = VA_INVALID_ID;

  vaSyncSurface(glInterop.vadsp, glInterop.procPic.videoSurface);

  status = vaDeriveImage(glInterop.vadsp, glInterop.procPic.videoSurface,
                                                  &glInterop.vaImage);
  if (status != VA_STATUS_SUCCESS)
  {
    CLog::Log(LOGERROR, "VAAPI::%s - Error: %s(%d)", __FUNCTION__, vaErrorStr(status), status);
    return false;
  }
  memset(&glInterop.vBufInfo, 0, sizeof(glInterop.vBufInfo));
  glInterop.vBufInfo.mem_type = VA_SURFACE_ATTRIB_MEM_TYPE_DRM_PRIME;
  status = vaAcquireBufferHandle(glInterop.vadsp, glInterop.vaImage.buf,
                                 &glInterop.vBufInfo);
  if (status != VA_STATUS_SUCCESS)
  {
    CLog::Log(LOGERROR, "VAAPI::%s - Error: %s(%d)", __FUNCTION__, vaErrorStr(status), status);
    return false;
  }

  texWidth = glInterop.vaImage.width;
  texHeight = glInterop.vaImage.height;

  GLint attribs[23], *attrib;

  switch (glInterop.vaImage.format.fourcc)
  {
    case VA_FOURCC('N','V','1','2'):
    {
      attrib = attribs;
      *attrib++ = EGL_LINUX_DRM_FOURCC_EXT;
      *attrib++ = fourcc_code('R', '8', ' ', ' ');
      *attrib++ = EGL_WIDTH;
      *attrib++ = glInterop.vaImage.width;
      *attrib++ = EGL_HEIGHT;
      *attrib++ = glInterop.vaImage.height;
      *attrib++ = EGL_DMA_BUF_PLANE0_FD_EXT;
      *attrib++ = (intptr_t)glInterop.vBufInfo.handle;
      *attrib++ = EGL_DMA_BUF_PLANE0_OFFSET_EXT;
      *attrib++ = glInterop.vaImage.offsets[0];
      *attrib++ = EGL_DMA_BUF_PLANE0_PITCH_EXT;
      *attrib++ = glInterop.vaImage.pitches[0];
      *attrib++ = EGL_NONE;
      glInterop.eglImageY = glInterop.eglCreateImageKHR(glInterop.eglDisplay,
                                          EGL_NO_CONTEXT, EGL_LINUX_DMA_BUF_EXT, (EGLClientBuffer)NULL,
                                          attribs);
      if (!glInterop.eglImageY)
      {
        EGLint err = eglGetError();
        CLog::Log(LOGERROR, "failed to import VA buffer NV12 into EGL image: %d", err);
        return false;
      }

      attrib = attribs;
      *attrib++ = EGL_LINUX_DRM_FOURCC_EXT;
      *attrib++ = fourcc_code('G', 'R', '8', '8');
      *attrib++ = EGL_WIDTH;
      *attrib++ = (glInterop.vaImage.width + 1) >> 1;
      *attrib++ = EGL_HEIGHT;
      *attrib++ = (glInterop.vaImage.height + 1) >> 1;
      *attrib++ = EGL_DMA_BUF_PLANE0_FD_EXT;
      *attrib++ = (intptr_t)glInterop.vBufInfo.handle;
      *attrib++ = EGL_DMA_BUF_PLANE0_OFFSET_EXT;
      *attrib++ = glInterop.vaImage.offsets[1];
      *attrib++ = EGL_DMA_BUF_PLANE0_PITCH_EXT;
      *attrib++ = glInterop.vaImage.pitches[1];
      *attrib++ = EGL_NONE;
      glInterop.eglImageVU = glInterop.eglCreateImageKHR(glInterop.eglDisplay,
                                          EGL_NO_CONTEXT, EGL_LINUX_DMA_BUF_EXT, (EGLClientBuffer)NULL,
                                          attribs);
      if (!glInterop.eglImageVU)
      {
        EGLint err = eglGetError();
        CLog::Log(LOGERROR, "failed to import VA buffer NV12 into EGL image: %d", err);
        return false;
      }

      GLint format;

      glGenTextures(1, &textureY);
      glEnable(glInterop.textureTarget);
      glBindTexture(glInterop.textureTarget, textureY);
      glTexParameteri(glInterop.textureTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(glInterop.textureTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(glInterop.textureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(glInterop.textureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      glInterop.glEGLImageTargetTexture2DOES(glInterop.textureTarget, glInterop.eglImageY);
      glGetIntegerv(GL_IMPLEMENTATION_COLOR_READ_FORMAT, &format);

      glGenTextures(1, &textureVU);
      glEnable(glInterop.textureTarget);
      glBindTexture(glInterop.textureTarget, textureVU);
      glTexParameteri(glInterop.textureTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(glInterop.textureTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(glInterop.textureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(glInterop.textureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      glInterop.glEGLImageTargetTexture2DOES(glInterop.textureTarget, glInterop.eglImageVU);
      glGetIntegerv(GL_IMPLEMENTATION_COLOR_READ_FORMAT, &format);

      glBindTexture(glInterop.textureTarget, 0);
      glDisable(glInterop.textureTarget);

      break;
    }
    case VA_FOURCC('P','0','1','0'):
    {
      attrib = attribs;
      *attrib++ = EGL_LINUX_DRM_FOURCC_EXT;
      *attrib++ = fourcc_code('R', '1', '6', ' ');
      *attrib++ = EGL_WIDTH;
      *attrib++ = glInterop.vaImage.width;
      *attrib++ = EGL_HEIGHT;
      *attrib++ = glInterop.vaImage.height;
      *attrib++ = EGL_DMA_BUF_PLANE0_FD_EXT;
      *attrib++ = (intptr_t)glInterop.vBufInfo.handle;
      *attrib++ = EGL_DMA_BUF_PLANE0_OFFSET_EXT;
      *attrib++ = glInterop.vaImage.offsets[0];
      *attrib++ = EGL_DMA_BUF_PLANE0_PITCH_EXT;
      *attrib++ = glInterop.vaImage.pitches[0];
      *attrib++ = EGL_NONE;
      glInterop.eglImageY = glInterop.eglCreateImageKHR(glInterop.eglDisplay,
                                          EGL_NO_CONTEXT, EGL_LINUX_DMA_BUF_EXT, (EGLClientBuffer)NULL,
                                          attribs);
      if (!glInterop.eglImageY)
      {
        EGLint err = eglGetError();
        CLog::Log(LOGERROR, "failed to import VA buffer P010 into EGL image: %d", err);
        return false;
      }

      attrib = attribs;
      *attrib++ = EGL_LINUX_DRM_FOURCC_EXT;
      *attrib++ = fourcc_code('G', 'R', '3', '2');
      *attrib++ = EGL_WIDTH;
      *attrib++ = (glInterop.vaImage.width + 1) >> 1;
      *attrib++ = EGL_HEIGHT;
      *attrib++ = (glInterop.vaImage.height + 1) >> 1;
      *attrib++ = EGL_DMA_BUF_PLANE0_FD_EXT;
      *attrib++ = (intptr_t)glInterop.vBufInfo.handle;
      *attrib++ = EGL_DMA_BUF_PLANE0_OFFSET_EXT;
      *attrib++ = glInterop.vaImage.offsets[1];
      *attrib++ = EGL_DMA_BUF_PLANE0_PITCH_EXT;
      *attrib++ = glInterop.vaImage.pitches[1];
      *attrib++ = EGL_NONE;
      glInterop.eglImageVU = glInterop.eglCreateImageKHR(glInterop.eglDisplay,
                                          EGL_NO_CONTEXT, EGL_LINUX_DMA_BUF_EXT, (EGLClientBuffer)NULL,
                                          attribs);
      if (!glInterop.eglImageVU)
      {
        EGLint err = eglGetError();
        CLog::Log(LOGERROR, "failed to import VA buffer P010 into EGL image: %d", err);
        return false;
      }

      GLint format;

      glGenTextures(1, &textureY);
      glEnable(glInterop.textureTarget);
      glBindTexture(glInterop.textureTarget, textureY);
      glTexParameteri(glInterop.textureTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(glInterop.textureTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(glInterop.textureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(glInterop.textureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      glInterop.glEGLImageTargetTexture2DOES(glInterop.textureTarget, glInterop.eglImageY);
      glGetIntegerv(GL_IMPLEMENTATION_COLOR_READ_FORMAT, &format);

      glGenTextures(1, &textureVU);
      glEnable(glInterop.textureTarget);
      glBindTexture(glInterop.textureTarget, textureVU);
      glTexParameteri(glInterop.textureTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(glInterop.textureTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(glInterop.textureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(glInterop.textureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      glInterop.glEGLImageTargetTexture2DOES(glInterop.textureTarget, glInterop.eglImageVU);
      glGetIntegerv(GL_IMPLEMENTATION_COLOR_READ_FORMAT, &format);

      glBindTexture(glInterop.textureTarget, 0);
      glDisable(glInterop.textureTarget);

      break;
    }
    case VA_FOURCC('B','G','R','A'):
    {
      attrib = attribs;
      *attrib++ = EGL_DRM_BUFFER_FORMAT_MESA;
      *attrib++ = EGL_DRM_BUFFER_FORMAT_ARGB32_MESA;
      *attrib++ = EGL_WIDTH;
      *attrib++ = glInterop.vaImage.width;
      *attrib++ = EGL_HEIGHT;
      *attrib++ = glInterop.vaImage.height;
      *attrib++ = EGL_DRM_BUFFER_STRIDE_MESA;
      *attrib++ = glInterop.vaImage.pitches[0] / 4;
      *attrib++ = EGL_NONE;
      glInterop.eglImage = glInterop.eglCreateImageKHR(glInterop.eglDisplay, EGL_NO_CONTEXT,
                                         EGL_DRM_BUFFER_MESA,
                                         (EGLClientBuffer)glInterop.vBufInfo.handle,
                                         attribs);
      if (!glInterop.eglImage)
      {
        EGLint err = eglGetError();
        CLog::Log(LOGERROR, "failed to import VA buffer BGRA into EGL image: %d", err);
        return false;
      }

      glGenTextures(1, &texture);
      glEnable(glInterop.textureTarget);
      glBindTexture(glInterop.textureTarget, texture);
      glTexParameteri(glInterop.textureTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(glInterop.textureTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(glInterop.textureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(glInterop.textureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

      glInterop.glEGLImageTargetTexture2DOES(glInterop.textureTarget, glInterop.eglImage);

      glBindTexture(glInterop.textureTarget, 0);
      glDisable(glInterop.textureTarget);

      break;
    }
    default:
      return false;
  }

  return true;
}

void CVaapiRenderPicture::GLUnMapSurface()
{
  if (glInterop.vaImage.image_id == VA_INVALID_ID)
    return;

  glInterop.eglDestroyImageKHR(glInterop.eglDisplay, glInterop.eglImageY);
  glInterop.eglDestroyImageKHR(glInterop.eglDisplay, glInterop.eglImageVU);

  VAStatus status;
  status = vaReleaseBufferHandle(glInterop.vadsp, glInterop.vaImage.buf);
  if (status != VA_STATUS_SUCCESS)
  {
    CLog::Log(LOGERROR, "VAAPI::%s - Error: %s(%d)", __FUNCTION__, vaErrorStr(status), status);
  }

  status = vaDestroyImage(glInterop.vadsp, glInterop.vaImage.image_id);
  if (status != VA_STATUS_SUCCESS)
  {
    CLog::Log(LOGERROR, "VAAPI::%s - Error: %s(%d)", __FUNCTION__, vaErrorStr(status), status);
  }
  glInterop.mapped = false;
  glInterop.vaImage.image_id = VA_INVALID_ID;

  glDeleteTextures(1, &textureY);
  glDeleteTextures(1, &textureVU);
}

//-----------------------------------------------------------------------------
// Buffer Pool
//-----------------------------------------------------------------------------

VaapiBufferPool::VaapiBufferPool()
{
  CVaapiRenderPicture *pic;
  for (unsigned int i = 0; i < NUM_RENDER_PICS; i++)
  {
    pic = new CVaapiRenderPicture(renderPicSec);
    allRenderPics.push_back(pic);
  }
}

VaapiBufferPool::~VaapiBufferPool()
{
  CVaapiRenderPicture *pic;
  for (unsigned int i = 0; i < NUM_RENDER_PICS; i++)
  {
    pic = allRenderPics[i];
    delete pic;
  }
  allRenderPics.clear();
}

//-----------------------------------------------------------------------------
// Output
//-----------------------------------------------------------------------------
COutput::COutput(CEvent *inMsgEvent) :
  CThread("Vaapi-Output"),
  m_controlPort("OutputControlPort", inMsgEvent, &m_outMsgEvent),
  m_dataPort("OutputDataPort", inMsgEvent, &m_outMsgEvent)
{
  m_inMsgEvent = inMsgEvent;

  for (unsigned int i = 0; i < m_bufferPool.allRenderPics.size(); ++i)
  {
    m_bufferPool.freeRenderPics.push_back(i);
  }
}

void COutput::Start()
{
  Create();
}

COutput::~COutput()
{
  Dispose();

  m_bufferPool.freeRenderPics.clear();
  m_bufferPool.usedRenderPics.clear();
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
          CVaapiConfig *data;
          data = (CVaapiConfig*)msg->data;
          if (data)
          {
            m_config = *data;
          }
          Init();

          // set initial number of
          EnsureBufferPool();
          if (!m_vaError)
          {
            m_state = O_TOP_CONFIGURED_IDLE;
            msg->Reply(COutputControlProtocol::ACC);
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
          ReleaseBufferPool(true);
          msg->Reply(COutputControlProtocol::ACC);
          m_state = O_TOP_UNCONFIGURED;
          m_extTimeout = 10000;
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
          CVaapiDecodedPicture *frame;
          frame = (CVaapiDecodedPicture*)msg->data;
          if (frame)
          {
            m_bufferPool.decodedPics.push_back(*frame);
            m_extTimeout = 0;
          }
          return;
        case COutputDataProtocol::RETURNPIC:
          CVaapiRenderPicture *pic;
          pic = *((CVaapiRenderPicture**)msg->data);
          QueueReturnPicture(pic);
          m_controlPort.SendInMessage(COutputControlProtocol::STATS);
          m_extTimeout = 0;
          return;
        case COutputDataProtocol::RETURNPROCPIC:
          int id;
          id = *((int*)msg->data);
          ProcessReturnProcPicture(id);
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
          if (ProcessSyncPicture())
            m_extTimeout = 10;
          else
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
          if (PreferPP())
          {
            m_currentPicture = m_bufferPool.decodedPics.front();
            m_bufferPool.decodedPics.pop_front();
            InitCycle();
            m_state = O_TOP_CONFIGURED_STEP1;
            m_extTimeout = 0;
            return;
          }
          else if (!m_bufferPool.freeRenderPics.empty() &&
                   !m_bufferPool.processedPics.empty())
          {
            m_state = O_TOP_CONFIGURED_OUTPUT;
            m_extTimeout = 0;
            return;
          }
          else
            m_state = O_TOP_CONFIGURED_IDLE;
          m_extTimeout = 100;
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
          if (!m_pp->AddPicture(m_currentPicture))
          {
            m_state = O_TOP_ERROR;
            return;
          }
          CVaapiProcessedPicture outPic;
          if (m_pp->Filter(outPic))
          {
            m_config.stats->IncProcessed();
            m_bufferPool.processedPics.push_back(outPic);
            m_state = O_TOP_CONFIGURED_STEP2;
          }
          else
          {
            m_state = O_TOP_CONFIGURED_IDLE;
          }
          m_config.stats->DecDecoded();
          m_controlPort.SendInMessage(COutputControlProtocol::STATS);
          m_extTimeout = 0;
          return;
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
          CVaapiProcessedPicture outPic;
          if (m_pp->Filter(outPic))
          {
            m_bufferPool.processedPics.push_back(outPic);
            m_config.stats->IncProcessed();
            m_extTimeout = 0;
            return;
          }
          m_state = O_TOP_CONFIGURED_IDLE;
          m_extTimeout = 0;
          return;
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
          if (!m_bufferPool.processedPics.empty())
          {
            CVaapiRenderPicture *outPic;
            CVaapiProcessedPicture procPic;
            procPic = m_bufferPool.processedPics.front();
            m_config.stats->DecProcessed();
            m_bufferPool.processedPics.pop_front();
            outPic = ProcessPicture(procPic);
            if (outPic)
            {
              m_config.stats->IncRender();
              m_dataPort.SendInMessage(COutputDataProtocol::PICTURE, &outPic, sizeof(outPic));
            }
            if (m_vaError)
            {
              m_state = O_TOP_ERROR;
              return;
            }
          }
          m_state = O_TOP_CONFIGURED_IDLE;
          m_extTimeout = 0;
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
  if (!CreateEGLContext())
    return false;

  if (!GLInit())
    return false;

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

  m_vaError = false;

  return true;
}

bool COutput::Uninit()
{
  glFlush();
  while(ProcessSyncPicture())
  {
    Sleep(10);
  }
  ReleaseBufferPool();
  delete m_pp;
  m_pp = NULL;
  DestroyEGLContext();
  return true;
}

void COutput::Flush()
{
  Message *msg;
  while (m_dataPort.ReceiveOutMessage(&msg))
  {
    if (msg->signal == COutputDataProtocol::NEWFRAME)
    {
      CVaapiDecodedPicture pic = *(CVaapiDecodedPicture*)msg->data;
      m_config.videoSurfaces->ClearRender(pic.videoSurface);
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
      QueueReturnPicture(pic);
    }
    msg->Release();
  }

  for (unsigned int i = 0; i < m_bufferPool.decodedPics.size(); i++)
  {
    m_config.videoSurfaces->ClearRender(m_bufferPool.decodedPics[i].videoSurface);
  }
  m_bufferPool.decodedPics.clear();

  for (unsigned int i = 0; i < m_bufferPool.processedPics.size(); i++)
  {
    ReleaseProcessedPicture(m_bufferPool.processedPics[i]);
  }
  m_bufferPool.processedPics.clear();

  if (m_pp)
    m_pp->Flush();
}

bool COutput::HasWork()
{
  // send a pic to renderer
  if (!m_bufferPool.freeRenderPics.empty() && !m_bufferPool.processedPics.empty())
    return true;

  bool ppWantsPic = true;
  if (m_pp)
    ppWantsPic = m_pp->WantsPic();

  if (!m_bufferPool.decodedPics.empty() && m_bufferPool.processedPics.size() < 4 && ppWantsPic)
    return true;

  return false;
}

bool COutput::PreferPP()
{
  if (!m_bufferPool.decodedPics.empty())
  {
    if (!m_pp)
      return true;

    if (!m_pp->WantsPic())
      return false;

    if (!m_pp->DoesSync() && m_bufferPool.processedPics.size() < 4)
      return true;

    if (m_bufferPool.freeRenderPics.empty() || m_bufferPool.processedPics.empty())
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

  EINTERLACEMETHOD method = CMediaSettings::GetInstance().GetCurrentVideoSettings().m_InterlaceMethod;
  bool interlaced = m_currentPicture.DVDPic.iFlags & DVP_FLAG_INTERLACED;

  if (!(flags & DVD_CODEC_CTRL_NO_POSTPROC) &&
      interlaced &&
      method != VS_INTERLACEMETHOD_NONE)
  {
    if (!m_config.processInfo->Supports(method))
      method = VS_INTERLACEMETHOD_VAAPI_BOB;

    if (m_pp && (method != m_currentDiMethod || !m_pp->Compatible(method)))
    {
      delete m_pp;
      m_pp = NULL;
      DropVppProcessedPictures();
      m_config.processInfo->SetVideoDeintMethod("unknown");
    }
    if (!m_pp)
    {
      if (method == VS_INTERLACEMETHOD_DEINTERLACE ||
          method == VS_INTERLACEMETHOD_RENDER_BOB)
      {
        m_pp = new CFFmpegPostproc();
        m_config.stats->SetVpp(false);
      }
      else
      {
        m_pp = new CVppPostproc();
        m_config.stats->SetVpp(true);
      }
      if (m_pp->PreInit(m_config))
      {
        m_pp->Init(method);
        m_currentDiMethod = method;

        if (method == VS_INTERLACEMETHOD_DEINTERLACE)
          m_config.processInfo->SetVideoDeintMethod("yadif");
        else if (method == VS_INTERLACEMETHOD_RENDER_BOB)
          m_config.processInfo->SetVideoDeintMethod("render-bob");
        else if (method == VS_INTERLACEMETHOD_VAAPI_BOB)
          m_config.processInfo->SetVideoDeintMethod("vaapi-bob");
        else if (method == VS_INTERLACEMETHOD_VAAPI_MADI)
          m_config.processInfo->SetVideoDeintMethod("vaapi-madi");
        else if (method == VS_INTERLACEMETHOD_VAAPI_MACI)
          m_config.processInfo->SetVideoDeintMethod("vaapi-mcdi");
      }
      else
      {
        delete m_pp;
        m_pp = NULL;
      }
    }
  }
  // progressive
  else
  {
    method = VS_INTERLACEMETHOD_NONE;
    if (m_pp && !m_pp->Compatible(method))
    {
      delete m_pp;
      m_pp = NULL;
      DropVppProcessedPictures();
    }
    if (!m_pp)
    {
      m_config.stats->SetVpp(false);
      if (!CServiceBroker::GetSettings().GetBool(CSettings::SETTING_VIDEOPLAYER_PREFERVAAPIRENDER))
        m_pp = new CFFmpegPostproc();
      else
      {
        m_pp = new CSkipPostproc();
        m_config.stats->SetVpp(true);
      }
      if (m_pp->PreInit(m_config))
      {
        m_pp->Init(method);
        m_currentDiMethod = method;
        m_config.processInfo->SetVideoDeintMethod("none");
      }
      else
      {
        delete m_pp;
        m_pp = NULL;
      }
    }
  }
  if (!m_pp) // fallback
  {
    m_pp = new CSkipPostproc();
    if (m_pp->PreInit(m_config))
      m_pp->Init(method);
  }
}

CVaapiRenderPicture* COutput::ProcessPicture(CVaapiProcessedPicture &pic)
{
  CVaapiRenderPicture *retPic;
  int idx = m_bufferPool.freeRenderPics.front();
  retPic = m_bufferPool.allRenderPics[idx];
  retPic->DVDPic = pic.DVDPic;

  if (pic.source == CVaapiProcessedPicture::SKIP_SRC ||
      pic.source == CVaapiProcessedPicture::VPP_SRC)
  {
    pic.id = m_bufferPool.procPicId++;
    m_bufferPool.processedPicsAway.push_back(pic);
    retPic->DVDPic.format = RENDER_FMT_VAAPI;
    retPic->glInterop.procPic = pic;
    retPic->glInterop.mapped = false;
    retPic->GLMapSurface();
  }
  else if (pic.source == CVaapiProcessedPicture::FFMPEG_SRC)
  {
    av_frame_move_ref(retPic->avFrame, pic.frame);
    av_frame_free(&pic.frame);
    retPic->DVDPic.format = RENDER_FMT_VAAPINV12;
  }
  else
    return NULL;

  m_bufferPool.freeRenderPics.pop_front();
  m_bufferPool.usedRenderPics.push_back(idx);

  retPic->DVDPic.dts = DVD_NOPTS_VALUE;
  retPic->DVDPic.iWidth = m_config.vidWidth;
  retPic->DVDPic.iHeight = m_config.vidHeight;

  retPic->valid = true;
  retPic->crop.x1 = 0;
  retPic->crop.y1 = 0;
  retPic->crop.x2 = m_config.outWidth;
  retPic->crop.y2 = m_config.outHeight;

  return retPic;
}

void COutput::ReleaseProcessedPicture(CVaapiProcessedPicture &pic)
{
  if (pic.source == CVaapiProcessedPicture::VPP_SRC && m_pp)
  {
    CVppPostproc *pp = dynamic_cast<CVppPostproc*>(m_pp);
    if (pp)
    {
      pp->ClearRef(pic.videoSurface);
    }
  }
  else if (pic.source == CVaapiProcessedPicture::SKIP_SRC)
  {
    m_config.videoSurfaces->ClearRender(pic.videoSurface);
  }
  else if (pic.source == CVaapiProcessedPicture::FFMPEG_SRC)
  {
    av_frame_free(&pic.frame);
  }
}

void COutput::DropVppProcessedPictures()
{
  auto it = m_bufferPool.processedPics.begin();
  while (it != m_bufferPool.processedPics.end())
  {
    if (it->source == CVaapiProcessedPicture::VPP_SRC)
    {
      it = m_bufferPool.processedPics.erase(it);
      m_config.stats->DecProcessed();
    }
    else
      ++it;
  }

  it = m_bufferPool.processedPicsAway.begin();
  while (it != m_bufferPool.processedPicsAway.end())
  {
    if (it->source == CVaapiProcessedPicture::VPP_SRC)
    {
      it = m_bufferPool.processedPicsAway.erase(it);
    }
    else
      ++it;
  }

  m_controlPort.SendInMessage(COutputControlProtocol::STATS);
}

void COutput::QueueReturnPicture(CVaapiRenderPicture *pic)
{
  std::deque<int>::iterator it;
  for (it = m_bufferPool.usedRenderPics.begin(); it != m_bufferPool.usedRenderPics.end(); ++it)
  {
    if (m_bufferPool.allRenderPics[*it] == pic)
    {
      break;
    }
  }

  if (it == m_bufferPool.usedRenderPics.end())
  {
    CLog::Log(LOGWARNING, "COutput::QueueReturnPicture - pic not found");
    return;
  }

  // check if already queued
  auto it2 = find(m_bufferPool.syncRenderPics.begin(),
                  m_bufferPool.syncRenderPics.end(),
                  *it);
  if (it2 == m_bufferPool.syncRenderPics.end())
  {
    m_bufferPool.syncRenderPics.push_back(*it);
  }

  ProcessSyncPicture();
}

bool COutput::ProcessSyncPicture()
{
  CVaapiRenderPicture *pic;
  bool busy = false;

  for (auto it = m_bufferPool.syncRenderPics.begin(); it != m_bufferPool.syncRenderPics.end(); )
  {
    pic = m_bufferPool.allRenderPics[*it];

#ifdef GL_ARB_sync
    if (pic->usefence)
    {
      if (pic->fence)
      {
        GLint state;
        GLsizei length;
        glGetSynciv(pic->fence, GL_SYNC_STATUS, 1, &length, &state);
        if(state == GL_SIGNALED)
        {
          glDeleteSync(pic->fence);
          pic->fence = None;
        }
        else
        {
          busy = true;
          ++it;
          continue;
        }
      }
    }
#endif

    m_bufferPool.freeRenderPics.push_back(*it);

    auto it2 = find(m_bufferPool.usedRenderPics.begin(),
                    m_bufferPool.usedRenderPics.end(),
                    *it);
    if (it2 == m_bufferPool.usedRenderPics.end())
    {
      CLog::Log(LOGERROR, "COutput::ProcessSyncPicture - pic not found in queue");
    }
    else
    {
      m_bufferPool.usedRenderPics.erase(it2);
    }
    it = m_bufferPool.syncRenderPics.erase(it);

    if (pic->valid)
    {
      ProcessReturnPicture(pic);
    }
    else
    {
      if (g_advancedSettings.CanLogComponent(LOGVIDEO))
        CLog::Log(LOGDEBUG, "COutput::%s - return of invalid render pic", __FUNCTION__);
    }
  }
  return busy;
}

void COutput::ProcessReturnPicture(CVaapiRenderPicture *pic)
{
  if (pic->avFrame)
    av_frame_unref(pic->avFrame);

  pic->GLUnMapSurface();
  ProcessReturnProcPicture(pic->glInterop.procPic.id);
  pic->valid = false;
}

void COutput::ProcessReturnProcPicture(int id)
{
  for (auto it=m_bufferPool.processedPicsAway.begin(); it!=m_bufferPool.processedPicsAway.end(); ++it)
  {
    if (it->id == id)
    {
      ReleaseProcessedPicture(*it);
      m_bufferPool.processedPicsAway.erase(it);
      break;
    }
  }
}

bool COutput::EnsureBufferPool()
{
  // create avFrames and init interop
  CVaapiRenderPicture *pic;
  for (unsigned int i = 0; i < m_bufferPool.allRenderPics.size(); i++)
  {
    pic = m_bufferPool.allRenderPics[i];
    pic->glInterop.vadsp = m_config.dpy;

    pic->glInterop.eglDisplay = m_eglDisplay;
    pic->glInterop.textureTarget = m_textureTarget;
    pic->glInterop.eglCreateImageKHR = eglCreateImageKHR;
    pic->glInterop.eglDestroyImageKHR = eglDestroyImageKHR;
    pic->glInterop.glEGLImageTargetTexture2DOES = glEGLImageTargetTexture2DOES;
    pic->glInterop.vaImage.image_id = VA_INVALID_ID;
    pic->glInterop.mapped = false;

    pic->avFrame = av_frame_alloc();
    pic->valid = false;
  }

  m_bufferPool.procPicId = 0;
  if (g_advancedSettings.CanLogComponent(LOGVIDEO))
    CLog::Log(LOGDEBUG, "VAAPI::COutput::InitBufferPool - Surfaces created");
  return true;
}

void COutput::ReleaseBufferPool(bool precleanup)
{
  CVaapiRenderPicture *pic;

  CSingleLock lock(m_bufferPool.renderPicSec);

  if (!precleanup)
  {
    // wait for all fences
    XbmcThreads::EndTime timeout(1000);
    for (unsigned int i = 0; i < m_bufferPool.allRenderPics.size(); i++)
    {
      pic = m_bufferPool.allRenderPics[i];
      if (pic->usefence)
      {
#ifdef GL_ARB_sync
        while (pic->fence)
        {
          GLint state;
          GLsizei length;
          glGetSynciv(pic->fence, GL_SYNC_STATUS, 1, &length, &state);
          if(state == GL_SIGNALED || timeout.IsTimePast())
          {
            glDeleteSync(pic->fence);
            pic->fence = None;
          }
          else
          {
            Sleep(5);
          }
        }
        pic->fence = None;
#endif
      }
    }
    if (timeout.IsTimePast())
    {
      CLog::Log(LOGERROR, "COutput::%s - timeout waiting for fence", __FUNCTION__);
    }
  }

  ProcessSyncPicture();

  for (unsigned int i = 0; i < m_bufferPool.allRenderPics.size(); i++)
  {
    pic = m_bufferPool.allRenderPics[i];

    if (precleanup && pic->valid)
      continue;


    if (pic->texture)
    {
      glDeleteTextures(1, &pic->texture);
      pic->texture = None;
    }
    av_frame_free(&pic->avFrame);
    pic->valid = false;
  }

  for (unsigned int i = 0; i < m_bufferPool.decodedPics.size(); i++)
  {
    m_config.videoSurfaces->ClearRender(m_bufferPool.decodedPics[i].videoSurface);
  }
  m_bufferPool.decodedPics.clear();

  for (unsigned int i = 0; i < m_bufferPool.processedPics.size(); i++)
  {
    ReleaseProcessedPicture(m_bufferPool.processedPics[i]);
  }
  m_bufferPool.processedPics.clear();
}

bool COutput::GLInit()
{
#ifdef GL_ARB_sync
  bool hasfence = g_Windowing.IsExtSupported("GL_ARB_sync");
  for (unsigned int i = 0; i < m_bufferPool.allRenderPics.size(); i++)
  {
    m_bufferPool.allRenderPics[i]->usefence = hasfence;
  }
#endif

  if (!g_Windowing.IsExtSupported("GL_ARB_texture_non_power_of_two") &&
       g_Windowing.IsExtSupported("GL_ARB_texture_rectangle"))
  {
    m_textureTarget = GL_TEXTURE_RECTANGLE_ARB;
  }
  else
    m_textureTarget = GL_TEXTURE_2D;

  eglCreateImageKHR = (PFNEGLCREATEIMAGEKHRPROC)eglGetProcAddress("eglCreateImageKHR");
  eglDestroyImageKHR = (PFNEGLDESTROYIMAGEKHRPROC)eglGetProcAddress("eglDestroyImageKHR");
  glEGLImageTargetTexture2DOES = (PFNGLEGLIMAGETARGETTEXTURE2DOESPROC)eglGetProcAddress("glEGLImageTargetTexture2DOES");
  return true;
}

bool COutput::CheckSuccess(VAStatus status)
{
  if (status != VA_STATUS_SUCCESS)
  {
    CLog::Log(LOGERROR, "VAAPI::%s - Error: %s(%d)", __FUNCTION__, vaErrorStr(status), status);
    m_vaError = true;
    return false;
  }
  return true;
}

bool COutput::CreateEGLContext()
{
  EGLDisplay eglDisplay = g_Windowing.GetEGLDisplay();
  EGLContext eglMainContext = g_Windowing.GetEGLContext();
  EGLConfig eglMainConfig = g_Windowing.GetEGLConfig();

  EGLint pbufferAttribs[] =
  {
    EGL_WIDTH, 8,
    EGL_HEIGHT, 8,
    EGL_TEXTURE_TARGET, EGL_NO_TEXTURE,
    EGL_TEXTURE_FORMAT, EGL_NO_TEXTURE,
    EGL_NONE
  };
  EGLint contextAttributes[] =
  {
    EGL_CONTEXT_CLIENT_VERSION, 2,
    EGL_NONE
  };
  if (!eglBindAPI(EGL_OPENGL_API))
  {
    CLog::Log(LOGERROR, "VAAPI::COutput::CreateEGLContext -failed to bind egl API");
    return false;
  }
  m_eglSurface = eglCreatePbufferSurface(eglDisplay, eglMainConfig, pbufferAttribs);
  m_eglContext = eglCreateContext(eglDisplay, eglMainConfig, eglMainContext, contextAttributes);
  m_eglDisplay = eglDisplay;

  if (!eglMakeCurrent(eglDisplay, m_eglSurface, m_eglSurface, m_eglContext))
  {
    CLog::Log(LOGERROR, "VAAPI::COutput::CreateEGLContext - Could not make surface current");
    return false;
  }
  if (g_advancedSettings.CanLogComponent(LOGVIDEO))
    CLog::Log(LOGDEBUG, "VAAPI::COutput::CreateEGLContext - created context");
  return true;
}

bool COutput::DestroyEGLContext()
{
  if (m_eglContext)
  {
    glFinish();
    //eglMakeCurrent(m_eglContext, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroyContext(m_eglDisplay, m_eglContext);
  }
  m_eglContext = EGL_NO_CONTEXT;

  if (m_eglSurface)
    eglDestroySurface(m_eglDisplay, m_eglSurface);
  m_eglSurface = EGL_NO_SURFACE;

  return true;
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
  if (m_step>0)
    return false;
  outPic.DVDPic = m_pic.DVDPic;
  outPic.videoSurface = m_pic.videoSurface;
  outPic.source = CVaapiProcessedPicture::SKIP_SRC;
  outPic.DVDPic.iFlags &= ~(DVP_FLAG_TOP_FIELD_FIRST |
                            DVP_FLAG_REPEAT_TOP_FIELD |
                            DVP_FLAG_INTERLACED);
  m_step++;
  return true;
}

void CSkipPostproc::ClearRef(VASurfaceID surf)
{

}

void CSkipPostproc::Flush()
{

}

bool CSkipPostproc::Compatible(EINTERLACEMETHOD method)
{
  if (method == VS_INTERLACEMETHOD_NONE)
    return true;

  return false;
}

bool CSkipPostproc::DoesSync()
{
  return false;
}

//-----------------------------------------------------------------------------
// VPP Postprocessing
//-----------------------------------------------------------------------------

CVppPostproc::CVppPostproc()
{
  m_contextId = VA_INVALID_ID;
  m_configId = VA_INVALID_ID;
  m_filter = VA_INVALID_ID;
}

CVppPostproc::~CVppPostproc()
{
  Dispose();
}

bool CVppPostproc::PreInit(CVaapiConfig &config, SDiMethods *methods)
{
  m_config = config;

  // create config
  if (!CheckSuccess(vaCreateConfig(m_config.dpy, VAProfileNone, VAEntrypointVideoProc, NULL, 0, &m_configId)))
  {
    if (g_advancedSettings.CanLogComponent(LOGVIDEO))
      CLog::Log(LOGDEBUG, "CVppPostproc::PreInit  - VPP init failed");

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
    format = VA_RT_FORMAT_YUV420_10BPP;
  int nb_surfaces = NUM_RENDER_PICS;
  if (!CheckSuccess(vaCreateSurfaces(m_config.dpy,
                                     format,
                                     m_config.surfaceWidth,
                                     m_config.surfaceHeight,
                                     surfaces,
                                     nb_surfaces,
                                     attribs, 1)))
  {
    if (g_advancedSettings.CanLogComponent(LOGVIDEO))
      CLog::Log(LOGDEBUG, "CVppPostproc::PreInit  - VPP init failed");

    return false;
  }
  for (int i=0; i<nb_surfaces; i++)
  {
    m_videoSurfaces.AddSurface(surfaces[i]);
  }

  // create vaapi decoder context
  if (!CheckSuccess(vaCreateContext(m_config.dpy,
                                    m_configId,
                                    0,
                                    0,
                                    0,
                                    surfaces,
                                    nb_surfaces,
                                    &m_contextId)))
  {
    m_contextId = VA_INVALID_ID;
    if (g_advancedSettings.CanLogComponent(LOGVIDEO))
      CLog::Log(LOGDEBUG, "CVppPostproc::PreInit  - VPP init failed");

    return false;
  }

  VAProcFilterType filters[VAProcFilterCount];
  unsigned int numFilters = VAProcFilterCount;
  VAProcFilterCapDeinterlacing deinterlacingCaps[VAProcDeinterlacingCount];
  unsigned int numDeinterlacingCaps = VAProcDeinterlacingCount;

  if (!CheckSuccess(vaQueryVideoProcFilters(m_config.dpy, m_contextId, filters, &numFilters)))
  {
    if (g_advancedSettings.CanLogComponent(LOGVIDEO))
      CLog::Log(LOGDEBUG, "CVppPostproc::PreInit  - VPP init failed");

    return false;
  }

  if (!CheckSuccess(vaQueryVideoProcFilterCaps(m_config.dpy,
                                               m_contextId,
                                               VAProcFilterDeinterlacing,
                                               deinterlacingCaps,
                                               &numDeinterlacingCaps)))
  {
    if (g_advancedSettings.CanLogComponent(LOGVIDEO))
      CLog::Log(LOGDEBUG, "CVppPostproc::PreInit  - VPP init failed");

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
  m_vppMethod = method;

  VAProcDeinterlacingType vppMethod = VAProcDeinterlacingNone;
  switch (method)
  {
  case VS_INTERLACEMETHOD_VAAPI_BOB:
    vppMethod = VAProcDeinterlacingBob;
    break;
  case VS_INTERLACEMETHOD_VAAPI_MADI:
    vppMethod = VAProcDeinterlacingMotionAdaptive;
    break;
  case VS_INTERLACEMETHOD_VAAPI_MACI:
    vppMethod = VAProcDeinterlacingMotionCompensated;
    break;
  case VS_INTERLACEMETHOD_NONE:
    // vppMethod = VAProcDeinterlacingNone;
    break;
  default:
    return false;
  }

  m_forwardRefs = 0;
  m_backwardRefs = 0;
  m_currentIdx = 0;
  m_frameCount = 0;

//  if (method == VS_INTERLACEMETHOD_NONE)
//    return true;

  VAProcFilterParameterBufferDeinterlacing filterparams;
  filterparams.type = VAProcFilterDeinterlacing;
  filterparams.algorithm = vppMethod;
  filterparams.flags = 0;

  if (!CheckSuccess(vaCreateBuffer(m_config.dpy, m_contextId,
                    VAProcFilterParameterBufferType,
                    sizeof(filterparams), 1, &filterparams, &m_filter)))
  {
    m_filter = VA_INVALID_ID;
    return false;
  }

  VAProcPipelineCaps pplCaps;
  if (!CheckSuccess(vaQueryVideoProcPipelineCaps(m_config.dpy, m_contextId,
                    &m_filter, 1, &pplCaps)))
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
    CheckSuccess(vaSyncSurface(m_config.dpy, m_videoSurfaces.GetAtIndex(i)));
  }

  if (m_filter != VA_INVALID_ID)
  {
    CheckSuccess(vaDestroyBuffer(m_config.dpy, m_filter));
    m_filter = VA_INVALID_ID;
  }
  if (m_contextId != VA_INVALID_ID)
  {
    CheckSuccess(vaDestroyContext(m_config.dpy, m_contextId));
    m_contextId = VA_INVALID_ID;
  }
  VASurfaceID surf;
  while((surf = m_videoSurfaces.RemoveNext()) != VA_INVALID_SURFACE)
  {
    CheckSuccess(vaDestroySurfaces(m_config.dpy, &surf, 1));
  }
  m_videoSurfaces.Reset();

  if (m_configId != VA_INVALID_ID)
  {
    CheckSuccess(vaDestroyConfig(m_config.dpy, m_configId));
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

  // make sure we have all needed forward refs
  if ((m_currentIdx - m_forwardRefs) < m_decodedPics.back().index)
  {
    Advance();
    return false;
  }

  const auto currentIdx = m_currentIdx;
  auto it = std::find_if(m_decodedPics.begin(), m_decodedPics.end(),
                         [currentIdx](const CVaapiDecodedPicture &picture){
                           return picture.index == currentIdx;
                         });
  if (it==m_decodedPics.end())
  {
    return false;
  }
  outPic.DVDPic = it->DVDPic;

  // skip deinterlacing cycle if requested
  if ((m_step == 1) &&
      ((outPic.DVDPic.iFlags & DVD_CODEC_CTRL_SKIPDEINT) || (m_vppMethod == VS_INTERLACEMETHOD_NONE)))
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

  if (!CheckSuccess(vaBeginPicture(m_config.dpy, m_contextId, surf)))
  {
    return false;
  }

  if (!CheckSuccess(vaCreateBuffer(m_config.dpy, m_contextId,
                    VAProcPipelineParameterBufferType,
                    sizeof(VAProcPipelineParameterBuffer), 1, NULL, &pipelineBuf)))
  {
    return false;
  }
  if (!CheckSuccess(vaMapBuffer(m_config.dpy, pipelineBuf, (void**)&pipelineParams)))
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

  VASurfaceID forwardRefs[32];
  VASurfaceID backwardRefs[32];
  pipelineParams->forward_references = forwardRefs;
  pipelineParams->backward_references = backwardRefs;
  pipelineParams->num_forward_references = 0;
  pipelineParams->num_backward_references = 0;

  int maxPic = m_currentIdx + m_backwardRefs;
  int minPic = m_currentIdx - m_forwardRefs;
  int curPic = m_currentIdx;

  // deinterlace flag
  if (m_vppMethod != VS_INTERLACEMETHOD_NONE)
  {
    unsigned int flags = 0;
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
    if (!CheckSuccess(vaMapBuffer(m_config.dpy, m_filter, (void**)&filterParams)))
    {
      return false;
    }
    filterParams->flags = flags;
    if (!CheckSuccess(vaUnmapBuffer(m_config.dpy, m_filter)))
    {
      return false;
    }

    if (m_vppMethod == VS_INTERLACEMETHOD_VAAPI_BOB)
      pipelineParams->filter_flags = (flags & VA_DEINTERLACING_BOTTOM_FIELD) ? VA_BOTTOM_FIELD : VA_TOP_FIELD;
    else
      pipelineParams->filter_flags = 0;

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

  if (!CheckSuccess(vaUnmapBuffer(m_config.dpy, pipelineBuf)))
  {
    return false;
  }

  if (!CheckSuccess(vaRenderPicture(m_config.dpy, m_contextId,
                                    &pipelineBuf, 1)))
  {
    return false;
  }

  if (!CheckSuccess(vaEndPicture(m_config.dpy, m_contextId)))
  {
    return false;
  }

  if (!CheckSuccess(vaDestroyBuffer(m_config.dpy, pipelineBuf)))
  {
    return false;
  }

  m_step++;
  outPic.videoSurface = m_videoSurfaces.GetFree(surf);
  outPic.source = CVaapiProcessedPicture::VPP_SRC;
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
    if (it->index < m_currentIdx - m_forwardRefs)
    {
      m_config.videoSurfaces->ClearRender(it->videoSurface);
      it = m_decodedPics.erase(it);
    }
    else
      ++it;
  }
}

void CVppPostproc::ClearRef(VASurfaceID surf)
{
  m_videoSurfaces.ClearReference(surf);
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

bool CVppPostproc::Compatible(EINTERLACEMETHOD method)
{
  if (method == VS_INTERLACEMETHOD_VAAPI_BOB ||
      method == VS_INTERLACEMETHOD_VAAPI_MADI ||
      method == VS_INTERLACEMETHOD_VAAPI_MACI ||
      method == VS_INTERLACEMETHOD_NONE)
    return true;

  return false;
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

bool CVppPostproc::CheckSuccess(VAStatus status)
{
  if (status != VA_STATUS_SUCCESS)
  {
    CLog::Log(LOGERROR, "VAAPI::%s - Error: %s(%d)", __FUNCTION__, vaErrorStr(status), status);
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
  _aligned_free(m_cache);
  m_dllSSE4.Unload();
  av_frame_free(&m_pFilterFrameIn);
  av_frame_free(&m_pFilterFrameOut);
}

bool CFFmpegPostproc::PreInit(CVaapiConfig &config, SDiMethods *methods)
{
  m_config = config;
  bool use_filter = true;
  if (!m_dllSSE4.Load())
  {
    CLog::Log(LOGERROR,"VAAPI::SupportsFilter failed loading sse4 lib");
    return false;
  }

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
    CLog::Log(LOGWARNING,"VAAPI::SupportsFilter vaDeriveImage not supported");
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
    CheckSuccess(vaDestroyImage(config.dpy,image.image_id));

  if (use_filter)
  {
    m_cache = (uint8_t*)_aligned_malloc(CACHED_BUFFER_SIZE, 64);
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
    CLog::Log(LOGERROR, "CFFmpegPostproc::Init - unable to alloc filter graph");
    return false;
  }

  AVFilter* srcFilter = avfilter_get_by_name("buffer");
  AVFilter* outFilter = avfilter_get_by_name("buffersink");

  std::string args = StringUtils::Format("%d:%d:%d:%d:%d:%d:%d",
                                        m_config.vidWidth,
                                        m_config.vidHeight,
                                        AV_PIX_FMT_NV12,
                                        1,
                                        1,
                                        (m_config.aspect.num != 0) ? m_config.aspect.num : 1,
                                        (m_config.aspect.num != 0) ? m_config.aspect.den : 1);

  if (avfilter_graph_create_filter(&m_pFilterIn, srcFilter, "src", args.c_str(), NULL, m_pFilterGraph) < 0)
  {
    CLog::Log(LOGERROR, "CFFmpegPostproc::Init - avfilter_graph_create_filter: src");
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
    CLog::Log(LOGERROR, "CFFmpegPostproc::Init  - failed settings pix formats");
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

    filter = "yadif=1:-1";

    if (avfilter_graph_parse_ptr(m_pFilterGraph, filter.c_str(), &inputs, &outputs, NULL) < 0)
    {
      CLog::Log(LOGERROR, "CFFmpegPostproc::Init  - avfilter_graph_parse");
      avfilter_inout_free(&outputs);
      avfilter_inout_free(&inputs);
      return false;
    }

    avfilter_inout_free(&outputs);
    avfilter_inout_free(&inputs);

    if (avfilter_graph_config(m_pFilterGraph, NULL) < 0)
    {
      CLog::Log(LOGERROR, "CFFmpegPostproc::Init  - avfilter_graph_config");
      return false;
    }
  }
  else if (method == VS_INTERLACEMETHOD_RENDER_BOB ||
           method == VS_INTERLACEMETHOD_NONE)
  {
    if (g_advancedSettings.CanLogComponent(LOGVIDEO))
      CLog::Log(LOGDEBUG, "CFFmpegPostproc::Init  - skip deinterlacing");
    avfilter_inout_free(&outputs);
    avfilter_inout_free(&inputs);
  }
  else
  {
    avfilter_inout_free(&outputs);
    avfilter_inout_free(&inputs);
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
  m_DVDPic = inPic.DVDPic;
  bool result = false;

  if (!CheckSuccess(vaSyncSurface(m_config.dpy, surf)))
    goto error;

  if (!CheckSuccess(vaDeriveImage(m_config.dpy, surf, &image)))
    goto error;

  if (!CheckSuccess(vaMapBuffer(m_config.dpy, image.buf, (void**)&buf)))
    goto error;

  m_pFilterFrameIn->format = AV_PIX_FMT_NV12;
  m_pFilterFrameIn->width = m_config.vidWidth;
  m_pFilterFrameIn->height = m_config.vidHeight;
  m_pFilterFrameIn->linesize[0] = image.pitches[0];
  m_pFilterFrameIn->linesize[1] = image.pitches[1];
  m_pFilterFrameIn->interlaced_frame = (inPic.DVDPic.iFlags & DVP_FLAG_INTERLACED) ? 1 : 0;
  m_pFilterFrameIn->top_field_first = (inPic.DVDPic.iFlags & DVP_FLAG_TOP_FIELD_FIRST) ? 1 : 0;

  if (inPic.DVDPic.pts == DVD_NOPTS_VALUE)
    m_pFilterFrameIn->pkt_pts = AV_NOPTS_VALUE;
  else
    m_pFilterFrameIn->pkt_pts = (inPic.DVDPic.pts / DVD_TIME_BASE) * AV_TIME_BASE;

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

  CheckSuccess(vaUnmapBuffer(m_config.dpy, image.buf));
  CheckSuccess(vaDestroyImage(m_config.dpy,image.image_id));

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
  outPic.DVDPic = m_DVDPic;
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
    if (m_step>0)
      return false;
  }

  m_step++;
  outPic.frame = av_frame_clone(m_pFilterFrameOut);
  av_frame_unref(m_pFilterFrameOut);

  outPic.source = CVaapiProcessedPicture::FFMPEG_SRC;

  if(outPic.frame->pkt_pts != AV_NOPTS_VALUE)
  {
    outPic.DVDPic.pts = (double)outPic.frame->pkt_pts * DVD_TIME_BASE / AV_TIME_BASE;
  }
  else
    outPic.DVDPic.pts = DVD_NOPTS_VALUE;

  double pts = outPic.DVDPic.pts;
  if (m_lastOutPts != DVD_NOPTS_VALUE && m_lastOutPts == pts)
  {
    outPic.DVDPic.pts += m_frametime/2;
  }
  m_lastOutPts = pts;

  for (int i = 0; i < 4; i++)
    outPic.DVDPic.data[i] = outPic.frame->data[i];
  for (int i = 0; i < 4; i++)
    outPic.DVDPic.iLineSize[i] = outPic.frame->linesize[i];

  return true;
}

void CFFmpegPostproc::ClearRef(VASurfaceID surf)
{

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

bool CFFmpegPostproc::Compatible(EINTERLACEMETHOD method)
{
  if (method == VS_INTERLACEMETHOD_DEINTERLACE)
    return true;
  else if (method == VS_INTERLACEMETHOD_RENDER_BOB)
    return true;
  else if (method == VS_INTERLACEMETHOD_NONE &&
           !CServiceBroker::GetSettings().GetBool(CSettings::SETTING_VIDEOPLAYER_PREFERVAAPIRENDER))
    return true;

  return false;
}

bool CFFmpegPostproc::DoesSync()
{
  return true;
}

bool CFFmpegPostproc::CheckSuccess(VAStatus status)
{
  if (status != VA_STATUS_SUCCESS)
  {
    CLog::Log(LOGERROR, "VAAPI - Error: %s(%d)", vaErrorStr(status), status);
    return false;
  }
  return true;
}
