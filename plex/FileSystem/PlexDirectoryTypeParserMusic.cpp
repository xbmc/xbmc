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
#include "music/tags/MusicInfoTag.h"

using namespace MUSIC_INFO;

void
CPlexDirectoryTypeParserAlbum::Process(CFileItem &item, CFileItem &mediaContainer, XML_ELEMENT *itemElement)
{
  CAlbum album;

  CStdString albumName = item.GetLabel();
  if (albumName.empty() && item.HasProperty("album"))
  {
    albumName = item.GetProperty("album").asString();
    item.SetLabel(albumName);
  }

  album.strLabel = albumName;
  album.strAlbum = albumName;
  album.iYear = item.GetProperty("year").asInteger();
  album.m_strDateOfRelease = item.GetProperty("originallyAvailableAt").asString();

  if(item.HasProperty("parentTitle"))
    album.artist.push_back(item.GetProperty("parentTitle").asString());
  else if (mediaContainer.HasProperty("parentTitle"))
    album.artist.push_back(mediaContainer.GetProperty("parentTitle").asString());
  else if (item.HasProperty("artist"))
    album.artist.push_back(item.GetProperty("artist").asString());

  if (item.HasProperty("genre"))
    album.genre.push_back(item.GetProperty("genre").asString());

  item.SetFromAlbum(album);

  item.SetProperty("description", item.GetProperty("summary"));
  if (!item.HasArt(PLEX_ART_THUMB))
    item.SetArt(PLEX_ART_THUMB, mediaContainer.GetArt(PLEX_ART_THUMB));

  #ifdef USE_RAPIDXML
  for (XML_ELEMENT *el = itemElement->first_node(); el; el = el->next_sibling())
  #else
  for (XML_ELEMENT *el = itemElement->FirstChildElement(); el; el = el->NextSiblingElement())
  #endif
  {
    CFileItemPtr tagItem = XFILE::CPlexDirectory::NewPlexElement(el, item, item.GetPath());

    if (tagItem &&
        tagItem->GetPlexDirectoryType() == PLEX_DIR_TYPE_GENRE)
      ParseTag(item, *tagItem.get());
  }
}

void CPlexDirectoryTypeParserAlbum::ParseTag(CFileItem &item, CFileItem &tagItem)
{
  if (!item.HasMusicInfoTag())
    return;

  CMusicInfoTag* tag = item.GetMusicInfoTag();
  CStdString tagVal = tagItem.GetProperty("tag").asString();
  switch(tagItem.GetPlexDirectoryType())
  {
    case PLEX_DIR_TYPE_GENRE:
    {
      std::vector<std::string> genres = tag->GetGenre();
      genres.push_back(tagVal);
      tag->SetGenre(genres);
    }
      break;
    default:
      CLog::Log(LOGINFO, "CPlexDirectoryTypeParserAlbum::ParseTag I have no idea how to handle %d", tagItem.GetPlexDirectoryType());
      break;
  }
}

void
CPlexDirectoryTypeParserTrack::Process(CFileItem &item, CFileItem &mediaContainer, XML_ELEMENT *itemElement)
{
  CSong song;

  CStdString songName = item.GetLabel();
  if (songName.empty() && item.HasProperty("track"))
  {
    songName = item.GetProperty("track").asString();
    item.SetLabel(songName);
  }

  song.strTitle = songName;
  song.strComment = item.GetProperty("summary").asString();
  if (item.HasProperty("duration"))
    song.iDuration = item.GetProperty("duration").asInteger() / 1000;
  else if (item.HasProperty("totalTime"))
    song.iDuration = item.GetProperty("totalTime").asInteger() / 1000;

  song.iTrack = item.GetProperty("index").asInteger();

  if (item.HasProperty("parentYear"))
    song.iYear = item.GetProperty("parentYear").asInteger();
  else if (mediaContainer.HasProperty("parentYear"))
    song.iYear = mediaContainer.GetProperty("parentYear").asInteger();

  if (!item.HasArt(PLEX_ART_THUMB) && mediaContainer.HasArt(PLEX_ART_THUMB))
    item.SetArt(PLEX_ART_THUMB, mediaContainer.GetArt(PLEX_ART_THUMB));
  song.strThumb = item.GetArt(PLEX_ART_THUMB);

  if (item.HasProperty("grandparentTitle"))
    song.artist.push_back(item.GetProperty("grandparentTitle").asString());
  else if (mediaContainer.HasProperty("grandparentTitle"))
    song.artist.push_back(mediaContainer.GetProperty("grandparentTitle").asString());
  else if (item.HasProperty("artist"))
    song.artist.push_back(item.GetProperty("artist").asString());

  if (item.HasProperty("parentTitle"))
    song.strAlbum = item.GetProperty("parentTitle").asString();
  else if (mediaContainer.HasProperty("parentTitle"))
    song.strAlbum = mediaContainer.GetProperty("parentTitle").asString();
  else if (item.HasProperty("album"))
    song.strAlbum = item.GetProperty("album").asString();

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
    const PropertyMap pMap = firstMedia->GetAllProperties();
    std::pair<CStdString, CVariant> p;
    BOOST_FOREACH(p, pMap)
      item.SetProperty(p.first, p.second);

    if (firstMedia->m_mediaParts.size() > 0)
      song.strFileName = firstMedia->m_mediaParts[0]->GetPath();
  }

  item.SetFromSong(song);

  if (item.HasProperty("playQueueItemID"))
    item.GetMusicInfoTag()->SetDatabaseId(item.GetProperty("playQueueItemID").asInteger(), "song");
  else
    item.GetMusicInfoTag()->SetDatabaseId(item.GetProperty("ratingKey").asInteger(), "song");
}

void CPlexDirectoryTypeParserArtist::Process(CFileItem &item, CFileItem &mediaContainer, XML_ELEMENT *itemElement)
{
  CArtist artist;

  CStdString artistName = item.GetLabel();
  if (item.GetLabel().empty() && item.HasProperty("artist"))
  {
    artistName = item.GetProperty("artist").asString();
    item.SetLabel(artistName);
  }

  artist.strArtist = artistName;
  artist.strBiography = item.GetProperty("summary").asString();
  item.SetProperty("description", item.GetProperty("summary"));

  item.GetMusicInfoTag()->SetArtist(artist);

  int thumbIdx = 0;
  #ifdef USE_RAPIDXML
  for (XML_ELEMENT *el = itemElement->first_node(); el; el = el->next_sibling())
  #else
  for (XML_ELEMENT *el = itemElement->FirstChildElement(); el; el = el->NextSiblingElement())
  #endif
  {
    CFileItemPtr tagItem = XFILE::CPlexDirectory::NewPlexElement(el, item, item.GetPath());

    if (tagItem &&
        tagItem->GetPlexDirectoryType() == PLEX_DIR_TYPE_GENRE)
      ParseTag(item, *tagItem.get());
    else if (tagItem && tagItem->GetPlexDirectoryType() == PLEX_DIR_TYPE_THUMB)
      item.SetArt(PLEX_ART_THUMB, thumbIdx ++, tagItem->GetPath());
  }

  item.GetMusicInfoTag()->SetDatabaseId(item.GetProperty("ratingKey").asInteger(), "artist");
}
