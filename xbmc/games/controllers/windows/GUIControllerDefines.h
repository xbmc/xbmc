/*
 *      Copyright (C) 2014-2016 Team Kodi
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
#pragma once

// Duration to wait for input from the user
#define COUNTDOWN_DURATION_SEC  6

// Warn the user that time is running out after this duration
#define WAIT_TO_WARN_SEC        2

// GUI Control IDs
#define CONTROL_CONTROLLER_LIST             3
#define CONTROL_FEATURE_LIST                5
#define CONTROL_FEATURE_BUTTON_TEMPLATE     7
#define CONTROL_FEATURE_GROUP_TITLE         8
#define CONTROL_FEATURE_SEPARATOR           9
#define CONTROL_CONTROLLER_BUTTON_TEMPLATE  10
#define CONTROL_HELP_BUTTON                 17
#define CONTROL_CLOSE_BUTTON                18
#define CONTROL_RESET_BUTTON                19
#define CONTROL_GET_MORE                    20
#define CONTROL_GAME_CONTROLLER             31

#define MAX_CONTROLLER_COUNT  100 // large enough
#define MAX_FEATURE_COUNT     100 // large enough

#define CONTROL_CONTROLLER_BUTTONS_START  100
#define CONTROL_CONTROLLER_BUTTONS_END    (CONTROL_CONTROLLER_BUTTONS_START + MAX_CONTROLLER_COUNT)
#define CONTROL_FEATURE_BUTTONS_START     CONTROL_CONTROLLER_BUTTONS_END
#define CONTROL_FEATURE_BUTTONS_END       (CONTROL_FEATURE_BUTTONS_START + MAX_FEATURE_COUNT)
#define CONTROL_FEATURE_GROUPS_START      CONTROL_FEATURE_BUTTONS_END
#define CONTROL_FEATURE_GROUPS_END        (CONTROL_FEATURE_GROUPS_START + MAX_FEATURE_COUNT)
#define CONTROL_FEATURE_SEPARATORS_START  CONTROL_FEATURE_GROUPS_END
#define CONTROL_FEATURE_SEPARATORS_END    (CONTROL_FEATURE_SEPARATORS_START + MAX_FEATURE_COUNT)
