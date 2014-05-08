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
#include "utils/StringUtils.h"
#include "AdvancedSettings.h"
#include "guilib/LocalizeStrings.h"

#include "music/tags/MusicInfoTag.h"

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
CPlexDirectoryTypeParserVideo::Process(CFileItem &item, CFileItem &mediaContainer, XML_ELEMENT *itemElement)
{
  /* Element recevied here is the <Video> tag */
  CVideoInfoTag videoTag;
  EPlexDirectoryType dirType = item.GetPlexDirectoryType();

  videoTag.m_strFileNameAndPath = item.GetPath();
  videoTag.m_strTitle = item.GetProperty("title").asString();
  videoTag.m_strOriginalTitle = item.GetProperty("originalTitle").asString();
  videoTag.m_iYear = item.GetProperty("year").asInteger();
  videoTag.m_strPath = item.GetPath();
  videoTag.m_duration = item.GetProperty("duration").asInteger() > 0 ? item.GetProperty("duration").asInteger() / 1000 : 0;
  
  if (item.HasProperty("userRating") && item.GetProperty("userRating").asDouble() > 0.0)
  {
    videoTag.m_fRating = item.GetProperty("userRating").asDouble();
    item.SetProperty("hasUserRating", "weHazIt!");
  }
  else
    videoTag.m_fRating = item.GetProperty("rating").asDouble();
  
  if (item.HasProperty("summary") && !item.GetProperty("summary").empty())
    videoTag.m_strPlot = videoTag.m_strPlotOutline = item.GetProperty("summary").asString();
  else if (item.HasProperty("parentSummary"))
    videoTag.m_strPlot = videoTag.m_strPlotOutline = item.GetProperty("parentSummary").asString();
  
  if (item.HasProperty("viewCount"))
    videoTag.m_playCount = item.GetProperty("viewCount").asInteger();
  else
    videoTag.m_playCount = 0;

  if (item.HasProperty("grandparentTitle"))
    videoTag.m_strShowTitle = item.GetProperty("grandparentTitle").asString();
  
  item.SetArt(PLEX_ART_POSTER, item.GetArt(PLEX_ART_THUMB));

  if (dirType == PLEX_DIR_TYPE_EPISODE)
  {
    videoTag.m_iEpisode = item.GetProperty("index").asInteger();
    videoTag.m_iSeason = item.GetProperty("parentIndex").asInteger();
    if (videoTag.m_iEpisode == 0)
      item.SetProperty("allepisodes", 1);
    item.SetArt(PLEX_ART_POSTER, item.GetArt("parentThumb"));
    item.SetArt(PLEX_ART_BANNER, mediaContainer.GetArt(PLEX_ART_BANNER));

  }
  else if (dirType == PLEX_DIR_TYPE_SEASON)
  {
    videoTag.m_strShowTitle = item.GetProperty("parentTitle").asString();
    videoTag.m_iSeason = item.GetProperty("index").asInteger();
    if (!item.HasArt(PLEX_ART_THUMB) && item.HasArt("parentThumb"))
    {
      item.SetArt(PLEX_ART_THUMB, item.GetArt("parentThumb"));
    }
  }
  else if (dirType == PLEX_DIR_TYPE_SHOW)
  {
    videoTag.m_strShowTitle = videoTag.m_strTitle;
  }

  if (item.HasProperty("contentRating"))
    videoTag.m_strMPAARating = item.GetProperty("contentRating").asString();

  item.SetFromVideoInfoTag(videoTag);
  
  if (item.HasProperty("viewOffset") && item.GetProperty("viewOffset").asInteger() > 0)
    item.SetOverlayImage(CGUIListItem::ICON_OVERLAY_IN_PROGRESS);
  else
    item.SetOverlayImage(CGUIListItem::ICON_OVERLAY_UNWATCHED, videoTag.m_playCount > 0);
  
  /* for directories with leafCount and viewLeafCount */
  if ((item.GetPlexDirectoryType() == PLEX_DIR_TYPE_SHOW ||
      item.GetPlexDirectoryType() == PLEX_DIR_TYPE_SEASON) &&
      (item.HasProperty("leafCount") && item.HasProperty("viewedLeafCount")))
  {
    int numeps = item.GetProperty("leafCount").asInteger();
    int watchedeps = item.GetProperty("viewedLeafCount").asInteger();
    
    item.SetEpisodeData(numeps, watchedeps);
    item.GetVideoInfoTag()->m_iEpisode = numeps;
    item.GetVideoInfoTag()->m_playCount = watchedeps;
    if (watchedeps == numeps)
      item.SetOverlayImage(CGUIListItem::ICON_OVERLAY_WATCHED);
    else if (watchedeps == 0)
      item.SetOverlayImage(CGUIListItem::ICON_OVERLAY_UNWATCHED);
    else if (watchedeps > 0)
      item.SetOverlayImage(CGUIListItem::ICON_OVERLAY_IN_PROGRESS);
  }
  
  ParseMediaNodes(item, itemElement);

  /* Now we have the Media nodes, we need to "borrow" some properties from it */
  if (item.m_mediaItems.size() > 0)
  {
    CFileItemPtr firstMedia = item.m_mediaItems[0];
    const boost::unordered_map<CStdString, CVariant> pMap;
    std::pair<CStdString, CVariant> p;
    BOOST_FOREACH(p, pMap)
      item.SetProperty(p.first, p.second);

    /* also forward art, this is the mediaTags */
    item.AppendArt(firstMedia->GetArt());
  }
  
  item.SetProperty("selectedAudioStream", PlexUtils::GetPrettyStreamName(item, true));
  item.SetProperty("selectedSubtitleStream", PlexUtils::GetPrettyStreamName(item, false));

  // We misuse the MusicInfoTag a bit here so we can call PlayListPlayer::PlaySongId()
  if (item.HasProperty("playQueueItemID"))
    item.GetMusicInfoTag()->SetDatabaseId(item.GetProperty("playQueueItemID").asInteger(), "video");
  else
    item.GetMusicInfoTag()->SetDatabaseId(item.GetProperty("ratingKey").asInteger(), "video");
  
  //DebugPrintVideoItem(item);
}

/* Loop over <Media> tags under <Video> */
void
CPlexDirectoryTypeParserVideo::ParseMediaNodes(CFileItem &item, XML_ELEMENT *element)
{
  int mediaIndex = 0;

#ifndef USE_RAPIDXML
  for (XML_ELEMENT* media = element->FirstChildElement(); media; media = media->NextSiblingElement())
#else
  for (XML_ELEMENT* media = element->first_node(); media; media = media->next_sibling())
#endif
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
      mediaItem->SetPath(item.GetPath());
      mediaItem->SetProperty("mediaIndex", mediaIndex ++);

      /* Parse children */
      ParseMediaParts(*mediaItem, media);
      
      /* we want to make sure that the main <video> tag knows about indirect */
      if (mediaItem->HasProperty("indirect"))
        item.SetProperty("indirect", mediaItem->GetProperty("indirect"));

      /* Also forward unavailable flag */
      if (mediaItem->HasProperty("unavailable"))
        item.SetProperty("unavailable", mediaItem->GetProperty("unavailable"));

      item.m_mediaItems.push_back(mediaItem);
    }
  }

  if (item.m_mediaItems.size() == 0)
    item.SetProperty("isSynthesized", true);
  else
    item.SetProperty("isSynthesized", false);

  SetTagsAsProperties(item);
}

void CPlexDirectoryTypeParserVideo::ParseMediaParts(CFileItem &mediaItem, XML_ELEMENT* element)
{
  int partIndex = 0;
#ifndef USE_RAPIDXML
  for (XML_ELEMENT* part = element->FirstChildElement(); part; part = part->NextSiblingElement())
#else
  for (XML_ELEMENT* part = element->first_node(); part; part = part->next_sibling())
#endif
  {
    CFileItemPtr mediaPart = CPlexDirectory::NewPlexElement(part, mediaItem, mediaItem.GetPath());
    mediaPart->SetProperty("partIndex", partIndex ++);

    ParseMediaStreams(*mediaPart, part);
    
    if ((mediaPart->HasProperty("exists") && !mediaPart->GetProperty("exists").asBoolean()) ||
        (mediaPart->HasProperty("accessible") && !mediaPart->GetProperty("accessible").asBoolean()))
      mediaItem.SetProperty("unavailable", true);
    
    if (mediaPart->IsDVDImage() || mediaPart->IsDVD() || mediaPart->IsDVDFile())
      mediaItem.SetProperty("isdvd", true);

    mediaItem.m_mediaParts.push_back(mediaPart);
  }
}

void CPlexDirectoryTypeParserVideo::ParseMediaStreams(CFileItem &mediaPart, XML_ELEMENT* element)
{
#ifndef USE_RAPIDXML
  for (XML_ELEMENT* stream = element->FirstChildElement(); stream; stream = stream->NextSiblingElement())
#else
  for (XML_ELEMENT* stream = element->first_node(); stream; stream = stream->next_sibling())
#endif
  {
    CFileItemPtr mediaStream = CPlexDirectory::NewPlexElement(stream, mediaPart, mediaPart.GetPath());

    CStdString streamName = PlexUtils::GetPrettyStreamNameFromStreamItem(mediaStream);
    mediaStream->SetLabel(streamName);

    /* FIXME: legacy, calling code should check if the
     * property is set instead, but for now we don't want
     * to audit all that shit */
    if (!mediaStream->HasProperty("subIndex"))
      mediaStream->SetProperty("subIndex", -1);

    if (!mediaStream->HasProperty("index"))
      mediaStream->SetProperty("index", -1);
    
    if (mediaStream->HasProperty("selected"))
      mediaStream->Select(mediaStream->GetProperty("selected").asBoolean());
    
    mediaPart.m_mediaPartStreams.push_back(mediaStream);
  }

}

void CPlexDirectoryTypeParserVideo::ParseTag(CFileItem &item, CFileItem &tagItem)
{
  if (!item.HasVideoInfoTag())
    return;

  CVideoInfoTag* tag = item.GetVideoInfoTag();
  CStdString tagVal = tagItem.GetProperty("tag").asString();
  switch(tagItem.GetPlexDirectoryType())
  {
    case PLEX_DIR_TYPE_GENRE:
      // limit to two genres for now
      if (tag->m_genre.size() < 2)
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
  if (!item.HasVideoInfoTag())
    return;

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
