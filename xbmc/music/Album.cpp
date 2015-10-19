/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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

#include "Album.h"
#include "music/tags/MusicInfoTag.h"
#include "settings/AdvancedSettings.h"
#include "utils/StringUtils.h"
#include "utils/XMLUtils.h"
#include "utils/MathUtils.h"
#include "FileItem.h"

#include <algorithm>

using namespace MUSIC_INFO;

typedef struct ReleaseTypeInfo {
  CAlbum::ReleaseType type;
  std::string name;
} ReleaseTypeInfo;

ReleaseTypeInfo releaseTypes[] = {
  { CAlbum::Album,  "album" },
  { CAlbum::Single, "single" }
};

#define RELEASE_TYPES_SIZE sizeof(releaseTypes) / sizeof(ReleaseTypeInfo)

CAlbum::CAlbum(const CFileItem& item)
{
  Reset();
  const CMusicInfoTag& tag = *item.GetMusicInfoTag();
  SYSTEMTIME stTime;
  tag.GetReleaseDate(stTime);
  strAlbum = tag.GetAlbum();
  strMusicBrainzAlbumID = tag.GetMusicBrainzAlbumID();
  genre = tag.GetGenre();
  std::vector<std::string> musicBrainAlbumArtistHints = tag.GetMusicBrainzAlbumArtistHints();
  strArtistDesc = tag.GetAlbumArtistString();

  if (!tag.GetMusicBrainzAlbumArtistID().empty())
  { // have musicbrainz artist info, so use it
    for (size_t i = 0; i < tag.GetMusicBrainzAlbumArtistID().size(); i++)
    {
      std::string artistId = tag.GetMusicBrainzAlbumArtistID()[i];
      std::string artistName;

      /*
         We try and get the mbrainzid <-> name matching from the hints and match on the same index.
         If not found, we try and use the mbrainz <-> name matching from the artists fields
         If still not found, try and use the same index of the albumartist field.
         If still not found, use the mbrainzid and hope we later on can update that entry
         */

      if (i < musicBrainAlbumArtistHints.size())
        artistName = musicBrainAlbumArtistHints[i];
      else if (!tag.GetMusicBrainzArtistID().empty() && !tag.GetArtist().empty())
      {
        for (size_t j = 0; j < tag.GetMusicBrainzArtistID().size(); j++)
        {
          if (artistId == tag.GetMusicBrainzArtistID()[j])
          {
            if (j < tag.GetMusicBrainzArtistHints().size())
              artistName = tag.GetMusicBrainzArtistHints()[j];
            else
              artistName = (j < tag.GetArtist().size()) ? tag.GetArtist()[j] : tag.GetArtist()[0];
          }
        }
      }

      if (artistName.empty() && tag.GetMusicBrainzAlbumArtistID().size() == tag.GetAlbumArtist().size())
        artistName = tag.GetAlbumArtist()[i];

      if (artistName.empty())
        artistName = artistId;

      std::string strJoinPhrase = (i == tag.GetMusicBrainzAlbumArtistID().size()-1) ? "" : g_advancedSettings.m_musicItemSeparator;
      CArtistCredit artistCredit(artistName, tag.GetMusicBrainzAlbumArtistID()[i], strJoinPhrase);
      artistCredits.push_back(artistCredit);
    }
  }
  else
  { // no musicbrainz info, so fill in directly
    for (std::vector<std::string>::const_iterator it = tag.GetAlbumArtist().begin(); it != tag.GetAlbumArtist().end(); ++it)
    {
      std::string strJoinPhrase = (it == --tag.GetAlbumArtist().end() ? "" : g_advancedSettings.m_musicItemSeparator);
      CArtistCredit artistCredit(*it, "", strJoinPhrase);
      artistCredits.push_back(artistCredit);
    }
  }
  iYear = stTime.wYear;
  bCompilation = tag.GetCompilation();
  iTimesPlayed = 0;
  dateAdded.Reset();
  lastPlayed.Reset();
  releaseType = tag.GetAlbumReleaseType();
}

void CAlbum::MergeScrapedAlbum(const CAlbum& source, bool override /* = true */)
{
  /*
   We don't merge musicbrainz album ID so that a refresh of album information
   allows a lookup based on name rather than directly (re)using musicbrainz.
   In future, we may wish to be able to override lookup by musicbrainz so
   this might be dropped.
   */
//  strMusicBrainzAlbumID = source.strMusicBrainzAlbumID;
  if ((override && !source.genre.empty()) || genre.empty())
    genre = source.genre;
  if ((override && !source.strAlbum.empty()) || strAlbum.empty())
    strAlbum = source.strAlbum;
  if ((override && source.iYear > 0) || iYear == 0)
    iYear = source.iYear;
  if (override)
    bCompilation = source.bCompilation;
  //  iTimesPlayed = source.iTimesPlayed; // times played is derived from songs
  for (std::map<std::string, std::string>::const_iterator i = source.art.begin(); i != source.art.end(); ++i)
  {
    if (override || art.find(i->first) == art.end())
      art[i->first] = i->second;
  }
  strLabel = source.strLabel;
  thumbURL = source.thumbURL;
  moods = source.moods;
  styles = source.styles;
  themes = source.themes;
  strReview = source.strReview;
  strType = source.strType;
//  strPath = source.strPath; // don't merge the path
  m_strDateOfRelease = source.m_strDateOfRelease;
  iRating = source.iRating;
  if (override)
  {
    artistCredits = source.artistCredits;
  }
  else if (source.artistCredits.size() > artistCredits.size())
    artistCredits.insert(artistCredits.end(), source.artistCredits.begin()+artistCredits.size(), source.artistCredits.end());
  if (!strMusicBrainzAlbumID.empty())
  {
    /* update local songs with MB information */
    for (VECSONGS::iterator song = songs.begin(); song != songs.end(); ++song)
    {
      if (!song->strMusicBrainzTrackID.empty())
        for (VECSONGS::const_iterator sourceSong = source.infoSongs.begin(); sourceSong != source.infoSongs.end(); ++sourceSong)
          if (sourceSong->strMusicBrainzTrackID == song->strMusicBrainzTrackID)
            song->MergeScrapedSong(*sourceSong, override);
    }
  }
  infoSongs = source.infoSongs;
}

std::string CAlbum::GetGenreString() const
{
  return StringUtils::Join(genre, g_advancedSettings.m_musicItemSeparator);
}

const std::vector<std::string> CAlbum::GetAlbumArtist() const
{
  //Get artist names as vector from artist credits
  std::vector<std::string> albumartists;
  for (VECARTISTCREDITS::const_iterator artistCredit = artistCredits.begin(); artistCredit != artistCredits.end(); ++artistCredit)
  {
    albumartists.push_back(artistCredit->GetArtist());
  }
  return albumartists;
}

const std::vector<std::string> CAlbum::GetMusicBrainzAlbumArtistID() const
{
  //Get artist MusicBrainz IDs as vector from artist credits
  std::vector<std::string> muisicBrainzID;
  for (VECARTISTCREDITS::const_iterator artistCredit = artistCredits.begin(); artistCredit != artistCredits.end(); ++artistCredit)
  {
    muisicBrainzID.push_back(artistCredit->GetMusicBrainzArtistID());
  }
  return muisicBrainzID;
}

const std::string CAlbum::GetAlbumArtistString() const
{
  //Artist description may be different from the artists in artistcredits (see ALBUMARTISTS tag processing)
  //but is takes precidence as a string because artistcredits is not always filled during processing
  if (!strArtistDesc.empty())
    return strArtistDesc;
  std::string artistString;
  for (VECARTISTCREDITS::const_iterator artistCredit = artistCredits.begin(); artistCredit != artistCredits.end(); ++artistCredit)
    artistString += artistCredit->GetArtist() + artistCredit->GetJoinPhrase();
  return artistString;
}

const std::vector<int> CAlbum::GetArtistIDArray() const
{
  // Get album artist IDs for json rpc
  std::vector<int> artistids;
  for (VECARTISTCREDITS::const_iterator artistCredit = artistCredits.begin(); artistCredit != artistCredits.end(); ++artistCredit)
    artistids.push_back(artistCredit->GetArtistId());
  return artistids;
}

std::string CAlbum::GetReleaseType() const
{
  return ReleaseTypeToString(releaseType);
}

void CAlbum::SetReleaseType(const std::string& strReleaseType)
{
  releaseType = ReleaseTypeFromString(strReleaseType);
}

void CAlbum::SetDateAdded(const std::string& strDateAdded)
{
  dateAdded.SetFromDBDateTime(strDateAdded);
}

void CAlbum::SetLastPlayed(const std::string& strLastPlayed)
{
  lastPlayed.SetFromDBDateTime(strLastPlayed);
}

std::string CAlbum::ReleaseTypeToString(CAlbum::ReleaseType releaseType)
{
  for (size_t i = 0; i < RELEASE_TYPES_SIZE; i++)
  {
    const ReleaseTypeInfo& releaseTypeInfo = releaseTypes[i];
    if (releaseTypeInfo.type == releaseType)
      return releaseTypeInfo.name;
  }

  return "album";
}

CAlbum::ReleaseType CAlbum::ReleaseTypeFromString(const std::string& strReleaseType)
{
  for (size_t i = 0; i < RELEASE_TYPES_SIZE; i++)
  {
    const ReleaseTypeInfo& releaseTypeInfo = releaseTypes[i];
    if (releaseTypeInfo.name == strReleaseType)
      return releaseTypeInfo.type;
  }

  return Album;
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

  XMLUtils::GetString(album,              "title", strAlbum);
  XMLUtils::GetString(album, "musicBrainzAlbumID", strMusicBrainzAlbumID);
  std::vector<std::string> artist; // Support old style <artist></artist> for backwards compatibility
  XMLUtils::GetStringArray(album, "artist", artist, prioritise, g_advancedSettings.m_musicItemSeparator);
  XMLUtils::GetStringArray(album, "genre", genre, prioritise, g_advancedSettings.m_musicItemSeparator);
  XMLUtils::GetStringArray(album, "style", styles, prioritise, g_advancedSettings.m_musicItemSeparator);
  XMLUtils::GetStringArray(album, "mood", moods, prioritise, g_advancedSettings.m_musicItemSeparator);
  XMLUtils::GetStringArray(album, "theme", themes, prioritise, g_advancedSettings.m_musicItemSeparator);
  XMLUtils::GetBoolean(album, "compilation", bCompilation);

  XMLUtils::GetString(album,"review",strReview);
  XMLUtils::GetString(album,"releasedate",m_strDateOfRelease);
  XMLUtils::GetString(album,"label",strLabel);
  XMLUtils::GetString(album,"type",strType);

  XMLUtils::GetInt(album,"year",iYear);
  const TiXmlElement* rElement = album->FirstChildElement("rating");
  if (rElement)
  {
    float rating = 0;
    float max_rating = 5;
    XMLUtils::GetFloat(album, "rating", rating);
    if (rElement->QueryFloatAttribute("max", &max_rating) == TIXML_SUCCESS && max_rating>=1)
      rating *= (5.f / max_rating); // Normalise the Rating to between 0 and 5 
    if (rating > 5.f)
      rating = 5.f;
    iRating = MathUtils::round_int(rating);
  }

  size_t iThumbCount = thumbURL.m_url.size();
  std::string xmlAdd = thumbURL.m_xml;
  const TiXmlElement* thumb = album->FirstChildElement("thumb");
  while (thumb)
  {
    thumbURL.ParseElement(thumb);
    if (prioritise)
    {
      std::string temp;
      temp << *thumb;
      xmlAdd = temp+xmlAdd;
    }
    thumb = thumb->NextSiblingElement("thumb");
  }
  // prioritise thumbs from nfos
  if (prioritise && iThumbCount && iThumbCount != thumbURL.m_url.size())
  {
    rotate(thumbURL.m_url.begin(),
           thumbURL.m_url.begin()+iThumbCount, 
           thumbURL.m_url.end());
    thumbURL.m_xml = xmlAdd;
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
      XMLUtils::GetString(albumArtistCreditsNode,  "joinphrase",           artistCredit.m_strJoinPhrase);
      XMLUtils::GetBoolean(albumArtistCreditsNode, "featuring",            artistCredit.m_boolFeatured);
      artistCredits.push_back(artistCredit);
    }

    albumArtistCreditsNode = albumArtistCreditsNode->NextSiblingElement("albumArtistCredits");
  }

  // Support old style <artist></artist> for backwards compatibility
  // .nfo files should ideally be updated to use the artist credits structure above
  // or removed entirely in preference for better tags (MusicBrainz?)
  if (artistCredits.empty() && !artist.empty())
  {
    for (std::vector<std::string>::const_iterator it = artist.begin(); it != artist.end(); ++it)
    {
      CArtistCredit artistCredit(*it, "",
                                 it == --artist.end() ? "" : g_advancedSettings.m_musicItemSeparator);
      artistCredits.push_back(artistCredit);
    }
  }

  const TiXmlElement* node = album->FirstChildElement("track");
  if (node)
    infoSongs.clear();  // this means that the tracks can't be spread over separate pages
                    // but this is probably a reasonable limitation
  bool bIncrement = false;
  while (node)
  {
    if (node->FirstChild())
    {

      CSong song;
      const TiXmlElement* songArtistCreditsNode = node->FirstChildElement("songArtistCredits");
      if (songArtistCreditsNode)
        song.artistCredits.clear();
      
      while (songArtistCreditsNode)
      {
        if (songArtistCreditsNode->FirstChild())
        {
          CArtistCredit artistCredit;
          XMLUtils::GetString(songArtistCreditsNode,  "artist",               artistCredit.m_strArtist);
          XMLUtils::GetString(songArtistCreditsNode,  "musicBrainzArtistID",  artistCredit.m_strMusicBrainzArtistID);
          XMLUtils::GetString(songArtistCreditsNode,  "joinphrase",           artistCredit.m_strJoinPhrase);
          XMLUtils::GetBoolean(songArtistCreditsNode, "featuring",            artistCredit.m_boolFeatured);
          song.artistCredits.push_back(artistCredit);
        }
        
        songArtistCreditsNode = songArtistCreditsNode->NextSiblingElement("songArtistCredits");
      }

      XMLUtils::GetString(node,   "musicBrainzTrackID",   song.strMusicBrainzTrackID);
      XMLUtils::GetInt(node, "position", song.iTrack);

      if (song.iTrack == 0)
        bIncrement = true;

      XMLUtils::GetString(node,"title",song.strTitle);
      std::string strDur;
      XMLUtils::GetString(node,"duration",strDur);
      song.iDuration = StringUtils::TimeStringToSeconds(strDur);

      if (bIncrement)
        song.iTrack = song.iTrack + 1;

      infoSongs.push_back(song);
    }
    node = node->NextSiblingElement("track");
  }

  std::string strReleaseType;
  if (XMLUtils::GetString(album, "releasetype", strReleaseType))
    SetReleaseType(strReleaseType);
  else
    releaseType = Album;

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
  XMLUtils::SetString(album,       "musicBrainzAlbumID", strMusicBrainzAlbumID);
  XMLUtils::SetStringArray(album,              "artist", GetAlbumArtist());
  XMLUtils::SetStringArray(album,               "genre", genre);
  XMLUtils::SetStringArray(album,               "style", styles);
  XMLUtils::SetStringArray(album,                "mood", moods);
  XMLUtils::SetStringArray(album,               "theme", themes);
  XMLUtils::SetBoolean(album,      "compilation", bCompilation);

  XMLUtils::SetString(album,      "review", strReview);
  XMLUtils::SetString(album,        "type", strType);
  XMLUtils::SetString(album, "releasedate", m_strDateOfRelease);
  XMLUtils::SetString(album,       "label", strLabel);
  XMLUtils::SetString(album,        "type", strType);
  if (!thumbURL.m_xml.empty())
  {
    CXBMCTinyXML doc;
    doc.Parse(thumbURL.m_xml);
    const TiXmlNode* thumb = doc.FirstChild("thumb");
    while (thumb)
    {
      album->InsertEndChild(*thumb);
      thumb = thumb->NextSibling("thumb");
    }
  }
  XMLUtils::SetString(album,        "path", strPath);

  XMLUtils::SetInt(album,         "rating", iRating);
  XMLUtils::SetInt(album,           "year", iYear);

  for( VECARTISTCREDITS::const_iterator artistCredit = artistCredits.begin();artistCredit != artistCredits.end();++artistCredit)
  {
    // add an <albumArtistCredits> tag
    TiXmlElement albumArtistCreditsElement("albumArtistCredits");
    TiXmlNode *albumArtistCreditsNode = album->InsertEndChild(albumArtistCreditsElement);
    XMLUtils::SetString(albumArtistCreditsNode,               "artist", artistCredit->m_strArtist);
    XMLUtils::SetString(albumArtistCreditsNode,  "musicBrainzArtistID", artistCredit->m_strMusicBrainzArtistID);
    XMLUtils::SetString(albumArtistCreditsNode,           "joinphrase", artistCredit->m_strJoinPhrase);
    XMLUtils::SetString(albumArtistCreditsNode,            "featuring", artistCredit->GetArtist());
  }

  for( VECSONGS::const_iterator song = infoSongs.begin(); song != infoSongs.end(); ++song)
  {
    // add a <song> tag
    TiXmlElement cast("track");
    TiXmlNode *node = album->InsertEndChild(cast);
    for( VECARTISTCREDITS::const_iterator artistCredit = song->artistCredits.begin(); artistCredit != song->artistCredits.end(); ++artistCredit)
    {
      // add an <albumArtistCredits> tag
      TiXmlElement songArtistCreditsElement("songArtistCredits");
      TiXmlNode *songArtistCreditsNode = node->InsertEndChild(songArtistCreditsElement);
      XMLUtils::SetString(songArtistCreditsNode,               "artist", artistCredit->m_strArtist);
      XMLUtils::SetString(songArtistCreditsNode,  "musicBrainzArtistID", artistCredit->m_strMusicBrainzArtistID);
      XMLUtils::SetString(songArtistCreditsNode,           "joinphrase", artistCredit->m_strJoinPhrase);
      XMLUtils::SetString(songArtistCreditsNode,            "featuring", artistCredit->GetArtist());
    }
    XMLUtils::SetString(node,   "musicBrainzTrackID",   song->strMusicBrainzTrackID);
    XMLUtils::SetString(node,   "title",                song->strTitle);
    XMLUtils::SetInt(node,      "position",             song->iTrack);
    XMLUtils::SetString(node,   "duration",             StringUtils::SecondsToTimeString(song->iDuration));
  }

  XMLUtils::SetString(album, "releasetype", GetReleaseType());

  return true;
}

