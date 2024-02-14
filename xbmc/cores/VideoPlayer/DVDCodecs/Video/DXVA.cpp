/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

// setting that here because otherwise SampleFormat is defined to AVSampleFormat
// which we don't use here
#define FF_API_OLD_SAMPLE_FMT 0

#define LIMIT_VIDEO_MEMORY_4K 2960ull

#include "DXVA.h"

#include "ServiceBroker.h"
#include "cores/VideoPlayer/DVDCodecs/DVDCodecUtils.h"
#include "cores/VideoPlayer/DVDCodecs/DVDFactoryCodec.h"
#include "cores/VideoPlayer/Process/ProcessInfo.h"
#include "cores/VideoPlayer/VideoRenderers/RenderManager.h"
#include "rendering/dx/DeviceResources.h"
#include "rendering/dx/RenderContext.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/CPUInfo.h"
#include "utils/StringUtils.h"
#include "utils/SystemInfo.h"
#include "utils/log.h"

#include <algorithm>
#include <mutex>

#include <Windows.h>
#include <dxva.h>
#include <initguid.h>
#include <sdkddkver.h>

using namespace DXVA;
using namespace Microsoft::WRL;
using namespace std::chrono_literals;

// clang-format off
DEFINE_GUID(DXVADDI_Intel_ModeH264_A,      0x604F8E64,0x4951,0x4c54,0x88,0xFE,0xAB,0xD2,0x5C,0x15,0xB3,0xD6);
DEFINE_GUID(DXVADDI_Intel_ModeH264_C,      0x604F8E66,0x4951,0x4c54,0x88,0xFE,0xAB,0xD2,0x5C,0x15,0xB3,0xD6);
DEFINE_GUID(DXVADDI_Intel_ModeH264_E,      0x604F8E68,0x4951,0x4c54,0x88,0xFE,0xAB,0xD2,0x5C,0x15,0xB3,0xD6);
DEFINE_GUID(DXVADDI_Intel_ModeVC1_E,       0xBCC5DB6D,0xA2B6,0x4AF0,0xAC,0xE4,0xAD,0xB1,0xF7,0x87,0xBC,0x89);
DEFINE_GUID(DXVA_ModeH264_VLD_NoFGT_Flash, 0x4245F676,0x2BBC,0x4166,0xa0,0xBB,0x54,0xE7,0xB8,0x49,0xC3,0x80);
DEFINE_GUID(DXVA_Intel_VC1_ClearVideo_2,   0xE07EC519,0xE651,0x4CD6,0xAC,0x84,0x13,0x70,0xCC,0xEE,0xC8,0x51);

// These AV1 GUIDs are only defined from Windows SDK 10.0.22000.0
// Defined conditional here to able compile with lower SDK versions
#ifndef NTDDI_WIN10_CO
DEFINE_GUID(D3D11_DECODER_PROFILE_AV1_VLD_PROFILE0, 0xb8be4ccb,0xcf53,0x46ba,0x8d,0x59,0xd6,0xb8,0xa6,0xda,0x5d,0x2a);
DEFINE_GUID(D3D11_DECODER_PROFILE_AV1_VLD_PROFILE1, 0x6936ff0f,0x45b1,0x4163,0x9c,0xc1,0x64,0x6e,0xf6,0x94,0x61,0x08);
DEFINE_GUID(D3D11_DECODER_PROFILE_AV1_VLD_PROFILE2, 0x0c5f2aa1,0xe541,0x4089,0xbb,0x7b,0x98,0x11,0x0a,0x19,0xd7,0xc8);
#endif

// redefine DXVA_NoEncrypt with other macro, solves unresolved external symbol linker error
#ifndef DXVA_NoEncrypt
DEFINE_GUID(DXVA_NoEncrypt, 0x1b81beD0, 0xa0c7, 0x11d3, 0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5);
#endif
// clang-format on

namespace
{
constexpr int PROFILES_MPEG2_SIMPLE[] = {FF_PROFILE_MPEG2_SIMPLE, FF_PROFILE_UNKNOWN};
constexpr int PROFILES_MPEG2_MAIN[] = {FF_PROFILE_MPEG2_SIMPLE, FF_PROFILE_MPEG2_MAIN,
                                       FF_PROFILE_UNKNOWN};
constexpr int PROFILES_H264_HIGH[] = {FF_PROFILE_H264_BASELINE,
                                      FF_PROFILE_H264_CONSTRAINED_BASELINE, FF_PROFILE_H264_MAIN,
                                      FF_PROFILE_H264_HIGH, FF_PROFILE_UNKNOWN};
constexpr int PROFILES_HEVC_MAIN[] = {FF_PROFILE_HEVC_MAIN, FF_PROFILE_UNKNOWN};
constexpr int PROFILES_HEVC_MAIN10[] = {FF_PROFILE_HEVC_MAIN, FF_PROFILE_HEVC_MAIN_10,
                                        FF_PROFILE_UNKNOWN};
constexpr int PROFILES_VP9_0[] = {FF_PROFILE_VP9_0, FF_PROFILE_UNKNOWN};
constexpr int PROFILES_VP9_10_2[] = {FF_PROFILE_VP9_2, FF_PROFILE_UNKNOWN};
constexpr int PROFILES_AV1_MAIN[] = {FF_PROFILE_AV1_MAIN, FF_PROFILE_UNKNOWN};
constexpr int PROFILES_AV1_HIGH[] = {FF_PROFILE_AV1_HIGH, FF_PROFILE_UNKNOWN};
constexpr int PROFILES_AV1_PROFESSIONAL[] = {FF_PROFILE_AV1_PROFESSIONAL, FF_PROFILE_UNKNOWN};
} // namespace

typedef struct
{
  const char* name;
  const GUID* guid;
  int codec;
  const int* profiles;
} dxva2_mode_t;

/* XXX Preferred modes must come first */
// clang-format off
static const std::vector<dxva2_mode_t> dxva2_modes = {
    { "MPEG2 variable-length decoder",                                              &D3D11_DECODER_PROFILE_MPEG2_VLD,     AV_CODEC_ID_MPEG2VIDEO, PROFILES_MPEG2_MAIN },
    { "MPEG1/2 variable-length decoder",                                            &D3D11_DECODER_PROFILE_MPEG2and1_VLD, AV_CODEC_ID_MPEG2VIDEO, PROFILES_MPEG2_MAIN },
    { "MPEG2 motion compensation",                                                  &D3D11_DECODER_PROFILE_MPEG2_MOCOMP,  0, nullptr },
    { "MPEG2 inverse discrete cosine transform",                                    &D3D11_DECODER_PROFILE_MPEG2_IDCT,    0, nullptr},

    { "MPEG-1 variable-length decoder",                                             &D3D11_DECODER_PROFILE_MPEG1_VLD,     0, nullptr },

    { "H.264 variable-length decoder, film grain technology",                       &D3D11_DECODER_PROFILE_H264_VLD_FGT,              AV_CODEC_ID_H264, PROFILES_H264_HIGH },
    { "H.264 variable-length decoder, no film grain technology (Intel ClearVideo)", &DXVADDI_Intel_ModeH264_E,                        AV_CODEC_ID_H264, PROFILES_H264_HIGH },
    { "H.264 variable-length decoder, no film grain technology",                    &D3D11_DECODER_PROFILE_H264_VLD_NOFGT,            AV_CODEC_ID_H264, PROFILES_H264_HIGH },
    { "H.264 variable-length decoder, no film grain technology, FMO/ASO",           &D3D11_DECODER_PROFILE_H264_VLD_WITHFMOASO_NOFGT, AV_CODEC_ID_H264, PROFILES_H264_HIGH },
    { "H.264 variable-length decoder, no film grain technology, Flash",             &DXVA_ModeH264_VLD_NoFGT_Flash,                   AV_CODEC_ID_H264, PROFILES_H264_HIGH },

    { "H.264 inverse discrete cosine transform, film grain technology",             &D3D11_DECODER_PROFILE_H264_IDCT_FGT,     0, nullptr },
    { "H.264 inverse discrete cosine transform, no film grain technology",          &D3D11_DECODER_PROFILE_H264_IDCT_NOFGT,   0, nullptr },
    { "H.264 inverse discrete cosine transform, no film grain technology (Intel)",  &DXVADDI_Intel_ModeH264_C,                0, nullptr },

    { "H.264 motion compensation, film grain technology",                           &D3D11_DECODER_PROFILE_H264_MOCOMP_FGT,   0, nullptr },
    { "H.264 motion compensation, no film grain technology",                        &D3D11_DECODER_PROFILE_H264_MOCOMP_NOFGT, 0, nullptr },
    { "H.264 motion compensation, no film grain technology (Intel)",                &DXVADDI_Intel_ModeH264_A,                0, nullptr },

    { "H.264 stereo high profile, mbs flag set",                                    &D3D11_DECODER_PROFILE_H264_VLD_STEREO_PROGRESSIVE_NOFGT, 0, nullptr },
    { "H.264 stereo high profile",                                                  &D3D11_DECODER_PROFILE_H264_VLD_STEREO_NOFGT,             0, nullptr },
    { "H.264 multi-view high profile",                                              &D3D11_DECODER_PROFILE_H264_VLD_MULTIVIEW_NOFGT,          0, nullptr },

    { "Windows Media Video 8 motion compensation",                                  &D3D11_DECODER_PROFILE_WMV8_MOCOMP,   0, nullptr },
    { "Windows Media Video 8 post processing",                                      &D3D11_DECODER_PROFILE_WMV8_POSTPROC, 0, nullptr },

    { "Windows Media Video 9 inverse discrete cosine transform",                    &D3D11_DECODER_PROFILE_WMV9_IDCT,     0, nullptr },
    { "Windows Media Video 9 motion compensation",                                  &D3D11_DECODER_PROFILE_WMV9_MOCOMP,   0, nullptr },
    { "Windows Media Video 9 post processing",                                      &D3D11_DECODER_PROFILE_WMV9_POSTPROC, 0, nullptr },

    { "VC-1 variable-length decoder",                                               &D3D11_DECODER_PROFILE_VC1_VLD,      AV_CODEC_ID_VC1, nullptr },
    { "VC-1 variable-length decoder",                                               &D3D11_DECODER_PROFILE_VC1_VLD,      AV_CODEC_ID_WMV3, nullptr },
    { "VC-1 variable-length decoder 2010",                                          &D3D11_DECODER_PROFILE_VC1_D2010,    AV_CODEC_ID_VC1, nullptr },
    { "VC-1 variable-length decoder 2010",                                          &D3D11_DECODER_PROFILE_VC1_D2010,    AV_CODEC_ID_WMV3, nullptr },
    { "VC-1 variable-length decoder 2 (Intel)",                                     &DXVA_Intel_VC1_ClearVideo_2,        0, nullptr },
    { "VC-1 variable-length decoder (Intel)",                                       &DXVADDI_Intel_ModeVC1_E,            0, nullptr },

    { "VC-1 inverse discrete cosine transform",                                     &D3D11_DECODER_PROFILE_VC1_IDCT,     0, nullptr },
    { "VC-1 motion compensation",                                                   &D3D11_DECODER_PROFILE_VC1_MOCOMP,   0, nullptr },
    { "VC-1 post processing",                                                       &D3D11_DECODER_PROFILE_VC1_POSTPROC, 0, nullptr },

    { "HEVC variable-length decoder, main",                                         &D3D11_DECODER_PROFILE_HEVC_VLD_MAIN,   AV_CODEC_ID_HEVC, PROFILES_HEVC_MAIN },
    { "HEVC variable-length decoder, main10",                                       &D3D11_DECODER_PROFILE_HEVC_VLD_MAIN10, AV_CODEC_ID_HEVC, PROFILES_HEVC_MAIN10 },

    { "VP9 variable-length decoder, Profile 0",                                     &D3D11_DECODER_PROFILE_VP9_VLD_PROFILE0,       AV_CODEC_ID_VP9, PROFILES_VP9_0 },
    { "VP9 variable-length decoder, 10bit, profile 2",                              &D3D11_DECODER_PROFILE_VP9_VLD_10BIT_PROFILE2, AV_CODEC_ID_VP9, PROFILES_VP9_10_2 },

    { "AV1 Profile 0 (main)",                                                       &D3D11_DECODER_PROFILE_AV1_VLD_PROFILE0, AV_CODEC_ID_AV1, PROFILES_AV1_MAIN },
    { "AV1 Profile 1 (high)",                                                       &D3D11_DECODER_PROFILE_AV1_VLD_PROFILE1, AV_CODEC_ID_AV1, PROFILES_AV1_HIGH },
    { "AV1 Profile 2 (professional)",                                               &D3D11_DECODER_PROFILE_AV1_VLD_PROFILE2, AV_CODEC_ID_AV1, PROFILES_AV1_PROFESSIONAL },
};
// clang-format on

// Preferred targets must be first
static const DXGI_FORMAT render_targets_dxgi[] = {
  DXGI_FORMAT_NV12,
  DXGI_FORMAT_P010,
  DXGI_FORMAT_P016,
  DXGI_FORMAT_UNKNOWN
};

// List of PCI Device ID of ATI cards with UVD or UVD+ decoding block.
static DWORD UVDDeviceID [] = {
  0x95C0, // ATI Radeon HD 3400 Series (and others)
  0x95C5, // ATI Radeon HD 3400 Series (and others)
  0x95C4, // ATI Radeon HD 3400 Series (and others)
  0x94C3, // ATI Radeon HD 3410
  0x9589, // ATI Radeon HD 3600 Series (and others)
  0x9598, // ATI Radeon HD 3600 Series (and others)
  0x9591, // ATI Radeon HD 3600 Series (and others)
  0x9501, // ATI Radeon HD 3800 Series (and others)
  0x9505, // ATI Radeon HD 3800 Series (and others)
  0x9507, // ATI Radeon HD 3830
  0x9513, // ATI Radeon HD 3850 X2
  0x950F, // ATI Radeon HD 3850 X2
  0x0000
};

// List of PCI Device ID of nVidia cards with the macroblock width issue. More or less the VP3 block.
// Per NVIDIA Accelerated Linux Graphics Driver, Appendix A Supported NVIDIA GPU Products, cards with note 1.
static DWORD VP3DeviceID [] = {
  0x06E0, // GeForce 9300 GE
  0x06E1, // GeForce 9300 GS
  0x06E2, // GeForce 8400
  0x06E4, // GeForce 8400 GS
  0x06E5, // GeForce 9300M GS
  0x06E6, // GeForce G100
  0x06E8, // GeForce 9200M GS
  0x06E9, // GeForce 9300M GS
  0x06EC, // GeForce G 105M
  0x06EF, // GeForce G 103M
  0x06F1, // GeForce G105M
  0x0844, // GeForce 9100M G
  0x0845, // GeForce 8200M G
  0x0846, // GeForce 9200
  0x0847, // GeForce 9100
  0x0848, // GeForce 8300
  0x0849, // GeForce 8200
  0x084A, // nForce 730a
  0x084B, // GeForce 9200
  0x084C, // nForce 980a/780a SLI
  0x084D, // nForce 750a SLI
  0x0860, // GeForce 9400
  0x0861, // GeForce 9400
  0x0862, // GeForce 9400M G
  0x0863, // GeForce 9400M
  0x0864, // GeForce 9300
  0x0865, // ION
  0x0866, // GeForce 9400M G
  0x0867, // GeForce 9400
  0x0868, // nForce 760i SLI
  0x086A, // GeForce 9400
  0x086C, // GeForce 9300 / nForce 730i
  0x086D, // GeForce 9200
  0x086E, // GeForce 9100M G
  0x086F, // GeForce 8200M G
  0x0870, // GeForce 9400M
  0x0871, // GeForce 9200
  0x0872, // GeForce G102M
  0x0873, // GeForce G102M
  0x0874, // ION
  0x0876, // ION
  0x087A, // GeForce 9400
  0x087D, // ION
  0x087E, // ION LE
  0x087F, // ION LE
  0x0000
};

static std::string GUIDToString(const GUID& guid)
{
  std::string buffer = StringUtils::Format(
      "{:08X}-{:04x}-{:04x}-{:02x}{:02x}-{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}", guid.Data1,
      guid.Data2, guid.Data3, guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
      guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);
  return buffer;
}

static const dxva2_mode_t* dxva2_find_mode(const GUID* guid)
{
  for (const dxva2_mode_t& mode : dxva2_modes)
  {
    if (IsEqualGUID(*mode.guid, *guid))
      return &mode;
  }
  return nullptr;
}

//-----------------------------------------------------------------------------
// DXVA Context
//-----------------------------------------------------------------------------

CContext::weak_ptr CContext::m_context;
CCriticalSection CContext::m_section;

CContext::~CContext()
{
  Close();
}

void CContext::Release(CDecoder* decoder)
{
  std::unique_lock<CCriticalSection> lock(m_section);

  const auto it = std::find(m_decoders.begin(), m_decoders.end(), decoder);
  if (it != m_decoders.end())
    m_decoders.erase(it);
}

void CContext::Close()
{
  CLog::Log(LOGINFO, "DXVA: closing decoder context.");
  DestroyContext();
}

CContext::shared_ptr CContext::EnsureContext(CDecoder* decoder)
{
  std::unique_lock<CCriticalSection> lock(m_section);

  auto context = m_context.lock();
  if (context)
  {
    if (!context->IsValidDecoder(decoder))
      context->m_decoders.push_back(decoder);
    return context;
  }

  context.reset(new CContext());
  {
    if (!context->CreateContext())
      return shared_ptr();

    m_context = context;
  }

  return EnsureContext(decoder);
}

bool CContext::CreateContext()
{
  HRESULT hr = E_FAIL;
  ComPtr<ID3D11Device> pD3DDevice;
  ComPtr<ID3D11DeviceContext> pD3DDeviceContext;

  m_sharingAllowed = DX::DeviceResources::Get()->IsNV12SharedTexturesSupported();

  if (m_sharingAllowed)
  {
    CLog::LogF(LOGINFO, "creating discrete d3d11va device for decoding.");

    D3D_FEATURE_LEVEL d3dFeatureLevel{D3D_FEATURE_LEVEL_9_1};
    UINT creationFlags = D3D11_CREATE_DEVICE_VIDEO_SUPPORT;
#if defined(_DEBUG)
    if (DX::SdkLayersAvailable())
    {
      // If the project is in a debug build, enable debugging via SDK Layers with this flag.
      creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
    }
#endif

    std::vector<D3D_FEATURE_LEVEL> featureLevels;
    if (CSysInfo::IsWindowsVersionAtLeast(CSysInfo::WindowsVersionWin10))
    {
      featureLevels.push_back(D3D_FEATURE_LEVEL_12_1);
      featureLevels.push_back(D3D_FEATURE_LEVEL_12_0);
    }
    featureLevels.push_back(D3D_FEATURE_LEVEL_11_1);
    featureLevels.push_back(D3D_FEATURE_LEVEL_11_0);
    featureLevels.push_back(D3D_FEATURE_LEVEL_10_1);
    featureLevels.push_back(D3D_FEATURE_LEVEL_10_0);
    featureLevels.push_back(D3D_FEATURE_LEVEL_9_3);
    featureLevels.push_back(D3D_FEATURE_LEVEL_9_2);
    featureLevels.push_back(D3D_FEATURE_LEVEL_9_1);

    hr = D3D11CreateDevice(DX::DeviceResources::Get()->GetAdapter(), D3D_DRIVER_TYPE_UNKNOWN,
                           nullptr, creationFlags, featureLevels.data(), featureLevels.size(),
                           D3D11_SDK_VERSION, &pD3DDevice, &d3dFeatureLevel, &pD3DDeviceContext);
    if (SUCCEEDED(hr))
    {
      CLog::LogF(
          LOGINFO, "device for decoding created on adapter '{}' with {}",
          KODI::PLATFORM::WINDOWS::FromW(DX::DeviceResources::Get()->GetAdapterDesc().Description),
          DX::GetFeatureLevelDescription(d3dFeatureLevel));

#ifdef _DEBUG
      if (FAILED(pD3DDevice.As(&m_d3d11Debug)))
      {
        CLog::LogF(LOGDEBUG, "unable to create debug interface. Error {}",
                   DX::GetErrorDescription(hr));
      }
#endif
    }
    else
    {
      CLog::LogF(LOGWARNING,
                 "unable to create device for decoding, fallback to using app device. Error {}",
                 DX::GetErrorDescription(hr));
      m_sharingAllowed = false;
    }
  }
  else
  {
    CLog::LogF(LOGWARNING, "using app d3d11 device for decoding due extended NV12 shared "
                           "textures it's not supported.");
  }

  if (FAILED(hr))
  {
    pD3DDevice = DX::DeviceResources::Get()->GetD3DDevice();
    pD3DDeviceContext = DX::DeviceResources::Get()->GetImmediateContext();
  }

  if (FAILED(pD3DDevice.As(&m_pD3D11Device)) || FAILED(pD3DDeviceContext.As(&m_pD3D11Context)))
  {
    CLog::LogF(LOGWARNING, "failed to get Video Device and Context.");
    return false;
  }

  if (FAILED(hr) || !m_sharingAllowed)
  {
    // enable multi-threaded protection only if is used same d3d11 device for rendering and decoding
    ComPtr<ID3D11Multithread> multithread;
    hr = pD3DDevice.As(&multithread);
    if (SUCCEEDED(hr))
      multithread->SetMultithreadProtected(1);
  }

  QueryCaps();

  // Some older Ati devices can only open a single decoder at a given time
  std::string renderer = DX::Windowing()->GetRenderRenderer();
  if (renderer.find("Radeon HD 2") != std::string::npos ||
      renderer.find("Radeon HD 3") != std::string::npos ||
      renderer.find("Radeon HD 4") != std::string::npos ||
      renderer.find("Radeon HD 5") != std::string::npos)
  {
    m_atiWorkaround = true;
  }

  // Sets high priority process for smooth playback in all circumstances
  SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);

  return true;
}

void CContext::DestroyContext()
{
  delete[] m_input_list;
  m_pD3D11Device = nullptr;
  m_pD3D11Context = nullptr;
#ifdef _DEBUG
  if (m_d3d11Debug)
  {
    m_d3d11Debug->ReportLiveDeviceObjects(D3D11_RLDO_SUMMARY | D3D11_RLDO_DETAIL);
    m_d3d11Debug = nullptr;
    // References in the report to ID3D11VideoDecoderOutputView are normal at this point
    // They are held for VideoPlayer and should be freed later.
  }
#endif

  // Restores normal priority process
  SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);
}

void CContext::QueryCaps()
{
  m_input_count = m_pD3D11Device->GetVideoDecoderProfileCount();

  m_input_list = new GUID[m_input_count];
  for (unsigned i = 0; i < m_input_count; i++)
  {
    if (FAILED(m_pD3D11Device->GetVideoDecoderProfile(i, &m_input_list[i])))
    {
      CLog::Log(LOGINFO, "DXVA: failed getting video decoder profile");
      return;
    }
    const dxva2_mode_t* mode = dxva2_find_mode(&m_input_list[i]);
    if (mode)
      CLog::Log(LOGDEBUG, "DXVA: supports '{}'", mode->name);
    else
      CLog::Log(LOGDEBUG, "DXVA: supports {}", GUIDToString(m_input_list[i]));
  }
}

bool CContext::GetFormatAndConfig(AVCodecContext* avctx, D3D11_VIDEO_DECODER_DESC &format, D3D11_VIDEO_DECODER_CONFIG &config) const
{
  format.OutputFormat = DXGI_FORMAT_UNKNOWN;

  // iterate through our predefined dxva modes and find the first matching for desired codec
  // once we found a mode, get a target we support in render_targets_dxgi DXGI_FORMAT_UNKNOWN
  for (const dxva2_mode_t& mode : dxva2_modes)
  {
    if (mode.codec != avctx->codec_id)
      continue;

    bool supported = false;
    for (unsigned i = 0; i < m_input_count && !supported; i++)
    {
      supported = IsEqualGUID(m_input_list[i], *mode.guid) != 0;
    }
    if (supported)
    {
      // check profiles
      supported = false;
      if (mode.profiles == nullptr)
        supported = true;
      else if (avctx->profile == FF_PROFILE_UNKNOWN)
        supported = true;
      else
        for (const int* pProfile = &mode.profiles[0]; *pProfile != FF_PROFILE_UNKNOWN; ++pProfile)
        {
          if (*pProfile == avctx->profile)
          {
            supported = true;
            break;
          }
        }
      if (!supported)
        CLog::Log(LOGDEBUG, "DXVA: Unsupported profile {} for {}.", avctx->profile, mode.name);
    }
    if (!supported)
      continue;

    CLog::Log(LOGDEBUG, "DXVA: trying '{}'.", mode.name);
    for (unsigned j = 0; render_targets_dxgi[j]; ++j)
    {
      bool bHighBits =
          (avctx->codec_id == AV_CODEC_ID_HEVC && (avctx->sw_pix_fmt == AV_PIX_FMT_YUV420P10 ||
                                                   avctx->profile == FF_PROFILE_HEVC_MAIN_10)) ||
          (avctx->codec_id == AV_CODEC_ID_VP9 &&
           (avctx->sw_pix_fmt == AV_PIX_FMT_YUV420P10 || avctx->profile == FF_PROFILE_VP9_2)) ||
          (avctx->codec_id == AV_CODEC_ID_AV1 && avctx->sw_pix_fmt == AV_PIX_FMT_YUV420P10);

      if (bHighBits && render_targets_dxgi[j] < DXGI_FORMAT_P010)
        continue;

      BOOL format_supported = FALSE;
      HRESULT res = m_pD3D11Device->CheckVideoDecoderFormat(mode.guid, render_targets_dxgi[j], &format_supported);
      if (FAILED(res) || !format_supported)
      {
        CLog::Log(LOGINFO, "DXVA: Output format {} is not supported by '{}'",
                  DX::DXGIFormatToString(render_targets_dxgi[j]), mode.name);
        continue;
      }

      // check decoder config
      D3D11_VIDEO_DECODER_DESC checkFormat = {*mode.guid, static_cast<UINT>(avctx->coded_width),
                                              static_cast<UINT>(avctx->coded_height),
                                              render_targets_dxgi[j]};
      if (!GetConfig(checkFormat, config))
        continue;

      // config is found, update decoder description
      format.Guid = *mode.guid;
      format.OutputFormat = render_targets_dxgi[j];
      format.SampleWidth = avctx->coded_width;
      format.SampleHeight = avctx->coded_height;
      return true;
    }
  }

  return false;
}

bool CContext::GetConfig(const D3D11_VIDEO_DECODER_DESC &format, D3D11_VIDEO_DECODER_CONFIG &config) const
{
  // find what decode configs are available
  UINT cfg_count = 0;
  if (FAILED(m_pD3D11Device->GetVideoDecoderConfigCount(&format, &cfg_count)))
  {
    CLog::LogF(LOGINFO, "failed getting decoder configuration count.");
    return false;
  }
  if (!cfg_count)
  {
    CLog::LogF(LOGINFO, "no decoder configuration possible for {}x{} ({}).", format.SampleWidth,
               format.SampleHeight, DX::DXGIFormatToString(format.OutputFormat));
    return false;
  }

  config = {};
  const unsigned bitstream = 2; // ConfigBitstreamRaw = 2 is required for Poulsbo and handles skipping better with nVidia
  for (unsigned i = 0; i< cfg_count; i++)
  {
    D3D11_VIDEO_DECODER_CONFIG pConfig = {};
    if (FAILED(m_pD3D11Device->GetVideoDecoderConfig(&format, i, &pConfig)))
    {
      CLog::LogF(LOGINFO, "failed getting decoder configuration.");
      return false;
    }

    CLog::Log(LOGDEBUG, "DXVA: config {}: bitstream type {}{}.", i, pConfig.ConfigBitstreamRaw,
              IsEqualGUID(pConfig.guidConfigBitstreamEncryption, DXVA_NoEncrypt) ? ""
                                                                                 : ", encrypted");

    // select first available
    if (config.ConfigBitstreamRaw == 0 && pConfig.ConfigBitstreamRaw != 0)
      config = pConfig;
    // override with preferred if found
    if (config.ConfigBitstreamRaw != bitstream && pConfig.ConfigBitstreamRaw == bitstream)
      config = pConfig;
  }

  if (!config.ConfigBitstreamRaw)
  {
    CLog::Log(LOGDEBUG, "DXVA: failed to find a raw input bitstream.");
    return false;
  }

  return true;
}

bool CContext::CreateSurfaces(const D3D11_VIDEO_DECODER_DESC& format, uint32_t count,
                              uint32_t alignment, ID3D11VideoDecoderOutputView** surfaces,
                              HANDLE* pHandle, bool trueShared) const
{
  HRESULT hr = S_OK;
  ComPtr<ID3D11Device> pD3DDevice;
  ComPtr<ID3D11DeviceContext> pD3DDeviceContext;
  ComPtr<ID3D11DeviceContext1> pD3DDeviceContext1;

  m_pD3D11Context->GetDevice(&pD3DDevice);
  pD3DDevice->GetImmediateContext(&pD3DDeviceContext);
  pD3DDeviceContext.As(&pD3DDeviceContext1);

  CD3D11_TEXTURE2D_DESC texDesc(format.OutputFormat,
                                FFALIGN(format.SampleWidth, alignment),
                                FFALIGN(format.SampleHeight, alignment),
                                count, 1, D3D11_BIND_DECODER);
  UINT supported;
  if (SUCCEEDED(pD3DDevice->CheckFormatSupport(format.OutputFormat, &supported)) &&
      (supported & D3D11_FORMAT_SUPPORT_SHADER_SAMPLE))
  {
    texDesc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
  }
  if (trueShared)
  {
    texDesc.MiscFlags |= D3D11_RESOURCE_MISC_SHARED;
  }

  CLog::Log(LOGDEBUG, "DXVA: allocating {} surfaces with format {}.", count,
            DX::DXGIFormatToString(format.OutputFormat));

  ComPtr<ID3D11Texture2D> texture;
  if (FAILED(hr = pD3DDevice->CreateTexture2D(&texDesc, NULL, texture.GetAddressOf())))
  {
    CLog::LogF(LOGERROR, "failed creating decoder texture array. Error {}",
               DX::GetErrorDescription(hr));
    return false;
  }

  // acquire shared handle once
  if (trueShared && pHandle)
  {
    ComPtr<IDXGIResource> dxgiResource;
    if (FAILED(texture.As(&dxgiResource)) || FAILED(dxgiResource->GetSharedHandle(pHandle)))
    {
      CLog::LogF(LOGERROR, "unable to get shared handle for texture");
      *pHandle = INVALID_HANDLE_VALUE;
    }
  }

  D3D11_VIDEO_DECODER_OUTPUT_VIEW_DESC vdovDesc = {
    format.Guid,
    D3D11_VDOV_DIMENSION_TEXTURE2D,
    { 0 }
  };
  // For video views with YUV or YCbBr formats, ClearView doesn't
  // convert color values but assumes UINT texture format
  float clearColor[] = {0.f, 127.f, 127.f, 255.f}; // black color in YUV

  size_t i;
  for (i = 0; i < count; ++i)
  {
    vdovDesc.Texture2D.ArraySlice = D3D11CalcSubresource(0, i, texDesc.MipLevels);
    hr = m_pD3D11Device->CreateVideoDecoderOutputView(texture.Get(), &vdovDesc, &surfaces[i]);
    if (FAILED(hr))
    {
      CLog::LogF(LOGERROR, "failed creating surfaces. Error {}", DX::GetErrorDescription(hr));
      break;
    }
    if (pD3DDeviceContext1)
      pD3DDeviceContext1->ClearView(surfaces[i], clearColor, nullptr, 0);
  }

  if (FAILED(hr))
  {
    for (size_t j = 0; j < i && surfaces[j]; ++j)
    {
      surfaces[j]->Release();
      surfaces[j] = nullptr;
    };
  }

  return SUCCEEDED(hr);
}

bool CContext::CreateDecoder(const D3D11_VIDEO_DECODER_DESC &format, const D3D11_VIDEO_DECODER_CONFIG &config
                               , ID3D11VideoDecoder **decoder, ID3D11VideoContext **context)
{
  std::unique_lock<CCriticalSection> lock(m_section);

  int retry = 0;
  while (retry < 2)
  {
    if (!m_atiWorkaround || retry > 0)
    {
      ComPtr<ID3D11VideoDecoder> pDecoder;
      HRESULT res = m_pD3D11Device->CreateVideoDecoder(&format, &config, pDecoder.GetAddressOf());
      if (!FAILED(res))
      {
        *decoder = pDecoder.Detach();
        return SUCCEEDED(m_pD3D11Context.CopyTo(context));
      }
    }

    if (retry == 0)
    {
      CLog::LogF(LOGINFO, "hw may not support multiple decoders, releasing existing ones.");
      for (auto& m_decoder : m_decoders)
        m_decoder->CloseDXVADecoder();
    }
    retry++;
  }

  CLog::LogF(LOGERROR, "failed creating decoder.");
  return false;
}

bool CContext::IsValidDecoder(CDecoder* decoder)
{
  return std::find(m_decoders.begin(), m_decoders.end(), decoder) != m_decoders.end();
}

bool CContext::Check() const
{
  if (!m_sharingAllowed)
    return true;

  ComPtr<ID3D11Device> pDevice;
  m_pD3D11Context->GetDevice(&pDevice);

  return SUCCEEDED(pDevice->GetDeviceRemovedReason());
}

bool CContext::Reset()
{
  if (Check())
  {
    const DXGI_ADAPTER_DESC appDesc = DX::DeviceResources::Get()->GetAdapterDesc();

    ComPtr<IDXGIDevice> ctxDevice;
    ComPtr<IDXGIAdapter> ctxAdapter;
    if (SUCCEEDED(m_pD3D11Device.As(&ctxDevice)) && SUCCEEDED(ctxDevice->GetAdapter(&ctxAdapter)))
    {
      DXGI_ADAPTER_DESC ctxDesc = {};
      ctxAdapter->GetDesc(&ctxDesc);

      if (appDesc.AdapterLuid.HighPart == ctxDesc.AdapterLuid.HighPart &&
        appDesc.AdapterLuid.LowPart == ctxDesc.AdapterLuid.LowPart)
      {
        // 1. we have valid device
        // 2. we are on the same adapter
        // 3. don't reset context.
        return true;
      }
    }
  }
  DestroyContext();
  return CreateContext();
}

//-----------------------------------------------------------------------------
// DXVA::CVideoBuffer
//-----------------------------------------------------------------------------

DXVA::CVideoBuffer::CVideoBuffer(int id)
    : ::CVideoBuffer(id)
{
  m_pixFormat = AV_PIX_FMT_D3D11VA_VLD;
  m_pFrame = av_frame_alloc();
}

DXVA::CVideoBuffer::~CVideoBuffer()
{
  av_frame_free(&m_pFrame);
}

void DXVA::CVideoBuffer::Initialize(CDecoder* decoder)
{
  width = FFALIGN(decoder->m_format.SampleWidth, decoder->m_surface_alignment);
  height = FFALIGN(decoder->m_format.SampleHeight, decoder->m_surface_alignment);
  format = decoder->m_format.OutputFormat;
}

HRESULT DXVA::CVideoBuffer::GetResource(ID3D11Resource** ppResource)
{
  if (!view)
    return E_NOT_SET;

  view->GetResource(ppResource);
  return S_OK;
}

unsigned DXVA::CVideoBuffer::GetIdx()
{
  D3D11_VIDEO_DECODER_OUTPUT_VIEW_DESC vpivd = {};
  ComPtr<ID3D11VideoDecoderOutputView> pView = reinterpret_cast<ID3D11VideoDecoderOutputView*>(view);
  pView->GetDesc(&vpivd);

  return vpivd.Texture2D.ArraySlice;
}

void DXVA::CVideoBuffer::SetRef(AVFrame* frame)
{
  av_frame_unref(m_pFrame);
  av_frame_ref(m_pFrame, frame);
  view = reinterpret_cast<ID3D11View*>(frame->data[3]);
}

void DXVA::CVideoBuffer::Unref()
{
  view = nullptr;
  av_frame_unref(m_pFrame);
}

CVideoBufferShared::~CVideoBufferShared()
{
  if (m_handleFence != INVALID_HANDLE_VALUE)
    CloseHandle(m_handleFence);
}

HRESULT CVideoBufferShared::GetResource(ID3D11Resource** ppResource)
{
  HRESULT hr = S_OK;
  if (handle == INVALID_HANDLE_VALUE)
    return E_HANDLE;

  if (!m_sharedRes)
  {
    // open resource on app device
    ComPtr<ID3D11Device> pD3DDevice = DX::DeviceResources::Get()->GetD3DDevice();
    if (FAILED(hr = pD3DDevice->OpenSharedResource(handle, __uuidof(ID3D11Resource), &m_sharedRes)))
    {
      CLog::LogF(LOGDEBUG, "unable to open the shared resource, error description: {}",
                 DX::GetErrorDescription(hr));
      return hr;
    }

    // open fence on app device. Errors if any are not blocking, log only
    if (m_handleFence != INVALID_HANDLE_VALUE)
    {
      ComPtr<ID3D11DeviceContext1> context1 = DX::DeviceResources::Get()->GetImmediateContext();
      ComPtr<ID3D11Device5> device5;
      if (FAILED(hr = context1.As(&m_appContext4)))
      {
        CLog::LogF(LOGDEBUG, "ID3D11DeviceContext4 is not available, error description: {}",
                   DX::GetErrorDescription(hr));
      }
      else if (FAILED(hr = pD3DDevice.As(&device5)))
      {
        CLog::LogF(LOGDEBUG, "ID3D11Device5 is not available, error description: {}",
                   DX::GetErrorDescription(hr));
        m_appContext4 = nullptr;
      }
      else if (FAILED(hr = device5->OpenSharedFence(m_handleFence, IID_PPV_ARGS(&m_appFence))))
      {
        CLog::LogF(LOGDEBUG, "unable to open the shared fence, error description: {}",
                   DX::GetErrorDescription(hr));
        m_appContext4 = nullptr;
      }
    }
  }

  if (m_appFence)
  {
    // Make the GPU wait for the fence value that produced the picture
    if (FAILED(hr = m_appContext4->Wait(m_appFence.Get(), m_fenceValue)))
    {
      CLog::LogF(LOGDEBUG, "error waiting for the fence value, error description: {}",
                 DX::GetErrorDescription(hr));
    }
  }

  return m_sharedRes.CopyTo(ppResource);
}

void CVideoBufferShared::Initialize(CDecoder* decoder)
{
  CVideoBuffer::Initialize(decoder);

  if (handle == INVALID_HANDLE_VALUE)
  {
    handle = decoder->m_sharedHandle;
    if (DX::DeviceResources::Get()->UseFence())
      InitializeFence(decoder);
  }
  // Set the fence to wait until this picture is ready
  SetFence();
}

void CVideoBufferShared::InitializeFence(CDecoder* decoder)
{
  if (!decoder)
  {
    CLog::LogF(LOGERROR, "NULL decoder");
    return;
  }

  CLog::LogF(LOGDEBUG, "activating fence synchronization.");

  ComPtr<ID3D11Device> device;
  decoder->m_pD3D11Context->GetDevice(&device);
  ComPtr<ID3D11DeviceContext> immediateContext;
  device->GetImmediateContext(&immediateContext);
  ComPtr<ID3D11Device5> d3ddev5;

  HRESULT hr;
  if (FAILED(hr = immediateContext.As(&m_deviceContext4)))
  {
    CLog::LogF(LOGDEBUG, "ID3D11DeviceContext4 is not available, error description: {}",
               DX::GetErrorDescription(hr));
    goto error;
  }

  if (FAILED(hr = device.As(&d3ddev5)))
  {
    CLog::LogF(LOGDEBUG, "ID3D11Device5 is not available, error description: {}",
               DX::GetErrorDescription(hr));
    goto error;
  }

  if (FAILED(hr = d3ddev5->CreateFence(0, D3D11_FENCE_FLAG_SHARED, IID_PPV_ARGS(&m_fence))))
  {
    CLog::LogF(LOGDEBUG, "unable to create ID3D11Fence, error description: {}",
               DX::GetErrorDescription(hr));
    goto error;
  }

  if (FAILED(hr = m_fence->CreateSharedHandle(NULL, GENERIC_ALL, NULL, &m_handleFence)))
  {
    CLog::LogF(LOGDEBUG, "unable to create the shared handle of the fence, error description: {}",
               DX::GetErrorDescription(hr));
    goto error;
  }

  CLog::LogF(LOGINFO, "fence synchronization activated.");

  return;

error:
  CLog::LogF(LOGWARNING, "The dxva decoder will run without fence synchronization of the shared "
                         "surfaces with the main device.");

  m_deviceContext4 = nullptr;
  m_fence = nullptr;
  m_handleFence = INVALID_HANDLE_VALUE;
}

void CVideoBufferShared::SetFence()
{
  if (m_fence)
  {
    static UINT64 fenceValue = 0;
    // Not called from multiple threads, no synchronization needed to increment
    fenceValue++;

    m_fenceValue = fenceValue;

    m_deviceContext4->Signal(m_fence.Get(), m_fenceValue);
  }
}

void CVideoBufferCopy::Initialize(CDecoder* decoder)
{
  CVideoBuffer::Initialize(decoder);

  if (!m_copyRes)
  {
    ComPtr<ID3D11Device> pDevice;
    ComPtr<ID3D11DeviceContext> pDeviceContext;
    ComPtr<ID3D11Texture2D> pDecoderTexture;
    ComPtr<ID3D11Texture2D> pCopyTexture;
    ComPtr<IDXGIResource> pDXGIResource;
    ComPtr<ID3D11Resource> pResource;
    HRESULT hr;

    decoder->m_pD3D11Context->GetDevice(&pDevice);
    pDevice->GetImmediateContext(&pDeviceContext);

    if (FAILED(hr = CVideoBuffer::GetResource(&pResource)))
    {
      CLog::LogF(LOGDEBUG, "unable to get decoder resource. Error {}", DX::GetErrorDescription(hr));
      return;
    }

    if (FAILED(hr = pResource.As(&pDecoderTexture)))
    {
      CLog::LogF(LOGDEBUG, "unable to get decoder texture. Error {}", DX::GetErrorDescription(hr));
      return;
    }

    D3D11_TEXTURE2D_DESC desc;
    pDecoderTexture->GetDesc(&desc);
    desc.ArraySize = 1;
    desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    desc.MiscFlags = D3D11_RESOURCE_MISC_SHARED;

    if (FAILED(hr = pDevice->CreateTexture2D(&desc, nullptr, &pCopyTexture)))
    {
      CLog::LogF(LOGDEBUG, "unable to create copy texture. Error {}", DX::GetErrorDescription(hr));
      return;
    }
    if (FAILED(hr = pCopyTexture.As(&pDXGIResource)))
    {
      CLog::LogF(LOGDEBUG, "unable to get DXGI resource for copy texture. Error {}",
                 DX::GetErrorDescription(hr));
      return;
    }

    HANDLE shared_handle;
    if (FAILED(hr = pDXGIResource->GetSharedHandle(&shared_handle)))
    {
      CLog::LogF(LOGDEBUG, "unable to get shared handle. Error {}", DX::GetErrorDescription(hr));
      return;
    }

    handle = shared_handle;
    pCopyTexture.As(&m_copyRes);
    pResource.As(&m_pResource);
    pDeviceContext.As(&m_pDeviceContext);
  }

  if (m_copyRes)
  {
    // sends commands to GPU (ensures that the last decoded image is ready)
    m_pDeviceContext->Flush();

    // copy decoder surface on decoder device
    m_pDeviceContext->CopySubresourceRegion(m_copyRes.Get(), 0, 0, 0, 0, m_pResource.Get(),
                                            CVideoBuffer::GetIdx(), nullptr);

    if (decoder->m_DVDWorkaround) // DVDs menus/stills need extra Flush()
      m_pDeviceContext->Flush();
  }
}

//-----------------------------------------------------------------------------
// DXVA::CVideoBufferPool
//-----------------------------------------------------------------------------

CVideoBufferPool::CVideoBufferPool() = default;

CVideoBufferPool::~CVideoBufferPool()
{
  CLog::LogF(LOGDEBUG, "destructing buffer pool.");
  Reset();
}

::CVideoBuffer* CVideoBufferPool::Get()
{
  std::unique_lock<CCriticalSection> lock(m_section);

  CVideoBuffer* retPic;
  if (!m_freeOut.empty())
  {
    const size_t idx = m_freeOut.front();
    m_freeOut.pop_front();
    retPic = m_out[idx];
  }
  else
  {
    const size_t idx = m_out.size();
    retPic = CreateBuffer(idx);
    m_out.push_back(retPic);
  }

  retPic->Acquire(GetPtr());
  return retPic;
}

void CVideoBufferPool::Return(int id)
{
  std::unique_lock<CCriticalSection> lock(m_section);

  auto buf = m_out[id];
  buf->Unref();

  m_freeOut.push_back(id);
}

void CVideoBufferPool::AddView(ID3D11View* view)
{
  std::unique_lock<CCriticalSection> lock(m_section);
  const size_t idx = m_views.size();
  m_views.push_back(view);
  m_freeViews.push_back(idx);
}

bool CVideoBufferPool::IsValid(ID3D11View* view)
{
  std::unique_lock<CCriticalSection> lock(m_section);
  return std::find(m_views.begin(), m_views.end(), view) != m_views.end();
}

bool CVideoBufferPool::ReturnView(ID3D11View* view)
{
  std::unique_lock<CCriticalSection> lock(m_section);

  const auto it = std::find(m_views.begin(), m_views.end(), view);
  if (it == m_views.end())
    return false;

  const size_t idx = it - m_views.begin();
  m_freeViews.push_back(idx);
  return true;
}

ID3D11View* CVideoBufferPool::GetView()
{
  std::unique_lock<CCriticalSection> lock(m_section);

  if (!m_freeViews.empty())
  {
    const size_t idx = m_freeViews.front();
    m_freeViews.pop_front();

    return m_views[idx];
  }
  return nullptr;
}

void CVideoBufferPool::Reset()
{
  std::unique_lock<CCriticalSection> lock(m_section);

  for (auto v : m_views)
    if (v)
      v->Release();

  for (auto buf : m_out)
    delete buf;

  m_views.clear();
  m_freeViews.clear();
  m_out.clear();
  m_freeOut.clear();
}

size_t CVideoBufferPool::Size()
{
  std::unique_lock<CCriticalSection> lock(m_section);
  return m_views.size();
}

bool CVideoBufferPool::HasFree()
{
  std::unique_lock<CCriticalSection> lock(m_section);
  return !m_freeViews.empty();
}

//-----------------------------------------------------------------------------
// DXVA::CDecoder
//-----------------------------------------------------------------------------

IHardwareDecoder* CDecoder::Create(CDVDStreamInfo &hint, CProcessInfo &processInfo, AVPixelFormat fmt)
{
  if (Supports(fmt) && CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
                           CSettings::SETTING_VIDEOPLAYER_USEDXVA2))
    return new CDecoder(processInfo);

  return nullptr;
}

bool CDecoder::Register()
{
  CDVDFactoryCodec::RegisterHWAccel("dxva", Create);
  return true;
}

CDecoder::CDecoder(CProcessInfo& processInfo)
    : m_processInfo(processInfo)
{
  m_event.Set();
  m_avD3D11Context = av_d3d11va_alloc_context();
  m_avD3D11Context->cfg = reinterpret_cast<D3D11_VIDEO_DECODER_CONFIG*>(av_mallocz(sizeof(D3D11_VIDEO_DECODER_CONFIG)));
  m_avD3D11Context->surface = reinterpret_cast<ID3D11VideoDecoderOutputView**>(
      av_calloc(32, sizeof(ID3D11VideoDecoderOutputView*)));
  m_bufferPool.reset();

  DX::Windowing()->Register(this);
}

CDecoder::~CDecoder()
{
  CLog::LogF(LOGDEBUG, "destructing decoder, {}.", fmt::ptr(this));
  DX::Windowing()->Unregister(this);

  Close();
  av_freep(&m_avD3D11Context->surface);
  av_freep(&m_avD3D11Context->cfg);
  av_freep(&m_avD3D11Context);
}

void CDecoder::Close()
{
  std::unique_lock<CCriticalSection> lock(m_section);
  m_pD3D11Decoder = nullptr;
  m_pD3D11Context = nullptr;

  if (m_videoBuffer)
  {
    m_videoBuffer->Release();
    m_videoBuffer = nullptr;
  }
  m_format = {};
  m_sharedHandle = INVALID_HANDLE_VALUE;

  if (m_dxvaContext)
  {
    auto dxva_context = m_dxvaContext;
    CLog::LogF(LOGINFO, "closing decoder.");
    m_dxvaContext = nullptr;
    dxva_context->Release(this);
  }
}

static bool CheckH264L41(AVCodecContext* avctx)
{
  unsigned widthmbs = (avctx->coded_width + 15) / 16;   // width in macroblocks
  unsigned heightmbs = (avctx->coded_height + 15) / 16; // height in macroblocks
  unsigned maxdpbmbs = 32768; // Decoded Picture Buffer (DPB) capacity in macroblocks for L4.1

  return avctx->refs * widthmbs * heightmbs <= maxdpbmbs;
}

static bool IsL41LimitedATI()
{
  const DXGI_ADAPTER_DESC AIdentifier = DX::DeviceResources::Get()->GetAdapterDesc();

  if (AIdentifier.VendorId == PCIV_AMD)
  {
    for (unsigned idx = 0; UVDDeviceID[idx] != 0; idx++)
    {
      if (UVDDeviceID[idx] == AIdentifier.DeviceId)
        return true;
    }
  }
  return false;
}

static bool HasVP3WidthBug(AVCodecContext* avctx)
{
  // Some nVidia VP3 hardware cannot do certain macroblock widths
  const DXGI_ADAPTER_DESC AIdentifier = DX::DeviceResources::Get()->GetAdapterDesc();

  if (AIdentifier.VendorId == PCIV_NVIDIA &&
      !CDVDCodecUtils::IsVP3CompatibleWidth(avctx->coded_width))
  {
    // Find the card in a known list of problematic VP3 hardware
    for (unsigned idx = 0; VP3DeviceID[idx] != 0; idx++)
      if (VP3DeviceID[idx] == AIdentifier.DeviceId)
        return true;
  }
  return false;
}

static bool HasATIMP2Bug(AVCodecContext* avctx)
{
  const DXGI_ADAPTER_DESC AIdentifier = DX::DeviceResources::Get()->GetAdapterDesc();

  if (AIdentifier.VendorId != PCIV_AMD)
    return false;

  // AMD/ATI card doesn't like some SD MPEG2 content
  // here are params of these videos
  return avctx->height <= 576
      && avctx->colorspace == AVCOL_SPC_BT470BG
      && avctx->color_primaries == AVCOL_PRI_BT470BG
      && avctx->color_trc == AVCOL_TRC_GAMMA28;
}

static bool HasAMDH264SDiBug(AVCodecContext* avctx)
{
  const DXGI_ADAPTER_DESC AIdentifier = DX::DeviceResources::Get()->GetAdapterDesc();

  if (AIdentifier.VendorId != PCIV_AMD)
    return false;

  // AMD card has issues with SD H264 interlaced content
  return (avctx->width <= 720 && avctx->height <= 576 && avctx->codec_id == AV_CODEC_ID_H264 &&
          avctx->field_order != AV_FIELD_PROGRESSIVE);
}

static bool CheckCompatibility(AVCodecContext* avctx)
{
  if (avctx->codec_id == AV_CODEC_ID_MPEG2VIDEO && HasATIMP2Bug(avctx))
    return false;

  // The incompatibilities are all for H264
  if (avctx->codec_id != AV_CODEC_ID_H264)
    return true;

  // Macroblock width incompatibility
  if (HasVP3WidthBug(avctx))
  {
    CLog::Log(LOGWARNING,
              "DXVA: width {} is not supported with nVidia VP3 hardware. DXVA will not be used.",
              avctx->coded_width);
    return false;
  }

  // AMD H264 SD interlaced incompatibility
  if (HasAMDH264SDiBug(avctx))
  {
    CLog::Log(
        LOGWARNING,
        "DXVA: H264 SD interlaced has issues on AMD graphics hardware. DXVA will not be used.");
    return false;
  }

  // Check for hardware limited to H264 L4.1 (ie Bluray).

  // No advanced settings: autodetect.
  // The advanced setting lets the user override the autodetection (in case of false positive or negative)

  bool checkcompat;
  if (!CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_DXVACheckCompatibilityPresent)
    checkcompat = IsL41LimitedATI();  // ATI UVD and UVD+ cards can only do L4.1 - corresponds roughly to series 3xxx
  else
    checkcompat = CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_DXVACheckCompatibility;

  if (checkcompat && !CheckH264L41(avctx))
  {
    CLog::Log(LOGWARNING, "DXVA: compatibility check: video exceeds L4.1. DXVA will not be used.");
    return false;
  }

  return true;
}

bool CDecoder::Open(AVCodecContext* avctx, AVCodecContext* mainctx, enum AVPixelFormat fmt)
{
  if (!CheckCompatibility(avctx))
    return false;

  // DVDs menus/stills need extra Flush() after copy texture
  if (avctx->codec_id == AV_CODEC_ID_MPEG2VIDEO && avctx->height <= 576)
    m_DVDWorkaround = true;

  std::unique_lock<CCriticalSection> lock(m_section);
  Close();

  if (m_state == DXVA_LOST)
  {
    CLog::Log(LOGDEBUG, "DXVA: device is in lost state, we can't start.");
    return false;
  }

  CLog::Log(LOGDEBUG, "DXVA: open decoder.");
  m_dxvaContext = CContext::EnsureContext(this);
  if (!m_dxvaContext)
    return false;

  if (!m_dxvaContext->GetFormatAndConfig(avctx, m_format, *m_avD3D11Context->cfg))
  {
    CLog::Log(LOGDEBUG, "DXVA: unable to find an input/output format combination.");
    return false;
  }

  CLog::Log(LOGDEBUG, "DXVA: selected output format: {}.",
            DX::DXGIFormatToString(m_format.OutputFormat));
  CLog::Log(LOGDEBUG, "DXVA: source requires {} references.", avctx->refs);
  if (m_format.Guid == DXVADDI_Intel_ModeH264_E && avctx->refs > 11)
  {
    const dxva2_mode_t* mode = dxva2_find_mode(&m_format.Guid);
    CLog::Log(LOGWARNING, "DXVA: too many references {} for selected decoder '{}'.", avctx->refs,
              mode->name);
    return false;
  }

  if (6 > m_shared)
    m_shared = 6;

  m_refs = 2 + m_shared; // 1 decode + 1 safety + display
  m_surface_alignment = 16;

  const DXGI_ADAPTER_DESC ad = DX::DeviceResources::Get()->GetAdapterDesc();

  size_t videoMem = ad.SharedSystemMemory + ad.DedicatedVideoMemory + ad.DedicatedSystemMemory;
  CLog::LogF(LOGINFO, "Total video memory available is {} MB (dedicated = {} MB, shared = {} MB)",
             videoMem / MB, (ad.DedicatedVideoMemory + ad.DedicatedSystemMemory) / MB,
             ad.SharedSystemMemory / MB);

  switch (avctx->codec_id)
  {
  case AV_CODEC_ID_MPEG2VIDEO:
    /* decoding MPEG-2 requires additional alignment on some Intel GPUs,
       but it causes issues for H.264 on certain AMD GPUs..... */
    m_surface_alignment = 32;
    m_refs += 2;
    break;
  case AV_CODEC_ID_HEVC:
    /* the HEVC DXVA2 spec asks for 128 pixel aligned surfaces to ensure
       all coding features have enough room to work with */
    m_surface_alignment = 128;
    // a driver may use multi-thread decoding internally
    // on Xbox only add refs for <= Full HD due memory constraints (max 16 refs for 4K)
    if (CSysInfo::GetWindowsDeviceFamily() != CSysInfo::Xbox)
    {
      m_refs += CServiceBroker::GetCPUInfo()->GetCPUCount();
    }
    else
    {
      if (avctx->width <= 1920)
        m_refs += CServiceBroker::GetCPUInfo()->GetCPUCount() / 2;
    }
    // by specification hevc decoder can hold up to 8 unique refs
    // ffmpeg may report only 1 refs frame when is unknown or not present in headers
    m_refs += (avctx->refs > 1) ? avctx->refs : 8;
    break;
  case AV_CODEC_ID_H264:
    // by specification h264 decoder can hold up to 16 unique refs
    // but use 8 at least to avoid potential issues in some rare streams
    m_refs += std::max(8, avctx->refs ? avctx->refs : 16);
    break;
  case AV_CODEC_ID_VP9:
    m_refs += 8;
    break;
  case AV_CODEC_ID_AV1:
    // the AV1 DXVA2 spec asks for 128 pixel aligned surfaces
    m_surface_alignment = 128;
    // by specification AV1 decoder can hold up to 8 unique refs
    m_refs += 8;
    break;
  default:
    m_refs += 2;
  }

  if (avctx->active_thread_type & FF_THREAD_FRAME)
    m_refs += avctx->thread_count;

  // Limit decoder surfaces to 32 maximum in any case. Since with some 16 cores / 32 threads
  // new CPU's (Ryzen 5950x) this number may be higher than what the graphics card can handle.
  if (m_refs > 32)
  {
    CLog::LogF(LOGWARNING, "The number of decoder surfaces has been limited from {} to 32.", m_refs);
    m_refs = 32;
  }

  // Check if available video memory is sufficient for 4K decoding (is need ~3000 MB)
  if (avctx->width >= 3840 && m_refs > 16 && videoMem < (LIMIT_VIDEO_MEMORY_4K * MB))
  {
    CLog::LogF(LOGWARNING,
               "Current available video memory ({} MB) is insufficient 4K video decoding (DXVA2) "
               "using {} surfaces. Decoder surfaces has been limited to 16.", videoMem / MB, m_refs);
    m_refs = 16;
  }

  if (!OpenDecoder())
  {
    m_bufferPool.reset();
    return false;
  }

  avctx->get_buffer2 = FFGetBuffer;
  avctx->hwaccel_context = m_avD3D11Context;
  avctx->slice_flags = SLICE_FLAG_ALLOW_FIELD | SLICE_FLAG_CODED_ORDER;

  mainctx->get_buffer2 = FFGetBuffer;
  mainctx->hwaccel_context = m_avD3D11Context;
  mainctx->slice_flags = SLICE_FLAG_ALLOW_FIELD | SLICE_FLAG_CODED_ORDER;

  if (m_format.Guid == DXVADDI_Intel_ModeH264_E)
  {
#ifdef FF_DXVA2_WORKAROUND_INTEL_CLEARVIDEO
    m_avD3D11Context->workaround |= FF_DXVA2_WORKAROUND_INTEL_CLEARVIDEO;
#else
    CLog::Log(
        LOGWARNING,
        "DXVA: used Intel ClearVideo decoder, but no support workaround for it in libavcodec.");
#endif
  }
  else if (ad.VendorId == PCIV_AMD && IsL41LimitedATI())
  {
#ifdef FF_DXVA2_WORKAROUND_SCALING_LIST_ZIGZAG
    m_avD3D11Context->workaround |= FF_DXVA2_WORKAROUND_SCALING_LIST_ZIGZAG;
#else
    CLog::Log(LOGWARNING, "DXVA: video card with different scaling list zigzag order detected, but "
                          "no support in libavcodec.");
#endif
  }

  std::list<EINTERLACEMETHOD> deintMethods;
  deintMethods.push_back(VS_INTERLACEMETHOD_NONE);
  m_processInfo.UpdateDeinterlacingMethods(deintMethods);
  m_processInfo.SetDeinterlacingMethodDefault(VS_INTERLACEMETHOD_DXVA_AUTO);

  m_state = DXVA_OPEN;
  return true;
}

CDVDVideoCodec::VCReturn CDecoder::Decode(AVCodecContext* avctx, AVFrame* frame)
{
  std::unique_lock<CCriticalSection> lock(m_section);
  const CDVDVideoCodec::VCReturn result = Check(avctx);
  if (result != CDVDVideoCodec::VC_NONE)
    return result;

  if (frame)
  {
    if (m_bufferPool->IsValid(reinterpret_cast<ID3D11View*>(frame->data[3])))
    {
      if (m_videoBuffer)
        m_videoBuffer->Release();
      m_videoBuffer = reinterpret_cast<CVideoBuffer*>(m_bufferPool->Get());
      if (!m_videoBuffer)
      {
        CLog::Log(LOGERROR, "DXVA: ran out of buffers.");
        return CDVDVideoCodec::VC_ERROR;
      }
      m_videoBuffer->SetRef(frame);
      m_videoBuffer->Initialize(this);
      return CDVDVideoCodec::VC_PICTURE;
    }
    CLog::Log(LOGWARNING, "DXVA: ignoring invalid surface.");
    return CDVDVideoCodec::VC_BUFFER;
  }

  return CDVDVideoCodec::VC_BUFFER;
}

bool CDecoder::GetPicture(AVCodecContext* avctx, VideoPicture* picture)
{
  static_cast<ICallbackHWAccel*>(avctx->opaque)->GetPictureCommon(picture);
  std::unique_lock<CCriticalSection> lock(m_section);

  if (picture->videoBuffer)
    picture->videoBuffer->Release();
  picture->videoBuffer = m_videoBuffer;
  m_videoBuffer = nullptr;

  if (!m_dxvaContext->IsContextShared())
  {
    int queued, discard, free;
    m_processInfo.GetRenderBuffers(queued, discard, free);
    if (free > 1)
    {
      DX::Windowing()->RequestDecodingTime();
    }
    else
    {
      DX::Windowing()->ReleaseDecodingTime();
    }
  }
  return true;
}

void CDecoder::Reset()
{
  if (m_videoBuffer)
  {
    m_videoBuffer->Release();
    m_videoBuffer = nullptr;
  }
}

CDVDVideoCodec::VCReturn CDecoder::Check(AVCodecContext* avctx)
{
  std::unique_lock<CCriticalSection> lock(m_section);

  // we may not have a hw decoder on systems (AMD HD2xxx, HD3xxx) which are only capable
  // of opening a single decoder and VideoPlayer opened a new stream without having flushed
  // current one.
  if (!m_pD3D11Decoder)
    return CDVDVideoCodec::VC_BUFFER;

  // reset decoder if context detects an error on its device
  if (!m_dxvaContext->Check())
    m_state = DXVA_RESET;

  // app device is lost
  if (m_state == DXVA_LOST)
  {
    lock.unlock();
    // wait app device restoration
    m_event.Wait(2000ms);
    lock.lock();

    // still in lost state after 2sec
    if (m_state == DXVA_LOST)
    {
      Close();
      CLog::LogF(LOGERROR, "device didn't reset in reasonable time.");
      return CDVDVideoCodec::VC_ERROR;
    }
  }

  if (m_state != DXVA_OPEN)
  {
    // reset context in case of app device reset or context device error
    if (!m_dxvaContext->Reset())
    {
      CLog::LogF(LOGERROR, "context didn't reset.");
      return CDVDVideoCodec::VC_ERROR;
    }

    if (!Open(avctx, avctx, avctx->pix_fmt))
    {
      CLog::LogF(LOGERROR, "decoder was not able to reset.");
      Close();
      return CDVDVideoCodec::VC_ERROR;
    }
    // decoder re-opened
    m_state = DXVA_OPEN;
    return CDVDVideoCodec::VC_FLUSHED;
  }

  if (avctx->refs > m_refs)
  {
    CLog::LogF(LOGWARNING, "number of required reference frames increased, recreating decoder.");
    Close();
    return CDVDVideoCodec::VC_FLUSHED;
  }

  // Status reports are available only for the DXVA2_ModeH264 and DXVA2_ModeVC1 modes
  if (avctx->codec_id != AV_CODEC_ID_H264 && avctx->codec_id != AV_CODEC_ID_VC1 &&
      avctx->codec_id != AV_CODEC_ID_WMV3)
    return CDVDVideoCodec::VC_NONE;

#ifdef TARGET_WINDOWS_DESKTOP
  D3D11_VIDEO_DECODER_EXTENSION data = {};
  union {
    DXVA_Status_H264 h264;
    DXVA_Status_VC1 vc1;
  } status = {};

  /* I'm not sure, but MSDN says nothing about extensions functions in D3D11, try to using with same way as in DX9 */
  data.Function = DXVA_STATUS_REPORTING_FUNCTION;
  data.pPrivateOutputData = &status;
  data.PrivateOutputDataSize = avctx->codec_id == AV_CODEC_ID_H264 ? sizeof(DXVA_Status_H264) : sizeof(DXVA_Status_VC1);
  HRESULT hr;
  if (FAILED(hr = m_pD3D11Context->DecoderExtension(m_pD3D11Decoder.Get(), &data)))
  {
    CLog::Log(LOGWARNING, "DXVA: failed to get decoder status - {:#08X}.", hr);
    return CDVDVideoCodec::VC_ERROR;
  }

  if (avctx->codec_id == AV_CODEC_ID_H264)
  {
    if (status.h264.bStatus)
      CLog::Log(LOGWARNING, "DXVA: decoder problem of status {} with {}.", status.h264.bStatus,
                status.h264.bBufType);
  }
  else
  {
    if (status.vc1.bStatus)
      CLog::Log(LOGWARNING, "DXVA: decoder problem of status {} with {}.", status.vc1.bStatus,
                status.vc1.bBufType);
  }
#endif
  return CDVDVideoCodec::VC_NONE;
}

bool CDecoder::OpenDecoder()
{
  m_pD3D11Decoder = nullptr;
  m_pD3D11Context = nullptr;
  m_avD3D11Context->decoder = nullptr;
  m_avD3D11Context->video_context = nullptr;
  m_avD3D11Context->surface_count = m_refs;

  // use true shared buffers always on Intel or Nvidia/AMD with recent drivers
  const bool trueShared = DX::DeviceResources::Get()->IsDXVA2SharedDecoderSurfaces();

  if (!m_dxvaContext->CreateSurfaces(m_format, m_avD3D11Context->surface_count, m_surface_alignment,
                                     m_avD3D11Context->surface, &m_sharedHandle, trueShared))
    return false;

  if (!m_dxvaContext->CreateDecoder(m_format, *m_avD3D11Context->cfg, m_pD3D11Decoder.GetAddressOf(),
                                     m_pD3D11Context.GetAddressOf()))
    return false;

  if (m_dxvaContext->IsContextShared())
  {
    if (trueShared)
      m_bufferPool = std::make_shared<CVideoBufferPoolTyped<CVideoBufferShared>>();
    else
      m_bufferPool = std::make_shared<CVideoBufferPoolTyped<CVideoBufferCopy>>();
  }
  else
    m_bufferPool = std::make_shared<CVideoBufferPoolTyped<CVideoBuffer>>();

  for (unsigned i = 0; i < m_avD3D11Context->surface_count; i++)
    m_bufferPool->AddView(m_avD3D11Context->surface[i]);

  m_avD3D11Context->decoder = m_pD3D11Decoder.Get();
  m_avD3D11Context->video_context = m_pD3D11Context.Get();

  return true;
}

bool CDecoder::Supports(enum AVPixelFormat fmt)
{
  return fmt == AV_PIX_FMT_D3D11VA_VLD;
}

void CDecoder::FFReleaseBuffer(void* opaque, uint8_t* data)
{
  auto decoder = static_cast<CDecoder*>(opaque);
  decoder->ReleaseBuffer(data);
}

void CDecoder::ReleaseBuffer(uint8_t* data)
{
  const auto view = reinterpret_cast<ID3D11VideoDecoderOutputView*>(data);
  if (!m_bufferPool->ReturnView(view))
  {
    CLog::LogF(LOGWARNING, "return of invalid surface.");
  }

  Release();
}

int CDecoder::FFGetBuffer(AVCodecContext* avctx, AVFrame* pic, int flags)
{
  auto* cb = static_cast<ICallbackHWAccel*>(avctx->opaque);
  auto decoder = dynamic_cast<CDecoder*>(cb->GetHWAccel());

  return decoder->GetBuffer(avctx, pic);
}

int CDecoder::GetBuffer(AVCodecContext* avctx, AVFrame* pic)
{
  if (!m_pD3D11Decoder)
    return -1;

  ID3D11View* view = m_bufferPool->GetView();
  if (view == nullptr)
  {
    CLog::LogF(LOGERROR, "no surface available.");
    m_state = DXVA_RESET;
    return -1;
  }

  for (unsigned i = 0; i < 4; i++)
  {
    pic->data[i] = nullptr;
    pic->linesize[i] = 0;
  }

  pic->data[0] = reinterpret_cast<uint8_t*>(view);
  pic->data[3] = reinterpret_cast<uint8_t*>(view);
  AVBufferRef* buffer = av_buffer_create(pic->data[3], 0, CDecoder::FFReleaseBuffer, this, 0);
  if (!buffer)
  {
    CLog::LogF(LOGERROR, "error creating buffer.");
    return -1;
  }
  pic->buf[0] = buffer;

#if LIBAVCODEC_VERSION_MAJOR < 60
  pic->reordered_opaque = avctx->reordered_opaque;
#endif

  Acquire();

  return 0;
}

unsigned CDecoder::GetAllowedReferences()
{
  return m_shared;
}

void CDecoder::CloseDXVADecoder()
{
  std::unique_lock<CCriticalSection> lock(m_section);
  m_pD3D11Decoder = nullptr;
}
