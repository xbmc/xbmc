/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IndexParser.h"
#include "NavigationCommand.h"
#include "M2TSParser.h"
#include "URL.h"

#include <map>
#include <optional>
#include <set>
#include <vector>

namespace XFILE
{
/*! \brief What one of a movie object's play commands plays. */
struct MovieObjectPlay
{
  //! The playlist played, where the object said which outright or put it in a register that could
  //! be followed. Empty when it arrived some other way.
  std::optional<unsigned int> playlist;

  //! The register the playlist number was to be taken from, set only when it could not be followed
  unsigned int registerNumber{0};

  //! Whether that register is one of the player's own rather than a general purpose one. The
  //! player owns its state, so a playlist taken from one can never be followed here.
  bool playerStatusRegister{false};
};

/*! \brief One object of MovieObject.bdmv. */
struct MovieObject
{
  unsigned int object{0};
  std::vector<MovieObjectPlay> plays;

  //! The commands that branch, play or set a register, in the order the object runs them.
  //! What an object plays often depends on a register another one set - a menu is rarely one
  //! object, and its buttons commonly put a playlist number in a register before handing over -
  //! so following it means re-running the sequence with those values, not just reading plays.
  std::vector<NavigationCommand> commands;
};

/*!
 \brief How a disc navigates - its table of contents, its movie objects and its menu.

 Everything here is read from the disc once and holds for as long as it is in the drive, so it is
 gathered in one structure that can be cached rather than re-read.
 */
struct MovieObjectInformation
{
  //! Whether index.bdmv could be read. Nothing else here means anything when it could not.
  bool indexRead{false};

  //! Whether MovieObject.bdmv was read. False on a disc that navigates entirely through BD-J,
  //! where there is nothing in it to describe.
  bool movieObjectsRead{false};

  IndexInformation index;
  std::vector<MovieObject> movieObjects;

  //! The playlists the top menu plays, followed through the branches between objects
  std::set<unsigned int> menuPlaylists;

  //! The menus found in the clips those playlists reference, by clip
  std::map<unsigned int, IGMenuInformation> menus;

  //! The playlists the menu's buttons lead to. A button rarely names a playlist - it jumps to a
  //! title, and the object index.bdmv maps that title to does the playing - so these are found by
  //! following each button through the title table and into the object behind it.
  std::set<unsigned int> menuTargetPlaylists;

  /*!
   \brief Where a movie object is referenced from, or an unused entry when it is not this disc's.
   */
  MovieObjectUsage GetMovieObjectUsage(unsigned int object) const
  {
    return index.GetMovieObjectUsage(object);
  }
};

class CMovieObjectParser
{
public:
  /*!
   \brief Read how the disc navigates - index.bdmv, MovieObject.bdmv and the menu behind them.

   Reads the disc even where it turns out to describe nothing, so that the result says as much and
   a caller holding it does not go back to the disc to find out again.

   \return true if index.bdmv could be read, whatever it then turned out to say
   */
  static bool GetMovieObject(const CURL& url, MovieObjectInformation& information);

  /*! \brief Report what was found, for the bluray debug log. */
  static void LogMovieObject(const MovieObjectInformation& information);
};
} // namespace XFILE
