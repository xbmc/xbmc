/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

enum StartupAction
{
  STARTUP_ACTION_NONE = 0,
  STARTUP_ACTION_PLAY_TV,
  STARTUP_ACTION_PLAY_RADIO
};

// Do not change the numbers; external scripts depend on them
enum
{
  EXITCODE_QUIT = 0,
  EXITCODE_POWERDOWN = 64,
  EXITCODE_RESTARTAPP = 65,
  EXITCODE_REBOOT = 66,
};
