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

typedef enum
{
  LOCK_MODE_UNKNOWN            = -1,
  LOCK_MODE_EVERYONE           =  0,
  LOCK_MODE_NUMERIC            =  1,
  LOCK_MODE_GAMEPAD            =  2,
  LOCK_MODE_QWERTY             =  3,
  LOCK_MODE_SAMBA              =  4,
  LOCK_MODE_EEPROM_PARENTAL    =  5
} LockType;
