/*
 *  Copyright (C) 2005-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

// This file contains keysyms from newer libxkbcommon versions,
// that Kodi still builds with older versions of that library.

// From libxkbcommon 1.11.0:

#if !defined(XKB_KEY_XF86OK)
#define XKB_KEY_XF86OK 0x10081160
#endif

#if !defined(XKB_KEY_XF86MediaSelectProgramGuide)
#define XKB_KEY_XF86MediaSelectProgramGuide 0x1008116a
#endif
