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
#include "XBMCTinyXML.h"

class CPlexDirectoryTypeParserAlbum : public CPlexDirectoryTypeParserBase
{
  public:
    virtual void Process(CFileItem& item, CFileItem& mediaContainer, TiXmlElement* itemElement);
};

#endif // PLEXDIRECTORYTYPEPARSERMUSIC_H
