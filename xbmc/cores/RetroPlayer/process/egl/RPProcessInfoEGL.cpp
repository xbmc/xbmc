/*
 *  Copyright (C) 2017-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RPProcessInfoEGL.h"

#include <EGL/egl.h>

using namespace KODI;
using namespace RETRO;

CRPProcessInfoEGL::CRPProcessInfoEGL(std::string platformName)
  : CRPProcessInfo(std::move(platformName))
{
}

HwProcedureAddress CRPProcessInfoEGL::GetHwProcedureAddress(const char* symbol)
{
  return static_cast<HwProcedureAddress>(eglGetProcAddress(symbol));
}
