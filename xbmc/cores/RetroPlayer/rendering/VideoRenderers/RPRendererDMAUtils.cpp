/*
 *  Copyright (C) 2017-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RPRendererDMAUtils.h"

#if !defined(HAS_GLES)
#include "RPRendererDMAOpenGL.h"
#else
#include "RPRendererDMAOpenGLES.h"
#endif

using namespace KODI;
using namespace RETRO;

bool CRPRendererDMAUtils::SupportsScalingMethod(SCALINGMETHOD method)
{
#if !defined(HAS_GLES)
  return CRPRendererDMAOpenGL::SupportsScalingMethod(method);
#else
  return CRPRendererDMAOpenGLES::SupportsScalingMethod(method);
#endif
}
