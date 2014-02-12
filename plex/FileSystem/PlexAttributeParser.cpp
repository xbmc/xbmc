//
//  PlexAttributeParser.cpp
//  Plex
//
//  Created by Tobias Hieta <tobias@plexapp.com> on 2013-04-05.
//  Copyright 2013 Plex Inc. All rights reserved.
//

#include "PlexAttributeParser.h"
#include "PlexDirectory.h"
#include "utils/log.h"

#include <string>

#include <boost/algorithm/string.hpp>

#include "AdvancedSettings.h"

#include "Client/PlexServer.h"
#include "Client/PlexServerManager.h"
#include "StringUtils.h"
#include "URL.h"
#include "PlexApplication.h"

////////////////////////////////////////////////////////////////////////////////
void CPlexAttributeParserBase::Process(const CURL& url, const CStdString& key, const CStdString& value, CFileItem *item)
{
  item->SetProperty(key, value);
}

////////////////////////////////////////////////////////////////////////////////
int64_t CPlexAttributeParserInt::GetInt(const CStdString &value)
{
  int64_t intval;
  try { intval = boost::lexical_cast<int64_t>(value); }
  catch (boost::bad_lexical_cast &e){ return -1; }
  return intval;
}

void CPlexAttributeParserInt::Process(const CURL& url, const CStdString &key, const CStdString &value, CFileItem *item)
{
  item->SetProperty("unprocessed_" + key, value);

  int64_t intval = GetInt(value);
  if (intval == -1)
    return;

  item->SetProperty(key, intval);
}

////////////////////////////////////////////////////////////////////////////////
void CPlexAttributeParserBool::Process(const CURL& url, const CStdString &key, const CStdString &value, CFileItem *item)
{
  int64_t intval = GetInt(value);

  if (value == "true")
    item->SetProperty(key, true);
  else if (value == "false")
    item->SetProperty(key, false);
  if (intval == -1)
    item->SetProperty(key, (bool)!value.empty());
  else
    item->SetProperty(key, (bool)intval);
}

////////////////////////////////////////////////////////////////////////////////
void CPlexAttributeParserKey::Process(const CURL& url, const CStdString &key, const CStdString &value, CFileItem *item)
{
  CURL keyUrl(url);

  if (boost::starts_with(value, "/"))
  {
    /* cut of the first / and set it */
    keyUrl.SetFileName(value.substr(1, std::string::npos));
    keyUrl.SetOptions("");
  }
  else if (boost::starts_with(value, "http://") || boost::starts_with(value, "https://"))
  {
    keyUrl = value;
    if (keyUrl.GetHostName() == "node.plexapp.com")
    {
      keyUrl.SetProtocol("plexserver");
      keyUrl.SetHostName("node");
      keyUrl.SetPort(0);
    }
  }
  else
  {
    PlexUtils::AppendPathToURL(keyUrl, value);
  }

  item->SetProperty(key, keyUrl.Get());
  item->SetProperty("unprocessed_" + key, value);
}

////////////////////////////////////////////////////////////////////////////////
void CPlexAttributeParserMediaUrl::Process(const CURL &url, const CStdString &key, const CStdString &value, CFileItem *item)
{
  CURL mediaUrl(url);
  CURL imageURL;
  
  /* reset the options to not bust cache stuff */
  mediaUrl.SetOptions("");

  if ((mediaUrl.GetHostName() == "myplex" || mediaUrl.GetHostName() == "node") && g_plexApplication.serverManager)
  {
    CPlexServerPtr bestServer = g_plexApplication.serverManager->GetBestServer();
    if (bestServer)
      mediaUrl.SetHostName(bestServer->GetUUID());
    else
      /* if we don't have a best server we use myPlex even for node content */
      mediaUrl.SetHostName("myplex");
  }

  if (boost::starts_with(value, "http://") || boost::starts_with(value, "https://"))
  {
    imageURL = CURL(value);
  }
  else
  {
    imageURL.SetProtocol("http");
    imageURL.SetHostName("127.0.0.1");
    imageURL.SetPort(32400);
    if (boost::starts_with(value, "/"))
      imageURL.SetFileName(value.substr(1, std::string::npos));
    else
      imageURL.SetFileName(value);
  }

  CStdString width="0", height="0";
  CStdString propertyName = key;
  if (key == "thumb" || key == "poster" || key == "grandparentThumb")
  {
    width = height = "320";
    if (key == "poster")
    {
      Process(url, "bigPoster", value, item);
      Process(url, "originalPoster", value, item);
    }
    else if (key == "thumb")
      Process(url, "bigThumb", value, item);
    else if (key == "grandparentThumb")
      Process(url, "bigGrandparentThumb", value, item);
  }
  else if (key == "bigPoster" || key == "bigThumb" || key == "bigGrandparentThumb")
  {
    width = height = "720";
  }
  else if (key == "banner")
  {
    width = "800";
    height = "200";
  }
  else if (key == "art")
  {
    width = "1920";
    height = "1080";
    propertyName = PLEX_ART_FANART;
  }
  else if (key == "picture")
  {
    width = "1920";
    height = "1080";
  }

  mediaUrl.SetOption("width", width);
  mediaUrl.SetOption("height", height);
  mediaUrl.SetOption("url", imageURL.Get());
  if (g_advancedSettings.m_bForceJpegImageFormat)
    mediaUrl.SetOption("format", "jpg");

  mediaUrl.SetFileName("photo/:/transcode");

  //CLog::Log(LOGDEBUG, "CPlexAttributeParserMediaUrl::Process setting %s = %s for item %s", propertyName.c_str(), mediaUrl.Get().c_str(), item.GetLabel().c_str());
  item->SetArt(propertyName, mediaUrl.Get());
}

////////////////////////////////////////////////////////////////////////////////
void CPlexAttributeParserMediaFlag::Process(const CURL &url, const CStdString &key, const CStdString &value, CFileItem *item)
{
  CURL mediaTagUrl;

  mediaTagUrl.SetProtocol("http");
  mediaTagUrl.SetHostName("127.0.0.1");
  mediaTagUrl.SetPort(32400);

  if (!item->HasProperty("mediaTagPrefix"))
  {
    CLog::Log(LOGWARNING, "CPlexAttributeParserMediaFlag::Process got a mediaflag on %s but we don't have any mediaTagPrefix", url.Get().c_str());
    return;
  }

  CStdString mediaTagPrefix = item->GetProperty("mediaTagPrefix").asString();
  CStdString mediaTagVersion = item->GetProperty("mediaTagVersion").asString();

  CStdString flagUrl = mediaTagPrefix;

  flagUrl = PlexUtils::AppendPathToURL(flagUrl, key);
  flagUrl = PlexUtils::AppendPathToURL(flagUrl, CURL::Encode(value));

  if (boost::starts_with(flagUrl, "/"))
    mediaTagUrl.SetFileName(flagUrl.substr(1, std::string::npos));
  else
    mediaTagUrl.SetFileName(flagUrl);

  if (!mediaTagVersion.empty())
    mediaTagUrl.SetOption("t", mediaTagVersion);

  //CLog::Log(LOGDEBUG, "CPlexAttributeParserMediaFlag::Process MEDIATAG: mediaTag::%s = %s | mediaTag-%s = %s", key.c_str(), mediaTagUrl.Get().c_str(), key.c_str(), value.c_str());
  CPlexAttributeParserMediaUrl::Process(url, "mediaTag::" + key, mediaTagUrl.Get(), item);

  /* also store the raw value */
  item->SetProperty("mediaTag-" + key, value);
}

////////////////////////////////////////////////////////////////////////////////
void CPlexAttributeParserType::Process(const CURL &url, const CStdString &key, const CStdString &value, CFileItem *item)
{
  CStdString lookupVal = boost::algorithm::to_lower_copy(std::string(value));
  EPlexDirectoryType dirType = XFILE::CPlexDirectory::GetDirectoryType(lookupVal);

  if (dirType == PLEX_DIR_TYPE_PHOTO)
  {
    if (item->GetProperty("xmlElementName") == "Directory")
    {
      dirType = PLEX_DIR_TYPE_PHOTOALBUM;
      lookupVal = "photoalbum";
    }
  }

  if (dirType != PLEX_DIR_TYPE_UNKNOWN)
  {
    item->SetProperty(key, lookupVal);
    item->SetPlexDirectoryType(dirType);
  }
}

////////////////////////////////////////////////////////////////////////////////
void CPlexAttributeParserLabel::Process(const CURL &url, const CStdString &key, const CStdString &value, CFileItem *item)
{
  item->SetLabel(value);
  item->SetProperty(key, value);
}

////////////////////////////////////////////////////////////////////////////////
void CPlexAttributeParserDateTime::Process(const CURL &url, const CStdString &key, const CStdString &value, CFileItem *item)
{
  CStdString XBMCFormat = value + " 00:00:00";
  CDateTime time;

  time.SetFromDBDate(XBMCFormat);
  if (time.IsValid())
    item->m_dateTime = time;
  else
    CLog::Log(LOGDEBUG, "CPlexAttributeParserDateTime::Process failed to parse %s into something sensible.", XBMCFormat.c_str());

  item->SetProperty(key, value);
}

////////////////////////////////////////////////////////////////////////////////
void CPlexAttributeParserTitleSort::Process(const CURL &url, const CStdString &key, const CStdString &value, CFileItem *item)
{
  item->SetSortLabel(value);
  item->SetProperty(key, value);
}
