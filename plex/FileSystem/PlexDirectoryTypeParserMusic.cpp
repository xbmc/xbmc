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
  album.iYear = item.GetProperty("year").asInteger();
  album.m_strDateOfRelease = item.GetProperty("originallyAvailableAt").asString();

  if(item.HasProperty("parentTitle"))
    album.artist.push_back(item.GetProperty("parentTitle").asString());
  else if (mediaContainer.HasProperty("parentTitle"))
    album.artist.push_back(mediaContainer.GetProperty("parentTitle").asString());

  item.SetFromAlbum(album);
  item.SetProperty("description", item.GetProperty("summary"));
  if (!item.HasArt(PLEX_ART_THUMB))
    item.SetArt(PLEX_ART_THUMB, mediaContainer.GetArt(PLEX_ART_THUMB));
}

void
CPlexDirectoryTypeParserTrack::Process(CFileItem &item, CFileItem &mediaContainer, TiXmlElement *itemElement)
{
  CSong song;

  song.strTitle = item.GetLabel();
  song.strComment = item.GetProperty("summary").asString();
  song.iDuration = item.GetProperty("duration").asInteger() / 1000;

  if (!item.HasArt(PLEX_ART_THUMB) && mediaContainer.HasArt(PLEX_ART_THUMB))
    item.SetArt(PLEX_ART_THUMB, mediaContainer.GetArt(PLEX_ART_THUMB));
  song.strThumb = item.GetArt(PLEX_ART_THUMB);

  if (item.HasProperty("grandparentTitle"))
    song.artist.push_back(item.GetProperty("grandparentTitle").asString());
  else if (mediaContainer.HasProperty("grandparentTitle"))
    song.artist.push_back(mediaContainer.GetProperty("grandparentTitle").asString());

  if (item.HasProperty("parentTitle"))
    song.strAlbum = item.GetProperty("parentTitle").asString();
  else if (mediaContainer.HasProperty("parentTitle"))
    song.strAlbum = mediaContainer.GetProperty("parentTitle").asString();

  if (item.HasProperty("originallyAvailableAt"))
  {
    std::vector<std::string> s = StringUtils::Split(item.GetProperty("originallyAvailableAt").asString(), "-");
    if (s.size() > 0)
      song.iYear = boost::lexical_cast<int>(s[0]);
  }

  ParseMediaNodes(item, itemElement);

  /* Now we have the Media nodes, we need to "borrow" some properties from it */
  if (item.m_mediaItems.size() > 0)
  {
    CFileItemPtr firstMedia = item.m_mediaItems[0];
    item.m_mapProperties.insert(firstMedia->m_mapProperties.begin(), firstMedia->m_mapProperties.end());

    if (firstMedia->m_mediaParts.size() > 0)
      song.strFileName = firstMedia->m_mediaParts[0]->GetPath();
  }

  item.SetFromSong(song);
}


void CPlexDirectoryTypeParserArtist::Process(CFileItem &item, CFileItem &mediaContainer, TiXmlElement *itemElement)
{
  CArtist artist;

  artist.strArtist = item.GetLabel();
  artist.strBiography = item.GetProperty("summary").asString();
  item.SetProperty("description", item.GetProperty("summary"));

  item.GetMusicInfoTag()->SetArtist(artist);
}
