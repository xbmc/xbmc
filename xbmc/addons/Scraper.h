/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "XBDateTime.h"
#include "addons/Addon.h"
#include "utils/ScraperParser.h"
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

namespace XFILE
{
class CCurlFile;
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

// thrown as exception to signal abort or show error dialog
class CScraperError
{
public:
  CScraperError() = default;
  CScraperError(const std::string &sTitle, const std::string &sMessage) :
    m_fAborted(false), m_sTitle(sTitle), m_sMessage(sMessage) {}

  bool FAborted() const { return m_fAborted; }
  const std::string &Title() const { return m_sTitle; }
  const std::string &Message() const { return m_sMessage; }

private:
  bool m_fAborted = true;
  std::string m_sTitle;
  std::string m_sMessage;
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

  /*! \brief Clear any previously cached results for this scraper
   Any previously cached files are cleared if they have been cached for longer than the specified
   cachepersistence.
   */
  void ClearCache() const;

  ContentType Content() const { return m_pathContent; }
  bool RequiresSettings() const { return m_requiressettings; }
  bool Supports(ContentType content) const;

  bool IsInUse() const override;
  bool IsNoop();
  bool IsPython() const { return m_isPython; }

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

  std::vector<CScraperUrl> FindMovie(XFILE::CCurlFile &fcurl,
    const std::string &movieTitle, int movieYear, bool fFirst);
  std::vector<MUSIC_GRABBER::CMusicAlbumInfo> FindAlbum(XFILE::CCurlFile &fcurl,
    const std::string &sAlbum, const std::string &sArtist = "");
  std::vector<MUSIC_GRABBER::CMusicArtistInfo> FindArtist(
    XFILE::CCurlFile &fcurl, const std::string &sArtist);
  KODI::VIDEO::EPISODELIST GetEpisodeList(XFILE::CCurlFile& fcurl, const CScraperUrl& scurl);

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

  bool GetVideoDetails(XFILE::CCurlFile& fcurl,
                       const UniqueIDs& uniqueIDs,
                       const CScraperUrl& scurl,
                       bool fMovie /*else episode*/,
                       CVideoInfoTag& video);
  bool GetAlbumDetails(XFILE::CCurlFile &fcurl, const CScraperUrl &scurl,
    CAlbum &album);
  bool GetArtistDetails(XFILE::CCurlFile &fcurl, const CScraperUrl &scurl,
    const std::string &sSearch, CArtist &artist);
  bool GetArtwork(XFILE::CCurlFile &fcurl, CVideoInfoTag &details);

private:
  CScraper(const CScraper &rhs) = delete;
  CScraper& operator=(const CScraper&) = delete;
  CScraper(CScraper&&) = delete;
  CScraper& operator=(CScraper&&) = delete;

  std::string SearchStringEncoding() const
    { return m_parser.GetSearchStringEncoding(); }

  /*! \brief Get the scraper settings for a particular path in the form of a JSON string
   Loads the default and user settings (if not already loaded) and returns the user settings in the
   form of an JSON string. It is used in Python scrapers.
   \return a string containing the JSON settings
   \sa SetPathSettings
   */
  std::string GetPathSettingsAsJSON();

  bool Load();
  std::vector<std::string> Run(const std::string& function,
                               const CScraperUrl& url,
                               XFILE::CCurlFile& http,
                               const std::vector<std::string>* extras = nullptr);
  std::vector<std::string> RunNoThrow(const std::string& function,
                                      const CScraperUrl& url,
                                      XFILE::CCurlFile& http,
                                      const std::vector<std::string>* extras = nullptr);
  std::string InternalRun(const std::string& function,
                         const CScraperUrl& url,
                         XFILE::CCurlFile& http,
                         const std::vector<std::string>* extras);

  //! \brief FindMovie without the result cache in front of it
  std::vector<CScraperUrl> FindMovieUncached(XFILE::CCurlFile& fcurl,
                                             const std::string& movieTitle,
                                             int movieYear,
                                             bool fFirst);

  //! \brief GetVideoDetails without the result cache in front of it
  bool GetVideoDetailsUncached(XFILE::CCurlFile& fcurl,
                               const UniqueIDs& uniqueIDs,
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

  bool m_fLoaded = false;
  bool m_isPython = false;
  bool m_requiressettings = false;
  CDateTimeSpan m_persistence;
  ContentType m_pathContent = ContentType::NONE;
  CScraperParser m_parser;
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
