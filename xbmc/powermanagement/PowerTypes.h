/*
 *  Copyright (C) 2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

enum PowerState
{
  POWERSTATE_QUIT = 0,
  POWERSTATE_SHUTDOWN,
  POWERSTATE_HIBERNATE,
  POWERSTATE_SUSPEND,
  POWERSTATE_REBOOT,
  POWERSTATE_MINIMIZE,
  POWERSTATE_NONE,
  POWERSTATE_ASK
};
