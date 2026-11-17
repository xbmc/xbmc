/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "addons/Addon.h"
#include "utils/ScraperUrl.h"
#include "video/Episode.h"

#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <fmt/format.h>

class CAlbum;
class CArtist;
class CVideoInfoTag;

namespace MUSIC_GRABBER
{
class CMusicAlbumInfo;
class CMusicArtistInfo;
}

namespace ADDON
{
class CScraper;
using ScraperPtr = std::shared_ptr<CScraper>;

enum class ContentType
{
  MOVIES,
  MOVIE_VERSIONS,
  TVSHOWS,
  MUSICVIDEOS,
  ALBUMS,
  ARTISTS,
  NONE,
};

std::string TranslateContent(ContentType content, bool pretty = false);
ContentType TranslateContent(std::string_view string);
AddonType ScraperTypeFromContent(ContentType content);

//! Thrown when a scraper's search fails, rather than finding nothing
class CScraperError
{
};

class CScraper : public CAddon
{
public:
  explicit CScraper(const AddonInfoPtr& addonInfo, AddonType addonType);

  /*! \brief Set the scraper settings for a particular path from an XML string
   Loads the default and user settings (if not already loaded) and, if the given XML string is non-empty,
   overrides the user settings with the XML.
   \param content Content type of the path
   \param xml string of XML with the settings.  If non-empty this overrides any saved user settings.
   \return true if settings are available, false otherwise
   \sa GetPathSettings
   */
  bool SetPathSettings(ContentType content, const std::string& xml);

  /*! \brief Get the scraper settings for a particular path in the form of an XML string
   Loads the default and user settings (if not already loaded) and returns the user settings in the
   form or an XML string
   \return a string containing the XML settings
   \sa SetPathSettings
   */
  std::string GetPathSettings();

  ContentType Content() const { return m_pathContent; }
  bool RequiresSettings() const { return m_requiressettings; }
  bool Supports(ContentType content) const;

  /*! \brief Whether Kodi runs every scraper an add-on provides. Kodi runs python scrapers only,
   and "Local information only", which has no library as there is nothing to run. The library is
   the one parsed for this platform, or the extension's library attribute for an add-on a
   repository lists, which has none parsed.
   */
  static bool IsSupported(const CAddonInfo& info);

  bool IsInUse() const override;

  bool IsLocalInformationOnly() const;

  // scraper media functions
  CScraperUrl NfoUrl(const std::string &sNfoContent);

  /*! \brief Resolve an external ID (e.g. MusicBrainz IDs) to a URL using scrapers
   If we have an ID in hand, e.g. MusicBrainz IDs or TheTVDB Season IDs
   we can get directly to a URL instead of searching by name and choosing from
   the search results. The correct scraper type should be used to get the right
   URL for a given ID, so we can differentiate albums, artists, TV Seasons, etc.
   \param externalID the external ID - e.g. MusicBrainzArtist/AlbumID
   \return a populated URL pointing to the details page for the given ID or
           an empty URL if we couldn't resolve the ID.
   */
  CScraperUrl ResolveIDToUrl(const std::string &externalID);

  /*! \brief Search for a movie, show or music video by its title and year. A search that finds
   nothing is made again with the title cleaned less.
   \param movieTitle the title, cleaned here when no year is given
   \param movieYear the year, or -1 if it isn't known
   \throws CScraperError if a search fails
   */
  std::vector<CScraperUrl> FindMovie(const std::string& movieTitle, int movieYear);
  std::vector<MUSIC_GRABBER::CMusicAlbumInfo> FindAlbum(const std::string& sAlbum,
                                                        const std::string& sArtist = "");
  std::vector<MUSIC_GRABBER::CMusicArtistInfo> FindArtist(const std::string& sArtist);
  KODI::VIDEO::EPISODELIST GetEpisodeList(const CScraperUrl& scurl);

  struct StringHash
  {
    using is_transparent = void; // Enables heterogeneous operations.
    std::size_t operator()(std::string_view sv) const
    {
      std::hash<std::string_view> hasher;
      return hasher(sv);
    }
  };
  using UniqueIDs = std::unordered_map<std::string, std::string, StringHash, std::equal_to<>>;

  bool GetVideoDetails(const UniqueIDs& uniqueIDs,
                       const CScraperUrl& scurl,
                       bool fMovie /*else episode*/,
                       CVideoInfoTag& video);
  bool GetAlbumDetails(const CScraperUrl& scurl, CAlbum& album);
  bool GetArtistDetails(const CScraperUrl& scurl, CArtist& artist);
  bool GetArtwork(CVideoInfoTag& details);

private:
  CScraper(const CScraper &rhs) = delete;
  CScraper& operator=(const CScraper&) = delete;
  CScraper(CScraper&&) = delete;
  CScraper& operator=(CScraper&&) = delete;

  /*! \brief Get the scraper settings for a particular path in the form of a JSON string
   Loads the default and user settings (if not already loaded) and returns the user settings in the
   form of an JSON string. It is used in Python scrapers.
   \return a string containing the JSON settings
   \sa SetPathSettings
   */
  std::string GetPathSettingsAsJSON();

  //! \brief One of FindMovie's searches, without the result cache in front of it. Without fFirst,
  //! the title is cleaned less.
  std::vector<CScraperUrl> FindMovieUncached(const std::string& movieTitle,
                                             int movieYear,
                                             bool fFirst);

  //! \brief GetVideoDetails without the result cache in front of it
  bool GetVideoDetailsUncached(const UniqueIDs& uniqueIDs,
                               const CScraperUrl& scurl,
                               bool fMovie,
                               CVideoInfoTag& video);

  /*! \brief Cache scraper results rather than repeating the request.

   Lifetime is this instance.
   Repeats come from versions, discs and folders of the same title, which are scraped close together.
   Not synchronised, matching the rest of CScraper: a scraper shared between threads needs the
   caller to provide that.
   */
  static constexpr size_t MAX_CACHED_RESULTS{50};
  std::unordered_map<std::string, std::vector<CScraperUrl>, StringHash, std::equal_to<>>
      m_findCache;
  std::unordered_map<std::string, std::shared_ptr<const CVideoInfoTag>, StringHash, std::equal_to<>>
      m_detailsCache;

  bool m_requiressettings = false;
  ContentType m_pathContent = ContentType::NONE;
};

} // namespace ADDON

template<>
struct fmt::formatter<ADDON::ContentType> : fmt::formatter<std::string_view>
{
  template<typename FormatContext>
  constexpr auto format(const ADDON::ContentType& type, FormatContext& ctx) const
  {
    return fmt::formatter<std::string_view>::format(enumToSV(type), ctx);
  }

private:
  static constexpr std::string_view enumToSV(ADDON::ContentType type)
  {
    using namespace std::literals::string_view_literals;
    switch (type)
    {
      using enum ADDON::ContentType;

      case MOVIES:
        return "movies"sv;
      case MOVIE_VERSIONS:
        return "movie versions"sv;
      case TVSHOWS:
        return "TV shows"sv;
      case MUSICVIDEOS:
        return "music videos"sv;
      case ALBUMS:
        return "albums"sv;
      case ARTISTS:
        return "artists"sv;
      case NONE:
        return "none"sv;
    }
    throw std::invalid_argument("no content string found");
  }
};
