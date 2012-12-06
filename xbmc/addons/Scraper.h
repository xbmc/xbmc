#pragma once
/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
#include "addons/Addon.h"
#include "XBDateTime.h"
#include "utils/ScraperUrl.h"
#include "utils/ScraperParser.h"
#include "video/Episode.h"

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
typedef boost::shared_ptr<CScraper> ScraperPtr;

CStdString TranslateContent(const CONTENT_TYPE &content, bool pretty=false);
CONTENT_TYPE TranslateContent(const CStdString &string);
TYPE ScraperTypeFromContent(const CONTENT_TYPE &content);

// thrown as exception to signal abort or show error dialog
class CScraperError
{
public:
  CScraperError() : m_fAborted(true) {}
  CScraperError(const CStdString &sTitle, const CStdString &sMessage) :
    m_fAborted(false), m_sTitle(sTitle), m_sMessage(sMessage) {}

  bool FAborted() const { return m_fAborted; }
  const CStdString &Title() const { return m_sTitle; }
  const CStdString &Message() const { return m_sMessage; }

private:
  bool m_fAborted;
  CStdString m_sTitle;
  CStdString m_sMessage;
};

class CScraper : public CAddon
{
public:
  CScraper(const AddonProps &props) : CAddon(props), m_fLoaded(false) {}
  CScraper(const cp_extension_t *ext);
  virtual ~CScraper() {}
  virtual AddonPtr Clone(const AddonPtr &self) const;

  /*! \brief Set the scraper settings for a particular path from an XML string
   Loads the default and user settings (if not already loaded) and, if the given XML string is non-empty,
   overrides the user settings with the XML.
   \param content Content type of the path
   \param xml string of XML with the settings.  If non-empty this overrides any saved user settings.
   \return true if settings are available, false otherwise
   \sa GetPathSettings
   */
  bool SetPathSettings(CONTENT_TYPE content, const CStdString& xml);

  /*! \brief Get the scraper settings for a particular path in the form of an XML string
   Loads the default and user settings (if not already loaded) and returns the user settings in the
   form or an XML string
   \return a string containing the XML settings
   \sa SetPathSettings
   */
  CStdString GetPathSettings();

  /*! \brief Clear any previously cached results for this scraper
   Any previously cached files are cleared if they have been cached for longer than the specified
   cachepersistence.
   */
  void ClearCache();

  CONTENT_TYPE Content() const { return m_pathContent; }
  const CStdString& Language() const { return m_language; }
  bool RequiresSettings() const { return m_requiressettings; }
  bool Supports(const CONTENT_TYPE &content) const;

  bool IsInUse() const;

  // scraper media functions
  CScraperUrl NfoUrl(const CStdString &sNfoContent);

  std::vector<CScraperUrl> FindMovie(XFILE::CCurlFile &fcurl,
    const CStdString &sMovie, bool fFirst);
  std::vector<MUSIC_GRABBER::CMusicAlbumInfo> FindAlbum(XFILE::CCurlFile &fcurl,
    const CStdString &sAlbum, const CStdString &sArtist = "");
  std::vector<MUSIC_GRABBER::CMusicArtistInfo> FindArtist(
    XFILE::CCurlFile &fcurl, const CStdString &sArtist);
  VIDEO::EPISODELIST GetEpisodeList(XFILE::CCurlFile &fcurl, const CScraperUrl &scurl);

  bool GetVideoDetails(XFILE::CCurlFile &fcurl, const CScraperUrl &scurl,
    bool fMovie/*else episode*/, CVideoInfoTag &video);
  bool GetAlbumDetails(XFILE::CCurlFile &fcurl, const CScraperUrl &scurl,
    CAlbum &album);
  bool GetArtistDetails(XFILE::CCurlFile &fcurl, const CScraperUrl &scurl,
    const CStdString &sSearch, CArtist &artist);

private:
  CScraper(const CScraper&, const AddonPtr&);
  CStdString SearchStringEncoding() const
    { return m_parser.GetSearchStringEncoding(); }

  bool Load();
  std::vector<CStdString> Run(const CStdString& function,
                              const CScraperUrl& url,
                              XFILE::CCurlFile& http,
                              const std::vector<CStdString>* extras = NULL);
  std::vector<CStdString> RunNoThrow(const CStdString& function,
                              const CScraperUrl& url,
                              XFILE::CCurlFile& http,
                              const std::vector<CStdString>* extras = NULL);
  CStdString InternalRun(const CStdString& function,
                         const CScraperUrl& url,
                         XFILE::CCurlFile& http,
                         const std::vector<CStdString>* extras);

  bool m_fLoaded;
  CStdString m_language;
  bool m_requiressettings;
  CDateTimeSpan m_persistence;
  CONTENT_TYPE m_pathContent;
  CScraperParser m_parser;
};

}

