/*
 *  Copyright (C) 2005-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "BeyondTVParser.h"

#include "FileItem.h"
#include "utils/URIUtils.h"
#include "utils/XBMCTinyXML2.h"
#include "utils/log.h"

using namespace EDL;

std::string CBeyondTVParser::GetEdlFilePath(const CFileItem& item) const
{
  const std::string& mediaFilePath = item.GetDynPath();
  return URIUtils::ReplaceExtension(mediaFilePath,
                                    URIUtils::GetExtension(mediaFilePath) + ".chapters.xml");
}

CEdlParserResult CBeyondTVParser::Parse(const CFileItem& item, float fps)
{
  CEdlParserResult result;

  const std::string beyondTVFilename = GetEdlFilePath(item);

  CXBMCTinyXML2 xmlDoc;
  if (!xmlDoc.LoadFile(beyondTVFilename))
  {
    CLog::LogF(LOGERROR, "Could not load Beyond TV file: {}. {}",
               CURL::GetRedacted(beyondTVFilename), xmlDoc.ErrorStr());
    return result;
  }

  if (xmlDoc.Error())
  {
    CLog::LogF(LOGERROR, "Could not parse Beyond TV file: {}. {}",
               CURL::GetRedacted(beyondTVFilename), xmlDoc.ErrorStr());
    return result;
  }

  const tinyxml2::XMLElement* root = xmlDoc.RootElement();
  if (!root || strcmp(root->Value(), "cutlist"))
  {
    CLog::LogF(LOGERROR, "Invalid Beyond TV file: {}. Expected root node to be <cutlist>",
               CURL::GetRedacted(beyondTVFilename));
    return result;
  }

  bool valid = true;
  int regionIndex = 0;
  const tinyxml2::XMLElement* region = root->FirstChildElement("Region");
  while (valid && region)
  {
    regionIndex++;
    const tinyxml2::XMLElement* start = region->FirstChildElement("start");
    const tinyxml2::XMLElement* end = region->FirstChildElement("end");
    if (start && end && start->FirstChild() && end->FirstChild())
    {
      /*
       * Need to divide the start and end times by a factor of 10,000 to get msec.
       * E.g. <start comment="00:02:44.9980867">1649980867</start>
       *
       * Use atof so doesn't overflow 32 bit float or integer / long.
       * E.g. <end comment="0:26:49.0000009">16090090000</end>
       *
       * Don't use atoll even though it is more correct as it isn't natively supported by
       * Visual Studio.
       *
       * atof() returns 0 if there were any problems and will subsequently be rejected in AddEdit().
       */
      Edit edit;
      edit.start =
          std::chrono::milliseconds(std::lround((std::atof(start->FirstChild()->Value()) / 10000)));
      edit.end =
          std::chrono::milliseconds(std::lround((std::atof(end->FirstChild()->Value()) / 10000)));
      edit.action = Action::COMM_BREAK;
      result.AddEdit(edit, EdlSourceLocation{beyondTVFilename, regionIndex});
    }
    else
      valid = false;

    region = region->NextSiblingElement("Region");
  }

  if (!valid)
  {
    CLog::LogF(LOGERROR, "Invalid Beyond TV file: {}. Clearing any valid commercial breaks found.",
               CURL::GetRedacted(beyondTVFilename));
    return {};
  }
  else if (!result.GetEdits().empty())
  {
    CLog::LogF(LOGDEBUG, "Read {} commercial breaks from Beyond TV file: {}",
               result.GetEdits().size(), CURL::GetRedacted(beyondTVFilename));
  }
  else
  {
    CLog::LogF(LOGDEBUG, "No commercial breaks found in Beyond TV file: {}",
               CURL::GetRedacted(beyondTVFilename));
  }

  return result;
}
