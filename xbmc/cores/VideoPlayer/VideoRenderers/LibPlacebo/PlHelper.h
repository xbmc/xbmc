/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "libplacebo/log.h"
#include "libplacebo/renderer.h"
#include "libplacebo/d3d11.h"
#include <libplacebo/options.h>
#include "libplacebo/utils/frame_queue.h"
#include "libplacebo/utils/upload.h"
#include "libplacebo/colorspace.h"

extern "C" {
#include <libavutil/pixfmt.h>
#include <libavutil/dovi_meta.h>
}

#include <string>
#include <vector>
#include <strmif.h>
#include <d3d9types.h>
#include <dxva2api.h>
#include <memory>
#include <libavutil/mastering_display_metadata.h>
#include <libavutil/hdr_dynamic_metadata.h>

#define MAX_FRAME_PASSES 256
#define MAX_BLEND_PASSES 8
#define MAX_BLEND_FRAMES 8
namespace PL
{
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
  };
}
