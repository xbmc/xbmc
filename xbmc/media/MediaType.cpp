/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "MediaType.h"

#include "guilib/LocalizeStrings.h"
#include "utils/StringUtils.h"

#include <utility>

static std::map<std::string, CMediaTypes::MediaTypeInfo> fillDefaultMediaTypes()
{
  std::map<std::string, CMediaTypes::MediaTypeInfo> mediaTypes;

  // clang-format off
  mediaTypes.insert(std::make_pair(MediaTypeMusic,            CMediaTypes::MediaTypeInfo(MediaTypeMusic,           MediaTypeMusic,               true,  36914, 36915,   249,   249)));
  mediaTypes.insert(std::make_pair(MediaTypeArtist,           CMediaTypes::MediaTypeInfo(MediaTypeArtist,          MediaTypeArtist "s",          true,  36916, 36917,   557,   133)));
  mediaTypes.insert(std::make_pair(MediaTypeAlbum,            CMediaTypes::MediaTypeInfo(MediaTypeAlbum,           MediaTypeAlbum "s",           true,  36918, 36919,   558,   132)));
  mediaTypes.insert(std::make_pair(MediaTypeSong,             CMediaTypes::MediaTypeInfo(MediaTypeSong,            MediaTypeSong "s",            false, 36920, 36921,   179,   134)));
  mediaTypes.insert(std::make_pair(MediaTypeVideo,            CMediaTypes::MediaTypeInfo(MediaTypeVideo,           MediaTypeVideo "s",           true,  36912, 36913,   291,     3)));
  mediaTypes.insert(std::make_pair(MediaTypeVideoCollection,  CMediaTypes::MediaTypeInfo(MediaTypeVideoCollection, MediaTypeVideoCollection "s", true,  36910, 36911, 20141, 20434)));
  mediaTypes.insert(std::make_pair(MediaTypeMusicVideo,       CMediaTypes::MediaTypeInfo(MediaTypeMusicVideo,      MediaTypeMusicVideo "s",      false, 36908, 36909, 20391, 20389)));
  mediaTypes.insert(std::make_pair(MediaTypeMovie,            CMediaTypes::MediaTypeInfo(MediaTypeMovie,           MediaTypeMovie "s",           false, 36900, 36901, 20338, 20342)));
  mediaTypes.insert(std::make_pair(MediaTypeTvShow,           CMediaTypes::MediaTypeInfo(MediaTypeTvShow,          MediaTypeTvShow "s",          true,  36902, 36903, 36902, 36903)));
  mediaTypes.insert(std::make_pair(MediaTypeSeason,           CMediaTypes::MediaTypeInfo(MediaTypeSeason,          MediaTypeSeason "s",          true,  36904, 36905, 20373, 33054)));
  mediaTypes.insert(std::make_pair(MediaTypeEpisode,          CMediaTypes::MediaTypeInfo(MediaTypeEpisode,         MediaTypeEpisode "s",         false, 36906, 36907, 20359, 20360)));
  mediaTypes.insert(std::make_pair(MediaTypeVideoVersion,     CMediaTypes::MediaTypeInfo(MediaTypeVideoVersion,    MediaTypeVideoVersion "s",    false, 40010, 40011, 40012, 40013)));
  // clang-format on

  return mediaTypes;
}

std::map<std::string, CMediaTypes::MediaTypeInfo> CMediaTypes::m_mediaTypes = fillDefaultMediaTypes();

bool CMediaTypes::IsValidMediaType(const MediaType &mediaType)
{
  return findMediaType(mediaType) != m_mediaTypes.end();
}

bool CMediaTypes::IsMediaType(const std::string &strMediaType, const MediaType &mediaType)
{
  std::map<std::string, MediaTypeInfo>::const_iterator strMediaTypeIt = findMediaType(strMediaType);
  std::map<std::string, MediaTypeInfo>::const_iterator mediaTypeIt = findMediaType(mediaType);

  return strMediaTypeIt != m_mediaTypes.end() && mediaTypeIt != m_mediaTypes.end() &&
         strMediaTypeIt->first.compare(mediaTypeIt->first) == 0;
}

MediaType CMediaTypes::FromString(const std::string &strMediaType)
{
  std::map<std::string, MediaTypeInfo>::const_iterator mediaTypeIt = findMediaType(strMediaType);
  if (mediaTypeIt == m_mediaTypes.end())
    return MediaTypeNone;

  return mediaTypeIt->first;
}

MediaType CMediaTypes::ToPlural(const MediaType &mediaType)
{
  std::map<std::string, MediaTypeInfo>::const_iterator mediaTypeIt = findMediaType(mediaType);
  if (mediaTypeIt == m_mediaTypes.end())
    return MediaTypeNone;

  return mediaTypeIt->second.plural;
}

bool CMediaTypes::IsContainer(const MediaType &mediaType)
{
  std::map<std::string, MediaTypeInfo>::const_iterator mediaTypeIt = findMediaType(mediaType);
  if (mediaTypeIt == m_mediaTypes.end())
    return false;

  return mediaTypeIt->second.container;
}

std::map<std::string, CMediaTypes::MediaTypeInfo>::const_iterator CMediaTypes::findMediaType(const std::string &mediaType)
{
  std::string strMediaType = mediaType;
  StringUtils::ToLower(strMediaType);

  std::map<std::string, MediaTypeInfo>::const_iterator it = m_mediaTypes.find(strMediaType);
  if (it != m_mediaTypes.end())
    return it;

  for (it = m_mediaTypes.begin(); it != m_mediaTypes.end(); ++it)
  {
    if (strMediaType.compare(it->second.plural) == 0)
      return it;
  }

  return m_mediaTypes.end();
}

std::string CMediaTypes::GetLocalization(const MediaType &mediaType)
{
  std::map<std::string, MediaTypeInfo>::const_iterator mediaTypeIt = findMediaType(mediaType);
  if (mediaTypeIt == m_mediaTypes.end() ||
    mediaTypeIt->second.localizationSingular <= 0)
    return "";

  return g_localizeStrings.Get(mediaTypeIt->second.localizationSingular);
}

std::string CMediaTypes::GetPluralLocalization(const MediaType &mediaType)
{
  std::map<std::string, MediaTypeInfo>::const_iterator mediaTypeIt = findMediaType(mediaType);
  if (mediaTypeIt == m_mediaTypes.end() ||
    mediaTypeIt->second.localizationPlural <= 0)
    return "";

  return g_localizeStrings.Get(mediaTypeIt->second.localizationPlural);
}

std::string CMediaTypes::GetCapitalLocalization(const MediaType &mediaType)
{
  std::map<std::string, MediaTypeInfo>::const_iterator mediaTypeIt = findMediaType(mediaType);
  if (mediaTypeIt == m_mediaTypes.end() ||
    mediaTypeIt->second.localizationSingular <= 0)
    return "";

  return g_localizeStrings.Get(mediaTypeIt->second.localizationSingularCapital);
}

std::string CMediaTypes::GetCapitalPluralLocalization(const MediaType &mediaType)
{
  std::map<std::string, MediaTypeInfo>::const_iterator mediaTypeIt = findMediaType(mediaType);
  if (mediaTypeIt == m_mediaTypes.end() ||
    mediaTypeIt->second.localizationPlural <= 0)
    return "";

  return g_localizeStrings.Get(mediaTypeIt->second.localizationPluralCapital);
}
