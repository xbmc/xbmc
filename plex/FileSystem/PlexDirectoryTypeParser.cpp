//
//  PlexDirectoryTypeParser.cpp
//  Plex
//
//  Created by Tobias Hieta <tobias@plexapp.com> on 2013-04-08.
//  Copyright 2013 Plex Inc. All rights reserved.
//

#include "PlexDirectoryTypeParser.h"
#include "video/VideoInfoTag.h"
#include "plex/PlexTypes.h"

#include "PlexDirectoryTypeParserVideo.h"
#include "PlexDirectoryTypeParserMusic.h"
#include "PlexDirectoryTypeParserPicture.h"
#include "PlexDirectoryTypeParserRelease.h"

static CPlexDirectoryTypeParserBase* videoParser = new CPlexDirectoryTypeParserVideo;
static CPlexDirectoryTypeParserBase* albumParser = new CPlexDirectoryTypeParserAlbum;
static CPlexDirectoryTypeParserBase* baseParser = new CPlexDirectoryTypeParserBase;
static CPlexDirectoryTypeParserBase* trackParser = new CPlexDirectoryTypeParserTrack;
static CPlexDirectoryTypeParserBase* artistParser = new CPlexDirectoryTypeParserArtist;
static CPlexDirectoryTypeParserBase* pictureParser = new CPlexDirectoryTypeParserPicture;
static CPlexDirectoryTypeParserBase* releaseParser = new CPlexDirectoryTypeParserRelease;

CPlexDirectoryTypeParserBase*
CPlexDirectoryTypeParserBase::GetDirectoryTypeParser(EPlexDirectoryType type)
{
  if (type == PLEX_DIR_TYPE_MOVIE ||
      type == PLEX_DIR_TYPE_EPISODE ||
      type == PLEX_DIR_TYPE_SHOW ||
      type == PLEX_DIR_TYPE_SEASON ||
      type == PLEX_DIR_TYPE_CLIP ||
      type == PLEX_DIR_TYPE_VIDEO)
    return videoParser;
  
  else if (type == PLEX_DIR_TYPE_TRACK)
    return trackParser;

  else if (type == PLEX_DIR_TYPE_ALBUM)
    return albumParser;

  else if (type == PLEX_DIR_TYPE_ARTIST)
    return artistParser;

  else if (type == PLEX_DIR_TYPE_PHOTO ||
           type == PLEX_DIR_TYPE_PHOTOALBUM)
    return pictureParser;

  else if (type == PLEX_DIR_TYPE_RELEASE)
    return releaseParser;

  return baseParser;
}
