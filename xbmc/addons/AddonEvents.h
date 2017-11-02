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
    std::string id;
    explicit AddonEvent(std::string id) : id(std::move(id)) {};
    virtual ~AddonEvent() = default;
  };

  namespace AddonEvents
  {
    /**
     * Emitted after the add-on has been enabled.
     */
    struct Enabled : AddonEvent
    {
      explicit Enabled(std::string id) : AddonEvent(std::move(id)) {}
    };

    /**
     * Emitted after the add-on has been disabled.
     */
    struct Disabled : AddonEvent
    {
      explicit Disabled(std::string id) : AddonEvent(std::move(id)) {}
    };

    /**
     * Emitted after the add-on's metadata has been changed.
     */
    struct MetadataChanged : AddonEvent
    {
      explicit MetadataChanged(std::string id) : AddonEvent(std::move(id)) {}
    };

    /**
     * Emitted when a different version of the add-on has been installed
     * to the file system and should be reloaded.
     */
    struct ReInstalled: AddonEvent
    {
      explicit ReInstalled(std::string id) : AddonEvent(std::move(id)) {}
    };

    /**
     * Emitted after the add-on has been uninstalled.
     */
    struct UnInstalled : AddonEvent
    {
      explicit UnInstalled(std::string id) : AddonEvent(std::move(id)) {}
    };

    /**
     * Emitted after the add-on has been loaded.
     */
    struct Load : AddonEvent
    {
      explicit Load(std::string id) : AddonEvent(std::move(id)) {}
    };

    /**
     * Emitted after the add-on has been unloaded.
     */
    struct Unload : AddonEvent
    {
      explicit Unload(std::string id) : AddonEvent(std::move(id)) {}
    };
  }
}
