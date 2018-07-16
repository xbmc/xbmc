/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

namespace LOCK_LEVEL {
  /**
   Specifies, what Settings levels are locked for the user
   **/
  enum SETTINGS_LOCK
  {
    NONE,     //settings are unlocked => user can access all settings levels
    ALL,      //all settings are locked => user always has to enter password, when entering the settings screen
    STANDARD, //settings level standard and up are locked => user can still access the beginner levels
    ADVANCED,
    EXPERT
  };
}


