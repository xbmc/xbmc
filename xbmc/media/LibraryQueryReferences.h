/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "media/LibraryQueryType.h"

#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace KODI::MEDIA
{

class CLibraryQuery;

/*!
 * \brief The folders of the playlists path a smart playlist of this type can name another smart
 * playlist from, by a playlist or a virtual folder rule.
 */
std::vector<std::string> PlaylistFoldersOf(LIBRARY_QUERY::TYPE type);

/*!
 * \brief The smart playlists the rules of a library query name, found and loaded once for a
 * listing. The WHERE clause and the virtual folders of the listing are both built from it.
 */
class CLibraryQueryReferences
{
public:
  /*!
   * \brief Finds the playlist every playlist and virtual folder rule of \p query names, and of the
   * playlists those take their rules from.
   * \return nullopt, with the rule and the playlist logged, if a rule names a playlist that is
   * missing, that does not load, whose type the playlist holding the rule cannot take rules from,
   * or that leads back to itself
   */
  static std::optional<CLibraryQueryReferences> Resolve(const CLibraryQuery& query);

  //! \return the playlist a playlist rule with this value takes its rules from; null if none does
  const CLibraryQuery* Find(std::string_view name) const;

  //! The files of the playlists the query lists as folders
  const std::vector<std::string>& GetVirtualFolders() const { return m_virtualFolders; }

private:
  std::map<std::string, std::shared_ptr<const CLibraryQuery>, std::less<>> m_playlists;
  std::vector<std::string> m_virtualFolders;
};

} // namespace KODI::MEDIA
