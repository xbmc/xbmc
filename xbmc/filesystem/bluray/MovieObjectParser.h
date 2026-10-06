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

#include <vector>

class CURL;

namespace XFILE
{
/*! \brief One object of MovieObject.bdmv. */
struct MovieObject
{
  unsigned int object{0};

  //! The commands that branch, play or set a register, in the order the object runs them.
  //! What an object plays often depends on a register another one set, so following it means
  //! re-running the sequence with those values.
  std::vector<NavigationCommand> commands;
};

/*!
 \brief How a disc navigates - its table of contents and its movie objects.
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

  //! The playlists the disc's HDMV titles play, followed through the branches between objects, in
  //! the order of the titles. A title's playlist chosen through a register another object set is
  //! not found.
  std::vector<unsigned int> titlePlaylists;
};

class CMovieObjectParser
{
public:
  /*!
   \brief Read how the disc navigates - index.bdmv, MovieObject.bdmv and the playlists its titles
   play.
   \return true if index.bdmv could be read, whatever it then turned out to say
   */
  static bool GetMovieObject(const CURL& url, MovieObjectInformation& information);
};
} // namespace XFILE
