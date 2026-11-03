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

} // namespace WEB
} // namespace KODI
