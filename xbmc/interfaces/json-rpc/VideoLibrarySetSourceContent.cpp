/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "VideoLibrarySetSourceContent.h"

#include "utils/Variant.h"

#include <climits>
#include <string>

namespace JSONRPC
{

namespace
{
bool ParseContent(const std::string& value, ADDON::ContentType& content)
{
  content = ADDON::TranslateContent(value);

  return value == "none" || content == ADDON::ContentType::MOVIES ||
         content == ADDON::ContentType::TVSHOWS || content == ADDON::ContentType::MUSICVIDEOS;
}

bool ParseClearMode(const std::string& value, SourceContentClearMode& mode)
{
  if (value == "clear")
  {
    mode = SourceContentClearMode::CLEAR;
  }
  else if (value == "exclude")
  {
    mode = SourceContentClearMode::EXCLUDE;
  }
  else if (value == "remove")
  {
    mode = SourceContentClearMode::REMOVE;
  }
  else
  {
    return false;
  }

  return true;
}
} // unnamed namespace

JSONRPC_STATUS ParseSetSourceContentParams(const CVariant& parameterObject,
                                           ParsedSetSourceContent& parsed)
{
  parsed.path = parameterObject["path"].asString();
  if (parsed.path.empty())
  {
    return InvalidParams;
  }

  if (!ParseContent(parameterObject["content"].asString(), parsed.content))
  {
    return InvalidParams;
  }

  parsed.refresh = parameterObject["refresh"].asBoolean(false);

  if (parsed.content == ADDON::ContentType::NONE)
  {
    if (!ParseClearMode(parameterObject["clearmode"].asString("clear"), parsed.clearMode))
    {
      return InvalidParams;
    }

    parsed.settings.exclude = parsed.clearMode == SourceContentClearMode::EXCLUDE;
    return OK;
  }

  parsed.scraperId = parameterObject["scraperid"].asString();
  if (parsed.scraperId.empty())
  {
    return InvalidParams;
  }

  parsed.scraperSettings = parameterObject["scrapersettings"].asString();

  const bool scanRecursive = parameterObject["scanrecursive"].asBoolean(true);
  const bool useDirectoryNames = parameterObject["usedirectorynames"].asBoolean(false);
  const bool containsSingleItem = parameterObject["containssingleitem"].asBoolean(false);

  parsed.settings.exclude = false;
  parsed.settings.noupdate = parameterObject["noupdate"].asBoolean(false);

  if (parsed.content == ADDON::ContentType::TVSHOWS)
  {
    parsed.settings.parent_name = parsed.settings.parent_name_root = containsSingleItem;
    parsed.settings.recurse = 0;
  }
  else if (useDirectoryNames)
  {
    parsed.settings.parent_name = true;
    parsed.settings.parent_name_root = false;
    parsed.settings.recurse = scanRecursive ? INT_MAX : 1;

    if (containsSingleItem)
    {
      parsed.settings.parent_name_root = true;
      parsed.settings.recurse = 0;
    }
  }
  else
  {
    parsed.settings.parent_name = false;
    parsed.settings.parent_name_root = false;
    parsed.settings.recurse = scanRecursive ? INT_MAX : 0;
  }

  return OK;
}

} // namespace JSONRPC
