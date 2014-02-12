//
//  PlexDirectoryTypeParser.h
//  Plex
//
//  Created by Tobias Hieta <tobias@plexapp.com> on 2013-04-08.
//  Copyright 2013 Plex Inc. All rights reserved.
//

#ifndef PLEXDIRECTORYTYPEPARSER_H
#define PLEXDIRECTORYTYPEPARSER_H

#include "FileItem.h"
#include "FileSystem/PlexDirectory.h"
#include "XMLChoice.h"

class CPlexDirectoryTypeParserBase
{
  public:
    CPlexDirectoryTypeParserBase() {}
    virtual void Process(CFileItem& item, CFileItem& mediaContainer, XML_ELEMENT* itemElement) {}

    static CPlexDirectoryTypeParserBase* GetDirectoryTypeParser(EPlexDirectoryType type);
};


#endif // PLEXDIRECTORYTYPEPARSER_H
