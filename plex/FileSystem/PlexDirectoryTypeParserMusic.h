//
//  PlexDirectoryTypeParserMusic.h
//  Plex
//
//  Created by Tobias Hieta <tobias@plexapp.com> on 2013-04-09.
//  Copyright 2013 Plex Inc. All rights reserved.
//

#ifndef PLEXDIRECTORYTYPEPARSERMUSIC_H
#define PLEXDIRECTORYTYPEPARSERMUSIC_H

#include "PlexDirectoryTypeParser.h"
#include "PlexDirectoryTypeParserVideo.h"
#include "XBMCTinyXML.h"

class CPlexDirectoryTypeParserAlbum : public CPlexDirectoryTypeParserBase
{
  public:
    virtual void Process(CFileItem& item, CFileItem& mediaContainer, XML_ELEMENT* itemElement);
    void ParseTag(CFileItem &item, CFileItem &tagItem);
};

class CPlexDirectoryTypeParserTrack : public CPlexDirectoryTypeParserVideo
{
public:
  CPlexDirectoryTypeParserTrack() {}
  virtual void Process(CFileItem& item, CFileItem& mediaContainer, XML_ELEMENT* itemElement);
};

class CPlexDirectoryTypeParserArtist : public CPlexDirectoryTypeParserAlbum
{
public:
  CPlexDirectoryTypeParserArtist() {}
  virtual void Process(CFileItem& item, CFileItem& mediaContainer, XML_ELEMENT* itemElement);
};


#endif // PLEXDIRECTORYTYPEPARSERMUSIC_H
