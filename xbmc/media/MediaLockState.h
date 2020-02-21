/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

typedef enum
{
  LOCK_STATE_NO_LOCK = 0,
  LOCK_STATE_LOCK_BUT_UNLOCKED = 1,
  LOCK_STATE_LOCKED = 2,
} MediaLockState;
