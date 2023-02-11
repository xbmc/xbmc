/*
 *  Copyright (C) 2020-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

// Name of list item property for savestate captions
constexpr auto SAVESTATE_LABEL = "savestate.label";
constexpr auto SAVESTATE_CAPTION = "savestate.caption";
constexpr auto SAVESTATE_GAME_CLIENT = "savestate.gameclient";

// Control IDs for game dialogs
constexpr unsigned int CONTROL_VIDEO_HEADING = 10810;
constexpr unsigned int CONTROL_VIDEO_THUMBS = 10811;
constexpr unsigned int CONTROL_VIDEO_DESCRIPTION = 10812;
constexpr unsigned int CONTROL_SAVES_HEADING = 10820;
constexpr unsigned int CONTROL_SAVES_DETAILED_LIST = 3; // Select dialog defaults to this control ID
constexpr unsigned int CONTROL_SAVES_DESCRIPTION = 10822;
constexpr unsigned int CONTROL_SAVES_EMULATOR_NAME = 10823;
constexpr unsigned int CONTROL_SAVES_EMULATOR_ICON = 10824;
constexpr unsigned int CONTROL_SAVES_NEW_BUTTON = 10825;
constexpr unsigned int CONTROL_SAVES_CANCEL_BUTTON = 10826;
constexpr unsigned int CONTROL_NUMBER_OF_ITEMS = 10827;
