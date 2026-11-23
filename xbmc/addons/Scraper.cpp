/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Scraper.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "Util.h"
#include "addons/addoninfo/AddonInfo.h"
#include "addons/addoninfo/AddonType.h"
#include "addons/settings/AddonSettings.h"
#include "filesystem/Directory.h"
#include "filesystem/PluginDirectory.h"
#include "music/Album.h"
#include "music/Artist.h"
#include "music/MusicDatabase.h"
#include "music/infoscanner/MusicAlbumInfo.h"
#include "music/infoscanner/MusicArtistInfo.h"
#include "resources/LocalizeStrings.h"
#include "resources/ResourcesComponent.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "settings/SettingsValueFlatJsonSerializer.h"
#include "utils/ArtTypes.h"
#include "utils/ContentNames.h"
#include "utils/JSONVariantWriter.h"
#include "utils/ScraperUrl.h"
#include "utils/Set.h"
#include "utils/StringUtils.h"
#include "utils/XBMCTinyXML.h"
#include "utils/log.h"
#include "video/VideoInfoTag.h"
#include "video/VideoLibrary.h"

#include <algorithm>
#include <array>
#include <map>
#include <sstream>

using namespace XFILE;
using namespace KODI;
using namespace MUSIC_GRABBER;

namespace CONTENT = KODI::MEDIA::CONTENT;

namespace
{
struct ContentMapping
{
  const char *name;
  ADDON::ContentType type;
  int pretty;
};

// clang-format off
const std::array<ContentMapping, 6> content = {{
  {CONTENT::UNKNOWN,     ADDON::ContentType::NONE,        231},
  {CONTENT::ALBUMS,      ADDON::ContentType::ALBUMS,      132},
  {CONTENT::ARTISTS,     ADDON::ContentType::ARTISTS,     133},
  {CONTENT::MOVIES,      ADDON::ContentType::MOVIES,      20342},
  {CONTENT::TVSHOWS,     ADDON::ContentType::TVSHOWS,     20343},
  {CONTENT::MUSICVIDEOS, ADDON::ContentType::MUSICVIDEOS, 20389},
}};
// clang-format on

constexpr std::string_view LOCAL_SCRAPER_ID{"metadata.local"};
} // unnamed namespace

namespace ADDON
{
std::string TranslateContent(ContentType type, bool pretty /*=false*/)
{
  for (const ContentMapping& map : content)
  {
    if (type == map.type)
    {
      if (pretty && map.pretty)
        return CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(map.pretty);
      else
        return map.name;
    }
  }
  return "";
}

ContentType TranslateContent(std::string_view string)
{
  for (const ContentMapping& map : content)
  {
    if (string == map.name)
      return map.type;
  }
  return ContentType::NONE;
}

AddonType ScraperTypeFromContent(ContentType content)
{
  switch (content)
  {
    using enum ADDON::AddonType;
    using enum ContentType;
    case ALBUMS:
      return SCRAPER_ALBUMS;
    case ARTISTS:
      return SCRAPER_ARTISTS;
    case MOVIES:
      return SCRAPER_MOVIES;
    case MUSICVIDEOS:
      return SCRAPER_MUSICVIDEOS;
    case TVSHOWS:
      return SCRAPER_TVSHOWS;
    default:
      return UNKNOWN;
  }
}

CScraper::CScraper(const AddonInfoPtr& addonInfo, AddonType addonType)
  : CAddon(addonInfo, addonType)
{
  m_requiressettings = addonInfo->Type(addonType)->GetValue("@requiressettings").asBoolean();

  switch (addonType)
  {
    using enum ADDON::AddonType;
    using enum ContentType;
    case SCRAPER_ALBUMS:
      m_pathContent = ALBUMS;
      break;
    case SCRAPER_ARTISTS:
      m_pathContent = ARTISTS;
      break;
    case SCRAPER_MOVIES:
      m_pathContent = MOVIES;
      break;
    case SCRAPER_MUSICVIDEOS:
      m_pathContent = MUSICVIDEOS;
      break;
    case SCRAPER_TVSHOWS:
      m_pathContent = TVSHOWS;
      break;
    default:
      break;
  }
}

bool CScraper::Supports(ContentType content) const
{
  return Type() == ScraperTypeFromContent(content);
}

bool CScraper::IsSupported(const CAddonInfo& info)
{
  if (info.ID() == LOCAL_SCRAPER_ID)
    return true;

  static constexpr CSet scraperTypes{
      AddonType::SCRAPER_ALBUMS,      AddonType::SCRAPER_ARTISTS, AddonType::SCRAPER_MOVIES,
      AddonType::SCRAPER_MUSICVIDEOS, AddonType::SCRAPER_TVSHOWS,
  };
  return std::ranges::none_of(
      info.Types(),
      [](const CAddonType& type)
      {
        const std::string library{type.LibName().empty() ? type.GetValue("@library").asString()
                                                         : type.LibName()};
        return scraperTypes.contains(type.Type()) && !library.ends_with(".py");
      });
}

bool CScraper::SetPathSettings(ContentType content, const std::string& xml)
{
  m_pathContent = content;
  if (!LoadSettings(false, false))
    return false;

  if (xml.empty())
    return true;

  CXBMCTinyXML doc;
  doc.Parse(xml);
  return SettingsFromXML(doc, false);
}

std::string CScraper::GetPathSettings()
{
  if (!LoadSettings(false, true))
    return "";

  std::stringstream stream;
  CXBMCTinyXML doc;
  SettingsToXML(doc);
  if (doc.RootElement())
    stream << *doc.RootElement();

  return stream.str();
}

std::string CScraper::GetPathSettingsAsJSON()
{
  static const std::string EmptyPathSettings = "{}";

  if (!LoadSettings(false, true))
    return EmptyPathSettings;

  CSettingsValueFlatJsonSerializer jsonSerializer;
  auto json = jsonSerializer.SerializeValues(GetSettings()->GetSettingsManager());
  if (json.empty())
    return EmptyPathSettings;

  return json;
}

bool CScraper::IsInUse() const
{
  if (Supports(ContentType::ALBUMS) || Supports(ContentType::ARTISTS))
  { // music scraper
    CMusicDatabase db;
    if (db.Open() && db.ScraperInUse(ID()))
      return true;
  }
  else
  { // video scraper
    if (VIDEO::LIBRARY::ScraperInUse(ID()))
      return true;
  }
  return false;
}

bool CScraper::IsLocalInformationOnly() const
{
  return ID() == LOCAL_SCRAPER_ID;
}

namespace
{
//! Run a scraper's script for a listing. Local information only has no script, and lists nothing.
bool RunForListing(const CScraper& scraper, const std::string& url, CFileItemList& items)
{
  if (scraper.IsLocalInformationOnly())
    return true;
  return CDirectory::GetDirectory(url, items, "", DIR_FLAG_DEFAULTS);
}

//! Run a scraper's script for one result. Local information only has no script, and gives none.
bool RunForResult(const CScraper& scraper, const std::string& url, CFileItem& item)
{
  return !scraper.IsLocalInformationOnly() && CPluginDirectory::GetPluginResult(url, item, false);
}
} // unnamed namespace

// pass in contents of .nfo file; returns URL (possibly empty if none found)
CScraperUrl CScraper::NfoUrl(const std::string &sNfoContent)
{
  CScraperUrl scurlRet;

  std::stringstream str;
  str << "plugin://" << ID() << "?action=NfoUrl&nfo=" << CURL::Encode(sNfoContent)
      << "&pathSettings=" << CURL::Encode(GetPathSettingsAsJSON());

  CFileItemList items;
  if (!RunForListing(*this, str.str(), items))
    return scurlRet;

  if (items.IsEmpty())
    return scurlRet;
  if (items.Size() > 1)
    CLog::LogF(LOGWARNING, "Scraper returned multiple results; using first");

  scurlRet.AppendUrl(CScraperUrl::SUrlEntry(items[0]->GetDynPath()));
  return scurlRet;
}

CScraperUrl CScraper::ResolveIDToUrl(const std::string &externalID)
{
  CScraperUrl scurlRet;

  std::stringstream str;
  str << "plugin://" << ID() << "?action=resolveid&key=" << CURL::Encode(externalID)
      << "&pathSettings=" << CURL::Encode(GetPathSettingsAsJSON());

  CFileItem item("resolve me", false);

  if (RunForResult(*this, str.str(), item))
    scurlRet.ParseFromData(item.GetDynPath());

  return scurlRet;
}

namespace
{
template<class T>
T FromFileItem(const CFileItem& item);

template<>
CScraperUrl FromFileItem<CScraperUrl>(const CFileItem &item)
{
  CScraperUrl url;

  url.SetTitle(item.GetLabel());
  if (item.HasProperty("relevance"))
    url.SetRelevance(item.GetProperty("relevance").asDouble());
  CScraperUrl::SUrlEntry surl;
  surl.m_type = CScraperUrl::UrlType::General;
  surl.m_url = item.GetDynPath();
  url.AppendUrl(surl);

  return url;
}

template<>
CMusicAlbumInfo FromFileItem<CMusicAlbumInfo>(const CFileItem &item)
{
  CMusicAlbumInfo info;
  const std::string& sTitle = item.GetLabel();
  std::string sArtist = item.GetProperty("album.artist").asString();
  std::string sAlbumName;
  if (!sArtist.empty())
    sAlbumName = StringUtils::Format("{} - {}", sArtist, sTitle);
  else
    sAlbumName = sTitle;

  CScraperUrl url;
  url.AppendUrl(CScraperUrl::SUrlEntry(item.GetDynPath()));

  info = CMusicAlbumInfo(sTitle, sArtist, sAlbumName, url);
  if (item.HasProperty("relevance"))
    info.SetRelevance(item.GetProperty("relevance").asFloat());

  if (item.HasProperty("album.releasestatus"))
    info.GetAlbum().strReleaseStatus = item.GetProperty("album.releasestatus").asString();
  if (item.HasProperty("album.type"))
    info.GetAlbum().strType = item.GetProperty("album.type").asString();
  if (item.HasProperty("album.year"))
    info.GetAlbum().strReleaseDate = item.GetProperty("album.year").asString();
  if (item.HasProperty("album.label"))
    info.GetAlbum().strLabel = item.GetProperty("album.label").asString();
  info.GetAlbum().art = item.GetArt();

  return info;
}

template<>
CMusicArtistInfo FromFileItem<CMusicArtistInfo>(const CFileItem &item)
{
  CMusicArtistInfo info;
  const std::string& sTitle = item.GetLabel();

  CScraperUrl url;
  url.AppendUrl(CScraperUrl::SUrlEntry(item.GetDynPath()));

  info = CMusicArtistInfo(sTitle, url);
  if (item.HasProperty("artist.genre"))
    info.GetArtist().genre = StringUtils::Split(item.GetProperty("artist.genre").asString(),
                                                CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_musicItemSeparator);
  if (item.HasProperty("artist.disambiguation"))
    info.GetArtist().strDisambiguation = item.GetProperty("artist.disambiguation").asString();
  if (item.HasProperty("artist.type"))
    info.GetArtist().strType = item.GetProperty("artist.type").asString();
  if (item.HasProperty("artist.gender"))
    info.GetArtist().strGender = item.GetProperty("artist.gender").asString();
  if (item.HasProperty("artist.born"))
    info.GetArtist().strBorn = item.GetProperty("artist.born").asString();

  return info;
}

/*!
 \brief Run a python scraper's search, adding what it finds to \p result
 \return false if the search failed: the scraper said so, or ended without a listing
 */
template<class T>
bool PythonFind(const CScraper& scraper,
                const std::map<std::string, std::string, std::less<>>& additionals,
                std::vector<T>& result)
{
  CFileItemList items;
  std::stringstream str;
  str << "plugin://" << scraper.ID() << "?action=find";
  for (const auto& [first, second] : additionals)
    str << "&" << first << "=" << CURL::Encode(second);

  if (!RunForListing(scraper, str.str(), items))
    return false;

  for (const auto& it : items)
    result.emplace_back(std::move(FromFileItem<T>(*it)));

  return true;
}

std::string FromString(const CFileItem& item, const std::string& key)
{
  return item.GetProperty(key).asString();
}

std::vector<std::string> FromArray(const CFileItem& item, const std::string& key, int sep)
{
  return StringUtils::Split(item.GetProperty(key).asString(),
                            sep ? CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_videoItemSeparator
                                : CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_musicItemSeparator);
}

void ParseThumbs(CScraperUrl& scurl, const CFileItem& item, int nThumbs, const std::string& tag)
{
  for (int i = 0; i < nThumbs; ++i)
  {
    std::stringstream prefix;
    prefix << tag << i + 1;
    std::string url = FromString(item, prefix.str() + ".url");
    std::string aspect = FromString(item, prefix.str() + ".aspect");
    std::string preview = FromString(item, prefix.str() + ".preview");
    scurl.AddParsedUrl(url, aspect, preview);
  }
}

std::string ParseFanart(const CFileItem& item, int nFanart, const std::string& tag)
{
  std::string result;
  TiXmlElement fanart("fanart");
  for (int i = 0; i < nFanart; ++i)
  {
    std::stringstream prefix;
    prefix << tag << i + 1;
    std::string url = FromString(item, prefix.str() + ".url");
    std::string preview = FromString(item, prefix.str() + ".preview");
    TiXmlElement thumb("thumb");
    thumb.SetAttribute("preview", preview);
    TiXmlText text(url);
    thumb.InsertEndChild(text);
    fanart.InsertEndChild(thumb);
  }
  result << fanart;

  return result;
}

template<class T>
bool DetailsFromFileItem(const CFileItem&, T&);

template<>
bool DetailsFromFileItem<CAlbum>(const CFileItem& item, CAlbum& album)
{
  album.strAlbum = item.GetLabel();
  album.strMusicBrainzAlbumID = FromString(item, "album.musicbrainzid");
  album.strReleaseGroupMBID = FromString(item, "album.releasegroupid");

  int nArtists = item.GetProperty("album.artists").asInteger32();
  album.artistCredits.reserve(nArtists);
  for (int i = 0; i < nArtists; ++i)
  {
    std::stringstream prefix;
    prefix << "album.artist" << i + 1;
    CArtistCredit artistCredit;
    artistCredit.SetArtist(FromString(item, prefix.str() + ".name"));
    artistCredit.SetMusicBrainzArtistID(FromString(item, prefix.str() + ".musicbrainzid"));
    album.artistCredits.push_back(artistCredit);
  }

  album.strArtistDesc = FromString(item, "album.artist_description");
  album.genre = FromArray(item, "album.genre", 0);
  album.styles = FromArray(item, "album.styles", 0);
  album.moods = FromArray(item, "album.moods", 0);
  album.themes = FromArray(item, "album.themes", 0);
  album.bCompilation = item.GetProperty("album.compilation").asBoolean();
  album.strReview = FromString(item, "album.review");
  album.strReleaseDate = FromString(item, "album.releasedate");
  if (album.strReleaseDate.empty())
    album.strReleaseDate = FromString(item, "album.year");
  album.strOrigReleaseDate = FromString(item, "album.originaldate");
  album.strLabel = FromString(item, "album.label");
  album.strType = FromString(item, "album.type");
  album.strReleaseStatus = FromString(item, "album.releasestatus");
  album.fRating = item.GetProperty("album.rating").asFloat();
  album.iUserrating = item.GetProperty("album.user_rating").asInteger32();
  album.iVotes = item.GetProperty("album.votes").asInteger32();
  
  /* Scrapers fetch a list of possible art but do not set the current images used because art
     selection depends on other preferences so is handled by CMusicInfoScanner
     album.art = item.GetArt();
  */

  int nThumbs = item.GetProperty("album.thumbs").asInteger32();
  ParseThumbs(album.thumbURL, item, nThumbs, "album.thumb");
  return true;
}

template<>
bool DetailsFromFileItem<CArtist>(const CFileItem& item, CArtist& artist)
{
  artist.strArtist = item.GetLabel();
  artist.strMusicBrainzArtistID = FromString(item, "artist.musicbrainzid");
  artist.strDisambiguation = FromString(item, "artist.disambiguation");
  artist.strType = FromString(item, "artist.type");
  artist.strGender = FromString(item, "artist.gender");
  artist.genre = FromArray(item, "artist.genre", 0);
  artist.styles = FromArray(item, "artist.styles", 0);
  artist.moods = FromArray(item, "artist.moods", 0);
  artist.yearsActive = FromArray(item, "artist.years_active", 0);
  artist.instruments = FromArray(item, "artist.instruments", 0);
  artist.strBorn = FromString(item, "artist.born");
  artist.strFormed = FromString(item, "artist.formed");
  artist.strBiography = FromString(item, "artist.biography");
  artist.strDied = FromString(item, "artist.died");
  artist.strDisbanded = FromString(item, "artist.disbanded");

  /* Scrapers fetch a list of possible art but do not set the current images used because art
     selection depends on other preferences so is handled by CMusicInfoScanner
     artist.art = item.GetArt();
  */

  int nAlbums = item.GetProperty("artist.albums").asInteger32();
  artist.discography.reserve(nAlbums);
  for (int i = 0; i < nAlbums; ++i)
  {
    std::stringstream prefix;
    prefix << "artist.album" << i + 1;
    CDiscoAlbum discoAlbum;
    discoAlbum.strAlbum = FromString(item, prefix.str() + ".title");
    discoAlbum.strYear = FromString(item, prefix.str() + ".year");
    discoAlbum.strReleaseGroupMBID = FromString(item, prefix.str() + ".musicbrainzreleasegroupid");
    artist.discography.emplace_back(discoAlbum);
  }

  const int numvideolinks = item.GetProperty("artist.videolinks").asInteger32();
  if (numvideolinks > 0)
  {
    artist.videolinks.reserve(numvideolinks);
    for (int i = 1; i <= numvideolinks; ++i)
    {
      std::stringstream prefix;
      prefix << "artist.videolink" << i;
      ArtistVideoLinks videoLink;
      videoLink.title = FromString(item, prefix.str() + ".title");
      videoLink.mbTrackID = FromString(item, prefix.str() + ".mbtrackid");
      videoLink.videoURL = FromString(item, prefix.str() + ".url");
      videoLink.thumbURL = FromString(item, prefix.str() + ".thumb");
      artist.videolinks.emplace_back(std::move(videoLink));
    }
  }

  int nThumbs = item.GetProperty("artist.thumbs").asInteger32();
  ParseThumbs(artist.thumbURL, item, nThumbs, "artist.thumb");

  // Support deprecated fanarts property, add to artist.thumbURL
  int nFanart = item.GetProperty("artist.fanarts").asInteger32();
  if (nFanart > 0)
  {
    CFanart fanart;
    fanart.m_xml = ParseFanart(item, nFanart, "artist.fanart");
    fanart.Unpack();
    for (unsigned int i = 0; i < fanart.GetNumFanarts(); i++)
      artist.thumbURL.AddParsedUrl(fanart.GetImageURL(i), ART::TYPE::FANART,
                                   fanart.GetPreviewURL(i));
  }
  return true;
}

template<>
bool DetailsFromFileItem<CVideoInfoTag>(const CFileItem& item, CVideoInfoTag& tag)
{
  if (item.HasVideoInfoTag())
  {
    tag = *item.GetVideoInfoTag();
    return true;
  }
  return false;
}

template<class T>
bool PythonDetails(const CScraper& scraper,
                   const std::string& key,
                   const std::string& url,
                   const std::string& action,
                   const std::string& pathSettings,
                   const CScraper::UniqueIDs& uniqueIDs,
                   T& result)
{
  CVariant ids;
  for (const auto& [identifierType, identifier] : uniqueIDs)
    ids[identifierType] = identifier;
  std::string uids;
  CJSONVariantWriter::Write(ids, uids, true);
  std::stringstream str;
  str << "plugin://" << scraper.ID() << "?action=" << action << "&" << key << "="
      << CURL::Encode(url);
  str << "&pathSettings=" << CURL::Encode(pathSettings);
  if (!uniqueIDs.empty())
    str << "&uniqueIDs=" << CURL::Encode(uids);

  CFileItem item(url, false);

  if (!RunForResult(scraper, str.str(), item))
    return false;

  return DetailsFromFileItem(item, result);
}

template<class T>
bool PythonDetails(const CScraper& scraper,
                   const std::string& key,
                   const std::string& url,
                   const std::string& action,
                   const std::string& pathSettings,
                   T& result)
{
  const ADDON::CScraper::UniqueIDs ids;
  return PythonDetails(scraper, key, url, action, pathSettings, ids, result);
}
//! \brief Length prefixed, so that no separator can appear in what it is separating
std::string Keyed(std::string_view value)
{
  return StringUtils::Format("{}:{}", value.size(), value);
}

//! \brief Key a set of unique ids into something a result cache can look up
std::string SerialiseUniqueIDs(const ADDON::CScraper::UniqueIDs& uniqueIDs)
{
  std::vector<std::string> ids;
  ids.reserve(uniqueIDs.size());
  for (const auto& [type, value] : uniqueIDs)
    ids.emplace_back(Keyed(type) + Keyed(value));

  std::ranges::sort(ids); // the ids are unordered, the key cannot be
  return StringUtils::Join(ids, "");
}
} // unnamed namespace

std::vector<CScraperUrl> CScraper::FindMovie(const std::string& movieTitle, int movieYear)
{
  // The title is cleaned below, so the arguments as given identify the search
  const std::string key{StringUtils::Format("{}\n{}", movieTitle, movieYear)};
  if (const auto it = m_findCache.find(key); it != m_findCache.end())
    return it->second;

  auto result = FindMovieUncached(movieTitle, movieYear, true);
  // nothing found: try again without cleaning characters like '.' and '_'
  if (result.empty())
    result = FindMovieUncached(movieTitle, movieYear, false);

  if (!result.empty())
  {
    if (m_findCache.size() >= MAX_CACHED_RESULTS)
      m_findCache.clear();
    m_findCache.emplace(key, result);
  }

  return result;
}

std::vector<CScraperUrl> CScraper::FindMovieUncached(const std::string& movieTitle,
                                                     int movieYear,
                                                     bool fFirst)
{
  // prepare parameters for URL creation
  std::string sTitle;
  std::string sYear;
  if (movieYear < 0)
  {
    std::string sTitleYear;
    CUtil::CleanString(movieTitle, sTitle, sTitleYear, sYear, true /*fRemoveExt*/, fFirst);
  }
  else
  {
    sTitle = movieTitle;
    sYear = std::to_string( movieYear );
  }

  CLog::LogF(LOGDEBUG,
             "Searching for '{}' using {} scraper (path: '{}', content: '{}', version: '{}')",
             sTitle, Name(), Path(), Content(), Version().asString());

  std::vector<CScraperUrl> vcscurl;
  if (!fFirst)
    StringUtils::Replace(sTitle, '-', ' ');

  std::map<std::string, std::string, std::less<>> additionals{{"title", sTitle}};
  if (!sYear.empty())
    additionals.try_emplace("year", sYear);
  additionals.try_emplace("pathSettings", GetPathSettingsAsJSON());
  if (!PythonFind(*this, additionals, vcscurl))
  {
    CLog::LogF(LOGDEBUG, "Search for '{}' using {} scraper failed", sTitle, Name());
    throw CScraperError();
  }
  return vcscurl;
}

std::vector<CMusicAlbumInfo> CScraper::FindAlbum(const std::string& sAlbum,
                                                 const std::string& sArtist)
{
  CLog::LogF(LOGDEBUG,
             "Searching for '{} - {}' using {} scraper (path: '{}', content: '{}', version: '{}')",
             sArtist, sAlbum, Name(), Path(), Content(), Version().asString());

  std::vector<CMusicAlbumInfo> vcali;
  // A failed search finds nothing here, as the music scanner doesn't catch an error
  PythonFind(*this,
             {{"title", sAlbum}, {"artist", sArtist}, {"pathSettings", GetPathSettingsAsJSON()}},
             vcali);
  return vcali;
}

std::vector<CMusicArtistInfo> CScraper::FindArtist(const std::string& sArtist)
{
  CLog::LogF(LOGDEBUG,
             "Searching for '{}' using {} scraper (file: '{}', content: '{}', version: '{}')",
             sArtist, Name(), Path(), Content(), Version().asString());

  std::vector<CMusicArtistInfo> vcari;
  // A failed search finds nothing here, as the music scanner doesn't catch an error
  PythonFind(*this, {{"artist", sArtist}, {"pathSettings", GetPathSettingsAsJSON()}}, vcari);
  return vcari;
}

// fetch list of episodes from URL (from video database)
VIDEO::EPISODELIST CScraper::GetEpisodeList(const CScraperUrl& scurl)
{
  VIDEO::EPISODELIST vcep;
  if (!scurl.HasUrls())
    return vcep;

  CLog::LogF(LOGDEBUG, "Searching '{}' using {} scraper (file: '{}', content: '{}', version: '{}')",
             scurl.GetFirstThumbUrl(), Name(), Path(), Content(), Version().asString());

  std::stringstream str;
  str << "plugin://" << ID()
      << "?action=getepisodelist&url=" << CURL::Encode(scurl.GetFirstThumbUrl())
      << "&pathSettings=" << CURL::Encode(GetPathSettingsAsJSON());

  CFileItemList items;
  if (!RunForListing(*this, str.str(), items))
    return vcep;

  vcep.reserve(items.Size());
  for (int i = 0; i < items.Size(); ++i)
  {
    auto& ep = vcep.emplace_back();
    const auto& tag = *items[i]->GetVideoInfoTag();
    ep.strTitle = tag.m_strTitle;
    ep.iSeason = tag.m_iSeason;
    ep.iEpisode = tag.m_iEpisode;
    ep.cDate = tag.m_firstAired;
    ep.iSubepisode = static_cast<int>(items[i]->GetProperty("video.sub_episode").asInteger());
    ep.cScraperUrl.AppendUrl(CScraperUrl::SUrlEntry(items[i]->GetURL().Get()));
  }

  return vcep;
}

// takes URL; returns true and populates video details on success, false otherwise
bool CScraper::GetVideoDetails(const UniqueIDs& uniqueIDs,
                               const CScraperUrl& scurl,
                               bool fMovie /*else episode*/,
                               CVideoInfoTag& video)
{
  const std::string key{Keyed(scurl.GetFirstThumbUrl()) + (fMovie ? "m" : "e") +
                        SerialiseUniqueIDs(uniqueIDs)};
  if (const auto it = m_detailsCache.find(key); it != m_detailsCache.end())
  {
    // A copy - the caller goes on to fill in where this came from and what art it ended up with
    video = *it->second;
    return true;
  }

  if (!GetVideoDetailsUncached(uniqueIDs, scurl, fMovie, video))
    return false;

  if (m_detailsCache.size() >= MAX_CACHED_RESULTS)
    m_detailsCache.clear();
  m_detailsCache.emplace(key, std::make_shared<const CVideoInfoTag>(video));

  return true;
}

bool CScraper::GetVideoDetailsUncached(const UniqueIDs& uniqueIDs,
                                       const CScraperUrl& scurl,
                                       bool fMovie,
                                       CVideoInfoTag& video)
{
  CLog::LogF(LOGDEBUG,
             "Reading {} '{}' using {} scraper (file: '{}', content: '{}', version: '{}')",
             fMovie ? MEDIA::TYPE::MOVIE : MEDIA::TYPE::EPISODE, scurl.GetFirstThumbUrl(), Name(),
             Path(), Content(), Version().asString());

  video.Reset();

  const bool fRet{PythonDetails(*this, "url", scurl.GetFirstThumbUrl(),
                                fMovie ? "getdetails" : "getepisodedetails",
                                GetPathSettingsAsJSON(), uniqueIDs, video)};

  video.m_streamDetails.Reset(); // Scrapers should not return streamdetails

  return fRet;
}

// takes a URL; returns true and populates album on success, false otherwise
bool CScraper::GetAlbumDetails(const CScraperUrl& scurl, CAlbum& album)
{
  CLog::LogF(LOGDEBUG, "Reading '{}' using {} scraper (file: '{}', content: '{}', version: '{}')",
             scurl.GetFirstThumbUrl(), Name(), Path(), Content(), Version().asString());

  return PythonDetails(*this, "url", scurl.GetFirstThumbUrl(), "getdetails",
                       GetPathSettingsAsJSON(), album);
}

bool CScraper::GetArtistDetails(const CScraperUrl& scurl, CArtist& artist)
{
  if (!scurl.HasUrls())
    return false;

  CLog::LogF(LOGDEBUG, "Reading '{}' using {} scraper (file: '{}', content: '{}', version: '{}')",
             scurl.GetFirstThumbUrl(), Name(), Path(), Content(), Version().asString());

  return PythonDetails(*this, "url", scurl.GetFirstThumbUrl(), "getdetails",
                       GetPathSettingsAsJSON(), artist);
}

bool CScraper::GetArtwork(CVideoInfoTag& details)
{
  if (!details.HasUniqueID())
    return false;

  CLog::LogF(LOGDEBUG,
             "Reading artwork for '{}' using {} scraper (file: '{}', content: '{}', version: '{}')",
             details.GetUniqueID(), Name(), Path(), Content(), Version().asString());

  return PythonDetails(*this, "id", details.GetUniqueID(), "getartwork", GetPathSettingsAsJSON(),
                       details);
}
} // namespace ADDON
