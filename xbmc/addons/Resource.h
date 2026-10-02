/*
 *  Copyright (C) 2014-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "addons/Addon.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"

#include <algorithm>
#include <memory>
#include <span>
#include <string>
#include <string_view>

namespace ADDON
{

class CResource : public CAddon
{
public:
  ~CResource() override = default;

  /*!
   * \brief Whether a path below this add-on's resources directory resolves to something it
   *        publishes.
   *
   * One rule for every resource type. The resources directory itself and the directories under
   * it always resolve, so any type can be browsed. A type that publishes no set resolves every
   * file; every other type resolves the file names and the extensions it publishes, and nothing
   * else.
   *
   * \param[in] file The path, relative to that directory. Empty names the directory itself.
   * \return True where the path resolves.
   */
  bool CanResolve(const std::string& file) const
  {
    if (file.empty() || URIUtils::HasSlashAtEnd(file, true))
      return true;

    const Published published{PublishedFiles()};
    if (published.names.empty() && published.extensions.empty())
      return true;

    if (std::ranges::any_of(published.names, [&file](std::string_view name)
                            { return StringUtils::EqualsNoCase(file, name); }))
    {
      return true;
    }

    const std::string extension{URIUtils::GetExtension(file)};
    return std::ranges::any_of(published.extensions, [&extension](std::string_view candidate)
                               { return StringUtils::EqualsNoCase(extension, candidate); });
  }

  virtual std::string GetFullPath(const std::string &filePath) const
  {
    return URIUtils::AddFileToFolder(GetResourcePath(), filePath);
  }

protected:
  //! The files a resource type publishes. Both sets empty publishes every file.
  struct Published
  {
    std::span<const std::string_view> names; //!< exact file names
    std::span<const std::string_view> extensions; //!< extensions, leading dot included
  };

  explicit CResource(const AddonInfoPtr& addonInfo, AddonType addonType)
    : CAddon(addonInfo, addonType)
  {
  }

  //! \brief What this resource type publishes, as CanResolve describes.
  virtual Published PublishedFiles() const { return {}; }

  std::string GetResourcePath() const
  {
    return URIUtils::AddFileToFolder(Path(), "resources");
  }
};

}
