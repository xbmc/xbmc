/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <ctime>
#include <string>

#include <sys/stat.h>

#include "PlatformDefs.h" // for __stat64

namespace tinyxml2
{
class XMLElement;
class XMLNode;
} // namespace tinyxml2

class CFileItem;

namespace XFILE
{
  class CDAVCommon
  {
    public:
      static bool ValueWithoutNamespace(const tinyxml2::XMLNode* node, const std::string& value);
      static std::string GetStatusTag(const tinyxml2::XMLElement* element);
      static bool ParseHttpDate(const std::string& date, time_t& result);
      static bool ParseIsoDateTime(const std::string& dateTime, time_t& result);
      // Parses a single <response> element of a PROPFIND multistatus reply into
      // stat data. Returns false when the response has no usable properties.
      static bool ParseDavStatResponse(const tinyxml2::XMLNode* response, struct __stat64& stat);
      // Parses a single <response> element of a depth:1 PROPFIND listing reply
      // into a file item.
      static void ParseDavDirectoryResponse(const tinyxml2::XMLElement* element, CFileItem& item);
  };
}
