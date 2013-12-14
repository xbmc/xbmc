//
//  PlexDirectoryTypeParserPicture.h
//  Plex
//
//  Created by Tobias Hieta <tobias@plexapp.com> on 2013-06-21.
//  Copyright 2013 Plex Inc. All rights reserved.
//

#ifndef PLEXDIRECTORYTYPEPARSERPICTURE_H
#define PLEXDIRECTORYTYPEPARSERPICTURE_H

#include "PlexDirectoryTypeParserVideo.h"

class CPlexDirectoryTypeParserPicture : public CPlexDirectoryTypeParserVideo
{
  public:
    CPlexDirectoryTypeParserPicture() {}
    void Process(CFileItem &item, CFileItem &mediaContainer, XML_ELEMENT *itemElement);
};

#endif // PLEXDIRECTORYTYPEPARSERPICTURE_H
