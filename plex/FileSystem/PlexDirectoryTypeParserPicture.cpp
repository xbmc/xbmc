//
//  PlexDirectoryTypeParserPicture.cpp
//  Plex
//
//  Created by Tobias Hieta <tobias@plexapp.com> on 2013-06-21.
//  Copyright 2013 Plex Inc. All rights reserved.
//

#include "PlexDirectoryTypeParserPicture.h"

void CPlexDirectoryTypeParserPicture::Process(CFileItem &item, CFileItem &mediaContainer, TiXmlElement *itemElement)
{
  ParseMediaNodes(item, itemElement);
  if (item.m_mediaItems.size() > 0 && item.m_mediaItems[0]->m_mediaParts.size() > 0)
    item.SetPath(item.m_mediaItems[0]->m_mediaParts[0]->GetPath());
}
