/*
 *  Copyright (C) 2005-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PVRCachedImages.h"

#include "ServiceBroker.h"
#include "TextureCache.h"
#include "TextureDatabase.h"
#include "URL.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"
#include "utils/log.h"

using namespace PVR;

int CPVRCachedImages::Cleanup(const std::vector<PVRImagePattern>& urlPatterns,
                              const std::vector<std::string>& urlsToCheck,
                              bool clearTextureForPath /* = false */)
{
  int iCleanedImages = 0;

  if (urlPatterns.empty())
  {
    CLog::LogFC(LOGERROR, LOGPVR, "No URL patterns given");
    return iCleanedImages;
  }

  CTextureDatabase db;
  if (!db.Open())
  {
    CLog::LogFC(LOGERROR, LOGPVR, "Failed to open texture database");
    return iCleanedImages;
  }

  CDatabase::Filter filter;

  for (const auto& pattern : urlPatterns)
  {
    const std::string encodedPattern =
        StringUtils::Format("{}@{}", pattern.owner, CURL::Encode(pattern.path));

    std::string escapedPattern;
    for (size_t i = 0; i < encodedPattern.size(); ++i)
    {
      if (encodedPattern[i] == '%' || encodedPattern[i] == '^')
        escapedPattern += '^';

      escapedPattern += encodedPattern[i];
    }

    const std::string where =
        StringUtils::Format("url LIKE 'image://{}%' ESCAPE '^'", escapedPattern);
    filter.AppendWhere(where, false); // logical OR
  }

  CVariant items;
  if (!db.GetTextures(items, filter))
  {
    CLog::LogFC(LOGERROR, LOGPVR, "Failed to get items from texture database");
    return iCleanedImages;
  }

  for (unsigned int i = 0; i < items.size(); ++i)
  {
    // Unwrap the image:// URL returned from texture db.
    const std::string textureURL = UnwrapImageURL(items[i]["url"].asString());

    bool bFound = false;
    for (const auto& url : urlsToCheck)
    {
      if (url == textureURL)
      {
        bFound = true;
        break; // next item
      }
    }

    if (!bFound)
    {
      CLog::LogFC(LOGDEBUG, LOGPVR, "Removing stale cached image: '{}'", textureURL);
      CServiceBroker::GetTextureCache()->ClearCachedImage(items[i]["textureid"].asInteger());

      if (clearTextureForPath)
        db.ClearTextureForPath(textureURL, "thumb");

      iCleanedImages++;
    }
  }

  return iCleanedImages;
}

std::string CPVRCachedImages::UnwrapImageURL(const std::string& url)
{
  return StringUtils::StartsWith(url, "image://") ? CURL(url).GetHostName() : url;
}
