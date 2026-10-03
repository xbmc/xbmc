/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "ProjectParser.h"
#include "filesystem/IPlaylistHints.h"

#include <string_view>

namespace XFILE
{
/*!
 \brief Classify a playlist by the name the disc's authoring project gave it.

 The names follow a convention rather than a specification, so this is a hint about what a
 playlist holds, not a statement of fact. It has held on every disc carrying a project so far, but
 a disc from another authoring house may well name things differently.
 */
PlaylistRole GetProjectPlaylistRole(std::string_view name);

/*!
 \brief What a disc's authoring project says about its playlists, as hints.
 */
class CBlurayPlaylistHints : public IPlaylistHints
{
public:
  explicit CBlurayPlaylistHints(const ProjectInformation& project);

  bool HasHints() const override { return m_present; }
  const PlaylistHintMap& GetHints() const override { return m_hints; }

private:
  bool m_present{false};
  PlaylistHintMap m_hints;
};
} // namespace XFILE
