//
//  PlexDirectoryTypeParserPicture.cpp
//  Plex
//
//  Created by Tobias Hieta <tobias@plexapp.com> on 2013-06-21.
//  Copyright 2013 Plex Inc. All rights reserved.
//

#include "PlexDirectoryTypeParserPicture.h"
#include "PlexAttributeParser.h"

void CPlexDirectoryTypeParserPicture::Process(CFileItem &item, CFileItem &mediaContainer, TiXmlElement *itemElement)
{
  ParseMediaNodes(item, itemElement);
  if (item.m_mediaItems.size() > 0 && item.m_mediaItems[0]->m_mediaParts.size() > 0)
  {
    /* Pass the path URL via the transcoder, because XBMC raw file reader sucks */
    CPlexAttributeParserMediaUrl mUrl;
    mUrl.Process(item.GetPath(), "picture", item.m_mediaItems[0]->m_mediaParts[0]->GetProperty("unprocessed_key").asString(), &item);
    item.SetPath(item.GetArt("picture"));
    CLog::Log(LOGDEBUG, "CPlexDirectoryTypeParserPicture: setting key = %s", item.GetPath().c_str());
  }
}
