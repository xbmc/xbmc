/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include "Album.h"
#include "settings/AdvancedSettings.h"
#include "utils/StringUtils.h"
#include "utils/XMLUtils.h"
#include "utils/MathUtils.h"
#include "FileItem.h"

using namespace std;
using namespace MUSIC_INFO;

CAlbum::CAlbum(const CFileItem& item)
{
  Reset();
  const CMusicInfoTag& tag = *item.GetMusicInfoTag();
  SYSTEMTIME stTime;
  tag.GetReleaseDate(stTime);
  strAlbum = tag.GetAlbum();
  strMusicBrainzAlbumID = tag.GetMusicBrainzAlbumID();
  genre = tag.GetGenre();
  artist = tag.GetAlbumArtist();
  iYear = stTime.wYear;
  bCompilation = tag.GetCompilation();
  iTimesPlayed = 0;
}

CStdString CAlbum::GetArtistString() const
{
  return StringUtils::Join(artist, g_advancedSettings.m_musicItemSeparator);
}

CStdString CAlbum::GetGenreString() const
{
  return StringUtils::Join(artist, g_advancedSettings.m_musicItemSeparator);
}

bool CAlbum::operator<(const CAlbum &a) const
{
  if (strAlbum < a.strAlbum) return true;
  if (strAlbum > a.strAlbum) return false;
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
  CStdString xmlAdd = thumbURL.m_xml;
  const TiXmlElement* thumb = album->FirstChildElement("thumb");
  while (thumb)
  {
    thumbURL.ParseElement(thumb);
    if (prioritise)
    {
      CStdString temp;
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

  const TiXmlElement* node = album->FirstChildElement("track");
  if (node)
    songs.clear();  // this means that the tracks can't be spread over separate pages
                    // but this is probably a reasonable limitation
  bool bIncrement = false;
  while (node)
  {
    if (node->FirstChild())
    {

      CSong song;
      XMLUtils::GetInt(node, "position", song.iTrack);

      if (song.iTrack == 0)
        bIncrement = true;

      XMLUtils::GetString(node,"title",song.strTitle);
      CStdString strDur;
      XMLUtils::GetString(node,"duration",strDur);
      song.iDuration = StringUtils::TimeStringToSeconds(strDur);

      if (bIncrement)
        song.iTrack = song.iTrack + 1;

      songs.push_back(song);
    }
    node = node->NextSiblingElement("track");
  }

  return true;
}

bool CAlbum::Save(TiXmlNode *node, const CStdString &tag, const CStdString& strPath)
{
  if (!node) return false;

  // we start with a <tag> tag
  TiXmlElement albumElement(tag.c_str());
  TiXmlNode *album = node->InsertEndChild(albumElement);

  if (!album) return false;

  XMLUtils::SetString(album,                    "title", strAlbum);
  XMLUtils::SetString(album,       "musicBrainzAlbumID", strMusicBrainzAlbumID);
  XMLUtils::SetStringArray(album,              "artist", artist);
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

  for( VECARTISTCREDITS::const_iterator it = artistCredits.begin();it != artistCredits.end();++it)
  {
    // add an <albumArtistCredits> tag
    TiXmlElement albumArtistCreditsElement("albumArtistCredits");
    TiXmlNode *albumArtistCreditsNode = album->InsertEndChild(albumArtistCreditsElement);
    XMLUtils::SetString(albumArtistCreditsNode,               "artist", it->m_strArtist);
    XMLUtils::SetString(albumArtistCreditsNode,  "musicBrainzArtistID", it->m_strMusicBrainzArtistID);
    XMLUtils::SetString(albumArtistCreditsNode,           "joinphrase", it->m_strJoinPhrase);
    XMLUtils::SetString(albumArtistCreditsNode,            "featuring", it->GetArtist());
  }

  for( VECSONGS::const_iterator it = songs.begin();it != songs.end();++it)
  {
    // add a <song> tag
    TiXmlElement cast("track");
    TiXmlNode *node = album->InsertEndChild(cast);
    XMLUtils::SetString(node,   "title",    it->strTitle);
    XMLUtils::SetInt(node,      "position", it->iTrack);
    XMLUtils::SetString(node,   "duration", StringUtils::SecondsToTimeString(it->iDuration));
  }

  return true;
}

