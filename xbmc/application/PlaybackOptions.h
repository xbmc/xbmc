/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>

namespace KODI::APPLICATION
{

//! Whether the file is opened again in place: a restart, another player, the next stack part.
enum class Reopen
{
  No,
  Yes
};

//! Whether this playback begins something the user started, rather than continuing it; only a
//! start may switch to fullscreen.
enum class StartsRun
{
  No,
  Yes
};

//! What a failed playlist entry moves on to.
enum class OnFail
{
  Next,
  Previous
};

//! How a playlist play starts.
struct PlayOptions
{
  //! The player to use; empty for the default.
  std::string player{};
  Reopen reopen{Reopen::No};
  OnFail onFail{OnFail::Next};
  //! Play in list order: the shuffle is turned off, and stays off.
  bool inOrder{false};
};

} // namespace KODI::APPLICATION
