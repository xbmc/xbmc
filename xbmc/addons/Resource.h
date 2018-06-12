/*
 *      Copyright (C) 2014 Team XBMC
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

#include <memory>

#include "addons/Addon.h"
#include "utils/URIUtils.h"

namespace ADDON
{

class CResource : public CAddon
{
public:
  ~CResource() override = default;

  virtual bool IsAllowed(const std::string &file) const = 0;

  virtual std::string GetFullPath(const std::string &filePath) const
  {
    return URIUtils::AddFileToFolder(GetResourcePath(), filePath);
  }

protected:
  explicit CResource(CAddonInfo addonInfo) : CAddon(std::move(addonInfo)) {}

  std::string GetResourcePath() const
  {
    return URIUtils::AddFileToFolder(Path(), "resources");
  }
};

}
