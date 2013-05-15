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
#include "Utils/StringUtils.h"
#include "AdvancedSettings.h"

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
 *   <Genre />
 *   <Role />
 *   <Producer />
 * </Video>
 *
 * This parser parses <Video> tags. It starts by setting up the Video container
 * in Process(), then it goes down into ParseMediaNodes() that loops over all
 * <Media> elements and parses them.
 * Each Media node contains <Part> which is handled by ParseMediaParts()
 * and each Part contains <Stream> that is handled by ParseMediaStreams()
 *
 * Tag elements like Role, Genre and Producer is handled by ParseTag()
 *
 * Each element is represented by a CFileItemPtr
 */

void
CPlexDirectoryTypeParserVideo::Process(CFileItem &item, CFileItem &mediaContainer, TiXmlElement *itemElement)
{
  /* Element recevied here is the <Video> tag */
  CVideoInfoTag videoTag;
  EPlexDirectoryType dirType = item.GetPlexDirectoryType();

  item.m_bIsFolder = false;

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
    item.m_bIsFolder = true;
  }
  else if (dirType == PLEX_DIR_TYPE_SHOW)
  {
    videoTag.m_strShowTitle = videoTag.m_strTitle;
    item.m_bIsFolder = true;
  }

  item.SetFromVideoInfoTag(videoTag);

  ParseMediaNodes(item, itemElement);

  /* Now we have the Media nodes, we need to "borrow" some properties from it */
  if (item.m_mediaItems.size() > 0)
  {
    CFileItemPtr firstMedia = item.m_mediaItems[0];
    item.m_mapProperties.insert(firstMedia->m_mapProperties.begin(), firstMedia->m_mapProperties.end());

    /* also forward art, this is the mediaTags */
    item.AppendArt(firstMedia->GetArt());
  }

  //DebugPrintVideoItem(item);
}

/* Loop over <Media> tags under <Video> */
void
CPlexDirectoryTypeParserVideo::ParseMediaNodes(CFileItem &item, TiXmlElement *element)
{
  for (TiXmlElement* media = element->FirstChildElement(); media ; media = media->NextSiblingElement())
  {
    CFileItemPtr mediaItem = CPlexDirectory::NewPlexElement(media, item, item.GetPath());

    if (mediaItem->GetPlexDirectoryType() == PLEX_DIR_TYPE_GENRE ||
        mediaItem->GetPlexDirectoryType() == PLEX_DIR_TYPE_WRITER ||
        mediaItem->GetPlexDirectoryType() == PLEX_DIR_TYPE_DIRECTOR ||
        mediaItem->GetPlexDirectoryType() == PLEX_DIR_TYPE_PRODUCER ||
        mediaItem->GetPlexDirectoryType() == PLEX_DIR_TYPE_ROLE ||
        mediaItem->GetPlexDirectoryType() == PLEX_DIR_TYPE_COUNTRY)
    {
      ParseTag(item, *mediaItem);
    }
    else if (mediaItem->GetPlexDirectoryType() == PLEX_DIR_TYPE_MEDIA)
    {
      /* these items are not folders */
      mediaItem->m_bIsFolder = false;

      /* Parse children */
      ParseMediaParts(*mediaItem, media);

      item.m_mediaItems.push_back(mediaItem);
    }
  }

  SetTagsAsProperties(item);
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

    /* Set a sensible Label so we can use this in dialogs */
    CStdString streamName = !mediaStream->HasProperty("language") ? mediaStream->GetProperty("language").asString() : "Unknown";
    if (mediaStream->GetProperty("streamType").asInteger() == PLEX_STREAM_AUDIO)
      streamName += " (" + PlexUtils::GetStreamCodecName(mediaStream) + " " + boost::to_upper_copy(PlexUtils::GetStreamChannelName(mediaStream)) + ")";
    else if (mediaStream->GetProperty("streamType").asInteger() == PLEX_STREAM_VIDEO)
      streamName += " (" + PlexUtils::GetStreamCodecName(mediaStream) + ")";

    mediaStream->SetLabel(streamName);

    mediaPart.m_mediaPartStreams.push_back(mediaStream);
  }

}

void CPlexDirectoryTypeParserVideo::ParseTag(CFileItem &item, CFileItem &tagItem)
{
  CVideoInfoTag* tag = item.GetVideoInfoTag();
  CStdString tagVal = tagItem.GetProperty("tag").asString();
  switch(tagItem.GetPlexDirectoryType())
  {
    case PLEX_DIR_TYPE_GENRE:
      tag->m_genre.push_back(tagVal);
      break;
    case PLEX_DIR_TYPE_WRITER:
      tag->m_writingCredits.push_back(tagVal);
      break;
    case PLEX_DIR_TYPE_DIRECTOR:
      tag->m_director.push_back(tagVal);
      break;
    case PLEX_DIR_TYPE_COUNTRY:
      tag->m_country.push_back(tagVal);
      break;
    case PLEX_DIR_TYPE_PRODUCER:
      /* not in VideoInfoTag? */
      break;
    case PLEX_DIR_TYPE_ROLE:
    {
      SActorInfo actor;
      actor.strName = tagVal;
      actor.strRole = tagItem.GetProperty("role").asString();
      actor.thumb = tagItem.GetArt("thumb");
      tag->m_cast.push_back(actor);
    }
      break;
    default:
      CLog::Log(LOGINFO, "CPlexDirectoryTypeParserVideo::ParseTag I have no idea how to handle %d", tagItem.GetPlexDirectoryType());
      break;
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

void CPlexDirectoryTypeParserVideo::SetTagsAsProperties(CFileItem &item)
{
  CVideoInfoTag* infoTag = item.GetVideoInfoTag();

  if (infoTag->m_genre.size() > 0)
    item.SetProperty("genre", StringUtils::Join(infoTag->m_genre, g_advancedSettings.m_videoItemSeparator));
  if (infoTag->m_writingCredits.size() > 0)
    item.SetProperty("writingCredits", StringUtils::Join(infoTag->m_writingCredits, g_advancedSettings.m_videoItemSeparator));
  if (infoTag->m_director.size() > 0)
    item.SetProperty("director", StringUtils::Join(infoTag->m_director, g_advancedSettings.m_videoItemSeparator));
  if (infoTag->m_country.size() > 0)
    item.SetProperty("country", StringUtils::Join(infoTag->m_country, g_advancedSettings.m_videoItemSeparator));
  if (infoTag->m_cast.size() > 0)
  {
    std::vector<std::string> cast;
    BOOST_FOREACH(SActorInfo &actor, infoTag->m_cast)
    {
      cast.push_back(actor.strName);
    }
    item.SetProperty("cast", StringUtils::Join(cast, g_advancedSettings.m_videoItemSeparator));
  }
}
