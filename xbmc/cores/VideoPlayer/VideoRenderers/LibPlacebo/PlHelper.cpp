/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PlHelper.h"
#include <mfobjects.h>

#include "rendering/dx/RenderContext.h"


static void pl_log_cb(void*, enum pl_log_level level, const char* msg)
{
  switch (level) {
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
}
void PL::PLInstance::Reset()
{ 
  m_plSwapchain = nullptr;
  m_plLog = nullptr;
  m_plD3d11 = nullptr;
  m_plRenderer = nullptr;
}

void PL::PLInstance::LogCurrent()
{
  if (CurrentPrim == PL_COLOR_PRIM_COUNT)
    CurrentPrim = 0;
  if (CurrentMatrix == PL_COLOR_SYSTEM_COUNT)
    CurrentMatrix = 0;
  if (Currenttransfer == PL_COLOR_TRC_COUNT)
    Currenttransfer = 0;
  std::string sSys =  pl_color_system_name((pl_color_system)CurrentMatrix);
  std::string sTrans = pl_color_transfer_name((pl_color_transfer)Currenttransfer);
  std::string sPrim = pl_color_primaries_name((pl_color_primaries)CurrentPrim);
  CLog::Log(LOGINFO, "LibPlaceboCurrent Color Settings: Primaries: {}", sPrim.c_str());
  CLog::Log(LOGINFO, "LibPlaceboCurrent Color Settings: Transfer: {}", sTrans.c_str());
  CLog::Log(LOGINFO, "LibPlaceboCurrent Color Settings: Matrix: {}", sSys.c_str());
}

void PL::PLInstance::fill_d3d_format(pl_d3d_format* info, DXGI_FORMAT format)
{
  
  memset(info, 0, sizeof(pl_d3d_format));
  switch (format) {
  case DXGI_FORMAT_NV12:
    info->bits.color_depth = 8;
    info->bits.sample_depth = 8;
    info->bits.bit_shift = 0;
    info->planes[0] = DXGI_FORMAT_R8_UNORM;      // Y plane
    info->planes[1] = DXGI_FORMAT_R8G8_UNORM;    // UV plane
    info->component_mapping[0][0] = PL_CHANNEL_Y;
    info->component_mapping[1][0] = PL_CHANNEL_U;
    info->component_mapping[1][1] = PL_CHANNEL_V;
    info->components[0] = 1;
    info->components[1] = 2;
    info->width_div[0] = 1;   // full width
    info->height_div[0] = 1;  // full height
    info->width_div[1] = 2;   // half width
    info->height_div[1] = 2;  // half height
    info->num_planes = 2;
    strcpy(info->description, "nv12");
    break;

  case DXGI_FORMAT_P010:
    info->bits.color_depth = 10;
    info->bits.sample_depth = 16;
    info->bits.bit_shift = 6;
    info->planes[0] = DXGI_FORMAT_R16_UNORM;     // Y plane
    info->planes[1] = DXGI_FORMAT_R16G16_UNORM;  // UV plane
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
