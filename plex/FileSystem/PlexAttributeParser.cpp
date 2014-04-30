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

///////////////////////////////////////////////////////////////////////////////////////////////////
CStdString CPlexAttributeParserMediaUrl::GetImageURL(const CURL &url, const CStdString &source, int height, int width)
{
  CURL mediaUrl(url);
  CURL imageURL;
  CUrlOptions options;
  CPlexServerPtr server;

  mediaUrl.SetOptions("");

  if (g_plexApplication.serverManager)
    server = g_plexApplication.serverManager->FindByUUID(mediaUrl.GetHostName());

  if ((mediaUrl.GetHostName() == "myplex" || mediaUrl.GetHostName() == "node") ||
      (server && (server->GetSynced() || !server->GetServerClass().empty())))
  {
    CPlexServerPtr bestServer;
    if (g_plexApplication.serverManager)
      bestServer = g_plexApplication.serverManager->GetBestServer();

    if (bestServer)
      mediaUrl.SetHostName(bestServer->GetUUID());
    else
      /* if we don't have a best server we use myPlex even for node content */
      mediaUrl.SetHostName("myplex");
  }

  if (boost::starts_with(source, "http://") || boost::starts_with(source, "https://"))
  {
    imageURL = CURL(source);
  }
  else
  {
    imageURL.SetProtocol("http");
    imageURL.SetHostName("127.0.0.1");
    imageURL.SetPort(32400);

    // Now check if we have a local connection that might override the port
    // this is especially true for secondary servers
    if (server)
    {
      CPlexConnectionPtr localConn = server->GetLocalConnection();
      if (localConn)
        imageURL.SetPort(localConn->GetAddress().GetPort());
    }

    if (boost::starts_with(source, "/"))
      imageURL.SetFileName(source.substr(1, std::string::npos));
    else
      imageURL.SetFileName(source);
  }

  CStdString swidth = "320", sheight = "320";

  try
  {
    swidth = boost::lexical_cast<CStdString>(width);
    sheight = boost::lexical_cast<CStdString>(height);
  }
  catch (boost::bad_lexical_cast)
  {
    CLog::Log(LOGWARNING, "CPlexAttributeParser::GetImageURL Could not convert width or height to a string");
  }

  options.AddOption("width", swidth);
  options.AddOption("height", sheight);
  options.AddOption("url", imageURL.Get());
  if (g_advancedSettings.m_bForceJpegImageFormat)
    options.AddOption("format", "jpg");

  mediaUrl.AddOptions(options);

  mediaUrl.SetFileName("photo/:/transcode");

  return mediaUrl.Get();
}

#define SMALL_SIZE 320
#define MEDIUM_SIZE 720
#define LARGE_SIZE 2048

////////////////////////////////////////////////////////////////////////////////
void CPlexAttributeParserMediaUrl::Process(const CURL &url, const CStdString &key, const CStdString &value, CFileItem *item)
{
  if (key == "thumb")
  {
    item->SetArt("smallThumb", GetImageURL(url, value, SMALL_SIZE, SMALL_SIZE));
    item->SetArt("thumb", GetImageURL(url, value, MEDIUM_SIZE, MEDIUM_SIZE));
    item->SetArt("bigThumb", GetImageURL(url, value, LARGE_SIZE, LARGE_SIZE));
  }
  else if (key == "poster")
  {
    item->SetArt("smallPoster", GetImageURL(url, value, SMALL_SIZE, SMALL_SIZE));
    item->SetArt("poster", GetImageURL(url, value, MEDIUM_SIZE, MEDIUM_SIZE));
    item->SetArt("bigPoster", GetImageURL(url, value, LARGE_SIZE, LARGE_SIZE));
  }
  else if (key == "grandparentThumb")
  {
    item->SetArt("smallGrandparentThumb", GetImageURL(url, value, SMALL_SIZE, SMALL_SIZE));
    item->SetArt("grandparentThumb", GetImageURL(url, value, MEDIUM_SIZE, MEDIUM_SIZE));
    item->SetArt(PLEX_ART_TVSHOW_THUMB, GetImageURL(url, value, MEDIUM_SIZE, MEDIUM_SIZE));
    item->SetArt("bigGrandparentThumb", GetImageURL(url, value, LARGE_SIZE, LARGE_SIZE));
  }
  else if (key == "banner")
    item->SetArt("banner", GetImageURL(url, value, 200, 800));
  else if (key == "art")
    item->SetArt(PLEX_ART_FANART, GetImageURL(url, value, LARGE_SIZE, LARGE_SIZE));
  else if (key == "picture")
    item->SetArt("picture", GetImageURL(url, value, LARGE_SIZE, LARGE_SIZE));
  else
    item->SetArt(key, GetImageURL(url, value, 320, 320));
}

////////////////////////////////////////////////////////////////////////////////
void CPlexAttributeParserMediaFlag::Process(const CURL &url, const CStdString &key, const CStdString &value, CFileItem *item)
{
  static std::map<std::string, std::string> FlagsMap;
  static CCriticalSection FlagsMapSection;
  CSingleLock Lock(FlagsMapSection);

  std::map<std::string, std::string>::const_iterator got = FlagsMap.find(key + "|" + value);
  if ((got != FlagsMap.end()) && true)
  {
    item->SetArt("mediaTag::" + key, got->second);
    item->SetProperty("mediaTag-" + key, value);
    //CLog::Log(LOGDEBUG, "CPlexAttributeParserMediaFlag::Process MEDIATAG (CACHED): mediaTag::%s = %s | mediaTag-%s = %s", key.c_str(), got->second.c_str(), key.c_str(), value.c_str());
  }
  else
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
    FlagsMap[key + "|" + value] = item->GetArt("mediaTag::" + key);
  }
}

////////////////////////////////////////////////////////////////////////////////
void CPlexAttributeParserType::Process(const CURL &url, const CStdString &key, const CStdString &value, CFileItem *item)
{
  CStdString lookupVal = boost::algorithm::to_lower_copy(std::string(value));

  // This is a hack to appease Sebastian :)
  // basically it's refered to as song everywhere but from the server it's called
  // track. And we can't make the typeMap map two values, so we'll just hack it for
  // now.
  if (lookupVal == "track")
    lookupVal = "song";

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
