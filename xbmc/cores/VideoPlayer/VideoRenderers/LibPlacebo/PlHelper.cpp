/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PlHelper.h"

#include "rendering/dx/RenderContext.h"

#include <mfobjects.h>

static void pl_log_cb(void*, enum pl_log_level level, const char* msg)
{
  switch (level)
  {
    case PL_LOG_FATAL:
      CLog::Log(LOGFATAL, "libPlacebo Fatal: {}", msg);
      break;
    case PL_LOG_ERR:
      CLog::Log(LOGERROR, "libPlacebo Error: {}", msg);

      break;
    case PL_LOG_WARN:
      CLog::Log(LOGWARNING, "libPlacebo Warning: {}", msg);
      break;
    case PL_LOG_INFO:
      CLog::Log(LOGINFO, "libPlacebo Info: {}", msg);
      break;
    case PL_LOG_DEBUG:
      CLog::Log(LOGDEBUG, "libPlacebo Debug: {}", msg);
      break;
    case PL_LOG_NONE:
    case PL_LOG_TRACE:
      CLog::Log(LOGNONE, "libPlacebo Trace: {}", msg);
      break;
  }
}

std::shared_ptr<PL::PLInstance> PL::PLInstance::Get()
{
  static std::shared_ptr<PLInstance> sPLResources(new PLInstance);
  return sPLResources;
}

PL::PLInstance::PLInstance()
  : m_plD3d11(nullptr),
    m_plLog(nullptr),
    m_plRenderer(nullptr),
    m_plSwapchain(nullptr),
    CurrentPrim(0),
    CurrentMatrix(0),
    Currenttransfer(0)
{
}

PL::PLInstance::~PLInstance() = default;

bool PL::PLInstance::Init()
{
  pl_log_params log_param{};
  log_param.log_cb = pl_log_cb;
  log_param.log_level = PL_LOG_DEBUG;
  m_plLog = pl_log_create(PL_API_VER, &log_param);
  //d3d device
  pl_d3d11_params d3d_param{};
  d3d_param.device = DX::DeviceResources::Get()->GetD3DDevice();
  d3d_param.adapter = DX::DeviceResources::Get()->GetAdapter();
  d3d_param.adapter_luid = DX::DeviceResources::Get()->GetAdapterDesc().AdapterLuid;
  d3d_param.allow_software = true;
  d3d_param.force_software = false;
  d3d_param.no_compute = false;
  d3d_param.debug = false;
  //libplacebo dont touch it if 0
  d3d_param.max_frame_latency = 0;
  //this was added to libplacebo to handle multi threaded rendering for kodi

  m_plD3d11 = pl_d3d11_create(m_plLog, &d3d_param);
  if (!m_plD3d11)
    return false;
  //swap chain
  pl_d3d11_swapchain_params swapchain_param{};
  swapchain_param.swapchain = DX::DeviceResources::Get()->GetSwapChain();
  //everything else is not used
  m_plSwapchain = pl_d3d11_create_swapchain(m_plD3d11, &swapchain_param);
  if (!m_plSwapchain)
    return false;

  m_plRenderer = pl_renderer_create(m_plLog, m_plD3d11->gpu);
  m_isInitialized = true;
  return true;

}
void PL::PLInstance::Reset()
{
  m_plSwapchain = nullptr;
  m_plLog = nullptr;
  m_plD3d11 = nullptr;
  m_plRenderer = nullptr;
  if (m_isInitialized)
  {
    pl_renderer_destroy(&m_plRenderer);
    pl_swapchain_destroy(&m_plSwapchain);
    pl_d3d11_destroy(&m_plD3d11);
    pl_log_destroy(&m_plLog);
    m_isInitialized = false;
  }
}

void PL::PLInstance::LogCurrent()
{
  if (CurrentPrim == PL_COLOR_PRIM_COUNT)
    CurrentPrim = 0;
  if (CurrentMatrix == PL_COLOR_SYSTEM_COUNT)
    CurrentMatrix = 0;
  if (Currenttransfer == PL_COLOR_TRC_COUNT)
    Currenttransfer = 0;
  std::string sSys = pl_color_system_name((pl_color_system)CurrentMatrix);
  std::string sTrans = pl_color_transfer_name((pl_color_transfer)Currenttransfer);
  std::string sPrim = pl_color_primaries_name((pl_color_primaries)CurrentPrim);
  CLog::Log(LOGINFO, "LibPlaceboCurrent Color Settings: Primaries: {}", sPrim.c_str());
  CLog::Log(LOGINFO, "LibPlaceboCurrent Color Settings: Transfer: {}", sTrans.c_str());
  CLog::Log(LOGINFO, "LibPlaceboCurrent Color Settings: Matrix: {}", sSys.c_str());
}

const char* PL::PLInstance::pl_color_primaries_short_names[PL_COLOR_PRIM_COUNT] = {
    "Auto",
    "BT.601 NTSC",
    "BT.601 PAL",
    "BT.709",
    "BT.470 M",
    "EBU Tech.",
    "BT.2020",
    "Apple RGB",
    "Adobe RGB (1998)",
    "ProPhoto RGB (ROMM)",
    "CIE 1931 RGB primaries",
    "DCI-P3",
    "DCI-P3 with D65",
    "Panasonic V-Gamut",
    "Sony S-Gamut",
    "Traditional film primaries with Illuminant C",
    "ACES Primaries #0",
    "ACES Primaries #1"
};

const char* PL::PLInstance::pl_color_primaries_short_name(pl_color_primaries prim) {
  assert(prim >= 0 && prim < PL_COLOR_PRIM_COUNT);
  return pl_color_primaries_short_names[prim];
}

const char* pl_color_transfer_short_names[PL_COLOR_TRC_COUNT] = {
    "Auto",                                          // PL_COLOR_TRC_UNKNOWN
    "BT.1886",                   // PL_COLOR_TRC_BT_1886
    "IEC 61966-2-4",                          // PL_COLOR_TRC_SRGB
    "Linear light content",                                        // PL_COLOR_TRC_LINEAR
    "Pure power gamma 1.8",                                        // PL_COLOR_TRC_GAMMA18
    "Pure power gamma 2.0",                                        // PL_COLOR_TRC_GAMMA20
    "Pure power gamma 2.2",                                        // PL_COLOR_TRC_GAMMA22
    "Pure power gamma 2.4",                                        // PL_COLOR_TRC_GAMMA24
    "Pure power gamma 2.6",                                        // PL_COLOR_TRC_GAMMA26
    "Pure power gamma 2.8",                                        // PL_COLOR_TRC_GAMMA28
    "ProPhoto RGB (ROMM)",                                         // PL_COLOR_TRC_PRO_PHOTO
    "Digital Cinema Distribution",                    // PL_COLOR_TRC_ST428
    "BT.2100 PQ",   // PL_COLOR_TRC_PQ
    "BT.2100 HLG",      // PL_COLOR_TRC_HLG
    "Panasonic V-Log",                                   // PL_COLOR_TRC_V_LOG
    "Sony S-Log1",                                                 // PL_COLOR_TRC_S_LOG1
    "Sony S-Log2"                                                  // PL_COLOR_TRC_S_LOG2
};

const char* PL::PLInstance::pl_color_transfer_short_name(pl_color_transfer trc) {
  assert(trc >= 0 && trc < PL_COLOR_TRC_COUNT);
  return pl_color_transfer_short_names[trc];
}

const char* pl_color_system_short_names[PL_COLOR_SYSTEM_COUNT] = {
    "Auto",                                // PL_COLOR_SYSTEM_UNKNOWN
    "BT.601 (SD)",                        // PL_COLOR_SYSTEM_BT_601
    "BT.709 (HD)",                        // PL_COLOR_SYSTEM_BT_709
    "SMPTE-240M",                                    // PL_COLOR_SYSTEM_SMPTE_240M
    "BT.2020 N-C",   // PL_COLOR_SYSTEM_BT_2020_NC
    "BT.2020 C",       // PL_COLOR_SYSTEM_BT_2020_C
    "BT.2100 PQ",           // PL_COLOR_SYSTEM_BT_2100_PQ
    "BT.2100 HLG",          // PL_COLOR_SYSTEM_BT_2100_HLG
    "Dolby Vision",             // PL_COLOR_SYSTEM_DOLBYVISION
    "YCgCo",                      // PL_COLOR_SYSTEM_YCGCO
    "RGB",                           // PL_COLOR_SYSTEM_RGB
    "XYZ"       // PL_COLOR_SYSTEM_XYZ
};

const char* PL::PLInstance::pl_color_system_short_name(pl_color_system sys) {
  assert(sys >= 0 && sys < PL_COLOR_SYSTEM_COUNT);
  return pl_color_system_short_names[sys];
}



void PL::PLInstance::fill_d3d_format(pl_d3d_format* info, DXGI_FORMAT format)
{
  memset(info, 0, sizeof(pl_d3d_format));
  
  switch (format)
  {
  case DXGI_FORMAT_R10G10B10A2_UNORM:
    info->bits.color_depth = 10;
    info->bits.sample_depth = 10;
    info->bits.bit_shift = 0;
    info->planes[0] = DXGI_FORMAT_R10G10B10A2_UNORM;
    info->component_mapping[0][0] = PL_CHANNEL_R;
    info->component_mapping[0][1] = PL_CHANNEL_G;
    info->component_mapping[0][2] = PL_CHANNEL_B;
    info->component_mapping[0][3] = PL_CHANNEL_A;
    info->components[0] = 4;
    info->width_div[0] = 1;
    info->height_div[0] = 1;
    info->num_planes = 1;
    strcpy(info->description, "rgba10");
    break;

  case DXGI_FORMAT_R8G8B8A8_UNORM:
    info->bits.color_depth = 8;
    info->bits.sample_depth = 8;
    info->bits.bit_shift = 0;
    info->planes[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    info->component_mapping[0][0] = PL_CHANNEL_R;
    info->component_mapping[0][1] = PL_CHANNEL_G;
    info->component_mapping[0][2] = PL_CHANNEL_B;
    info->component_mapping[0][3] = PL_CHANNEL_A;
    info->components[0] = 4;
    info->width_div[0] = 1;
    info->height_div[0] = 1;
    info->num_planes = 1;
    strcpy(info->description, "rgba");
    break;

  case DXGI_FORMAT_B8G8R8A8_UNORM:
    info->bits.color_depth = 8;
    info->bits.sample_depth = 8;
    info->bits.bit_shift = 0;
    info->planes[0] = DXGI_FORMAT_B8G8R8A8_UNORM;
    info->component_mapping[0][0] = PL_CHANNEL_B;
    info->component_mapping[0][1] = PL_CHANNEL_G;
    info->component_mapping[0][2] = PL_CHANNEL_R;
    info->component_mapping[0][3] = PL_CHANNEL_A;
    info->components[0] = 4;
    info->width_div[0] = 1;
    info->height_div[0] = 1;
    info->num_planes = 1;
    strcpy(info->description, "bgra");
    break;

  case DXGI_FORMAT_NV12:
    info->bits.color_depth = 8;
    info->bits.sample_depth = 8;
    info->bits.bit_shift = 0;
    info->planes[0] = DXGI_FORMAT_R8_UNORM; // Y plane
    info->planes[1] = DXGI_FORMAT_R8G8_UNORM; // UV plane
    info->component_mapping[0][0] = PL_CHANNEL_Y;
    info->component_mapping[1][0] = PL_CHANNEL_U;
    info->component_mapping[1][1] = PL_CHANNEL_V;
    info->components[0] = 1;
    info->components[1] = 2;
    info->width_div[0] = 1; // full width
    info->height_div[0] = 1; // full height
    info->width_div[1] = 2; // half width
    info->height_div[1] = 2; // half height
    info->num_planes = 2;
    strcpy(info->description, "nv12");
    break;

  case DXGI_FORMAT_P010:
    info->bits.color_depth = 10;
    info->bits.sample_depth = 16;
    info->bits.bit_shift = 6;
    info->planes[0] = DXGI_FORMAT_R16_UNORM; // Y plane
    info->planes[1] = DXGI_FORMAT_R16G16_UNORM; // UV plane
    info->component_mapping[0][0] = PL_CHANNEL_Y;
    info->component_mapping[1][0] = PL_CHANNEL_U;
    info->component_mapping[1][1] = PL_CHANNEL_V;
    info->components[0] = 1;
    info->components[1] = 2;
    info->width_div[0] = 1;
    info->height_div[0] = 1;
    info->width_div[1] = 2;
    info->height_div[1] = 2;
    info->num_planes = 2;
    strcpy(info->description, "p010");
    break;

  case DXGI_FORMAT_P016:
    info->bits.color_depth = 16;
    info->bits.sample_depth = 16;
    info->bits.bit_shift = 0;
    info->planes[0] = DXGI_FORMAT_R16_UNORM;
    info->planes[1] = DXGI_FORMAT_R16G16_UNORM;
    info->component_mapping[0][0] = PL_CHANNEL_Y;
    info->component_mapping[1][0] = PL_CHANNEL_U;
    info->component_mapping[1][1] = PL_CHANNEL_V;
    info->components[0] = 1;
    info->components[1] = 2;
    info->width_div[0] = 1;
    info->height_div[0] = 1;
    info->width_div[1] = 2;
    info->height_div[1] = 2;
    info->num_planes = 2;
    strcpy(info->description, "p016");
    break;

  case DXGI_FORMAT_YUY2:
    info->bits.color_depth = 8;
    info->bits.sample_depth = 16; // packed 2 bytes per component pair
    info->bits.bit_shift = 0;
    info->planes[0] = DXGI_FORMAT_R8G8B8A8_UNORM; // pseudo-plane
    info->component_mapping[0][0] = PL_CHANNEL_R;
    info->component_mapping[0][1] = PL_CHANNEL_G;
    info->component_mapping[0][2] = PL_CHANNEL_B;
    info->component_mapping[0][3] = PL_CHANNEL_A;
    info->width_div[0] = 1;
    info->height_div[0] = 1;
    info->num_planes = 1;
    strcpy(info->description, "yuy2");
    break;

  default:
    info->num_planes = 0;
    strcpy(info->description, "unknown");
    break;
  }
}

/*Settings conversion*/
const pl_tone_map_function* PL::PLInstance::GetToneMappingFunction(pl_tone_mapping method)
{
  switch (method)
  {
  case TONE_MAPPING_AUTO:      return &pl_tone_map_auto;
  case TONE_MAPPING_CLIP:      return &pl_tone_map_clip;
  case TONE_MAPPING_MOBIUS:    return &pl_tone_map_mobius;
  case TONE_MAPPING_REINHARD:  return &pl_tone_map_reinhard;
  case TONE_MAPPING_HABLE:     return &pl_tone_map_hable;
  case TONE_MAPPING_GAMMA:     return &pl_tone_map_gamma;
  case TONE_MAPPING_LINEAR:    return &pl_tone_map_linear;
  case TONE_MAPPING_SPLINE:    return &pl_tone_map_spline;
  case TONE_MAPPING_BT_2390:   return &pl_tone_map_bt2390;
  case TONE_MAPPING_BT_2446A:  return &pl_tone_map_bt2446a;
  case TONE_MAPPING_ST2094_40: return &pl_tone_map_st2094_40;
  case TONE_MAPPING_ST2094_10: return &pl_tone_map_st2094_10;
  default:
    return nullptr;
  }
}