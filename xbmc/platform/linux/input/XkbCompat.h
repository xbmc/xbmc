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

#if !defined(XKB_KEY_XF86GoTo)
#define XKB_KEY_XF86GoTo 0x10081162
#endif

#if !defined(XKB_KEY_XF86MediaSelectProgramGuide)
#define XKB_KEY_XF86MediaSelectProgramGuide 0x1008116a
#endif

#if !defined(XKB_KEY_XF86MediaSelectHome)
#define XKB_KEY_XF86MediaSelectHome 0x1008116e
#endif

#if !defined(XKB_KEY_XF86MediaLanguageMenu)
#define XKB_KEY_XF86MediaLanguageMenu 0x10081170
#endif

#if !defined(XKB_KEY_XF86MediaTitleMenu)
#define XKB_KEY_XF86MediaTitleMenu 0x10081171
#endif

#if !defined(XKB_KEY_XF86AudioChannelMode)
#define XKB_KEY_XF86AudioChannelMode 0x10081175
#endif

#if !defined(XKB_KEY_XF86MediaSelectTV)
#define XKB_KEY_XF86MediaSelectTV 0x10081179
#endif

#if !defined(XKB_KEY_XF86MediaSelectCable)
#define XKB_KEY_XF86MediaSelectCable 0x1008117a
#endif

#if !defined(XKB_KEY_XF86MediaSelectRadio)
#define XKB_KEY_XF86MediaSelectRadio 0x10081181
#endif

#if !defined(XKB_KEY_XF86MediaSelectTuner)
#define XKB_KEY_XF86MediaSelectTuner 0x10081182
#endif

#if !defined(XKB_KEY_XF86MediaPlayer)
#define XKB_KEY_XF86MediaPlayer 0x10081183
#endif

#if !defined(XKB_KEY_XF86MediaSelectTeletext)
#define XKB_KEY_XF86MediaSelectTeletext 0x10081184
#endif
