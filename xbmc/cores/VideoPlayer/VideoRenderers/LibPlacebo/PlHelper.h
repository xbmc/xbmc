/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "libplacebo/colorspace.h"
#include "libplacebo/d3d11.h"
#include "libplacebo/log.h"
#include "libplacebo/renderer.h"
#include "libplacebo/utils/frame_queue.h"
#include "libplacebo/utils/upload.h"

#include <libplacebo/options.h>

extern "C"
{
#include <libavutil/dovi_meta.h>
#include <libavutil/pixfmt.h>
}

#include <memory>
#include <string>
#include <vector>

#include <d3d9types.h>
#include <dxva2api.h>
#include <libavutil/hdr_dynamic_metadata.h>
#include <libavutil/mastering_display_metadata.h>
#include <strmif.h>

#define MAX_FRAME_PASSES 256
#define MAX_BLEND_PASSES 8
#define MAX_BLEND_FRAMES 8
namespace PL
{
typedef struct pl_d3d_format
{
  pl_bit_encoding bits; // per picture
  DXGI_FORMAT planes[4]; // DXGI format per plane
  int components[4]; // number of components per plane
  pl_channel component_mapping[4][4];
  int width_div[4]; // divide full width by this for each plane
  int height_div[4]; // divide full height by this for each plane
  char description[16]; // short description
  int num_planes; // actual number of planes used
} pl_d3d_format;

class PLInstance
{
public:
  static std::shared_ptr<PLInstance> Get();
  PLInstance();

  virtual ~PLInstance();
  bool Init();
  void Reset();

  pl_d3d11 GetD3d11() { return m_plD3d11; }
  pl_swapchain GetSwapchain() { return m_plSwapchain; }
  pl_renderer GetRenderer() { return m_plRenderer; }
  pl_gpu GetGpu() { return m_plD3d11->gpu; }

  pl_log m_plLog;
  pl_d3d11 m_plD3d11;
  pl_swapchain m_plSwapchain;
  pl_renderer m_plRenderer;
  int CurrentPrim;
  int Currenttransfer;
  int CurrentMatrix;
  void LogCurrent();
  void fill_d3d_format(pl_d3d_format* info, DXGI_FORMAT format);
};
} // namespace PL
