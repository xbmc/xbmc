/*
 *      Copyright (C) 2005-2008 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "Album.h"
#include "settings/AdvancedSettings.h"
#include "utils/StringUtils.h"
#include "utils/XMLUtils.h"
#include "utils/MathUtils.h"

using namespace std;
using namespace MUSIC_INFO;

bool CAlbum::Load(const TiXmlElement *album, bool append, bool prioritise)
{
  if (!album) return false;
  if (!append)
    Reset();

  XMLUtils::GetString(album,"title",strAlbum);

  XMLUtils::GetAdditiveString(album, "artist", g_advancedSettings.m_musicItemSeparator, strArtist, prioritise);
  XMLUtils::GetAdditiveString(album, "genre", g_advancedSettings.m_musicItemSeparator, strGenre, prioritise);
  XMLUtils::GetAdditiveString(album, "style", g_advancedSettings.m_musicItemSeparator, strStyles, prioritise);
  XMLUtils::GetAdditiveString(album, "mood", g_advancedSettings.m_musicItemSeparator, strMoods, prioritise);
  XMLUtils::GetAdditiveString(album, "theme", g_advancedSettings.m_musicItemSeparator, strThemes, prioritise);

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
      XMLUtils::GetInt(node,"position",song.iTrack);

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

  XMLUtils::SetString(album,  "title", strAlbum);
  XMLUtils::SetAdditiveString(album, "artist",
                           g_advancedSettings.m_musicItemSeparator, strArtist);
  XMLUtils::SetAdditiveString(album,  "genre",
                           g_advancedSettings.m_musicItemSeparator, strGenre);
  XMLUtils::SetAdditiveString(album, "style",
                           g_advancedSettings.m_musicItemSeparator, strStyles);
  XMLUtils::SetAdditiveString(album,  "mood",
                           g_advancedSettings.m_musicItemSeparator, strMoods);
  XMLUtils::SetAdditiveString(album,  "theme",
                           g_advancedSettings.m_musicItemSeparator, strThemes);

  XMLUtils::SetString(album,      "review", strReview);
  XMLUtils::SetString(album,        "type", strType);
  XMLUtils::SetString(album, "releasedate", m_strDateOfRelease);
  XMLUtils::SetString(album,       "label", strLabel);
  XMLUtils::SetString(album,        "type", strType);
  if (!thumbURL.m_xml.empty())
  {
    TiXmlDocument doc;
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

  for( VECSONGS::const_iterator it = songs.begin();it != songs.end();++it)
  {
    // add a <song> tag
    TiXmlElement cast("track");
    TiXmlNode *node = album->InsertEndChild(cast);
    TiXmlElement title("title");
    TiXmlNode *titleNode = node->InsertEndChild(title);
    TiXmlText name(it->strTitle);
    titleNode->InsertEndChild(name);
    TiXmlElement year("position");
    TiXmlNode *yearNode = node->InsertEndChild(year);
    CStdString strTrack;
    strTrack.Format("%i",it->iTrack);
    TiXmlText name2(strTrack);
    yearNode->InsertEndChild(name2);
    TiXmlElement duration("duration");
    TiXmlNode *durNode = node->InsertEndChild(duration);
    TiXmlText name3(StringUtils::SecondsToTimeString(it->iDuration));
    durNode->InsertEndChild(name3);
  }

  return true;
}

