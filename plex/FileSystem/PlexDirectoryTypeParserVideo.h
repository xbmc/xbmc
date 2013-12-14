//
//  PlexDirectoryTypeParserVideo.h
//  Plex
//
//  Created by Tobias Hieta <tobias@plexapp.com> on 2013-04-09.
//  Copyright 2013 Plex Inc. All rights reserved.
//

#ifndef PLEXDIRECTORYTYPEPARSERVIDEO_H
#define PLEXDIRECTORYTYPEPARSERVIDEO_H

class CPlexDirectoryTypeParserVideo;

#include "PlexDirectoryTypeParser.h"
#include "XMLChoice.h"

class CPlexDirectoryTypeParserVideo : public CPlexDirectoryTypeParserBase
{
  public:
    virtual void Process(CFileItem& item, CFileItem& mediaContainer, XML_ELEMENT* itemElement);

    void ParseMediaNodes(CFileItem& item, XML_ELEMENT* element);
    void ParseMediaParts(CFileItem& mediaItem, XML_ELEMENT* element);
    void ParseMediaStreams(CFileItem& mediaPart, XML_ELEMENT* element);
    virtual void ParseTag(CFileItem& item, CFileItem& tagItem);

    static void DebugPrintVideoItem(const CFileItem &item);

    virtual void SetTagsAsProperties(CFileItem& item);
};

#endif // PLEXDIRECTORYTYPEPARSERVIDEO_H
