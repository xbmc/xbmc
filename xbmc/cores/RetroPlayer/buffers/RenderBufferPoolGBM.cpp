/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RenderBufferPoolGBM.h"
#include "RenderBufferGBM.h"
#include "cores/RetroPlayer/rendering/VideoRenderers/RPRendererGBM.h"

#include <drm_fourcc.h>

using namespace KODI;
using namespace RETRO;

CRenderBufferPoolGBM::CRenderBufferPoolGBM(CRenderContext &context)
  : m_context(context)
{
}

bool CRenderBufferPoolGBM::IsCompatible(const CRenderVideoSettings &renderSettings) const
{
  if (!CRPRendererGBM::SupportsScalingMethod(renderSettings.GetScalingMethod()))
    return false;

  return true;
}


IRenderBuffer *CRenderBufferPoolGBM::CreateRenderBuffer(void *header /* = nullptr */)
{
  return new CRenderBufferGBM(m_context,
                              m_fourcc);
}

bool CRenderBufferPoolGBM::ConfigureInternal()
{
  switch (m_format)
  {
    case AV_PIX_FMT_0RGB32:
    {
      m_fourcc = DRM_FORMAT_ARGB8888;
      return true;
    }
    case AV_PIX_FMT_RGB555:
    case AV_PIX_FMT_RGB565:
    {
      m_fourcc = DRM_FORMAT_RGB565;
      return true;
    }
    default:
      break; // we shouldn't even get this far if we are given an unsupported pixel format
  }

  return false;
}
