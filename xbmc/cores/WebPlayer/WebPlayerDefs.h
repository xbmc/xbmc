/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

namespace KODI
{
namespace WEB
{

//! \brief Name under which the web player is registered with the player factory
constexpr const char* PLAYER_NAME = "WebPlayer";

//! \brief The web video frames Kodi keeps while it waits for the next one: the frame on the
//! video plane, the frame it replaced until the plane has moved on, and a late frame the render
//! manager skipped
constexpr unsigned int VIDEO_FRAMES_HELD = 3;

//! \brief Item property naming the web profile a page opens in; a web add-on's id or "default"
constexpr const char* PROPERTY_PROFILE = "web.profile";

//! \brief The profile of plain browsing
constexpr const char* DEFAULT_PROFILE = "default";

//! \brief The add-on that installs the Widevine CDM for a web app importing it
constexpr const char* INPUTSTREAMHELPER = "script.module.inputstreamhelper";

//! \brief The category of the settings Kodi gives only a web app that names a red button
constexpr const char* RED_BUTTON_CATEGORY = "hbbtv";

//! \brief The setting whether the red button hides the app, as on a television
constexpr const char* SETTING_RED_BUTTON_HIDES = "redbuttonhides";

//! \brief The setting whether an app that can do without encrypted media gets it
constexpr const char* SETTING_ENCRYPTED_MEDIA = "encryptedmedia";

//! \brief The user script that takes the encrypted media API from a page
constexpr const char* NO_ENCRYPTED_MEDIA_SCRIPT =
    "special://xbmc/system/webapp/no-encrypted-media.js";

} // namespace WEB
} // namespace KODI
