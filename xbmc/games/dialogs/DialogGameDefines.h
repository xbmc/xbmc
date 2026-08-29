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
constexpr auto SAVESTATE_GAME_CLIENT_VERSION = "savestate.gameclientversion";

// String of list item property "game.videofilter" when no filter is set
constexpr auto PROPERTY_NO_VIDEO_FILTER = "";

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
constexpr unsigned int CONTROL_SAVES_EMULATOR_VERSION = 10828;

// Control ID for the RetroAchievements dialog
constexpr unsigned int CONTROL_CHEEVOS_LIST = 3; // Kodi list dialogs default to this control ID

// Names of list item properties for achievements
constexpr auto ACHIEVEMENT_ID = "achievement.id";
constexpr auto ACHIEVEMENT_POINTS = "achievement.points";
constexpr auto ACHIEVEMENT_EARNED = "achievement.earned";
constexpr auto ACHIEVEMENT_UNLOCKED_DATE = "achievement.unlockeddate";
constexpr auto ACHIEVEMENT_RARITY_CATEGORY = "achievement.raritycategory";
constexpr auto ACHIEVEMENT_RARITY_STARS = "achievement.raritystars";
constexpr auto ACHIEVEMENT_MEASURED = "achievement.measured";
constexpr auto ACHIEVEMENT_MEASURED_PERCENT = "achievement.measuredpercent";
constexpr auto ACHIEVEMENT_MEASURED_PROGRESS = "achievement.measuredprogress";
