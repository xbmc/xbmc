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
#include "XBMCTinyXML.h"

class CPlexDirectoryTypeParserVideo : public CPlexDirectoryTypeParserBase
{
  public:
    virtual void Process(CFileItem& item, CFileItem& mediaContainer, TiXmlElement* itemElement);

    void ParseMediaNodes(CFileItem& item, TiXmlElement* element);
    void ParseMediaParts(CFileItem& mediaItem, TiXmlElement* element);
    void ParseMediaStreams(CFileItem& mediaPart, TiXmlElement* element);
    void ParseTag(CFileItem& item, CFileItem& tagItem);

    static void DebugPrintVideoItem(const CFileItem &item);

    void SetTagsAsProperties(CFileItem& item);
};

#endif // PLEXDIRECTORYTYPEPARSERVIDEO_H
