/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Artist.h"

#include "ServiceBroker.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "utils/Fanart.h"
#include "utils/XBMCTinyXML2.h"
#include "utils/XMLUtils.h"

#include <algorithm>

#include <tinyxml2.h>

void CArtist::MergeScrapedArtist(const CArtist& source, bool override /* = true */)
{
  /*
  Initial scraping of artist information when the mbid is derived from tags is done directly
  using that ID, otherwise the lookup is based on name and can mis-identify the artist
  (many have same name). It is useful to store the scraped mbid, but we need to be
  able to correct any mistakes. Hence a manual refresh of artist information uses either
  the mbid is derived from tags or the artist name, not any previously scraped mbid.

   A Musicbrainz artist ID derived from music file tags is always taken as accurate and so can
   not be overwritten by a scraped value. When the artist does not already have an mbid or has
   a previously scraped mbid, merge the new scraped value, flagging it as being from the
   scraper rather than derived from music file tags.
   */
  if (!source.strMusicBrainzArtistID.empty() && (strMusicBrainzArtistID.empty() || bScrapedMBID))
  {
    strMusicBrainzArtistID = source.strMusicBrainzArtistID;
    bScrapedMBID = true;
  }

  if ((override && !source.strArtist.empty()) || strArtist.empty())
    strArtist = source.strArtist;

  if ((override && !source.strSortName.empty()) || strSortName.empty())
    strSortName = source.strSortName;

  strType = source.strType;
  strGender = source.strGender;
  strDisambiguation = source.strDisambiguation;
  genre = source.genre;
  strBiography = source.strBiography;
  styles = source.styles;
  moods = source.moods;
  instruments = source.instruments;
  strBorn = source.strBorn;
  strFormed = source.strFormed;
  strDied = source.strDied;
  strDisbanded = source.strDisbanded;
  yearsActive = source.yearsActive;

  thumbURL = source.thumbURL; // Available remote art
  // Current artwork - thumb, fanart etc., to be stored in art table
  if (!source.art.empty())
    art = source.art;

  discography = source.discography;
  videolinks = source.videolinks;
}

bool CArtist::Load(const tinyxml2::XMLElement* artist, bool append, bool prioritise)
{
  if (!artist) return false;
  if (!append)
    Reset();

  const std::string itemSeparator = CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_musicItemSeparator;

  XMLUtils::GetString(artist,                "name", strArtist);
  XMLUtils::GetString(artist, "musicBrainzArtistID", strMusicBrainzArtistID);
  XMLUtils::GetString(artist,            "sortname", strSortName);
  XMLUtils::GetString(artist, "type", strType);
  XMLUtils::GetString(artist, "gender", strGender);
  XMLUtils::GetString(artist, "disambiguation", strDisambiguation);
  XMLUtils::GetStringArray(artist,       "genre", genre, prioritise, itemSeparator);
  XMLUtils::GetStringArray(artist,       "style", styles, prioritise, itemSeparator);
  XMLUtils::GetStringArray(artist,        "mood", moods, prioritise, itemSeparator);
  XMLUtils::GetStringArray(artist, "yearsactive", yearsActive, prioritise, itemSeparator);
  XMLUtils::GetStringArray(artist, "instruments", instruments, prioritise, itemSeparator);

  XMLUtils::GetString(artist,      "born", strBorn);
  XMLUtils::GetString(artist,    "formed", strFormed);
  XMLUtils::GetString(artist, "biography", strBiography);
  XMLUtils::GetString(artist,      "died", strDied);
  XMLUtils::GetString(artist, "disbanded", strDisbanded);

  size_t iThumbCount = thumbURL.GetUrls().size();
  std::string xmlAdd = thumbURL.GetData();

  // Available artist thumbs
  tinyxml2::XMLPrinter printer;
  const auto* thumb = artist->FirstChildElement("thumb");

  while (thumb)
  {
    thumbURL.ParseAndAppendUrl(thumb);
    if (prioritise)
    {
      thumb->Accept(&printer);
      const char* temp{printer.CStr()};
      xmlAdd = temp + xmlAdd;
    }
    thumb = thumb->NextSiblingElement("thumb");
  }
  // prefix thumbs from nfos
  if (prioritise && iThumbCount && iThumbCount != thumbURL.GetUrls().size())
  {
    auto thumbUrls = thumbURL.GetUrls();
    rotate(thumbUrls.begin(), thumbUrls.begin() + iThumbCount, thumbUrls.end());
    thumbURL.SetUrls(thumbUrls);
    thumbURL.SetData(xmlAdd);
  }

  // Discography
  const auto* node = artist->FirstChildElement("album");
  if (node)
    discography.clear();
  while (node)
  {
    if (node->FirstChild())
    {
      CDiscoAlbum album;
      XMLUtils::GetString(node, "title", album.strAlbum);
      XMLUtils::GetString(node, "year", album.strYear);
      XMLUtils::GetString(node, "musicbrainzreleasegroupid", album.strReleaseGroupMBID);
      discography.push_back(album);
    }
    node = node->NextSiblingElement("album");
  }

  //song video links
  const auto* songurls = artist->FirstChildElement("videourl");
  if (songurls)
    videolinks.clear();
  while (songurls)
  {
    if (songurls->FirstChild())
    {
      ArtistVideoLinks videoLink;
      XMLUtils::GetString(songurls, "title", videoLink.title);
      XMLUtils::GetString(songurls, "musicbrainztrackid", videoLink.mbTrackID);
      XMLUtils::GetString(songurls, "url", videoLink.videoURL);
      XMLUtils::GetString(songurls, "thumburl", videoLink.thumbURL);
      videolinks.emplace_back(std::move(videoLink));
    }
    songurls = songurls->NextSiblingElement("videourl");
  }

  // Support old style <fanart></fanart> for backwards compatibility of old nfo files and scrapers
  const auto* fanart2 = artist->FirstChildElement("fanart");
  if (fanart2)
  {
    CFanart fanart;
    // we prefix to handle mixed-mode nfo's with fanart set
    fanart2->Accept(&printer);

    if (prioritise)
    {
      const char* temp{printer.CStr()};
      fanart.m_xml = temp + fanart.m_xml;
    }
    else
      fanart.m_xml.append(printer.CStr());
    fanart.Unpack();
    // Append fanart to other image URLs
    for (unsigned int i = 0; i < fanart.GetNumFanarts(); i++)
      thumbURL.AddParsedUrl(fanart.GetImageURL(i), "fanart", fanart.GetPreviewURL(i));
  }

 // Current artwork  - thumb, fanart etc. (the chosen art, not the lists of those available)
  node = artist->FirstChildElement("art");
  if (node)
  {
    const auto* artdetailNode = node->FirstChild();
    while (artdetailNode && artdetailNode->FirstChild())
    {
      art.insert(std::make_pair(artdetailNode->Value(), artdetailNode->FirstChild()->Value()));
      artdetailNode = artdetailNode->NextSibling();
    }
  }

  return true;
}

bool CArtist::Save(tinyxml2::XMLNode* node, const std::string& tag, const std::string& strPath)
{
  if (!node) return false;

  // we start with a <tag> tag
  tinyxml2::XMLElement* artistElement = node->GetDocument()->NewElement(tag.c_str());
  auto* artist = node->InsertEndChild(artistElement);

  if (!artist) return false;

  XMLUtils::SetString(artist,                      "name", strArtist);
  XMLUtils::SetString(artist,       "musicBrainzArtistID", strMusicBrainzArtistID);
  XMLUtils::SetString(artist,                  "sortname", strSortName);
  XMLUtils::SetString(artist,                      "type", strType);
  XMLUtils::SetString(artist,                    "gender", strGender);
  XMLUtils::SetString(artist,            "disambiguation", strDisambiguation);
  XMLUtils::SetStringArray(artist,                "genre", genre);
  XMLUtils::SetStringArray(artist,                "style", styles);
  XMLUtils::SetStringArray(artist,                 "mood", moods);
  XMLUtils::SetStringArray(artist,          "yearsactive", yearsActive);
  XMLUtils::SetStringArray(artist,          "instruments", instruments);
  XMLUtils::SetString(artist,                      "born", strBorn);
  XMLUtils::SetString(artist,                    "formed", strFormed);
  XMLUtils::SetString(artist,                 "biography", strBiography);
  XMLUtils::SetString(artist,                      "died", strDied);
  XMLUtils::SetString(artist,                 "disbanded", strDisbanded);
  // Available remote art
  if (thumbURL.HasData())
  {
    CXBMCTinyXML2 doc;
    doc.Parse(thumbURL.GetData());
    auto* thumb = doc.FirstChildElement("thumb");
    while (thumb)
    {
      auto* copyThumb = thumb->DeepClone(node->GetDocument());
      artist->InsertEndChild(copyThumb);
      thumb = thumb->NextSiblingElement("thumb");
    }
  }
  XMLUtils::SetString(artist,        "path", strPath);

  // Discography
  for (const auto& it : discography)
  {
    // add a <album> tag
    auto* discoElement = node->GetDocument()->NewElement("album");
    auto* discoNode = artist->InsertEndChild(discoElement);
    XMLUtils::SetString(discoNode, "title", it.strAlbum);
    XMLUtils::SetString(discoNode, "year", it.strYear);
    XMLUtils::SetString(discoNode, "musicbrainzreleasegroupid", it.strReleaseGroupMBID);
  }
  // song video links
  for (const auto& it : videolinks)
  {
    auto* videolinkElement = node->GetDocument()->NewElement("videourl");
    auto* videolinkNode = artist->InsertEndChild(videolinkElement);
    XMLUtils::SetString(videolinkNode, "title", it.title);
    XMLUtils::SetString(videolinkNode, "musicbrainztrackid", it.mbTrackID);
    XMLUtils::SetString(videolinkNode, "url", it.videoURL);
    XMLUtils::SetString(videolinkNode, "thumburl", it.thumbURL);
  }

  return true;
}

void CArtist::SetDateAdded(const std::string& strDateAdded)
{
  dateAdded.SetFromDBDateTime(strDateAdded);
}

void CArtist::SetDateUpdated(const std::string& strDateUpdated)
{
  dateUpdated.SetFromDBDateTime(strDateUpdated);
}

void CArtist::SetDateNew(const std::string& strDateNew)
{
  dateNew.SetFromDBDateTime(strDateNew);
}

