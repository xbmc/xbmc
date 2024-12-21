/*
 *  Copyright (C) 2005-2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#define USE_OS_TZDB 1

#define HAS_STRING_VIEW 1

#define HAS_REMOTE_API 0
#include <date/date.h>
#include <date/iso_week.h>
#if !defined(TARGET_WINDOWS)
#include <date/tz.h>
#else // defined(TARGET_WINDOWS)
#include <windows.h>
#endif // !defined(TARGET_WINDOWS)
