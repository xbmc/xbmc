/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "EvdevKeyMapping.h"

#include "utils/Map.h"

#include <linux/input.h>

constexpr auto evdevMap = make_map<uint32_t, XBMCKey>({
    // still missing in libxkbcommon 1.13.2
    {KEY_PREVIOUS, XBMCK_PREVIOUS},
    {KEY_NEXT, XBMCK_NEXT},
    {KEY_POWER2, XBMCK_POWER2},
    {KEY_SETUP, XBMCK_SETUP},
    {KEY_LAST, XBMCK_RECALL_LAST},
    {KEY_LIST, XBMCK_LIST},
    {KEY_MP3, XBMCK_MP3},

    // ambiguously mapped in xkb, map via evdev
    {KEY_AGAIN, XBMCK_AGAIN},
    {KEY_CHANNEL, XBMCK_CHANNEL},
    {KEY_EXIT, XBMCK_EXIT},

    // added in libxkbcommon 1.11.0 - see also XkbCompat.h
    {KEY_OK, XBMCK_OK}, // XKB_KEY_XF86OK
    {KEY_EPG, XBMCK_EPG}, // XKB_KEY_XF86MediaSelectProgramGuide
    {KEY_PROGRAM, XBMCK_EPG}, // XKB_KEY_XF86MediaSelectProgramGuide
    {KEY_PVR, XBMCK_PVR}, // XKB_KEY_XF86MediaSelectHome
    {KEY_LANGUAGE, XBMCK_LANGUAGE}, // XKB_KEY_XF86MediaLanguageMenu
    {KEY_TITLE, XBMCK_TITLE_MENU}, // XKB_KEY_XF86MediaTitleMenu
    {KEY_MODE, XBMCK_AUDIO_MODE}, // XKB_KEY_XF86AudioChannelMode
    {KEY_TV, XBMCK_TV}, // XKB_KEY_XF86MediaSelectTV
    {KEY_RADIO, XBMCK_RADIO}, // XKB_KEY_XF86MediaSelectRadio
    {KEY_TUNER, XBMCK_TUNER}, // XKB_KEY_XF86MediaSelectTuner
    {KEY_PLAYER, XBMCK_LAUNCH_MEDIA_SELECT}, // XKB_KEY_XF86MediaPlayer
    {KEY_TEXT, XBMCK_TEXT} // XKB_KEY_XF86MediaSelectTeletext
});

std::optional<XBMCKey> CEvdevKeyMapping::XBMCKeyForEvdevCode(uint32_t code)
{
  return evdevMap.get(code);
}
