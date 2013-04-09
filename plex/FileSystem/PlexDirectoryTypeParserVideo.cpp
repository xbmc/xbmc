//
//  PlexDirectoryTypeParserVideo.cpp
//  Plex
//
//  Created by Tobias Hieta <tobias@plexapp.com> on 2013-04-09.
//  Copyright 2013 Plex Inc. All rights reserved.
//

#include "PlexDirectoryTypeParserVideo.h"
#include "FileItem.h"
#include "PlexDirectory.h"
#include "video/VideoInfoTag.h"
#include "PlexTypes.h"
#include "utils/log.h"

#include <boost/foreach.hpp>

using namespace XFILE;

///////////////////////////////////////////////////////////////////
/* Example Video tag (without attributes):
 * <Video>
 *   <Media>
 *      <Part>
 *        <Stream/>
 *        <Stream/>
 *      </Part>
 *   </Media>
 * </Video>
 *
 * This parser parses <Video> tags. It starts by setting up the Video container
 * in Process(), then it goes down into ParseMediaNodes() that loops over all
 * <Media> elements and parses them.
 * Each Media node contains <Part> which is handled by ParseMediaParts()
 * and each Part contains <Stream> that is handled by ParseMediaStreams()
 *
 * Each element is represented by a CFileItemPtr
 */

void
CPlexDirectoryTypeParserVideo::Process(CFileItem &item, CFileItem &mediaContainer, TiXmlElement *itemElement)
{
  /* Element recevied here is the <Video> tag */
  CVideoInfoTag videoTag;
  EPlexDirectoryType dirType = item.GetPlexDirectoryType();

  videoTag.m_strTitle = item.GetProperty("title").asString();
  videoTag.m_strOriginalTitle = item.GetProperty("originalTitle").asString();
  videoTag.m_strPlot = videoTag.m_strPlotOutline = item.GetProperty("summary").asString();
  videoTag.m_iYear = item.GetProperty("year").asInteger();
  videoTag.m_strPath = item.GetPath();
  videoTag.m_duration = item.GetProperty("duration").asInteger() > 0 ? item.GetProperty("duration").asInteger() / 1000 : 0;

  if (item.HasProperty("grandparentTitle"))
    videoTag.m_strShowTitle = item.GetProperty("grandparentTitle").asString();

  if (dirType == PLEX_DIR_TYPE_EPISODE)
  {
    videoTag.m_iEpisode = item.GetProperty("index").asInteger();
    videoTag.m_iSeason = item.GetProperty("parentIndex").asInteger();
  }
  else if (dirType == PLEX_DIR_TYPE_SEASON)
  {
    videoTag.m_strShowTitle = item.GetProperty("parentTitle").asString();
    videoTag.m_iSeason = item.GetProperty("index").asInteger();
  }
  else if (dirType == PLEX_DIR_TYPE_SHOW)
  {
    videoTag.m_strShowTitle = videoTag.m_strTitle;
  }

  ParseMediaNodes(item, itemElement);

  item.SetFromVideoInfoTag(videoTag);
  item.m_bIsFolder = true;

  //DebugPrintVideoItem(item);
}

/* Loop over <Media> tags under <Video> */
void
CPlexDirectoryTypeParserVideo::ParseMediaNodes(CFileItem &item, TiXmlElement *element)
{
  for (TiXmlElement* media = element->FirstChildElement(); media ; media = media->NextSiblingElement())
  {
    CFileItemPtr mediaItem = CPlexDirectory::NewPlexElement(media, item, item.GetPath());

    /* these items are not folders */
    mediaItem->m_bIsFolder = false;

    /* Parse children */
    ParseMediaParts(*mediaItem, media);

    item.m_mediaItems.push_back(mediaItem);
  }
}

void CPlexDirectoryTypeParserVideo::ParseMediaParts(CFileItem &mediaItem, TiXmlElement* element)
{
  for (TiXmlElement* part = element->FirstChildElement(); part; part = part->NextSiblingElement())
  {
    CFileItemPtr mediaPart = CPlexDirectory::NewPlexElement(part, mediaItem, mediaItem.GetPath());

    ParseMediaStreams(*mediaPart, part);

    mediaItem.m_mediaParts.push_back(mediaPart);
  }
}

void CPlexDirectoryTypeParserVideo::ParseMediaStreams(CFileItem &mediaPart, TiXmlElement* element)
{
  for (TiXmlElement* stream = element->FirstChildElement(); stream; stream = stream->NextSiblingElement())
  {
    CFileItemPtr mediaStream = CPlexDirectory::NewPlexElement(stream, mediaPart, mediaPart.GetPath());
    mediaPart.m_mediaPartStreams.push_back(mediaStream);
  }
}

void CPlexDirectoryTypeParserVideo::DebugPrintVideoItem(const CFileItem &item)
{
  CLog::Log(LOGDEBUG, "******************* DEBUG PRINTOUT of item %s", item.GetPath().c_str());
  CLog::Log(LOGDEBUG, "Label: %s\nPath: %s\nType: %s", item.GetLabel().c_str(), item.GetPath().c_str(), CPlexDirectory::GetDirectoryTypeString(item.GetPlexDirectoryType()).c_str());

  BOOST_FOREACH(CFileItemPtr mItem, item.m_mediaItems)
  {
    CLog::Log(LOGDEBUG, "** Media Item: %s", mItem->GetProperty("id").asString().c_str());
    BOOST_FOREACH(CFileItemPtr pItem, mItem->m_mediaParts)
    {
      CLog::Log(LOGDEBUG, "**** Media Part: %s", pItem->GetProperty("file").asString().c_str());
      BOOST_FOREACH(CFileItemPtr sItem, pItem->m_mediaPartStreams)
      {
        CLog::Log(LOGDEBUG, "****** Media Stream: %lld = %s", sItem->GetProperty("streamType").asInteger(), sItem->GetProperty("language").asString().c_str());
      }
    }
  }
}
