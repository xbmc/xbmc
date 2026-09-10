/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "HwRenderingContextEGL.h"
#include "IHwRenderingContext.h"
#include "cores/RetroPlayer/rendering/RenderContext.h"

std::unique_ptr<KODI::RETRO::IHwRenderingContext> KODI::RETRO::CreateHwRenderingContext(
    CRenderContext& context)
{
  return CreateHwRenderingContextEGL(context);
}
