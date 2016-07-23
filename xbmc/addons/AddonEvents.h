/*
 *      Copyright (C) 2016 Team Kodi
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
#pragma once

#include <string>

namespace ADDON
{
  struct AddonEvent
  {
    virtual ~AddonEvent() {};
  };

  namespace AddonEvents
  {
    struct Enabled : AddonEvent
    {
      std::string id;
      Enabled(std::string id) : id(std::move(id)) {}
    };

    struct Disabled : AddonEvent
    {
      std::string id;
      Disabled(std::string id) : id(std::move(id)) {}
    };

    struct MetadataChanged : AddonEvent
    {
      std::string id;
      MetadataChanged(std::string id) : id(std::move(id)) {}
    };

    struct InstalledChanged : AddonEvent {};
  };
};
