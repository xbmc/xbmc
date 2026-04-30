/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string_view>

namespace KODI
{
namespace GAME
{

constexpr int CONTROL_DISC_MANAGER_MENU = 3;
constexpr int CONTROL_DISC_MANAGER_DISC_LIST = 108321;

constexpr int CONTROL_BUTTON_SELECT_DISC = 108323;
constexpr int CONTROL_BUTTON_EJECT_INSERT = 108324;
constexpr int CONTROL_BUTTON_ADD = 108325;
constexpr int CONTROL_BUTTON_DELETE = 108326;
constexpr int CONTROL_BUTTON_RESUME_GAME = 108327;

constexpr unsigned int MENU_INDEX_SELECT_DISC = 0;
constexpr unsigned int MENU_INDEX_EJECT_INSERT = 1;
constexpr unsigned int MENU_INDEX_ADD_DISC = 2;
constexpr unsigned int MENU_INDEX_DELETE_DISC = 3;
constexpr unsigned int MENU_INDEX_RESUME_GAME = 4;
constexpr unsigned int MENU_ITEM_COUNT = 5;

constexpr std::string_view WINDOW_PROPERTY_SHOW_MENU = "GameDiscManager.ShowMenu";
constexpr std::string_view WINDOW_PROPERTY_SHOW_DISC_LIST = "GameDiscManager.ShowDiscList";

constexpr std::string_view ITEM_PROPERTY_DISC_INDEX = "GameDiscManager.DiscIndex";
constexpr std::string_view ITEM_PROPERTY_IS_NO_DISC = "GameDiscManager.IsNoDisc";

} // namespace GAME
} // namespace KODI
