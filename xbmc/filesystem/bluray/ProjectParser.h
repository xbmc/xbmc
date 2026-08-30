/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "URL.h"

#include <chrono>
#include <map>
#include <string>

namespace XFILE
{
using namespace std::chrono_literals;

/*!
 \brief One playlist as the disc's authoring project named it.
 */
struct ProjectPlaylistInformation
{
  unsigned int playlist{0};
  std::string name; //!< the name the disc was authored under, eg. EPL_01, FPL_MainFeature
  std::string presentation; //!< 2D or 3D
  float frameRate{0.0f};
  std::chrono::milliseconds duration{0ms};
  unsigned int playItems{0};

  //
  // The names follow a convention rather than a specification, so these are a hint about what a
  // playlist holds, not a statement of fact. They have been seen to hold on every disc carrying
  // the file so far, but a disc from another authoring house may well name things differently.
  //

  /*!
   \brief The whole feature - FPL_MainFeature, FPL_MainFeature_EXT, SEG_MainFeature.
   Not one of the numbered segments a feature is sometimes assembled from.
   */
  bool IsFeature() const;

  /*! \brief An episode of a series - EPL_01, SEG_EPL_02. */
  bool IsEpisode() const;

  /*! \brief A special feature - SF_Inside_Derry_102, SEG_SF_BTV_Power_Of_Sound. */
  bool IsSpecialFeature() const;

  /*!
   \brief Front matter - a studio ident, piracy or copyright warning, age certificate or
   disclaimer, shown before anything is played.
   */
  bool IsWarningOrLogo() const;

  /*! \brief A menu background or transition rather than content. */
  bool IsMenu() const;
};

/*!
 \brief What the disc's authoring project says its playlists are.

 Some discs ship the project their author built them from, which names every playlist and says
 what it holds. Nothing in the BDMV structure records that - MovieObject.bdmv and the menus say
 which playlists are reachable, but never that one is the feature, an episode or an extra - so
 where this file is present it is the only thing on the disc that says so outright.

 The file is not part of the Blu-ray specification. It is a serialised authoring project that
 happens to be left in the BD-J application's asset directory, so only a fraction of discs have
 one and its format is a matter of observation rather than record.
 */
struct ProjectInformation
{
  //! Whether the disc carried a project at all. Most do not, so this says that the disc
  //! has been looked at and has none, rather than that it has not been looked at.
  bool present{false};

  std::map<unsigned int, ProjectPlaylistInformation> playlists;
};

class CProjectParser
{
public:
  /*!
   \brief Parse the disc's authoring project, if it left one behind.

   Looks for BDMV/JAR/<application>/bluray_project.bin.

   \return true when a project was found and at least one playlist was named
   */
  static bool GetProject(const CURL& url, ProjectInformation& projectInformation);

  /*! \brief Report what the project named, for the bluray debug log. */
  static void LogProject(const ProjectInformation& projectInformation);
};
} // namespace XFILE
