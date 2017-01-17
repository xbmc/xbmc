#pragma once
/*
 *      Copyright (C) 2005-2017 Team Kodi
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "addons/IAddon.h"
#include "addons/AddonVersion.h"

namespace ADDON
{

  class AddonProps
  {
  public:
    AddonProps();
    AddonProps(std::string id, TYPE type);

    std::string id;
    TYPE type;
    AddonVersion version{"0.0.0"};
    AddonVersion minversion{"0.0.0"};
    std::string name;
    std::string license;
    std::string summary;
    std::string description;
    std::string libname;
    std::string author;
    std::string source;
    std::string path;
    std::string icon;
    std::string changelog;
    std::string fanart;
    std::vector<std::string> screenshots;
    std::string disclaimer;
    ADDONDEPS dependencies;
    std::string broken;
    InfoMap extrainfo;
    CDateTime installDate;
    CDateTime lastUpdated;
    CDateTime lastUsed;
    std::string origin;
    uint64_t packageSize;
  };

} /* namespace ADDON */
