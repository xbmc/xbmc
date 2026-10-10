/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "MediaType.h"

#include "ServiceBroker.h"
#include "resources/LocalizeStrings.h"
#include "resources/ResourcesComponent.h"
#include "utils/StringUtils.h"

#include <utility>

static std::map<std::string, CMediaTypes::MediaTypeInfo> fillDefaultMediaTypes()
{
  std::map<std::string, CMediaTypes::MediaTypeInfo> mediaTypes;

  // clang-format off
  mediaTypes.insert(std::make_pair(MediaTypeMusic,            CMediaTypes::MediaTypeInfo(MediaTypeMusic,           MediaTypeMusic,               true,    249)));
  mediaTypes.insert(std::make_pair(MediaTypeArtist,           CMediaTypes::MediaTypeInfo(MediaTypeArtist,          MediaTypeArtist "s",          true,    557)));
  mediaTypes.insert(std::make_pair(MediaTypeAlbum,            CMediaTypes::MediaTypeInfo(MediaTypeAlbum,           MediaTypeAlbum "s",           true,    558)));
  mediaTypes.insert(std::make_pair(MediaTypeSong,             CMediaTypes::MediaTypeInfo(MediaTypeSong,            MediaTypeSong "s",            false,   179)));
  mediaTypes.insert(std::make_pair(MediaTypeVideo,            CMediaTypes::MediaTypeInfo(MediaTypeVideo,           MediaTypeVideo "s",           true,    291)));
  mediaTypes.insert(std::make_pair(MediaTypeVideoCollection,  CMediaTypes::MediaTypeInfo(MediaTypeVideoCollection, MediaTypeVideoCollection "s", true,  20141)));
  mediaTypes.insert(std::make_pair(MediaTypeMusicVideo,       CMediaTypes::MediaTypeInfo(MediaTypeMusicVideo,      MediaTypeMusicVideo "s",      false, 20391)));
  mediaTypes.insert(std::make_pair(MediaTypeMovie,            CMediaTypes::MediaTypeInfo(MediaTypeMovie,           MediaTypeMovie "s",           false, 20338)));
  mediaTypes.insert(std::make_pair(MediaTypeTvShow,           CMediaTypes::MediaTypeInfo(MediaTypeTvShow,          MediaTypeTvShow "s",          true,  36902)));
  mediaTypes.insert(std::make_pair(MediaTypeSeason,           CMediaTypes::MediaTypeInfo(MediaTypeSeason,          MediaTypeSeason "s",          true,  20373)));
  mediaTypes.insert(std::make_pair(MediaTypeEpisode,          CMediaTypes::MediaTypeInfo(MediaTypeEpisode,         MediaTypeEpisode "s",         false, 20359)));
  mediaTypes.insert(std::make_pair(MediaTypeVideoVersion,     CMediaTypes::MediaTypeInfo(MediaTypeVideoVersion,    MediaTypeVideoVersion "s",    false, 40012)));
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

std::string CMediaTypes::GetCapitalLocalization(const MediaType &mediaType)
{
  std::map<std::string, MediaTypeInfo>::const_iterator mediaTypeIt = findMediaType(mediaType);
  if (mediaTypeIt == m_mediaTypes.end() || mediaTypeIt->second.localizationSingularCapital <= 0)
    return "";

  return CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(
      mediaTypeIt->second.localizationSingularCapital);
}
