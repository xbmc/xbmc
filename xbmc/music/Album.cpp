/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Album.h"

#include "FileItem.h"
#include "ServiceBroker.h"
#include "music/tags/MusicInfoTag.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "utils/MathUtils.h"
#include "utils/StringUtils.h"
#include "utils/XMLUtils.h"
#include "utils/log.h"

#include <algorithm>
#include <string_view>

using namespace MUSIC_INFO;

CAlbum::CAlbum(const CFileItem& item)
{
  Reset();
  const CMusicInfoTag& tag = *item.GetMusicInfoTag();
  strAlbum = tag.GetAlbum();
  strMusicBrainzAlbumID = tag.GetMusicBrainzAlbumID();
  strReleaseGroupMBID = tag.GetMusicBrainzReleaseGroupID();
  genre = tag.GetGenre();
  strArtistDesc = tag.GetAlbumArtistString();
  //Set sort string before processing artist credits
  strArtistSort = tag.GetAlbumArtistSort();
  // Determine artist credits from various tag arrays, inc fallback to song artist names
  SetArtistCredits(tag.GetAlbumArtist(), tag.GetMusicBrainzAlbumArtistHints(), tag.GetMusicBrainzAlbumArtistID(),
                   tag.GetArtist(), tag.GetMusicBrainzArtistHints(), tag.GetMusicBrainzArtistID());

  strOrigReleaseDate = tag.GetOriginalDate();
  strReleaseDate = tag.GetReleaseDate();
  strLabel = tag.GetRecordLabel();
  strType = tag.GetMusicBrainzReleaseType();
  bCompilation = tag.GetCompilation();
  iTimesPlayed = 0;
  bBoxedSet = tag.GetBoxset();
  dateAdded.Reset();
  dateUpdated.Reset();
  lastPlayed.Reset();
  releaseType = tag.GetAlbumReleaseType();
  strReleaseStatus = tag.GetAlbumReleaseStatus();
}

void CAlbum::SetArtistCredits(const std::vector<std::string>& names, const std::vector<std::string>& hints,
                              const std::vector<std::string>& mbids,
                              const std::vector<std::string>& artistnames, const std::vector<std::string>& artisthints,
                              const std::vector<std::string>& artistmbids)
{
  using namespace std::literals;
  std::vector<std::string> albumartistHints = hints;
  //Split the artist sort string to try and get sort names for individual artists
  auto artistSort = StringUtils::Split(strArtistSort, CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_musicItemSeparator);
  artistCredits.clear();

  if (!mbids.empty())
  { // Have musicbrainz artist info, so use it

    // Vector of possible separators in the order least likely to be part of artist name
    const std::vector<std::string> separators{ " feat. ", ";", ":", "|", "#", "/", ",", "&" };

    // Establish tag consistency
    // Do the number of musicbrainz ids and *both* number of names and number of hints mismatch?
    if (albumartistHints.size() != mbids.size() && names.size() != mbids.size())
    {
      // Tags mismatch - report it and then try to fix
      CLog::Log(LOGDEBUG, "Mismatch in song file albumartist tags: {} mbid {} name album: {} {}",
                (int)mbids.size(), (int)names.size(), strAlbum, strArtistDesc);
      /*
      Most likely we have no hints and a single artist name like "Artist1 feat. Artist2"
      or "Composer; Conductor, Orchestra, Soloist" or "Artist1/Artist2" where the
      expected single item separator (default = space-slash-space) as not been used.
      Comma and slash (no spaces) are poor delimiters as could be in name e.g. AC/DC,
      but here treat them as such in attempt to find artist names.
      When there are hints they could be poorly formatted using unexpected separators,
      so attempt to split them. Or we could have more hints or artist names than
      musicbrainz so ignore them but raise warning.
      */

      // Do hints exist yet mismatch
      if (!albumartistHints.empty() && albumartistHints.size() != mbids.size())
      {
        if (names.size() == mbids.size())
          // Album artist name count matches, use that as hints
          albumartistHints = names;
        else if (albumartistHints.size() < mbids.size())
        { // Try splitting the hints until have matching number
          albumartistHints = StringUtils::SplitMulti(albumartistHints, separators, mbids.size());
        }
        else
          // Extra hints, discard them.
          albumartistHints.resize(mbids.size());
      }
      // Do hints not exist or still mismatch, try album artists
      if (albumartistHints.size() != mbids.size())
        albumartistHints = names;
      // Still mismatch, try splitting the hints (now artists) until have matching number
      if (albumartistHints.size() < mbids.size())
        albumartistHints = StringUtils::SplitMulti(albumartistHints, separators, mbids.size());
      // Try matching on artists or artist hints field, if it is reliable
      if (albumartistHints.size() != mbids.size())
      {
        if (!artistmbids.empty() &&
          (artistmbids.size() == artistnames.size() ||
            artistmbids.size() == artisthints.size()))
        {
          for (size_t i = 0; i < mbids.size(); i++)
          {
            for (size_t j = 0; j < artistmbids.size(); j++)
            {
              if (mbids[i] == artistmbids[j])
              {
                if (albumartistHints.size() < i + 1)
                  albumartistHints.resize(i + 1);
                if (artistmbids.size() == artisthints.size())
                  albumartistHints[i] = artisthints[j];
                else
                  albumartistHints[i] = artistnames[j];
              }
            }
          }
        }
      }
    }
    else
    { // Either hints or album artists (or both) name matches number of musicbrainz id
      // If hints mismatch, use album artists
      if (albumartistHints.size() != mbids.size())
        albumartistHints = names;
    }

    // Try to get number of artist sort names and musicbrainz ids to match. Split sort names
    // further using multiple possible delimiters, over single separator applied in Tag loader
    if (artistSort.size() != mbids.size())
    {
      artistSort = StringUtils::SplitMulti(artistSort, {{";"sv, ":"sv, "|"sv, "#"sv}});
    }

    for (size_t i = 0; i < mbids.size(); i++)
    {
      std::string artistId = mbids[i];
      std::string artistName;
      /*
         We try and get the musicbrainz id <-> name matching from the hints and match on the same index.
         Some album artist hints could be blank (if populated from artist or artist hints).
         If not found, use the musicbrainz id and hope we later on can update that entry.
         If we have more names than musicbrainz id they are ignored, but raise a warning.
      */
      if (i < albumartistHints.size())
        artistName = albumartistHints[i];
      if (artistName.empty())
        artistName = artistId;

      // Use artist sort name providing we have as many as we have mbid,
      // otherwise something is wrong with them so ignore and leave blank
      if (artistSort.size() == mbids.size())
        artistCredits.emplace_back(StringUtils::Trim(artistName), StringUtils::Trim(artistSort[i]), artistId);
      else
        artistCredits.emplace_back(StringUtils::Trim(artistName), "", artistId);
    }
  }
  else
  {
    /*
      No musicbrainz album artist ids so fill artist names directly.
      This method only called during scanning when there is a musicbrainz album id, so
      means mbid tags are incomplete. But could also be called by JSON to SetAlbumDetails
      Try to separate album artist names further, and trim blank space.
    */
    std::vector<std::string> albumArtists = names;
    if (albumartistHints.size() > albumArtists.size())
      // Make use of hints (ALBUMARTISTS tag), when present, to separate artist names
      albumArtists = albumartistHints;
    else
      // Split album artist names further using multiple possible delimiters, over single separator applied in Tag loader
      albumArtists = StringUtils::SplitMulti(albumArtists, CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_musicArtistSeparators);

    if (artistSort.size() != albumArtists.size())
    {
      // Split artist sort names further using multiple possible delimiters, over single separator applied in Tag loader
      artistSort = StringUtils::SplitMulti(artistSort, {{";"sv, ":"sv, "|"sv, "#"sv}});
    }

    for (size_t i = 0; i < albumArtists.size(); i++)
    {
      artistCredits.emplace_back(StringUtils::Trim(albumArtists[i]));
      // Set artist sort name providing we have as many as we have artists,
      // otherwise something is wrong with them so ignore rather than guess.
      if (artistSort.size() == albumArtists.size())
        artistCredits.back().SetSortName(StringUtils::Trim(artistSort[i]));
    }
  }
}

void CAlbum::MergeScrapedAlbum(const CAlbum& source, bool override /* = true */)
{
  /*
   Initial scraping of album information when there is a Musicbrainz album ID derived from
   tags is done directly using that ID, otherwise the lookup is based on album and artist names
   but this can sometimes mis-identify the album (i.e. classical music has many "Symphony No. 5").
   It is useful to store the scraped mbid, but we need to be able to correct any mistakes. Hence
   a manual refresh of album information uses either the mbid as derived from tags or the album
   and artist names, not any previously scraped mbid.

   When overwriting the data derived from tags, AND the original and scraped album have the same
   Musicbrainz album ID, then merging is used to keep Kodi up to date with changes in the Musicbrainz
   database including album artist credits, song artist credits and song titles. However it is only
   appropriate when the music files are tagged with mbids, these are taken as definative, scraped
   mbids can not be depended on in this way.

   When the album is merged in this deep way it is flagged so that the database album update is aware
   artist credits and songs need to be updated too.
  */

  bArtistSongMerge = override && !bScrapedMBID
    && !source.strMusicBrainzAlbumID.empty() && !strMusicBrainzAlbumID.empty()
    && (strMusicBrainzAlbumID.compare(source.strMusicBrainzAlbumID) == 0);

  /*
   Musicbrainz album (release) ID and release group ID values derived from music file tags are
   always taken as accurate and so can not be overwritten by a scraped value. When the album does
   not already have an mbid or has a previously scraped mbid, merge the new scraped value,
   flagging it as being from the scraper rather than derived from music file tags.
  */
  if (!source.strMusicBrainzAlbumID.empty() && (strMusicBrainzAlbumID.empty() || bScrapedMBID))
  {
    strMusicBrainzAlbumID = source.strMusicBrainzAlbumID;
    bScrapedMBID = true;
  }
  if (!source.strReleaseGroupMBID.empty() && (strReleaseGroupMBID.empty() || bScrapedMBID))
  {
    strReleaseGroupMBID = source.strReleaseGroupMBID;
  }

  /*
   Scraping can return different album artists from the originals derived from tags, even when
   doing a lookup on artist name.

   When overwriting the data derived from tags, AND the original and scraped album have the same
   Musicbrainz album ID, then merging an album replaces both the album artsts and the song artists
   with those scraped (providing they are not empty).

   When not doing that kind of merge, for any matching artist names the Musicbrainz artist id
   returned by the scraper can be used to populate any previously missing Musicbrainz artist id values.
  */
  if (bArtistSongMerge && !source.artistCredits.empty())
  {
    artistCredits = source.artistCredits; // Replace artists and store mbid returned by scraper
    strArtistDesc.clear();  // @todo: set artist display string e.g. "artist1 & artist2" when scraped
  }
  else
  {
    // Compare original album artists with those scraped (ignoring order), and set any missing mbid
    for (auto &artistCredit : artistCredits)
    {
      if (artistCredit.GetMusicBrainzArtistID().empty())
      {
        for (const auto& sourceartistCredit : source.artistCredits)
        {
          if (StringUtils::EqualsNoCase(artistCredit.GetArtist(), sourceartistCredit.GetArtist()))
          {
            artistCredit.SetMusicBrainzArtistID(sourceartistCredit.GetMusicBrainzArtistID());
            artistCredit.SetScrapedMBID(true);
            break;
          }
        }
      }
    }
  }

  //@todo: scraped album genre needs adding to genre and album_genre tables, this just changes the string
  if ((override && !source.genre.empty()) || genre.empty())
    genre = source.genre;
  if ((override && !source.strAlbum.empty()) || strAlbum.empty())
    strAlbum = source.strAlbum;
  //@todo: validate ISO8601 format YYYY, YYYY-MM, or YYYY-MM-DD
  if ((override && !source.strReleaseDate.empty()) || strReleaseDate.empty())
    strReleaseDate = source.strReleaseDate;
  if ((override && !source.strOrigReleaseDate.empty()) || strOrigReleaseDate.empty())
    strOrigReleaseDate = source.strOrigReleaseDate;

  if (override)
    bCompilation = source.bCompilation;
  //  iTimesPlayed = source.iTimesPlayed; // times played is derived from songs

  if ((override && !source.strArtistSort.empty()) || strArtistSort.empty())
    strArtistSort = source.strArtistSort;
  for (const auto& i : source.art)
  {
    if (override || !art.contains(i.first))
      art[i.first] = i.second;
  }
  if((override && !source.strLabel.empty()) || strLabel.empty())
    strLabel = source.strLabel;
  thumbURL = source.thumbURL;
  moods = source.moods;
  styles = source.styles;
  themes = source.themes;
  strReview = source.strReview;
  if ((override && !source.strType.empty()) || strType.empty())
    strType = source.strType;
//  strPath = source.strPath; // don't merge the path
  if ((override && !source.strReleaseStatus.empty()) || strReleaseStatus.empty())
    strReleaseStatus = source.strReleaseStatus;
  fRating = source.fRating;
  iUserrating = source.iUserrating;
  iVotes = source.iVotes;

  /*
   When overwriting the data derived from tags, AND the original and scraped album have the same
   Musicbrainz album ID, update the local songs with scaped Musicbrainz information including the
   artist credits.
  */
  if (bArtistSongMerge)
  {
    for (auto &song : songs)
    {
      if (!song.strMusicBrainzTrackID.empty())
        for (const auto& sourceSong : source.songs)
          if ((sourceSong.strMusicBrainzTrackID == song.strMusicBrainzTrackID) && (sourceSong.iTrack == song.iTrack))
            song.MergeScrapedSong(sourceSong, override);
    }
  }
}

std::string CAlbum::GetGenreString() const
{
  return StringUtils::Join(genre, CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_musicItemSeparator);
}

const std::vector<std::string> CAlbum::GetAlbumArtist() const
{
  //Get artist names as vector from artist credits
  std::vector<std::string> albumartists;
  for (const auto& artistCredit : artistCredits)
  {
    albumartists.push_back(artistCredit.GetArtist());
  }
  return albumartists;
}

const std::vector<std::string> CAlbum::GetMusicBrainzAlbumArtistID() const
{
  //Get artist MusicBrainz IDs as vector from artist credits
  std::vector<std::string> musicBrainzID;
  for (const auto& artistCredit : artistCredits)
  {
    musicBrainzID.push_back(artistCredit.GetMusicBrainzArtistID());
  }
  return musicBrainzID;
}

const std::string CAlbum::GetAlbumArtistString() const
{
  //Artist description may be different from the artists in artistcredits (see ALBUMARTISTS tag processing)
  //but is takes precedence as a string because artistcredits is not always filled during processing
  if (!strArtistDesc.empty())
    return strArtistDesc;
  std::vector<std::string> artistvector;
  for (const auto& i : artistCredits)
    artistvector.emplace_back(i.GetArtist());
  std::string artistString;
  if (!artistvector.empty())
    artistString = StringUtils::Join(artistvector, CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_musicItemSeparator);
  return artistString;
}

const std::string CAlbum::GetAlbumArtistSort() const
{
  //The stored artist sort name string takes precedence but a
  //value could be created from individual sort names held in artistcredits
  if (!strArtistSort.empty())
    return strArtistSort;
  std::vector<std::string> artistvector;
  for (const auto& artistcredit : artistCredits)
    if (!artistcredit.GetSortName().empty())
      artistvector.emplace_back(artistcredit.GetSortName());
  std::string artistString;
  if (!artistvector.empty())
    artistString = StringUtils::Join(artistvector, "; ");
  return artistString;
}

const std::vector<int> CAlbum::GetArtistIDArray() const
{
  // Get album artist IDs for json rpc
  std::vector<int> artistids;
  for (const auto& artistCredit : artistCredits)
    artistids.push_back(artistCredit.GetArtistId());
  return artistids;
}


std::string CAlbum::GetReleaseType() const
{
  return AudioType::ToString(releaseType);
}

void CAlbum::SetReleaseType(const std::string& strReleaseType)
{
  releaseType = AudioType::FromString(strReleaseType);
}

void CAlbum::SetDateAdded(const std::string& strDateAdded)
{
  dateAdded.SetFromDBDateTime(strDateAdded);
}

void CAlbum::SetDateUpdated(const std::string& strDateUpdated)
{
  dateUpdated.SetFromDBDateTime(strDateUpdated);
}

void CAlbum::SetDateNew(const std::string& strDateNew)
{
  dateNew.SetFromDBDateTime(strDateNew);
}

void CAlbum::SetLastPlayed(const std::string& strLastPlayed)
{
  lastPlayed.SetFromDBDateTime(strLastPlayed);
}

bool CAlbum::operator<(const CAlbum &a) const
{
  if (strMusicBrainzAlbumID.empty() && a.strMusicBrainzAlbumID.empty())
  {
    if (strAlbum < a.strAlbum) return true;
    if (strAlbum > a.strAlbum) return false;

    // This will do an std::vector compare (i.e. item by item)
    if (GetAlbumArtist() < a.GetAlbumArtist()) return true;
    if (GetAlbumArtist() > a.GetAlbumArtist()) return false;
    return false;
  }

  if (strMusicBrainzAlbumID < a.strMusicBrainzAlbumID) return true;
  if (strMusicBrainzAlbumID > a.strMusicBrainzAlbumID) return false;
  return false;
}

bool CAlbum::Load(const TiXmlElement *album, bool append, bool prioritise)
{
  if (!album) return false;
  if (!append)
    Reset();

  const std::string itemSeparator = CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_musicItemSeparator;

  XMLUtils::GetString(album,              "title", strAlbum);
  XMLUtils::GetString(album, "musicbrainzalbumid", strMusicBrainzAlbumID);
  XMLUtils::GetString(album, "musicbrainzreleasegroupid", strReleaseGroupMBID);
  XMLUtils::GetBoolean(album, "scrapedmbid", bScrapedMBID);
  XMLUtils::GetString(album, "artistdesc", strArtistDesc);
  std::vector<std::string> artist; // Support old style <artist></artist> for backwards compatibility
  XMLUtils::GetStringArray(album, "artist", artist, prioritise, itemSeparator);
  XMLUtils::GetStringArray(album, "genre", genre, prioritise, itemSeparator);
  XMLUtils::GetStringArray(album, "style", styles, prioritise, itemSeparator);
  XMLUtils::GetStringArray(album, "mood", moods, prioritise, itemSeparator);
  XMLUtils::GetStringArray(album, "theme", themes, prioritise, itemSeparator);
  XMLUtils::GetBoolean(album, "compilation", bCompilation);
  XMLUtils::GetBoolean(album, "boxset", bBoxedSet);

  XMLUtils::GetString(album,"review",strReview);
  XMLUtils::GetString(album,"label",strLabel);
  XMLUtils::GetInt(album, "duration", iAlbumDuration);
  XMLUtils::GetString(album,"type",strType);
  XMLUtils::GetString(album, "releasestatus", strReleaseStatus);

  XMLUtils::GetString(album, "releasedate", strReleaseDate);
  StringUtils::Trim(strReleaseDate);  // @todo: validate ISO8601 format
  // Support old style <year></year> for backwards compatibility
  if (strReleaseDate.empty())
  {
    int year;
    XMLUtils::GetInt(album, "year", year);
    if (year > 0)
      strReleaseDate = StringUtils::Format("{:04}", year);
  }
  XMLUtils::GetString(album, "originalreleasedate", strOrigReleaseDate);

  const TiXmlElement* rElement = album->FirstChildElement("rating");
  if (rElement)
  {
    float rating = 0;
    float max_rating = 10;
    XMLUtils::GetFloat(album, "rating", rating);
    if (rElement->QueryFloatAttribute("max", &max_rating) == TIXML_SUCCESS && max_rating>=1)
      rating *= (10.f / max_rating); // Normalise the Rating to between 0 and 10
    if (rating > 10.f)
      rating = 10.f;
    fRating = rating;
  }
  const TiXmlElement* userrating = album->FirstChildElement("userrating");
  if (userrating)
  {
    float rating = 0;
    float max_rating = 10;
    XMLUtils::GetFloat(album, "userrating", rating);
    if (userrating->QueryFloatAttribute("max", &max_rating) == TIXML_SUCCESS && max_rating >= 1)
      rating *= (10.f / max_rating); // Normalise the Rating to between 0 and 10
    if (rating > 10.f)
      rating = 10.f;
    iUserrating = MathUtils::round_int(static_cast<double>(rating));
  }
  XMLUtils::GetInt(album, "votes", iVotes);

  size_t iThumbCount = thumbURL.GetUrls().size();
  std::string xmlAdd = thumbURL.GetData();
  const TiXmlElement* thumb = album->FirstChildElement("thumb");
  while (thumb)
  {
    thumbURL.ParseAndAppendUrl(thumb);
    if (prioritise)
    {
      std::string temp;
      temp << *thumb;
      xmlAdd = temp+xmlAdd;
    }
    thumb = thumb->NextSiblingElement("thumb");
  }
  // prioritise thumbs from nfos
  if (prioritise && iThumbCount && iThumbCount != thumbURL.GetUrls().size())
  {
    auto thumbUrls = thumbURL.GetUrls();
    rotate(thumbUrls.begin(), thumbUrls.begin() + iThumbCount, thumbUrls.end());
    thumbURL.SetUrls(thumbUrls);
    thumbURL.SetData(xmlAdd);
  }

  const TiXmlElement* albumArtistCreditsNode = album->FirstChildElement("albumArtistCredits");
  if (albumArtistCreditsNode)
    artistCredits.clear();

  while (albumArtistCreditsNode)
  {
    if (albumArtistCreditsNode->FirstChild())
    {
      CArtistCredit artistCredit;
      XMLUtils::GetString(albumArtistCreditsNode,  "artist",               artistCredit.m_strArtist);
      XMLUtils::GetString(albumArtistCreditsNode,  "musicBrainzArtistID",  artistCredit.m_strMusicBrainzArtistID);
      artistCredits.push_back(artistCredit);
    }

    albumArtistCreditsNode = albumArtistCreditsNode->NextSiblingElement("albumArtistCredits");
  }

  // Support old style <artist></artist> for backwards compatibility
  // .nfo files should ideally be updated to use the artist credits structure above
  // or removed entirely in preference for better tags (MusicBrainz?)
  if (artistCredits.empty() && !artist.empty())
  {
    for (const auto& it : artist)
    {
      CArtistCredit artistCredit(it);
      artistCredits.push_back(artistCredit);
    }
  }

  std::string strReleaseType;
  if (XMLUtils::GetString(album, "releasetype", strReleaseType))
    SetReleaseType(strReleaseType);
  else
    releaseType = AudioType::Type::Album;

  return true;
}

bool CAlbum::Save(TiXmlNode *node, const std::string &tag, const std::string& strPath)
{
  if (!node) return false;

  // we start with a <tag> tag
  TiXmlElement albumElement(tag.c_str());
  TiXmlNode *album = node->InsertEndChild(albumElement);

  if (!album) return false;

  XMLUtils::SetString(album,                    "title", strAlbum);
  XMLUtils::SetString(album,       "musicbrainzalbumid", strMusicBrainzAlbumID);
  XMLUtils::SetString(album, "musicbrainzreleasegroupid", strReleaseGroupMBID);
  XMLUtils::SetBoolean(album, "scrapedmbid", bScrapedMBID);
  XMLUtils::SetString(album,              "artistdesc", strArtistDesc); //Can be different from artist credits
  XMLUtils::SetStringArray(album,               "genre", genre);
  XMLUtils::SetStringArray(album,               "style", styles);
  XMLUtils::SetStringArray(album,                "mood", moods);
  XMLUtils::SetStringArray(album,               "theme", themes);
  XMLUtils::SetBoolean(album,      "compilation", bCompilation);
  XMLUtils::SetBoolean(album, "boxset", bBoxedSet);

  XMLUtils::SetString(album,      "review", strReview);
  XMLUtils::SetString(album,        "type", strType);
  XMLUtils::SetString(album, "releasestatus", strReleaseStatus);
  XMLUtils::SetString(album, "releasedate", strReleaseDate);
  XMLUtils::SetString(album, "originalreleasedate", strOrigReleaseDate);
  XMLUtils::SetString(album,       "label", strLabel);
  XMLUtils::SetInt(album, "duration", iAlbumDuration);
  if (thumbURL.HasData())
  {
    CXBMCTinyXML doc;
    doc.Parse(thumbURL.GetData());
    const TiXmlNode* thumb = doc.FirstChild("thumb");
    while (thumb)
    {
      album->InsertEndChild(*thumb);
      thumb = thumb->NextSibling("thumb");
    }
  }
  XMLUtils::SetString(album,        "path", strPath);

  auto* rating = XMLUtils::SetFloat(album, "rating", fRating);
  if (rating)
    rating->ToElement()->SetAttribute("max", 10);

  auto* userrating = XMLUtils::SetInt(album, "userrating", iUserrating);
  if (userrating)
    userrating->ToElement()->SetAttribute("max", 10);

  XMLUtils::SetInt(album,           "votes", iVotes);

  for (const auto& artistCredit : artistCredits)
  {
    // add an <albumArtistCredits> tag
    TiXmlElement albumArtistCreditsElement("albumArtistCredits");
    TiXmlNode *albumArtistCreditsNode = album->InsertEndChild(albumArtistCreditsElement);
    XMLUtils::SetString(albumArtistCreditsNode, "artist", artistCredit.m_strArtist);
    XMLUtils::SetString(albumArtistCreditsNode, "musicBrainzArtistID",
                        artistCredit.m_strMusicBrainzArtistID);
  }

  XMLUtils::SetString(album, "releasetype", GetReleaseType());

  return true;
}

