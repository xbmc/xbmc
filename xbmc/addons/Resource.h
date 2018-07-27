/*
 *  Copyright (C) 2014-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
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
