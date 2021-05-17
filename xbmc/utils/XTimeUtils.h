/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>

#if !defined(TARGET_WINDOWS)
#include "PlatformDefs.h"
#else
// This is needed, a forward declaration of FILETIME
// breaks everything
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#endif

namespace KODI
{
namespace TIME
{

void Sleep(uint32_t milliSeconds);

} // namespace TIME
} // namespace KODI
