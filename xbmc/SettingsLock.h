/*
 *      Copyright (C) 2005-2013 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
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


