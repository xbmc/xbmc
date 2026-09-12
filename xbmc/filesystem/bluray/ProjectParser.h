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
#include <cstdint>
#include <map>
#include <set>
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

/*!
 \brief How a look for the disc's authoring project ended.

 A disc carrying no project and a disc that could not be read both name no playlists, but only the
 first is an answer. The second has to be told apart so that it can be looked for again.
 */
enum class ProjectReadResult : uint8_t
{
  READ, //!< the disc carries a project and it was parsed
  ABSENT, //!< the disc carries no project, as most do not
  FAILED, //!< the disc carries a project that could not be read, so nothing is known yet
};

class CProjectParser
{
public:
  /*!
   \brief Parse the disc's authoring project, if it left one behind.

   Looks for BDMV/JAR/<application>/bluray_project.bin.

   \return whether the project was read, is absent, or could not be read
   */
  static ProjectReadResult GetProject(const CURL& url,
                                      const std::set<unsigned int>& discPlaylists,
                                      ProjectInformation& projectInformation);

  /*! \brief Report what the project named, for the bluray debug log. */
  static void LogProject(const ProjectInformation& projectInformation);
};
} // namespace XFILE
