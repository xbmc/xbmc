/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#ifdef TARGET_WINDOWS
#include "DllPaths_win32.h"
#elif defined (TARGET_ANDROID)
#include "DllPaths_generated_android.h"
#else
#include "DllPaths_generated.h"
#endif

