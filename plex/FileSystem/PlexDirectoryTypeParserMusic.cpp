//
//  PlexDirectoryTypeParserMusic.cpp
//  Plex
//
//  Created by Tobias Hieta <tobias@plexapp.com> on 2013-04-09.
//  Copyright 2013 Plex Inc. All rights reserved.
//

#include "PlexDirectoryTypeParserMusic.h"

#include "music/Album.h"

void
CPlexDirectoryTypeParserAlbum::Process(CFileItem &item, CFileItem &mediaContainer, TiXmlElement *itemElement)
{
  CAlbum album;
  album.strLabel = item.GetProperty("title").asString();
  album.strAlbum = item.GetProperty("title").asString();
  album.artist.push_back(item.GetProperty("parentTitle").asString());
  album.genre.push_back(item.GetProperty("genre").asString());
  album.iYear = item.GetProperty("year").asInteger();
  album.m_strDateOfRelease = item.GetProperty("originallyAvailableAt").asString();

  item.SetFromAlbum(album);
}
