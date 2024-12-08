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
#include <vector>

class CAlbum;
class CArtist;
class CVideoInfoTag;

namespace MUSIC_GRABBER
{
class CMusicAlbumInfo;
class CMusicArtistInfo;
}

typedef enum
{
  CONTENT_MOVIES,
  CONTENT_TVSHOWS,
  CONTENT_MUSICVIDEOS,
  CONTENT_ALBUMS,
  CONTENT_ARTISTS,
  CONTENT_NONE,
} CONTENT_TYPE;

namespace XFILE
{
  class CCurlFile;
}

class CScraperUrl;

namespace ADDON
{
class CScraper;
typedef std::shared_ptr<CScraper> ScraperPtr;

std::string TranslateContent(const CONTENT_TYPE &content, bool pretty=false);
CONTENT_TYPE TranslateContent(const std::string &string);
AddonType ScraperTypeFromContent(const CONTENT_TYPE& content);

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
  bool SetPathSettings(CONTENT_TYPE content, const std::string& xml);

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
  void ClearCache();

  CONTENT_TYPE Content() const { return m_pathContent; }
  bool RequiresSettings() const { return m_requiressettings; }
  bool Supports(const CONTENT_TYPE &content) const;

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

  bool GetVideoDetails(XFILE::CCurlFile& fcurl,
                       const std::unordered_map<std::string, std::string>& uniqueIDs,
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

  bool m_fLoaded = false;
  bool m_isPython = false;
  bool m_requiressettings = false;
  CDateTimeSpan m_persistence;
  CONTENT_TYPE m_pathContent = CONTENT_NONE;
  CScraperParser m_parser;
};

}

