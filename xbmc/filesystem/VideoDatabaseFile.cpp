/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "VideoDatabaseFile.h"
#include "URL.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"

using namespace XFILE;

CVideoDatabaseFile::CVideoDatabaseFile(void)
  : COverrideFile(true)
{ }

CVideoDatabaseFile::~CVideoDatabaseFile(void) = default;

CVideoInfoTag CVideoDatabaseFile::GetVideoTag(const CURL& url)
{
  CVideoInfoTag tag;

  std::string strFileName = URIUtils::GetFileName(url.Get());
  if (strFileName.empty())
    return tag;

  URIUtils::RemoveExtension(strFileName);
  if (!StringUtils::IsNaturalNumber(strFileName))
    return tag;
  long idDb = atol(strFileName.c_str());

  VIDEODB_CONTENT_TYPE type = GetType(url);
  if (type == VIDEODB_CONTENT_UNKNOWN)
    return tag;

  CVideoDatabase videoDatabase;
  if (!videoDatabase.Open())
    return tag;

  tag = videoDatabase.GetDetailsByTypeAndId(type, idDb);

  return tag;
}

VIDEODB_CONTENT_TYPE CVideoDatabaseFile::GetType(const CURL& url)
{
  std::string strPath = URIUtils::GetDirectory(url.Get());
  if (strPath.empty())
    return VIDEODB_CONTENT_UNKNOWN;

  std::vector<std::string> pathElem = StringUtils::Split(strPath, "/");
  if (pathElem.size() == 0)
    return VIDEODB_CONTENT_UNKNOWN;

  std::string itemType = pathElem.at(2);
  VIDEODB_CONTENT_TYPE type;
  if (itemType == "movies" || itemType == "recentlyaddedmovies")
    type = VIDEODB_CONTENT_MOVIES;
  else if (itemType == "episodes" || itemType == "recentlyaddedepisodes" || itemType == "inprogresstvshows" || itemType == "tvshows")
    type = VIDEODB_CONTENT_EPISODES;
  else if (itemType == "musicvideos" || itemType == "recentlyaddedmusicvideos")
    type = VIDEODB_CONTENT_MUSICVIDEOS;
  else
    type = VIDEODB_CONTENT_UNKNOWN;

  return type;
}


std::string CVideoDatabaseFile::TranslatePath(const CURL& url)
{
  std::string strFileName = URIUtils::GetFileName(url.Get());
  if (strFileName.empty())
    return "";

  URIUtils::RemoveExtension(strFileName);
  if (!StringUtils::IsNaturalNumber(strFileName))
    return "";
  long idDb = atol(strFileName.c_str());

  VIDEODB_CONTENT_TYPE type = GetType(url);
  if (type == VIDEODB_CONTENT_UNKNOWN)
    return "";

  CVideoDatabase videoDatabase;
  if (!videoDatabase.Open())
    return "";

  std::string realFilename;
  videoDatabase.GetFilePathById(idDb, realFilename, type);

  return realFilename;
}
