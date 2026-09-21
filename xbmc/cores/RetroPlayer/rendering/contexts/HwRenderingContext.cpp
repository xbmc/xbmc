/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "IHwRenderingContext.h"
#include "cores/RetroPlayer/rendering/RenderContext.h"
#if defined(TARGET_DARWIN_OSX)
#include "HwRenderingContextOSX.h"
#include "windowing/osx/WinSystemOSX.h"
#else
#include "HwRenderingContextEGL.h"
#endif

std::unique_ptr<KODI::RETRO::IHwRenderingContext> KODI::RETRO::CreateHwRenderingContext(
    CRenderContext& context)
{
#if defined(TARGET_DARWIN_OSX)
  auto* windowing = dynamic_cast<CWinSystemOSX*>(context.Windowing());
  return CreateHwRenderingContextOSX(windowing ? windowing->GetNSOpenGLContext() : nullptr);
#else
  return CreateHwRenderingContextEGL(context);
#endif
}
