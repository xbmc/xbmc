/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "BlurayPlaylistHints.h"

#include <algorithm>
#include <array>
#include <optional>
#include <string>
#include <string_view>

namespace XFILE
{
namespace
{
/*!
 \brief The whole feature - FPL_MainFeature, FPL_MainFeature_EXT, SEG_MainFeature.
 Not one of the numbered segments a feature is sometimes assembled from.
 */
bool IsFeature(std::string_view name)
{
  // FPL_MainFeature and the variants that follow it - _EXT, _Narrative, _EXT_Narrative, and one
  // per language. The prefix is not always FPL_, as "SEG FPL_MainFeature" also occurs, so this
  // looks for the name rather than testing the start of it.
  if (name.find("FPL_MainFeature") != std::string_view::npos)
    return true;

  // SEG_MainFeature on its own is the whole presentation too. Suffixed it is not - a disc that
  // assembles its feature from parts numbers them SEG_MainFeature_01, _EXT_12, _TH_23 and so on,
  // and those are minutes long where the feature is hours.
  return name == "SEG_MainFeature";
}

/*! \brief An episode of a series - EPL_01, SEG_EPL_02. */
bool IsEpisode(std::string_view name)
{
  return name.starts_with("EPL_") || name.starts_with("SEG_EPL_");
}

/*! \brief A special feature - SF_Inside_Derry_102, SEG_SF_BTV_Power_Of_Sound. */
bool IsSpecialFeature(std::string_view name)
{
  return name.starts_with("SF_") || name.starts_with("SEG_SF_");
}

/*!
 \brief Front matter - a studio ident, piracy or copyright warning, age certificate or
 disclaimer, shown before anything is played.
 */
bool IsWarningOrLogo(std::string_view name)
{
  // Studios name these a dozen ways, so matching a single prefix covers only one of them
  static constexpr std::array MARKERS{"Warn", "Disclaimer", "Logo", "Parental", " AGE "};
  if (std::ranges::any_of(MARKERS, [name](std::string_view marker)
                          { return name.find(marker) != std::string_view::npos; }))
    return true;

  // The same thing under names that do not say so - an anti-piracy warning, a studio ident, an
  // intellectual property notice, a certificate
  return name.starts_with("WRN_") || name.starts_with("FBI") || name.starts_with("Studio") ||
         name.starts_with("IPR") || name == "MPAA";
}

/*! \brief A menu background or transition rather than content. */
bool IsMenu(std::string_view name)
{
  return name.find("Menu") != std::string_view::npos || name.starts_with("TMPL");
}

struct EpisodeName
{
  unsigned int ordinal{0};

  //! Whether the name ends at the ordinal. EPL_02 is the episode.
  //! EPL_02_Narrative and EPL_02_JPN are the same episode described or dubbed.
  bool base{false};
};

//! \brief The ordinal in an episode playlist's name - the 2 of EPL_02, SEG_EPL_02, EPL_02_Narrative
std::optional<EpisodeName> ReadEpisodeName(std::string_view name)
{
  const size_t marker{name.find("EPL_")};
  if (marker == std::string_view::npos)
    return std::nullopt;

  EpisodeName episode;
  size_t i{marker + 4};
  size_t digits{0};
  for (; i < name.size() && name[i] >= '0' && name[i] <= '9'; ++i, ++digits)
    episode.ordinal = episode.ordinal * 10 + static_cast<unsigned int>(name[i] - '0');

  if (digits == 0)
    return std::nullopt;

  episode.base = i == name.size();
  return episode;
}

//! \brief The words of a name without its role prefix - SEG_SF_Becoming_Pennywise -> "Becoming Pennywise"
std::string GetTitle(std::string_view name)
{
  if (name.starts_with("SEG_"))
    name.remove_prefix(4);
  if (name.starts_with("SF_"))
    name.remove_prefix(3);

  std::string title{name};
  std::ranges::replace(title, '_', ' ');
  return title;
}
} // namespace

PlaylistRole GetProjectPlaylistRole(std::string_view name)
{
  if (IsFeature(name))
    return PlaylistRole::FEATURE;
  if (IsEpisode(name))
    return PlaylistRole::EPISODE;
  if (IsSpecialFeature(name))
    return PlaylistRole::SPECIAL;
  if (IsWarningOrLogo(name))
    return PlaylistRole::FRONT_MATTER;
  if (IsMenu(name))
    return PlaylistRole::MENU;
  return PlaylistRole::UNKNOWN;
}

CBlurayPlaylistHints::CBlurayPlaylistHints(const ProjectInformation& project)
  : m_present(project.present)
{
  for (const auto& [playlist, information] : project.playlists)
  {
    PlaylistHint hint{.role = GetProjectPlaylistRole(information.name), .name = information.name};

    switch (hint.role)
    {
      case PlaylistRole::EPISODE:
        // A disc numbering its episodes some other way offers no ordinal, and cannot be matched
        // to them by number
        if (const std::optional<EpisodeName> episode{ReadEpisodeName(information.name)})
        {
          hint.ordinal = episode->ordinal;
          hint.basePresentation = episode->base;
        }
        break;
      case PlaylistRole::SPECIAL:
        hint.title = GetTitle(information.name);
        break;
      default:
        break;
    }

    m_hints.emplace(playlist, std::move(hint));
  }
}
} // namespace XFILE
