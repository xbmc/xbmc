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

class CPlexDirectoryTypeParserBase
{
  public:
    CPlexDirectoryTypeParserBase() {}
    virtual void Process(CFileItem& item, CFileItem& mediaContainer, TiXmlElement* itemElement) {}

    static CPlexDirectoryTypeParserBase* GetDirectoryTypeParser(EPlexDirectoryType type);
};

class CPlexDirectoryTypeParserDirectory : public CPlexDirectoryTypeParserBase
{
  public:
    virtual void Process(CFileItem& item, CFileItem& mediaContainer, TiXmlElement* itemElement)
    {
      item.m_bIsFolder = true;
    }
};

#endif // PLEXDIRECTORYTYPEPARSER_H
