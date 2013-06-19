//
//  PlexDirectoryTypeParserMusic.cpp
//  Plex
//
//  Created by Tobias Hieta <tobias@plexapp.com> on 2013-04-09.
//  Copyright 2013 Plex Inc. All rights reserved.
//

#include "PlexDirectoryTypeParserMusic.h"

#include "music/Album.h"
#include "music/Artist.h"
#include "music/Song.h"
#include "utils/StringUtils.h"

void
CPlexDirectoryTypeParserAlbum::Process(CFileItem &item, CFileItem &mediaContainer, TiXmlElement *itemElement)
{
  CAlbum album;
  album.strLabel = item.GetProperty("title").asString();
  album.strAlbum = item.GetProperty("title").asString();
  album.artist.push_back(mediaContainer.GetProperty("parentTitle").asString());
  album.iYear = item.GetProperty("year").asInteger();
  album.m_strDateOfRelease = item.GetProperty("originallyAvailableAt").asString();

  item.SetFromAlbum(album);
  if (!item.HasArt(PLEX_ART_THUMB))
    item.SetArt(PLEX_ART_THUMB, mediaContainer.GetArt(PLEX_ART_THUMB));
}

void
CPlexDirectoryTypeParserTrack::Process(CFileItem &item, CFileItem &mediaContainer, TiXmlElement *itemElement)
{
  CSong song;

  song.strFileName = item.GetPath();
  song.strTitle = item.GetLabel();
  song.strThumb = item.GetArt(PLEX_ART_THUMB);
  song.strComment = item.GetProperty("summary").asString();

  if (item.HasProperty("originallyAvailableAt"))
  {
    std::vector<std::string> s = StringUtils::Split(item.GetProperty("originallyAvailableAt").asString(), "-");
    if (s.size() > 0)
      song.iYear = boost::lexical_cast<int>(s[0]);
  }

  item.SetFromSong(song);

  ParseMediaNodes(item, itemElement);

  /* Now we have the Media nodes, we need to "borrow" some properties from it */
  if (item.m_mediaItems.size() > 0)
  {
    CFileItemPtr firstMedia = item.m_mediaItems[0];
    item.m_mapProperties.insert(firstMedia->m_mapProperties.begin(), firstMedia->m_mapProperties.end());

    /* also forward art, this is the mediaTags */
    item.AppendArt(firstMedia->GetArt());
  }
}


void CPlexDirectoryTypeParserArtist::Process(CFileItem &item, CFileItem &mediaContainer, TiXmlElement *itemElement)
{
  CArtist artist;

  artist.strArtist = item.GetLabel();
  artist.strBiography = item.GetProperty("summary").asString();
  artist.thumbURL = CScraperUrl(item.GetArt(PLEX_ART_THUMB));

  item.GetMusicInfoTag()->SetArtist(artist);
}
